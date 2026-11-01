#pragma once

#include "app/app_state.h"

namespace mfm {

bool AddTrayIcon(AppContext& ctx, HWND hwnd);
void RemoveTrayIcon(HWND hwnd);

} // namespace mfm
