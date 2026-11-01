#include "app/menu_model.h"

#include <algorithm>
#include <memory>
#include <vector>

#include "app/actions.h"
#include "app/menu_view.h"
#include "core/config.h"
#include "core/paths.h"
#include "core/strings.h"
#include "core/theme.h"

namespace mfm {

UINT NewMenuId(AppContext& ctx) {
    return ctx.nextMenuId++;
}

void AddMenuItem(AppContext& ctx, HMENU menu, const std::wstring& text, HICON icon,
                 HMENU submenu, bool disabled, const MenuAction& action) {
    UINT id = NewMenuId(ctx);
    auto data = std::make_unique<MenuItemData>();
    data->text = text;
    data->icon = icon;
    data->hasSubmenu = submenu != nullptr;
    data->disabled = disabled;

    std::wstring escaped = EscapeAmp(text);
    MENUITEMINFOW item = {};
    item.cbSize = sizeof(item);
    item.fMask = MIIM_FTYPE | MIIM_ID | MIIM_DATA | MIIM_STRING | MIIM_STATE;
    item.fType = MFT_OWNERDRAW;
    item.fState = disabled ? MFS_DISABLED : MFS_ENABLED;
    item.wID = id;
    item.dwTypeData = escaped.data();
    item.cch = (UINT)escaped.size();
    item.dwItemData = (ULONG_PTR)data.get();
    if (submenu) {
        item.fMask |= MIIM_SUBMENU;
        item.hSubMenu = submenu;
    }
    InsertMenuItemW(menu, GetMenuItemCount(menu), TRUE, &item);
    ctx.menuItems.push_back(std::move(data));
    if (action.type != ActionType::None) {
        ctx.menuActions[id] = action;
    }
}

void AddSeparator(AppContext& ctx, HMENU menu) {
    auto data = std::make_unique<MenuItemData>();
    data->separator = true;
    data->disabled = true;

    MENUITEMINFOW item = {};
    item.cbSize = sizeof(item);
    item.fMask = MIIM_FTYPE | MIIM_ID | MIIM_DATA | MIIM_STRING | MIIM_STATE;
    item.fType = MFT_OWNERDRAW;
    item.fState = MFS_DISABLED;
    item.wID = NewMenuId(ctx);
    item.dwTypeData = const_cast<wchar_t*>(L"");
    item.cch = 0;
    item.dwItemData = (ULONG_PTR)data.get();
    InsertMenuItemW(menu, GetMenuItemCount(menu), TRUE, &item);
    ctx.menuItems.push_back(std::move(data));
}

void AddFolderItem(AppContext& ctx, HMENU menu, const std::wstring& path) {
    std::wstring label = FolderLabel(path);
    HICON icon = ctx.icons.FolderIcon();
    if (!HasVisibleEntries(path)) {
        MenuAction action;
        action.type = ActionType::OpenPath;
        action.path = path;
        AddMenuItem(ctx, menu, label, icon, nullptr, false, action);
        return;
    }

    HMENU submenu = CreatePopupMenu();
    ApplyMenuInfo(ctx, submenu);
    MenuAction open;
    open.type = ActionType::OpenPath;
    open.path = path;
    AddMenuItem(ctx, submenu, L"Abrir esta pasta", icon, nullptr, false, open);
    AddSeparator(ctx, submenu);
    AddMenuItem(ctx, submenu, L"Carregando...", nullptr, nullptr, true, MenuAction());
    ctx.lazyMenus[submenu] = path;
    AddMenuItem(ctx, menu, label, icon, submenu, false, MenuAction());
}

void AddFileItem(AppContext& ctx, HMENU menu, const std::wstring& path) {
    MenuAction action;
    action.type = ActionType::OpenPath;
    action.path = path;
    AddMenuItem(ctx, menu, FileLabel(path, ctx.hideExtensions),
                ctx.icons.FileIcon(path), nullptr, false, action);
}

void PopulateSubmenu(AppContext& ctx, HMENU menu, const std::wstring& path) {
    while (GetMenuItemCount(menu) > 0) {
        DeleteMenu(menu, 0, MF_BYPOSITION);
    }

    MenuAction open;
    open.type = ActionType::OpenPath;
    open.path = path;
    AddMenuItem(ctx, menu, L"Abrir esta pasta", ctx.icons.FolderIcon(), nullptr, false, open);
    AddSeparator(ctx, menu);

    std::vector<DirEntry> entries;
    if (!ListEntries(path, entries)) {
        AddMenuItem(ctx, menu, L"(pasta indisponível)", nullptr, nullptr, true, MenuAction());
        return;
    }

    std::vector<DirEntry> directories;
    std::vector<DirEntry> files;
    for (DirEntry& entry : entries) {
        if (entry.isDir) {
            directories.push_back(std::move(entry));
        } else {
            files.push_back(std::move(entry));
        }
    }

    auto compare = [](const DirEntry& a, const DirEntry& b) {
        int result = CompareStringEx(LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE,
                                     a.name.c_str(), -1, b.name.c_str(), -1,
                                     nullptr, nullptr, 0);
        return result == CSTR_LESS_THAN;
    };
    std::sort(directories.begin(), directories.end(), compare);
    std::sort(files.begin(), files.end(), compare);

    if (directories.empty() && files.empty()) {
        AddMenuItem(ctx, menu, L"(vazio)", nullptr, nullptr, true, MenuAction());
        return;
    }

    for (const DirEntry& entry : directories) {
        AddFolderItem(ctx, menu, entry.path);
    }
    if (!directories.empty() && !files.empty()) {
        AddSeparator(ctx, menu);
    }
    for (const DirEntry& entry : files) {
        AddFileItem(ctx, menu, entry.path);
    }
}

HMENU BuildMenu(AppContext& ctx) {
    AppSettings settings = LoadSettings();
    ctx.hideExtensions = settings.hideFileExtensions;
    ctx.menuDark = IsDarkTheme(settings.theme);
    SetAppThemeMode(ctx.menuDark);

    ResetMenuState(ctx);

    COLORREF background = ctx.menuDark ? RGB(37, 37, 38) : GetSysColor(COLOR_MENU);
    ctx.menuBrush = CreateSolidBrush(background);
    ctx.menuBgBrush = CreateSolidBrush(background);

    HMENU root = CreatePopupMenu();
    ApplyMenuInfo(ctx, root);

    std::vector<std::wstring> folders = LoadFolders();
    int available = 0;
    for (const std::wstring& folder : folders) {
        if (DirectoryExists(folder)) {
            AddFolderItem(ctx, root, folder);
            ++available;
        }
    }
    if (available == 0) {
        AddMenuItem(ctx, root, L"Nenhuma pasta configurada", nullptr, nullptr, true, MenuAction());
    }
    AddSeparator(ctx, root);

    HMENU manage = CreatePopupMenu();
    ApplyMenuInfo(ctx, manage);
    MenuAction add;
    add.type = ActionType::AddFolder;
    AddMenuItem(ctx, manage, L"Adicionar pasta...", nullptr, nullptr, false, add);

    HMENU remove = CreatePopupMenu();
    ApplyMenuInfo(ctx, remove);
    if (folders.empty()) {
        AddMenuItem(ctx, manage, L"Remover pasta", nullptr, nullptr, true, MenuAction());
    } else {
        for (const std::wstring& folder : folders) {
            MenuAction action;
            action.type = ActionType::RemoveFolder;
            action.path = folder;
            AddMenuItem(ctx, remove, FolderLabel(folder) + L"  —  " + folder,
                        nullptr, nullptr, false, action);
        }
        AddMenuItem(ctx, manage, L"Remover pasta", nullptr, remove, false, MenuAction());
    }
    AddMenuItem(ctx, root, L"Pastas", nullptr, manage, false, MenuAction());

    MenuAction settingsAction;
    settingsAction.type = ActionType::Settings;
    AddMenuItem(ctx, root, L"Configurações...", nullptr, nullptr, false, settingsAction);

    MenuAction aboutAction;
    aboutAction.type = ActionType::About;
    AddMenuItem(ctx, root, L"Sobre", nullptr, nullptr, false, aboutAction);

    AddSeparator(ctx, root);
    MenuAction exitAction;
    exitAction.type = ActionType::ExitApp;
    AddMenuItem(ctx, root, L"Sair", nullptr, nullptr, false, exitAction);

    return root;
}

void ShowTrayMenu(AppContext& ctx, HWND owner, POINT point) {
    HMENU root = BuildMenu(ctx);
    SetForegroundWindow(owner);
    UINT command = TrackPopupMenuEx(root,
                                    TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_TOPALIGN,
                                    point.x, point.y, owner, nullptr);
    PostMessageW(owner, WM_NULL, 0, 0);

    MenuAction action;
    if (command != 0) {
        auto it = ctx.menuActions.find(command);
        if (it != ctx.menuActions.end()) {
            action = it->second;
        }
    }

    ctx.lazyMenus.clear();
    DestroyMenu(root);
    ResetMenuState(ctx);
    ctx.icons.ClearFiles();

    if (action.type != ActionType::None) {
        ExecuteAction(ctx, owner, action);
    }
}

void ResetMenuState(AppContext& ctx) {
    ctx.menuItems.clear();
    ctx.menuActions.clear();
    ctx.lazyMenus.clear();
    ctx.nextMenuId = kMenuIdBase;

    if (ctx.menuBrush) {
        DeleteObject(ctx.menuBrush);
        ctx.menuBrush = nullptr;
    }
    if (ctx.menuBgBrush) {
        DeleteObject(ctx.menuBgBrush);
        ctx.menuBgBrush = nullptr;
    }
}

} // namespace mfm
