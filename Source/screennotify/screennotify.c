/*
 * screennotify.c V1.0
 *
 * Library routines
 *
 * (c) 1995 Stefan Becker
 */

#include "screennotify.h"

/*
 * Object file dummy entry point
 */
static ULONG Dummy(void)
{
 return(-1);
}

/* Library name and ID string (exported for ROM tag) */
const char LibraryName[] = SCREENNOTIFY_NAME;
const char LibraryID[]   = "$VER: " SCREENNOTIFY_NAME " "
                          INTTOSTR(SCREENNOTIFY_VERSION) "."
                          INTTOSTR(SCREENNOTIFY_REVISION) " ("
                          __COMMODORE_DATE__ ")\r\n";

/* Standard library function prototypes */
__SAVE_DS__ __ASM__ struct Library *LibraryInit(__REG__(a0, BPTR Segment),
                                                __REG__(a6, struct Library *ExecBase));
__SAVE_DS__ __ASM__ static struct Library *LibraryOpen(__REG__(a6, struct ScreenNotifyBase *snb));
__SAVE_DS__ __ASM__ static BPTR            LibraryClose(__REG__(a6, struct Library *lib));
__SAVE_DS__ __ASM__ static BPTR            LibraryExpunge(__REG__(a6, struct ScreenNotifyBase *snb));
        static ULONG           LibraryReserved(void);

/* Library functions table */
static const APTR LibraryVectors[] = {
 /* Standard functions */
 (APTR) LibraryOpen,
 (APTR) LibraryClose,
 (APTR) LibraryExpunge,
 (APTR) LibraryReserved,

 /* Library specific functions */
 (APTR) _AddCloseScreenClient,
 (APTR) _RemCloseScreenClient,
 (APTR) _AddPubScreenClient,
 (APTR) _RemPubScreenClient,
 (APTR) _AddWorkbenchClient,
 (APTR) _RemWorkbenchClient,

 /* End of table */
 (APTR) -1
};

/* Library bases */
struct ExecBase          *SysBase          = NULL;
struct ScreenNotifyBase *ScreenNotifyBase = NULL;

/* Initialize client list data */
static void InitClientListData(struct ClientListData *cld)
{
 /* Initialize client list */
 NewList(&cld->cld_List);

 /* Initialize semaphore */
 InitSemaphore(&cld->cld_Semaphore);

 /* Initialize message */
 cld->cld_Message.snm_Message.mn_ReplyPort = &cld->cld_MsgPort;
 cld->cld_Message.snm_Message.mn_Length    = sizeof(struct ScreenNotifyMessage);

 /* Initialize message port */
 NewList(&cld->cld_MsgPort.mp_MsgList);
 cld->cld_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
 cld->cld_MsgPort.mp_Flags        = PA_SIGNAL;
}

/* Initialize library */
__SAVE_DS__ __ASM__ struct Library *LibraryInit(__REG__(a0, BPTR Segment),
                                                __REG__(a6, struct Library *ExecBase))
{
 SysBase = (struct ExecBase *)ExecBase;

 if (ScreenNotifyBase = (struct ScreenNotifyBase *) MakeLibrary(LibraryVectors,
                                                                NULL, NULL,
                                                       sizeof(struct ScreenNotifyBase),
                                                                NULL)) {
  /* Initialize library structure */
  ScreenNotifyBase->snb_Library.lib_Node.ln_Type = NT_LIBRARY;
  ScreenNotifyBase->snb_Library.lib_Node.ln_Name = (char *)LibraryName;
  ScreenNotifyBase->snb_Library.lib_Flags        = LIBF_CHANGED | LIBF_SUMUSED;
  ScreenNotifyBase->snb_Library.lib_Version      = SCREENNOTIFY_VERSION;
  ScreenNotifyBase->snb_Library.lib_Revision    = SCREENNOTIFY_REVISION;
  ScreenNotifyBase->snb_Library.lib_IdString     = (APTR) LibraryID;
  ScreenNotifyBase->snb_Segment                  = Segment;
  ScreenNotifyBase->snb_IntuitionBase            = NULL;
  InitClientListData(&ScreenNotifyBase->snb_CloseScreenClients);
  InitClientListData(&ScreenNotifyBase->snb_PubScreenClients);
  InitClientListData(&ScreenNotifyBase->snb_WorkbenchClients);

  /* Add the library to the system */
  AddLibrary((struct Library *) ScreenNotifyBase);
 }

 /* Return new library pointer */
 return((struct Library *)ScreenNotifyBase);
}

/* Standard library function: Open. Called in Forbid() state */
__SAVE_DS__ __ASM__ static struct Library *LibraryOpen(__REG__(a6, struct ScreenNotifyBase *snb))
{
 /* Oh another user :-) */
 snb->snb_Library.lib_OpenCnt++;

 /* Reset delayed expunge flag */
 snb->snb_Library.lib_Flags &= ~LIBF_DELEXP;

 /* Check if patches have to be applied */
 if ((snb->snb_IntuitionBase == NULL) &&
     (snb->snb_IntuitionBase = OpenLibrary("intuition.library", 37)))
  /* Install patches */
  PatchFunctions(snb->snb_IntuitionBase);

 /* Return library pointer */
 return((struct Library *)snb);
}

/* Standard library function: Close. Called in Forbid() state */
__SAVE_DS__ __ASM__ static BPTR LibraryClose(__REG__(a6, struct Library *lib))
{
 BPTR rc = NULL;

 /* Open count greater zero, only one user and delayed expunge bit set? */
 if ((lib->lib_OpenCnt > 0) && (--lib->lib_OpenCnt == 0) &&
     (lib->lib_Flags & LIBF_DELEXP))
  /* Yes, try to remove the library */
  rc = LibraryExpunge((struct ScreenNotifyBase *) lib);

 DEBUGLOG(kprintf("Close result: %08lx\n", rc);)

 /* Return library segment if remove successful */
 return(rc);
}

/* Standard library function: Expunge. Called in Forbid() state */
__SAVE_DS__ __ASM__ static BPTR LibraryExpunge(__REG__(a6, struct ScreenNotifyBase *snb))
{
 BPTR rc = NULL;

 /* Does no-one use library now? */
 if (snb->snb_Library.lib_OpenCnt > 0) {
  /* No, library still in use -> set delayed expunge flag */
  snb->snb_Library.lib_Flags |= LIBF_DELEXP;

 /* Patches applied and can they be removed? */
 } else if ((snb->snb_IntuitionBase == NULL) ||
            RemoveFunctionPatches(snb->snb_IntuitionBase)) {
  /* Yes, remove library */
  Remove(&snb->snb_Library.lib_Node);

  /* Close intuition.library if needed */
  if (snb->snb_IntuitionBase) CloseLibrary(snb->snb_IntuitionBase);

  /* Return library segment */
  rc = snb->snb_Segment;

  /* Free memory for library base */
  FreeMem((void *) ((ULONG) snb - snb->snb_Library.lib_NegSize),
          snb->snb_Library.lib_NegSize + snb->snb_Library.lib_PosSize);
 }

 DEBUGLOG(kprintf("Expunge result: %08lx\n", rc);)

 /* Return library segment if remove successful */
 return(rc);
}

/* Reserved function, returns NULL */
static ULONG LibraryReserved(void)
{
 return(NULL);
}
