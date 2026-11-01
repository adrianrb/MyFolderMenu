#include "test_framework.h"

#include <windows.h>

#include <string>
#include <vector>

#include "core/config.h"
#include "core/paths.h"
#include "core/strings.h"
#include "test_util.h"

using namespace mfm;

namespace {

std::vector<std::wstring> Lines(std::initializer_list<const wchar_t*> lines) {
    std::vector<std::wstring> result;
    for (const wchar_t* line : lines) {
        result.push_back(line);
    }
    return result;
}

bool FileExists(const std::wstring& path) {
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

} // namespace

TEST(Config_ThemeConversion) {
    CHECK_EQ(ThemeToIni(AppTheme::Light), L"Claro");
    CHECK_EQ(ThemeToIni(AppTheme::Dark), L"Escuro");
    CHECK_EQ(ThemeToIni(AppTheme::System), L"Sistema");

    CHECK(ThemeFromIni(L"Claro") == AppTheme::Light);
    CHECK(ThemeFromIni(L"claro") == AppTheme::Light);
    CHECK(ThemeFromIni(L"LIGHT") == AppTheme::Light);
    CHECK(ThemeFromIni(L"Escuro") == AppTheme::Dark);
    CHECK(ThemeFromIni(L"dark") == AppTheme::Dark);
    CHECK(ThemeFromIni(L"Sistema") == AppTheme::System);
    CHECK(ThemeFromIni(L"desconhecido") == AppTheme::System);
    CHECK(ThemeFromIni(L"  Escuro  ") == AppTheme::Dark);
}

TEST(Config_HasFoldersSection) {
    CHECK(HasFoldersSection(Lines({L"[Folders]"})));
    CHECK(HasFoldersSection(Lines({L"[Pastas]"})));
    CHECK(HasFoldersSection(Lines({L"[Settings]", L"  [folders]  "})));
    CHECK(!HasFoldersSection(Lines({L"[Settings]"})));
    CHECK(!HasFoldersSection(Lines({L"Folders"})));
    CHECK(!HasFoldersSection(Lines({})));
}

TEST(Config_ParseIniFolders) {
    std::vector<std::wstring> folders = ParseIniFolders(Lines({
        L"; comentário",
        L"[Folders]",
        L"Folder1=C:\\Temp",
        L"Folder2=\"C:\\Other\"",
        L"  Folder3 = C:\\Third  ",
        L"[Settings]",
        L"Folder4=C:\\Ignored",
    }));
    CHECK_EQ(folders.size(), (size_t)3);
    CHECK(EqualsCI(folders[0], L"C:\\Temp"));
    CHECK(EqualsCI(folders[1], L"C:\\Other"));
    CHECK(EqualsCI(folders[2], L"C:\\Third"));

    std::vector<std::wstring> portuguese = ParseIniFolders(Lines({
        L"[Pastas]",
        L"Pasta1=C:\\Um",
    }));
    CHECK_EQ(portuguese.size(), (size_t)1);
    CHECK(EqualsCI(portuguese[0], L"C:\\Um"));

    std::vector<std::wstring> comments = ParseIniFolders(Lines({
        L"[Folders]",
        L"# comentário",
        L"InvalidLine",
        L"=semchave",
        L"Folder1=C:\\Ok",
    }));
    CHECK_EQ(comments.size(), (size_t)1);

    std::vector<std::wstring> duplicate = ParseIniFolders(Lines({
        L"[Folders]",
        L"Folder1=C:\\Temp",
        L"Folder2=c:\\temp",
    }));
    CHECK_EQ(duplicate.size(), (size_t)1);
}

TEST(Config_ParseIniSettings) {
    AppSettings defaults = ParseIniSettings(Lines({L"[Settings]"}));
    CHECK(defaults.hideFileExtensions);
    CHECK(defaults.theme == AppTheme::System);

    AppSettings disabled = ParseIniSettings(Lines({
        L"[Settings]",
        L"HideFileExtensions=0",
        L"Theme=Escuro",
    }));
    CHECK(!disabled.hideFileExtensions);
    CHECK(disabled.theme == AppTheme::Dark);

    for (const wchar_t* truthy : {L"1", L"true", L"TRUE", L"sim", L"yes"}) {
        AppSettings settings = ParseIniSettings(
            Lines({L"[Settings]", (std::wstring(L"HideFileExtensions=") + truthy).c_str()}));
        CHECK(settings.hideFileExtensions);
    }

    for (const wchar_t* falsy : {L"0", L"false", L"nao", L"não", L"no"}) {
        AppSettings settings = ParseIniSettings(
            Lines({L"[Settings]", (std::wstring(L"HideFileExtensions=") + falsy).c_str()}));
        CHECK(!settings.hideFileExtensions);
    }

    AppSettings invalid = ParseIniSettings(Lines({
        L"[Settings]",
        L"HideFileExtensions=maybe",
        L"Theme=qualquer",
    }));
    CHECK(invalid.hideFileExtensions);
    CHECK(invalid.theme == AppTheme::System);

    AppSettings otherSection = ParseIniSettings(Lines({
        L"[Outra]",
        L"HideFileExtensions=0",
    }));
    CHECK(otherSection.hideFileExtensions);
}

TEST(Config_TextFileRoundTrip) {
    test::TempDir temp;
    std::wstring path = temp.Join(L"text.txt");
    CHECK(WriteTextFile(path, L"Linha 1\r\nAção\r\n"));

    std::wstring text;
    CHECK(ReadTextFile(path, text));
    CHECK_EQ(text, L"Linha 1\r\nAção\r\n");

    std::wstring bomPath = temp.Join(L"bom.txt");
    CHECK(test::WriteBytes(bomPath, "\xEF\xBB\xBF" "Conte\xC3\xBA" "do"));
    CHECK(ReadTextFile(bomPath, text));
    CHECK_EQ(text, L"Conteúdo");

    std::wstring missing;
    CHECK(!ReadTextFile(temp.Join(L"missing.txt"), missing));
}

TEST(Config_IniRoundTrip) {
    test::TempDir temp;
    std::wstring path = temp.Join(L"folders.ini");
    std::wstring unicodeFolder = temp.Join(L"pasta_ç");
    CHECK(temp.CreateSubdir(L"pasta_ç"));

    std::vector<std::wstring> folders = {unicodeFolder, temp.Join(L"outra")};
    AppSettings settings;
    settings.hideFileExtensions = false;
    settings.theme = AppTheme::Dark;

    CHECK(WriteIniFile(path, folders, settings));

    std::vector<std::wstring> loadedFolders;
    AppSettings loadedSettings;
    CHECK(ReadIniFile(path, loadedFolders, loadedSettings));
    CHECK_EQ(loadedFolders.size(), (size_t)2);
    CHECK(EqualsCI(loadedFolders[0], FullPath(unicodeFolder)));
    CHECK(EqualsCI(loadedFolders[1], FullPath(temp.Join(L"outra"))));
    CHECK(!loadedSettings.hideFileExtensions);
    CHECK(loadedSettings.theme == AppTheme::Dark);

    CHECK(WriteIniFile(path, {}, AppSettings()));
    CHECK(ReadIniFile(path, loadedFolders, loadedSettings));
    CHECK(loadedFolders.empty());
    CHECK(loadedSettings.hideFileExtensions);
    CHECK(loadedSettings.theme == AppTheme::System);
}

TEST(Config_ReadIniFileRejectsInvalidFiles) {
    test::TempDir temp;
    std::vector<std::wstring> folders;
    AppSettings settings;

    CHECK(!ReadIniFile(temp.Join(L"missing.ini"), folders, settings));

    std::wstring path = temp.Join(L"invalid.ini");
    CHECK(test::WriteTextUtf8(path, L"[Other]\r\nKey=Value\r\n"));
    CHECK(!ReadIniFile(path, folders, settings));

    CHECK(test::WriteTextUtf8(path, L"sem secao\r\n"));
    CHECK(!ReadIniFile(path, folders, settings));
}

TEST(Config_ParseLegacyJson) {
    std::vector<std::wstring> folders = ParseLegacyJson(
        L"{\"Folders\":[\"C:\\\\A\",\"C:\\\\B\"]}");
    CHECK_EQ(folders.size(), (size_t)2);
    CHECK_EQ(folders[0], L"C:\\A");
    CHECK_EQ(folders[1], L"C:\\B");

    std::vector<std::wstring> escaped = ParseLegacyJson(
        L"{\"Folders\":[\"a\\\"b\",\"c\\\\d\",\"e\\nf\"]}");
    CHECK_EQ(escaped.size(), (size_t)3);
    CHECK_EQ(escaped[0], L"a\"b");
    CHECK_EQ(escaped[1], L"c\\d");
    CHECK_EQ(escaped[2], L"e\nf");

    std::vector<std::wstring> unicode = ParseLegacyJson(
        L"{\"Folders\":[\"\\u00e1\\u00e7\\u00e3o\"]}");
    CHECK_EQ(unicode.size(), (size_t)1);
    CHECK_EQ(unicode[0], L"áção");

    CHECK(ParseLegacyJson(L"{}").empty());
    CHECK(ParseLegacyJson(L"{\"Other\":[\"x\"]}").empty());
    CHECK(ParseLegacyJson(L"{\"Folders\":[]}").empty());
    CHECK(ParseLegacyJson(L"{\"Folders\":[\"C:\\\\A\"").empty());
}

TEST(Config_ParseLegacyJsonMalformedInputs) {
    CHECK(ParseLegacyJson(L"").empty());
    CHECK(ParseLegacyJson(L"{\"Folders\":[\"C:\\\\A\"").empty());       // missing ']'
    CHECK(ParseLegacyJson(L"{\"Folders\":[\"C:\\\\A").empty());        // unclosed string
    CHECK(ParseLegacyJson(L"{\"Folders\":[\"\\u12\"]}").empty());    // short \u escape
    CHECK(ParseLegacyJson(L"{\"Folders\":[\"\\uZZZZ\"]}").empty());  // invalid hex
    CHECK(ParseLegacyJson(L"{\"Folders\":not-an-array}").empty());
    CHECK(ParseLegacyJson(L"{\"folders\":[\"A\"]}").empty());          // property is case-sensitive

    std::vector<std::wstring> trailingComma = ParseLegacyJson(L"{\"Folders\":[\"A\",]}");
    CHECK_EQ(trailingComma.size(), (size_t)1);
    CHECK_EQ(trailingComma[0], L"A");

    std::vector<std::wstring> extraContent = ParseLegacyJson(
        L"{\"Folders\":[\"A\"],\"Other\":1} restante");
    CHECK_EQ(extraContent.size(), (size_t)1);
}

TEST(Config_ParseIniFoldersEdgeCases) {
    std::vector<std::wstring> beforeSection = ParseIniFolders(Lines({
        L"Folder1=C:\\Ignorada",
        L"[Folders]",
        L"A=C:\\Com=Igual",
        L"B=\"\"",
        L"C= C:\\Com espaco ; nao e comentario",
        L"[Settings]",
        L"D=C:\\TambemIgnorada",
    }));
    CHECK_EQ(beforeSection.size(), (size_t)2);
    CHECK(EqualsCI(beforeSection[0], L"C:\\Com=Igual"));
    CHECK(EqualsCI(beforeSection[1], L"C:\\Com espaco ; nao e comentario"));

    std::vector<std::wstring> empty = ParseIniFolders(Lines({L"[Folders]"}));
    CHECK(empty.empty());

    std::vector<std::wstring> noSection = ParseIniFolders(Lines({L"Folder1=C:\\X"}));
    CHECK(noSection.empty());
}

TEST(Config_ParseIniSettingsEdgeCases) {
    AppSettings caseInsensitive = ParseIniSettings(Lines({
        L"[settings]",
        L"hidefileextensions=FALSE",
        L"theme=claro",
    }));
    CHECK(!caseInsensitive.hideFileExtensions);
    CHECK(caseInsensitive.theme == AppTheme::Light);

    AppSettings beforeSection = ParseIniSettings(Lines({
        L"HideFileExtensions=0",
        L"[Settings]",
        L"Theme=Escuro",
    }));
    CHECK(beforeSection.hideFileExtensions);
    CHECK(beforeSection.theme == AppTheme::Dark);

    AppSettings lastWins = ParseIniSettings(Lines({
        L"[Settings]",
        L"HideFileExtensions=0",
        L"HideFileExtensions=1",
        L"Theme=Claro",
        L"Theme=Escuro",
    }));
    CHECK(lastWins.hideFileExtensions);
    CHECK(lastWins.theme == AppTheme::Dark);

    AppSettings unknownValue = ParseIniSettings(Lines({
        L"[Settings]",
        L"HideFileExtensions=maybe",
    }));
    CHECK(unknownValue.hideFileExtensions);
}

TEST(Config_FileIoFailureCases) {
    test::TempDir temp;
    std::wstring text = L"unchanged";
    CHECK(!ReadTextFile(temp.Join(L"missing.txt"), text));
    CHECK(text.empty());

    CHECK(!WriteTextFile(temp.Join(L"missing_dir\\file.txt"), L"x"));

    std::wstring emptyPath = temp.Join(L"empty.txt");
    CHECK(test::WriteBytes(emptyPath, ""));
    CHECK(ReadTextFile(emptyPath, text));
    CHECK(text.empty());
}

TEST(Config_SaveConfigCreatesNestedAppDataDirectory) {
    test::TempDir temp;
    std::wstring nested = temp.Join(L"nested\\appdata");
    test::ScopedEnv env(L"MFM_APPDATA", nested);

    AppSettings settings;
    settings.hideFileExtensions = false;
    settings.theme = AppTheme::Light;
    CHECK(SaveConfig({temp.path()}, settings));
    CHECK(DirectoryExists(nested));
    CHECK(FileExists(IniPath()));

    AppSettings loaded = LoadSettings();
    CHECK(!loaded.hideFileExtensions);
    CHECK(loaded.theme == AppTheme::Light);
}

TEST(Config_LegacyJsonEmptyFallsBackToDefaults) {
    test::TempDir temp;
    test::ScopedEnv env(L"MFM_APPDATA", temp.path());
    CHECK(test::WriteTextUtf8(LegacyJsonPath(), L"{\"Folders\":[]}"));

    EnsureConfigCreated();
    CHECK(FileExists(IniPath()));
    CHECK(!LoadFolders().empty());
}

TEST(Config_ReadIniFileAcceptsPortugueseSectionAndBom) {
    test::TempDir temp;
    std::wstring path = temp.Join(L"pastas.ini");
    std::wstring content = L"[Pastas]\r\nPasta1=\"C:\\Temp\"\r\n\r\n[Settings]\r\n"
                           L"Theme=Claro\r\nHideFileExtensions=0\r\n";
    CHECK(test::WriteBytes(path, "\xEF\xBB\xBF" + test::ToUtf8(content)));

    std::vector<std::wstring> folders;
    AppSettings settings;
    CHECK(ReadIniFile(path, folders, settings));
    CHECK_EQ(folders.size(), (size_t)1);
    CHECK(EqualsCI(folders[0], L"C:\\Temp"));
    CHECK(settings.theme == AppTheme::Light);
    CHECK(!settings.hideFileExtensions);
}

TEST(Config_AppDataOverride) {
    test::TempDir temp;
    test::ScopedEnv env(L"MFM_APPDATA", temp.path());

    CHECK_EQ(AppDataDir(), temp.path());
    CHECK_EQ(IniPath(), temp.path() + L"\\folders.ini");
    CHECK_EQ(LegacyJsonPath(), temp.path() + L"\\folders.json");
}

TEST(Config_SaveAndLoad) {
    test::TempDir temp;
    test::ScopedEnv env(L"MFM_APPDATA", temp.path());

    CHECK(temp.CreateSubdir(L"a"));
    CHECK(temp.CreateSubdir(L"b"));
    std::vector<std::wstring> folders = {temp.Join(L"a"), temp.Join(L"b")};
    AppSettings settings;
    settings.hideFileExtensions = false;
    settings.theme = AppTheme::Light;

    CHECK(SaveConfig(folders, settings));
    CHECK(FileExists(IniPath()));

    std::vector<std::wstring> loadedFolders = LoadFolders();
    CHECK_EQ(loadedFolders.size(), (size_t)2);
    CHECK(EqualsCI(loadedFolders[0], FullPath(temp.Join(L"a"))));
    CHECK(EqualsCI(loadedFolders[1], FullPath(temp.Join(L"b"))));

    AppSettings loadedSettings = LoadSettings();
    CHECK(!loadedSettings.hideFileExtensions);
    CHECK(loadedSettings.theme == AppTheme::Light);
}

TEST(Config_EnsureConfigCreated) {
    test::TempDir temp;
    test::ScopedEnv env(L"MFM_APPDATA", temp.path());

    CHECK(!FileExists(IniPath()));
    EnsureConfigCreated();
    CHECK(FileExists(IniPath()));

    // A second call must not overwrite the existing file.
    CHECK(test::WriteTextUtf8(IniPath(), L"[Folders]\r\nFolder1=C:\\Custom\r\n"));
    EnsureConfigCreated();
    std::wstring text;
    CHECK(ReadTextFile(IniPath(), text));
    CHECK(text.find(L"C:\\Custom") != std::wstring::npos);
}

TEST(Config_LegacyMigration) {
    test::TempDir temp;
    test::ScopedEnv env(L"MFM_APPDATA", temp.path());

    CHECK(test::WriteTextUtf8(
        LegacyJsonPath(), L"{\"Folders\":[\"C:\\\\LegacyA\",\"C:\\\\LegacyB\"]}"));

    std::vector<std::wstring> legacy = LoadLegacyFolders();
    CHECK_EQ(legacy.size(), (size_t)2);
    CHECK(EqualsCI(legacy[0], L"C:\\LegacyA"));

    std::vector<std::wstring> folders = LoadFolders();
    CHECK_EQ(folders.size(), (size_t)2);

    EnsureConfigCreated();
    CHECK(FileExists(IniPath()));
    std::vector<std::wstring> after = LoadFolders();
    CHECK_EQ(after.size(), (size_t)2);
    CHECK(EqualsCI(after[0], L"C:\\LegacyA"));
}

TEST(Config_DefaultsWhenNothingIsConfigured) {
    test::TempDir temp;
    test::ScopedEnv env(L"MFM_APPDATA", temp.path());

    std::vector<std::wstring> defaults = DefaultFolders();
    CHECK(!defaults.empty());

    std::vector<std::wstring> folders = LoadFolders();
    CHECK(!folders.empty());

    AppSettings settings = LoadSettings();
    CHECK(settings.hideFileExtensions);
    CHECK(settings.theme == AppTheme::System);
}
