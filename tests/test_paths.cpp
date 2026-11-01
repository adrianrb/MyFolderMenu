#include "test_framework.h"

#include <shlobj.h>
#include <windows.h>

#include <string>
#include <vector>

#include "core/paths.h"
#include "core/strings.h"
#include "test_util.h"

using namespace mfm;

TEST(Paths_JoinPath) {
    CHECK_EQ(JoinPath(L"C:\\a", L"b"), L"C:\\a\\b");
    CHECK_EQ(JoinPath(L"C:\\a\\", L"b"), L"C:\\a\\b");
    CHECK_EQ(JoinPath(L"C:\\a/", L"b"), L"C:\\a/b");
    CHECK_EQ(JoinPath(L"", L"b"), L"b");
    CHECK_EQ(JoinPath(L"C:\\a", L""), L"C:\\a\\");
}

TEST(Paths_DirectoryExists) {
    test::TempDir temp;
    CHECK(DirectoryExists(temp.path()));
    CHECK(!DirectoryExists(temp.Join(L"missing")));
    CHECK(!DirectoryExists(L""));

    CHECK(test::WriteBytes(temp.Join(L"file.txt"), "x"));
    CHECK(!DirectoryExists(temp.Join(L"file.txt")));
}

TEST(Paths_EnsureDirectory) {
    test::TempDir temp;
    std::wstring nested = temp.Join(L"a\\b\\c");
    CHECK(EnsureDirectory(nested));
    CHECK(DirectoryExists(nested));
    CHECK(EnsureDirectory(nested));
    CHECK(EnsureDirectory(temp.path()));
}

TEST(Paths_ExpandEnv) {
    std::wstring expanded = ExpandEnv(L"%TEMP%");
    CHECK(!expanded.empty());
    CHECK(expanded.find(L'%') == std::wstring::npos);

    wchar_t buffer[MAX_PATH] = {};
    DWORD length = GetEnvironmentVariableW(L"TEMP", buffer, MAX_PATH);
    CHECK(length > 0);
    CHECK(EqualsCI(expanded, std::wstring(buffer, length)));

    CHECK_EQ(ExpandEnv(L"plain"), L"plain");
    CHECK_EQ(ExpandEnv(L""), L"");
}

TEST(Paths_FullPath) {
    std::wstring full = FullPath(L"relative_name");
    CHECK(FileName(full) == L"relative_name");
    CHECK(full.size() > wcslen(L"relative_name"));

    std::wstring absolute = FullPath(L"C:\\Windows");
    CHECK(EqualsCI(absolute, L"C:\\Windows"));
}

TEST(Paths_NormalizeFolders) {
    std::vector<std::wstring> normalized = NormalizeFolders(
        {L"  C:\\Temp  ", L"c:\\temp", L"", L"   ", L"C:\\Other"});
    CHECK_EQ(normalized.size(), (size_t)2);
    CHECK(EqualsCI(normalized[0], L"C:\\Temp"));
    CHECK(EqualsCI(normalized[1], L"C:\\Other"));

    std::vector<std::wstring> env = NormalizeFolders({L"%TEMP%"});
    CHECK_EQ(env.size(), (size_t)1);
    CHECK(EqualsCI(env[0], FullPath(ExpandEnv(L"%TEMP%"))));

    std::vector<std::wstring> relative = NormalizeFolders({L"abc"});
    CHECK_EQ(relative.size(), (size_t)1);
    CHECK(FileName(relative[0]) == L"abc");
}

TEST(Paths_HasVisibleEntries) {
    test::TempDir temp;
    CHECK(!HasVisibleEntries(temp.path()));
    CHECK(!HasVisibleEntries(temp.Join(L"missing")));

    CHECK(test::WriteBytes(temp.Join(L"visible.txt"), "x"));
    CHECK(HasVisibleEntries(temp.path()));

    test::TempDir hiddenOnly;
    CHECK(test::CreateHiddenFile(hiddenOnly.Join(L"hidden.txt")));
    CHECK(!HasVisibleEntries(hiddenOnly.path()));
}

TEST(Paths_ListEntries) {
    test::TempDir temp;
    CHECK(temp.CreateSubdir(L"sub"));
    CHECK(test::WriteBytes(temp.Join(L"a.txt"), "a"));
    CHECK(test::WriteBytes(temp.Join(L"b.txt"), "b"));
    CHECK(test::CreateHiddenFile(temp.Join(L"hidden.txt")));

    std::vector<DirEntry> entries;
    CHECK(ListEntries(temp.path(), entries));
    CHECK_EQ(entries.size(), (size_t)3);

    int directories = 0;
    int files = 0;
    bool sawSub = false;
    bool sawA = false;
    bool sawB = false;
    bool sawHidden = false;
    for (const DirEntry& entry : entries) {
        if (entry.isDir) {
            ++directories;
        } else {
            ++files;
        }
        if (entry.name == L"sub") {
            sawSub = true;
        } else if (entry.name == L"a.txt") {
            sawA = true;
        } else if (entry.name == L"b.txt") {
            sawB = true;
        } else if (entry.name == L"hidden.txt") {
            sawHidden = true;
        }
    }
    CHECK_EQ(directories, 1);
    CHECK_EQ(files, 2);
    CHECK(sawSub);
    CHECK(sawA);
    CHECK(sawB);
    CHECK(!sawHidden);

    std::vector<DirEntry> missing;
    CHECK(!ListEntries(temp.Join(L"missing"), missing));
}

TEST(Paths_SpecialFolderAndExePath) {
    CHECK(!GetSpecialFolder(CSIDL_APPDATA).empty());
    CHECK(!GetSpecialFolder(CSIDL_PERSONAL).empty());

    std::wstring exe = ExePath();
    CHECK(!exe.empty());
    CHECK(GetFileAttributesW(exe.c_str()) != INVALID_FILE_ATTRIBUTES);
}
