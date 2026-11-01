#pragma once

#include <windows.h>

namespace mfm {

// Test-friendly variants that accept a custom registry value name.
bool StartupEnabledFor(const wchar_t* valueName);
bool StartupSetEnabledFor(const wchar_t* valueName, bool enabled);

// Production wrappers using the application name.
bool StartupEnabled();
bool StartupSetEnabled(bool enabled);

} // namespace mfm
