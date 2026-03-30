/*
 * execwindow.c  RAprefs
 *
 * Exec edit window: ReAction GUI (name, exec type chooser, command/hotkey/
 * stack/priority/delay, curdir/path/output/pubscreen buttons+strings,
 * args/tofront checkboxes, OK/Cancel). File/list requesters unchanged.
 * Integrates via SubWindowRAObject; UpdateExecEditWindow restores RA state.
 *
 * (c) 1990-1993 Stefan Becker
 */

#include "RAprefsConf.h"

struct IClass *INTEGER_GetClass(void);
struct IClass *CHECKBOX_GetClass(void);

struct ExecNode {
                 struct Node  en_Node;
                 ULONG        en_Flags;
                 UWORD        en_ExecType;
                 WORD         en_Priority;
                 LONG         en_Delay;
                 ULONG        en_Stack;
                 char        *en_Command;
                 char        *en_CurrentDir;
                 char        *en_HotKey;
                 char        *en_Output;
                 char        *en_Path;
                 char        *en_PubScreen;
                };

#define G_EXEC_NAME      1
#define G_EXEC_TYPE      2
#define G_EXEC_CMD_BUT   3
#define G_EXEC_CMD_STR   4
#define G_EXEC_HOTKEY    5
#define G_EXEC_STACK     6
#define G_EXEC_PRIO      7
#define G_EXEC_DELAY     8
#define G_EXEC_CURDIR_BUT 9
#define G_EXEC_CURDIR_STR 10
#define G_EXEC_PATH_BUT  11
#define G_EXEC_PATH_STR  12
#define G_EXEC_OUTPUT_BUT 13
#define G_EXEC_OUTPUT_STR 14
#define G_EXEC_PSCREEN_BUT 15
#define G_EXEC_PSCREEN_STR 16
#define G_EXEC_ARGS      17
#define G_EXEC_TOFRONT   18
#define G_EXEC_OK        19
#define G_EXEC_CANCEL    20

static struct ExecNode *CurrentNode;
static ULONG CurrentGadgetNum;
static Object *RAExecWindowObj;
static Object *RAExecNameStrObj;
static Object *RAExecTypeChooserObj;
static Object *RAExecCmdStrObj;
static Object *RAExecHotkeyStrObj;
static Object *RAExecStackObj;
static Object *RAExecPrioObj;
static Object *RAExecDelayObj;
static Object *RAExecCurdirStrObj;
static Object *RAExecPathStrObj;
static Object *RAExecOutputStrObj;
static Object *RAExecPscreenStrObj;
static Object *RAExecArgsObj;
static Object *RAExecToFrontObj;
static void *RAExecAppWindow;
static struct Requester DummyReq;
static struct List *ExecTypeChooserLabels;
static STRPTR execTypeLabels[8];

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

static void DoCommandButton(void)
{
 struct Window *w;
 STRPTR oldFile;
 char *file;

 w=NULL;
 if (RAExecWindowObj) GetAttr(WINDOW_Window,RAExecWindowObj,(ULONG *)&w);
 if (!w) return;
 if (CurrentNode->en_ExecType==TMET_Dock) {
  CurrentGadgetNum=G_EXEC_CMD_STR;
  if (OpenListRequester(LISTREQ_DOCK,w))
   UpdateWindow=UpdateExecEditWindow;
  return;
 }
 oldFile=NULL;
 if (RAExecCmdStrObj) GetAttr(STRINGA_TextVal,RAExecCmdStrObj,(ULONG *)&oldFile);
 FileReqParms.frp_Title=AppStrings[MSG_FILEREQ_TITLE_FILE];
 FileReqParms.frp_Flags2=FRF_REJECTICONS;
 FileReqParms.frp_OldFile=oldFile ? oldFile : (STRPTR)"";
 FileReqParms.frp_Window=w;
 if (file=OpenFileRequester(&DummyReq)) {
  if (RAExecCmdStrObj) SetAttrs(RAExecCmdStrObj,STRINGA_TextVal,file,TAG_END);
  free(file);
 }
}

static void DoCurDirButton(void)
{
 struct Window *w;
 STRPTR oldFile;
 char *file;

 w=NULL;
 if (RAExecWindowObj) GetAttr(WINDOW_Window,RAExecWindowObj,(ULONG *)&w);
 if (!w) return;
 oldFile=NULL;
 if (RAExecCurdirStrObj) GetAttr(STRINGA_TextVal,RAExecCurdirStrObj,(ULONG *)&oldFile);
 FileReqParms.frp_Title=AppStrings[MSG_FILEREQ_TITLE_DRAWER];
 FileReqParms.frp_Flags2=FRF_DRAWERSONLY|FRF_REJECTICONS;
 FileReqParms.frp_OldFile=oldFile ? oldFile : (STRPTR)"";
 FileReqParms.frp_Window=w;
 if (file=OpenFileRequester(&DummyReq)) {
  if (RAExecCurdirStrObj) SetAttrs(RAExecCurdirStrObj,STRINGA_TextVal,file,TAG_END);
  free(file);
 }
}

static void DoPathButton(void)
{
 struct Window *w;
 STRPTR oldPath;
 char *file;
 char *path;
 ULONG len;

 w=NULL;
 if (RAExecWindowObj) GetAttr(WINDOW_Window,RAExecWindowObj,(ULONG *)&w);
 if (!w) return;
 oldPath=NULL;
 if (RAExecPathStrObj) GetAttr(STRINGA_TextVal,RAExecPathStrObj,(ULONG *)&oldPath);
 FileReqParms.frp_Title=AppStrings[MSG_FILEREQ_TITLE_DRAWER];
 FileReqParms.frp_Flags2=FRF_DRAWERSONLY|FRF_REJECTICONS;
 FileReqParms.frp_OldFile="";
 FileReqParms.frp_Window=w;
 if (file=OpenFileRequester(&DummyReq)) {
  len=oldPath ? strlen(oldPath) : 0;
  path=malloc(len+strlen(file)+3);
  if (path) {
   if (len) { strcpy(path,oldPath); path[len]=';'; strcpy(path+len+1,file); }
   else strcpy(path,file);
   if (RAExecPathStrObj) SetAttrs(RAExecPathStrObj,STRINGA_TextVal,path,TAG_END);
   free(path);
  }
  free(file);
 }
}

static void DoOutputButton(void)
{
 struct Window *w;
 STRPTR oldFile;
 char *file;

 w=NULL;
 if (RAExecWindowObj) GetAttr(WINDOW_Window,RAExecWindowObj,(ULONG *)&w);
 if (!w) return;
 oldFile=NULL;
 if (RAExecOutputStrObj) GetAttr(STRINGA_TextVal,RAExecOutputStrObj,(ULONG *)&oldFile);
 FileReqParms.frp_Title=AppStrings[MSG_FILEREQ_TITLE_FILE];
 FileReqParms.frp_Flags2=FRF_REJECTICONS;
 FileReqParms.frp_OldFile=oldFile ? oldFile : (STRPTR)"";
 FileReqParms.frp_Window=w;
 if (file=OpenFileRequester(&DummyReq)) {
  if (RAExecOutputStrObj) SetAttrs(RAExecOutputStrObj,STRINGA_TextVal,file,TAG_END);
  free(file);
 }
}

static void DoPubScreenButton(void)
{
 struct Window *w;

 w=NULL;
 if (RAExecWindowObj) GetAttr(WINDOW_Window,RAExecWindowObj,(ULONG *)&w);
 if (!w) return;
 CurrentGadgetNum=G_EXEC_PSCREEN_STR;
 if (OpenListRequester(LISTREQ_PUBSC,w))
  UpdateWindow=UpdateExecEditWindow;
}

void InitExecEditWindow(UWORD left, UWORD fheight)
{
 (void)left;
 (void)fheight;
 execTypeLabels[0]="CLI";
 execTypeLabels[1]="WB";
 execTypeLabels[2]="ARexx";
 execTypeLabels[3]="Dock";
 execTypeLabels[4]="Hot Key";
 execTypeLabels[5]="Network";
 execTypeLabels[6]=NULL;
 if (ExecTypeChooserLabels) FreeChooserLabels(ExecTypeChooserLabels);
 ExecTypeChooserLabels=ChooserLabelsA(execTypeLabels);
 InitRequester(&DummyReq);
}

void FreeExecNode(struct Node *node)
{
 struct ExecNode *en=(struct ExecNode *) node;
 char *s;

 if (s=en->en_Node.ln_Name) free(s);
 if (s=en->en_Command) free(s);
 if (s=en->en_CurrentDir) free(s);
 if (s=en->en_HotKey) free(s);
 if (s=en->en_Output) free(s);
 if (s=en->en_Path) free(s);
 if (s=en->en_PubScreen) free(s);
 FreeMem(en,sizeof(struct ExecNode));
}

struct Node *CopyExecNode(struct Node *node)
{
 struct ExecNode *en,*orignode=(struct ExecNode *) node;

 if (en=AllocMem(sizeof(struct ExecNode),MEMF_PUBLIC|MEMF_CLEAR)) {
  if (orignode) {
   if ((!orignode->en_Node.ln_Name || (en->en_Node.ln_Name=strdup(orignode->en_Node.ln_Name))) &&
       (!orignode->en_Command || (en->en_Command=strdup(orignode->en_Command))) &&
       (!orignode->en_CurrentDir || (en->en_CurrentDir=strdup(orignode->en_CurrentDir))) &&
       (!orignode->en_HotKey || (en->en_HotKey=strdup(orignode->en_HotKey))) &&
       (!orignode->en_Output || (en->en_Output=strdup(orignode->en_Output))) &&
       (!orignode->en_Path || (en->en_Path=strdup(orignode->en_Path))) &&
       (!orignode->en_PubScreen || (en->en_PubScreen=strdup(orignode->en_PubScreen)))) {
    en->en_ExecType=orignode->en_ExecType;
    en->en_Flags=orignode->en_Flags;
    en->en_Stack=orignode->en_Stack;
    en->en_Priority=orignode->en_Priority;
    en->en_Delay=orignode->en_Delay;
    return (struct Node *)en;
   }
  } else {
   if (en->en_Node.ln_Name=strdup(AppStrings[MSG_EXECWIN_NEWNAME])) {
    en->en_Flags=EXPOF_ARGS;
    en->en_Stack=4096;
    return (struct Node *)en;
   }
  }
  FreeExecNode((struct Node *)en);
 }
 return NULL;
}

struct Node *CreateExecNode(char *name, struct WBArg *wa)
{
 struct ExecNode *en;
 char *dirbuf;

 if (!(en=AllocMem(sizeof(struct ExecNode),MEMF_PUBLIC|MEMF_CLEAR)))
  return NULL;
 if ((en->en_Node.ln_Name=strdup(name)) && (en->en_Command=strdup(wa->wa_Name)) &&
     (dirbuf=malloc(4096))) {
  if (NameFromLock(wa->wa_Lock,dirbuf,4096) && (en->en_CurrentDir=strdup(dirbuf))) {
   en->en_ExecType=TMET_WB;
   en->en_Flags=EXPOF_ARGS;
   en->en_Stack=4096;
   free(dirbuf);
   return (struct Node *)en;
  }
  free(dirbuf);
 }
 FreeExecNode((struct Node *)en);
 return NULL;
}

static void CloseRAExecWindow(void)
{
 if (RAExecAppWindow && WorkbenchBase) {
  RemoveAppWindow(RAExecAppWindow);
  RAExecAppWindow=NULL;
 }
 HandleAppMsg=HandleMainWindowAppMsg;
 if (RAExecWindowObj) {
  DisposeObject(RAExecWindowObj);
  RAExecWindowObj=NULL;
 }
 RAExecNameStrObj=NULL;
 RAExecTypeChooserObj=NULL;
 RAExecCmdStrObj=NULL;
 RAExecHotkeyStrObj=NULL;
 RAExecStackObj=NULL;
 RAExecPrioObj=NULL;
 RAExecDelayObj=NULL;
 RAExecCurdirStrObj=NULL;
 RAExecPathStrObj=NULL;
 RAExecOutputStrObj=NULL;
 RAExecPscreenStrObj=NULL;
 RAExecArgsObj=NULL;
 RAExecToFrontObj=NULL;
}

static void *ExecOKGadgetFunc(void)
{
 char *nameStr;
 char *cmdStr;
 char *curdirStr;
 char *pathStr;
 char *outputStr;
 char *hotkeyStr;
 char *pscreenStr;
 LONG stackVal;
 LONG prioVal;
 LONG delayVal;
 ULONG argsVal;
 ULONG toFrontVal;
 WORD typeVal;

 nameStr=DuplicateRAString(RAExecNameStrObj);
 cmdStr=DuplicateRAString(RAExecCmdStrObj);
 curdirStr=DuplicateRAString(RAExecCurdirStrObj);
 pathStr=DuplicateRAString(RAExecPathStrObj);
 outputStr=DuplicateRAString(RAExecOutputStrObj);
 hotkeyStr=DuplicateRAString(RAExecHotkeyStrObj);
 pscreenStr=DuplicateRAString(RAExecPscreenStrObj);
 if (nameStr==(char *)-1 || cmdStr==(char *)-1 || curdirStr==(char *)-1 ||
     pathStr==(char *)-1 || outputStr==(char *)-1 || hotkeyStr==(char *)-1 ||
     pscreenStr==(char *)-1) {
  if (nameStr && nameStr!=(char *)-1) free(nameStr);
  if (cmdStr && cmdStr!=(char *)-1) free(cmdStr);
  if (curdirStr && curdirStr!=(char *)-1) free(curdirStr);
  if (pathStr && pathStr!=(char *)-1) free(pathStr);
  if (outputStr && outputStr!=(char *)-1) free(outputStr);
  if (hotkeyStr && hotkeyStr!=(char *)-1) free(hotkeyStr);
  if (pscreenStr && pscreenStr!=(char *)-1) free(pscreenStr);
  FreeExecNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return (void *)-1;
 }
 stackVal=0;
 prioVal=0;
 delayVal=0;
 if (RAExecStackObj) GetAttr(INTEGER_Number,RAExecStackObj,(ULONG *)&stackVal);
 if (RAExecPrioObj) GetAttr(INTEGER_Number,RAExecPrioObj,(ULONG *)&prioVal);
 if (RAExecDelayObj) GetAttr(INTEGER_Number,RAExecDelayObj,(ULONG *)&delayVal);
 typeVal=0;
 if (RAExecTypeChooserObj) GetAttr(CHOOSER_Selected,RAExecTypeChooserObj,(ULONG *)&typeVal);
 argsVal=0;
 toFrontVal=0;
 if (RAExecArgsObj) GetAttr(GA_Selected,RAExecArgsObj,(ULONG *)&argsVal);
 if (RAExecToFrontObj) GetAttr(GA_Selected,RAExecToFrontObj,(ULONG *)&toFrontVal);

 if (CurrentNode->en_Node.ln_Name) free(CurrentNode->en_Node.ln_Name);
 CurrentNode->en_Node.ln_Name=nameStr ? nameStr : strdup("");
 if (CurrentNode->en_Command) free(CurrentNode->en_Command);
 CurrentNode->en_Command=cmdStr ? cmdStr : strdup("");
 if (CurrentNode->en_CurrentDir) free(CurrentNode->en_CurrentDir);
 CurrentNode->en_CurrentDir=curdirStr ? curdirStr : strdup("");
 if (CurrentNode->en_Path) free(CurrentNode->en_Path);
 CurrentNode->en_Path=pathStr ? pathStr : strdup("");
 if (CurrentNode->en_Output) free(CurrentNode->en_Output);
 CurrentNode->en_Output=outputStr ? outputStr : strdup("");
 if (CurrentNode->en_HotKey) free(CurrentNode->en_HotKey);
 CurrentNode->en_HotKey=hotkeyStr ? hotkeyStr : strdup("");
 if (CurrentNode->en_PubScreen) free(CurrentNode->en_PubScreen);
 CurrentNode->en_PubScreen=pscreenStr ? pscreenStr : strdup("");
 CurrentNode->en_ExecType=(UWORD)typeVal;
 CurrentNode->en_Stack=(ULONG)stackVal;
 CurrentNode->en_Priority=(WORD)prioVal;
 CurrentNode->en_Delay=(LONG)delayVal;
 CurrentNode->en_Flags=(CurrentNode->en_Flags & ~(EXPOF_ARGS|EXPOF_TOFRONT)) |
  (argsVal ? EXPOF_ARGS : 0) | (toFrontVal ? EXPOF_TOFRONT : 0);
 return (void *)CurrentNode;
}

BOOL HandleRAExecWindowEvent(Object *windowObj, ULONG result, UWORD code)
{
 ULONG v;

 (void)windowObj;
 switch (result) {
  case WMHI_CLOSEWINDOW:
   if (CurrentNode) { FreeExecNode((struct Node *)CurrentNode); CurrentNode=NULL; }
   SubWindowRAReturnData=(void *)-1;
   return TRUE;
  case WMHI_GADGETUP:
   switch (code) {
    case G_EXEC_OK:
     SubWindowRAReturnData=ExecOKGadgetFunc();
     return TRUE;
    case G_EXEC_CANCEL:
     if (CurrentNode) { FreeExecNode((struct Node *)CurrentNode); CurrentNode=NULL; }
     SubWindowRAReturnData=(void *)-1;
     return TRUE;
    case G_EXEC_TYPE:
     if (RAExecTypeChooserObj) {
      GetAttr(CHOOSER_Selected,RAExecTypeChooserObj,(ULONG *)&v);
      CurrentNode->en_ExecType=(UWORD)v;
     }
     break;
    case G_EXEC_CMD_BUT: DoCommandButton(); break;
    case G_EXEC_CURDIR_BUT: DoCurDirButton(); break;
    case G_EXEC_PATH_BUT: DoPathButton(); break;
    case G_EXEC_OUTPUT_BUT: DoOutputButton(); break;
    case G_EXEC_PSCREEN_BUT: DoPubScreenButton(); break;
    case G_EXEC_ARGS:
     if (RAExecArgsObj) {
      GetAttr(GA_Selected,RAExecArgsObj,(ULONG *)&v);
      CurrentNode->en_Flags=(CurrentNode->en_Flags & ~EXPOF_ARGS) | (v ? EXPOF_ARGS : 0);
     }
     break;
    case G_EXEC_TOFRONT:
     if (RAExecToFrontObj) {
      GetAttr(GA_Selected,RAExecToFrontObj,(ULONG *)&v);
      CurrentNode->en_Flags=(CurrentNode->en_Flags & ~EXPOF_TOFRONT) | (v ? EXPOF_TOFRONT : 0);
     }
     break;
   }
   break;
 }
 return FALSE;
}

BOOL OpenExecEditWindow(struct Node *node, struct Window *parent)
{
 Object *layout;
 struct Window *w;
 char *cmd;
 char *curdir;
 char *path;
 char *output;
 char *hotkey;
 char *pscreen;
 char *name;

 if (!ExecTypeChooserLabels) return FALSE;
 if (!(CurrentNode=(struct ExecNode *)CopyExecNode(node)))
  return FALSE;
 name=CurrentNode->en_Node.ln_Name ? CurrentNode->en_Node.ln_Name : "";
 cmd=CurrentNode->en_Command ? CurrentNode->en_Command : "";
 curdir=CurrentNode->en_CurrentDir ? CurrentNode->en_CurrentDir : "";
 path=CurrentNode->en_Path ? CurrentNode->en_Path : "";
 output=CurrentNode->en_Output ? CurrentNode->en_Output : "";
 hotkey=CurrentNode->en_HotKey ? CurrentNode->en_HotKey : "";
 pscreen=CurrentNode->en_PubScreen ? CurrentNode->en_PubScreen : "";

 layout=VGroupObject,
  LAYOUT_SpaceOuter,TRUE,LAYOUT_SpaceInner,TRUE,LAYOUT_BevelStyle,BVS_THIN,
  StartMember,RAExecNameStrObj=StringObject,GA_ID,G_EXEC_NAME,GA_RelVerify,TRUE,STRINGA_TextVal,name,STRINGA_MaxChars,SGBUFLEN,EndMember,MemberLabel((ULONG)AppStrings[MSG_WINDOW_NAME_GAD]),
  StartMember,RAExecTypeChooserObj=ChooserObject,GA_ID,G_EXEC_TYPE,GA_RelVerify,TRUE,CHOOSER_PopUp,TRUE,CHOOSER_Labels,ExecTypeChooserLabels,CHOOSER_Selected,CurrentNode->en_ExecType,EndMember,MemberLabel((ULONG)AppStrings[MSG_EXECWIN_EXECTYPE_GAD]),
  StartMember,HGroupObject,ButtonObject,GA_ID,G_EXEC_CMD_BUT,GA_RelVerify,TRUE,GA_Text,(ULONG)AppStrings[MSG_WINDOW_COMMAND_GAD],ButtonEnd,RAExecCmdStrObj=StringObject,GA_ID,G_EXEC_CMD_STR,GA_RelVerify,TRUE,STRINGA_TextVal,cmd,STRINGA_MaxChars,SGBUFLEN,EndGroup,EndMember,MemberLabel((ULONG)AppStrings[MSG_WINDOW_COMMAND_GAD]),
  StartMember,RAExecHotkeyStrObj=StringObject,GA_ID,G_EXEC_HOTKEY,GA_RelVerify,TRUE,STRINGA_TextVal,hotkey,STRINGA_MaxChars,SGBUFLEN,EndMember,MemberLabel((ULONG)AppStrings[MSG_WINDOW_HOTKEY_GAD]),
  StartMember,RAExecStackObj=IntegerObject,GA_ID,G_EXEC_STACK,GA_RelVerify,TRUE,INTEGER_Number,CurrentNode->en_Stack,INTEGER_MaxChars,10,EndMember,MemberLabel((ULONG)AppStrings[MSG_EXECWIN_STACK_GAD]),
  StartMember,RAExecPrioObj=IntegerObject,GA_ID,G_EXEC_PRIO,GA_RelVerify,TRUE,INTEGER_Number,CurrentNode->en_Priority,INTEGER_MaxChars,10,EndMember,MemberLabel((ULONG)AppStrings[MSG_EXECWIN_PRIORITY_GAD]),
  StartMember,RAExecDelayObj=IntegerObject,GA_ID,G_EXEC_DELAY,GA_RelVerify,TRUE,INTEGER_Number,CurrentNode->en_Delay,INTEGER_MaxChars,10,EndMember,MemberLabel((ULONG)AppStrings[MSG_EXECWIN_DELAY_GAD]),
  StartMember,HGroupObject,ButtonObject,GA_ID,G_EXEC_CURDIR_BUT,GA_RelVerify,TRUE,GA_Text,(ULONG)AppStrings[MSG_EXECWIN_CURRENTDIR_GAD],ButtonEnd,RAExecCurdirStrObj=StringObject,GA_ID,G_EXEC_CURDIR_STR,GA_RelVerify,TRUE,STRINGA_TextVal,curdir,STRINGA_MaxChars,SGBUFLEN,EndGroup,EndMember,MemberLabel((ULONG)AppStrings[MSG_EXECWIN_CURRENTDIR_GAD]),
  StartMember,HGroupObject,ButtonObject,GA_ID,G_EXEC_PATH_BUT,GA_RelVerify,TRUE,GA_Text,(ULONG)AppStrings[MSG_EXECWIN_PATH_GAD],ButtonEnd,RAExecPathStrObj=StringObject,GA_ID,G_EXEC_PATH_STR,GA_RelVerify,TRUE,STRINGA_TextVal,path,STRINGA_MaxChars,SGBUFLEN,EndGroup,EndMember,MemberLabel((ULONG)AppStrings[MSG_EXECWIN_PATH_GAD]),
  StartMember,HGroupObject,ButtonObject,GA_ID,G_EXEC_OUTPUT_BUT,GA_RelVerify,TRUE,GA_Text,(ULONG)AppStrings[MSG_EXECWIN_OUTPUT_GAD],ButtonEnd,RAExecOutputStrObj=StringObject,GA_ID,G_EXEC_OUTPUT_STR,GA_RelVerify,TRUE,STRINGA_TextVal,output,STRINGA_MaxChars,SGBUFLEN,EndGroup,EndMember,MemberLabel((ULONG)AppStrings[MSG_EXECWIN_OUTPUT_GAD]),
  StartMember,HGroupObject,ButtonObject,GA_ID,G_EXEC_PSCREEN_BUT,GA_RelVerify,TRUE,GA_Text,(ULONG)AppStrings[MSG_WINDOW_PUBSCREEN_GAD],ButtonEnd,RAExecPscreenStrObj=StringObject,GA_ID,G_EXEC_PSCREEN_STR,GA_RelVerify,TRUE,STRINGA_TextVal,pscreen,STRINGA_MaxChars,SGBUFLEN,EndGroup,EndMember,MemberLabel((ULONG)AppStrings[MSG_WINDOW_PUBSCREEN_GAD]),
  StartMember,RAExecArgsObj=CheckBoxObject,GA_ID,G_EXEC_ARGS,GA_RelVerify,TRUE,GA_Selected,(CurrentNode->en_Flags&EXPOF_ARGS)?TRUE:FALSE,GA_Text,(ULONG)AppStrings[MSG_EXECWIN_ARGUMENTS_GAD],EndMember,
  StartMember,RAExecToFrontObj=CheckBoxObject,GA_ID,G_EXEC_TOFRONT,GA_RelVerify,TRUE,GA_Selected,(CurrentNode->en_Flags&EXPOF_TOFRONT)?TRUE:FALSE,GA_Text,(ULONG)AppStrings[MSG_EXECWIN_TOFRONT_GAD],EndMember,
  StartHGroup,EvenSized,StartMember,ButtonObject,GA_ID,G_EXEC_OK,GA_RelVerify,TRUE,GA_Text,(ULONG)AppStrings[MSG_WINDOW_OK_GAD],ButtonEnd,StartMember,ButtonObject,GA_ID,G_EXEC_CANCEL,GA_RelVerify,TRUE,GA_Text,(ULONG)AppStrings[MSG_WINDOW_CANCEL_GAD],ButtonEnd,EndGroup,
 EndGroup;

 if (!layout) {
  FreeExecNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return FALSE;
 }

 RAExecWindowObj=WindowObject,
  WA_PubScreen,PublicScreen,
  WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_RMBTRAP|WFLG_ACTIVATE,
  WA_Title,AppStrings[MSG_EXECWIN_TITLE],
  WINDOW_RefWindow,parent,
  WINDOW_Position,WPOS_CENTERWINDOW,
  WINDOW_ParentGroup,layout,
 EndWindow;

 if (!RAExecWindowObj) {
  DisposeObject(layout);
  FreeExecNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return FALSE;
 }

 if (!(w=(struct Window *)RA_OpenWindow(RAExecWindowObj))) {
  DisposeObject(RAExecWindowObj);
  RAExecWindowObj=NULL;
  RAExecNameStrObj=NULL;
  RAExecTypeChooserObj=NULL;
  RAExecCmdStrObj=NULL;
  RAExecHotkeyStrObj=NULL;
  RAExecStackObj=NULL;
  RAExecPrioObj=NULL;
  RAExecDelayObj=NULL;
  RAExecCurdirStrObj=NULL;
  RAExecPathStrObj=NULL;
  RAExecOutputStrObj=NULL;
  RAExecPscreenStrObj=NULL;
  RAExecArgsObj=NULL;
  RAExecToFrontObj=NULL;
  FreeExecNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return FALSE;
 }

 CurrentWindow=w;
 SubWindowPort=w->UserPort;
 SubWindowRAObject=RAExecWindowObj;
 SubWindowRAHandler=HandleRAExecWindowEvent;
 SubWindowRACloseFunc=CloseRAExecWindow;

 FileReqParms.frp_Window=w;
 FileReqParms.frp_OKText=AppStrings[MSG_FILEREQ_OK_GAD];
 FileReqParms.frp_Flags1=FRF_DOPATTERNS;

 RAExecAppWindow=NULL;
 if (WorkbenchBase && WBScreen)
  RAExecAppWindow=AddAppWindowA(0,0,w,AppMsgPort,NULL);
 if (RAExecAppWindow)
  HandleAppMsg=HandleExecEditWindowAppMsg;

 return TRUE;
}

void HandleExecEditWindowAppMsg(struct AppMessage *msg)
{
 struct WBArg *wa;
 BPTR olddir;
 struct DiskObject *dobj;
 char *command;
 char *dirbuf;

 if (!RAExecNameStrObj || !RAExecCmdStrObj || !RAExecCurdirStrObj) return;
 wa=msg->am_ArgList;
 if (!wa) return;
 olddir=CurrentDir(wa->wa_Lock);
 dobj=GetDiskObjectNew(wa->wa_Name);
 if (!dobj) { CurrentDir(olddir); return; }
 command=NULL;
 if (dobj->do_Type==WBTOOL || dobj->do_Type==WBPROJECT) command=(char *)wa->wa_Name;
 FreeDiskObject(dobj);
 if (!command) { CurrentDir(olddir); return; }
 dirbuf=malloc(4096);
 if (!dirbuf) { CurrentDir(olddir); return; }
 if (!NameFromLock(wa->wa_Lock,dirbuf,4096)) { free(dirbuf); CurrentDir(olddir); return; }
 CurrentNode->en_ExecType=TMET_WB;
 SetAttrs(RAExecTypeChooserObj,CHOOSER_Selected,TMET_WB,TAG_END);
 SetAttrs(RAExecNameStrObj,STRINGA_TextVal,wa->wa_Name,TAG_END);
 SetAttrs(RAExecCmdStrObj,STRINGA_TextVal,command,TAG_END);
 SetAttrs(RAExecCurdirStrObj,STRINGA_TextVal,dirbuf,TAG_END);
 free(dirbuf);
 CurrentDir(olddir);
}

void UpdateExecEditWindow(void *data)
{
 struct Window *w;
 char *new;

 if (data != LREQRET_CANCEL) {
  new=(data == LREQRET_NOSELECT) ? NULL : ((struct Node *)data)->ln_Name;
  if (!new) new="";
  if (CurrentGadgetNum==G_EXEC_CMD_STR && RAExecCmdStrObj)
   SetAttrs(RAExecCmdStrObj,STRINGA_TextVal,new,TAG_END);
  else if (CurrentGadgetNum==G_EXEC_PSCREEN_STR && RAExecPscreenStrObj)
   SetAttrs(RAExecPscreenStrObj,STRINGA_TextVal,new,TAG_END);
 }
 w=NULL;
 if (RAExecWindowObj) GetAttr(WINDOW_Window,RAExecWindowObj,(ULONG *)&w);
 if (w) {
  SubWindowPort=w->UserPort;
  SubWindowRAObject=RAExecWindowObj;
  SubWindowRAHandler=HandleRAExecWindowEvent;
  SubWindowRACloseFunc=CloseRAExecWindow;
  CurrentWindow=w;
 }
 UpdateWindow=UpdateMainWindow;
}

void *HandleExecEditWindowIDCMP(struct IntuiMessage *msg)
{
 (void)msg;
 return NULL;
}

struct Node *ReadExecNode(UBYTE *buf, ULONG size)
{
 struct ExecNode *en;
 struct ExecPrefsObject *epo;
 ULONG sbits;
 UBYTE *ptr;

 (void)size;
 epo=(struct ExecPrefsObject *)buf;
 sbits=epo->epo_StringBits;
 ptr=(UBYTE *)&epo[1];

 if (en=AllocMem(sizeof(struct ExecNode),MEMF_PUBLIC|MEMF_CLEAR)) {

  if ((!(sbits & EXPO_NAME) || (en->en_Node.ln_Name=GetConfigStr(&ptr))) &&
      (!(sbits & EXPO_COMMAND) || (en->en_Command=GetConfigStr(&ptr))) &&
      (!(sbits & EXPO_CURDIR) || (en->en_CurrentDir=GetConfigStr(&ptr))) &&
      (!(sbits & EXPO_HOTKEY) || (en->en_HotKey=GetConfigStr(&ptr))) &&
      (!(sbits & EXPO_OUTPUT) || (en->en_Output=GetConfigStr(&ptr))) &&
      (!(sbits & EXPO_PATH) || (en->en_Path=GetConfigStr(&ptr))) &&
      (!(sbits & EXPO_PSCREEN) || (en->en_PubScreen=GetConfigStr(&ptr)))) {
   en->en_Flags=epo->epo_Flags;
   en->en_ExecType=epo->epo_ExecType;
   en->en_Priority=epo->epo_Priority;
   en->en_Delay=epo->epo_Delay;
   en->en_Stack=epo->epo_Stack;
   return (struct Node *)en;
  }
  FreeExecNode((struct Node *)en);
 }
 return NULL;
}

BOOL WriteExecNode(struct IFFHandle *iff, UBYTE *buf, struct Node *node)
{
 struct ExecNode *en=(struct ExecNode *) node;
 struct ExecPrefsObject *epo=(struct ExecPrefsObject *) buf;
 ULONG sbits=0;
 UBYTE *ptr=(UBYTE *) &epo[1];

 if (PutConfigStr(en->en_Node.ln_Name,&ptr)) sbits|=EXPO_NAME;
 if (PutConfigStr(en->en_Command,&ptr)) sbits|=EXPO_COMMAND;
 if (PutConfigStr(en->en_CurrentDir,&ptr)) sbits|=EXPO_CURDIR;
 if (PutConfigStr(en->en_HotKey,&ptr)) sbits|=EXPO_HOTKEY;
 if (PutConfigStr(en->en_Output,&ptr)) sbits|=EXPO_OUTPUT;
 if (PutConfigStr(en->en_Path,&ptr)) sbits|=EXPO_PATH;
 if (PutConfigStr(en->en_PubScreen,&ptr)) sbits|=EXPO_PSCREEN;
 epo->epo_StringBits=sbits;
 epo->epo_Flags=en->en_Flags;
 epo->epo_ExecType=en->en_ExecType;
 epo->epo_Priority=en->en_Priority;
 epo->epo_Delay=en->en_Delay;
 epo->epo_Stack=en->en_Stack;
 sbits=ptr-buf;

 if (PushChunk(iff,0,ID_TMEX,sbits)) return FALSE;
 if (WriteChunkBytes(iff,buf,sbits)!=sbits) return FALSE;
 if (PopChunk(iff)) return FALSE;
 return TRUE;
}
