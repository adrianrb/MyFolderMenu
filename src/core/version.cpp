#include "core/version.h"

#include <winver.h>

#include <vector>

#include "core/paths.h"

namespace mfm {

namespace {

constexpr const wchar_t* kFallbackVersion = L"1.0.0";

} // namespace

std::wstring FormatVersion(DWORD fileVersionMS, DWORD fileVersionLS) {
    return std::to_wstring(HIWORD(fileVersionMS)) + L"." +
           std::to_wstring(LOWORD(fileVersionMS)) + L"." +
           std::to_wstring(HIWORD(fileVersionLS));
}

std::wstring GetAppVersionString() {
    std::wstring exe = ExePath();
    DWORD dummy = 0;
    DWORD size = GetFileVersionInfoSizeW(exe.c_str(), &dummy);
    if (size == 0) {
        return kFallbackVersion;
    }
    std::vector<BYTE> data(size);
    if (!GetFileVersionInfoW(exe.c_str(), 0, size, data.data())) {
        return kFallbackVersion;
    }
    VS_FIXEDFILEINFO* info = nullptr;
    UINT length = 0;
    if (VerQueryValueW(data.data(), L"\\", (LPVOID*)&info, &length) && info) {
        return FormatVersion(info->dwFileVersionMS, info->dwFileVersionLS);
    }
    return kFallbackVersion;
}

} // namespace mfm
