#pragma once

#include <windows.h>

#include <string>

namespace mfm {

// Formats a VS_FIXEDFILEINFO version pair as "major.minor.build".
std::wstring FormatVersion(DWORD fileVersionMS, DWORD fileVersionLS);

// Reads the version resource from the running executable.
std::wstring GetAppVersionString();

} // namespace mfm
