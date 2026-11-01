#include "test_framework.h"

#include <windows.h>

#include <string>

#include "core/version.h"

using namespace mfm;

TEST(Version_FormatVersion) {
    CHECK_EQ(FormatVersion(0x00010002, 0x00030004), L"1.2.3");
    CHECK_EQ(FormatVersion(0x00000000, 0x00000000), L"0.0.0");
    CHECK_EQ(FormatVersion(0xFFFF0000, 0x0000FFFF), L"65535.0.0");
    CHECK_EQ(FormatVersion(0x000A000B, 0x000C000D), L"10.11.12");
}

TEST(Version_AppVersionHasThreeComponents) {
    std::wstring version = GetAppVersionString();
    CHECK(!version.empty());
    size_t first = version.find(L'.');
    CHECK(first != std::wstring::npos);
    size_t second = version.find(L'.', first + 1);
    CHECK(second != std::wstring::npos);
    CHECK(version.find(L'.', second + 1) == std::wstring::npos);
}
