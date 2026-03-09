/*
 * screennotify.h V1.0
 *
 * Main include file (internal)
 *
 * (c) 1995 Stefan Becker
 */

/* Compiler-independent register/saveds keywords (must be before any use) */
#include <clib/compiler-specific.h>

#ifndef __COMMODORE_DATE__
#define __COMMODORE_DATE__ __AMIGADATE__
#endif

/* OS include files */
#include <dos/dos.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/resident.h>
#include <exec/semaphores.h>
#include <intuition/screens.h>

/* OS function prototypes */
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/alib.h>

/* Compiler specific include files */
#include <lists.h>

/* Library include files */
#include "libraries/screennotify.h"

/* Client list data */
struct ClientListData {
 struct List                cld_List;
 struct SignalSemaphore     cld_Semaphore;
 struct ScreenNotifyMessage cld_Message;
 struct MsgPort             cld_MsgPort;
};

/* Library base */
struct ScreenNotifyBase {
 struct Library         snb_Library;
 UWORD                  snb_Pad;
 BPTR                   snb_Segment;
 struct Library        *snb_IntuitionBase;
 struct ClientListData  snb_CloseScreenClients;
 struct ClientListData  snb_PubScreenClients;
 struct ClientListData  snb_WorkbenchClients;
};

/* CloseScreen client node */
struct CloseScreenClientNode {
 struct Node    cscn_Node;
 struct Screen *cscn_Screen;
};

/* Library version & revision */
#define INTTOSTR(a) #a
#define SCREENNOTIFY_REVISION 0

/* Global data */
extern struct ExecBase *SysBase;
extern struct ScreenNotifyBase *ScreenNotifyBase;

/* Function prototypes */
void PatchFunctions(struct Library *);
BOOL RemoveFunctionPatches(struct Library *);
__SAVE_DS__ __ASM__ APTR _AddCloseScreenClient(__REG__(a0, struct Screen *s), __REG__(a1, struct MsgPort *port),
                                               __REG__(d0, BYTE pri));
__SAVE_DS__ __ASM__ BOOL _RemCloseScreenClient(__REG__(a0, struct Node *n));
__SAVE_DS__ __ASM__ APTR _AddPubScreenClient(__REG__(a0, struct MsgPort *port), __REG__(d0, BYTE pri));
__SAVE_DS__ __ASM__ BOOL _RemPubScreenClient(__REG__(a0, struct Node *n));
__SAVE_DS__ __ASM__ APTR _AddWorkbenchClient(__REG__(a0, struct MsgPort *port), __REG__(d0, BYTE pri));
__SAVE_DS__ __ASM__ BOOL _RemWorkbenchClient(__REG__(a0, struct Node *n));

/* Debugging */
#ifdef DEBUG
void kprintf(char *, ...);
#define DEBUGLOG(x) x
#else
#define DEBUGLOG(X)
#endif
