#pragma once

#include <windows.h>

namespace mfm {

constexpr wchar_t kAppName[] = L"MyFolderMenu";
constexpr wchar_t kMutexName[] = L"Local\\MyFolderMenu";

constexpr UINT kTrayMessage = WM_APP + 1;
constexpr UINT_PTR kTrayIconId = 1;

constexpr UINT kMenuIdBase = 100;
constexpr int kIdStartup = 2001;
constexpr int kIdHideExt = 2002;
constexpr int kIdThemeLight = 2011;
constexpr int kIdThemeDark = 2012;
constexpr int kIdThemeSystem = 2013;
constexpr int kIdExport = 2021;
constexpr int kIdImport = 2022;

} // namespace mfm
