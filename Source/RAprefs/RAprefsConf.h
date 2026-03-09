/*
 * RAprefsConf.h
 *
 * RAprefs (ReAction) configuration: extends ToolManagerConf.h with
 * Reaction GUI includes and RA main window declarations.
 */

#include "ToolManagerConf.h"

#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>
#include <gadgets/chooser.h>
#include <gadgets/string.h>
#include <images/label.h>
#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>
#include <classes/window.h>
#include <proto/button.h>
#include <proto/chooser.h>
#include <proto/label.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/window.h>
#include <proto/string.h>
#include <clib/reaction_lib_protos.h>

/* ReAction main window (replaces GadTools main window) */
void   InitRAMainWindow(UWORD left, UWORD fheight);
ULONG  OpenRAMainWindow(UWORD wx, UWORD wy);
void   CloseRAMainWindow(void);
void   HandleRAMainWindowAppMsg(struct AppMessage *msg);
BOOL   HandleRAMainWindowEvent(Object *windowObj, ULONG result, UWORD code);
Object *GetRAMainWindowObject(void);
