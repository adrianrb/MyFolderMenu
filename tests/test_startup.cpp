#include "test_framework.h"

#include <windows.h>

#include <string>

#include "core/paths.h"
#include "core/startup.h"

using namespace mfm;

namespace {

std::wstring UniqueValueName() {
    static int counter = 0;
    return L"MyFolderMenu.Tests." + std::to_wstring(GetCurrentProcessId()) + L"." +
           std::to_wstring(++counter);
}

std::wstring ReadRunValue(const std::wstring& name) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_READ, &key) != ERROR_SUCCESS) {
        return std::wstring();
    }
    wchar_t buffer[2048] = {};
    DWORD size = sizeof(buffer);
    DWORD type = 0;
    LONG result = RegQueryValueExW(key, name.c_str(), nullptr, &type,
                                   (LPBYTE)buffer, &size);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS || type != REG_SZ) {
        return std::wstring();
    }
    return buffer;
}

} // namespace

TEST(Startup_CustomValueLifecycle) {
    std::wstring name = UniqueValueName();

    CHECK(StartupSetEnabledFor(name.c_str(), false));
    CHECK(!StartupEnabledFor(name.c_str()));

    CHECK(StartupSetEnabledFor(name.c_str(), true));
    CHECK(StartupEnabledFor(name.c_str()));

    std::wstring value = ReadRunValue(name);
    CHECK(!value.empty());
    CHECK_EQ(value.front(), L'"');
    CHECK_EQ(value.back(), L'"');
    CHECK(value.find(L"MyFolderMenu") != std::wstring::npos ||
          value.find(L"mfm_tests") != std::wstring::npos);
    CHECK(value.find(ExePath()) != std::wstring::npos);

    CHECK(StartupSetEnabledFor(name.c_str(), false));
    CHECK(!StartupEnabledFor(name.c_str()));
    CHECK(ReadRunValue(name).empty());

    // Deleting a value that does not exist is not an error.
    CHECK(StartupSetEnabledFor(name.c_str(), false));
}

TEST(Startup_InvalidNamesAreRejected) {
    CHECK(!StartupEnabledFor(nullptr));
    CHECK(!StartupEnabledFor(L""));
    CHECK(!StartupSetEnabledFor(nullptr, true));
    CHECK(!StartupSetEnabledFor(L"", true));
}

TEST(Startup_ProductionWrapper) {
    // The production wrapper must be safe to call even when the value is absent.
    bool initial = StartupEnabled();
    CHECK(initial == true || initial == false);
}
