#include "app/fonts.h"

namespace mfm {

namespace {

bool IsStockFont(HFONT font) {
    return font == nullptr || font == (HFONT)GetStockObject(DEFAULT_GUI_FONT);
}

} // namespace

void CreateFonts(AppContext& ctx) {
    NONCLIENTMETRICSW metrics = {};
    metrics.cbSize = sizeof(metrics);
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0)) {
        ctx.menuFont = CreateFontIndirectW(&metrics.lfMenuFont);
        ctx.uiFont = CreateFontIndirectW(&metrics.lfMessageFont);
    }
    if (!ctx.menuFont) {
        ctx.menuFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    }
    if (!ctx.uiFont) {
        ctx.uiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    }
}

void DestroyFonts(AppContext& ctx) {
    if (!IsStockFont(ctx.menuFont)) {
        DeleteObject(ctx.menuFont);
    }
    if (!IsStockFont(ctx.uiFont)) {
        DeleteObject(ctx.uiFont);
    }
    ctx.menuFont = nullptr;
    ctx.uiFont = nullptr;
}

} // namespace mfm
