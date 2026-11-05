# MyFolderMenu 1.0.0

[![English](https://img.shields.io/badge/lang-English-blue.svg)](RELEASE_1.0.0.md)
[![Português](https://img.shields.io/badge/lang-Português-green.svg)](RELEASE_1.0.0.pt.md)

First stable release of the native C++/Win32 port of MyFolderMenu.

## Highlights

- **No .NET and no external dependencies**: single executable, static CRT.
- **System tray** with folder menu and on-demand submenus (lazy loading).
- **Folders first, files second**, both in alphabetical order, with real shell icons and `hidden`/`system` item hiding.
- **Light, Dark, and System themes**, including dark title bar and dark mode standard controls.
- **INI Configuration** at `%APPDATA%\MyFolderMenu\folders.ini`, with automatic migration from the old `folders.json`.
- **Autostart** via `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`.
- **Single instance** (`Local\MyFolderMenu`).
- **PerMonitorV2 DPI** declared in the manifest.
- **Settings** and **About** dialogs, with INI export/import.

## Code Structure

The code is organized into testable modules:

- `src/core` — configuration, paths, strings, theme, startup, icons, and version.
- `src/app` — state, fonts, tray, menu (model/draw), and actions.
- `src/ui` — Settings/About dialogs and common helpers.
- `src/resources` — icon, manifest, and `VERSIONINFO` generated from `version.txt`.

## Quality

- **74 unit tests** (484 checks) covering config/INI/JSON, paths, strings, theme, startup, version, menu model, measurement/drawing, and dialogs.
- **16 integration tests** that run the actual binary and validate version/PE, configuration creation/migration, dialogs, menus, tray, single instance, and diagnostic parameters.
- MSVC and MinGW-w64 builds; cross-compilation from WSL.

## Installation

1. Download `MyFolderMenu-1.0.0-win64.zip`.
2. Extract to any folder.
3. Run `MyFolderMenu.exe`. The app starts in the tray.

No runtime installation required: the executable uses static CRT.

## Requirements

- Windows 10 or 11, x64.
- No additional dependencies.

## Download Verification

```powershell
Get-FileHash .\MyFolderMenu-1.0.0-win64.zip -Algorithm SHA256
```

Compare with the hash in `SHA256SUMS.txt` attached to the release.

## Notes

- Menus are owner-drawn to faithfully reproduce theme colors and preserve shell icon alpha channel.
- Version displayed in **About** is read from the executable's `VERSIONINFO`.
- Configuration and logs are not sent anywhere; everything stays local.

## Acknowledgments

Based on the behavior of the original C# WinForms MyFolderMenu.
