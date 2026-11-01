#pragma once

#include <windows.h>

#include <string>
#include <vector>

#include "app/app_state.h"
#include "core/config.h"

namespace mfm {

bool RegisterSettingsDialogClass(HINSTANCE instance);
bool ShowSettingsDialog(AppContext& ctx, HWND owner, AppSettings& settings,
                        std::vector<std::wstring>& folders);

} // namespace mfm
