#pragma once

#include <windows.h>

#include "app/app_state.h"

namespace mfm {

bool RegisterMainWindowClass(AppContext& ctx);
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

} // namespace mfm
