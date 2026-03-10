/*
 * data.c  V2.1
 *
 * configuration program global data
 *
 * (c) 1990-1993 Stefan Becker
 */

#include "ToolManagerConf.h"

struct Library *WorkbenchBase=NULL;
char *ProgramName;
UpdateWindowFuncPtr UpdateWindow=NULL;
HandleAppMsgFuncPtr HandleAppMsg=NULL;
struct Window *CurrentWindow;
FreeNodeFuncPtr FreeNodeFunctions[TMOBJTYPES]={FreeExecNode,
                                               FreeImageNode,
                                               FreeSoundNode,
                                               FreeMenuNode,
                                               FreeIconNode,
                                               FreeDockNode,
                                               FreeAccessNode};
struct List ObjectLists[TMOBJTYPES];
struct List PubScreenList;
struct RastPort TmpRastPort;
struct NewGadget NewGadget;
struct FileReqParms FileReqParms;
struct TextAttr ScreenTextAttr;
struct Screen *PublicScreen;
void *ScreenVI;
struct TextFont *ScreenFont;
struct MsgPort *IDCMPPort;
struct MsgPort *AppMsgPort=NULL;
/* When a GadTools sub-window (edit/select/listreq etc.) is open, main loop waits on its port and dispatches here */
struct MsgPort *SubWindowPort=NULL;
void *(*SubWindowHandler)(struct IntuiMessage *)=NULL;
/* When opening move window or list req on top of an edit window, save edit window port so we can restore */
struct MsgPort *SavedSubWindowPort=NULL;
void *(*SavedSubWindowHandler)(struct IntuiMessage *)=NULL;
/* ReAction sub-window: object, event handler, close callback, and return value for UpdateWindow */
Object *SubWindowRAObject=NULL;
BOOL (*SubWindowRAHandler)(Object *, ULONG result, UWORD code)=NULL;
void (*SubWindowRACloseFunc)(void)=NULL;
void *SubWindowRAReturnData=NULL;
/* Saved when opening move window (RA icon/dock etc.) so we restore on CloseMoveWindow */
Object *SavedSubWindowRAObject=NULL;
BOOL (*SavedSubWindowRAHandler)(Object *, ULONG result, UWORD code)=NULL;
void (*SavedSubWindowRACloseFunc)(void)=NULL;
BOOL OSV39=FALSE;
BOOL WBScreen;
BOOL CreateIcons=TRUE;
UWORD WindowTop;
UWORD ListViewColumns=20;
UWORD ListViewRows=8;
UWORD WBXOffset=0;
UWORD WBYOffset=0;

/* Global gadget tags */
const struct TagItem DisabledTags[]={GA_Disabled, TRUE,
                                     TAG_DONE};

/* Preferences file names */
const char PrefsFileName[]=TMPREFSUSE;
const char SavePrefsFileName[]=TMPREFSSAVE;
