// Dark mode support for the classic Win32/WTL controls used by the viewer.
//
// The colours come from ThemeManager (Light / Dark / System). This module takes care of the parts of the
// window that Windows does not darken by itself: menus, toolbars, rebar, tab strips, list view headers,
// scroll bars, the status bar and the standard dialogs.

#pragma once

#include <windows.h>

namespace fusion {
namespace debugviewpp {
namespace DarkMode {

// Call once on the UI thread, before the first window is created. It installs a thread hook that
// themes dialogs and standard controls as they are created.
void Initialize();

// True when the effective theme is dark.
bool IsEnabled();

// Synchronises the OS level dark mode (popup menus, scroll bars) with ThemeManager and (re)themes
// `root` and all of its descendants. Call it after the theme changed and after loading the settings.
void Refresh(HWND root);

// Subclasses the main frame so that toolbars and the command bar (menu bar) draw with dark colours.
void InstallFrameHooks(HWND frame);

// (Re)themes a single control, picking the right treatment from its window class.
void ApplyToControl(HWND hwnd);

// Colours used for owner drawn popup menus.
struct MenuColors
{
    COLORREF background;
    COLORREF text;
    COLORREF disabledText;
    COLORREF highlight;
    COLORREF border;
    COLORREF accent;
};
MenuColors GetMenuColors();

} // namespace DarkMode
} // namespace debugviewpp
} // namespace fusion
