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
#define GetHead(l)   (((struct Node *)((l)->lh_Head))->ln_Succ ? (struct Node *)((l)->lh_Head) : (struct Node *)0)
#define GetTail(l)   (((struct Node *)((l)->lh_TailPred))->ln_Pred ? (struct Node *)((l)->lh_TailPred) : (struct Node *)0)
#define GetSucc(n)   (((struct Node *)((n)->ln_Succ))->ln_Succ ? (struct Node *)((n)->ln_Succ) : (struct Node *)0)
#define GetPred(n)   (((struct Node *)((n)->ln_Pred))->ln_Pred ? (struct Node *)((n)->ln_Pred) : (struct Node *)0)

/*
 * MinList / MinNode macros for minimal lists (no type/pri/name).
 */
#define GetHead_Min(l)   (((struct MinNode *)((l)->mlh_Head))->mln_Succ ? (struct MinNode *)((l)->mlh_Head) : (struct MinNode *)0)
#define GetTail_Min(l)   (((struct MinNode *)((l)->mlh_TailPred))->mln_Pred ? (struct MinNode *)((l)->mlh_TailPred) : (struct MinNode *)0)
#define GetSucc_Min(n)   (((struct MinNode *)((n)->mln_Succ))->mln_Succ ? (struct MinNode *)((n)->mln_Succ) : (struct MinNode *)0)
#define GetPred_Min(n)   (((struct MinNode *)((n)->mln_Pred))->mln_Pred ? (struct MinNode *)((n)->mln_Pred) : (struct MinNode *)0)

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
