#pragma once

#include <windows.h>

#include <string>

namespace mfm {

enum class ActionType {
    None,
    OpenPath,
    AddFolder,
    RemoveFolder,
    Settings,
    About,
    ExitApp,
};

struct MenuAction {
    ActionType type = ActionType::None;
    std::wstring path;
};

struct MenuItemData {
    std::wstring text;
    HICON icon = nullptr;
    bool hasSubmenu = false;
    bool separator = false;
    bool disabled = false;
};

} // namespace mfm
