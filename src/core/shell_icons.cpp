#include "core/shell_icons.h"

#include <shellapi.h>

#include "core/strings.h"

namespace mfm {

namespace {

HICON LoadShellIcon(const std::wstring& path, bool useAttributes, DWORD attributes) {
    SHFILEINFOW info = {};
    UINT flags = SHGFI_ICON | SHGFI_SMALLICON;
    if (useAttributes) {
        flags |= SHGFI_USEFILEATTRIBUTES;
    }
    if (SHGetFileInfoW(path.c_str(), attributes, &info, sizeof(info), flags) && info.hIcon) {
        return info.hIcon;
    }
    return nullptr;
}

HICON DefaultApplicationIcon() {
    return CopyIcon(LoadIconW(nullptr, IDI_APPLICATION));
}

} // namespace

ShellIconCache::~ShellIconCache() {
    Clear();
}

HICON ShellIconCache::FolderIcon() {
    if (!folder_) {
        folder_ = LoadShellIcon(L"folder", true, FILE_ATTRIBUTE_DIRECTORY);
        if (!folder_) {
            folder_ = DefaultApplicationIcon();
        }
    }
    return folder_;
}

HICON ShellIconCache::FileIcon(const std::wstring& path) {
    auto it = files_.find(path);
    if (it != files_.end()) {
        return it->second;
    }
    HICON icon = LoadShellIcon(path, false, 0);
    if (!icon) {
        std::wstring name = FileName(path);
        size_t dot = name.find_last_of(L'.');
        std::wstring extension = dot == std::wstring::npos ? L"" : name.substr(dot);
        icon = LoadShellIcon(L"file" + extension, true, FILE_ATTRIBUTE_NORMAL);
    }
    if (!icon) {
        icon = DefaultApplicationIcon();
    }
    files_[path] = icon;
    return icon;
}

void ShellIconCache::ClearFiles() {
    for (auto& item : files_) {
        if (item.second) {
            DestroyIcon(item.second);
        }
    }
    files_.clear();
}

void ShellIconCache::Clear() {
    ClearFiles();
    if (folder_) {
        DestroyIcon(folder_);
        folder_ = nullptr;
    }
}

} // namespace mfm
