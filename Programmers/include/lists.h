/*
 * lists.h -- List management macros (GetHead et al.)
 *
 * Provides common exec list/node access macros used by ToolManager.
 * Depends on exec/nodes.h and exec/lists.h from the NDK.
 * Include this when your build does not supply a compiler-specific lists.h.
 *
 * (c) 1990-1993 Stefan Becker; macro set for ToolManager refactor.
 */

#ifndef TOOLMANAGER_LISTS_H
#define TOOLMANAGER_LISTS_H

#include <exec/nodes.h>
#include <exec/lists.h>

/*
 * Full List / Node macros (struct List, struct Node from exec).
 * GetHead returns first node; GetTail returns last; GetSucc/GetPred traverse.
 * AddTail(), Remove(), AddHead(), Insert(), Enqueue() are exec library
 * functions (use clib/exec_protos.h).
 */
#define GetHead(l)   ((struct Node *)((l)->lh_Head))
#define GetTail(l)   ((struct Node *)((l)->lh_TailPred))
#define GetSucc(n)   ((struct Node *)((n)->ln_Succ))
#define GetPred(n)   ((struct Node *)((n)->ln_Pred))

/*
 * MinList / MinNode macros for minimal lists (no type/pri/name).
 */
#define GetHead_Min(l)   ((struct MinNode *)((l)->mlh_Head))
#define GetTail_Min(l)   ((struct MinNode *)((l)->mlh_TailPred))
#define GetSucc_Min(n)   ((struct MinNode *)((n)->mln_Succ))
#define GetPred_Min(n)   ((struct MinNode *)((n)->mln_Pred))

/*
 * Initialize a list header to empty state (optional; exec NewList() also does this).
 */
#ifndef NEWLIST
#define NEWLIST(l) \
	((l)->lh_Head = (struct Node *)(&(l)->lh_Tail), \
	 (l)->lh_Tail = NULL, \
	 (l)->lh_TailPred = (struct Node *)(&(l)->lh_Head))
#endif

#ifndef IsListEmpty
#define IsListEmpty(l)   (GetHead(l) == (struct Node *)(&(l)->lh_Tail))
#endif

#endif /* TOOLMANAGER_LISTS_H */
