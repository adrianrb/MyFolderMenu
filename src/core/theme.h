#pragma once

#include <windows.h>

#include "core/config.h"

namespace mfm {

// Resolves the effective theme (System follows the Windows "AppsUseLightTheme").
bool IsDarkTheme(AppTheme theme);

// Loads the undocumented uxtheme ordinals used to switch the app to dark mode.
void InitThemeApi();

// Applies the preferred app mode and flushes the menu theme cache.
void SetAppThemeMode(bool dark);

// Applies the immersive dark title bar (Windows 10 1809+ / Windows 11).
void ApplyDarkTitleBar(HWND hwnd, bool dark);

} // namespace mfm
