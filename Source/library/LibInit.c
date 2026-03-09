/*
 * LibInit.c  V2.1
 *
 * shared library C stub
 *
 * (c) 1990-1993 Stefan Becker
 */

#include "ToolManagerLib.h"

/* library name & id string */
#define INTTOSTR_INNER(a) #a
#define INTTOSTR(a) INTTOSTR_INNER(a)
const char LibName[]=TMLIBNAME;
/* const char LibId[]="$VER: " TMLIBNAME " " TMLIBVERSION "." TMLIBREVISION " (" __COMMODORE_DATE__ ")\r\n"; */
const char LibId[]="$VER: toolmanager.library " INTTOSTR(TMLIBVERSION) "." INTTOSTR(TMLIBREVISION) " "__AMIGADATE__"\n";

/* prototypes for library management functions (param names required for SAS/C __REG__) */
__SAVE_DS__ __ASM__ static struct Library *LibOpen(__REG__(a6, struct Library *lib), __REG__(d0, ULONG version));
__SAVE_DS__ __ASM__ static BPTR            LibClose(__REG__(a6, struct Library *lib));
__SAVE_DS__ __ASM__ static BPTR            LibExpunge(__REG__(a6, struct Library *lib));
         static ULONG           LibReserved(void);
__SAVE_DS__ static void            QuitToolManager(void);
__SAVE_DS__ void *AllocTMHandle(void);
__SAVE_DS__ __ASM__ static void            FreeTMHandle(__REG__(a0, struct TMHandle *handle));
__SAVE_DS__ __ASM__ BOOL            CreateTMObjectTagList(__REG__(a0, struct TMHandle *handle),
                                                     __REG__(a1, char *object), __REG__(d0, ULONG type),
                                                     __REG__(a2, struct TagItem *tags));
__SAVE_DS__ __ASM__ BOOL            DeleteTMObject(__REG__(a0, struct TMHandle *handle),
                                              __REG__(a1, char *object));
__SAVE_DS__ __ASM__ BOOL            ChangeTMObjectTagList(__REG__(a0, struct TMHandle *handle),
                                                     __REG__(a1, char *object),
                                                     __REG__(a2, struct TagItem *tags));

/* library functions table */
static const APTR LibVectors[]={
                                /* Standard functions */
                                (APTR) LibOpen,
                                (APTR) LibClose,
                                (APTR) LibExpunge,
                                (APTR) NULL,

                                /* Library specific functions */
                                (APTR) LibReserved, /* reserved for ARexx */
                                (APTR) QuitToolManager,
                                (APTR) AllocTMHandle,
                                (APTR) FreeTMHandle,
                                (APTR) CreateTMObjectTagList,
                                (APTR) DeleteTMObject,
                                (APTR) ChangeTMObjectTagList,

                                /* Table end */
                                (APTR) -1
                                };

/* misc. data */
static BPTR LibSegment=NULL;
static struct Library *PrivateDOSBase=NULL;
static struct Task *HandlerTask;
extern struct ExecBase *SysBase=NULL;
struct Library *LibBase=NULL;
BOOL Closing=FALSE;
const char DosName[]="dos.library";

/* Prototype & pragma for local calls to CreateNewProc() */
struct Task *MyCreateNewProc(struct TagItem *tags);
#if defined(__SASC) || defined(_DCC)
#pragma libcall PrivateDOSBase MyCreateNewProc 1f2 101
#endif

/* library init routine */
__SAVE_DS__ __ASM__ struct Library *LibInit(__REG__(a0, BPTR LibSegList))
{
 struct Library *MyLib;

 /* Get ExecBase */
 SysBase=*(struct ExecBase **) 4; /* AbsExecBase */

 /* Open dos.library */
 if (!(PrivateDOSBase=OpenLibrary(DosName,37))) return(0);

 LibSegment=LibSegList;

 if (!(LibBase=MyLib=MakeLibrary(LibVectors, NULL, NULL,
                                 sizeof(struct Library), NULL))) {
  CloseLibrary(PrivateDOSBase);
  return(0);
 }

 MyLib->lib_Node.ln_Type=NT_LIBRARY;
 MyLib->lib_Node.ln_Name=(char *)LibName;
 MyLib->lib_Flags=LIBF_CHANGED|LIBF_SUMUSED;
 MyLib->lib_Version=TMLIBVERSION;
 MyLib->lib_Revision=TMLIBREVISION;
 MyLib->lib_IdString=(APTR) LibId;
 AddLibrary(MyLib);

 DEBUG_PRINTF("Init Lib: %08lx ",MyLib);
 DEBUG_PRINTF("Seg: %08lx\n",LibSegment);

 return(MyLib);
}

/* shared library open function */
__SAVE_DS__ __ASM__ static struct Library *LibOpen(__REG__(a6, struct Library *lib),
                                       __REG__(d0, ULONG version))
{
 /* Handle special case: OpenCnt=0 & Handler is just closing down */
 if ((lib->lib_OpenCnt == 0) && Closing) return(NULL);

 /* Handler active? Try to start it... */
 if (!LibraryPort && !(HandlerTask=MyCreateNewProc((struct TagItem *)HandlerProcessTags)))
  return(NULL);

 /* Oh another user :-) */
 lib->lib_OpenCnt++;

 /* Reset delayed expunge flag */
 lib->lib_Flags&=~LIBF_DELEXP;

 /* Return library pointer */
 DEBUG_PRINTF("Open Lib: %ld\n",lib->lib_OpenCnt);
 return(lib);
}

/* shared library close function */
__SAVE_DS__ __ASM__ static BPTR LibClose(__REG__(a6, struct Library *lib))
{
 /* Open count already zero or more than one user? */
 if ((lib->lib_OpenCnt == 0) || (--lib->lib_OpenCnt > 0)) return(NULL);

 /* Is handler active? Yes, send him a signal if he should shut down */
 if (LibraryPort && Closing) Signal(HandlerTask,SIGBREAKF_CTRL_F);

 /* Is the delayed expunge bit set?  Yes, try to remove the library */
 if (lib->lib_Flags & LIBF_DELEXP) return(LibExpunge(lib));

 /* No. Don't remove library now */
 return(NULL);
}

/* shared library expunge function */
__SAVE_DS__ __ASM__ static BPTR LibExpunge(__REG__(a6, struct Library *lib))
{
 DEBUG_PRINTF("Expunge Lib: %08lx ",lib);
 DEBUG_PRINTF("Seg: %08lx\n",LibSegment);

 /* Does no-one use library now or is handler active/closing down?? */
 if ((lib->lib_OpenCnt > 0) || LibraryPort || Closing) {
  /* No, library still in use -> set delayed expunge flag */
  lib->lib_Flags|=LIBF_DELEXP;
  return(NULL);
 }

 /* Yes, remove library and free resources */
 Remove(&lib->lib_Node);
 FreeMem((void *) ((ULONG) lib-lib->lib_NegSize),
         lib->lib_NegSize+lib->lib_PosSize);
 if (PrivateDOSBase) {
  CloseLibrary(PrivateDOSBase);
  PrivateDOSBase=NULL;
 }

 /* return BPTR to our seglist */
 DEBUG_PUTSTR("Removing library...\n");
 return(LibSegment);
}

/* Reserved function, returns NULL */
static ULONG LibReserved(void)
{
 return(NULL);
}

/* Set quit flag for handler process */
__SAVE_DS__ static void QuitToolManager(void)
{
 /* Set flag */
 if (LibraryPort && !Closing) Closing=TRUE;
}

/* Send IPC message */
static BOOL SendIPC(struct TMHandle *handle)
{
 /* Handler ready? */
 if (LibraryPort) {
  /* Yep, send message */
  PutMsg(LibraryPort,(struct Message *) handle);

  /* Wait on reply */
  WaitPort(handle->tmh_Msg.mn_ReplyPort);

  /* Get reply */
  GetMsg(handle->tmh_Msg.mn_ReplyPort);

  /* get return code */
  return(handle->tmh_Command);
 }

 /* Oops nobody listening :-( */
 return(FALSE);
}

/* Allocate a TMHandle */
__SAVE_DS__ void *AllocTMHandle(void)
{
 struct TMHandle *handle;

 DEBUG_PUTSTR("AllocTMHandle() called.\n");

 /* Allocate memory for handle structure */
 if (handle=AllocMem(sizeof(struct TMHandle),MEMF_PUBLIC)) {
  struct MsgPort *rp;

  /* Create IPC Port */
  if (rp=CreateMsgPort()) {
   /* Init message */
   handle->tmh_Msg.mn_ReplyPort=rp;
   handle->tmh_Msg.mn_Length=sizeof(struct TMHandle);

   /* Send command to handler */
   handle->tmh_Command=TMIPC_AllocTMHandle;
   if (SendIPC(handle)) return(handle); /* All OK. */

   /* Something went wrong */
   DeleteMsgPort(handle->tmh_Msg.mn_ReplyPort);
  }
  FreeMem(handle,sizeof(struct TMHandle));
 }

 /* call failed */
 DEBUG_PUTSTR("AllocTMHandle() failed.\n");
 return(NULL);
}

__SAVE_DS__ __ASM__ static void FreeTMHandle(__REG__(a0, struct TMHandle *handle))
{
 /* Send command to handler */
 handle->tmh_Command=TMIPC_FreeTMHandle;
 SendIPC(handle);

 /* Free handle */
 DeleteMsgPort(handle->tmh_Msg.mn_ReplyPort);
 FreeMem(handle,sizeof(struct TMHandle));
}

__SAVE_DS__ __ASM__ BOOL CreateTMObjectTagList(__REG__(a0, struct TMHandle *handle),
                                   __REG__(a1, char *object),
                                   __REG__(d0, ULONG type),
                                   __REG__(a2, struct TagItem *tags))
{
 /* Sanity checks */
 if ((handle==NULL) || (object==NULL) || (type>=TMOBJTYPES))
  return(FALSE); /* Bad arguments! */

 /* Build IPC command */
 handle->tmh_Command=TMIPC_CreateTMObject;
 handle->tmh_Type=type;
 handle->tmh_Object=object;
 handle->tmh_Tags=tags;

 /* Send command to handler */
 return(SendIPC(handle));
}

/* Delete a TMObject (shared library version) */
__SAVE_DS__ __ASM__ BOOL DeleteTMObject(__REG__(a0, struct TMHandle *handle), __REG__(a1, char *object))
{
 /* Sanity checks */
 if ((handle==NULL) || (object==NULL)) return(FALSE); /* Bad arguments! */

 /* Build IPC command */
 handle->tmh_Command=TMIPC_DeleteTMObject;
 handle->tmh_Object=object;

 /* Send command to handler */
 return(SendIPC(handle));
}

/* Change a TMObject (shared library version) */
__SAVE_DS__ __ASM__ BOOL ChangeTMObjectTagList(__REG__(a0, struct TMHandle *handle),
                                   __REG__(a1, char *object),
                                   __REG__(a2, struct TagItem *tags))
{
 /* Sanity checks */
 if ((handle==NULL) || (object==NULL)) return(FALSE); /* Bad arguments! */

 /* Build IPC command */
 handle->tmh_Command=TMIPC_ChangeTMObject;
 handle->tmh_Object=object;
 handle->tmh_Tags=tags;

 /* Send command to handler */
 return(SendIPC(handle));
}

