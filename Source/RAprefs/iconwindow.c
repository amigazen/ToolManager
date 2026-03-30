/*
 * iconwindow.c  RAprefs
 *
 * Icon edit window: ReAction GUI (name, Exec/Image/Sound buttons+text,
 * XPos/YPos integers, Position chooser, ShowName checkbox, OK/Cancel).
 * Integrates via SubWindowRAObject; UpdateIconEditWindow restores RA state.
 * Position opens/closes move window via OpenMoveWindowRA.
 *
 * (c) 1990-1993 Stefan Becker
 */

#include "RAprefsConf.h"

struct IconNode {
                 struct Node  in_Node;
                 ULONG        in_Flags;
                 char        *in_Exec;
                 char        *in_Image;
                 char        *in_Sound;
                 LONG         in_XPos;
                 LONG         in_YPos;
                };

#define G_ICON_NAME    1
#define G_ICON_EXEC_BUT 2
#define G_ICON_EXEC_TXT 3
#define G_ICON_IMAGE_BUT 4
#define G_ICON_IMAGE_TXT 5
#define G_ICON_SOUND_BUT 6
#define G_ICON_SOUND_TXT 7
#define G_ICON_XPOS    8
#define G_ICON_YPOS    9
#define G_ICON_POSITION 10
#define G_ICON_SHOWNAME 11
#define G_ICON_OK     12
#define G_ICON_CANCEL  13

#define ICON_GAD_EXEC_TXT  3
#define ICON_GAD_IMAGE_TXT 5
#define ICON_GAD_SOUND_TXT 7

static struct IconNode *CurrentNode;
static ULONG CurrentGadgetNum;
static Object *RAIconWindowObj;
static Object *RAIconNameStrObj;
static Object *RAIconExecTxtObj;
static Object *RAIconImageTxtObj;
static Object *RAIconSoundTxtObj;
static Object *RAIconXIntObj;
static Object *RAIconYIntObj;
static Object *RAIconPosChooserObj;
static Object *RAIconShowNameObj;
static struct List *IconPosChooserLabels;
static STRPTR iconPosLabels[3];

struct IClass *STRING_GetClass(void);

void InitIconEditWindow(UWORD left, UWORD fheight)
{
 (void)left;
 (void)fheight;
 iconPosLabels[0]=AppStrings[MSG_WINDOW_POSITION_OPEN_LABEL];
 iconPosLabels[1]=AppStrings[MSG_WINDOW_POSITION_CLOSE_LABEL];
 iconPosLabels[2]=NULL;
 if (IconPosChooserLabels) FreeChooserLabels(IconPosChooserLabels);
 IconPosChooserLabels=ChooserLabelsA(iconPosLabels);
}

void FreeIconNode(struct Node *node)
{
 struct IconNode *in=(struct IconNode *) node;
 char *s;

 if (s=in->in_Node.ln_Name) free(s);
 if (s=in->in_Exec) free(s);
 if (s=in->in_Image) free(s);
 if (s=in->in_Sound) free(s);
 FreeMem(in,sizeof(struct IconNode));
}

struct Node *CopyIconNode(struct Node *node)
{
 struct IconNode *in,*orignode=(struct IconNode *) node;

 if (in=AllocMem(sizeof(struct IconNode),MEMF_PUBLIC|MEMF_CLEAR)) {
  if (orignode) {
   if ((!orignode->in_Node.ln_Name || (in->in_Node.ln_Name=strdup(orignode->in_Node.ln_Name))) &&
       (!orignode->in_Exec || (in->in_Exec=strdup(orignode->in_Exec))) &&
       (!orignode->in_Image || (in->in_Image=strdup(orignode->in_Image))) &&
       (!orignode->in_Sound || (in->in_Sound=strdup(orignode->in_Sound)))) {
    in->in_Flags=orignode->in_Flags;
    in->in_XPos=orignode->in_XPos;
    in->in_YPos=orignode->in_YPos;
    return (struct Node *)in;
   }
  } else {
   if (in->in_Node.ln_Name=strdup(AppStrings[MSG_ICONWIN_NEWNAME])) {
    in->in_Flags=ICPOF_SHOWNAME;
    return (struct Node *)in;
   }
  }
  FreeIconNode((struct Node *)in);
 }
 return NULL;
}

struct Node *CreateIconNode(char *name)
{
 struct IconNode *in;

 if (in=AllocMem(sizeof(struct IconNode),MEMF_PUBLIC|MEMF_CLEAR)) {
  if ((in->in_Node.ln_Name=strdup(name)) &&
      (in->in_Exec=strdup(name)) &&
      (in->in_Image=strdup(name))) {
   in->in_Flags=ICPOF_SHOWNAME;
   return (struct Node *)in;
  }
  FreeIconNode((struct Node *)in);
 }
 return NULL;
}

static void CloseRAIconWindow(void)
{
 if (RAIconWindowObj) {
  DisposeObject(RAIconWindowObj);
  RAIconWindowObj=NULL;
 }
 RAIconNameStrObj=NULL;
 RAIconExecTxtObj=NULL;
 RAIconImageTxtObj=NULL;
 RAIconSoundTxtObj=NULL;
 RAIconXIntObj=NULL;
 RAIconYIntObj=NULL;
 RAIconPosChooserObj=NULL;
 RAIconShowNameObj=NULL;
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

static void OpenListReqForIcon(ULONG listnum, ULONG gadnum)
{
 struct Window *w;

 w=NULL;
 if (RAIconWindowObj) GetAttr(WINDOW_Window,RAIconWindowObj,(ULONG *)&w);
 if (!w) return;
 CurrentGadgetNum=gadnum;
 if (OpenListRequester(listnum,w))
  UpdateWindow=UpdateIconEditWindow;
}

static void DoPositionButton(void)
{
 struct Window *w;
 WORD sel;

 w=NULL;
 if (RAIconWindowObj) GetAttr(WINDOW_Window,RAIconWindowObj,(ULONG *)&w);
 if (!w) return;
 sel=0;
 if (RAIconPosChooserObj) GetAttr(CHOOSER_Selected,RAIconPosChooserObj,(ULONG *)&sel);
 if (sel==0) {
  MoveWindowOffX=WBXOffset;
  MoveWindowOffY=WBYOffset;
  OpenMoveWindowRA(w,RAIconXIntObj,RAIconYIntObj);
 } else
  CloseMoveWindow();
}

static void *IconOKGadgetFunc(void)
{
 char *nameStr;
 LONG xval;
 LONG yval;
 ULONG showVal;

 /* Match original: only (char*)-1 is error; NULL name allowed */
 nameStr=DuplicateRAString(RAIconNameStrObj);
 if (nameStr==(char *)-1) {
  FreeIconNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return (void *)-1;
 }
 if (CurrentNode->in_Node.ln_Name) free(CurrentNode->in_Node.ln_Name);
 CurrentNode->in_Node.ln_Name=nameStr;
 xval=0;
 yval=0;
 if (RAIconXIntObj) GetAttr(INTEGER_Number,RAIconXIntObj,(ULONG *)&xval);
 if (RAIconYIntObj) GetAttr(INTEGER_Number,RAIconYIntObj,(ULONG *)&yval);
 showVal=0;
 if (RAIconShowNameObj) GetAttr(GA_Selected,RAIconShowNameObj,(ULONG *)&showVal);
 CurrentNode->in_XPos=(LONG)xval;
 CurrentNode->in_YPos=(LONG)yval;
 CurrentNode->in_Flags=(CurrentNode->in_Flags & ~ICPOF_SHOWNAME) | (showVal ? ICPOF_SHOWNAME : 0);
 return (void *)CurrentNode;
}

BOOL HandleRAIconWindowEvent(Object *windowObj, ULONG result, UWORD code)
{
 (void)windowObj;

 switch (result) {
  case WMHI_CLOSEWINDOW:
   if (CurrentNode) { FreeIconNode((struct Node *)CurrentNode); CurrentNode=NULL; }
   SubWindowRAReturnData=(void *)-1;
   return TRUE;
  case WMHI_GADGETUP:
   switch (code) {
    case G_ICON_OK:
     SubWindowRAReturnData=IconOKGadgetFunc();
     return TRUE;
    case G_ICON_CANCEL:
     if (CurrentNode) { FreeIconNode((struct Node *)CurrentNode); CurrentNode=NULL; }
     SubWindowRAReturnData=(void *)-1;
     return TRUE;
    case G_ICON_EXEC_BUT:
     OpenListReqForIcon(LISTREQ_EXEC,ICON_GAD_EXEC_TXT);
     break;
    case G_ICON_IMAGE_BUT:
     OpenListReqForIcon(LISTREQ_IMAGE,ICON_GAD_IMAGE_TXT);
     break;
    case G_ICON_SOUND_BUT:
     OpenListReqForIcon(LISTREQ_SOUND,ICON_GAD_SOUND_TXT);
     break;
    case G_ICON_POSITION:
     DoPositionButton();
     break;
    case G_ICON_SHOWNAME: {
     ULONG v;
     v=0;
     if (RAIconShowNameObj) GetAttr(GA_Selected,RAIconShowNameObj,(ULONG *)&v);
     CurrentNode->in_Flags=(CurrentNode->in_Flags & ~ICPOF_SHOWNAME) | (v ? ICPOF_SHOWNAME : 0);
     }
     break;
   }
   break;
 }
 return FALSE;
}

BOOL OpenIconEditWindow(struct Node *node, struct Window *parent)
{
 Object *layout;
 struct Window *w;
 char *execStr;
 char *imageStr;
 char *soundStr;

 if (!IconPosChooserLabels) return FALSE;
 if (!(CurrentNode=(struct IconNode *)CopyIconNode(node)))
  return FALSE;
 execStr=CurrentNode->in_Exec ? CurrentNode->in_Exec : "";
 imageStr=CurrentNode->in_Image ? CurrentNode->in_Image : "";
 soundStr=CurrentNode->in_Sound ? CurrentNode->in_Sound : "";

 layout=VGroupObject,
  LAYOUT_SpaceOuter,TRUE,
  LAYOUT_SpaceInner,TRUE,
  LAYOUT_BevelStyle,BVS_THIN,
  StartMember,RAIconNameStrObj=StringObject,
   GA_ID,G_ICON_NAME,GA_RelVerify,TRUE,STRINGA_TextVal,CurrentNode->in_Node.ln_Name,STRINGA_MaxChars,SGBUFLEN,
  EndMember,MemberLabel(AppStrings[MSG_WINDOW_NAME_GAD]),
  StartMember,HGroupObject,
   ButtonObject,GA_ID,G_ICON_EXEC_BUT,GA_RelVerify,TRUE,GA_Text,AppStrings[MSG_WINDOW_EXEC_GAD],ButtonEnd,
   RAIconExecTxtObj=StringObject,GA_ID,G_ICON_EXEC_TXT,GA_ReadOnly,TRUE,STRINGA_TextVal,execStr,
  EndGroup,EndMember,MemberLabel(AppStrings[MSG_WINDOW_EXEC_GAD]),
  StartMember,HGroupObject,
   ButtonObject,GA_ID,G_ICON_IMAGE_BUT,GA_RelVerify,TRUE,GA_Text,AppStrings[MSG_WINDOW_IMAGE_GAD],ButtonEnd,
   RAIconImageTxtObj=StringObject,GA_ID,G_ICON_IMAGE_TXT,GA_ReadOnly,TRUE,STRINGA_TextVal,imageStr,
  EndGroup,EndMember,MemberLabel(AppStrings[MSG_WINDOW_IMAGE_GAD]),
  StartMember,HGroupObject,
   ButtonObject,GA_ID,G_ICON_SOUND_BUT,GA_RelVerify,TRUE,GA_Text,AppStrings[MSG_WINDOW_SOUND_GAD],ButtonEnd,
   RAIconSoundTxtObj=StringObject,GA_ID,G_ICON_SOUND_TXT,GA_ReadOnly,TRUE,STRINGA_TextVal,soundStr,
  EndGroup,EndMember,MemberLabel(AppStrings[MSG_WINDOW_SOUND_GAD]),
  StartMember,RAIconXIntObj=IntegerObject,
   GA_ID,G_ICON_XPOS,GA_RelVerify,TRUE,INTEGER_Number,CurrentNode->in_XPos,INTEGER_MaxChars,10,
  EndMember,MemberLabel(AppStrings[MSG_WINDOW_LEFTEDGE_GAD]),
  StartMember,RAIconYIntObj=IntegerObject,
   GA_ID,G_ICON_YPOS,GA_RelVerify,TRUE,INTEGER_Number,CurrentNode->in_YPos,INTEGER_MaxChars,10,
  EndMember,MemberLabel(AppStrings[MSG_WINDOW_TOPEDGE_GAD]),
  StartMember,RAIconPosChooserObj=ChooserObject,
   GA_ID,G_ICON_POSITION,GA_RelVerify,TRUE,CHOOSER_PopUp,TRUE,CHOOSER_Labels,IconPosChooserLabels,CHOOSER_Selected,0,
  EndMember,MemberLabel(AppStrings[MSG_WINDOW_POSITION_OPEN_LABEL]),
  StartMember,RAIconShowNameObj=CheckBoxObject,
   GA_ID,G_ICON_SHOWNAME,GA_RelVerify,TRUE,GA_Selected,(CurrentNode->in_Flags & ICPOF_SHOWNAME) ? TRUE : FALSE,GA_Text,AppStrings[MSG_ICONWIN_SHOWNAME_GAD],
  EndMember,
  StartHGroup,EvenSized,
   StartMember,ButtonObject,GA_ID,G_ICON_OK,GA_RelVerify,TRUE,GA_Text,AppStrings[MSG_WINDOW_OK_GAD],ButtonEnd,
   StartMember,ButtonObject,GA_ID,G_ICON_CANCEL,GA_RelVerify,TRUE,GA_Text,AppStrings[MSG_WINDOW_CANCEL_GAD],ButtonEnd,
  EndGroup,
 EndGroup;

 if (!layout) {
  FreeIconNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return FALSE;
 }

 RAIconWindowObj=WindowObject,
  WA_PubScreen,PublicScreen,
  WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_RMBTRAP|WFLG_ACTIVATE,
  WA_Title,AppStrings[MSG_ICONWIN_TITLE],
  WINDOW_RefWindow,parent,
  WINDOW_Position,WPOS_CENTERWINDOW,
  WINDOW_ParentGroup,layout,
 EndWindow;

 if (!RAIconWindowObj) {
  DisposeObject(layout);
  FreeIconNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return FALSE;
 }

 if (!(w=(struct Window *)RA_OpenWindow(RAIconWindowObj))) {
  DisposeObject(RAIconWindowObj);
  RAIconWindowObj=NULL;
  RAIconNameStrObj=NULL;
  RAIconExecTxtObj=NULL;
  RAIconImageTxtObj=NULL;
  RAIconSoundTxtObj=NULL;
  RAIconXIntObj=NULL;
  RAIconYIntObj=NULL;
  RAIconPosChooserObj=NULL;
  RAIconShowNameObj=NULL;
  FreeIconNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return FALSE;
 }

 CurrentWindow=w;
 SubWindowPort=w->UserPort;
 SubWindowRAObject=RAIconWindowObj;
 SubWindowRAHandler=HandleRAIconWindowEvent;
 SubWindowRACloseFunc=CloseRAIconWindow;

 return TRUE;
}

void UpdateIconEditWindow(void *data)
{
 struct Window *w;
 char *new;
 char *old;
 Object *txtObj;

 if (data != LREQRET_CANCEL) {
  new=(data == LREQRET_NOSELECT) ? NULL : ((struct Node *)data)->ln_Name;
  if (!new || (new=strdup(new))) {
   switch (CurrentGadgetNum) {
    case ICON_GAD_EXEC_TXT:
     old=CurrentNode->in_Exec;
     CurrentNode->in_Exec=new;
     txtObj=RAIconExecTxtObj;
     break;
    case ICON_GAD_IMAGE_TXT:
     old=CurrentNode->in_Image;
     CurrentNode->in_Image=new;
     txtObj=RAIconImageTxtObj;
     break;
    case ICON_GAD_SOUND_TXT:
     old=CurrentNode->in_Sound;
     CurrentNode->in_Sound=new;
     txtObj=RAIconSoundTxtObj;
     break;
    default:
     old=NULL;
     txtObj=NULL;
     break;
   }
   if (old) free(old);
   if (txtObj) SetAttrs(txtObj,STRINGA_TextVal,new ? new : "",TAG_END);
  }
 }
 w=NULL;
 if (RAIconWindowObj) GetAttr(WINDOW_Window,RAIconWindowObj,(ULONG *)&w);
 if (w) {
  SubWindowPort=w->UserPort;
  SubWindowRAObject=RAIconWindowObj;
  SubWindowRAHandler=HandleRAIconWindowEvent;
  SubWindowRACloseFunc=CloseRAIconWindow;
  CurrentWindow=w;
 }
 UpdateWindow=UpdateMainWindow;
}

void *HandleIconEditWindowIDCMP(struct IntuiMessage *msg)
{
 (void)msg;
 return NULL;
}

struct Node *ReadIconNode(UBYTE *buf, ULONG size)
{
 struct IconNode *in;
 struct IconPrefsObject *ipo;
 ULONG sbits;
 UBYTE *ptr;

 (void)size;
 ipo=(struct IconPrefsObject *)buf;
 sbits=ipo->ipo_StringBits;
 ptr=(UBYTE *)&ipo[1];

 if (in=AllocMem(sizeof(struct IconNode),MEMF_PUBLIC|MEMF_CLEAR)) {
  if ((!(sbits & ICPO_NAME) || (in->in_Node.ln_Name=GetConfigStr(&ptr))) &&
      (!(sbits & ICPO_EXEC) || (in->in_Exec=GetConfigStr(&ptr))) &&
      (!(sbits & ICPO_IMAGE) || (in->in_Image=GetConfigStr(&ptr))) &&
      (!(sbits & ICPO_SOUND) || (in->in_Sound=GetConfigStr(&ptr)))) {
   in->in_Flags=ipo->ipo_Flags;
   in->in_XPos=ipo->ipo_XPos;
   in->in_YPos=ipo->ipo_YPos;
   return (struct Node *)in;
  }
  FreeIconNode((struct Node *)in);
 }
 return NULL;
}

BOOL WriteIconNode(struct IFFHandle *iff, UBYTE *buf, struct Node *node)
{
 struct IconNode *in=(struct IconNode *) node;
 struct IconPrefsObject *ipo=(struct IconPrefsObject *) buf;
 ULONG sbits=0;
 UBYTE *ptr=(UBYTE *) &ipo[1];

 if (PutConfigStr(in->in_Node.ln_Name,&ptr)) sbits|=ICPO_NAME;
 if (PutConfigStr(in->in_Exec,&ptr)) sbits|=ICPO_EXEC;
 if (PutConfigStr(in->in_Image,&ptr)) sbits|=ICPO_IMAGE;
 if (PutConfigStr(in->in_Sound,&ptr)) sbits|=ICPO_SOUND;
 ipo->ipo_StringBits=sbits;
 ipo->ipo_Flags=in->in_Flags;
 ipo->ipo_XPos=in->in_XPos;
 ipo->ipo_YPos=in->in_YPos;
 sbits=ptr-buf;

 if (PushChunk(iff,0,ID_TMIC,sbits)) return FALSE;
 if (WriteChunkBytes(iff,buf,sbits)!=sbits) return FALSE;
 if (PopChunk(iff)) return FALSE;
 return TRUE;
}
