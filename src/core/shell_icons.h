#pragma once

#include <windows.h>

#include <map>
#include <string>

#include "core/strings.h"

namespace mfm {

// Owns and caches shell icons. Every icon returned by this class is destroyed
// by Clear()/the destructor, so callers must not destroy them.
class ShellIconCache {
public:
    ShellIconCache() = default;
    ~ShellIconCache();

    ShellIconCache(const ShellIconCache&) = delete;
    ShellIconCache& operator=(const ShellIconCache&) = delete;

    HICON FolderIcon();
    HICON FileIcon(const std::wstring& path);

    void ClearFiles();
    void Clear();

private:
    HICON folder_ = nullptr;
    std::map<std::wstring, HICON, CiLess> files_;
};

} // namespace mfm
