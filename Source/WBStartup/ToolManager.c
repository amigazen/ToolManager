/*
 * ToolManager.c  V2.1
 *
 * Start & quit ToolManager
 *
 * (c) 1990-1993 Stefan Becker
 */

#ifndef __COMMODORE_DATE__
#define __COMMODORE_DATE__ __DATE__
#endif

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/locale.h>
#include "proto/toolmanager.h"

#include <stdlib.h>
#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS
#include "/locale/toolmanager.h"

extern struct ExecBase *SysBase;
extern struct IntuitionBase *IntuitionBase;
struct Library *ToolManagerBase; /* Supress DICE auto-open */
extern struct LocaleBase *LocaleBase;
struct EasyStruct es={sizeof(struct EasyStruct),0,"ToolManager",
                      MSG_UTILITIES_QUITREQ_TEXT_STR,
                      MSG_UTILITIES_QUITREQ_GAD_STR};
const char VersionString[]="\0$VER: ToolManager_Starter 2.1 ("
                           __COMMODORE_DATE__ ")";

/* CLI entry point */
int main(int argc, char *argv[])
{
 struct Task *t;

 /* Find handler task */
 Forbid();
 t=FindTask("ToolManager Handler");
 Permit();

 /* Now open library */
 if (ToolManagerBase=OpenLibrary(TMLIBNAME,0)) {
  /* Handler active? Yes, call quit function */
  if (t) {
   struct Catalog *Catalog=NULL;

   /* Try to open locale.library */
   if (LocaleBase=OpenLibrary("locale.library",38)) {

    /* Try to get catalog for current language */
    if (Catalog=OpenCatalog(NULL,"toolmanager.catalog",
                            OC_BuiltInLanguage, "english",
                            OC_Version,         3,
                            TAG_DONE)) {
     /* Get translation strings */
     es.es_TextFormat=GetCatalogStr(Catalog,MSG_UTILITIES_QUITREQ_TEXT,
                                    es.es_TextFormat);
     es.es_GadgetFormat=GetCatalogStr(Catalog,MSG_UTILITIES_QUITREQ_GAD,
                                      es.es_GadgetFormat);
    }
   }

   /* Show requester */
   if (EasyRequestArgs(NULL,&es,NULL,NULL)) (void)QuitToolManager();

   /* Free locale stuff */
   if (LocaleBase) {
    if (Catalog) CloseCatalog(Catalog);
    CloseLibrary(LocaleBase);
   }
  }
  CloseLibrary(ToolManagerBase);
 }
 exit(0);
}

/* WB entry point */
int wbmain(struct WBStartup *wbs)
{
 return(main(0,0));
}
