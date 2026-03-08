/*
 * workbench.c  V2.1
 *
 * open & close icon/workbench.library
 *
 * (c) 1990-1993 Stefan Becker
 */

#include "ToolManagerLib.h"

/* Data */
static ULONG WBCount=0;
struct Library *WorkbenchBase=NULL;
BOOL WBUseOpenWorkbenchObject=FALSE;

/* Try to open workbench.library. Prefer 45+ for OpenWorkbenchObjectA (reliable
 * launch); fall back to 37 for AddAppIcon/AddAppWindow etc. when on older OS. */
BOOL GetWorkbench(void)
{
 if (!WorkbenchBase) {
  WorkbenchBase=OpenLibrary("workbench.library",45);
  if (WorkbenchBase) {
   WBUseOpenWorkbenchObject=TRUE;  /* 45.39+ safe for OpenWorkbenchObjectA */
  } else {
   WorkbenchBase=OpenLibrary("workbench.library",37);
   WBUseOpenWorkbenchObject=FALSE;
  }
 }
 if (WorkbenchBase) {
  WBCount++;
  DEBUG_PRINTF("WorkbenchBase 0x%08lx ",WorkbenchBase);
  DEBUG_PRINTF("(Count %2ld)\n",WBCount);
  DEBUG_PRINTF("OpenObject %s\n",WBUseOpenWorkbenchObject ? "yes" : "no");
  return(TRUE);
 }
 return(FALSE);
}

/* Try to close workbench.library */
void FreeWorkbench(void)
{
 if (--WBCount==0) {
  CloseLibrary(WorkbenchBase);
  WorkbenchBase=NULL;
  WBUseOpenWorkbenchObject=FALSE;
 }
}
