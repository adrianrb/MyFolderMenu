#include "test_framework.h"

#include <string>
#include <vector>

#include "core/strings.h"

using namespace mfm;

TEST(Strings_TrimHandlesWhitespace) {
    CHECK_EQ(Trim(L""), L"");
    CHECK_EQ(Trim(L"   "), L"");
    CHECK_EQ(Trim(L"abc"), L"abc");
    CHECK_EQ(Trim(L"  abc  "), L"abc");
    CHECK_EQ(Trim(L"\t\r\n abc \t\r\n"), L"abc");
}

TEST(Strings_ToLowerHandlesAsciiAndAccents) {
    CHECK_EQ(ToLower(L""), L"");
    CHECK_EQ(ToLower(L"AbC"), L"abc");
    CHECK_EQ(ToLower(L"MYFOLDERMENU"), L"myfoldermenu");
    CHECK_EQ(ToLower(L"Configurações"), L"configurações");
}

TEST(Strings_EqualsCI) {
    CHECK(EqualsCI(L"", L""));
    CHECK(EqualsCI(L"abc", L"ABC"));
    CHECK(EqualsCI(L"Configurações", L"configurações"));
    CHECK(!EqualsCI(L"abc", L"abd"));
    CHECK(!EqualsCI(L"a", L"ab"));
    CHECK(!EqualsCI(L"", L"a"));
}

TEST(Strings_EscapeAmp) {
    CHECK_EQ(EscapeAmp(L""), L"");
    CHECK_EQ(EscapeAmp(L"abc"), L"abc");
    CHECK_EQ(EscapeAmp(L"a&b"), L"a&&b");
    CHECK_EQ(EscapeAmp(L"&&"), L"&&&&");
    CHECK_EQ(EscapeAmp(L"R&D"), L"R&&D");
}

TEST(Strings_SplitLines) {
    std::vector<std::wstring> lf = SplitLines(L"a\nb");
    CHECK_EQ(lf.size(), (size_t)2);
    CHECK_EQ(lf[0], L"a");
    CHECK_EQ(lf[1], L"b");

    std::vector<std::wstring> crlf = SplitLines(L"a\r\nb\r\n");
    CHECK_EQ(crlf.size(), (size_t)3);
    CHECK_EQ(crlf[0], L"a");
    CHECK_EQ(crlf[1], L"b");
    CHECK_EQ(crlf[2], L"");

    std::vector<std::wstring> cr = SplitLines(L"a\rb");
    CHECK_EQ(cr.size(), (size_t)2);
    CHECK_EQ(cr[0], L"a");
    CHECK_EQ(cr[1], L"b");

    std::vector<std::wstring> empty = SplitLines(L"");
    CHECK_EQ(empty.size(), (size_t)1);
    CHECK_EQ(empty[0], L"");

    std::vector<std::wstring> blank = SplitLines(L"a\n\nb");
    CHECK_EQ(blank.size(), (size_t)3);
    CHECK_EQ(blank[1], L"");
}

TEST(Strings_Utf8RoundTrip) {
    CHECK_EQ(Utf8ToWide(""), L"");
    CHECK_EQ(Utf8ToWide("abc"), L"abc");
    CHECK_EQ(Utf8ToWide("Configura\xC3\xA7\xC3\xB5""es"), L"Configurações");
    CHECK_EQ(Utf8ToWide("A\xC3\xA7\xC3\xA3o"), L"Ação");

    CHECK_EQ(WideToUtf8(L""), std::string());
    CHECK_EQ(WideToUtf8(L"abc"), std::string("abc"));
    CHECK_EQ(WideToUtf8(L"Configurações"), std::string("Configura\xC3\xA7\xC3\xB5""es"));

    std::wstring original = L"Pasta \\ Arquivo — ação";
    CHECK_EQ(Utf8ToWide(WideToUtf8(original)), original);
}

TEST(Strings_FileName) {
    CHECK_EQ(FileName(L""), L"");
    CHECK_EQ(FileName(L"abc"), L"abc");
    CHECK_EQ(FileName(L"C:\\a\\b.txt"), L"b.txt");
    CHECK_EQ(FileName(L"C:/a/b.txt"), L"b.txt");
    CHECK_EQ(FileName(L"C:\\a\\"), L"");
    CHECK_EQ(FileName(L"\\\\server\\share\\file"), L"file");
}

TEST(Strings_FolderLabel) {
    CHECK_EQ(FolderLabel(L"C:\\a\\b\\"), L"b");
    CHECK_EQ(FolderLabel(L"C:\\a\\b"), L"b");
    CHECK_EQ(FolderLabel(L"C:/a/b/"), L"b");
    CHECK_EQ(FolderLabel(L"b"), L"b");
    CHECK_EQ(FolderLabel(L"C:\\"), L"C:");
    CHECK_EQ(FolderLabel(L""), L"");
    CHECK_EQ(FolderLabel(L"/"), L"/");
}

TEST(Strings_FileLabel) {
    CHECK_EQ(FileLabel(L"C:\\a\\b.txt", true), L"b");
    CHECK_EQ(FileLabel(L"C:\\a\\b.txt", false), L"b.txt");
    CHECK_EQ(FileLabel(L"archive.tar.gz", true), L"archive.tar");
    CHECK_EQ(FileLabel(L"archive.tar.gz", false), L"archive.tar.gz");
    CHECK_EQ(FileLabel(L"README", true), L"README");
    CHECK_EQ(FileLabel(L".gitignore", true), L".gitignore");
    CHECK_EQ(FileLabel(L"a.", true), L"a");
    CHECK_EQ(FileLabel(L"", true), L"");
}

TEST(Strings_CiLessOrdersCaseInsensitively) {
    CiLess less;
    CHECK(less(L"a", L"B"));
    CHECK(!less(L"B", L"a"));
    CHECK(!less(L"abc", L"ABC"));
    CHECK(!less(L"ABC", L"abc"));
}
