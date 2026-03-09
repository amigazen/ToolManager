# Mapping: wbstart.library / WBStart-Handler to workbench.library

This document describes how ToolManager's WB-type tool launch uses **workbench.library** when available (45+), with **WBStart-Handler** as fallback on older systems.

## Old API (WBStart-Handler)

- **Mechanism**: Send a `struct WBStartMsg` to the message port `WBS_PORTNAME` ("WBStart-Handler Port"). If no handler is present, ToolManager started `WBS_LOADNAME` ("L:WBStart-Handler") and retried.
- **Structures**: `WBStartMsg` (see `WBStart.h`): `wbsm_Name`, `wbsm_DirLock`, `wbsm_Stack`, `wbsm_Prio`, `wbsm_NumArgs`, `wbsm_ArgList` (array of `struct WBArg`).
- **Flow**: Build message → `FindPort()` → `PutMsg()` → (optional) run L:WBStart-Handler and retry → `WaitPort()` / `GetMsg()` → success in `wbsm_Stack`.
- **Dependencies**: `WBStart.h`, `exec` FindPort/PutMsg, `dos` Lock/UnLock, SystemTags to run L:WBStart-Handler.

## New API (workbench.library)

- **Function**: `OpenWorkbenchObjectA(CONST_STRPTR name, const struct TagItem *tags)` (V44).
- **Behaviour**: Opens the named object (drawer, tool, or project) as if the user had double-clicked it; for tools/projects, Workbench launches the program and can pass a list of arguments.
- **Tags** (for launching with arguments):
  - `WBOPENA_ArgLock` (BPTR): lock for the *following* `WBOPENA_ArgName` entries (same semantics as `WBArg.wa_Lock`).
  - `WBOPENA_ArgName` (STRPTR): name relative to the last `WBOPENA_ArgLock` (same semantics as `WBArg.wa_Name`).
- **Current directory**: For reliable launching (especially with arguments), the caller should set the current directory to the tool’s parent (e.g. lock on `eo_CurrentDir`) before calling `OpenWorkbenchObjectA`; Workbench uses the caller’s current directory when resolving the tool and building the argument list.

## Mapping Table

| Old (WBStartMsg / WBStart-Handler) | New (workbench.library) |
|-----------------------------------|-------------------------|
| `wbsm_Name` (tool/project name)   | First argument `name` to `OpenWorkbenchObjectA()` |
| `wbsm_DirLock` (tool’s directory) | Call `CurrentDir(wbsm_DirLock)` before `OpenWorkbenchObjectA()`; restore with `CurrentDir(oldcd)` after. |
| `wbsm_NumArgs` / `wbsm_ArgList`   | Build a tag list: for each `WBArg` in `am_ArgList`, add `WBOPENA_ArgLock`, `wa_Lock`, `WBOPENA_ArgName`, `wa_Name`. |
| FindPort + PutMsg + WaitPort/GetMsg | Removed; replaced by direct `OpenWorkbenchObjectA()` call. |
| Starting L:WBStart-Handler        | Removed; no external handler needed. |
| `wbsm_Stack` (stack size)         | **Not supported** by `OpenWorkbenchObjectA`; launched tool uses default stack. |
| `wbsm_Prio` (priority)            | **Not supported** by `OpenWorkbenchObjectA`; launched tool uses default priority. |

## Runtime selection (45 vs 37)

- **GetWorkbench()** tries `OpenLibrary("workbench.library", 45)` first. If that succeeds, **WBUseOpenWorkbenchObject** is TRUE and WB launches use `OpenWorkbenchObjectA`.
- If that fails, it opens version 37 and **WBUseOpenWorkbenchObject** is FALSE; **StartWBProgram** uses the WBStart-Handler path.

## Code summary

1. **workbench.c**
   - Try version 45 first; on success set `WBUseOpenWorkbenchObject=TRUE`. Else open version 37 and set it FALSE. Reset flag in `FreeWorkbench()`.

2. **execobj.c**
   - **StartWBProgram()**: If `WBUseOpenWorkbenchObject`, call **StartWBProgramViaOpenObject()** (lock, CurrentDir, tag list from `am_ArgList`, `OpenWorkbenchObjectA`, restore). Else call **StartWBProgramViaHandler()** (WBStartMsg, FindPort, PutMsg, optional run of L:WBStart-Handler, WaitPort/GetMsg).

3. **ToolManagerLib.h**
   - Keep `#include "WBStart.h"` for the handler fallback; declare `extern BOOL WBUseOpenWorkbenchObject`.


## Limitations

- **Stack and priority**: When using `OpenWorkbenchObjectA`, per-tool stack and priority are not settable; the launched process uses defaults. The handler path still honours `wbsm_Stack` and `wbsm_Prio`.
- **Version**: OpenWorkbenchObjectA is used only when workbench.library 45+ is present (45.39+ is safe; see workbench.doc BUGS).

## References

- workbench.doc: `OpenWorkbenchObjectA`, WBOPENA_ArgLock, WBOPENA_ArgName, notes on CurrentDir and version bugs.
- WBStart.h: `struct WBStartMsg`, `WBS_PORTNAME`, `WBS_LOADNAME`.
- RKM / AmigaDocs: WBStartup message, `struct WBArg` (wa_Lock, wa_Name).
