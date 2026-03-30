/*
 * soundwindow.c  RAprefs
 *
 * Sound edit window: ReAction GUI (WindowObject + 3 string gadgets + OK/Cancel).
 * Integrates via SubWindowRAObject and main loop RA_HandleInput path.
 *
 * (c) 1990-1993 Stefan Becker
 */

#include "RAprefsConf.h"

struct SoundNode {
                  struct Node  sn_Node;
                  char        *sn_Command;
                  char        *sn_Port;
                 };

#define G_SOUND_NAME   1
#define G_SOUND_COMM   2
#define G_SOUND_AREXX  3
#define G_SOUND_OK     4
#define G_SOUND_CANCEL 5

static struct SoundNode *CurrentNode;
static Object *RASoundWindowObj;
static Object *RASoundNameStrObj;
static Object *RASoundCommStrObj;
static Object *RASoundArexxStrObj;

struct IClass *STRING_GetClass(void);

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

void InitSoundEditWindow(UWORD left, UWORD fheight)
{
 (void)left;
 (void)fheight;
}

void FreeSoundNode(struct Node *node)
{
 struct SoundNode *sn=(struct SoundNode *) node;
 char *s;

 if (s=sn->sn_Node.ln_Name) free(s);
 if (s=sn->sn_Command) free(s);
 if (s=sn->sn_Port) free(s);
 FreeMem(sn,sizeof(struct SoundNode));
}

struct Node *CopySoundNode(struct Node *node)
{
 struct SoundNode *sn,*orignode=(struct SoundNode *) node;

 if (sn=AllocMem(sizeof(struct SoundNode),MEMF_CLEAR)) {
  if (orignode) {
   if ((!orignode->sn_Node.ln_Name || (sn->sn_Node.ln_Name=strdup(orignode->sn_Node.ln_Name))) &&
       (!orignode->sn_Command || (sn->sn_Command=strdup(orignode->sn_Command))) &&
       (!orignode->sn_Port || (sn->sn_Port=strdup(orignode->sn_Port))))
    return (struct Node *)sn;
  } else {
   if (sn->sn_Node.ln_Name=strdup(AppStrings[MSG_SOUNDWIN_NEWNAME]))
    return (struct Node *)sn;
  }
  FreeSoundNode((struct Node *)sn);
 }
 return NULL;
}

static void CloseRASoundWindow(void)
{
 if (RASoundWindowObj) {
  DisposeObject(RASoundWindowObj);
  RASoundWindowObj=NULL;
 }
 RASoundNameStrObj=NULL;
 RASoundCommStrObj=NULL;
 RASoundArexxStrObj=NULL;
}

static void *SoundOKGadgetFunc(void)
{
 char *nameStr;
 char *commStr;
 char *portStr;
 char *s;

 nameStr=DuplicateRAString(RASoundNameStrObj);
 commStr=DuplicateRAString(RASoundCommStrObj);
 portStr=DuplicateRAString(RASoundArexxStrObj);
 if (nameStr==(char *)-1 || commStr==(char *)-1 || portStr==(char *)-1) {
  if (nameStr && nameStr!=(char *)-1) free(nameStr);
  if (commStr && commStr!=(char *)-1) free(commStr);
  if (portStr && portStr!=(char *)-1) free(portStr);
  FreeSoundNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return (void *)-1;
 }

 s=CurrentNode->sn_Node.ln_Name;
 if (s) free(s);
 CurrentNode->sn_Node.ln_Name=nameStr ? nameStr : strdup("");
 s=CurrentNode->sn_Command;
 if (s) free(s);
 CurrentNode->sn_Command=commStr ? commStr : strdup("");
 s=CurrentNode->sn_Port;
 if (s) free(s);
 CurrentNode->sn_Port=portStr ? portStr : strdup("");

 return (void *)CurrentNode;
}

BOOL HandleRASoundWindowEvent(Object *windowObj, ULONG result, UWORD code)
{
 (void)windowObj;

 switch (result) {
  case WMHI_CLOSEWINDOW:
   if (CurrentNode) { FreeSoundNode((struct Node *)CurrentNode); CurrentNode=NULL; }
   SubWindowRAReturnData=(void *)-1;
   return TRUE;
  case WMHI_GADGETUP:
   switch (code) {
    case G_SOUND_OK:
     SubWindowRAReturnData=SoundOKGadgetFunc();
     return TRUE;
    case G_SOUND_CANCEL:
     if (CurrentNode) { FreeSoundNode((struct Node *)CurrentNode); CurrentNode=NULL; }
     SubWindowRAReturnData=(void *)-1;
     return TRUE;
   }
   break;
 }
 return FALSE;
}

BOOL OpenSoundEditWindow(struct Node *node, struct Window *parent)
{
 Object *layout;
 struct Window *w;

 if (!(CurrentNode=(struct SoundNode *)CopySoundNode(node)))
  return FALSE;

 layout=VGroupObject,
  LAYOUT_SpaceOuter,TRUE,
  LAYOUT_SpaceInner,TRUE,
  LAYOUT_BevelStyle,BVS_THIN,
  StartMember,
   RASoundNameStrObj=StringObject,
    GA_ID,G_SOUND_NAME,
    GA_RelVerify,TRUE,
    STRINGA_TextVal,CurrentNode->sn_Node.ln_Name ? CurrentNode->sn_Node.ln_Name : "",
    STRINGA_MaxChars,SGBUFLEN,
   EndMember,
   MemberLabel(AppStrings[MSG_WINDOW_NAME_GAD]),
  StartMember,
   RASoundCommStrObj=StringObject,
    GA_ID,G_SOUND_COMM,
    GA_RelVerify,TRUE,
    STRINGA_TextVal,CurrentNode->sn_Command ? CurrentNode->sn_Command : "",
    STRINGA_MaxChars,SGBUFLEN,
   EndMember,
   MemberLabel(AppStrings[MSG_WINDOW_COMMAND_GAD]),
  StartMember,
   RASoundArexxStrObj=StringObject,
    GA_ID,G_SOUND_AREXX,
    GA_RelVerify,TRUE,
    STRINGA_TextVal,CurrentNode->sn_Port ? CurrentNode->sn_Port : "",
    STRINGA_MaxChars,SGBUFLEN,
   EndMember,
   MemberLabel(AppStrings[MSG_SOUNDWIN_AREXX_GAD]),
  StartHGroup,EvenSized,
   StartMember,ButtonObject,GA_ID,G_SOUND_OK,GA_RelVerify,TRUE,GA_Text,AppStrings[MSG_WINDOW_OK_GAD],ButtonEnd,
   StartMember,ButtonObject,GA_ID,G_SOUND_CANCEL,GA_RelVerify,TRUE,GA_Text,AppStrings[MSG_WINDOW_CANCEL_GAD],ButtonEnd,
  EndGroup,
 EndGroup;

 if (!layout) {
  FreeSoundNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return FALSE;
 }

 RASoundWindowObj=WindowObject,
  WA_PubScreen,PublicScreen,
  WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_RMBTRAP|WFLG_ACTIVATE,
  WA_Title,AppStrings[MSG_SOUNDWIN_TITLE],
  WINDOW_RefWindow,parent,
  WINDOW_Position,WPOS_CENTERWINDOW,
  WINDOW_ParentGroup,layout,
 EndWindow;

 if (!RASoundWindowObj) {
  DisposeObject(layout);
  FreeSoundNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return FALSE;
 }

 if (!(w=(struct Window *)RA_OpenWindow(RASoundWindowObj))) {
  DisposeObject(RASoundWindowObj);
  RASoundWindowObj=NULL;
  RASoundNameStrObj=NULL;
  RASoundCommStrObj=NULL;
  RASoundArexxStrObj=NULL;
  FreeSoundNode((struct Node *)CurrentNode);
  CurrentNode=NULL;
  return FALSE;
 }

 CurrentWindow=w;
 SubWindowPort=w->UserPort;
 SubWindowRAObject=RASoundWindowObj;
 SubWindowRAHandler=HandleRASoundWindowEvent;
 SubWindowRACloseFunc=CloseRASoundWindow;

 return TRUE;
}

void *HandleSoundEditWindowIDCMP(struct IntuiMessage *msg)
{
 (void)msg;
 return NULL;
}

struct Node *ReadSoundNode(UBYTE *buf, ULONG size)
{
 struct SoundNode *sn;
 struct SoundPrefsObject *spo;
 ULONG sbits;
 UBYTE *ptr;

 (void)size;
 spo=(struct SoundPrefsObject *)buf;
 sbits=spo->spo_StringBits;
 ptr=(UBYTE *)&spo[1];

 if (sn=AllocMem(sizeof(struct SoundNode),MEMF_PUBLIC|MEMF_CLEAR)) {
  if ((!(sbits & SOPO_NAME) || (sn->sn_Node.ln_Name=GetConfigStr(&ptr))) &&
      (!(sbits & SOPO_COMMAND) || (sn->sn_Command=GetConfigStr(&ptr))) &&
      (!(sbits & SOPO_PORT) || (sn->sn_Port=GetConfigStr(&ptr))))
   return (struct Node *)sn;
  FreeSoundNode((struct Node *)sn);
 }
 return NULL;
}

BOOL WriteSoundNode(struct IFFHandle *iff, UBYTE *buf, struct Node *node)
{
 struct SoundNode *sn=(struct SoundNode *) node;
 struct SoundPrefsObject *spo=(struct SoundPrefsObject *) buf;
 ULONG sbits=0;
 UBYTE *ptr=(UBYTE *) &spo[1];

 if (PutConfigStr(sn->sn_Node.ln_Name,&ptr)) sbits|=SOPO_NAME;
 if (PutConfigStr(sn->sn_Command,&ptr)) sbits|=SOPO_COMMAND;
 if (PutConfigStr(sn->sn_Port,&ptr)) sbits|=SOPO_PORT;
 spo->spo_StringBits=sbits;
 sbits=ptr-buf;

 if (PushChunk(iff,0,ID_TMSO,sbits)) return FALSE;
 if (WriteChunkBytes(iff,buf,sbits)!=sbits) return FALSE;
 if (PopChunk(iff)) return FALSE;
 return TRUE;
}
