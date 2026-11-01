#include "test_util.h"

#include <cstdio>

namespace test {

namespace {

std::wstring JoinPath(const std::wstring& dir, const std::wstring& name) {
    std::wstring path = dir;
    if (!path.empty() && path.back() != L'\\' && path.back() != L'/') {
        path += L'\\';
    }
    path += name;
    return path;
}

bool WriteAllBytes(const std::wstring& path, const std::string& bytes) {
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

std::string ReadAllBytes(const std::wstring& path) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return std::string();
    }
    LARGE_INTEGER size = {};
    if (!GetFileSizeEx(file, &size) || size.QuadPart < 0) {
        CloseHandle(file);
        return std::string();
    }
    std::string bytes((size_t)size.QuadPart, '\0');
    DWORD total = 0;
    while (total < bytes.size()) {
        DWORD read = 0;
        if (!ReadFile(file, &bytes[total], (DWORD)(bytes.size() - total), &read, nullptr) ||
            read == 0) {
            break;
        }
        total += read;
    }
    CloseHandle(file);
    bytes.resize(total);
    return bytes;
}

} // namespace

std::wstring MakeTempDir(const std::wstring& prefix) {
    wchar_t temp[MAX_PATH] = {};
    DWORD length = GetTempPathW(MAX_PATH, temp);
    std::wstring base = length > 0 ? std::wstring(temp, length) : L".";
    if (!base.empty() && base.back() != L'\\' && base.back() != L'/') {
        base += L'\\';
    }
    static LONG counter = 0;
    for (int attempt = 0; attempt < 100; ++attempt) {
        std::wstring candidate = base + prefix + std::to_wstring(GetCurrentProcessId()) +
                                 L"_" + std::to_wstring(++counter);
        if (CreateDirectoryW(candidate.c_str(), nullptr)) {
            return candidate;
        }
    }
    return base + prefix + L"fallback";
}

void RemoveTree(const std::wstring& path) {
    DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        return;
    }
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL);
        DeleteFileW(path.c_str());
        return;
    }

    WIN32_FIND_DATAW data = {};
    HANDLE find = FindFirstFileW(JoinPath(path, L"*").c_str(), &data);
    if (find != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0) {
                continue;
            }
            RemoveTree(JoinPath(path, data.cFileName));
        } while (FindNextFileW(find, &data));
        FindClose(find);
    }
    SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL);
    RemoveDirectoryW(path.c_str());
}

bool WriteBytes(const std::wstring& path, const std::string& bytes) {
    return WriteAllBytes(path, bytes);
}

bool WriteTextUtf8(const std::wstring& path, const std::wstring& text) {
    if (text.empty()) {
        return WriteAllBytes(path, std::string());
    }
    int count = WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(),
                                    nullptr, 0, nullptr, nullptr);
    if (count <= 0) {
        return false;
    }
    std::string bytes((size_t)count, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(),
                        &bytes[0], count, nullptr, nullptr);
    return WriteAllBytes(path, bytes);
}

std::string ReadBytes(const std::wstring& path) {
    return ReadAllBytes(path);
}

std::wstring ReadTextUtf8(const std::wstring& path) {
    std::string bytes = ReadAllBytes(path);
    if (bytes.empty()) {
        return std::wstring();
    }
    int count = MultiByteToWideChar(CP_UTF8, 0, bytes.data(), (int)bytes.size(), nullptr, 0);
    if (count <= 0) {
        return std::wstring();
    }
    std::wstring text((size_t)count, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, bytes.data(), (int)bytes.size(), &text[0], count);
    return text;
}

bool CreateHiddenFile(const std::wstring& path) {
    if (!WriteAllBytes(path, "hidden")) {
        return false;
    }
    return SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_HIDDEN) != FALSE;
}

bool CreateFileWithBytes(const std::wstring& path, const std::string& bytes) {
    return WriteAllBytes(path, bytes);
}

TempDir::TempDir(const std::wstring& prefix)
    : path_(MakeTempDir(prefix)) {
}

TempDir::~TempDir() {
    RemoveTree(path_);
}

std::wstring TempDir::Join(const std::wstring& name) const {
    return JoinPath(path_, name);
}

bool TempDir::CreateSubdir(const std::wstring& name) const {
    return CreateDirectoryW(Join(name).c_str(), nullptr) != FALSE;
}

ScopedEnv::ScopedEnv(const wchar_t* name, const std::wstring& value)
    : name_(name ? name : L"") {
    DWORD length = GetEnvironmentVariableW(name_.c_str(), nullptr, 0);
    if (length > 0) {
        hadValue_ = true;
        oldValue_.resize(length);
        GetEnvironmentVariableW(name_.c_str(), &oldValue_[0], length);
    }
    SetEnvironmentVariableW(name_.c_str(), value.empty() ? nullptr : value.c_str());
}

ScopedEnv::~ScopedEnv() {
    SetEnvironmentVariableW(name_.c_str(), hadValue_ ? oldValue_.c_str() : nullptr);
}

} // namespace test
