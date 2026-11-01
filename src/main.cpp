// MyFolderMenu - native Win32 tray application (no third-party dependencies).
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define _WIN32_WINNT 0x0A00
#define WINVER 0x0A00

#include <windows.h>
#include <commctrl.h>
#include <objbase.h>
#include <shellapi.h>

#include <string>
#include <vector>

#include "app/actions.h"
#include "app/app_state.h"
#include "app/fonts.h"
#include "app/main_window.h"
#include "app/menu_model.h"
#include "app/tray.h"
#include "core/app_info.h"
#include "core/config.h"
#include "core/paths.h"
#include "core/theme.h"
#include "ui/about_dialog.h"
#include "ui/settings_dialog.h"

namespace {

struct CommandLineOptions {
    bool menu = false;
    bool settings = false;
    bool about = false;

    bool diagnostics() const {
        return menu || settings || about;
    }
};

CommandLineOptions ParseCommandLine() {
    CommandLineOptions options;
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        return options;
    }
    for (int i = 1; i < argc; ++i) {
        if (_wcsicmp(argv[i], L"--menu") == 0) {
            options.menu = true;
        } else if (_wcsicmp(argv[i], L"--settings") == 0) {
            options.settings = true;
        } else if (_wcsicmp(argv[i], L"--about") == 0) {
            options.about = true;
        }
    }
    LocalFree(argv);
    return options;
}

void InitializeDpi(mfm::AppContext& ctx) {
    HDC screen = GetDC(nullptr);
    if (screen) {
        ctx.dpi = GetDeviceCaps(screen, LOGPIXELSX);
        ReleaseDC(nullptr, screen);
    }
    if (ctx.dpi <= 0) {
        ctx.dpi = 96;
    }
}

void LoadApplicationIcon(mfm::AppContext& ctx) {
    HICON largeIcon = nullptr;
    HICON smallIcon = nullptr;
    if (ExtractIconExW(mfm::ExePath().c_str(), 0, &largeIcon, &smallIcon, 1) > 0) {
        if (smallIcon) {
            ctx.appIcon = smallIcon;
            if (largeIcon) {
                DestroyIcon(largeIcon);
            }
        } else if (largeIcon) {
            ctx.appIcon = largeIcon;
        }
    }
    if (!ctx.appIcon) {
        ctx.appIcon = CopyIcon(ctx.icons.FolderIcon());
    }
}

bool RegisterWindowClasses(mfm::AppContext& ctx) {
    return mfm::RegisterMainWindowClass(ctx) &&
           mfm::RegisterSettingsDialogClass(ctx.instance) &&
           mfm::RegisterAboutDialogClass(ctx.instance);
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    mfm::AppContext ctx;
    ctx.instance = instance;

    CommandLineOptions options = ParseCommandLine();
    bool diagnostics = options.diagnostics();

    HANDLE mutex = nullptr;
    if (!diagnostics) {
        mutex = CreateMutexW(nullptr, FALSE, mfm::kMutexName);
        if (mutex && GetLastError() == ERROR_ALREADY_EXISTS) {
            CloseHandle(mutex);
            return 0;
        }
    }

    HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    bool comInitialized = SUCCEEDED(comResult);

    INITCOMMONCONTROLSEX controls = { sizeof(controls), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES };
    InitCommonControlsEx(&controls);

    InitializeDpi(ctx);
    mfm::InitThemeApi();
    mfm::EnsureConfigCreated();
    mfm::CreateFonts(ctx);

    ctx.taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");
    ctx.icons.FolderIcon();
    LoadApplicationIcon(ctx);

    if (!RegisterWindowClasses(ctx)) {
        if (comInitialized) {
            CoUninitialize();
        }
        if (mutex) {
            CloseHandle(mutex);
        }
        return 1;
    }

    ctx.mainWindow = CreateWindowExW(0, L"MFMMainWindow", mfm::kAppName, WS_OVERLAPPED,
                                     0, 0, 0, 0, nullptr, nullptr, instance, &ctx);
    if (!ctx.mainWindow) {
        if (comInitialized) {
            CoUninitialize();
        }
        if (mutex) {
            CloseHandle(mutex);
        }
        return 1;
    }

    mfm::AppSettings settings = mfm::LoadSettings();
    mfm::SetAppThemeMode(mfm::IsDarkTheme(settings.theme));

    if (!diagnostics) {
        mfm::AddTrayIcon(ctx, ctx.mainWindow);
    }

    if (options.menu) {
        RECT work = {};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
        POINT point = {};
        point.x = work.left + (work.right - work.left) / 2;
        point.y = work.top + (work.bottom - work.top) / 2;
        mfm::ShowTrayMenu(ctx, ctx.mainWindow, point);
        PostQuitMessage(0);
    } else if (options.settings) {
        std::vector<std::wstring> folders = mfm::LoadFolders();
        if (mfm::ShowSettingsDialog(ctx, ctx.mainWindow, settings, folders)) {
            mfm::SaveConfig(folders, settings);
        }
        PostQuitMessage(0);
    } else if (options.about) {
        mfm::ShowAboutDialog(ctx, ctx.mainWindow, settings);
        PostQuitMessage(0);
    }

    MSG message = {};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    mfm::RemoveTrayIcon(ctx.mainWindow);
    if (ctx.mainWindow) {
        DestroyWindow(ctx.mainWindow);
        ctx.mainWindow = nullptr;
    }
    mfm::CleanupAppContext(ctx);
    mfm::DestroyFonts(ctx);

    if (comInitialized) {
        CoUninitialize();
    }
    if (mutex) {
        CloseHandle(mutex);
    }
    return 0;
}
