#pragma once

#include <windows.h>

#include <string>

#include "app/app_state.h"

namespace mfm {

UINT NewMenuId(AppContext& ctx);

void AddMenuItem(AppContext& ctx, HMENU menu, const std::wstring& text, HICON icon,
                 HMENU submenu, bool disabled, const MenuAction& action);
void AddSeparator(AppContext& ctx, HMENU menu);
void AddFolderItem(AppContext& ctx, HMENU menu, const std::wstring& path);
void AddFileItem(AppContext& ctx, HMENU menu, const std::wstring& path);

void PopulateSubmenu(AppContext& ctx, HMENU menu, const std::wstring& path);
HMENU BuildMenu(AppContext& ctx);

// Builds, tracks and dispatches the tray menu.
void ShowTrayMenu(AppContext& ctx, HWND owner, POINT point);

// Clears the bookkeeping and frees the menu brushes.
void ResetMenuState(AppContext& ctx);

} // namespace mfm
