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

struct MenuNode {
                  struct Node  mn_Node;
                  char        *mn_Exec;
                  char        *mn_Sound;
                 };

#define G_MENU_NAME   1
#define G_MENU_EXEC_BUT 2
#define G_MENU_EXEC_TXT 3
#define G_MENU_SOUND_BUT 4
#define G_MENU_SOUND_TXT 5
#define G_MENU_OK     6
#define G_MENU_CANCEL  7

#define MENU_GAD_EXEC_TXT  3
#define MENU_GAD_SOUND_TXT 5

static struct MenuNode *CurrentNode;
static ULONG CurrentGadgetNum;
static Object *RAMenuWindowObj;
static Object *RAMenuNameStrObj;
static Object *RAMenuExecTxtObj;
static Object *RAMenuSoundTxtObj;
static struct Requester DummyReq;

struct IClass *STRING_GetClass(void);

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
 FreeMem(mn,sizeof(struct MenuNode));
}

struct Node *CopyMenuNode(struct Node *node)
{
 struct MenuNode *mn,*orignode=(struct MenuNode *) node;

 if (mn=AllocMem(sizeof(struct MenuNode),MEMF_PUBLIC|MEMF_CLEAR)) {
  if (orignode) {
   if ((!orignode->mn_Node.ln_Name || (mn->mn_Node.ln_Name=strdup(orignode->mn_Node.ln_Name))) &&
       (!orignode->mn_Exec || (mn->mn_Exec=strdup(orignode->mn_Exec))) &&
       (!orignode->mn_Sound || (mn->mn_Sound=strdup(orignode->mn_Sound))))
    return (struct Node *)mn;
  } else {
   if (mn->mn_Node.ln_Name=strdup(AppStrings[MSG_MENUWIN_NEWNAME]))
    return (struct Node *)mn;
  }
  FreeMenuNode((struct Node *)mn);
 }
 return NULL;
}

struct Node *CreateMenuNode(char *name)
{
 struct MenuNode *mn;

 if (mn=AllocMem(sizeof(struct MenuNode),MEMF_PUBLIC|MEMF_CLEAR)) {
  if ((mn->mn_Node.ln_Name=strdup(name)) && (mn->mn_Exec=strdup(name)))
   return (struct Node *)mn;
  FreeMenuNode((struct Node *)mn);
 }
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
 if (CurrentNode) {
  FreeMenuNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
 }
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

 /* Match original: only (char*)-1 is error; NULL = empty name allowed */
 nameStr=DuplicateRAString(RAMenuNameStrObj);
 if (nameStr==(char *)-1) {
  FreeMenuNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return (void *)-1;
 }
 if (CurrentNode->mn_Node.ln_Name) free(CurrentNode->mn_Node.ln_Name);
 CurrentNode->mn_Node.ln_Name=nameStr;
 return (void *)CurrentNode;
}

BOOL HandleRAMenuWindowEvent(Object *windowObj, ULONG result, UWORD code)
{
 (void)windowObj;

 switch (result) {
  case WMHI_CLOSEWINDOW:
   SubWindowRAReturnData=(void *)-1;
   return TRUE;
  case WMHI_GADGETUP:
   switch (code) {
    case G_MENU_OK:
     SubWindowRAReturnData=MenuOKGadgetFunc();
     return TRUE;
    case G_MENU_CANCEL:
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

 if (!(CurrentNode=(struct MenuNode *)CopyMenuNode(node)))
  return FALSE;
 execStr=CurrentNode->mn_Exec ? CurrentNode->mn_Exec : "";
 soundStr=CurrentNode->mn_Sound ? CurrentNode->mn_Sound : "";

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

struct Node *ReadMenuNode(UBYTE *buf)
{
 struct MenuNode *mn;

 if (mn=AllocMem(sizeof(struct MenuNode),MEMF_PUBLIC|MEMF_CLEAR)) {
  struct MenuPrefsObject *mpo=(struct MenuPrefsObject *) buf;
  ULONG sbits=mpo->mpo_StringBits;
  UBYTE *ptr=(UBYTE *) &mpo[1];

  if ((!(sbits & MOPO_NAME) || (mn->mn_Node.ln_Name=GetConfigStr(&ptr))) &&
      (!(sbits & MOPO_EXEC) || (mn->mn_Exec=GetConfigStr(&ptr))) &&
      (!(sbits & MOPO_SOUND) || (mn->mn_Sound=GetConfigStr(&ptr))))
   return (struct Node *)mn;
  FreeMenuNode((struct Node *)mn);
 }
 return NULL;
}

BOOL WriteMenuNode(struct IFFHandle *iff, UBYTE *buf, struct Node *node)
{
 struct MenuNode *mn=(struct MenuNode *) node;
 struct MenuPrefsObject *mpo=(struct MenuPrefsObject *) buf;
 ULONG sbits=0;
 UBYTE *ptr=(UBYTE *) &mpo[1];

 if (PutConfigStr(mn->mn_Node.ln_Name,&ptr)) sbits|=MOPO_NAME;
 if (PutConfigStr(mn->mn_Exec,&ptr)) sbits|=MOPO_EXEC;
 if (PutConfigStr(mn->mn_Sound,&ptr)) sbits|=MOPO_SOUND;
 mpo->mpo_StringBits=sbits;
 sbits=ptr-buf;

 if (PushChunk(iff,0,ID_TMMO,sbits)) return FALSE;
 if (WriteChunkBytes(iff,buf,sbits)!=sbits) return FALSE;
 if (PopChunk(iff)) return FALSE;
 return TRUE;
}
