#include "app/app_state.h"

#include "app/fonts.h"

namespace mfm {

AppContext::AppContext() = default;

AppContext::~AppContext() {
    CleanupAppContext(*this);
    DestroyFonts(*this);
}

int Scale(const AppContext& ctx, int value) {
    return MulDiv(value, ctx.dpi, 96);
}

void CleanupAppContext(AppContext& ctx) {
    if (ctx.menuBrush) {
        DeleteObject(ctx.menuBrush);
        ctx.menuBrush = nullptr;
    }
    if (ctx.menuBgBrush) {
        DeleteObject(ctx.menuBgBrush);
        ctx.menuBgBrush = nullptr;
    }
    ctx.icons.Clear();
    if (ctx.appIcon) {
        DestroyIcon(ctx.appIcon);
        ctx.appIcon = nullptr;
    }
}

} // namespace mfm
