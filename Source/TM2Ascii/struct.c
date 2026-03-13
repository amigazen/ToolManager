#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/text.h>
#include <prefs/prefhdr.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/iffparse.h>
#include <proto/utility.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "toolmanager.h"
#include "ToolManagerPrefs.h"

typedef struct Node *(*ReadNodeFuncPtr)(UBYTE *, ULONG);
typedef BOOL         (*WriteNodeFuncPtr)(struct IFFHandle *, UBYTE *,
                                         struct Node *);
typedef void         (*FreeNodeFuncPtr)(struct Node *);
