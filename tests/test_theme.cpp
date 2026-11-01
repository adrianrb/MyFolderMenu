#include "test_framework.h"

#include <windows.h>

#include "core/config.h"
#include "core/theme.h"

using namespace mfm;

TEST(Theme_ExplicitModes) {
    CHECK(!IsDarkTheme(AppTheme::Light));
    CHECK(IsDarkTheme(AppTheme::Dark));

    // System mode depends on the machine configuration, but must not crash and
    // must return a deterministic boolean.
    bool system = IsDarkTheme(AppTheme::System);
    CHECK(system == true || system == false);
}

TEST(Theme_ApiCallsAreSafe) {
    InitThemeApi();
    SetAppThemeMode(true);
    SetAppThemeMode(false);

    HWND window = CreateWindowExW(0, L"STATIC", L"theme", WS_OVERLAPPED,
                                  0, 0, 100, 100, nullptr, nullptr,
                                  GetModuleHandleW(nullptr), nullptr);
    CHECK(window != nullptr);
    if (window) {
        ApplyDarkTitleBar(window, true);
        ApplyDarkTitleBar(window, false);
        ApplyDarkTitleBar(nullptr, true);
        DestroyWindow(window);
    }
}
