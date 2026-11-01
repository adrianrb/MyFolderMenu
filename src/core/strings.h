#pragma once

#include <string>
#include <vector>

namespace mfm {

std::wstring Trim(const std::wstring& text);
std::wstring ToLower(std::wstring text);
bool EqualsCI(const std::wstring& a, const std::wstring& b);
std::wstring EscapeAmp(const std::wstring& text);

std::vector<std::wstring> SplitLines(const std::wstring& text);
std::wstring Utf8ToWide(const std::string& bytes);
std::string WideToUtf8(const std::wstring& text);

std::wstring FileName(const std::wstring& path);
std::wstring FolderLabel(const std::wstring& path);
std::wstring FileLabel(const std::wstring& path, bool hideExtensions);

struct CiLess {
    bool operator()(const std::wstring& a, const std::wstring& b) const;
};

} // namespace mfm
