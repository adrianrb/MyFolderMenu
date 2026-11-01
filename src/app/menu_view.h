#pragma once

#include <windows.h>

#include "app/app_state.h"

namespace mfm {

// Applies the themed background to a menu and all of its submenus.
void ApplyMenuInfo(AppContext& ctx, HMENU menu);

// Owner-draw handlers. Return true when the message was handled.
bool HandleMenuMeasure(AppContext& ctx, HWND hwnd, MEASUREITEMSTRUCT* measure);
bool HandleMenuDraw(AppContext& ctx, DRAWITEMSTRUCT* draw);

// Lazily populates a submenu on WM_INITMENUPOPUP. Return true when handled.
bool HandleMenuInitPopup(AppContext& ctx, WPARAM wParam, LPARAM lParam);

} // namespace mfm
