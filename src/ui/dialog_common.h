#pragma once

#include <windows.h>

#include "app/app_state.h"

namespace mfm {

// Owns the background brush of a themed dialog.
struct DialogTheme {
    bool dark = false;
    HBRUSH brush = nullptr;
};

void CenterWindow(HWND hwnd);

HWND CreateChild(const AppContext& ctx, HWND parent, const wchar_t* className,
                 const wchar_t* text, DWORD style, int x, int y, int width, int height,
                 int id, bool dark);

int CheckState(HWND control);
void SetChecked(HWND control, bool checked);

// Enter activates the focused push button (Export/Import/OK/Cancel) instead of
// always sending IDOK. Falls back to IDOK when focus is not a push button.
void ActivateFocusedButtonOrOk(HWND dialog);

// Runs a modal message loop for a modeless window until it is destroyed.
bool RunModal(HWND hwnd, HWND owner, HWND firstFocus, bool dark);

// Recreates the background brush, themes children and repaints the dialog.
void ApplyDialogTheme(HWND hwnd, DialogTheme& theme, bool dark);

void PaintDialogBackground(HWND hwnd, HDC dc, const DialogTheme& theme);

// Handles WM_CTLCOLORSTATIC/WM_CTLCOLORBTN. Returns true when themed.
bool HandleDialogControlColor(const DialogTheme& theme, HDC dc, HBRUSH& brush);

void DestroyDialogTheme(DialogTheme& theme);

} // namespace mfm
