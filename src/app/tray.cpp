#include "app/tray.h"

#include <shellapi.h>

namespace mfm {

bool AddTrayIcon(AppContext& ctx, HWND hwnd) {
    if (!hwnd) {
        return false;
    }
    NOTIFYICONDATAW data = {};
    data.cbSize = sizeof(data);
    data.hWnd = hwnd;
    data.uID = (UINT)kTrayIconId;
    data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    data.uCallbackMessage = kTrayMessage;
    data.hIcon = ctx.appIcon ? ctx.appIcon : ctx.icons.FolderIcon();
    lstrcpynW(data.szTip, kAppName, ARRAYSIZE(data.szTip));
    return Shell_NotifyIconW(NIM_ADD, &data) != FALSE;
}

void RemoveTrayIcon(HWND hwnd) {
    if (!hwnd) {
        return;
    }
    NOTIFYICONDATAW data = {};
    data.cbSize = sizeof(data);
    data.hWnd = hwnd;
    data.uID = (UINT)kTrayIconId;
    Shell_NotifyIconW(NIM_DELETE, &data);
}

} // namespace mfm
