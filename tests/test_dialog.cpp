#include "test_framework.h"

#include <windows.h>

#include "app/app_state.h"
#include "ui/about_dialog.h"
#include "ui/dialog_common.h"
#include "ui/settings_dialog.h"

using namespace mfm;

namespace {

struct CommandSink {
    int lastCommand = 0;
    int commandCount = 0;
};

bool RegisterClassOnce(const wchar_t* name, WNDPROC proc) {
    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpfnWndProc = proc;
    windowClass.lpszClassName = name;
    if (RegisterClassExW(&windowClass)) {
        return true;
    }
    return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

LRESULT CALLBACK CommandSinkProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        CREATESTRUCTW* create = (CREATESTRUCTW*)lParam;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)create->lpCreateParams);
    }
    CommandSink* sink = (CommandSink*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (message == WM_COMMAND && sink) {
        sink->lastCommand = LOWORD(wParam);
        ++sink->commandCount;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK ModalProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        CREATESTRUCTW* create = (CREATESTRUCTW*)lParam;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)create->lpCreateParams);
    }
    CommandSink* sink = (CommandSink*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (message == WM_COMMAND) {
        if (sink) {
            sink->lastCommand = LOWORD(wParam);
            ++sink->commandCount;
        }
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            DestroyWindow(hwnd);
            return 0;
        }
    }
    if (message == WM_CLOSE) {
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

HWND CreateSinkWindow(CommandSink* sink, const wchar_t* className = L"MFMTestSink") {
    return CreateWindowExW(0, className, L"test", WS_OVERLAPPED,
                           0, 0, 300, 200, nullptr, nullptr,
                           GetModuleHandleW(nullptr), sink);
}

HWND CreateModalWindow(CommandSink* sink) {
    return CreateWindowExW(WS_EX_DLGMODALFRAME, L"MFMTestModal", L"modal",
                           WS_CAPTION | WS_SYSMENU, 0, 0, 300, 200,
                           nullptr, nullptr, GetModuleHandleW(nullptr), sink);
}

struct DialogFixture {
    AppContext ctx;

    DialogFixture() {
        ctx.instance = GetModuleHandleW(nullptr);
        ctx.dpi = 96;
        ctx.uiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        ctx.menuFont = ctx.uiFont;
    }
};

} // namespace

TEST(Dialog_CreateChildUsesParentFontAndIds) {
    CHECK(RegisterClassOnce(L"MFMTestSink", CommandSinkProc));
    DialogFixture fixture;
    CommandSink sink;
    HWND parent = CreateSinkWindow(&sink);
    CHECK(parent != nullptr);

    HWND child = CreateChild(fixture.ctx, parent, L"BUTTON", L"Teste",
                             BS_PUSHBUTTON | WS_TABSTOP, 10, 20, 120, 24, 1001, false);
    CHECK(child != nullptr);
    CHECK_EQ(GetParent(child), parent);
    CHECK_EQ(GetDlgCtrlID(child), 1001);
    CHECK(GetWindowLongW(child, GWL_STYLE) & WS_CHILD);

    wchar_t text[64] = {};
    GetWindowTextW(child, text, ARRAYSIZE(text));
    CHECK_EQ(std::wstring(text), L"Teste");

    RECT rect = {};
    GetWindowRect(child, &rect);
    CHECK_EQ(rect.right - rect.left, Scale(fixture.ctx, 120));
    CHECK_EQ(rect.bottom - rect.top, Scale(fixture.ctx, 24));

    DestroyWindow(parent);
}

TEST(Dialog_CheckStateHelpers) {
    CHECK(RegisterClassOnce(L"MFMTestSink", CommandSinkProc));
    DialogFixture fixture;
    CommandSink sink;
    HWND parent = CreateSinkWindow(&sink);

    HWND checkbox = CreateChild(fixture.ctx, parent, L"BUTTON", L"Check",
                                BS_AUTOCHECKBOX | WS_TABSTOP, 10, 10, 120, 24, 1002, false);
    CHECK(checkbox != nullptr);
    CHECK_EQ(CheckState(checkbox), BST_UNCHECKED);

    SetChecked(checkbox, true);
    CHECK_EQ(CheckState(checkbox), BST_CHECKED);
    SetChecked(checkbox, false);
    CHECK_EQ(CheckState(checkbox), BST_UNCHECKED);

    SetChecked(nullptr, true);
    DestroyWindow(parent);
}

TEST(Dialog_ActivateFocusedButtonOrOk) {
    CHECK(RegisterClassOnce(L"MFMTestSink", CommandSinkProc));
    DialogFixture fixture;
    CommandSink sink;
    HWND parent = CreateSinkWindow(&sink);
    ShowWindow(parent, SW_SHOW);
    SetActiveWindow(parent);

    HWND ok = CreateChild(fixture.ctx, parent, L"BUTTON", L"OK",
                          BS_DEFPUSHBUTTON | WS_TABSTOP, 10, 10, 80, 24, IDOK, false);
    HWND cancel = CreateChild(fixture.ctx, parent, L"BUTTON", L"Cancelar",
                              BS_PUSHBUTTON | WS_TABSTOP, 100, 10, 80, 24, IDCANCEL, false);
    HWND checkbox = CreateChild(fixture.ctx, parent, L"BUTTON", L"Check",
                                BS_AUTOCHECKBOX | WS_TABSTOP, 10, 50, 120, 24, 1003, false);
    CHECK(ok && cancel && checkbox);

    sink.lastCommand = 0;
    SetFocus(cancel);
    ActivateFocusedButtonOrOk(parent);
    CHECK_EQ(sink.lastCommand, IDCANCEL);

    sink.lastCommand = 0;
    SetFocus(ok);
    ActivateFocusedButtonOrOk(parent);
    CHECK_EQ(sink.lastCommand, IDOK);

    sink.lastCommand = 0;
    SetFocus(checkbox);
    ActivateFocusedButtonOrOk(parent);
    CHECK_EQ(sink.lastCommand, IDOK);

    sink.lastCommand = 0;
    SetFocus(parent);
    ActivateFocusedButtonOrOk(parent);
    CHECK_EQ(sink.lastCommand, IDOK);

    DestroyWindow(parent);
}

TEST(Dialog_CenterWindow) {
    CHECK(RegisterClassOnce(L"MFMTestSink", CommandSinkProc));
    CommandSink sink;
    HWND window = CreateSinkWindow(&sink);
    CHECK(window != nullptr);
    SetWindowPos(window, nullptr, 0, 0, 200, 100, SWP_NOZORDER | SWP_NOACTIVATE);

    CenterWindow(window);

    RECT windowRect = {};
    GetWindowRect(window, &windowRect);
    RECT work = {};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    int width = windowRect.right - windowRect.left;
    int height = windowRect.bottom - windowRect.top;
    CHECK_EQ(windowRect.left, work.left + ((work.right - work.left) - width) / 2);
    CHECK_EQ(windowRect.top, work.top + ((work.bottom - work.top) - height) / 2);

    CenterWindow(nullptr);
    DestroyWindow(window);
}

TEST(Dialog_ThemeHelpers) {
    CHECK(RegisterClassOnce(L"MFMTestSink", CommandSinkProc));
    DialogFixture fixture;
    CommandSink sink;
    HWND window = CreateSinkWindow(&sink);
    HWND child = CreateChild(fixture.ctx, window, L"BUTTON", L"Child",
                             BS_PUSHBUTTON, 10, 10, 80, 24, 2001, false);

    DialogTheme theme;
    ApplyDialogTheme(window, theme, true);
    CHECK(theme.dark);
    CHECK(theme.brush != nullptr);

    HDC dc = GetDC(window);
    CHECK(dc != nullptr);
    HBRUSH brush = nullptr;
    CHECK(HandleDialogControlColor(theme, dc, brush));
    CHECK(brush == theme.brush);
    PaintDialogBackground(window, dc, theme);
    ReleaseDC(window, dc);

    HBRUSH oldBrush = theme.brush;
    ApplyDialogTheme(window, theme, false);
    CHECK(!theme.dark);
    CHECK(theme.brush != nullptr);
    CHECK(theme.brush != oldBrush);

    DestroyDialogTheme(theme);
    CHECK(theme.brush == nullptr);

    HDC secondDc = GetDC(window);
    HBRUSH nullBrush = nullptr;
    CHECK(!HandleDialogControlColor(theme, secondDc, nullBrush));
    PaintDialogBackground(window, secondDc, theme);
    ReleaseDC(window, secondDc);

    DestroyWindow(child);
    DestroyWindow(window);
}

TEST(Dialog_RunModalCancel) {
    CHECK(RegisterClassOnce(L"MFMTestModal", ModalProc));
    CHECK(RegisterClassOnce(L"MFMTestSink", CommandSinkProc));
    CommandSink modalSink;
    CommandSink ownerSink;
    HWND owner = CreateSinkWindow(&ownerSink);
    HWND modal = CreateModalWindow(&modalSink);
    CHECK(owner && modal);

    PostMessageW(modal, WM_COMMAND, IDCANCEL, 0);
    CHECK(RunModal(modal, owner, nullptr, false));
    CHECK(!IsWindow(modal));
    CHECK(IsWindowEnabled(owner));
    CHECK_EQ(modalSink.lastCommand, IDCANCEL);

    DestroyWindow(owner);
}

TEST(Dialog_RunModalEnterActivatesFocusedButton) {
    CHECK(RegisterClassOnce(L"MFMTestModal", ModalProc));
    CHECK(RegisterClassOnce(L"MFMTestSink", CommandSinkProc));
    CommandSink modalSink;
    CommandSink ownerSink;
    HWND owner = CreateSinkWindow(&ownerSink);
    HWND modal = CreateModalWindow(&modalSink);
    HWND cancel = CreateWindowExW(0, L"BUTTON", L"Cancelar",
                                  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                  10, 10, 90, 30, modal, (HMENU)(INT_PTR)IDCANCEL,
                                  GetModuleHandleW(nullptr), nullptr);
    CHECK(owner && modal && cancel);

    PostMessageW(modal, WM_KEYDOWN, VK_RETURN, 0);
    CHECK(RunModal(modal, owner, cancel, false));
    CHECK(!IsWindow(modal));
    CHECK_EQ(modalSink.lastCommand, IDCANCEL);

    DestroyWindow(owner);
}

TEST(Dialog_RunModalEnterFallsBackToOk) {
    CHECK(RegisterClassOnce(L"MFMTestModal", ModalProc));
    CHECK(RegisterClassOnce(L"MFMTestSink", CommandSinkProc));
    CommandSink modalSink;
    CommandSink ownerSink;
    HWND owner = CreateSinkWindow(&ownerSink);
    HWND modal = CreateModalWindow(&modalSink);
    HWND checkbox = CreateWindowExW(0, L"BUTTON", L"Check",
                                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                    10, 10, 120, 30, modal, (HMENU)(INT_PTR)3001,
                                    GetModuleHandleW(nullptr), nullptr);
    CHECK(owner && modal && checkbox);

    PostMessageW(modal, WM_KEYDOWN, VK_RETURN, 0);
    CHECK(RunModal(modal, owner, checkbox, false));
    CHECK(!IsWindow(modal));
    CHECK_EQ(modalSink.lastCommand, IDOK);

    DestroyWindow(owner);
}

TEST(Dialog_RunModalEscapeCancels) {
    CHECK(RegisterClassOnce(L"MFMTestModal", ModalProc));
    CHECK(RegisterClassOnce(L"MFMTestSink", CommandSinkProc));
    CommandSink modalSink;
    CommandSink ownerSink;
    HWND owner = CreateSinkWindow(&ownerSink);
    HWND modal = CreateModalWindow(&modalSink);
    CHECK(owner && modal);

    PostMessageW(modal, WM_KEYDOWN, VK_ESCAPE, 0);
    CHECK(RunModal(modal, owner, nullptr, false));
    CHECK(!IsWindow(modal));
    CHECK_EQ(modalSink.lastCommand, IDCANCEL);

    DestroyWindow(owner);
}

TEST(Dialog_RunModalNullWindow) {
    CHECK(!RunModal(nullptr, nullptr, nullptr, false));
}

TEST(Dialog_DialogClassesCanBeRegistered) {
    CHECK(RegisterSettingsDialogClass(GetModuleHandleW(nullptr)));
    CHECK(!RegisterSettingsDialogClass(GetModuleHandleW(nullptr)));
    CHECK(RegisterAboutDialogClass(GetModuleHandleW(nullptr)));
    CHECK(!RegisterAboutDialogClass(GetModuleHandleW(nullptr)));
}
