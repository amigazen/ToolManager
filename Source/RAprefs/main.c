/*
 * main.c  RAprefs
 *
 * RAprefs (ReAction) configuration program main entry point.
 * Fork of ToolManager prefs using Reaction GUI for the main window.
 */

#include "RAprefsConf.h"

static const char Version[]="\0$VER: RAprefs " TMVERSION "." TMREVISION
                            " " __AMIGADATE__ "\r\n";
static const char PrefsPortName[]="ToolManager_Prefs";

/* Misc. data */
struct Library *IFFParseBase=NULL;
struct Library *UtilityBase=NULL;
static struct MsgPort *PrefsPort=NULL;
static char *FromFileName;
static char *PubScreenName;
static char Template[]="FROM,EDIT/S,USE/S,SAVE/S,PUBSCREEN/K,DEFAULTFONT/S";
static struct {
               char *from;
               long  edit;
               long  use;
               long  save;
               char *pubsc;
               long  deffont;
              } cmdlineparams={PrefsFileName,TRUE,FALSE,FALSE,NULL,FALSE};
static UWORD MainWinX=10;
static UWORD MainWinY=10;
static BOOL DefaultFont;
static char WBName[]="Workbench";
static char YesString[]="Y|Yes";

/* Open resources */
static BOOL OpenResources(void)
{
 if (!(UtilityBase=OpenLibrary("utility.library",47))) return(FALSE);
 if (!(IFFParseBase=OpenLibrary("iffparse.library",37))) return(FALSE);
 if (!(PrefsPort=CreateMsgPort())) return(FALSE);
 PrefsPort->mp_Node.ln_Pri=-127;
 PrefsPort->mp_Node.ln_Name=(char *)PrefsPortName;
 AddPort(PrefsPort);

 /* Try to open workbench.library */
 WorkbenchBase=OpenLibrary("workbench.library",37);

 if (!(AppMsgPort=CreateMsgPort())) return(FALSE);
 GetLocale();

 /* All OK */
 return(TRUE);
}

/* Close Resources */
static void CloseResources(void)
{
 FreeLocale();
 if (AppMsgPort) {
  struct Message *msg;
  while (msg=GetMsg(AppMsgPort)) ReplyMsg(msg);
  DeleteMsgPort(AppMsgPort);
 }
 if (WorkbenchBase) CloseLibrary(WorkbenchBase);
 if (PrefsPort) {
  RemPort(PrefsPort);
  DeleteMsgPort(PrefsPort);
 }
 if (IFFParseBase) CloseLibrary(IFFParseBase);
 if (UtilityBase) CloseLibrary(UtilityBase);
}

/* Free all preferences objects */
void FreeAllObjects(void)
{
 int i;

 for (i=0; i<TMOBJTYPES; i++) {
  struct List *list=&ObjectLists[i];
  FreeNodeFuncPtr freefunc=FreeNodeFunctions[i];
  struct Node *node;

  /* Scan list and free nodes */
  while (node=RemHead(list)) (*freefunc)(node);
 }
}

/* Copy a file */
BOOL CopyFile(char *source, char *dest)
{
 char *copybuf;
 BOOL rc=FALSE;

 /* Allocate copy buffer */
 if (copybuf=malloc(2048)) {
  BPTR infh;

  /* Open source file */
  if (infh=Open(source,MODE_OLDFILE)) {
   BPTR outfh;

   /* Open destination file */
   if (outfh=Open(dest,MODE_NEWFILE)) {
    LONG n;

    /* Copy file */
    while ((n=Read(infh,copybuf,2048))>0) Write(outfh,copybuf,n);

    /* No error? */
    if (n==0) rc=TRUE;

    Close(outfh);
   }
   Close(infh);
  }
  free(copybuf);
 }
 return(rc);
}

/* Main program */
static int mainprogram(struct WBArg *wa)
{
 (void)&Version;
 /* Another ToolManager Preferences program already running? */
 Forbid();
 if (PrefsPort=FindPort(PrefsPortName)) {
  /* Yes, signal other task... */
  Signal(PrefsPort->mp_SigTask,1L << PrefsPort->mp_SigBit);
  Permit();

  /* ...and exit! */
  return(0);
 }
 Permit();

 /* Open resources */
 if (OpenResources()) {
  /* Check OS version (Library.lib_Version; cast for strict compilers) */
  if (((struct Library *)SysBase)->lib_Version>=39)
   OSV39=TRUE;

  /* Set defaults */
  FromFileName=(char *)PrefsFileName;
  PubScreenName=NULL;
  ProgramName=NULL;
  DefaultFont=FALSE;

  /* WB or CLI startup? */
  if (wa) {
   /* WB Startup */
   struct DiskObject *dobj;

   /* Get icon */
   if (dobj=GetDiskObjectNew(wa->wa_Name)) {
    char *s,**tt=dobj->do_ToolTypes;

    /* USE? */
    if (FindToolType(tt,"USE")) {
     /* Copy From file to preferences file */
     CopyFile(wa->wa_Name,(char *)PrefsFileName);

     /* ...and exit */
     FreeDiskObject(dobj);
     CloseResources();
     return(0);
    }

    /* SAVE? */
    if (FindToolType(tt,"SAVE")) {
     /* Copy From file to preferences file */
     CopyFile(wa->wa_Name,(char *)SavePrefsFileName);

     /* ...and exit */
     FreeDiskObject(dobj);
     CloseResources();
     return(0);
    }

    /* PUBSCREEN */
    if (s=FindToolType(tt,"PUBSCREEN")) PubScreenName=strdup(s);

    /* DEFAULTFONT */
    if (s=FindToolType(tt,"DEFAULTFONT"))
     DefaultFont=MatchToolValue(YesString,s);

    /* CREATEICONS */
    if (s=FindToolType(tt,"CREATEICONS"))
     CreateIcons=MatchToolValue(YesString,s);

    /* XPOS */
    if (s=FindToolType(tt,"XPOS"))
     MainWinX=strtol(s,NULL,10);

    /* YPOS */
    if (s=FindToolType(tt,"YPOS"))
     MainWinY=strtol(s,NULL,10);

    /* LISTCOLUMNS */
    if (s=FindToolType(tt,"MINLISTCOLUMNS"))
     if ((ListViewColumns=strtol(s,NULL,10))<20)
      ListViewColumns=20;

    /* LISTROWS */
    if (s=FindToolType(tt,"MINLISTROWS"))
     if ((ListViewRows=strtol(s,NULL,10))<8)
      ListViewRows=8;

    /* Get program & prefs file name. Icon type? */
    switch (dobj->do_Type) {
     case WBTOOL:    if (ProgramName=malloc(256)) {
                      *ProgramName='\0';
                      if (NameFromLock(wa->wa_Lock,ProgramName,256))
                       AddPart(ProgramName,wa->wa_Name,256);
                     }
                     break;
     case WBPROJECT: ProgramName=strdup(dobj->do_DefaultTool);
                     if (!(FromFileName=strdup(wa->wa_Name)))
                      FromFileName=(char *)PrefsFileName;
                     break;
    }

    /* Free DiskObject */
    FreeDiskObject(dobj);
   }
  } else {
   /* CLI Startup */
   struct RDArgs *rda;

   /* Parse command line */
   rda=ReadArgs(Template,(LONG *) &cmdlineparams,NULL);

   /* Get values */
   if (cmdlineparams.from) FromFileName=strdup(cmdlineparams.from);
   if (!FromFileName) FromFileName=(char *)PrefsFileName;
   if (cmdlineparams.pubsc) PubScreenName=strdup(cmdlineparams.pubsc);
   if (cmdlineparams.deffont) DefaultFont=TRUE;

   /* Free ReadArgs parameters */
   if (rda) FreeArgs(rda);

   /* USE? */
   if (cmdlineparams.use) {
    /* Copy From file to preferences file */
    CopyFile(FromFileName,(char *)PrefsFileName);

    /* ...and exit */
    CloseResources();
    return(0);
   }

   /* SAVE? */
   if (cmdlineparams.save) {
    /* Copy From file to save preferences file */
    CopyFile(FromFileName,(char *)SavePrefsFileName);

    /* ...and exit */
    CloseResources();
    return(0);
   }

   /* Get program name */
   if (ProgramName=malloc(256)) {
    *ProgramName='\0';
    GetProgramName(ProgramName,256);
   }
  }

  /* Init object lists */
  {
   int i;

   for (i=0; i<TMOBJTYPES; i++) NewList(&ObjectLists[i]);
  }

  /* Read public screen list */
  NewList(&PubScreenList);
  {
   struct List *pubsclist;

   if (pubsclist=LockPubScreenList()) {
    struct PubScreenNode *pubscnode=(struct PubScreenNode *)GetHead(pubsclist);

    /* Scan list */
    while (pubscnode) {
     struct Node *mynode;

     /* Copy entry */
     if (mynode=malloc(sizeof(struct Node)))
      if (mynode->ln_Name=strdup(pubscnode->psn_Node.ln_Name))
       /* Node allocated, add it to list */
       AddTail(&PubScreenList,mynode);
      else
       /* no memory, free node */
       free(mynode);

     /* get a pointer to next node */
     pubscnode=(struct PubScreenNode *)GetSucc((struct Node *)pubscnode);
    }

    /* Release public screen list */
    UnlockPubScreenList();
   }
  }

  /* Try to get the Workbench window offsets */
  if (PublicScreen=LockPubScreen(WBName)) {
   struct Window *w=PublicScreen->FirstWindow;

   /* Scan Workbench screen window list */
   while (w) {
    struct MsgPort *mp=w->UserPort;

    /* Workbench window? (Lots of safety checks) */
    if ((w->Flags & WFLG_WBENCHWINDOW) && mp &&
        ((mp->mp_Flags & PF_ACTION) == PA_SIGNAL)) {
     struct Task *t=mp->mp_SigTask;

     /* More safety checks */
     if (t && t->tc_Node.ln_Name && !strcmp(t->tc_Node.ln_Name,WBName))
      /* Window title valid? */
      if (w->Title) {
       /* Window has title, compare it to string "Workbench" */
       if (!strcmp(w->Title,WBName)) break;
      } else
       /* Window has no title, check for Workbench backdrop window */
       if ((w->Flags & (WFLG_BACKDROP|WFLG_BORDERLESS)) ==
            (WFLG_BACKDROP|WFLG_BORDERLESS))
        break;
    }

    /* next window */
    w=w->NextWindow;
   }

   /* Window found? */
   if (w)
    /* Yes. Read offsets. Backdrop window? */
    if (w->Flags & WFLG_BACKDROP) {
     /* Yes. Just read window coordinates */
     WBXOffset=w->LeftEdge;
     WBYOffset=w->TopEdge;
    } else {
     /* No */
     WBXOffset=w->LeftEdge+w->BorderLeft;
     WBYOffset=w->TopEdge+w->BorderTop;
    }

   UnlockPubScreen(NULL,PublicScreen);
  }

  /* Read Configuration */
  if (ReadConfigFile(FromFileName)) {
   /* Lock default public screen */
   if (PublicScreen=LockPubScreen(PubScreenName)) {
    WBScreen=(PublicScreen->Flags & SCREENTYPE) == WBENCHSCREEN;

    /* Get visual info */
    if (ScreenVI=GetVisualInfo(PublicScreen,TAG_DONE)) {
     /* Which font should be used? */
     if (DefaultFont) {
      struct TextFont *tf=GfxBase->DefaultFont;

      /* Read system default font values */
      ScreenTextAttr.ta_Name=tf->tf_Message.mn_Node.ln_Name;
      ScreenTextAttr.ta_YSize=tf->tf_YSize;
      ScreenTextAttr.ta_Style=tf->tf_Style;
      ScreenTextAttr.ta_Flags=tf->tf_Flags;
     }
     else
      /* Copy public screen TextAttr */
      ScreenTextAttr=*(PublicScreen->Font);

     /* Limit font size. Fonts must be higher than 7 pixels! */
     if (ScreenTextAttr.ta_YSize<8) ScreenTextAttr.ta_YSize=8;

     /* Open screen font */
     if (ScreenFont=OpenDiskFont(&ScreenTextAttr)) {

      /* Calculate requester button image */
      if (CalcReqButtonImage()) {
       ULONG IDCMPSignalMask;
       Object *raWindowObj;

       /* Init global TmpRastPort */
       InitRastPort(&TmpRastPort);
       SetFont(&TmpRastPort,ScreenFont);

       /* Init windows (ReAction main + GadTools edit/requesters) */
       {
        UWORD left=PublicScreen->WBorLeft+INTERWIDTH/2;
        UWORD fheight=ScreenFont->tf_YSize+INTERHEIGHT;

        WindowTop=PublicScreen->WBorTop+PublicScreen->Font->ta_YSize+1;

        InitRAMainWindow(left,fheight);
        InitExecEditWindow(left,fheight);
        InitImageEditWindow(left,fheight);
        InitSoundEditWindow(left,fheight);
        InitMenuEditWindow(left,fheight);
        InitIconEditWindow(left,fheight);
        InitDockEditWindow(left,fheight);
        InitAccessEditWindow(left,fheight);
        InitSelectWindow(left,fheight);
        InitMoveWindow(left,fheight);
        InitDockListEditWindow(left,fheight);
        InitListRequester(left,fheight);
       }

       /* Init global NewGadget structure */
       NewGadget.ng_TextAttr=&ScreenTextAttr;
       NewGadget.ng_VisualInfo=ScreenVI;
       NewGadget.ng_UserData=NULL;

       /* Open ReAction main window */
       if (IDCMPSignalMask=OpenRAMainWindow(MainWinX,MainWinY)) {
        ULONG PrefsPortSignalMask,AppMsgPortSignalMask;
        ULONG SignalMask;
        BPTR oldcd;
        BOOL notend=TRUE;
        ULONG result;
        UWORD code;
        struct IntuiMessage *msg;
        void *handlerData;

        raWindowObj=GetRAMainWindowObject();

        /* Init signal masks */
        PrefsPortSignalMask=1L << PrefsPort->mp_SigBit;
        AppMsgPortSignalMask=1L << AppMsgPort->mp_SigBit;

        /* Go to boot directory */
        oldcd=CurrentDir(NULL);

        /* Main event loop: main window (ReAction) + optional GadTools sub-window port */
        while (notend) {
         ULONG GotSigs;

         SignalMask=IDCMPSignalMask | PrefsPortSignalMask | AppMsgPortSignalMask;
         if (SubWindowPort)
          SignalMask|=1L << SubWindowPort->mp_SigBit;

         /* Wait for messages */
         GotSigs=Wait(SignalMask);

         /* Sub-window (GadTools or ReAction) has events? */
         if (SubWindowPort && (GotSigs & (1L << SubWindowPort->mp_SigBit))) {
          if (SubWindowRAObject) {
           /* ReAction sub-window: drive RA_HandleInput until handler says close */
           while ((result=RA_HandleInput(SubWindowRAObject,&code))!=WMHI_LASTMSG) {
            if (SubWindowRAHandler && (*SubWindowRAHandler)(SubWindowRAObject,result,code)) {
             if (SubWindowRACloseFunc) (*SubWindowRACloseFunc)();
             SubWindowRAObject=NULL;
             SubWindowPort=NULL;
             SubWindowRAHandler=NULL;
             SubWindowRACloseFunc=NULL;
             handlerData=SubWindowRAReturnData;
             SubWindowRAReturnData=NULL;
             if (UpdateWindow)
              (*UpdateWindow)(handlerData);
             else
              notend=FALSE;
             break;
            }
           }
          } else {
           /* GadTools sub-window */
           while (msg=(struct IntuiMessage *)GT_GetIMsg(SubWindowPort)) {
            handlerData=(*SubWindowHandler)(msg);
            if (handlerData) {
             SubWindowPort=NULL;
             SubWindowHandler=NULL;
             if (UpdateWindow)
              (*UpdateWindow)(handlerData);
             else
              notend=FALSE;
            } else
             GT_ReplyIMsg(msg);
           }
          }
         }

         /* IDCMP event (main port): if ReAction sub-window open it may share this port, process it first */
         if (GotSigs & IDCMPSignalMask) {
          if (SubWindowRAObject) {
           while ((result=RA_HandleInput(SubWindowRAObject,&code))!=WMHI_LASTMSG) {
            if (SubWindowRAHandler && (*SubWindowRAHandler)(SubWindowRAObject,result,code)) {
             if (SubWindowRACloseFunc) (*SubWindowRACloseFunc)();
             SubWindowRAObject=NULL;
             SubWindowPort=NULL;
             SubWindowRAHandler=NULL;
             SubWindowRACloseFunc=NULL;
             handlerData=SubWindowRAReturnData;
             SubWindowRAReturnData=NULL;
             if (UpdateWindow)
              (*UpdateWindow)(handlerData);
             else
              notend=FALSE;
             break;
            }
           }
          } else {
           while ((result=RA_HandleInput(raWindowObj,&code))!=WMHI_LASTMSG) {
            if (HandleRAMainWindowEvent(raWindowObj,result,code))
             notend=FALSE;
           }
          }
         }

         /* Prefs port signal? */
         if (GotSigs & PrefsPortSignalMask)
          WindowToFront(CurrentWindow);

         /* Application message arrived? */
         if (GotSigs & AppMsgPortSignalMask) {
          struct AppMessage *msg;

          while (msg=(struct AppMessage *) GetMsg(AppMsgPort)) {
           if (HandleAppMsg)
            (*HandleAppMsg)(msg);
           ReplyMsg((struct Message *) msg);
          }
         }
        }

        /* Go back to old directory */
        CurrentDir(oldcd);

        /* Close ReAction main window */
        CloseRAMainWindow();

        /* End of program. Clear public screen pointer */
        PublicScreen=NULL;
       }
       FreeReqButtonImage();
      }
      CloseFont(ScreenFont);
     }
     FreeVisualInfo(ScreenVI);
    }
    if (PublicScreen) UnlockPubScreen(NULL,PublicScreen);
   }
  }

  /* Free public screen list */
  {
   struct Node *n1,*n2=GetHead(&PubScreenList);

   while (n1=n2) {
    /* Get next node */
    n2=GetSucc(n1);

    /* Free node */
    free(n1->ln_Name);
    free(n1);
   }
  }

  /* Free all object nodes */
  FreeAllObjects();
 }

 /* Closing down... */
 CloseResources();
 return(0);
}

/* CLI entry point */
int main(int argc, char *argv[])
{
 return(mainprogram(NULL));
}

/* WB entry point */
int wbmain(struct WBStartup *wbs)
{
 struct WBArg *wa;
 BPTR dirlock;
 int rc;

 /* Get last WBArg */
 wa=&wbs->sm_ArgList[wbs->sm_NumArgs-1];

 /* Go to icon directory */
 dirlock=CurrentDir(wa->wa_Lock);

 /* Do it :-) */
 rc=mainprogram(wa);

 /* Go back... */
 CurrentDir(dirlock);
 return(rc);
}
