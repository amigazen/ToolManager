/*
 * menuwindow.c  RAprefs
 *
 * Menu edit window: ReAction GUI (WindowObject + name string + Exec/Sound
 * buttons + read-only text + OK/Cancel). Exec/Sound open list requester.
 * Integrates via SubWindowRAObject; UpdateMenuEditWindow restores RA state.
 *
 * (c) 1990-1993 Stefan Becker
 */

#include "RAprefsConf.h"
#include <stdio.h>

struct MenuNode {
                  struct Node  mn_Node;
                  char        *mn_Exec;
                  char        *mn_Sound;
                  char        *mn_Title;
                  char        *mn_CommandKey;
                  ULONG       mn_Flags;
                  LONG        mn_ParentIndex;
                 };

#define G_MENU_NAME      1
#define G_MENU_EXEC_BUT  2
#define G_MENU_EXEC_TXT  3
#define G_MENU_SOUND_BUT 4
#define G_MENU_SOUND_TXT 5
#define G_MENU_OK        6
#define G_MENU_CANCEL    7
#define G_MENU_TITLE_STR 8
#define G_MENU_SEPARATOR_CHK 9
#define G_MENU_SUBMENU_CHK   10
#define G_MENU_PARENT_STR    11
#define G_MENU_CMDKEY_STR    12

#define MENU_GAD_EXEC_TXT  3
#define MENU_GAD_SOUND_TXT 5

#define MENU_TITLE_BUF_LEN  64
#define PARENT_BUF_LEN      16
#define CMDKEY_BUF_LEN       8
static char menu_parent_buf[PARENT_BUF_LEN];

static struct MenuNode *CurrentNode;
static ULONG CurrentGadgetNum;
static Object *RAMenuWindowObj;
static Object *RAMenuNameStrObj;
static Object *RAMenuExecTxtObj;
static Object *RAMenuSoundTxtObj;
static Object *RAMenuTitleStrObj;
static Object *RAMenuSeparatorChkObj;
static Object *RAMenuSubmenuChkObj;
static Object *RAMenuParentStrObj;
static Object *RAMenuCmdkeyStrObj;
static struct Requester DummyReq;

struct IClass *STRING_GetClass(void);
struct IClass *CHECKBOX_GetClass(void);

void InitMenuEditWindow(UWORD left, UWORD fheight)
{
 (void)left;
 (void)fheight;
 InitRequester(&DummyReq);
}

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

struct Node *CopyMenuNode(struct Node *node)
{
 struct MenuNode *mn;
 struct MenuNode *orignode=(struct MenuNode *) node;
 char *dup_title;
 char *dup_cmdkey;

 if (!(mn=AllocMem(sizeof(struct MenuNode),MEMF_PUBLIC|MEMF_CLEAR)))
  return NULL;
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
   return (struct Node *)mn;
  }
  if (dup_title) free(dup_title);
  if (dup_cmdkey) free(dup_cmdkey);
 } else {
  if (mn->mn_Node.ln_Name=strdup(AppStrings[MSG_MENUWIN_NEWNAME])) {
   mn->mn_ParentIndex=-1;
   return (struct Node *)mn;
  }
 }
 FreeMenuNode((struct Node *)mn);
 return NULL;
}

struct Node *CreateMenuNode(char *name)
{
 struct MenuNode *mn;

 if (!(mn=AllocMem(sizeof(struct MenuNode),MEMF_PUBLIC|MEMF_CLEAR)))
  return NULL;
 mn->mn_ParentIndex=-1;
 if ((mn->mn_Node.ln_Name=strdup(name)) && (mn->mn_Exec=strdup(name)))
  return (struct Node *)mn;
 FreeMenuNode((struct Node *)mn);
 return NULL;
}

static void CloseRAMenuWindow(void)
{
 if (RAMenuWindowObj) {
  DisposeObject(RAMenuWindowObj);
  RAMenuWindowObj=NULL;
 }
 RAMenuNameStrObj=NULL;
 RAMenuExecTxtObj=NULL;
 RAMenuSoundTxtObj=NULL;
 RAMenuTitleStrObj=NULL;
 RAMenuSeparatorChkObj=NULL;
 RAMenuSubmenuChkObj=NULL;
 RAMenuParentStrObj=NULL;
 RAMenuCmdkeyStrObj=NULL;
}

static char *DuplicateRAString(Object *strObj)
{
 STRPTR p;
 char *s;

 if (!strObj) return (char *)-1;
 p=NULL;
 GetAttr(STRINGA_TextVal,strObj,(ULONG *)&p);
 if (!p) return NULL;
 s=strdup(p);
 return s ? s : (char *)-1;
}

static void OpenListReqForMenu(ULONG listnum, ULONG gadnum)
{
 struct Window *w;

 w=NULL;
 if (RAMenuWindowObj)
  GetAttr(WINDOW_Window,RAMenuWindowObj,(ULONG *)&w);
 if (!w) return;
 CurrentGadgetNum=gadnum;
 if (OpenListRequester(listnum,w))
  UpdateWindow=UpdateMenuEditWindow;
}

static void *MenuOKGadgetFunc(void)
{
 char *nameStr;
 char *titleStr;
 char *cmdkeyStr;
 char *parentStr;
 ULONG sep;
 ULONG sub;

 nameStr=DuplicateRAString(RAMenuNameStrObj);
 if (nameStr==(char *)-1) {
  FreeMenuNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return (void *)-1;
 }
 if (CurrentNode->mn_Node.ln_Name) free(CurrentNode->mn_Node.ln_Name);
 CurrentNode->mn_Node.ln_Name=nameStr;

 titleStr=RAMenuTitleStrObj ? DuplicateRAString(RAMenuTitleStrObj) : NULL;
 cmdkeyStr=RAMenuCmdkeyStrObj ? DuplicateRAString(RAMenuCmdkeyStrObj) : NULL;
 parentStr=RAMenuParentStrObj ? DuplicateRAString(RAMenuParentStrObj) : NULL;
 if (titleStr==(char *)-1) titleStr=NULL;
 if (cmdkeyStr==(char *)-1) cmdkeyStr=NULL;
 if (CurrentNode->mn_Title) free(CurrentNode->mn_Title);
 if (CurrentNode->mn_CommandKey) free(CurrentNode->mn_CommandKey);
 CurrentNode->mn_Title=(titleStr && *titleStr) ? titleStr : (free(titleStr), (char *)NULL);
 CurrentNode->mn_CommandKey=(cmdkeyStr && *cmdkeyStr) ? cmdkeyStr : (free(cmdkeyStr), (char *)NULL);
 CurrentNode->mn_ParentIndex=(parentStr && *parentStr) ? (LONG)atoi(parentStr) : -1L;
 if (parentStr) free(parentStr);
 sep=0;
 sub=0;
 if (RAMenuSeparatorChkObj) GetAttr(GA_Selected,RAMenuSeparatorChkObj,(ULONG *)&sep);
 if (RAMenuSubmenuChkObj) GetAttr(GA_Selected,RAMenuSubmenuChkObj,(ULONG *)&sub);
 CurrentNode->mn_Flags=(sep ? MOPOF_SEPARATOR : 0u)|(sub ? MOPOF_SUBMENU_PARENT : 0u);
 return (void *)CurrentNode;
}

BOOL HandleRAMenuWindowEvent(Object *windowObj, ULONG result, UWORD code)
{
 (void)windowObj;

 switch (result) {
  case WMHI_CLOSEWINDOW:
   if (CurrentNode) { FreeMenuNode((struct Node *)CurrentNode); CurrentNode=NULL; }
   SubWindowRAReturnData=(void *)-1;
   return TRUE;
  case WMHI_GADGETUP:
   switch (code) {
    case G_MENU_OK:
     SubWindowRAReturnData=MenuOKGadgetFunc();
     return TRUE;
    case G_MENU_CANCEL:
     if (CurrentNode) { FreeMenuNode((struct Node *)CurrentNode); CurrentNode=NULL; }
     SubWindowRAReturnData=(void *)-1;
     return TRUE;
    case G_MENU_EXEC_BUT:
     OpenListReqForMenu(LISTREQ_EXEC,MENU_GAD_EXEC_TXT);
     break;
    case G_MENU_SOUND_BUT:
     OpenListReqForMenu(LISTREQ_SOUND,MENU_GAD_SOUND_TXT);
     break;
   }
   break;
 }
 return FALSE;
}

BOOL OpenMenuEditWindow(struct Node *node, struct Window *parent)
{
 Object *layout;
 struct Window *w;
 char *execStr;
 char *soundStr;
 char *titleStr;
 char *cmdkeyStr;
 ULONG sepChk;
 ULONG subChk;
 ULONG labName;
 ULONG labExec;
 ULONG labSound;
 ULONG labOk;
 ULONG labCancel;

 if (!(CurrentNode=(struct MenuNode *)CopyMenuNode(node)))
  return FALSE;
 execStr=CurrentNode->mn_Exec ? CurrentNode->mn_Exec : "";
 soundStr=CurrentNode->mn_Sound ? CurrentNode->mn_Sound : "";
 titleStr=(CurrentNode->mn_Title && *CurrentNode->mn_Title) ? CurrentNode->mn_Title : "";
 cmdkeyStr=(CurrentNode->mn_CommandKey && *CurrentNode->mn_CommandKey) ? CurrentNode->mn_CommandKey : "";
 sprintf(menu_parent_buf, "%ld", (long)CurrentNode->mn_ParentIndex);
 sepChk=(CurrentNode->mn_Flags&MOPOF_SEPARATOR) ? TRUE : FALSE;
 subChk=(CurrentNode->mn_Flags&MOPOF_SUBMENU_PARENT) ? TRUE : FALSE;
 labName=(ULONG)AppStrings[MSG_WINDOW_NAME_GAD];
 labExec=(ULONG)AppStrings[MSG_WINDOW_EXEC_GAD];
 labSound=(ULONG)AppStrings[MSG_WINDOW_SOUND_GAD];
 labOk=(ULONG)AppStrings[MSG_WINDOW_OK_GAD];
 labCancel=(ULONG)AppStrings[MSG_WINDOW_CANCEL_GAD];

 /* Layout matches last working commit: Name, Exec row, Sound row, OK/Cancel only (no Title/Checkboxes/Parent/Cmdkey in macro to avoid unbalanced parens) */
 layout=VGroupObject,
  LAYOUT_SpaceOuter,TRUE,
  LAYOUT_SpaceInner,TRUE,
  LAYOUT_BevelStyle,BVS_THIN,
  StartMember,
   RAMenuNameStrObj=StringObject,
    GA_ID,G_MENU_NAME,
    GA_RelVerify,TRUE,
    STRINGA_TextVal,CurrentNode->mn_Node.ln_Name,
    STRINGA_MaxChars,SGBUFLEN,
   EndMember,
   MemberLabel(AppStrings[MSG_WINDOW_NAME_GAD]),
  StartMember,
   HGroupObject,
    ButtonObject,GA_ID,G_MENU_EXEC_BUT,GA_RelVerify,TRUE,GA_Text,AppStrings[MSG_WINDOW_EXEC_GAD],ButtonEnd,
    RAMenuExecTxtObj=StringObject,
     GA_ID,G_MENU_EXEC_TXT,GA_ReadOnly,TRUE,STRINGA_TextVal,execStr,
   EndGroup,
   EndMember,
   MemberLabel(AppStrings[MSG_WINDOW_EXEC_GAD]),
  StartMember,
   HGroupObject,
    ButtonObject,GA_ID,G_MENU_SOUND_BUT,GA_RelVerify,TRUE,GA_Text,AppStrings[MSG_WINDOW_SOUND_GAD],ButtonEnd,
    RAMenuSoundTxtObj=StringObject,
     GA_ID,G_MENU_SOUND_TXT,GA_ReadOnly,TRUE,STRINGA_TextVal,soundStr,
   EndGroup,
   EndMember,
   MemberLabel(AppStrings[MSG_WINDOW_SOUND_GAD]),
  StartHGroup,EvenSized,
   StartMember,ButtonObject,GA_ID,G_MENU_OK,GA_RelVerify,TRUE,GA_Text,AppStrings[MSG_WINDOW_OK_GAD],ButtonEnd,
   StartMember,ButtonObject,GA_ID,G_MENU_CANCEL,GA_RelVerify,TRUE,GA_Text,AppStrings[MSG_WINDOW_CANCEL_GAD],ButtonEnd,
  EndGroup,
 EndGroup;

 if (!layout) {
  FreeMenuNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return FALSE;
 }

 RAMenuWindowObj=WindowObject,
  WA_PubScreen,PublicScreen,
  WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_RMBTRAP|WFLG_ACTIVATE,
  WA_Title,AppStrings[MSG_MENUWIN_TITLE],
  WINDOW_RefWindow,parent,
  WINDOW_Position,WPOS_CENTERWINDOW,
  WINDOW_ParentGroup,layout,
 EndWindow;

 if (!RAMenuWindowObj) {
  DisposeObject(layout);
  FreeMenuNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return FALSE;
 }

 if (!(w=(struct Window *)RA_OpenWindow(RAMenuWindowObj))) {
  DisposeObject(RAMenuWindowObj);
  RAMenuWindowObj=NULL;
  RAMenuNameStrObj=NULL;
  RAMenuExecTxtObj=NULL;
  RAMenuSoundTxtObj=NULL;
  RAMenuTitleStrObj=NULL;
  RAMenuSeparatorChkObj=NULL;
  RAMenuSubmenuChkObj=NULL;
  RAMenuParentStrObj=NULL;
  RAMenuCmdkeyStrObj=NULL;
  FreeMenuNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return FALSE;
 }

 CurrentWindow=w;
 SubWindowPort=w->UserPort;
 SubWindowRAObject=RAMenuWindowObj;
 SubWindowRAHandler=HandleRAMenuWindowEvent;
 SubWindowRACloseFunc=CloseRAMenuWindow;

 return TRUE;
}

void UpdateMenuEditWindow(void *data)
{
 struct Window *w;
 char *new;
 char *old;
 Object *txtObj;

 if (data != LREQRET_CANCEL) {
  new=(data == LREQRET_NOSELECT) ? NULL : ((struct Node *)data)->ln_Name;
  if (!new || (new=strdup(new))) {
   if (CurrentGadgetNum==MENU_GAD_EXEC_TXT) {
    old=CurrentNode->mn_Exec;
    CurrentNode->mn_Exec=new;
    txtObj=RAMenuExecTxtObj;
   } else {
    old=CurrentNode->mn_Sound;
    CurrentNode->mn_Sound=new;
    txtObj=RAMenuSoundTxtObj;
   }
   if (old) free(old);
   if (txtObj)
    SetAttrs(txtObj,STRINGA_TextVal,new ? new : "",TAG_END);
  }
 }

 w=NULL;
 if (RAMenuWindowObj)
  GetAttr(WINDOW_Window,RAMenuWindowObj,(ULONG *)&w);
 if (w) {
  SubWindowPort=w->UserPort;
  SubWindowRAObject=RAMenuWindowObj;
  SubWindowRAHandler=HandleRAMenuWindowEvent;
  SubWindowRACloseFunc=CloseRAMenuWindow;
  CurrentWindow=w;
 }
 UpdateWindow=UpdateMainWindow;
}

void *HandleMenuEditWindowIDCMP(struct IntuiMessage *msg)
{
 (void)msg;
 return NULL;
}

/* Forwards compatible: old prefs have only name/exec/sound and no extension;
 * mn_Title/mn_CommandKey stay NULL, mn_Flags 0, mn_ParentIndex -1. */
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
  return NULL;
 mpo=(struct MenuPrefsObject *)buf;
 sbits=mpo->mpo_StringBits;
 ptr=(UBYTE *)&mpo[1];
 chunk_start=(UBYTE *)buf;
 mn->mn_Flags=0;
 mn->mn_ParentIndex=-1;

 if (!(sbits & MOPO_NAME) || (mn->mn_Node.ln_Name=GetConfigStr(&ptr))) {
  if (!(sbits & MOPO_EXEC) || (mn->mn_Exec=GetConfigStr(&ptr))) {
   if (!(sbits & MOPO_SOUND) || (mn->mn_Sound=GetConfigStr(&ptr))) {
    if (!(sbits & MOPO_TITLE) || (mn->mn_Title=GetConfigStr(&ptr))) {
     if (!(sbits & MOPO_CMDKEY) || (mn->mn_CommandKey=GetConfigStr(&ptr))) {
      used=(ULONG)(ptr-chunk_start);
      if (chunk_size>=used+1u) {
       ext_len=*ptr++;
       if (ext_len>=8 && chunk_size>=used+9u) {
        mn->mn_Flags=*(ULONG *)ptr;
        ptr+=4;
        mn->mn_ParentIndex=*(LONG *)ptr;
       }
      }
      return (struct Node *)mn;
     }
    }
   }
  }
 }
 FreeMenuNode((struct Node *)mn);
 return NULL;
}

BOOL WriteMenuNode(struct IFFHandle *iff, UBYTE *buf, struct Node *node)
{
 struct MenuNode *mn=(struct MenuNode *) node;
 struct MenuPrefsObject *mpo=(struct MenuPrefsObject *) buf;
 ULONG sbits=0;
 UBYTE *ptr=(UBYTE *) &mpo[1];
 ULONG chunk_len;

 if (PutConfigStr(mn->mn_Node.ln_Name,&ptr)) sbits|=MOPO_NAME;
 if (PutConfigStr(mn->mn_Exec,&ptr)) sbits|=MOPO_EXEC;
 if (PutConfigStr(mn->mn_Sound,&ptr)) sbits|=MOPO_SOUND;
 if (mn->mn_Title && *mn->mn_Title && PutConfigStr(mn->mn_Title,&ptr))
  sbits|=MOPO_TITLE;
 if (mn->mn_CommandKey && *mn->mn_CommandKey && PutConfigStr(mn->mn_CommandKey,&ptr))
  sbits|=MOPO_CMDKEY;
 mpo->mpo_StringBits=sbits;

 if (mn->mn_Flags || mn->mn_ParentIndex!=-1) {
  *ptr++=(UBYTE)8;
  *(ULONG *)ptr=mn->mn_Flags;
  ptr+=4;
  *(LONG *)ptr=mn->mn_ParentIndex;
  ptr+=4;
 }
 chunk_len=(ULONG)(ptr-buf);
 if (PushChunk(iff,0,ID_TMMO,chunk_len)) return FALSE;
 if (WriteChunkBytes(iff,buf,chunk_len)!=chunk_len) return FALSE;
 if (PopChunk(iff)) return FALSE;
 return TRUE;
}
