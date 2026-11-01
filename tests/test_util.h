#pragma once

#include <windows.h>

#include <string>

namespace test {

std::wstring MakeTempDir(const std::wstring& prefix);
void RemoveTree(const std::wstring& path);

bool WriteBytes(const std::wstring& path, const std::string& bytes);
bool WriteTextUtf8(const std::wstring& path, const std::wstring& text);
std::string ReadBytes(const std::wstring& path);
std::wstring ReadTextUtf8(const std::wstring& path);
bool CreateHiddenFile(const std::wstring& path);
bool CreateFileWithBytes(const std::wstring& path, const std::string& bytes);

class TempDir {
public:
    explicit TempDir(const std::wstring& prefix = L"mfm_test_");
    ~TempDir();

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    const std::wstring& path() const { return path_; }
    std::wstring Join(const std::wstring& name) const;
    bool CreateSubdir(const std::wstring& name) const;

private:
    std::wstring path_;
};

class ScopedEnv {
public:
    ScopedEnv(const wchar_t* name, const std::wstring& value);
    ~ScopedEnv();

    ScopedEnv(const ScopedEnv&) = delete;
    ScopedEnv& operator=(const ScopedEnv&) = delete;

private:
    std::wstring name_;
    std::wstring oldValue_;
    bool hadValue_ = false;
};

} // namespace test
