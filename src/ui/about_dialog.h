#pragma once

#include <windows.h>

#include "app/app_state.h"
#include "core/config.h"

namespace mfm {

bool RegisterAboutDialogClass(HINSTANCE instance);
void ShowAboutDialog(AppContext& ctx, HWND owner, const AppSettings& settings);

} // namespace mfm
