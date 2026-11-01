#include "test_framework.h"

#include <windows.h>

#include <string>
#include <vector>

#include "app/actions.h"
#include "app/app_state.h"
#include "app/menu_model.h"
#include "app/menu_view.h"
#include "core/config.h"
#include "core/paths.h"
#include "core/strings.h"
#include "test_util.h"

using namespace mfm;

namespace {

MenuItemData* ItemData(HMENU menu, int position) {
    MENUITEMINFOW info = {};
    info.cbSize = sizeof(info);
    info.fMask = MIIM_DATA;
    if (!GetMenuItemInfoW(menu, (UINT)position, TRUE, &info)) {
        return nullptr;
    }
    return (MenuItemData*)info.dwItemData;
}

std::wstring ItemText(HMENU menu, int position) {
    MenuItemData* data = ItemData(menu, position);
    return data ? data->text : std::wstring();
}

int FindPosition(HMENU menu, const std::wstring& text) {
    int count = GetMenuItemCount(menu);
    for (int i = 0; i < count; ++i) {
        if (ItemText(menu, i) == text) {
            return i;
        }
    }
    return -1;
}

bool HasText(HMENU menu, const std::wstring& text) {
    return FindPosition(menu, text) >= 0;
}

bool HasActionType(const AppContext& ctx, ActionType type) {
    for (const auto& entry : ctx.menuActions) {
        if (entry.second.type == type) {
            return true;
        }
    }
    return false;
}

struct MenuFixture {
    test::TempDir temp;
    test::ScopedEnv env;
    AppContext ctx;
    std::wstring dirA;
    std::wstring emptyDir;
    std::wstring missing;

    MenuFixture()
        : env(L"MFM_APPDATA", temp.path()) {
        dirA = temp.Join(L"dirA");
        emptyDir = temp.Join(L"empty");
        missing = temp.Join(L"missing");
        CreateDirectoryW(dirA.c_str(), nullptr);
        CreateDirectoryW(emptyDir.c_str(), nullptr);
        CreateDirectoryW((dirA + L"\\sub").c_str(), nullptr);
        test::WriteBytes(dirA + L"\\b.txt", "b");
        test::WriteBytes(dirA + L"\\a.txt", "a");
        test::CreateHiddenFile(dirA + L"\\hidden.txt");
    }

    ~MenuFixture() {
        ResetMenuState(ctx);
    }

    void Save(const std::vector<std::wstring>& folders, const AppSettings& settings) {
        SaveConfig(folders, settings);
    }
};

} // namespace

TEST(Menu_BuildMenuContainsFoldersAndCommands) {
    MenuFixture fixture;
    AppSettings settings;
    settings.hideFileExtensions = false;
    fixture.Save({fixture.dirA, fixture.emptyDir, fixture.missing}, settings);

    HMENU root = BuildMenu(fixture.ctx);
    CHECK(root != nullptr);
    CHECK_GT(GetMenuItemCount(root), 0);
    CHECK(!fixture.ctx.hideExtensions);
    CHECK_EQ(fixture.ctx.menuBrush != nullptr, true);
    CHECK_EQ(fixture.ctx.menuBgBrush != nullptr, true);

    CHECK(HasText(root, L"dirA"));
    CHECK(HasText(root, L"empty"));
    CHECK(!HasText(root, L"missing"));
    CHECK(HasText(root, L"Pastas"));
    CHECK(HasText(root, L"Configurações..."));
    CHECK(HasText(root, L"Sobre"));
    CHECK(HasText(root, L"Sair"));
    CHECK(!HasText(root, L"Nenhuma pasta configurada"));

    int last = GetMenuItemCount(root) - 1;
    CHECK_EQ(ItemText(root, last), L"Sair");

    CHECK(HasActionType(fixture.ctx, ActionType::AddFolder));
    CHECK(HasActionType(fixture.ctx, ActionType::RemoveFolder));
    CHECK(HasActionType(fixture.ctx, ActionType::Settings));
    CHECK(HasActionType(fixture.ctx, ActionType::About));
    CHECK(HasActionType(fixture.ctx, ActionType::ExitApp));

    DestroyMenu(root);
    ResetMenuState(fixture.ctx);
}

TEST(Menu_BuildMenuWithoutAvailableFolders) {
    MenuFixture fixture;
    fixture.Save({fixture.missing}, AppSettings());

    HMENU root = BuildMenu(fixture.ctx);
    CHECK(HasText(root, L"Nenhuma pasta configurada"));
    CHECK(HasText(root, L"Pastas"));

    int pastas = FindPosition(root, L"Pastas");
    CHECK(pastas >= 0);
    HMENU manage = GetSubMenu(root, pastas);
    CHECK(manage != nullptr);
    CHECK(HasText(manage, L"Adicionar pasta..."));
    CHECK(HasText(manage, L"Remover pasta"));

    DestroyMenu(root);
    ResetMenuState(fixture.ctx);
}

TEST(Menu_BuildMenuWithEmptyFolderList) {
    MenuFixture fixture;
    fixture.Save({}, AppSettings());

    HMENU root = BuildMenu(fixture.ctx);
    CHECK(HasText(root, L"Nenhuma pasta configurada"));

    int pastas = FindPosition(root, L"Pastas");
    HMENU manage = GetSubMenu(root, pastas);
    CHECK(manage != nullptr);
    CHECK(HasText(manage, L"Remover pasta"));
    int remove = FindPosition(manage, L"Remover pasta");
    CHECK(GetSubMenu(manage, remove) == nullptr);

    DestroyMenu(root);
    ResetMenuState(fixture.ctx);
}

TEST(Menu_AddFolderItemForEmptyFolder) {
    MenuFixture fixture;
    HMENU menu = CreatePopupMenu();
    AddFolderItem(fixture.ctx, menu, fixture.emptyDir);

    CHECK_EQ(GetMenuItemCount(menu), 1);
    CHECK_EQ(fixture.ctx.menuItems.size(), (size_t)1);
    CHECK_EQ(ItemText(menu, 0), L"empty");
    CHECK(GetSubMenu(menu, 0) == nullptr);

    MenuItemData* data = ItemData(menu, 0);
    CHECK(data != nullptr);
    CHECK(!data->hasSubmenu);

    MENUITEMINFOW info = {};
    info.cbSize = sizeof(info);
    info.fMask = MIIM_ID;
    CHECK(GetMenuItemInfoW(menu, 0, TRUE, &info));
    auto action = fixture.ctx.menuActions.find(info.wID);
    CHECK(action != fixture.ctx.menuActions.end());
    if (action != fixture.ctx.menuActions.end()) {
        CHECK(action->second.type == ActionType::OpenPath);
        CHECK(EqualsCI(action->second.path, fixture.emptyDir));
    }

    DestroyMenu(menu);
    ResetMenuState(fixture.ctx);
}

TEST(Menu_AddFolderItemCreatesLazySubmenu) {
    MenuFixture fixture;
    HMENU menu = CreatePopupMenu();
    AddFolderItem(fixture.ctx, menu, fixture.dirA);

    CHECK_EQ(GetMenuItemCount(menu), 1);
    HMENU submenu = GetSubMenu(menu, 0);
    CHECK(submenu != nullptr);
    CHECK_EQ(GetMenuItemCount(submenu), 3);
    CHECK_EQ(ItemText(submenu, 0), L"Abrir esta pasta");
    CHECK_EQ(ItemText(submenu, 2), L"Carregando...");

    auto lazy = fixture.ctx.lazyMenus.find(submenu);
    CHECK(lazy != fixture.ctx.lazyMenus.end());
    if (lazy != fixture.ctx.lazyMenus.end()) {
        CHECK(EqualsCI(lazy->second, fixture.dirA));
    }

    DestroyMenu(menu);
    ResetMenuState(fixture.ctx);
}

TEST(Menu_PopulateSubmenuOrdersFoldersThenFiles) {
    MenuFixture fixture;
    fixture.ctx.hideExtensions = false;
    HMENU menu = CreatePopupMenu();
    AddFolderItem(fixture.ctx, menu, fixture.dirA);
    HMENU submenu = GetSubMenu(menu, 0);
    CHECK(submenu != nullptr);

    size_t before = fixture.ctx.menuItems.size();
    PopulateSubmenu(fixture.ctx, submenu, fixture.dirA);

    CHECK_EQ(GetMenuItemCount(submenu), 6);
    CHECK_EQ(ItemText(submenu, 0), L"Abrir esta pasta");
    CHECK_EQ(ItemText(submenu, 2), L"sub");
    CHECK_EQ(ItemText(submenu, 4), L"a.txt");
    CHECK_EQ(ItemText(submenu, 5), L"b.txt");

    bool sawHidden = false;
    for (size_t i = before; i < fixture.ctx.menuItems.size(); ++i) {
        if (fixture.ctx.menuItems[i]->text == L"hidden.txt") {
            sawHidden = true;
        }
    }
    CHECK(!sawHidden);

    MenuItemData* separator = ItemData(submenu, 1);
    CHECK(separator != nullptr);
    CHECK(separator->separator);

    DestroyMenu(menu);
    ResetMenuState(fixture.ctx);
}

TEST(Menu_PopulateSubmenuHidesExtensionsWhenConfigured) {
    MenuFixture fixture;
    fixture.ctx.hideExtensions = true;
    HMENU menu = CreatePopupMenu();
    PopulateSubmenu(fixture.ctx, menu, fixture.dirA);

    CHECK(HasText(menu, L"a"));
    CHECK(HasText(menu, L"b"));
    CHECK(!HasText(menu, L"a.txt"));
    CHECK(!HasText(menu, L"b.txt"));

    DestroyMenu(menu);
    ResetMenuState(fixture.ctx);
}

TEST(Menu_PopulateSubmenuMissingAndEmpty) {
    MenuFixture fixture;
    HMENU missingMenu = CreatePopupMenu();
    PopulateSubmenu(fixture.ctx, missingMenu, fixture.missing);
    CHECK_EQ(GetMenuItemCount(missingMenu), 3);
    CHECK_EQ(ItemText(missingMenu, 2), L"(pasta indisponível)");

    HMENU emptyMenu = CreatePopupMenu();
    PopulateSubmenu(fixture.ctx, emptyMenu, fixture.emptyDir);
    CHECK_EQ(GetMenuItemCount(emptyMenu), 3);
    CHECK_EQ(ItemText(emptyMenu, 2), L"(vazio)");

    DestroyMenu(missingMenu);
    DestroyMenu(emptyMenu);
    ResetMenuState(fixture.ctx);
}

TEST(Menu_HandleInitPopupPopulatesOnce) {
    MenuFixture fixture;
    HMENU menu = CreatePopupMenu();
    AddFolderItem(fixture.ctx, menu, fixture.dirA);
    HMENU submenu = GetSubMenu(menu, 0);
    CHECK(submenu != nullptr);

    CHECK(HandleMenuInitPopup(fixture.ctx, (WPARAM)submenu, 0));
    CHECK_EQ(GetMenuItemCount(submenu), 6);
    CHECK(fixture.ctx.lazyMenus.find(submenu) == fixture.ctx.lazyMenus.end());

    CHECK(!HandleMenuInitPopup(fixture.ctx, (WPARAM)submenu, 0));
    CHECK(!HandleMenuInitPopup(fixture.ctx, (WPARAM)submenu, MAKELPARAM(0, 1)));

    DestroyMenu(menu);
    ResetMenuState(fixture.ctx);
}

TEST(Menu_AddMenuItemRegistersActions) {
    MenuFixture fixture;
    HMENU menu = CreatePopupMenu();
    MenuAction action;
    action.type = ActionType::OpenPath;
    action.path = L"C:\\Somewhere";
    AddMenuItem(fixture.ctx, menu, L"Item & Teste", nullptr, nullptr, false, action);
    AddMenuItem(fixture.ctx, menu, L"Sem ação", nullptr, nullptr, false, MenuAction());
    AddSeparator(fixture.ctx, menu);

    CHECK_EQ(GetMenuItemCount(menu), 3);
    CHECK_EQ(ItemText(menu, 0), L"Item & Teste");
    CHECK_EQ(ItemText(menu, 2), L"");
    CHECK(ItemData(menu, 2)->separator);

    MENUITEMINFOW info = {};
    info.cbSize = sizeof(info);
    info.fMask = MIIM_ID;
    CHECK(GetMenuItemInfoW(menu, 0, TRUE, &info));
    auto found = fixture.ctx.menuActions.find(info.wID);
    CHECK(found != fixture.ctx.menuActions.end());
    if (found != fixture.ctx.menuActions.end()) {
        CHECK(found->second.type == ActionType::OpenPath);
        CHECK_EQ(found->second.path, L"C:\\Somewhere");
    }

    MENUITEMINFOW second = {};
    second.cbSize = sizeof(second);
    second.fMask = MIIM_ID;
    CHECK(GetMenuItemInfoW(menu, 1, TRUE, &second));
    CHECK(fixture.ctx.menuActions.find(second.wID) == fixture.ctx.menuActions.end());

    DestroyMenu(menu);
    ResetMenuState(fixture.ctx);
}

TEST(Menu_ExecuteActionRemoveFolder) {
    MenuFixture fixture;
    fixture.Save({fixture.dirA, fixture.emptyDir}, AppSettings());

    MenuAction remove;
    remove.type = ActionType::RemoveFolder;
    remove.path = fixture.dirA;
    ExecuteAction(fixture.ctx, nullptr, remove);

    std::vector<std::wstring> folders = LoadFolders();
    CHECK_EQ(folders.size(), (size_t)1);
    CHECK(EqualsCI(folders[0], fixture.emptyDir));

    MenuAction removeMissing;
    removeMissing.type = ActionType::RemoveFolder;
    removeMissing.path = fixture.missing;
    ExecuteAction(fixture.ctx, nullptr, removeMissing);

    folders = LoadFolders();
    CHECK_EQ(folders.size(), (size_t)1);
}

TEST(Menu_ResetMenuStateClearsBookkeeping) {
    MenuFixture fixture;
    HMENU menu = CreatePopupMenu();
    AddMenuItem(fixture.ctx, menu, L"X", nullptr, nullptr, false, MenuAction());
    fixture.ctx.menuDark = true;
    fixture.ctx.menuBrush = CreateSolidBrush(RGB(1, 2, 3));
    fixture.ctx.menuBgBrush = CreateSolidBrush(RGB(4, 5, 6));
    CHECK_EQ(fixture.ctx.menuItems.size(), (size_t)1);

    DestroyMenu(menu);
    ResetMenuState(fixture.ctx);

    CHECK(fixture.ctx.menuItems.empty());
    CHECK(fixture.ctx.menuActions.empty());
    CHECK(fixture.ctx.lazyMenus.empty());
    CHECK_EQ(fixture.ctx.nextMenuId, kMenuIdBase);
    CHECK(fixture.ctx.menuBrush == nullptr);
    CHECK(fixture.ctx.menuBgBrush == nullptr);
}
