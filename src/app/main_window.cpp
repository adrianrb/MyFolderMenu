#include "app/main_window.h"

#include "app/actions.h"
#include "app/menu_model.h"
#include "app/menu_view.h"
#include "app/tray.h"
#include "core/app_info.h"

namespace mfm {

bool RegisterMainWindowClass(AppContext& ctx) {
    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.hInstance = ctx.instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hIcon = ctx.appIcon;
    windowClass.hIconSm = ctx.appIcon;
    windowClass.lpfnWndProc = MainWndProc;
    windowClass.lpszClassName = L"MFMMainWindow";
    return RegisterClassExW(&windowClass) != 0;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        CREATESTRUCTW* create = (CREATESTRUCTW*)lParam;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)create->lpCreateParams);
    }
    AppContext* ctx = (AppContext*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (message) {
        case kTrayMessage: {
            UINT event = LOWORD(lParam);
            if (event == WM_RBUTTONUP || event == WM_CONTEXTMENU) {
                POINT point = {};
                GetCursorPos(&point);
                ShowTrayMenu(*ctx, hwnd, point);
            } else if (event == WM_LBUTTONDBLCLK) {
                OpenFirstFolder(hwnd);
            }
            return 0;
        }
        case WM_INITMENUPOPUP:
            if (ctx) {
                HandleMenuInitPopup(*ctx, wParam, lParam);
            }
            return 0;
        case WM_MEASUREITEM:
            if (ctx && HandleMenuMeasure(*ctx, hwnd, (MEASUREITEMSTRUCT*)lParam)) {
                return TRUE;
            }
            break;
        case WM_DRAWITEM:
            if (ctx && HandleMenuDraw(*ctx, (DRAWITEMSTRUCT*)lParam)) {
                return TRUE;
            }
            break;
        case WM_CLOSE:
            // Removing the icon here keeps the notification area in sync even
            // when the window is closed by the system (logoff/shutdown).
            RemoveTrayIcon(hwnd);
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            if (ctx && ctx->taskbarCreatedMessage != 0 &&
                message == ctx->taskbarCreatedMessage) {
                AddTrayIcon(*ctx, hwnd);
                return 0;
            }
            break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace mfm
