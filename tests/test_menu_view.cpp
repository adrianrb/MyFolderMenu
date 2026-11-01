#include "test_framework.h"

#include <windows.h>

#include "app/app_state.h"
#include "app/menu_model.h"
#include "app/menu_view.h"
#include "core/app_info.h"
#include "test_util.h"

using namespace mfm;

namespace {

struct ViewFixture {
    AppContext ctx;
    HWND window = nullptr;
    HDC dc = nullptr;
    HBITMAP bitmap = nullptr;
    HGDIOBJ oldBitmap = nullptr;

    ViewFixture() {
        ctx.dpi = 96;
        ctx.menuFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        ctx.uiFont = ctx.menuFont;
        ctx.menuDark = true;
        ctx.menuBrush = CreateSolidBrush(RGB(37, 37, 38));
        ctx.menuBgBrush = CreateSolidBrush(RGB(37, 37, 38));

        window = CreateWindowExW(0, L"STATIC", L"menu-view", WS_OVERLAPPED,
                                 0, 0, 200, 100, nullptr, nullptr,
                                 GetModuleHandleW(nullptr), nullptr);

        HDC screen = GetDC(nullptr);
        dc = CreateCompatibleDC(screen);
        bitmap = CreateCompatibleBitmap(screen, 300, 120);
        oldBitmap = SelectObject(dc, bitmap);
        ReleaseDC(nullptr, screen);
    }

    ~ViewFixture() {
        if (dc) {
            SelectObject(dc, oldBitmap);
            DeleteDC(dc);
        }
        if (bitmap) {
            DeleteObject(bitmap);
        }
        if (window) {
            DestroyWindow(window);
        }
    }
};

DRAWITEMSTRUCT MakeDraw(HDC dc, MenuItemData* data, UINT state = 0) {
    DRAWITEMSTRUCT draw = {};
    draw.CtlType = ODT_MENU;
    draw.hDC = dc;
    draw.itemData = (ULONG_PTR)data;
    draw.rcItem = {0, 0, 240, 30};
    draw.itemState = state;
    return draw;
}

} // namespace

TEST(MenuView_Scale) {
    ViewFixture fixture;
    fixture.ctx.dpi = 96;
    CHECK_EQ(Scale(fixture.ctx, 10), 10);
    fixture.ctx.dpi = 120;
    CHECK_EQ(Scale(fixture.ctx, 10), 13); // MulDiv rounds 12.5 up.
    fixture.ctx.dpi = 144;
    CHECK_EQ(Scale(fixture.ctx, 10), 15);
    fixture.ctx.dpi = 0;
    CHECK_EQ(Scale(fixture.ctx, 10), 0);
}

TEST(MenuView_ApplyMenuInfo) {
    ViewFixture fixture;
    HMENU menu = CreatePopupMenu();
    ApplyMenuInfo(fixture.ctx, menu);

    MENUINFO info = {};
    info.cbSize = sizeof(info);
    info.fMask = MIM_BACKGROUND | MIM_STYLE;
    CHECK(GetMenuInfo(menu, &info));
    CHECK(info.hbrBack == fixture.ctx.menuBrush);
    CHECK_EQ(info.dwStyle & MNS_NOCHECK, (DWORD)MNS_NOCHECK);

    ApplyMenuInfo(fixture.ctx, nullptr);
    DestroyMenu(menu);
}

TEST(MenuView_MeasureItems) {
    ViewFixture fixture;
    MenuItemData normal;
    normal.text = L"Um item de menu";

    MEASUREITEMSTRUCT measure = {};
    measure.CtlType = ODT_MENU;
    measure.itemData = (ULONG_PTR)&normal;
    CHECK(HandleMenuMeasure(fixture.ctx, fixture.window, &measure));
    CHECK_GT(measure.itemHeight, 0u);
    CHECK_GT(measure.itemWidth, 0u);

    MenuItemData submenu;
    submenu.text = L"Com submenu";
    submenu.hasSubmenu = true;
    MEASUREITEMSTRUCT withSubmenu = {};
    withSubmenu.CtlType = ODT_MENU;
    withSubmenu.itemData = (ULONG_PTR)&submenu;
    CHECK(HandleMenuMeasure(fixture.ctx, fixture.window, &withSubmenu));
    CHECK_GT(withSubmenu.itemWidth, measure.itemWidth);

    MenuItemData separator;
    separator.separator = true;
    MEASUREITEMSTRUCT separatorMeasure = {};
    separatorMeasure.CtlType = ODT_MENU;
    separatorMeasure.itemData = (ULONG_PTR)&separator;
    CHECK(HandleMenuMeasure(fixture.ctx, fixture.window, &separatorMeasure));
    CHECK_EQ(separatorMeasure.itemHeight, (UINT)Scale(fixture.ctx, 7));
    CHECK_EQ(separatorMeasure.itemWidth, 1u);

    MEASUREITEMSTRUCT notMenu = {};
    notMenu.CtlType = ODT_BUTTON;
    CHECK(!HandleMenuMeasure(fixture.ctx, fixture.window, &notMenu));

    MEASUREITEMSTRUCT noData = {};
    noData.CtlType = ODT_MENU;
    CHECK(!HandleMenuMeasure(fixture.ctx, fixture.window, &noData));
}

TEST(MenuView_DrawItems) {
    ViewFixture fixture;

    MenuItemData normal;
    normal.text = L"Item normal";
    normal.icon = fixture.ctx.icons.FolderIcon();
    DRAWITEMSTRUCT draw = MakeDraw(fixture.dc, &normal);
    CHECK(HandleMenuDraw(fixture.ctx, &draw));

    DRAWITEMSTRUCT selected = MakeDraw(fixture.dc, &normal, ODS_SELECTED);
    CHECK(HandleMenuDraw(fixture.ctx, &selected));

    normal.hasSubmenu = true;
    DRAWITEMSTRUCT submenu = MakeDraw(fixture.dc, &normal);
    CHECK(HandleMenuDraw(fixture.ctx, &submenu));

    normal.disabled = true;
    DRAWITEMSTRUCT disabled = MakeDraw(fixture.dc, &normal);
    CHECK(HandleMenuDraw(fixture.ctx, &disabled));

    MenuItemData separator;
    separator.separator = true;
    DRAWITEMSTRUCT separatorDraw = MakeDraw(fixture.dc, &separator);
    CHECK(HandleMenuDraw(fixture.ctx, &separatorDraw));

    DRAWITEMSTRUCT notMenu = MakeDraw(fixture.dc, &normal);
    notMenu.CtlType = ODT_BUTTON;
    CHECK(!HandleMenuDraw(fixture.ctx, &notMenu));

    DRAWITEMSTRUCT noData = MakeDraw(fixture.dc, nullptr);
    CHECK(!HandleMenuDraw(fixture.ctx, &noData));

    fixture.ctx.menuDark = false;
    DRAWITEMSTRUCT light = MakeDraw(fixture.dc, &normal);
    CHECK(HandleMenuDraw(fixture.ctx, &light));
}

TEST(MenuView_InitPopupPopulatesLazyMenu) {
    ViewFixture fixture;
    test::TempDir temp;
    test::WriteBytes(temp.Join(L"file.txt"), "x");

    HMENU menu = CreatePopupMenu();
    fixture.ctx.lazyMenus[menu] = temp.path();

    CHECK(HandleMenuInitPopup(fixture.ctx, (WPARAM)menu, 0));
    CHECK_GT(GetMenuItemCount(menu), 0);
    CHECK(fixture.ctx.lazyMenus.find(menu) == fixture.ctx.lazyMenus.end());

    CHECK(!HandleMenuInitPopup(fixture.ctx, (WPARAM)menu, 0));
    CHECK(!HandleMenuInitPopup(fixture.ctx, (WPARAM)menu, MAKELPARAM(0, 1)));

    DestroyMenu(menu);
    ResetMenuState(fixture.ctx);
}
