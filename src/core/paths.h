#pragma once

#include <string>
#include <vector>

namespace mfm {

struct DirEntry {
    std::wstring path;
    std::wstring name;
    bool isDir = false;
};

std::wstring ExePath();
std::wstring GetSpecialFolder(int csidl);
std::wstring ExpandEnv(const std::wstring& text);
std::wstring FullPath(const std::wstring& text);
std::vector<std::wstring> NormalizeFolders(const std::vector<std::wstring>& input);

std::wstring JoinPath(const std::wstring& dir, const std::wstring& name);
bool DirectoryExists(const std::wstring& path);
bool EnsureDirectory(const std::wstring& path);

bool HasVisibleEntries(const std::wstring& path);
bool ListEntries(const std::wstring& path, std::vector<DirEntry>& entries);

} // namespace mfm
