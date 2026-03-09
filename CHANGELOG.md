# Changelog

All notable changes to ToolManager and screennotify.library are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).  
ToolManager history is taken from the original documentation (Stefan Becker, 1990–1993); ScreenNotify and 2.1b from source and patch notes.

---

## [2.2] - 2026-03-10

### Added

- **workbench.library** Now uses the Workbench API to launch Tools if running v45 or later (from AmigaOS 3.2), instead of the WBStart-Handler

---

## [2.1b] – 1996-03-13

ToolManager 2.1b patch (from stefanb_src / Old_Projects/ToolManager). ScreenNotify support; SAS/C and NDK build retained.

### Added

- **ScreenNotify support:** Docks that specify a public screen name and are "Activated" automatically open when that screen opens and close when it closes. Allows changing Workbench screen mode without manually closing docks.
- **screennotify.library** built from `Source/screennotify`; optional at runtime (ToolManager runs without auto open/close if the library is absent).

### Changed

- Version metadata set to 2.1b (TMVERSION, TMREVISION, LibId, etc.).

### Known caveats

- ScreenNotify can cause deadlocks in some situations (e.g. other programs closing a public screen). Remove `screennotify.library` to disable.
- With MUI 2.3 or older, do not open docks automatically on MUI public screens or a deadlock may occur.

---

## [2.1] – 1993-05-16

### Added

- New Exec object types: Dock, Hot Key, Network.
- New Dock object flags: Backdrop, Sticky.
- New object type: Access.
- Network support.
- Editor main window is now an AppWindow.
- Gadget keyboard shortcuts in the preferences editor.
- New tooltypes for the preferences editor.

### Fixed

- Several bug fixes.

### Documentation

- Enhanced documentation.

---

## [2.0] – 1992-09-26 (Fish Disk #752)

Complete new concept (object oriented) and almost complete rewrite.

### Added

- ToolManager split into two parts: main handler in a shared library, configuration by a Preferences program.
- Configuration file format changed to IFF; config resides in ENV:.
- Multiple docks and multi-column docks; new window design.
- Dock automatically detects largest image size.
- Sound support.
- Direct ARexx support for Exec objects.
- Locale support.
- Path from Workbench used for CLI tools.
- Separate handler task for starting WB processes.

### Changed

- ToolManager can be used without the Workbench; if Workbench is not running, no App* features are used.

---

## [1.5] – 1991-10-10 (Fish Disk #551)

### Added

- Status Window: New/Open/Append/Save As menu items for config file.
- Edit Window: File requesters for file string gadgets.
- Dock Window (à la NeXT).
- DeleteTool.
- List of all active HotKeys can be shown.
- Icon positioning in the edit window; name of program icon can be set.
- CLI tools: output file and path list; UserShell; max command line 4096 bytes.
- AppIcons without a name supported.
- Workbench screen moved to front when popping up Status window or before starting a tool via HotKey.
- TM waits up to 20 seconds for workbench.library.
- DELAY switch: wait &lt;num&gt; seconds before adding any App* stuff.
- Tools can be moved around in the list.

### Changed

- Renamed some tooltypes/parameters; some visual cues; internal changes.

---

## [1.4] – 1991-07-09 (Fish Disk #527)

### Added

- Keyboard shortcuts for tools.
- AppIcons for tools.
- Menu item can be switched off.
- Safety check before program shutdown.
- Menu item "Open TM Window" only if program icon is disabled.
- WB startup method changed; supports project icons.

### Changed

- Configuration file format completely changed (hopefully the last time).
- CLI command line parsing now done by ReadArgs().
- Status & edit window updated to new features.
- Several internal changes.

---

## [1.3] – 1991-03-13 (Fish Disk #476)

### Added

- Different configuration files supported.
- Tool definitions changeable at runtime.
- CLI & Workbench startup method.
- Selected icons passed as parameters to tools.
- Startup icon used as program icon when started from Workbench.
- Icon position in config file; program icon can be disabled.
- New menu entry "Show TM Window".
- New started ToolManager passes startup parameters to the already running ToolManager process.

### Changed

- Configuration file format slightly changed.

---

## [1.2] – 1991-01-12 (Fish Disk #442)

### Added

- Status window gadget "Save Configuration": saves actual tool list in config file.
- Status window remembers its last position.

### Changed

- Status window: no-GZZ & simple refresh type (to save memory).
- Name of icon hard-wired to "ToolManager".

### Fixed

- Small bugs in ListView gadget handling.

---

## [1.1] – 1991-01-01

### Added

- Icons can be dropped on the status window.
- Status window contains a list of all tool names.
- Tools can be removed from the list.

---

## [1.0] – 1990-11-04

### Added

- Initial release.

---

## screennotify.library

### [1.0] – 1995-03-26

- Initial release (Stefan Becker).  
- Library notifies clients of screen close, public/private screen status, and Workbench open/close.  
- Used by ToolManager 2.1b for automatic dock open/close when public screens open/close.


