#include "app/actions.h"

#include <shellapi.h>
#include <shlobj.h>

#include <algorithm>
#include <vector>

#include "app/tray.h"
#include "core/config.h"
#include "core/paths.h"
#include "core/strings.h"
#include "core/theme.h"
#include "ui/settings_dialog.h"
#include "ui/about_dialog.h"

namespace mfm {

void OpenPath(HWND owner, const std::wstring& path) {
    HINSTANCE result = ShellExecuteW(owner, L"open", path.c_str(), nullptr, nullptr,
                                     SW_SHOWNORMAL);
    if ((INT_PTR)result <= 32) {
        std::wstring message = L"Não foi possível abrir:\n" + path +
                               L"\n\nCódigo: " + std::to_wstring((INT_PTR)result);
        MessageBoxW(owner, message.c_str(), kAppName, MB_OK | MB_ICONERROR);
    }
}

void OpenFirstFolder(HWND owner) {
    std::vector<std::wstring> folders = LoadFolders();
    for (const std::wstring& folder : folders) {
        if (DirectoryExists(folder)) {
            OpenPath(owner, folder);
            return;
        }
    }
}

std::wstring BrowseForFolder(HWND owner) {
    BROWSEINFOW info = {};
    info.hwndOwner = owner;
    info.lpszTitle = L"Escolha uma pasta para exibir no menu";
    info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&info);
    if (!pidl) {
        return L"";
    }
    wchar_t path[MAX_PATH] = {};
    std::wstring result;
    if (SHGetPathFromIDListW(pidl, path)) {
        result = path;
    }
    CoTaskMemFree(pidl);
    return result;
}

void ExecuteAction(AppContext& ctx, HWND owner, const MenuAction& action) {
    switch (action.type) {
        case ActionType::OpenPath:
            OpenPath(owner, action.path);
            break;
        case ActionType::AddFolder: {
            std::wstring selected = BrowseForFolder(owner);
            if (selected.empty()) {
                break;
            }
            std::vector<std::wstring> folders = LoadFolders();
            AppSettings settings = LoadSettings();
            bool exists = false;
            for (const std::wstring& folder : folders) {
                if (EqualsCI(folder, selected)) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                folders.push_back(selected);
            }
            SaveConfig(folders, settings);
            break;
        }
        case ActionType::RemoveFolder: {
            std::vector<std::wstring> folders = LoadFolders();
            AppSettings settings = LoadSettings();
            folders.erase(std::remove_if(folders.begin(), folders.end(),
                                         [&](const std::wstring& folder) {
                                             return EqualsCI(folder, action.path);
                                         }),
                          folders.end());
            SaveConfig(folders, settings);
            break;
        }
        case ActionType::Settings: {
            std::vector<std::wstring> folders = LoadFolders();
            AppSettings settings = LoadSettings();
            if (ShowSettingsDialog(ctx, owner, settings, folders)) {
                SaveConfig(folders, settings);
                SetAppThemeMode(IsDarkTheme(settings.theme));
            }
            break;
        }
        case ActionType::About:
            ShowAboutDialog(ctx, owner, LoadSettings());
            break;
        case ActionType::ExitApp:
            ExitApplication(ctx);
            break;
        default:
            break;
    }
}

void ExitApplication(AppContext& ctx) {
    if (ctx.mainWindow) {
        RemoveTrayIcon(ctx.mainWindow);
        DestroyWindow(ctx.mainWindow);
        ctx.mainWindow = nullptr;
    }
}

} // namespace mfm
