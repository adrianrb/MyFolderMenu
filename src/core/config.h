#pragma once

#include <string>
#include <vector>

namespace mfm {

enum class AppTheme { System, Light, Dark };

struct AppSettings {
    bool hideFileExtensions = true;
    AppTheme theme = AppTheme::System;
};

std::wstring ThemeToIni(AppTheme theme);
AppTheme ThemeFromIni(const std::wstring& value);

std::wstring AppDataDir();
std::wstring IniPath();
std::wstring LegacyJsonPath();

bool ReadTextFile(const std::wstring& path, std::wstring& out);
bool WriteTextFile(const std::wstring& path, const std::wstring& text);

bool HasFoldersSection(const std::vector<std::wstring>& lines);
std::vector<std::wstring> ParseIniFolders(const std::vector<std::wstring>& lines);
AppSettings ParseIniSettings(const std::vector<std::wstring>& lines);

bool WriteIniFile(const std::wstring& path, const std::vector<std::wstring>& folders, const AppSettings& settings);
bool ReadIniFile(const std::wstring& path, std::vector<std::wstring>& folders, AppSettings& settings);

std::vector<std::wstring> ParseLegacyJson(const std::wstring& text);
std::vector<std::wstring> LoadLegacyFolders();
std::vector<std::wstring> DefaultFolders();
std::vector<std::wstring> LoadFolders();
AppSettings LoadSettings();
bool SaveConfig(const std::vector<std::wstring>& folders, const AppSettings& settings);
void EnsureConfigCreated();

} // namespace mfm
