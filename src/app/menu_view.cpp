#include "app/menu_view.h"

#include <algorithm>

#include "app/menu_model.h"

namespace mfm {

void ApplyMenuInfo(AppContext& ctx, HMENU menu) {
    if (!menu || !ctx.menuBrush) {
        return;
    }
    MENUINFO info = {};
    info.cbSize = sizeof(info);
    info.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS | MIM_STYLE;
    info.dwStyle = MNS_NOCHECK;
    info.hbrBack = ctx.menuBrush;
    SetMenuInfo(menu, &info);
}

bool HandleMenuMeasure(AppContext& ctx, HWND hwnd, MEASUREITEMSTRUCT* measure) {
    if (!measure || measure->CtlType != ODT_MENU) {
        return false;
    }
    MenuItemData* data = (MenuItemData*)measure->itemData;
    if (!data) {
        return false;
    }
    if (data->separator) {
        measure->itemHeight = (UINT)Scale(ctx, 7);
        measure->itemWidth = 1;
        return true;
    }

    int textWidth = 0;
    int textHeight = 0;
    HDC dc = GetDC(hwnd);
    if (dc) {
        HGDIOBJ oldFont = SelectObject(dc, ctx.menuFont);
        SIZE size = {};
        if (!data->text.empty()) {
            GetTextExtentPoint32W(dc, data->text.c_str(), (int)data->text.size(), &size);
        }
        textWidth = (int)size.cx;
        textHeight = (int)size.cy;
        SelectObject(dc, oldFont);
        ReleaseDC(hwnd, dc);
    }

    int iconSize = GetSystemMetrics(SM_CXSMICON);
    int height = std::max(iconSize + Scale(ctx, 8), textHeight + Scale(ctx, 8));
    int width = Scale(ctx, 8) + iconSize + Scale(ctx, 8) + textWidth + Scale(ctx, 24) +
                (data->hasSubmenu ? Scale(ctx, 20) : 0);
    measure->itemHeight = (UINT)height;
    measure->itemWidth = (UINT)width;
    return true;
}

bool HandleMenuDraw(AppContext& ctx, DRAWITEMSTRUCT* draw) {
    if (!draw || draw->CtlType != ODT_MENU) {
        return false;
    }
    MenuItemData* data = (MenuItemData*)draw->itemData;
    if (!data) {
        return false;
    }

    if (data->separator) {
        FillRect(draw->hDC, &draw->rcItem, ctx.menuBgBrush);
        int y = draw->rcItem.top + (draw->rcItem.bottom - draw->rcItem.top) / 2;
        HPEN pen = CreatePen(PS_SOLID, 1,
                             ctx.menuDark ? RGB(78, 78, 82) : GetSysColor(COLOR_3DSHADOW));
        HGDIOBJ oldPen = SelectObject(draw->hDC, pen);
        MoveToEx(draw->hDC, draw->rcItem.left + Scale(ctx, 8), y, nullptr);
        LineTo(draw->hDC, draw->rcItem.right - Scale(ctx, 8), y);
        SelectObject(draw->hDC, oldPen);
        DeleteObject(pen);
        return true;
    }

    bool selected = (draw->itemState & ODS_SELECTED) != 0;
    bool disabled = data->disabled ||
                    (draw->itemState & (ODS_DISABLED | ODS_GRAYED)) != 0;

    COLORREF background = selected
        ? (ctx.menuDark ? RGB(62, 62, 66) : GetSysColor(COLOR_HIGHLIGHT))
        : (ctx.menuDark ? RGB(37, 37, 38) : GetSysColor(COLOR_MENU));
    COLORREF foreground = disabled
        ? (ctx.menuDark ? RGB(130, 130, 130) : GetSysColor(COLOR_GRAYTEXT))
        : selected
            ? (ctx.menuDark ? RGB(255, 255, 255) : GetSysColor(COLOR_HIGHLIGHTTEXT))
            : (ctx.menuDark ? RGB(255, 255, 255) : GetSysColor(COLOR_MENUTEXT));

    HBRUSH brush = CreateSolidBrush(background);
    FillRect(draw->hDC, &draw->rcItem, brush);
    DeleteObject(brush);

    int iconSize = GetSystemMetrics(SM_CXSMICON);
    int iconX = draw->rcItem.left + Scale(ctx, 8);
    int iconY = draw->rcItem.top + ((draw->rcItem.bottom - draw->rcItem.top) - iconSize) / 2;
    if (data->icon) {
        DrawIconEx(draw->hDC, iconX, iconY, data->icon, iconSize, iconSize,
                   0, nullptr, DI_NORMAL);
    }

    RECT textRect = draw->rcItem;
    textRect.left += Scale(ctx, 8) + iconSize + Scale(ctx, 8);
    textRect.right -= data->hasSubmenu ? Scale(ctx, 24) : Scale(ctx, 12);
    SetBkMode(draw->hDC, TRANSPARENT);
    SetTextColor(draw->hDC, foreground);
    HGDIOBJ oldFont = SelectObject(draw->hDC, ctx.menuFont);
    DrawTextW(draw->hDC, data->text.c_str(), -1, &textRect,
              DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
    SelectObject(draw->hDC, oldFont);

    if (data->hasSubmenu) {
        int arrowX = draw->rcItem.right - Scale(ctx, 14);
        int middleY = (draw->rcItem.top + draw->rcItem.bottom) / 2;
        POINT points[3] = {
            { arrowX, middleY - Scale(ctx, 4) },
            { arrowX + Scale(ctx, 5), middleY },
            { arrowX, middleY + Scale(ctx, 4) }
        };
        HBRUSH arrowBrush = CreateSolidBrush(foreground);
        HGDIOBJ oldBrush = SelectObject(draw->hDC, arrowBrush);
        HGDIOBJ oldPen = SelectObject(draw->hDC, GetStockObject(NULL_PEN));
        Polygon(draw->hDC, points, 3);
        SelectObject(draw->hDC, oldPen);
        SelectObject(draw->hDC, oldBrush);
        DeleteObject(arrowBrush);
    }
    return true;
}

bool HandleMenuInitPopup(AppContext& ctx, WPARAM wParam, LPARAM lParam) {
    if (HIWORD(lParam)) {
        return false;
    }
    HMENU menu = (HMENU)wParam;
    auto it = ctx.lazyMenus.find(menu);
    if (it == ctx.lazyMenus.end()) {
        return false;
    }
    std::wstring path = it->second;
    ctx.lazyMenus.erase(it);
    PopulateSubmenu(ctx, menu, path);
    return true;
}

} // namespace mfm
