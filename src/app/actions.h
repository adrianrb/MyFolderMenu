#pragma once

#include <windows.h>

#include <string>

#include "app/app_state.h"

namespace mfm {

void ExecuteAction(AppContext& ctx, HWND owner, const MenuAction& action);
void OpenPath(HWND owner, const std::wstring& path);
void OpenFirstFolder(HWND owner);
std::wstring BrowseForFolder(HWND owner);
void ExitApplication(AppContext& ctx);

} // namespace mfm
