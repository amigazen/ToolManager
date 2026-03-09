/*
 * ToolManagerConf.h  V2.1
 *
 * configuration program main include file
 *
 * (c) 1990-1993 Stefan Becker
 */

/* System include files */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <envoy/envoy.h>
#include <graphics/gfxbase.h>
#include <intuition/gadgetclass.h>
#define ASL_V38_NAMES_ONLY
#include <libraries/asl.h>
#include <libraries/iffparse.h>
#include <libraries/locale.h>
#include <libraries/toolmanager.h>
#include <prefs/prefhdr.h>

#include <intuition/icclass.h>
#include <libraries/gadtools.h>
#include <graphics/text.h>
#include <graphics/gfxmacros.h>
#include <utility/tagitem.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include <gadgets/chooser.h>
#include <images/label.h>
#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>
#include <classes/window.h>

/* System function prototypes */
#include <proto/alib.h>
#include <proto/asl.h>
#include <proto/diskfont.h>
#include <proto/dos.h>
#include <proto/envoy.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <proto/iffparse.h>
#include <proto/intuition.h>
#include <proto/locale.h>
#include <proto/wb.h>
#include <proto/utility.h>

#include <proto/button.h>
#include <proto/chooser.h>
#include <proto/label.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/window.h>
#include <clib/reaction_lib_protos.h>

/* ANSI C include files */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Compiler specific include files */
#include <clib/compiler-specific.h>
#include <lists.h>     /* GetHead() et al. */

#ifndef __COMMODORE_DATE__
#define __COMMODORE_DATE__ __AMIGADATE__
#endif

/* Project specific include files (paths relative to Source/ when -I.. from RAprefs) */
#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS
#include "locale/toolmanager.h"
#include "ToolManagerPrefs.h"

/* Structures */
struct GadgetData {
                   char           *name;
                   ULONG           type;
                   ULONG           flags;
                   struct TagItem *tags;
                   UWORD           left;
                   UWORD           top;
                   UWORD           width;
                   UWORD           height;
                   struct Gadget  *gadget;
                  };

struct FileReqParms {
                     struct Window *frp_Window;
                     char          *frp_Title;
                     char          *frp_OKText;
                     ULONG          frp_Flags1;
                     ULONG          frp_Flags2;
                     char          *frp_OldFile;
                    };

struct TMNode {
                 struct Node  tn_Node;
                 char        *tn_Image;
                 char        *tn_Sound;
                };

/* Internal function prototypes */
/* *window.c typedefs */
typedef void         (*UpdateWindowFuncPtr)(void *);
typedef BOOL         (*OpenWindowFuncPtr)(void *, struct Window *);
typedef void         (*HandleAppMsgFuncPtr)(struct AppMessage *);
typedef void        *(*HandleIDCMPFuncPtr)(struct IntuiMessage *);
typedef struct Node *(*CopyNodeFuncPtr)(struct Node *);
typedef void         (*FreeNodeFuncPtr)(struct Node *);
typedef struct Node *(*ReadNodeFuncPtr)(UBYTE *);
typedef BOOL         (*WriteNodeFuncPtr)(struct IFFHandle *, UBYTE *,
                                         struct Node *);

/* accesswindow.c */
void         InitAccessEditWindow(UWORD, UWORD);
BOOL         OpenAccessEditWindow(void *, struct Window *);
void         UpdateAccessEditWindow(void *);
void        *HandleAccessEditWindowIDCMP(struct IntuiMessage *);
struct Node *CopyAccessNode(struct Node *);
void         FreeAccessNode(struct Node *);
struct Node *ReadAccessNode(UBYTE *);
BOOL         WriteAccessNode(struct IFFHandle *, UBYTE *, struct Node *);

/* aslreqs.c */
char            *OpenFileRequester(struct Requester *);
struct TextAttr *OpenFontRequester(struct Window *, struct Requester *,
                                   struct TextAttr *);

/* button.c */
BOOL CalcReqButtonImage(void);
void FreeReqButtonImage(void);
void InitReqButtonGadget(struct Gadget *);

/* config.c */
BOOL  ReadConfigFile(char *);
BOOL  WriteConfigFile(char *);
char *GetConfigStr(UBYTE **);
BOOL  PutConfigStr(char *, UBYTE **);

/* dockwindow.c */
void         InitDockEditWindow(UWORD, UWORD);
BOOL         OpenDockEditWindow(void *, struct Window *);
void         UpdateDockEditWindow(void *);
void        *HandleDockEditWindowIDCMP(struct IntuiMessage *);
struct Node *CopyDockNode(struct Node *);
void         FreeDockNode(struct Node *);
struct Node *ReadDockNode(UBYTE *);
BOOL         WriteDockNode(struct IFFHandle *, UBYTE *, struct Node *);

/* docklistwindow.c */
void         InitDockListEditWindow(UWORD, UWORD);
BOOL         OpenDockListEditWindow(void *, struct Window *);
void         UpdateDockListEditWindow(void *);
void        *HandleDockListEditWindowIDCMP(struct IntuiMessage *);
struct List *CopyToolsList(struct List *);
void         FreeToolsList(struct List *toollist);

/* execwindow.c */
void         InitExecEditWindow(UWORD, UWORD);
BOOL         OpenExecEditWindow(void *, struct Window *);
void         UpdateExecEditWindow(void *);
void         HandleExecEditWindowAppMsg(struct AppMessage *);
void        *HandleExecEditWindowIDCMP(struct IntuiMessage *);
struct Node *CreateExecNode(char *, struct WBArg *);
struct Node *CopyExecNode(struct Node *);
void         FreeExecNode(struct Node *);
struct Node *ReadExecNode(UBYTE *);
BOOL         WriteExecNode(struct IFFHandle *, UBYTE *, struct Node *);

/* gadget.c */
struct Gadget *CreateGadgetList(struct GadgetData *, ULONG);
void           DisableGadget(struct Gadget *, struct Window *, BOOL);
char          *DuplicateBuffer(struct Gadget *);
char           FindVanillaKey(char *);
ULONG          MatchVanillaKey(unsigned char, char *);

/* iconwindow.c */
void         InitIconEditWindow(UWORD, UWORD);
BOOL         OpenIconEditWindow(void *, struct Window *);
void         UpdateIconEditWindow(void *);
void        *HandleIconEditWindowIDCMP(struct IntuiMessage *);
struct Node *CreateIconNode(char *);
struct Node *CopyIconNode(struct Node *);
void         FreeIconNode(struct Node *);
struct Node *ReadIconNode(UBYTE *);
BOOL         WriteIconNode(struct IFFHandle *, UBYTE *, struct Node *);

/* imagewindow.c */
void         InitImageEditWindow(UWORD, UWORD);
BOOL         OpenImageEditWindow(void *, struct Window *);
void         HandleImageEditWindowAppMsg(struct AppMessage *);
void        *HandleImageEditWindowIDCMP(struct IntuiMessage *);
struct Node *CreateImageNode(char *, struct WBArg *);
struct Node *CopyImageNode(struct Node *);
void         FreeImageNode(struct Node *);
struct Node *ReadImageNode(UBYTE *);
BOOL         WriteImageNode(struct IFFHandle *, UBYTE *, struct Node *);

/* listreq.c */
void  InitListRequester(UWORD, UWORD);
BOOL  OpenListRequester(ULONG, struct Window *);
void *HandleListRequesterIDCMP(struct IntuiMessage *);

/* locale.c */
void GetLocale(void);
void FreeLocale(void);

/* main.c */
void FreeAllObjects(void);
BOOL CopyFile(char *, char *);

/* mainwindow.c */
void   InitMainWindow(UWORD, UWORD);
ULONG  OpenMainWindow(UWORD, UWORD);
void   CloseMainWindow(void);
void   UpdateMainWindow(void *);
void   HandleMainWindowAppMsg(struct AppMessage *);
void  *HandleMainWindowIDCMP(struct IntuiMessage *);
void   UpdateAppMainWindow(void *);

/* menuwindow.c */
void         InitMenuEditWindow(UWORD, UWORD);
BOOL         OpenMenuEditWindow(void *, struct Window *);
void         UpdateMenuEditWindow(void *);
void        *HandleMenuEditWindowIDCMP(struct IntuiMessage *);
struct Node *CreateMenuNode(char *);
struct Node *CopyMenuNode(struct Node *);
void         FreeMenuNode(struct Node *);
struct Node *ReadMenuNode(UBYTE *);
BOOL         WriteMenuNode(struct IFFHandle *, UBYTE *, struct Node *);

/* movewindow.c */
void  InitMoveWindow(UWORD, UWORD);
void  OpenMoveWindow(struct Window *, struct Gadget *, struct Gadget *);
void  CloseMoveWindow(void);
void *HandleMoveWindowIDCMP(struct IntuiMessage *);

/* selectwindow.c */
void  InitSelectWindow(UWORD, UWORD);
BOOL  OpenSelectWindow(void *, struct Window *);
void *HandleSelectWindowIDCMP(struct IntuiMessage *);

/* soundwindow.c */
void         InitSoundEditWindow(UWORD, UWORD);
BOOL         OpenSoundEditWindow(void *, struct Window *);
void        *HandleSoundEditWindowIDCMP(struct IntuiMessage *);
struct Node *CopySoundNode(struct Node *);
void         FreeSoundNode(struct Node *);
struct Node *ReadSoundNode(UBYTE *);
BOOL         WriteSoundNode(struct IFFHandle *iff, UBYTE *, struct Node *);

/* window.c */
void CloseWindowSafely(struct Window *);
void DisableWindow(struct Window *, struct Requester *);
void EnableWindow(struct Window *, struct Requester *, ULONG);

/* System library base pointers */
extern struct Library *AslBase;
extern struct Library *DiskfontBase;
extern struct DosLibrary *DOSBase;
extern struct Library *GadToolsBase;
extern struct GfxBase *GfxBase;
extern struct Library *IconBase;
extern struct Library *IFFParseBase;
extern struct IntuitionBase *IntuitionBase;
extern struct ExecBase *SysBase;
extern struct Library *WorkbenchBase;

/* Global data */
extern char                 *ProgramName;
extern UpdateWindowFuncPtr   UpdateWindow;
extern HandleAppMsgFuncPtr   HandleAppMsg;
extern struct Window        *CurrentWindow;
extern FreeNodeFuncPtr       FreeNodeFunctions[];
extern char                 *AppStrings[];
extern struct List           ObjectLists[];
extern struct List           PubScreenList;
extern struct RastPort       TmpRastPort;
extern struct NewGadget      NewGadget;
extern struct FileReqParms   FileReqParms;
extern struct Screen        *PublicScreen;
extern void                 *ScreenVI;
extern struct TextAttr       ScreenTextAttr;
extern struct TextFont      *ScreenFont;
extern struct MsgPort       *IDCMPPort;
extern struct MsgPort       *AppMsgPort;
extern struct MsgPort       *SubWindowPort;
extern HandleIDCMPFuncPtr    SubWindowHandler;
extern struct MsgPort       *SavedSubWindowPort;
extern HandleIDCMPFuncPtr    SavedSubWindowHandler;
/* ReAction sub-window: when set, main loop uses RA_HandleInput and handler; close func called on done */
extern Object               *SubWindowRAObject;
extern BOOL                 (*SubWindowRAHandler)(Object *, ULONG result, UWORD code);
extern void                 (*SubWindowRACloseFunc)(void);
extern void                 *SubWindowRAReturnData;
extern struct Window        *MoveWindowPtr;
extern ULONG                 MoveWindowOffX,MoveWindowOffY;
extern BOOL                  OSV39;
extern BOOL                  WBScreen;
extern BOOL                  CreateIcons;
extern UWORD                 WindowTop;
extern UWORD                 ListViewColumns;
extern UWORD                 ListViewRows;
extern UWORD                 WBXOffset;
extern UWORD                 WBYOffset;
extern const struct TagItem  DisabledTags[];
extern const char            PrefsFileName[];
extern const char            SavePrefsFileName[];

/* AppStrings indices (integer subscript for SAS/C); match locale.c / toolmanager.h order */
#define IX_WINDOW_NAME_GAD 5
#define IX_WINDOW_OK_GAD 16
#define IX_WINDOW_CANCEL_GAD 17
#define IX_MAINWIN_TITLE 18
#define IX_MAINWIN_TYPE_EXEC_CYCLE_LABEL 20
#define IX_MAINWIN_TYPE_IMAGE_CYCLE_LABEL 21
#define IX_EXECWIN_NEWNAME 54
#define IX_SELECTWIN_TITLE 98
#define IX_SELECTWIN_OBJECT_GAD 99
#define IX_SELECTWIN_MENU_MX_LABEL 100
#define IX_SELECTWIN_ICON_MX_LABEL 101
#define IX_SELECTWIN_MENUICON_MX_LABEL 102
#define IX_FILEREQ_OK_GAD 106
#define IX_FILEREQ_CANCEL_GAD 108
#define IX_FONTREQ_TITLE 109

/* Global defines */
#define TMVERSION  "2.1b"
#define TMREVISION "0"
#define TMCRYEAR   "1996"

#define REQBUTTONWIDTH 20
#define SGBUFLEN 256

#define LISTREQ_EXEC  0
#define LISTREQ_IMAGE 1
#define LISTREQ_SOUND 2
#define LISTREQ_DOCK  3
#define LISTREQ_PUBSC 4
#define LREQRET_CANCEL   ((void *) -1)
#define LREQRET_NOSELECT ((void *) -2)

#ifdef DEBUG
__stkargs void kputs(char *);
__stkargs char kgetc(void);
__stkargs void kprintf(char *,...);

#define DEBUG_PUTSTR(a) kputs(a);
#define DEBUG_GETCHR    kgetc();
#define DEBUG_PRINTF(a,b)  kprintf(a,b);
#else
#define DEBUG_PUTSTR(a)
#define DEBUG_GETCHR
#define DEBUG_PRINTF(a,b)
#endif
