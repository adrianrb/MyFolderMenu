#include "core/strings.h"

#include <windows.h>

#include <cwctype>

namespace mfm {

std::wstring Trim(const std::wstring& text) {
    size_t start = 0;
    size_t end = text.size();
    while (start < end && iswspace(text[start])) {
        ++start;
    }
    while (end > start && iswspace(text[end - 1])) {
        --end;
    }
    return text.substr(start, end - start);
}

std::wstring ToLower(std::wstring text) {
    if (!text.empty()) {
        CharLowerBuffW(&text[0], (DWORD)text.size());
    }
    return text;
}

bool EqualsCI(const std::wstring& a, const std::wstring& b) {
    return CompareStringOrdinal(a.c_str(), (int)a.size(), b.c_str(), (int)b.size(), TRUE) == CSTR_EQUAL;
}

std::wstring EscapeAmp(const std::wstring& text) {
    std::wstring out;
    out.reserve(text.size() + 4);
    for (wchar_t c : text) {
        if (c == L'&') {
            out += L'&';
        }
        out += c;
    }
    return out;
}

std::vector<std::wstring> SplitLines(const std::wstring& text) {
    std::vector<std::wstring> lines;
    size_t start = 0;
    for (size_t i = 0; i <= text.size(); ++i) {
        if (i == text.size() || text[i] == L'\n' || text[i] == L'\r') {
            lines.push_back(text.substr(start, i - start));
            if (i < text.size() && text[i] == L'\r' && i + 1 < text.size() && text[i + 1] == L'\n') {
                ++i;
            }
            start = i + 1;
        }
    }
    return lines;
}

std::wstring Utf8ToWide(const std::string& bytes) {
    if (bytes.empty()) {
        return L"";
    }
    UINT codepage = CP_UTF8;
    int count = MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS, bytes.data(), (int)bytes.size(), nullptr, 0);
    if (count <= 0) {
        codepage = CP_ACP;
        count = MultiByteToWideChar(codepage, 0, bytes.data(), (int)bytes.size(), nullptr, 0);
    }
    if (count <= 0) {
        return L"";
    }
    std::wstring out((size_t)count, L'\0');
    MultiByteToWideChar(codepage, 0, bytes.data(), (int)bytes.size(), &out[0], count);
    return out;
}

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return std::string();
    }
    int count = WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(), nullptr, 0, nullptr, nullptr);
    if (count <= 0) {
        return std::string();
    }
    std::string out((size_t)count, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(), &out[0], count, nullptr, nullptr);
    return out;
}

std::wstring FileName(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return path;
    }
    return path.substr(pos + 1);
}

std::wstring FolderLabel(const std::wstring& path) {
    std::wstring trimmed = path;
    while (trimmed.size() > 1 && (trimmed.back() == L'\\' || trimmed.back() == L'/')) {
        trimmed.pop_back();
    }
    if (trimmed.empty()) {
        return path;
    }
    std::wstring name = FileName(trimmed);
    if (name.empty()) {
        return trimmed;
    }
    return name;
}

std::wstring FileLabel(const std::wstring& path, bool hideExtensions) {
    std::wstring name = FileName(path);
    if (!hideExtensions) {
        return name;
    }
    size_t dot = name.find_last_of(L'.');
    if (dot != std::wstring::npos && dot > 0) {
        return name.substr(0, dot);
    }
    return name;
}

bool CiLess::operator()(const std::wstring& a, const std::wstring& b) const {
    int result = CompareStringOrdinal(a.c_str(), (int)a.size(), b.c_str(), (int)b.size(), TRUE);
    return result == CSTR_LESS_THAN;
}

} // namespace mfm
