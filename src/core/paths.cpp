#include "core/paths.h"

#include <windows.h>
#include <shlobj.h>

#include "core/strings.h"

namespace mfm {

std::wstring ExePath() {
    std::vector<wchar_t> buffer(MAX_PATH);
    for (;;) {
        DWORD length = GetModuleFileNameW(nullptr, buffer.data(), (DWORD)buffer.size());
        if (length == 0) {
            return L"";
        }
        if (length < buffer.size() - 1) {
            return std::wstring(buffer.data(), length);
        }
        buffer.resize(buffer.size() * 2);
    }
}

std::wstring GetSpecialFolder(int csidl) {
    wchar_t buffer[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, csidl, nullptr, SHGFP_TYPE_CURRENT, buffer))) {
        return buffer;
    }
    return L"";
}

std::wstring ExpandEnv(const std::wstring& text) {
    DWORD required = ExpandEnvironmentStringsW(text.c_str(), nullptr, 0);
    if (required == 0) {
        return text;
    }
    std::vector<wchar_t> buffer(required);
    DWORD length = ExpandEnvironmentStringsW(text.c_str(), buffer.data(), required);
    if (length == 0 || length > required) {
        return text;
    }
    return std::wstring(buffer.data(), length - 1);
}

std::wstring FullPath(const std::wstring& text) {
    DWORD size = 512;
    std::vector<wchar_t> buffer;
    for (;;) {
        buffer.resize(size);
        DWORD length = GetFullPathNameW(text.c_str(), size, buffer.data(), nullptr);
        if (length == 0) {
            return text;
        }
        if (length < size) {
            return std::wstring(buffer.data(), length);
        }
        size = length + 1;
    }
}

std::vector<std::wstring> NormalizeFolders(const std::vector<std::wstring>& input) {
    std::vector<std::wstring> result;
    for (const std::wstring& raw : input) {
        std::wstring value = Trim(raw);
        if (value.empty()) {
            continue;
        }
        std::wstring full = FullPath(ExpandEnv(value));
        if (full.empty()) {
            full = value;
        }
        bool duplicate = false;
        for (const std::wstring& existing : result) {
            if (EqualsCI(existing, full)) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            result.push_back(full);
        }
    }
    return result;
}

std::wstring JoinPath(const std::wstring& dir, const std::wstring& name) {
    std::wstring path = dir;
    if (!path.empty() && path.back() != L'\\' && path.back() != L'/') {
        path += L'\\';
    }
    path += name;
    return path;
}

bool DirectoryExists(const std::wstring& path) {
    DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool EnsureDirectory(const std::wstring& path) {
    int result = SHCreateDirectoryExW(nullptr, path.c_str(), nullptr);
    return result == ERROR_SUCCESS || result == ERROR_ALREADY_EXISTS || result == ERROR_FILE_EXISTS;
}

bool HasVisibleEntries(const std::wstring& path) {
    WIN32_FIND_DATAW data = {};
    HANDLE find = FindFirstFileW(JoinPath(path, L"*").c_str(), &data);
    if (find == INVALID_HANDLE_VALUE) {
        return false;
    }
    bool found = false;
    do {
        if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0) {
            continue;
        }
        if ((data.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) == 0) {
            found = true;
            break;
        }
    } while (FindNextFileW(find, &data));
    FindClose(find);
    return found;
}

bool ListEntries(const std::wstring& path, std::vector<DirEntry>& entries) {
    WIN32_FIND_DATAW data = {};
    HANDLE find = FindFirstFileW(JoinPath(path, L"*").c_str(), &data);
    if (find == INVALID_HANDLE_VALUE) {
        return false;
    }
    do {
        if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0) {
            continue;
        }
        if ((data.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) != 0) {
            continue;
        }
        DirEntry entry;
        entry.name = data.cFileName;
        entry.path = JoinPath(path, data.cFileName);
        entry.isDir = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        entries.push_back(std::move(entry));
    } while (FindNextFileW(find, &data));
    FindClose(find);
    return true;
}

} // namespace mfm
