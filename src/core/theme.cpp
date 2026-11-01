#include "core/theme.h"

#include <dwmapi.h>
#include <uxtheme.h>

#include <cstring>

namespace mfm {

namespace {

using SetPreferredAppModeFn = int(WINAPI*)(int);
using FlushMenuThemesFn = void(WINAPI*)();

SetPreferredAppModeFn g_setPreferredAppMode = nullptr;
FlushMenuThemesFn g_flushMenuThemes = nullptr;

constexpr DWORD kDwmUseImmersiveDarkModeBefore20H1 = 19;
constexpr DWORD kDwmUseImmersiveDarkMode = 20;

} // namespace

bool IsDarkTheme(AppTheme theme) {
    if (theme == AppTheme::Dark) {
        return true;
    }
    if (theme == AppTheme::Light) {
        return false;
    }
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      0, KEY_READ, &key) != ERROR_SUCCESS) {
        return false;
    }
    DWORD value = 1;
    DWORD size = sizeof(value);
    DWORD type = 0;
    LONG result = RegQueryValueExW(key, L"AppsUseLightTheme", nullptr, &type,
                                   (LPBYTE)&value, &size);
    RegCloseKey(key);
    return result == ERROR_SUCCESS && type == REG_DWORD && value == 0;
}

void InitThemeApi() {
    HMODULE uxtheme = GetModuleHandleW(L"uxtheme.dll");
    if (!uxtheme) {
        return;
    }
    FARPROC setMode = GetProcAddress(uxtheme, MAKEINTRESOURCEA(135));
    FARPROC flushThemes = GetProcAddress(uxtheme, MAKEINTRESOURCEA(136));
    static_assert(sizeof(setMode) == sizeof(g_setPreferredAppMode),
                  "unexpected function pointer size");
    static_assert(sizeof(flushThemes) == sizeof(g_flushMenuThemes),
                  "unexpected function pointer size");
    std::memcpy(&g_setPreferredAppMode, &setMode, sizeof(setMode));
    std::memcpy(&g_flushMenuThemes, &flushThemes, sizeof(flushThemes));
}

void SetAppThemeMode(bool dark) {
    if (g_setPreferredAppMode) {
        // 2 = ForceDark, 3 = ForceLight.
        g_setPreferredAppMode(dark ? 2 : 3);
    }
    if (g_flushMenuThemes) {
        g_flushMenuThemes();
    }
}

void ApplyDarkTitleBar(HWND hwnd, bool dark) {
    if (!hwnd) {
        return;
    }
    BOOL value = dark ? TRUE : FALSE;
    if (FAILED(DwmSetWindowAttribute(hwnd, kDwmUseImmersiveDarkMode,
                                     &value, sizeof(value)))) {
        DwmSetWindowAttribute(hwnd, kDwmUseImmersiveDarkModeBefore20H1,
                              &value, sizeof(value));
    }
}

} // namespace mfm
