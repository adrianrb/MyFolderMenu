#include "core/startup.h"

#include <string>

#include "core/app_info.h"
#include "core/paths.h"

namespace mfm {

namespace {

constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

} // namespace

bool StartupEnabledFor(const wchar_t* valueName) {
    if (!valueName || !*valueName) {
        return false;
    }
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return false;
    }
    wchar_t buffer[1024] = {};
    DWORD size = sizeof(buffer);
    DWORD type = 0;
    LONG result = RegQueryValueExW(key, valueName, nullptr, &type,
                                   (LPBYTE)buffer, &size);
    RegCloseKey(key);
    return result == ERROR_SUCCESS && type == REG_SZ && buffer[0] != 0;
}

bool StartupSetEnabledFor(const wchar_t* valueName, bool enabled) {
    if (!valueName || !*valueName) {
        return false;
    }
    HKEY key = nullptr;
    LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, kRunKey, 0, nullptr, 0,
                                  KEY_SET_VALUE, nullptr, &key, nullptr);
    if (result != ERROR_SUCCESS) {
        return false;
    }
    if (enabled) {
        std::wstring command = L"\"" + ExePath() + L"\"";
        result = RegSetValueExW(key, valueName, 0, REG_SZ,
                                (const BYTE*)command.c_str(),
                                (DWORD)((command.size() + 1) * sizeof(wchar_t)));
    } else {
        result = RegDeleteValueW(key, valueName);
        if (result == ERROR_FILE_NOT_FOUND) {
            result = ERROR_SUCCESS;
        }
    }
    RegCloseKey(key);
    return result == ERROR_SUCCESS;
}

bool StartupEnabled() {
    return StartupEnabledFor(kAppName);
}

bool StartupSetEnabled(bool enabled) {
    return StartupSetEnabledFor(kAppName, enabled);
}

} // namespace mfm
