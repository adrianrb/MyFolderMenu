#include "ui/dialog_common.h"

#include <uxtheme.h>

#include "core/theme.h"

namespace mfm {

void CenterWindow(HWND hwnd) {
    if (!hwnd) {
        return;
    }
    RECT window = {};
    GetWindowRect(hwnd, &window);
    RECT work = {};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    int width = window.right - window.left;
    int height = window.bottom - window.top;
    int x = work.left + ((work.right - work.left) - width) / 2;
    int y = work.top + ((work.bottom - work.top) - height) / 2;
    SetWindowPos(hwnd, nullptr, x, y, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

HWND CreateChild(const AppContext& ctx, HWND parent, const wchar_t* className,
                 const wchar_t* text, DWORD style, int x, int y, int width, int height,
                 int id, bool dark) {
    HWND control = CreateWindowExW(0, className, text, WS_CHILD | WS_VISIBLE | style,
                                   Scale(ctx, x), Scale(ctx, y),
                                   Scale(ctx, width), Scale(ctx, height),
                                   parent, (HMENU)(INT_PTR)id, ctx.instance, nullptr);
    if (control) {
        SendMessageW(control, WM_SETFONT,
                     (WPARAM)(ctx.uiFont ? ctx.uiFont : GetStockObject(DEFAULT_GUI_FONT)),
                     TRUE);
        if (dark) {
            SetWindowTheme(control, L"DarkMode_Explorer", nullptr);
        }
    }
    return control;
}

int CheckState(HWND control) {
    return (int)SendMessageW(control, BM_GETCHECK, 0, 0);
}

void SetChecked(HWND control, bool checked) {
    if (control) {
        SendMessageW(control, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
    }
}

void ActivateFocusedButtonOrOk(HWND dialog) {
    HWND focus = GetFocus();
    wchar_t className[32] = {};
    if (focus && GetClassNameW(focus, className, ARRAYSIZE(className)) > 0 &&
        _wcsicmp(className, L"Button") == 0) {
        LONG style = GetWindowLongW(focus, GWL_STYLE);
        LONG type = style & BS_TYPEMASK;
        if (type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON) {
            SendMessageW(focus, BM_CLICK, 0, 0);
            return;
        }
    }
    SendMessageW(dialog, WM_COMMAND, IDOK, 0);
}

bool RunModal(HWND hwnd, HWND owner, HWND firstFocus, bool dark) {
    if (!hwnd) {
        return false;
    }
    CenterWindow(hwnd);
    if (owner) {
        EnableWindow(owner, FALSE);
    }
    ShowWindow(hwnd, SW_SHOW);
    ApplyDarkTitleBar(hwnd, dark);
    UpdateWindow(hwnd);
    if (firstFocus) {
        SetFocus(firstFocus);
    }

    MSG msg = {};
    while (IsWindow(hwnd) && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            SendMessageW(hwnd, WM_COMMAND, IDCANCEL, 0);
            continue;
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
            ActivateFocusedButtonOrOk(hwnd);
            continue;
        }
        if (!IsDialogMessageW(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (owner) {
        EnableWindow(owner, TRUE);
        SetForegroundWindow(owner);
    }
    return true;
}

void ApplyDialogTheme(HWND hwnd, DialogTheme& theme, bool dark) {
    theme.dark = dark;
    HBRUSH oldBrush = theme.brush;
    theme.brush = CreateSolidBrush(dark ? RGB(32, 32, 32) : GetSysColor(COLOR_BTNFACE));
    if (oldBrush) {
        DeleteObject(oldBrush);
    }
    ApplyDarkTitleBar(hwnd, dark);
    EnumChildWindows(hwnd, [](HWND child, LPARAM parameter) -> BOOL {
        SetWindowTheme(child, parameter ? L"DarkMode_Explorer" : L"Explorer", nullptr);
        return TRUE;
    }, dark ? 1 : 0);
    InvalidateRect(hwnd, nullptr, TRUE);
}

void PaintDialogBackground(HWND hwnd, HDC dc, const DialogTheme& theme) {
    if (!theme.brush) {
        return;
    }
    RECT rect = {};
    GetClientRect(hwnd, &rect);
    FillRect(dc, &rect, theme.brush);
}

bool HandleDialogControlColor(const DialogTheme& theme, HDC dc, HBRUSH& brush) {
    if (!theme.brush) {
        return false;
    }
    SetTextColor(dc, theme.dark ? RGB(255, 255, 255) : GetSysColor(COLOR_BTNTEXT));
    SetBkColor(dc, theme.dark ? RGB(32, 32, 32) : GetSysColor(COLOR_BTNFACE));
    brush = theme.brush;
    return true;
}

void DestroyDialogTheme(DialogTheme& theme) {
    if (theme.brush) {
        DeleteObject(theme.brush);
        theme.brush = nullptr;
    }
}

} // namespace mfm
