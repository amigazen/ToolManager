/*
 * menuwindow.c  V2.1
 *
 * menu edit window handling
 *
 * (c) 1990-1993 Stefan Becker
 */

#include "ToolManagerConf.h"
#include <stdio.h>

/* Menu node */
struct MenuNode {
                  struct Node  mn_Node;
                  char        *mn_Exec;
                  char        *mn_Sound;
                  char        *mn_Title;      /* V44/V45: menu strip title (NULL = Tools) */
                  char        *mn_CommandKey;  /* V44+: shortcut key string */
                  ULONG       mn_Flags;       /* MOPOF_SEPARATOR, MOPOF_SUBMENU_PARENT */
                  LONG        mn_ParentIndex; /* -1 = top under title, else index of parent */
                 };

/* Window data */
static struct Gadget *gl;             /* Gadget list */
static struct Window *w;              /* Window */
static struct MsgPort *wp;            /* Window user port */
static UWORD ww,wh;                   /* Window size */
static struct MenuNode *CurrentNode;
static ULONG CurrentGadgetNum;
static BOOL ReqOpen;
static struct Requester DummyReq;
#define WINDOW_IDCMP (IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|BUTTONIDCMP|\
                      CHECKBOXIDCMP|STRINGIDCMP|TEXTIDCMP|IDCMP_VANILLAKEY)

#ifndef CHECKBOX_WIDTH
#define CHECKBOX_WIDTH 16
#endif

/* Gadget data */
#define GAD_NAME_STR     0
#define GAD_EXEC_BUT     1
#define GAD_EXEC_TXT     2
#define GAD_SOUND_BUT    3
#define GAD_SOUND_TXT    4
#define GAD_TITLE_STR    5
#define GAD_SEPARATOR_CHK 6
#define GAD_SUBMENU_CHK  7
#define GAD_PARENT_STR   8
#define GAD_CMDKEY_STR   9
#define GAD_OK          10
#define GAD_CANCEL      11
#define GADGETS         12
static struct GadgetData gdata[GADGETS];

/* Buffers for new fields when node has NULL; and for parent index (number). */
#define MENU_TITLE_BUF_LEN  64
#define PARENT_BUF_LEN      16
#define CMDKEY_BUF_LEN      8
static char menu_title_buf[MENU_TITLE_BUF_LEN];
static char parent_index_buf[PARENT_BUF_LEN];
static char menu_cmdkey_buf[CMDKEY_BUF_LEN];

/* Gadget tags */
static struct TagItem nametags[]={GTST_String,   NULL,
                                  GTST_MaxChars, SGBUFLEN,
                                  TAG_DONE};

static struct TagItem exectags[]={GTTX_Text,   NULL,
                                  GTTX_Border, TRUE,
                                  TAG_DONE};

static struct TagItem soundtags[]={GTTX_Text,   NULL,
                                   GTTX_Border, TRUE,
                                   TAG_DONE};

static struct TagItem titletags[]={GTST_String,   NULL,
                                  GTST_MaxChars, MENU_TITLE_BUF_LEN-1,
                                  TAG_DONE};

static struct TagItem separatortags[]={GTCB_Checked, FALSE,
                                      GTCB_Scaled,  TRUE,
                                      TAG_DONE};

static struct TagItem submenutags[]={GTCB_Checked, FALSE,
                                     GTCB_Scaled,  TRUE,
                                     TAG_DONE};

static struct TagItem parentindextags[]={GTST_String,   NULL,
                                        GTST_MaxChars, PARENT_BUF_LEN-1,
                                        TAG_DONE};

static struct TagItem cmdkeytags[]={GTST_String,   NULL,
                                   GTST_MaxChars, CMDKEY_BUF_LEN-1,
                                   TAG_DONE};

/* Labels for V44/V45 menu options (no locale keys yet) */
static char *label_menu_title="Menu title";
static char *label_separator="Separator";
static char *label_submenu="Submenu parent";
static char *label_parent_idx="Parent index";
static char *label_cmdkey="Cmd key";

/* Gadget vanilla key data */
#define KEY_NAME   0
#define KEY_EXEC   1
#define KEY_SOUND  2
#define KEY_OK     3
#define KEY_CANCEL 4
static char KeyArray[KEY_CANCEL+1];

/* Init menu edit window */
void InitMenuEditWindow(UWORD left, UWORD fheight)
{
 ULONG tmp,tmp2,maxw1,maxw2,cbw;
 ULONG strheight=fheight+2;
 struct GadgetData *gd;

 gdata[GAD_OK].name       =AppStrings[MSG_WINDOW_OK_GAD];
 gdata[GAD_CANCEL].name   =AppStrings[MSG_WINDOW_CANCEL_GAD];

 maxw1=TextLength(&TmpRastPort,AppStrings[MSG_WINDOW_NAME_GAD],
                  strlen(AppStrings[MSG_WINDOW_NAME_GAD]));
 tmp=TextLength(&TmpRastPort,AppStrings[MSG_WINDOW_EXEC_GAD],
                strlen(AppStrings[MSG_WINDOW_EXEC_GAD]))+2*INTERWIDTH;
 if (tmp > maxw1) maxw1=tmp;
 tmp=TextLength(&TmpRastPort,AppStrings[MSG_WINDOW_SOUND_GAD],
                strlen(AppStrings[MSG_WINDOW_SOUND_GAD]))+2*INTERWIDTH;
 if (tmp > maxw1) maxw1=tmp;
 tmp=TextLength(&TmpRastPort,label_menu_title,strlen(label_menu_title))+2*INTERWIDTH;
 if (tmp > maxw1) maxw1=tmp;
 tmp=TextLength(&TmpRastPort,label_parent_idx,strlen(label_parent_idx))+2*INTERWIDTH;
 if (tmp > maxw1) maxw1=tmp;
 tmp=TextLength(&TmpRastPort,label_cmdkey,strlen(label_cmdkey))+2*INTERWIDTH;
 if (tmp > maxw1) maxw1=tmp;
 maxw1+=INTERWIDTH;

 ww=TextLength(&TmpRastPort,AppStrings[MSG_MENUWIN_NEWNAME],
               strlen(AppStrings[MSG_MENUWIN_NEWNAME]))+
    maxw1+3*INTERWIDTH;

 gd=&gdata[GAD_OK];
 maxw2=TextLength(&TmpRastPort,gd->name,strlen(gd->name));
 gd++;
 if ((tmp=TextLength(&TmpRastPort,gd->name,strlen(gd->name))) > maxw2)
  maxw2=tmp;
 maxw2+=2*INTERWIDTH;
 if ((tmp=2*(maxw2+INTERWIDTH)) > ww) ww=tmp;

 cbw=TextLength(&TmpRastPort,label_separator,strlen(label_separator))+
    CHECKBOX_WIDTH+INTERWIDTH;
 tmp=TextLength(&TmpRastPort,label_submenu,strlen(label_submenu))+
    CHECKBOX_WIDTH+INTERWIDTH;
 if (tmp > cbw) cbw=tmp;
 if ((tmp=2*cbw+INTERWIDTH) > ww) ww=tmp;

 wh=4*fheight+5*INTERHEIGHT+6+4*(strheight+INTERHEIGHT);

 gd=gdata;
 tmp=WindowTop+INTERHEIGHT;
 tmp2=ww-maxw1-INTERWIDTH;

 gd->name=AppStrings[MSG_WINDOW_NAME_GAD];
 gd->type=STRING_KIND;
 gd->flags=PLACETEXT_LEFT;
 gd->tags=nametags;
 gd->left=maxw1+left;
 gd->top=tmp;
 gd->width=tmp2;
 gd->height=strheight;
 tmp+=strheight+INTERHEIGHT;

 gd++;
 gd->name=AppStrings[MSG_WINDOW_EXEC_GAD];
 gd->type=BUTTON_KIND;
 gd->flags=PLACETEXT_IN;
 gd->left=left;
 gd->top=tmp;
 gd->width=maxw1-INTERWIDTH;
 gd->height=strheight;
 gd++;
 gd->type=TEXT_KIND;
 gd->tags=exectags;
 gd->left=maxw1+left;
 gd->top=tmp;
 gd->width=tmp2;
 gd->height=strheight;
 tmp+=strheight+INTERHEIGHT;

 gd++;
 gd->name=AppStrings[MSG_WINDOW_SOUND_GAD];
 gd->type=BUTTON_KIND;
 gd->flags=PLACETEXT_IN;
 gd->left=left;
 gd->top=tmp;
 gd->width=maxw1-INTERWIDTH;
 gd->height=strheight;
 gd++;
 gd->type=TEXT_KIND;
 gd->tags=soundtags;
 gd->left=maxw1+left;
 gd->top=tmp;
 gd->width=tmp2;
 gd->height=strheight;
 tmp+=strheight+INTERHEIGHT;

 gd++;
 gd->name=label_menu_title;
 gd->type=STRING_KIND;
 gd->flags=PLACETEXT_LEFT;
 gd->tags=titletags;
 gd->left=maxw1+left;
 gd->top=tmp;
 gd->width=tmp2;
 gd->height=strheight;
 tmp+=strheight+INTERHEIGHT;

 gd++;
 gd->name=label_separator;
 gd->type=CHECKBOX_KIND;
 gd->flags=PLACETEXT_RIGHT;
 gd->tags=separatortags;
 gd->left=left;
 gd->top=tmp;
 gd->width=CHECKBOX_WIDTH;
 gd->height=fheight-INTERHEIGHT;
 gd++;
 gd->name=label_submenu;
 gd->type=CHECKBOX_KIND;
 gd->flags=PLACETEXT_RIGHT;
 gd->tags=submenutags;
 gd->left=left+cbw;
 gd->top=tmp;
 gd->width=CHECKBOX_WIDTH;
 gd->height=fheight-INTERHEIGHT;
 tmp+=strheight+INTERHEIGHT;

 gd++;
 gd->name=label_parent_idx;
 gd->type=STRING_KIND;
 gd->flags=PLACETEXT_LEFT;
 gd->tags=parentindextags;
 gd->left=maxw1+left;
 gd->top=tmp;
 gd->width=tmp2;
 gd->height=strheight;
 tmp+=strheight+INTERHEIGHT;

 gd++;
 gd->name=label_cmdkey;
 gd->type=STRING_KIND;
 gd->flags=PLACETEXT_LEFT;
 gd->tags=cmdkeytags;
 gd->left=maxw1+left;
 gd->top=tmp;
 gd->width=tmp2;
 gd->height=strheight;
 tmp+=strheight+INTERHEIGHT;

 gd++;
 gd->type=BUTTON_KIND;
 gd->flags=PLACETEXT_IN;
 gd->left=left;
 gd->top=tmp;
 gd->width=maxw2;
 gd->height=fheight;

 gd++;
 gd->type=BUTTON_KIND;
 gd->flags=PLACETEXT_IN;
 gd->left=ww-maxw2-INTERWIDTH+left;
 gd->top=tmp;
 gd->width=maxw2;
 gd->height=fheight;

 KeyArray[KEY_NAME]  =FindVanillaKey(gdata[GAD_NAME_STR].name);
 KeyArray[KEY_EXEC]  =FindVanillaKey(gdata[GAD_EXEC_BUT].name);
 KeyArray[KEY_SOUND] =FindVanillaKey(gdata[GAD_SOUND_BUT].name);
 KeyArray[KEY_OK]    =FindVanillaKey(gdata[GAD_OK].name);
 KeyArray[KEY_CANCEL]=FindVanillaKey(gdata[GAD_CANCEL].name);

 InitRequester(&DummyReq);
}

/* Free menu node */
void FreeMenuNode(struct Node *node)
{
 struct MenuNode *mn=(struct MenuNode *) node;
 char *s;

 if (s=mn->mn_Node.ln_Name) free(s);
 if (s=mn->mn_Exec) free(s);
 if (s=mn->mn_Sound) free(s);
 if (s=mn->mn_Title) free(s);
 if (s=mn->mn_CommandKey) free(s);

 FreeMem(mn,sizeof(struct MenuNode));
}

/* Copy menu node */
struct Node *CopyMenuNode(struct Node *node)
{
 struct MenuNode *mn;
 struct MenuNode *orignode=(struct MenuNode *) node;
 char *dup_title;
 char *dup_cmdkey;

 /* Alloc memory for menu node */
 if (mn=AllocMem(sizeof(struct MenuNode),MEMF_PUBLIC|MEMF_CLEAR)) {

  /* Got an old node? */
  if (orignode) {
   dup_title=(orignode->mn_Title && *orignode->mn_Title) ?
             strdup(orignode->mn_Title) : NULL;
   dup_cmdkey=(orignode->mn_CommandKey && *orignode->mn_CommandKey) ?
              strdup(orignode->mn_CommandKey) : NULL;
   if ((!orignode->mn_Node.ln_Name || (mn->mn_Node.ln_Name=strdup(orignode->mn_Node.ln_Name))) &&
       (!orignode->mn_Exec || (mn->mn_Exec=strdup(orignode->mn_Exec))) &&
       (!orignode->mn_Sound || (mn->mn_Sound=strdup(orignode->mn_Sound))) &&
       (dup_title || !orignode->mn_Title) && (dup_cmdkey || !orignode->mn_CommandKey)) {
    mn->mn_Title=dup_title;
    mn->mn_CommandKey=dup_cmdkey;
    mn->mn_Flags=orignode->mn_Flags;
    mn->mn_ParentIndex=orignode->mn_ParentIndex;
    return((struct Node *)mn);
   }
   if (dup_title) free(dup_title);
   if (dup_cmdkey) free(dup_cmdkey);
  } else {
   if (mn->mn_Node.ln_Name=strdup(AppStrings[MSG_MENUWIN_NEWNAME])) {
    mn->mn_ParentIndex=-1;
    return((struct Node *)mn);
   }
  }
  FreeMenuNode((struct Node *)mn);
 }
 /* Call failed */
 return(NULL);
}

/* Create menu node from string */
struct Node *CreateMenuNode(char *name)
{
 struct MenuNode *mn;

 if (!(mn=AllocMem(sizeof(struct MenuNode),MEMF_PUBLIC|MEMF_CLEAR)))
  return(NULL);
 mn->mn_ParentIndex=-1;
 if ((mn->mn_Node.ln_Name=strdup(name)) && (mn->mn_Exec=strdup(name)))
  return((struct Node *)mn);
 FreeMenuNode((struct Node *)mn);
 return(NULL);
}

/* Activate name string gadget */
static void ActivateNameGadget(void)
{
 ActivateGadget(gdata[GAD_NAME_STR].gadget,w,NULL);
}

/* Open menu edit window */
BOOL OpenMenuEditWindow(struct Node *node, struct Window *parent)
{
 if (CurrentNode=(struct MenuNode *) CopyMenuNode(node)) {
  nametags[0].ti_Data=(ULONG) CurrentNode->mn_Node.ln_Name;
  exectags[0].ti_Data=(ULONG) CurrentNode->mn_Exec;
  soundtags[0].ti_Data=(ULONG) CurrentNode->mn_Sound;

  titletags[0].ti_Data=(ULONG)(CurrentNode->mn_Title && *CurrentNode->mn_Title
    ? CurrentNode->mn_Title : (menu_title_buf[0]='\0', menu_title_buf));
  separatortags[0].ti_Data=(CurrentNode->mn_Flags & MOPOF_SEPARATOR) ? 1u : 0u;
  submenutags[0].ti_Data=(CurrentNode->mn_Flags & MOPOF_SUBMENU_PARENT) ? 1u : 0u;
  sprintf(parent_index_buf, "%ld", (long)CurrentNode->mn_ParentIndex);
  parentindextags[0].ti_Data=(ULONG)parent_index_buf;
  cmdkeytags[0].ti_Data=(ULONG)(CurrentNode->mn_CommandKey && *CurrentNode->mn_CommandKey
    ? CurrentNode->mn_CommandKey : (menu_cmdkey_buf[0]='\0', menu_cmdkey_buf));

  if (gl=CreateGadgetList(gdata,GADGETS)) {
   /* Open window */
   if (w=OpenWindowTags(NULL,WA_Left,        parent->LeftEdge,
                             WA_Top,         parent->TopEdge+WindowTop,
                             WA_InnerWidth,  ww,
                             WA_InnerHeight, wh,
                             WA_AutoAdjust,  TRUE,
                             WA_Title,       AppStrings[MSG_MENUWIN_TITLE],
                             WA_PubScreen,   PublicScreen,
                             WA_Flags,       WFLG_CLOSEGADGET|WFLG_DRAGBAR|
                                             WFLG_DEPTHGADGET|WFLG_RMBTRAP|
                                             WFLG_ACTIVATE,
                             TAG_DONE)) {
    /* Add gadgets to window */
    AddGList(w,gl,(UWORD) -1,(UWORD) -1,NULL);
    RefreshGList(gl,w,NULL,(UWORD) -1);
    GT_RefreshWindow(w,NULL);

    /* Activate name string gadget */
    ActivateNameGadget();

    /* Set local variables */
    w->UserPort=IDCMPPort;
    w->UserData=(BYTE *) HandleMenuEditWindowIDCMP;
    ModifyIDCMP(w,WINDOW_IDCMP);
    CurrentWindow=w;
    ReqOpen=FALSE;

    /* All OK. */
    return(TRUE);
   }
   FreeGadgets(gl);
  }
  FreeMenuNode((struct Node *) CurrentNode);
 }
 /* Call failed */
 return(FALSE);
}

/* Close menu edit window */
static void CloseMenuEditWindow(void)
{
 /* Free resources */
 RemoveGList(w,gl,(UWORD) -1);
 CloseWindowSafely(w);
 FreeGadgets(gl);
}

/* Exec & Sound gadget function */
static void TextButtonGadgetFunc(ULONG gadnum, ULONG listnum)
{
 if (!ReqOpen) {
  /* Save gadget number */
  CurrentGadgetNum=gadnum;

  /* Open list requester */
  if (OpenListRequester(listnum,w)) {
   /* Disable window */
   DisableWindow(w,&DummyReq);

   /* Set update function */
   UpdateWindow=UpdateMenuEditWindow;
   ReqOpen=TRUE;
  }
 }
}

/* OK gadget function */
static struct Node *OKGadgetFunc(void)
{
 struct Node *rc;
 char *s;
 char *new_title;
 char *new_cmdkey;
 char *parent_str;
 ULONG sep;
 ULONG sub;

 if (s=CurrentNode->mn_Node.ln_Name) free(s);
 if ((CurrentNode->mn_Node.ln_Name=
       DuplicateBuffer(gdata[GAD_NAME_STR].gadget)) == (char *) -1) {
  FreeMenuNode((struct Node *) CurrentNode);
  return((struct Node *) -1);
 }

 new_title=DuplicateBuffer(gdata[GAD_TITLE_STR].gadget);
 new_cmdkey=DuplicateBuffer(gdata[GAD_CMDKEY_STR].gadget);
 parent_str=DuplicateBuffer(gdata[GAD_PARENT_STR].gadget);
 if (new_title == (char *) -1 || new_cmdkey == (char *) -1) {
  if (new_title != (char *) -1) free(new_title);
  if (new_cmdkey != (char *) -1) free(new_cmdkey);
  if (parent_str != (char *) -1 && parent_str) free(parent_str);
  FreeMenuNode((struct Node *) CurrentNode);
  return((struct Node *) -1);
 }

 GT_GetGadgetAttrs(gdata[GAD_SEPARATOR_CHK].gadget,w,NULL,GTCB_Checked,&sep,TAG_DONE);
 GT_GetGadgetAttrs(gdata[GAD_SUBMENU_CHK].gadget,w,NULL,GTCB_Checked,&sub,TAG_DONE);

 if (CurrentNode->mn_Title) free(CurrentNode->mn_Title);
 if (CurrentNode->mn_CommandKey) free(CurrentNode->mn_CommandKey);
 CurrentNode->mn_Title=(new_title && *new_title) ? new_title : (free(new_title), (char *)NULL);
 CurrentNode->mn_CommandKey=(new_cmdkey && *new_cmdkey) ? new_cmdkey : (free(new_cmdkey), (char *)NULL);
 CurrentNode->mn_ParentIndex=(parent_str && parent_str!=(char *)-1 && *parent_str)
  ? (LONG)atoi(parent_str) : -1L;
 if (parent_str && parent_str!=(char *)-1) free(parent_str);
 CurrentNode->mn_Flags=(sep ? MOPOF_SEPARATOR : 0u)|(sub ? MOPOF_SUBMENU_PARENT : 0u);
 rc=(struct Node *)CurrentNode;
 return(rc);
}

/* Handle menu edit window IDCMP events */
void *HandleMenuEditWindowIDCMP(struct IntuiMessage *msg)
{
 struct Node *NewNode=NULL;

 /* Which IDCMP class? */
 switch (msg->Class) {
  case IDCMP_CLOSEWINDOW:   NewNode=(struct Node *) -1;
                            FreeMenuNode((struct Node *) CurrentNode);
                            break;
  case IDCMP_REFRESHWINDOW: GT_BeginRefresh(w);
                            GT_EndRefresh(w,TRUE);
                            break;
  case IDCMP_GADGETUP:
   switch (((struct Gadget *) msg->IAddress)->GadgetID) {
    case GAD_EXEC_BUT:  TextButtonGadgetFunc(GAD_EXEC_TXT,LISTREQ_EXEC);
                        break;
    case GAD_SOUND_BUT: TextButtonGadgetFunc(GAD_SOUND_TXT,LISTREQ_SOUND);
                        break;
    case GAD_OK:        NewNode=OKGadgetFunc();
                        break;
    case GAD_CANCEL:    NewNode=(struct Node *) -1;
                        FreeMenuNode((struct Node *) CurrentNode);
                        break;
   }
   break;
  case IDCMP_VANILLAKEY:
   switch (MatchVanillaKey(msg->Code,KeyArray)) {
    case KEY_NAME:   ActivateNameGadget();
                     break;
    case KEY_EXEC:   TextButtonGadgetFunc(GAD_EXEC_TXT,LISTREQ_EXEC);
                     break;
    case KEY_SOUND:  TextButtonGadgetFunc(GAD_SOUND_TXT,LISTREQ_SOUND);
                     break;
    case KEY_OK:     NewNode=OKGadgetFunc();
                     break;
    case KEY_CANCEL: NewNode=(struct Node *) -1;
                     FreeMenuNode((struct Node *) CurrentNode);
                     break;
   }
   break;
 }

 /* Close window? */
 if (NewNode) {
  /* Yes. But first reply message!!! */
  GT_ReplyIMsg(msg);
  CloseMenuEditWindow();
 }

 return(NewNode);
}

/* Update Menu edit window */
void UpdateMenuEditWindow(void *data)
{
 /* Got data? */
 if (data != LREQRET_CANCEL) {
  char *new;

  /* Selected something? */
  new=(data == LREQRET_NOSELECT) ? NULL : ((struct Node *) data)->ln_Name;

  /* Duplicate name */
  if (!new || (new=strdup(new))) {
   char *old;

   /* Exec object? */
   if (CurrentGadgetNum==GAD_EXEC_TXT) {
    /* Yes. Set new Exec name */
    old=CurrentNode->mn_Exec;
    CurrentNode->mn_Exec=new;
   } else {
    /* No. Set new Sound name */
    old=CurrentNode->mn_Sound;
    CurrentNode->mn_Sound=new;
   }

   /* Free old string */
   if (old) free(old);

   /* Set new text */
   GT_SetGadgetAttrs(gdata[CurrentGadgetNum].gadget,w,NULL,GTTX_Text,new,
                                                           TAG_DONE);
  }
 }

 /* Enable window */
 EnableWindow(w,&DummyReq,WINDOW_IDCMP);

 /* Restore update function pointer */
 UpdateWindow=UpdateMainWindow;
 CurrentWindow=w;
 ReqOpen=FALSE;
}

/* Read TMMO IFF chunk into Menu node; chunk_size for optional extension.
 * Forwards compatible: old prefs files have only name/exec/sound and no
 * extension bytes; we leave mn_Title/mn_CommandKey NULL, mn_Flags 0,
 * mn_ParentIndex -1. */
struct Node *ReadMenuNode(UBYTE *buf, ULONG chunk_size)
{
 struct MenuNode *mn;
 struct MenuPrefsObject *mpo;
 ULONG sbits;
 UBYTE *ptr;
 UBYTE *chunk_start;
 ULONG used;
 UBYTE ext_len;

 if (!(mn=AllocMem(sizeof(struct MenuNode),MEMF_PUBLIC|MEMF_CLEAR)))
  return(NULL);

 mpo=(struct MenuPrefsObject *)buf;
 sbits=mpo->mpo_StringBits;
 ptr=(UBYTE *)&mpo[1];
 chunk_start=(UBYTE *)buf;

 if (!(sbits & MOPO_NAME) || (mn->mn_Node.ln_Name=GetConfigStr(&ptr))) {
  if (!(sbits & MOPO_EXEC) || (mn->mn_Exec=GetConfigStr(&ptr))) {
   if (!(sbits & MOPO_SOUND) || (mn->mn_Sound=GetConfigStr(&ptr))) {
    if (!(sbits & MOPO_TITLE) || (mn->mn_Title=GetConfigStr(&ptr))) {
     if (!(sbits & MOPO_CMDKEY) || (mn->mn_CommandKey=GetConfigStr(&ptr))) {
      mn->mn_Flags=0;
      mn->mn_ParentIndex=-1;
      used=(ULONG)(ptr-chunk_start);
      if (chunk_size>=used+1u) {
       ext_len=*ptr++;
       if (ext_len>=8 && chunk_size>=used+9u) {
        mn->mn_Flags=*(ULONG *)ptr;
        ptr+=4;
        mn->mn_ParentIndex=*(LONG *)ptr;
       }
      }
      return((struct Node *)mn);
     }
    }
   }
  }
 }
 FreeMenuNode((struct Node *)mn);
 return(NULL);
}

/* Write Menu node to TMMO IFF chunk */
BOOL WriteMenuNode(struct IFFHandle *iff, UBYTE *buf, struct Node *node)
{
 struct MenuNode *mn=(struct MenuNode *) node;
 struct MenuPrefsObject *mpo=(struct MenuPrefsObject *) buf;
 ULONG sbits=0;
 UBYTE *ptr=(UBYTE *) &mpo[1];
 ULONG chunk_len;
 ULONG ext_len_byte;

 if (PutConfigStr(mn->mn_Node.ln_Name,&ptr)) sbits|=MOPO_NAME;
 if (PutConfigStr(mn->mn_Exec,&ptr)) sbits|=MOPO_EXEC;
 if (PutConfigStr(mn->mn_Sound,&ptr)) sbits|=MOPO_SOUND;
 if (mn->mn_Title && *mn->mn_Title && PutConfigStr(mn->mn_Title,&ptr))
  sbits|=MOPO_TITLE;
 if (mn->mn_CommandKey && *mn->mn_CommandKey && PutConfigStr(mn->mn_CommandKey,&ptr))
  sbits|=MOPO_CMDKEY;

 mpo->mpo_StringBits=sbits;

 if (mn->mn_Flags || mn->mn_ParentIndex!=-1) {
  ext_len_byte=8;
  *ptr++=(UBYTE)ext_len_byte;
  *(ULONG *)ptr=mn->mn_Flags;
  ptr+=4;
  *(LONG *)ptr=mn->mn_ParentIndex;
  ptr+=4;
 }

 chunk_len=(ULONG)(ptr-buf);
 DEBUG_PRINTF("chunk size %ld\n",(long)chunk_len);

 if (PushChunk(iff,0,ID_TMMO,chunk_len)) return(FALSE);
 if (WriteChunkBytes(iff,buf,chunk_len)!=chunk_len) return(FALSE);
 if (PopChunk(iff)) return(FALSE);
 return(TRUE);
}
