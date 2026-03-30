/*
 * imagewindow.c  RAprefs
 *
 * Image edit window: ReAction GUI (WindowObject + layout + string gadgets +
 * File button + OK/Cancel). File button opens ASL file requester.
 * Integrates via SubWindowRAObject and main loop RA_HandleInput path.
 *
 * (c) 1990-1993 Stefan Becker
 */

#include "RAprefsConf.h"

struct ImageNode {
                  struct Node  in_Node;
                  char        *in_File;
                 };

#define G_IMAGE_NAME  1
#define G_IMAGE_FILE_BUT 2
#define G_IMAGE_FILE_STR 3
#define G_IMAGE_OK    4
#define G_IMAGE_CANCEL 5

static struct ImageNode *CurrentNode;
static char *DirName=NULL;
static struct Requester DummyReq;
static Object *RAImageWindowObj;
static Object *RAImageNameStrObj;
static Object *RAImageFileStrObj;
static void *RAImageAppWindow;

/* string.gadget: declare if missing from SDK */
struct IClass *STRING_GetClass(void);

/* Duplicate string from ReAction string gadget; returns strdup'd string, (char*)-1 on alloc fail, NULL if empty */
static char *DuplicateRAString(Object *strObj)
{
 STRPTR p;
 char *s;

 if (!strObj) return (char *)-1;
 p=NULL;
 GetAttr(STRINGA_TextVal,strObj,(ULONG *)&p);
 if (!p) return NULL;
 if (!*p) return NULL;
 s=strdup(p);
 return s ? s : (char *)-1;
}

void InitImageEditWindow(UWORD left, UWORD fheight)
{
 (void)left;
 (void)fheight;
 InitRequester(&DummyReq);
}

void FreeImageNode(struct Node *node)
{
 struct ImageNode *in=(struct ImageNode *) node;
 char *s;

 if (s=in->in_Node.ln_Name) free(s);
 if (s=in->in_File) free(s);
 FreeMem(in,sizeof(struct ImageNode));
}

struct Node *CopyImageNode(struct Node *node)
{
 struct ImageNode *in,*orignode=(struct ImageNode *) node;

 if (in=AllocMem(sizeof(struct ImageNode),MEMF_PUBLIC|MEMF_CLEAR)) {
  if (orignode) {
   if ((!orignode->in_Node.ln_Name || (in->in_Node.ln_Name=strdup(orignode->in_Node.ln_Name))) &&
       (!orignode->in_File || (in->in_File=strdup(orignode->in_File))))
    return (struct Node *)in;
  } else {
   if ((in->in_Node.ln_Name=strdup(AppStrings[MSG_IMAGEWIN_NEWNAME])) &&
       (!DirName || (in->in_File=strdup(DirName))))
    return (struct Node *)in;
  }
  FreeImageNode((struct Node *)in);
 }
 return NULL;
}

struct Node *CreateImageNode(char *name, struct WBArg *wa)
{
 struct ImageNode *in;

 if (in=AllocMem(sizeof(struct ImageNode),MEMF_PUBLIC|MEMF_CLEAR)) {
  char *filebuf;

  if ((in->in_Node.ln_Name=strdup(name)) && (filebuf=malloc(4096))) {
   if (NameFromLock(wa->wa_Lock,filebuf,4096) &&
       AddPart(filebuf,wa->wa_Name,4096) &&
       (in->in_File=strdup(filebuf))) {
    free(filebuf);
    return (struct Node *)in;
   }
   free(filebuf);
  }
  FreeImageNode((struct Node *)in);
 }
 return NULL;
}

static void CloseRAImageWindow(void)
{
 if (RAImageAppWindow && WorkbenchBase) {
  RemoveAppWindow(RAImageAppWindow);
  RAImageAppWindow=NULL;
 }
 HandleAppMsg=HandleMainWindowAppMsg;
 if (RAImageWindowObj) {
  DisposeObject(RAImageWindowObj);
  RAImageWindowObj=NULL;
 }
 RAImageNameStrObj=NULL;
 RAImageFileStrObj=NULL;
 /* Node freed in handler on cancel/close; on OK we return it to UpdateMainWindow */
}

static void DoFileRequester(void)
{
 char *file;
 STRPTR oldFile;

 oldFile=NULL;
 if (RAImageFileStrObj)
  GetAttr(STRINGA_TextVal,RAImageFileStrObj,(ULONG *)&oldFile);
 FileReqParms.frp_OldFile=oldFile ? oldFile : (STRPTR)"";

 if (file=OpenFileRequester(&DummyReq)) {
  char *path;

  if (RAImageFileStrObj)
   SetAttrs(RAImageFileStrObj,STRINGA_TextVal,file,TAG_END);
  path=FilePart(file);
  if (path!=file) {
   if (DirName) free(DirName);
   DirName=strdup(file);
   if (DirName) DirName[path-file]='\0';
  }
 }
}

static void *ImageOKGadgetFunc(void)
{
 char *nameStr;
 char *fileStr;
 char *s;
 ULONG len;

 /* Match original: only (char*)-1 is alloc error; NULL = empty field is allowed */
 nameStr=DuplicateRAString(RAImageNameStrObj);
 fileStr=DuplicateRAString(RAImageFileStrObj);
 if (nameStr==(char *)-1 || fileStr==(char *)-1) {
  if (nameStr && nameStr!=(char *)-1) free(nameStr);
  if (fileStr && fileStr!=(char *)-1) free(fileStr);
  FreeImageNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return (void *)-1;
 }

 s=CurrentNode->in_Node.ln_Name;
 if (s) free(s);
 CurrentNode->in_Node.ln_Name=nameStr;
 s=CurrentNode->in_File;
 if (s) free(s);
 CurrentNode->in_File=fileStr;

 /* Strip trailing ".info" from file path (match original) */
 if (fileStr && (len=strlen(fileStr))>=5 && !stricmp(fileStr+len-5,".info"))
  fileStr[len-5]='\0';

 return (void *)CurrentNode;
}

BOOL HandleRAImageWindowEvent(Object *windowObj, ULONG result, UWORD code)
{
 (void)windowObj;

 switch (result) {
  case WMHI_CLOSEWINDOW:
   /* Match original: free copied node on close */
   if (CurrentNode) { FreeImageNode((struct Node *)CurrentNode); CurrentNode=NULL; }
   SubWindowRAReturnData=(void *)-1;
   return TRUE;

  case WMHI_GADGETUP:
   switch (code) {
    case G_IMAGE_OK:
     SubWindowRAReturnData=ImageOKGadgetFunc();
     return TRUE;
    case G_IMAGE_CANCEL:
     if (CurrentNode) { FreeImageNode((struct Node *)CurrentNode); CurrentNode=NULL; }
     SubWindowRAReturnData=(void *)-1;
     return TRUE;
    case G_IMAGE_FILE_BUT:
     DoFileRequester();
     break;
   }
   break;
 }
 return FALSE;
}

BOOL OpenImageEditWindow(struct Node *node, struct Window *parent)
{
 Object *layout;
 struct Window *w;

 if (!(CurrentNode=(struct ImageNode *)CopyImageNode(node)))
  return FALSE;

 layout=VGroupObject,
  LAYOUT_SpaceOuter,TRUE,
  LAYOUT_SpaceInner,TRUE,
  LAYOUT_BevelStyle,BVS_THIN,
  StartMember,
   RAImageNameStrObj=StringObject,
    GA_ID,G_IMAGE_NAME,
    GA_RelVerify,TRUE,
    STRINGA_TextVal,CurrentNode->in_Node.ln_Name,
    STRINGA_MaxChars,SGBUFLEN,
   EndMember,
   MemberLabel((ULONG)AppStrings[MSG_WINDOW_NAME_GAD]),
  StartMember,
   HGroupObject,
    ButtonObject,GA_ID,G_IMAGE_FILE_BUT,GA_RelVerify,TRUE,GA_Text,(ULONG)AppStrings[MSG_IMAGEWIN_FILE_GAD],ButtonEnd,
    RAImageFileStrObj=StringObject,
     GA_ID,G_IMAGE_FILE_STR,
     GA_RelVerify,TRUE,
     STRINGA_TextVal,CurrentNode->in_File ? CurrentNode->in_File : "",
     STRINGA_MaxChars,SGBUFLEN,
   EndGroup,
   EndMember,
   MemberLabel((ULONG)AppStrings[MSG_IMAGEWIN_FILE_GAD]),
  StartHGroup,EvenSized,
   StartMember,ButtonObject,GA_ID,G_IMAGE_OK,GA_RelVerify,TRUE,GA_Text,(ULONG)AppStrings[MSG_WINDOW_OK_GAD],ButtonEnd,
   StartMember,ButtonObject,GA_ID,G_IMAGE_CANCEL,GA_RelVerify,TRUE,GA_Text,(ULONG)AppStrings[MSG_WINDOW_CANCEL_GAD],ButtonEnd,
  EndGroup,
 EndGroup;

 if (!layout) {
  FreeImageNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return FALSE;
 }

 RAImageWindowObj=WindowObject,
  WA_PubScreen,PublicScreen,
  WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_RMBTRAP|WFLG_ACTIVATE,
  WA_Title,AppStrings[MSG_IMAGEWIN_TITLE],
  WINDOW_RefWindow,parent,
  WINDOW_Position,WPOS_CENTERWINDOW,
  WINDOW_ParentGroup,layout,
 EndWindow;

 if (!RAImageWindowObj) {
  DisposeObject(layout);
  FreeImageNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return FALSE;
 }

 if (!(w=(struct Window *)RA_OpenWindow(RAImageWindowObj))) {
  DisposeObject(RAImageWindowObj);
  RAImageWindowObj=NULL;
  RAImageNameStrObj=NULL;
  RAImageFileStrObj=NULL;
  FreeImageNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return FALSE;
 }

 CurrentWindow=w;
 SubWindowPort=w->UserPort;
 SubWindowRAObject=RAImageWindowObj;
 SubWindowRAHandler=HandleRAImageWindowEvent;
 SubWindowRACloseFunc=CloseRAImageWindow;

 FileReqParms.frp_Window=w;
 FileReqParms.frp_Title=AppStrings[MSG_FILEREQ_TITLE_FILE];
 FileReqParms.frp_OKText=AppStrings[MSG_FILEREQ_OK_GAD];
 FileReqParms.frp_Flags1=FRF_DOPATTERNS;
 FileReqParms.frp_Flags2=0;

 RAImageAppWindow=NULL;
 if (WorkbenchBase && WBScreen)
  RAImageAppWindow=AddAppWindowA(0,0,w,AppMsgPort,NULL);
 if (RAImageAppWindow)
  HandleAppMsg=HandleImageEditWindowAppMsg;

 return TRUE;
}

void HandleImageEditWindowAppMsg(struct AppMessage *msg)
{
 struct WBArg *wa;
 char *filebuf;

 if (!RAImageNameStrObj || !RAImageFileStrObj) return;
 wa=msg->am_ArgList;
 if (!wa) return;
 filebuf=malloc(4096);
 if (!filebuf) return;
 if (NameFromLock(wa->wa_Lock,filebuf,4096) && AddPart(filebuf,wa->wa_Name,4096)) {
  SetAttrs(RAImageNameStrObj,STRINGA_TextVal,wa->wa_Name,TAG_END);
  SetAttrs(RAImageFileStrObj,STRINGA_TextVal,filebuf,TAG_END);
 }
 free(filebuf);
}

void *HandleImageEditWindowIDCMP(struct IntuiMessage *msg)
{
 (void)msg;
 return NULL;
}

struct Node *ReadImageNode(UBYTE *buf, ULONG size)
{
 struct ImageNode *in;
 struct ImagePrefsObject *ipo;
 ULONG sbits;
 UBYTE *ptr;

 (void)size;
 ipo=(struct ImagePrefsObject *)buf;
 sbits=ipo->ipo_StringBits;
 ptr=(UBYTE *)&ipo[1];

 if (in=AllocMem(sizeof(struct ImageNode),MEMF_PUBLIC|MEMF_CLEAR)) {
  if ((!(sbits & IMPO_NAME) || (in->in_Node.ln_Name=GetConfigStr(&ptr))) &&
      (!(sbits & IMPO_FILE) || (in->in_File=GetConfigStr(&ptr))))
   return (struct Node *)in;
  FreeImageNode((struct Node *)in);
 }
 return NULL;
}

BOOL WriteImageNode(struct IFFHandle *iff, UBYTE *buf, struct Node *node)
{
 struct ImageNode *in=(struct ImageNode *) node;
 struct ImagePrefsObject *ipo=(struct ImagePrefsObject *) buf;
 ULONG sbits=0;
 UBYTE *ptr=(UBYTE *) &ipo[1];

 if (PutConfigStr(in->in_Node.ln_Name,&ptr)) sbits|=IMPO_NAME;
 if (PutConfigStr(in->in_File,&ptr)) sbits|=IMPO_FILE;
 ipo->ipo_StringBits=sbits;
 sbits=ptr-buf;

 if (PushChunk(iff,0,ID_TMIM,sbits)) return FALSE;
 if (WriteChunkBytes(iff,buf,sbits)!=sbits) return FALSE;
 if (PopChunk(iff)) return FALSE;
 return TRUE;
}
