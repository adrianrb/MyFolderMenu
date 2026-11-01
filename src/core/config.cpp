#include "core/config.h"

#include <windows.h>
#include <shlobj.h>

#include <cwctype>

#include "core/paths.h"
#include "core/strings.h"

namespace mfm {

namespace {

constexpr const wchar_t* kAppDataOverride = L"MFM_APPDATA";

} // namespace

std::wstring AppDataDir() {
    // Test hook: allows the automated tests to redirect all configuration I/O.
    wchar_t overridePath[MAX_PATH] = {};
    DWORD overrideLength = GetEnvironmentVariableW(kAppDataOverride, overridePath, MAX_PATH);
    if (overrideLength > 0 && overrideLength < MAX_PATH) {
        return overridePath;
    }

    std::wstring appData = GetSpecialFolder(CSIDL_APPDATA);
    if (appData.empty()) {
        wchar_t buffer[MAX_PATH] = {};
        DWORD length = GetEnvironmentVariableW(L"APPDATA", buffer, MAX_PATH);
        if (length > 0 && length < MAX_PATH) {
            appData = std::wstring(buffer, length);
        }
    }
    if (appData.empty()) {
        return L".";
    }
    return appData + L"\\MyFolderMenu";
}

std::wstring IniPath() {
    return AppDataDir() + L"\\folders.ini";
}

std::wstring LegacyJsonPath() {
    return AppDataDir() + L"\\folders.json";
}

bool ReadTextFile(const std::wstring& path, std::wstring& out) {
    out.clear();
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    LARGE_INTEGER size = {};
    if (!GetFileSizeEx(file, &size) || size.QuadPart < 0 || size.QuadPart > 16 * 1024 * 1024) {
        CloseHandle(file);
        return false;
    }
    std::string bytes((size_t)size.QuadPart, '\0');
    DWORD total = 0;
    while (total < bytes.size()) {
        DWORD read = 0;
        if (!ReadFile(file, &bytes[total], (DWORD)(bytes.size() - total), &read, nullptr) || read == 0) {
            break;
        }
        total += read;
    }
    CloseHandle(file);
    bytes.resize(total);
    if (bytes.size() >= 3 &&
        (unsigned char)bytes[0] == 0xEF &&
        (unsigned char)bytes[1] == 0xBB &&
        (unsigned char)bytes[2] == 0xBF) {
        bytes.erase(0, 3);
    }
    out = Utf8ToWide(bytes);
    return true;
}

bool WriteTextFile(const std::wstring& path, const std::wstring& text) {
    std::string bytes = WideToUtf8(text);
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD written = 0;
    BOOL ok = WriteFile(file, bytes.data(), (DWORD)bytes.size(), &written, nullptr);
    CloseHandle(file);
    return ok != FALSE && written == bytes.size();
}

bool HasFoldersSection(const std::vector<std::wstring>& lines) {
    for (const std::wstring& raw : lines) {
        std::wstring line = Trim(raw);
        if (line.size() < 2 || line.front() != L'[' || line.back() != L']') {
            continue;
        }
        std::wstring section = Trim(line.substr(1, line.size() - 2));
        if (EqualsCI(section, L"Folders") || EqualsCI(section, L"Pastas")) {
            return true;
        }
    }
    return false;
}

std::vector<std::wstring> ParseIniFolders(const std::vector<std::wstring>& lines) {
    std::vector<std::wstring> folders;
    bool inFolders = false;
    for (const std::wstring& raw : lines) {
        std::wstring line = Trim(raw);
        if (line.empty() || line.front() == L';' || line.front() == L'#') {
            continue;
        }
        if (line.size() >= 2 && line.front() == L'[' && line.back() == L']') {
            std::wstring section = Trim(line.substr(1, line.size() - 2));
            inFolders = EqualsCI(section, L"Folders") || EqualsCI(section, L"Pastas");
            continue;
        }
        if (!inFolders) {
            continue;
        }
        size_t separator = line.find(L'=');
        if (separator == std::wstring::npos || separator == 0) {
            continue;
        }
        std::wstring value = Trim(line.substr(separator + 1));
        if (value.size() >= 2 && value.front() == L'"' && value.back() == L'"') {
            value = value.substr(1, value.size() - 2);
        }
        folders.push_back(value);
    }
    return NormalizeFolders(folders);
}

AppSettings ParseIniSettings(const std::vector<std::wstring>& lines) {
    AppSettings settings;
    bool inSettings = false;
    for (const std::wstring& raw : lines) {
        std::wstring line = Trim(raw);
        if (line.empty() || line.front() == L';' || line.front() == L'#') {
            continue;
        }
        if (line.size() >= 2 && line.front() == L'[' && line.back() == L']') {
            std::wstring section = Trim(line.substr(1, line.size() - 2));
            inSettings = EqualsCI(section, L"Settings");
            continue;
        }
        if (!inSettings) {
            continue;
        }
        size_t separator = line.find(L'=');
        if (separator == std::wstring::npos || separator == 0) {
            continue;
        }
        std::wstring key = Trim(line.substr(0, separator));
        std::wstring value = ToLower(Trim(line.substr(separator + 1)));
        if (EqualsCI(key, L"HideFileExtensions")) {
            if (value == L"1" || value == L"true" || value == L"sim" || value == L"yes") {
                settings.hideFileExtensions = true;
            } else if (value == L"0" || value == L"false" || value == L"nao" || value == L"não" || value == L"no") {
                settings.hideFileExtensions = false;
            }
        } else if (EqualsCI(key, L"Theme")) {
            settings.theme = ThemeFromIni(value);
        }
    }
    return settings;
}

std::wstring ThemeToIni(AppTheme theme) {
    switch (theme) {
        case AppTheme::Light: return L"Claro";
        case AppTheme::Dark: return L"Escuro";
        default: return L"Sistema";
    }
}

AppTheme ThemeFromIni(const std::wstring& value) {
    std::wstring lower = ToLower(Trim(value));
    if (lower == L"claro" || lower == L"light") {
        return AppTheme::Light;
    }
    if (lower == L"escuro" || lower == L"dark") {
        return AppTheme::Dark;
    }
    return AppTheme::System;
}

bool WriteIniFile(const std::wstring& path, const std::vector<std::wstring>& folders, const AppSettings& settings) {
    std::vector<std::wstring> normalized = NormalizeFolders(folders);
    std::wstring text = L"[Folders]\r\n";
    for (size_t i = 0; i < normalized.size(); ++i) {
        text += L"Folder" + std::to_wstring(i + 1) + L"=" + normalized[i] + L"\r\n";
    }
    text += L"\r\n[Settings]\r\n";
    text += std::wstring(L"HideFileExtensions=") + (settings.hideFileExtensions ? L"1" : L"0") + L"\r\n";
    text += L"Theme=" + ThemeToIni(settings.theme) + L"\r\n";
    return WriteTextFile(path, text);
}

bool ReadIniFile(const std::wstring& path, std::vector<std::wstring>& folders, AppSettings& settings) {
    std::wstring text;
    if (!ReadTextFile(path, text)) {
        return false;
    }
    std::vector<std::wstring> lines = SplitLines(text);
    if (!HasFoldersSection(lines)) {
        return false;
    }
    folders = ParseIniFolders(lines);
    settings = ParseIniSettings(lines);
    return true;
}

std::vector<std::wstring> ParseLegacyJson(const std::wstring& text) {
    std::vector<std::wstring> result;
    size_t pos = text.find(L"\"Folders\"");
    if (pos == std::wstring::npos) {
        return result;
    }
    pos = text.find(L':', pos);
    if (pos == std::wstring::npos) {
        return result;
    }
    pos = text.find(L'[', pos);
    if (pos == std::wstring::npos) {
        return result;
    }
    ++pos;
    bool arrayClosed = false;
    while (pos < text.size()) {
        while (pos < text.size() && (iswspace(text[pos]) || text[pos] == L',')) {
            ++pos;
        }
        if (pos >= text.size()) {
            break;
        }
        if (text[pos] == L']') {
            arrayClosed = true;
            break;
        }
        if (text[pos] != L'"') {
            break;
        }
        ++pos;
        std::wstring value;
        bool closed = false;
        while (pos < text.size()) {
            wchar_t c = text[pos++];
            if (c == L'"') {
                closed = true;
                break;
            }
            if (c != L'\\') {
                value += c;
                continue;
            }
            if (pos >= text.size()) {
                break;
            }
            wchar_t escaped = text[pos++];
            switch (escaped) {
                case L'"': value += L'"'; break;
                case L'\\': value += L'\\'; break;
                case L'/': value += L'/'; break;
                case L'b': value += L'\b'; break;
                case L'f': value += L'\f'; break;
                case L'n': value += L'\n'; break;
                case L'r': value += L'\r'; break;
                case L't': value += L'\t'; break;
                case L'u': {
                    if (pos + 4 > text.size()) {
                        return std::vector<std::wstring>();
                    }
                    unsigned int code = 0;
                    bool valid = true;
                    for (int i = 0; i < 4; ++i) {
                        wchar_t h = text[pos + i];
                        int digit = -1;
                        if (h >= L'0' && h <= L'9') digit = h - L'0';
                        else if (h >= L'a' && h <= L'f') digit = 10 + h - L'a';
                        else if (h >= L'A' && h <= L'F') digit = 10 + h - L'A';
                        if (digit < 0) { valid = false; break; }
                        code = code * 16 + (unsigned int)digit;
                    }
                    if (!valid) {
                        return std::vector<std::wstring>();
                    }
                    value += (wchar_t)code;
                    pos += 4;
                    break;
                }
                default: value += escaped; break;
            }
        }
        if (!closed) {
            return std::vector<std::wstring>();
        }
        result.push_back(value);
    }
    if (!arrayClosed) {
        return std::vector<std::wstring>();
    }
    return result;
}

std::vector<std::wstring> LoadLegacyFolders() {
    std::wstring text;
    if (!ReadTextFile(LegacyJsonPath(), text)) {
        return std::vector<std::wstring>();
    }
    return NormalizeFolders(ParseLegacyJson(text));
}

std::vector<std::wstring> DefaultFolders() {
    std::vector<std::wstring> folders;
    std::wstring desktop = GetSpecialFolder(CSIDL_DESKTOPDIRECTORY);
    std::wstring profile = GetSpecialFolder(CSIDL_PROFILE);
    std::wstring documents = GetSpecialFolder(CSIDL_PERSONAL);
    if (!desktop.empty()) {
        folders.push_back(desktop);
    }
    if (!profile.empty()) {
        folders.push_back(profile + L"\\Downloads");
    }
    if (!documents.empty()) {
        folders.push_back(documents);
    }
    return NormalizeFolders(folders);
}

std::vector<std::wstring> LoadFolders() {
    std::wstring text;
    if (ReadTextFile(IniPath(), text)) {
        return ParseIniFolders(SplitLines(text));
    }
    std::vector<std::wstring> legacy = LoadLegacyFolders();
    if (!legacy.empty()) {
        return legacy;
    }
    return DefaultFolders();
}

AppSettings LoadSettings() {
    std::wstring text;
    if (ReadTextFile(IniPath(), text)) {
        return ParseIniSettings(SplitLines(text));
    }
    return AppSettings();
}

bool SaveConfig(const std::vector<std::wstring>& folders, const AppSettings& settings) {
    if (!EnsureDirectory(AppDataDir())) {
        return false;
    }
    return WriteIniFile(IniPath(), folders, settings);
}

void EnsureConfigCreated() {
    if (GetFileAttributesW(IniPath().c_str()) != INVALID_FILE_ATTRIBUTES) {
        return;
    }
    std::vector<std::wstring> folders = LoadLegacyFolders();
    if (folders.empty()) {
        folders = DefaultFolders();
    }
    SaveConfig(folders, AppSettings());
}

} // namespace mfm
