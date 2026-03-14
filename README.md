# ToolManager

Workbench and CLI tool manager for classic Amiga with object-oriented configuration, docks, Tools menu, and a dedicated preferences program.

## [amigazen project](http://www.amigazen.com)

*A web, suddenly*

*Forty years meditation*

*Minds awaken, free*

**amigazen project** is using modern software development tools and methods to update and rerelease classic Amiga open source software. Projects include a new AWeb, ToolManager, Amiga Python 2, and the ToolKit project – a universal SDK for Amiga.

Key to the amigazen project approach is ensuring every project can be built with the same common set of development tools and configurations, so the ToolKit project was created to provide a standard configuration for Amiga development. All *amigazen project* releases will be guaranteed to build against the ToolKit standard so that anyone can download and begin contributing straightaway without having to tailor the toolchain for their own setup.

The original author of ToolManager, Stefan Becker, is not affiliated with the amigazen project. The software was released to the public domain by Stefan Becker in 1999; see [LICENSE.md](LICENSE.md) for copyright and distribution terms.

The amigazen project philosophy is based on openness:

*Open* to anyone and everyone – *Open* source and free for all – *Open* your mind and create!

PRs for all projects are gratefully received at [GitHub](https://github.com/amigazen/). While the focus now is on classic 68k software, it is intended that all amigazen project releases can be ported to other Amiga-like systems including AROS and MorphOS where feasible.

## About ToolManager

ToolManager is a full-featured program for Workbench and CLI tool management on classic Amiga. It extends the Workbench with object-oriented configuration: Tools menu items, Workbench icons, dock windows, and more. Configuration is handled by a dedicated preferences program. The project brings ToolManager up to date so it builds against the NDK 3.2 and can use workbench.library (from AmigaOS 3.2) to launch Tools when available.

## New Features in ToolManager 2.2
- **DataTypes for Image objects:** In addition to IFF format brushes, Image objects can now use any image supported by DataTypes
- **DataTypes for Sound objects:** Sound samples can now be in any format supported by DataTypes, not only IFF 8SVX
- **Workbench v44+:** When workbench.library version 44 or higher is present, and Tools will be launched in Workbench mode via the Workbench API. WBStart-Handler no longer needs to be installed in this case
- **Icon.library v44+:** ToolManager will now use the advanced color palette loading and remapping capabilities of icon.library version 44 or higher when loading images from icons
- **Reaction Prefs:** A new Reaction native ToolManager Prefs app is included, in addition to the classic GadTools version
- **AmigaOS 3.2 support:** ToolManager 2.2 has full support for new features of AmigaOS 3.2

## Features

- **Object types:** Exec (CLI, WB, ARexx, Dock, HotKey, Network), Image, Sound, Menu (Tools menu), Icon, Dock window, Access.
- **Dock objects:** Backdrop and Sticky flags; configurable layout (columns, font, pattern, pop-up, etc.).
- **Integration:** ARexx, localization (multiple languages), networking, sound.
- **Preferences:** Standalone editor with main AppWindow; gadget keyboard shortcuts; tooltypes for the prefs program.
- **ScreenNotify:** Docks tied to a public screen can auto open when that screen opens and close when it closes (optional screennotify.library).


## Requirements

- **OS:** AmigaOS 2.04 or higher. Localization needs OS 2.1+.
- **Run-time:** A hard disk is recommended for the full distribution. Optional: **L:WBStart-Handler** (e.g. from WBStart 1.2) to start ToolManager at boot from WBStartup. Optional: **screennotify.library** for auto open/close of docks on public screens.

## Building

ToolManager is built with **SAS/C** and **smake**; the project is set up to build against the ToolKit standard and NDK 3.2.

- Build order and dependencies are in [BUILD.md](BUILD.md)

## Version 2.2 and changelog

- **2.2** (March 2026): workbench.library support to launch Tools on AmigaOS 3.2+.
- **2.1b** (13 Mar 1996): ScreenNotify support; auto open/close docks when public screens open/close.

See [CHANGELOG.md](CHANGELOG.md) for full version history.

## Frequently Asked Questions

### What is ToolManager?

ToolManager is a program that manages Workbench and CLI tools on classic Amiga. You configure Exec, Image, Sound, Menu, Icon, Dock, and Access objects via a preferences program. It provides Tools menu items, dock windows, and Workbench integration with an object-oriented configuration stored in ENV:.

### Why update ToolManager for 2026?

The original ToolManager (2.1b) depended on WBStart-Handler to launch Tools and had no workbench.library integration. Building with the ToolKit and NDK 3.2 keeps it maintainable; workbench.library support (2.2) allows launching Tools on AmigaOS 3.2 without WBStart-Handler when the library is available.

### Does ToolManager work on AmigaOS 3.1?

ToolManager 2.x was designed for AmigaOS 2.04+. The 2026 packaging builds against NDK 3.2 and is tested on 3.2.

### Can I contribute?

Yes. Code, testing, documentation, and translations are welcome. See the repository and [amigazen project](https://github.com/amigazen/) for how to submit pull requests. Distribution and commercial/military restrictions are in [LICENSE.md](LICENSE.md).

## Contact

- **GitHub:** https://github.com/amigazen/ToolManager/
- **Web:** http://www.amigazen.com/ (Amiga browser compatible)
- **amigazen:** toolkit@amigazen.com

## Acknowledgements

*Amiga* is a trademark of **Amiga Inc**.

Original ToolManager by **Stefan Becker** (1990–1996); sources released to the public domain in 1999. TM2Ascii by **Michael Illgner**. Modifications and packaging in 2026 by **amigazen project**.
