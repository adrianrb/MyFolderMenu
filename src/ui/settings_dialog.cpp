#include "ui/settings_dialog.h"

#include <commdlg.h>

#include "core/app_info.h"
#include "core/config.h"
#include "core/startup.h"
#include "core/theme.h"
#include "ui/dialog_common.h"

namespace mfm {

namespace {

struct SettingsState {
    const AppContext* ctx = nullptr;
    HWND hwnd = nullptr;
    bool ok = false;
    bool dark = false;
    AppSettings settings;
    std::vector<std::wstring> folders;
    HWND startup = nullptr;
    HWND hideExtensions = nullptr;
    HWND lightRadio = nullptr;
    HWND darkRadio = nullptr;
    HWND systemRadio = nullptr;
    DialogTheme theme;
};

void CreateSettingsControls(HWND hwnd, SettingsState* state) {
    bool dark = state->dark;
    const AppContext& ctx = *state->ctx;

    state->startup = CreateChild(ctx, hwnd, L"BUTTON", L"Iniciar com o Windows",
                                 BS_AUTOCHECKBOX | WS_TABSTOP, 24, 24, 260, 22,
                                 kIdStartup, dark);
    state->hideExtensions = CreateChild(ctx, hwnd, L"BUTTON",
                                        L"Não exibir a extensão do arquivo",
                                        BS_AUTOCHECKBOX | WS_TABSTOP, 24, 58, 300, 22,
                                        kIdHideExt, dark);
    SetChecked(state->startup, StartupEnabled());
    SetChecked(state->hideExtensions, state->settings.hideFileExtensions);

    CreateChild(ctx, hwnd, L"BUTTON", L"Tema", BS_GROUPBOX, 24, 98, 332, 118, 0, dark);
    state->lightRadio = CreateChild(ctx, hwnd, L"BUTTON", L"Claro",
                                    BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP,
                                    44, 126, 140, 22, kIdThemeLight, dark);
    state->darkRadio = CreateChild(ctx, hwnd, L"BUTTON", L"Escuro",
                                   BS_AUTORADIOBUTTON | WS_TABSTOP,
                                   44, 154, 140, 22, kIdThemeDark, dark);
    state->systemRadio = CreateChild(ctx, hwnd, L"BUTTON", L"Sistema",
                                     BS_AUTORADIOBUTTON | WS_TABSTOP,
                                     44, 182, 140, 22, kIdThemeSystem, dark);

    SetChecked(state->lightRadio, state->settings.theme == AppTheme::Light);
    SetChecked(state->darkRadio, state->settings.theme == AppTheme::Dark);
    SetChecked(state->systemRadio, state->settings.theme == AppTheme::System);

    CreateChild(ctx, hwnd, L"BUTTON", L"Exportar configurações",
                BS_PUSHBUTTON | WS_TABSTOP, 24, 232, 164, 32, kIdExport, dark);
    CreateChild(ctx, hwnd, L"BUTTON", L"Importar configurações",
                BS_PUSHBUTTON | WS_TABSTOP, 192, 232, 164, 32, kIdImport, dark);
    CreateChild(ctx, hwnd, L"BUTTON", L"OK", BS_DEFPUSHBUTTON | WS_TABSTOP,
                166, 284, 90, 32, IDOK, dark);
    CreateChild(ctx, hwnd, L"BUTTON", L"Cancelar", BS_PUSHBUTTON | WS_TABSTOP,
                266, 284, 90, 32, IDCANCEL, dark);
}

AppSettings ReadSettingsControls(const SettingsState* state) {
    AppSettings settings;
    settings.hideFileExtensions = CheckState(state->hideExtensions) == BST_CHECKED;
    if (CheckState(state->darkRadio) == BST_CHECKED) {
        settings.theme = AppTheme::Dark;
    } else if (CheckState(state->lightRadio) == BST_CHECKED) {
        settings.theme = AppTheme::Light;
    } else {
        settings.theme = AppTheme::System;
    }
    return settings;
}

void ApplySettingsAndClose(HWND hwnd, SettingsState* state) {
    bool startWithWindows = CheckState(state->startup) == BST_CHECKED;
    if (!StartupSetEnabled(startWithWindows)) {
        MessageBoxW(hwnd, L"Não foi possível alterar a inicialização com o Windows.",
                    kAppName, MB_OK | MB_ICONERROR);
        return;
    }
    state->settings = ReadSettingsControls(state);
    state->ok = true;
    DestroyWindow(hwnd);
}

void ExportSettings(HWND hwnd, SettingsState* state) {
    AppSettings settings = ReadSettingsControls(state);
    wchar_t file[MAX_PATH] = L"MyFolderMenu.ini";
    OPENFILENAMEW dialog = {};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hwnd;
    dialog.lpstrFilter =
        L"Configuração do MyFolderMenu (*.ini)\0*.ini\0Todos os arquivos (*.*)\0*.*\0\0";
    dialog.lpstrFile = file;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrDefExt = L"ini";
    dialog.lpstrTitle = L"Exportar configurações";
    dialog.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameW(&dialog)) {
        return;
    }
    if (WriteIniFile(file, state->folders, settings)) {
        std::wstring message = L"Configurações exportadas para:\n" + std::wstring(file);
        MessageBoxW(hwnd, message.c_str(), kAppName, MB_OK | MB_ICONINFORMATION);
    } else {
        std::wstring message =
            L"Não foi possível exportar as configurações para:\n" + std::wstring(file);
        MessageBoxW(hwnd, message.c_str(), kAppName, MB_OK | MB_ICONERROR);
    }
}

void ImportSettings(HWND hwnd, SettingsState* state) {
    wchar_t file[MAX_PATH] = {};
    OPENFILENAMEW dialog = {};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hwnd;
    dialog.lpstrFilter =
        L"Configuração do MyFolderMenu (*.ini)\0*.ini\0Todos os arquivos (*.*)\0*.*\0\0";
    dialog.lpstrFile = file;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrDefExt = L"ini";
    dialog.lpstrTitle = L"Importar configurações";
    dialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&dialog)) {
        return;
    }

    std::vector<std::wstring> folders;
    AppSettings settings;
    if (!ReadIniFile(file, folders, settings)) {
        std::wstring message =
            L"Arquivo de configuração inválido ou inacessível:\n" + std::wstring(file);
        MessageBoxW(hwnd, message.c_str(), kAppName, MB_OK | MB_ICONERROR);
        return;
    }

    state->folders = folders;
    SetChecked(state->hideExtensions, settings.hideFileExtensions);
    SetChecked(state->lightRadio, settings.theme == AppTheme::Light);
    SetChecked(state->darkRadio, settings.theme == AppTheme::Dark);
    SetChecked(state->systemRadio, settings.theme == AppTheme::System);

    bool dark = IsDarkTheme(settings.theme);
    SetAppThemeMode(dark);
    ApplyDialogTheme(hwnd, state->theme, dark);
    state->dark = dark;

    std::wstring message = L"Configurações importadas de:\n" + std::wstring(file) +
                           L"\n\nClique em OK para aplicar.";
    MessageBoxW(hwnd, message.c_str(), kAppName, MB_OK | MB_ICONINFORMATION);
}

LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    SettingsState* state = (SettingsState*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    switch (message) {
        case WM_CREATE: {
            CREATESTRUCTW* create = (CREATESTRUCTW*)lParam;
            state = (SettingsState*)create->lpCreateParams;
            state->hwnd = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)state);
            CreateSettingsControls(hwnd, state);
            ApplyDialogTheme(hwnd, state->theme, state->dark);
            return 0;
        }
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            if (!state) {
                break;
            }
            HBRUSH brush = nullptr;
            if (HandleDialogControlColor(state->theme, (HDC)wParam, brush)) {
                return (LRESULT)brush;
            }
            break;
        }
        case WM_ERASEBKGND: {
            if (!state) {
                break;
            }
            PaintDialogBackground(hwnd, (HDC)wParam, state->theme);
            return 1;
        }
        case WM_COMMAND: {
            if (!state) {
                break;
            }
            switch (LOWORD(wParam)) {
                case IDOK:
                    ApplySettingsAndClose(hwnd, state);
                    return 0;
                case IDCANCEL:
                    DestroyWindow(hwnd);
                    return 0;
                case kIdExport:
                    ExportSettings(hwnd, state);
                    return 0;
                case kIdImport:
                    ImportSettings(hwnd, state);
                    return 0;
                default:
                    break;
            }
            break;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            if (state) {
                DestroyDialogTheme(state->theme);
            }
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace

bool RegisterSettingsDialogClass(HINSTANCE instance) {
    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpfnWndProc = SettingsWndProc;
    windowClass.lpszClassName = L"MFMSettingsDialog";
    return RegisterClassExW(&windowClass) != 0;
}

bool ShowSettingsDialog(AppContext& ctx, HWND owner, AppSettings& settings,
                        std::vector<std::wstring>& folders) {
    SettingsState state;
    state.ctx = &ctx;
    state.settings = settings;
    state.folders = folders;
    state.dark = IsDarkTheme(settings.theme);
    SetAppThemeMode(state.dark);

    RECT rect = {0, 0, Scale(ctx, 380), Scale(ctx, 340)};
    DWORD style = WS_CAPTION | WS_SYSMENU;
    DWORD exStyle = WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT;
    AdjustWindowRectEx(&rect, style, FALSE, exStyle);
    HWND hwnd = CreateWindowExW(exStyle, L"MFMSettingsDialog", L"Configurações", style,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                rect.right - rect.left, rect.bottom - rect.top,
                                owner, nullptr, ctx.instance, &state);
    if (!hwnd) {
        return false;
    }
    RunModal(hwnd, owner, state.startup, state.dark);
    if (state.ok) {
        settings = state.settings;
        folders = state.folders;
        return true;
    }
    return false;
}

} // namespace mfm
