#include "ui/about_dialog.h"

#include <string>

#include "core/theme.h"
#include "core/version.h"
#include "ui/dialog_common.h"

namespace mfm {

namespace {

struct AboutState {
    const AppContext* ctx = nullptr;
    bool dark = false;
    DialogTheme theme;
};

LRESULT CALLBACK AboutWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    AboutState* state = (AboutState*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    switch (message) {
        case WM_CREATE: {
            CREATESTRUCTW* create = (CREATESTRUCTW*)lParam;
            state = (AboutState*)create->lpCreateParams;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)state);
            const AppContext& ctx = *state->ctx;
            state->theme.dark = state->dark;
            state->theme.brush =
                CreateSolidBrush(state->dark ? RGB(32, 32, 32) : GetSysColor(COLOR_BTNFACE));

            CreateChild(ctx, hwnd, L"STATIC", L"MyFolderMenu", SS_LEFT,
                        24, 20, 380, 24, 0, state->dark);
            std::wstring version = L"Versão " + GetAppVersionString();
            CreateChild(ctx, hwnd, L"STATIC", version.c_str(), SS_LEFT,
                        24, 55, 380, 22, 0, state->dark);
            CreateChild(ctx, hwnd, L"STATIC",
                        L"(2026) Copyleft by adrian.barbosa@gmail.com",
                        SS_LEFT, 24, 85, 380, 22, 0, state->dark);
            CreateChild(ctx, hwnd, L"BUTTON", L"OK", BS_DEFPUSHBUTTON | WS_TABSTOP,
                        316, 128, 90, 30, IDOK, state->dark);
            ApplyDarkTitleBar(hwnd, state->dark);
            return 0;
        }
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            if (!state) {
                break;
            }
            HBRUSH brush = nullptr;
            if (HandleDialogControlColor(state->theme, (HDC)wParam, brush)) {
                return (LRESULT)brush;
            }
            break;
        }
        case WM_ERASEBKGND: {
            if (!state) {
                break;
            }
            PaintDialogBackground(hwnd, (HDC)wParam, state->theme);
            return 1;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            if (state) {
                DestroyDialogTheme(state->theme);
            }
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace

bool RegisterAboutDialogClass(HINSTANCE instance) {
    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpfnWndProc = AboutWndProc;
    windowClass.lpszClassName = L"MFMAboutDialog";
    return RegisterClassExW(&windowClass) != 0;
}

void ShowAboutDialog(AppContext& ctx, HWND owner, const AppSettings& settings) {
    AboutState state;
    state.ctx = &ctx;
    state.dark = IsDarkTheme(settings.theme);
    SetAppThemeMode(state.dark);

    RECT rect = {0, 0, Scale(ctx, 430), Scale(ctx, 180)};
    DWORD style = WS_CAPTION | WS_SYSMENU;
    DWORD exStyle = WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT;
    AdjustWindowRectEx(&rect, style, FALSE, exStyle);
    HWND hwnd = CreateWindowExW(exStyle, L"MFMAboutDialog", L"Sobre o MyFolderMenu", style,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                rect.right - rect.left, rect.bottom - rect.top,
                                owner, nullptr, ctx.instance, &state);
    if (!hwnd) {
        return;
    }
    RunModal(hwnd, owner, nullptr, state.dark);
}

} // namespace mfm
