/*
 * selectwindow.c  RAprefs
 *
 * New-object select window: ReAction GUI (WindowObject + layout + string +
 * chooser + buttons). Integrates via SubWindowRAObject and main loop
 * RA_HandleInput path.
 *
 * (c) 1990-1993 Stefan Becker
 */

#include "RAprefsConf.h"

/* string.gadget: STRING_GetClass() can be missing from SDK; declare so StringObject macro compiles (autodocs: STRING_GetClass) */
/*struct IClass *STRING_GetClass(void);*/

/* Action codes (object type in chooser) */
#define SELACT_EXEC     0
#define SELACT_IMAGE    1
#define SELACT_MENU     2
#define SELACT_ICON     3
#define SELACT_MENUICON 4
#define SELACTIONS      5

/* ReAction gadget IDs */
#define G_SEL_NAME   1
#define G_SEL_OBJECT 2
#define G_SEL_OK     3
#define G_SEL_CANCEL 4

static struct WBArg *CurrentWBArg;
static ULONG SelectAction;
static ULONG OldSelAction;

/* ReAction window and gadget object pointers */
static Object *RASelectWindowObj;
static Object *SelNameStrObj;
static Object *SelTypeChooserObj;
static struct List *SelectChooserLabels;

/* Free WBArg */
static void FreeWBArg(struct WBArg *wa)
{
 if (wa->wa_Name) free(wa->wa_Name);
 if (wa->wa_Lock) UnLock(wa->wa_Lock);
 FreeMem(wa,sizeof(struct WBArg));
}

/* Copy WBArg */
static struct WBArg *CopyWBArg(struct WBArg *oldwa)
{
 struct WBArg *newwa;

 if (newwa=AllocMem(sizeof(struct WBArg),MEMF_PUBLIC|MEMF_CLEAR)) {
  if ((newwa->wa_Lock=DupLock(oldwa->wa_Lock)) &&
      (newwa->wa_Name=(BYTE *) strdup(oldwa->wa_Name)))
   return newwa;
  FreeWBArg(newwa);
 }
 return NULL;
}

/* Create nodes for chosen type; name and CurrentWBArg must be set. Returns (void*)1 on success, (void*)-1 on failure */
static void *OKGadgetFuncWithName(char *name)
{
 switch (SelectAction) {
  case SELACT_EXEC: {
   struct Node *en;
   if (en=CreateExecNode(name,CurrentWBArg)) {
    AddHead(&ObjectLists[TMOBJTYPE_EXEC],en);
    return (void *)1;
   }
  }
  break;
  case SELACT_IMAGE: {
   struct Node *in;
   if (in=CreateImageNode(name,CurrentWBArg)) {
    AddHead(&ObjectLists[TMOBJTYPE_IMAGE],in);
    return (void *)1;
   }
  }
  break;
  case SELACT_MENU: {
   struct Node *en;
   struct Node *mn;
   if (en=CreateExecNode(name,CurrentWBArg)) {
    if (mn=CreateMenuNode(name)) {
     AddHead(&ObjectLists[TMOBJTYPE_EXEC],en);
     AddHead(&ObjectLists[TMOBJTYPE_MENU],mn);
     return (void *)1;
    }
    FreeExecNode(en);
   }
  }
  break;
  case SELACT_ICON: {
   struct Node *en;
   struct Node *in;
   struct Node *icn;
   if (en=CreateExecNode(name,CurrentWBArg)) {
    if (in=CreateImageNode(name,CurrentWBArg)) {
     if (icn=CreateIconNode(name)) {
      AddHead(&ObjectLists[TMOBJTYPE_EXEC],en);
      AddHead(&ObjectLists[TMOBJTYPE_IMAGE],in);
      AddHead(&ObjectLists[TMOBJTYPE_ICON],icn);
      return (void *)1;
     }
     FreeImageNode(in);
    }
    FreeExecNode(en);
   }
  }
  break;
  case SELACT_MENUICON: {
   struct Node *en;
   struct Node *in;
   struct Node *mn;
   struct Node *icn;
   if (en=CreateExecNode(name,CurrentWBArg)) {
    if (in=CreateImageNode(name,CurrentWBArg)) {
     if (mn=CreateMenuNode(name)) {
      if (icn=CreateIconNode(name)) {
       AddHead(&ObjectLists[TMOBJTYPE_EXEC],en);
       AddHead(&ObjectLists[TMOBJTYPE_IMAGE],in);
       AddHead(&ObjectLists[TMOBJTYPE_MENU],mn);
       AddHead(&ObjectLists[TMOBJTYPE_ICON],icn);
       return (void *)1;
      }
      FreeMenuNode(mn);
     }
     FreeImageNode(in);
    }
    FreeExecNode(en);
   }
  }
  break;
 }
 return (void *)-1;
}

/* Called from main loop when ReAction select window is closed; disposes window and frees WBArg */
void CloseRASelectWindow(void)
{
 if (CurrentWBArg) {
  FreeWBArg(CurrentWBArg);
  CurrentWBArg=NULL;
 }
 if (RASelectWindowObj) {
  DisposeObject(RASelectWindowObj);
  RASelectWindowObj=NULL;
 }
 SelNameStrObj=NULL;
 SelTypeChooserObj=NULL;
}

/* ReAction select window event handler; returns TRUE when window should close (main loop will call SubWindowRACloseFunc and UpdateWindow) */
BOOL HandleRASelectWindowEvent(Object *windowObj, ULONG result, UWORD code)
{
 STRPTR namePtr;
 void *rc;

 (void)windowObj;

 switch (result) {
  case WMHI_CLOSEWINDOW:
   SelectAction=OldSelAction;
   SubWindowRAReturnData=(void *)-1;
   return TRUE;

  case WMHI_GADGETUP:
   switch (code) {
    case G_SEL_OK:
     namePtr=NULL;
     if (SelNameStrObj)
      GetAttr(STRINGA_TextVal,SelNameStrObj,(ULONG *)&namePtr);
     if (namePtr && *namePtr)
      rc=OKGadgetFuncWithName(namePtr);
     else
      rc=(void *)-1;
     SubWindowRAReturnData=rc;
     return TRUE;

    case G_SEL_CANCEL:
     SelectAction=OldSelAction;
     SubWindowRAReturnData=(void *)-1;
     return TRUE;

    case G_SEL_OBJECT:
     if (SelTypeChooserObj) {
      GetAttr(CHOOSER_Selected,SelTypeChooserObj,(ULONG *)&SelectAction);
     }
     break;
   }
   break;
 }

 return FALSE;
}

/* Init select window: build chooser labels for object type list */
void InitSelectWindow(UWORD left, UWORD fheight)
{
 STRPTR lab[SELACTIONS+1];

 (void)left;
 (void)fheight;

 lab[0]=AppStrings[IX_MAINWIN_TYPE_EXEC_CYCLE_LABEL];
 lab[1]=AppStrings[IX_MAINWIN_TYPE_IMAGE_CYCLE_LABEL];
 lab[2]=AppStrings[IX_SELECTWIN_MENU_MX_LABEL];
 lab[3]=AppStrings[IX_SELECTWIN_ICON_MX_LABEL];
 lab[4]=AppStrings[IX_SELECTWIN_MENUICON_MX_LABEL];
 lab[5]=NULL;

 if (SelectChooserLabels) FreeChooserLabels(SelectChooserLabels);
 SelectChooserLabels=ChooserLabelsA(lab);
}

/* Open select window (ReAction); parent is main window for WINDOW_RefWindow */
BOOL OpenSelectWindow(struct WBArg *wa, struct Window *parent)
{
 Object *layout;
 struct Window *w;

 if (!SelectChooserLabels) return FALSE;
 if (!(CurrentWBArg=CopyWBArg(wa))) return FALSE;

 OldSelAction=SelectAction;

 layout=VGroupObject,
  LAYOUT_SpaceOuter,TRUE,
  LAYOUT_SpaceInner,TRUE,
  LAYOUT_BevelStyle,BVS_THIN,
  StartMember,
   SelNameStrObj=StringObject,
    GA_ID,G_SEL_NAME,
    GA_RelVerify,TRUE,
    STRINGA_TextVal,CurrentWBArg->wa_Name,
    STRINGA_MaxChars,SGBUFLEN,
   EndMember,
   MemberLabel(AppStrings[IX_WINDOW_NAME_GAD]),
  StartMember,
   SelTypeChooserObj=ChooserObject,
    GA_ID,G_SEL_OBJECT,
    GA_RelVerify,TRUE,
    CHOOSER_PopUp,TRUE,
    CHOOSER_Labels,SelectChooserLabels,
    CHOOSER_Selected,SelectAction,
   EndMember,
   MemberLabel(AppStrings[IX_SELECTWIN_OBJECT_GAD]),
  StartHGroup,EvenSized,
   StartMember,ButtonObject,GA_ID,G_SEL_OK,GA_RelVerify,TRUE,GA_Text,AppStrings[IX_WINDOW_OK_GAD],ButtonEnd,
   StartMember,ButtonObject,GA_ID,G_SEL_CANCEL,GA_RelVerify,TRUE,GA_Text,AppStrings[IX_WINDOW_CANCEL_GAD],ButtonEnd,
  EndGroup,
 EndGroup;

 if (!layout) {
  FreeWBArg(CurrentWBArg);
  CurrentWBArg=NULL;
  return FALSE;
 }

 RASelectWindowObj=WindowObject,
  WA_PubScreen,PublicScreen,
  WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_RMBTRAP|WFLG_ACTIVATE,
  WA_Title,AppStrings[IX_SELECTWIN_TITLE],
  WINDOW_RefWindow,parent,
  WINDOW_Position,WPOS_CENTERWINDOW,
  WINDOW_ParentGroup,layout,
 EndWindow;

 if (!RASelectWindowObj) {
  DisposeObject(layout);
  FreeWBArg(CurrentWBArg);
  CurrentWBArg=NULL;
  return FALSE;
 }

 if (!(w=(struct Window *)RA_OpenWindow(RASelectWindowObj))) {
  DisposeObject(RASelectWindowObj);
  RASelectWindowObj=NULL;
  SelNameStrObj=NULL;
  SelTypeChooserObj=NULL;
  FreeWBArg(CurrentWBArg);
  CurrentWBArg=NULL;
  return FALSE;
 }

 CurrentWindow=w;
 SubWindowPort=w->UserPort;
 SubWindowRAObject=RASelectWindowObj;
 SubWindowRAHandler=HandleRASelectWindowEvent;
 SubWindowRACloseFunc=CloseRASelectWindow;

 return TRUE;
}

/* Stub for compatibility with code that expects GadTools select window IDCMP handler (unused when using ReAction select window) */
void *HandleSelectWindowIDCMP(struct IntuiMessage *msg)
{
 (void)msg;
 return NULL;
}
