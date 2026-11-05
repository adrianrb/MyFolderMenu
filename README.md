# MyFolderMenu — Native C++ version (Win32)

[![English](https://img.shields.io/badge/lang-English-blue.svg)](README.md)
[![Português](https://img.shields.io/badge/lang-Português-green.svg)](README.pt.md)

Native C++ port of the original C# WinForms version of MyFolderMenu. Uses only the Win32 API and the C++ standard library; **no external dependencies** and no extra runtime required (does not need .NET installed).

## Features

- System tray icon, no main window and no taskbar entry.
- Menu with configured folders; submenus loaded on demand (lazy).
- Folders first, files second, both in alphabetical order.
- Real shell icons for folders and files.
- Hides files/folders with *hidden* and *system* attributes.
- Opens files using the default Windows application (`ShellExecute`).
- Light, Dark, and System themes (follows `AppsUseLightTheme`), including dark title bar.
- Option to hide file extensions in the menu.
- Configuration persisted in INI at `%APPDATA%\MyFolderMenu\folders.ini`.
- Automatic migration from the old `folders.json`.
- Startup with Windows via `HKCU\...\CurrentVersion\Run`.
- Settings and About dialogs, with settings export/import.
- Single instance via mutex `Local\MyFolderMenu`.
- DPI: PerMonitorV2 declared in the manifest (layout scaled by system DPI).

## Project Structure

```text
MyFolderMenu/
├── CMakeLists.txt              # build, tests, install, and CPack
├── version.txt                 # single source of version (1.0.0)
├── build_msvc.bat              # x64 build with MSVC (static CRT)
├── build_mingw.bat             # x64 build with MinGW-w64 (static linking)
├── build.sh                    # MinGW cross-compile from WSL/Linux
├── cmake/
│   └── toolchain-mingw-w64.cmake
├── src/
│   ├── main.cpp                # entry point, command line, and message loop
│   ├── core/                   # config, paths, strings, theme, startup, icons, version
│   ├── app/                    # state, fonts, tray, menu (model/draw), actions
│   ├── ui/                     # Settings/About dialogs and common helpers
│   └── resources/              # icon, manifest, resource.h, and .rc.in
├── tests/
│   ├── test_*.cpp              # 74 unit tests (484 checks)
│   ├── integration.ps1         # 16 GUI/tray integration tests
│   ├── run_tests.ps1           # full suite on Windows
│   ├── run_tests.sh            # full suite on WSL (MinGW build)
│   └── test_framework.h        # mini test framework
└── tools/
    └── package_release.ps1     # release ZIP + SHA-256
```

Each screen/feature has its own `.h`/`.cpp` pair; no more monolithic files. `CMakeLists.txt` generates the `.rc` with the version read from `version.txt`.

## Building

Requirements: CMake 3.20+ and one of the toolchains below.

### MSVC (recommended for release)

The script locates Visual Studio with `vswhere`, initializes `vcvars64`, and compiles with Ninja:

```bat
build_msvc.bat
```

Output: `build-msvc\MyFolderMenu.exe` (static CRT, no VC++ DLLs).

### MinGW-w64

With `cmake`, `g++`, and `windres` in `PATH`:

```bat
build_mingw.bat
```

Output: `build-mingw\MyFolderMenu.exe`.

### Cross-compile from WSL/Linux

```sh
./build.sh
```

Uses `x86_64-w64-mingw32-g++` and `cmake/toolchain-mingw-w64.cmake`. Output is `build-mingw/MyFolderMenu.exe` (~800 KB due to static libraries).

## Testing

On Windows (PowerShell 5.1+):

```powershell
tests\run_tests.ps1 -BuildDir build-msvc
```

The script runs unit tests (`mfm_tests.exe`) first, followed by integration tests that run the actual binary and verify:

- version resource 1.0.0 and x64 GUI PE header;
- default config creation and `folders.json` migration;
- invalid JSON/INI and preservation of existing configuration;
- Settings/About dialogs, including cancellation;
- menus with folders, empty, and in Light/Dark themes;
- tray mode, single instance, and diagnostic parameters.

Tests use the `MFM_APPDATA` variable to redirect all configuration to a temporary folder; the user's actual configuration is never touched.

On WSL, `tests/run_tests.sh` compiles with MinGW and runs both suites.

## Running

```bat
MyFolderMenu.exe
```

The app starts directly in the tray. Right-click the tray icon to open the menu; double-click opens the first available folder.

## Configuration

File: `%APPDATA%\MyFolderMenu\folders.ini`

```ini
[Folders]
Folder1=C:\Users\Adrian\Desktop
Folder2=C:\Users\Adrian\Downloads
Folder3=C:\Users\Adrian\Documents

[Settings]
HideFileExtensions=1
Theme=System
```

- `Theme`: `System`, `Light`, or `Dark` (also accepts `sistema`, `claro`, `escuro`).
- `HideFileExtensions`: `1`/`0` (accepts `true`/`false`, `sim`/`nao`).
- Paths accept environment variables (e.g., `%USERPROFILE%\Documents`).
- Duplicates are removed (case-insensitive comparison).
- If the INI does not exist, it is created with Desktop, Downloads, and Documents; if an old `folders.json` exists, it is migrated.

Settings can also be changed through the **Settings...** dialog, which allows exporting/importing the INI.

## Diagnostics (for testing)

These parameters open a direct window/menu and **ignore the single-instance mutex**:

| Parameter | Effect |
| --- | --- |
| `--menu` | Opens the menu at the center of the desktop and exits. |
| `--settings` | Opens the Settings dialog. |
| `--about` | Opens the About dialog. |
| `--version` | Prints version and exits. |

```bat
MyFolderMenu.exe --about
```

## Release Packaging

```powershell
tools\package_release.ps1 -BuildDir build-msvc
```

Generates `release\MyFolderMenu-1.0.0-win64.zip` (executable + README + version + notes) and `release\SHA256SUMS.txt`.

## Differences from the C# Version

- Menus are owner-drawn to reproduce exact light/dark theme colors and preserve shell icon alpha channels.
- No tooltips per menu item (original WinForms didn't show them either).
- Folder browser uses the classic shell browser (`SHBrowseForFolder`); WinForms used the same dialog via `FolderBrowserDialog`.
- Dialog layout scaled by monitor DPI at opening time (no dynamic repositioning when moving across different DPI monitors).
- Dark mode for standard controls uses `SetWindowTheme(..., L"DarkMode_Explorer")` and `SetPreferredAppMode`; on very old Windows 10 versions, some controls may appear in the system light theme.
- Version displayed in **About** is read from the executable's `VERSIONINFO`.

## License

Copyleft — adrian.barbosa@gmail.com.
