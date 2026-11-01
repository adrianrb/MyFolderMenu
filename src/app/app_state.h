#pragma once

#include <windows.h>

#include <map>
#include <memory>
#include <vector>

#include "app/menu_types.h"
#include "core/app_info.h"
#include "core/shell_icons.h"

namespace mfm {

// All mutable state owned by the application. Keeping it in one object (instead
// of file-scope globals) makes the modules testable and the ownership explicit.
struct AppContext {
    HINSTANCE instance = nullptr;
    HWND mainWindow = nullptr;

    HICON appIcon = nullptr;
    HFONT menuFont = nullptr;
    HFONT uiFont = nullptr;

    UINT taskbarCreatedMessage = 0;
    int dpi = 96;
    bool menuDark = false;

    HBRUSH menuBrush = nullptr;
    HBRUSH menuBgBrush = nullptr;

    std::vector<std::unique_ptr<MenuItemData>> menuItems;
    std::map<UINT, MenuAction> menuActions;
    std::map<HMENU, std::wstring> lazyMenus;
    UINT nextMenuId = kMenuIdBase;
    bool hideExtensions = true;

    ShellIconCache icons;

    AppContext();
    ~AppContext();

    AppContext(const AppContext&) = delete;
    AppContext& operator=(const AppContext&) = delete;
};

int Scale(const AppContext& ctx, int value);

// Releases GDI/USER objects held by the context. Safe to call more than once.
void CleanupAppContext(AppContext& ctx);

} // namespace mfm
