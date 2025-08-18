// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "DebugViewppLib/ThemeManager.h"
#include "Win32/Registry.h"
#include <dwmapi.h>
#include <uxtheme.h>
#include <versionhelpers.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

namespace fusion {
namespace debugviewpp {

// Windows 10+ Dark Mode APIs
typedef BOOL(WINAPI* fnShouldAppsUseDarkMode)();
typedef DWORD(WINAPI* fnSetPreferredAppMode)(DWORD appMode);
typedef BOOL(WINAPI* fnAllowDarkModeForWindow)(HWND hWnd, BOOL allow);
typedef VOID(WINAPI* fnRefreshImmersiveColorPolicyState)();

static fnShouldAppsUseDarkMode ShouldAppsUseDarkMode = nullptr;
static fnSetPreferredAppMode SetPreferredAppMode = nullptr;
static fnAllowDarkModeForWindow AllowDarkModeForWindow = nullptr;
static fnRefreshImmersiveColorPolicyState RefreshImmersiveColorPolicyState = nullptr;

// Light theme colors (default)
static const ThemeColors s_lightColors = {
    RGB(255, 255, 255),  // background
    RGB(0, 0, 0),        // text
    RGB(255, 255, 55),   // highlight
    RGB(128, 255, 255),  // selection
    RGB(51, 153, 255),   // itemHighlight (selection blue)
    RGB(255, 255, 255),  // itemHighlightText
    RGB(240, 240, 240),  // windowBackground
    RGB(255, 255, 255),  // controlBackground
    RGB(0, 0, 0),        // controlText
    RGB(200, 200, 200),  // border
    RGB(240, 240, 240),  // scrollbar
    RGB(240, 240, 240),  // menuBackground
    RGB(0, 0, 0),        // menuText
    RGB(51, 153, 255),   // menuHighlight
    RGB(240, 240, 240),  // statusBarBackground
    RGB(0, 0, 0),        // statusBarText
    RGB(240, 240, 240),  // tabBackground
    RGB(0, 0, 0),        // tabText
    RGB(255, 255, 255),  // tabActive
    RGB(240, 240, 240),  // toolbarBackground
    RGB(0, 0, 0),        // toolbarText
    RGB(220, 220, 220)   // gridLines
};

// Dark theme colors
static const ThemeColors s_darkColors = {
    RGB(30, 30, 30),     // background
    RGB(220, 220, 220),  // text
    RGB(100, 100, 30),   // highlight (darker yellow)
    RGB(50, 100, 120),   // selection (darker blue)
    RGB(40, 80, 120),    // itemHighlight (selection blue)
    RGB(255, 255, 255),  // itemHighlightText
    RGB(25, 25, 25),     // windowBackground
    RGB(35, 35, 35),     // controlBackground
    RGB(220, 220, 220),  // controlText
    RGB(60, 60, 60),     // border
    RGB(50, 50, 50),     // scrollbar
    RGB(40, 40, 40),     // menuBackground
    RGB(220, 220, 220),  // menuText
    RGB(60, 60, 60),     // menuHighlight
    RGB(35, 35, 35),     // statusBarBackground
    RGB(220, 220, 220),  // statusBarText
    RGB(40, 40, 40),     // tabBackground
    RGB(220, 220, 220),  // tabText
    RGB(50, 50, 50),     // tabActive
    RGB(40, 40, 40),     // toolbarBackground
    RGB(220, 220, 220),  // toolbarText
    RGB(60, 60, 60)      // gridLines
};

ThemeManager& ThemeManager::Instance() {
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager() 
    : m_theme(Theme::Light)  // Default to Light theme on first run
    , m_effectiveTheme(Theme::Light)
    , m_systemSupportsThemes(false) {
    
    // Try to load Windows 10+ dark mode functions
    HMODULE hUxTheme = GetModuleHandle(L"uxtheme.dll");
    if (hUxTheme) {
        ShouldAppsUseDarkMode = (fnShouldAppsUseDarkMode)GetProcAddress(hUxTheme, MAKEINTRESOURCEA(132));
        SetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(hUxTheme, MAKEINTRESOURCEA(135));
        AllowDarkModeForWindow = (fnAllowDarkModeForWindow)GetProcAddress(hUxTheme, MAKEINTRESOURCEA(133));
        RefreshImmersiveColorPolicyState = (fnRefreshImmersiveColorPolicyState)GetProcAddress(hUxTheme, MAKEINTRESOURCEA(104));
        
        m_systemSupportsThemes = (ShouldAppsUseDarkMode != nullptr);
    }
    
    UpdateColors();
}

void ThemeManager::SetTheme(Theme theme) {
    if (m_theme != theme) {
        m_theme = theme;
        UpdateColors();
        SaveToRegistry();
    }
}

Theme ThemeManager::GetTheme() const {
    return m_theme;
}

Theme ThemeManager::GetEffectiveTheme() const {
    return m_effectiveTheme;
}

const ThemeColors& ThemeManager::GetColors() const {
    return m_colors;
}

bool ThemeManager::IsDarkTheme() const {
    return m_effectiveTheme == Theme::Dark;
}

void ThemeManager::UpdateColors() {
    if (m_theme == Theme::System) {
        m_effectiveTheme = IsSystemDarkMode() ? Theme::Dark : Theme::Light;
    } else {
        m_effectiveTheme = m_theme;
    }
    
    m_colors = (m_effectiveTheme == Theme::Dark) ? s_darkColors : s_lightColors;
}

bool ThemeManager::IsSystemDarkMode() const {
    if (!m_systemSupportsThemes) {
        return false;
    }
    
    // Check Windows system theme
    if (ShouldAppsUseDarkMode) {
        return ShouldAppsUseDarkMode();
    }
    
    // Fallback: check registry
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD value = 0;
        DWORD size = sizeof(value);
        DWORD type = REG_DWORD;
        if (RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, &type, (BYTE*)&value, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return value == 0;  // 0 means dark theme
        }
        RegCloseKey(hKey);
    }
    
    return false;
}

void ThemeManager::EnableDarkModeForWindow(HWND hwnd) {
    if (!m_systemSupportsThemes || !hwnd) {
        return;
    }
    
    bool isDark = IsDarkTheme();
    
    // Allow dark mode for window
    if (AllowDarkModeForWindow) {
        AllowDarkModeForWindow(hwnd, isDark);
    }
    
    // Set dark mode for title bar
    BOOL darkMode = isDark ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, 20, &darkMode, sizeof(darkMode));  // DWMWA_USE_IMMERSIVE_DARK_MODE
    
    // Refresh frame
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void ThemeManager::ApplyToWindow(HWND hwnd) {
    EnableDarkModeForWindow(hwnd);
    
    // Force redraw
    InvalidateRect(hwnd, nullptr, TRUE);
}

void ThemeManager::RefreshSystemTheme() {
    if (RefreshImmersiveColorPolicyState) {
        RefreshImmersiveColorPolicyState();
    }
    UpdateColors();
}

void ThemeManager::LoadFromRegistry() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\DebugView++", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD theme = 0;
        DWORD size = sizeof(theme);
        DWORD type = REG_DWORD;
        if (RegQueryValueExW(hKey, L"Theme", nullptr, &type, (BYTE*)&theme, &size) == ERROR_SUCCESS) {
            if (theme <= static_cast<DWORD>(Theme::System)) {
                m_theme = static_cast<Theme>(theme);
            }
        }
        RegCloseKey(hKey);
    }
    UpdateColors();
}

void ThemeManager::SaveToRegistry() {
    HKEY hKey;
    DWORD disposition;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\DebugView++", 0, nullptr, 
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, &disposition) == ERROR_SUCCESS) {
        DWORD theme = static_cast<DWORD>(m_theme);
        RegSetValueExW(hKey, L"Theme", 0, REG_DWORD, (BYTE*)&theme, sizeof(theme));
        RegCloseKey(hKey);
    }
}

} // namespace debugviewpp
} // namespace fusion