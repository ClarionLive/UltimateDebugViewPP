// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <windows.h>
#include <string>

namespace fusion {
namespace debugviewpp {

enum class Theme {
    Light = 0,
    Dark = 1,
    System = 2
};

struct ThemeColors {
    COLORREF background;
    COLORREF text;
    COLORREF highlight;
    COLORREF selection;
    COLORREF itemHighlight;
    COLORREF itemHighlightText;
    COLORREF windowBackground;
    COLORREF controlBackground;
    COLORREF controlText;
    COLORREF border;
    COLORREF scrollbar;
    COLORREF menuBackground;
    COLORREF menuText;
    COLORREF menuHighlight;
    COLORREF statusBarBackground;
    COLORREF statusBarText;
    COLORREF tabBackground;
    COLORREF tabText;
    COLORREF tabActive;
    COLORREF toolbarBackground;
    COLORREF toolbarText;
    COLORREF gridLines;
};

class ThemeManager {
public:
    static ThemeManager& Instance();
    
    void SetTheme(Theme theme);
    Theme GetTheme() const;
    Theme GetEffectiveTheme() const;
    
    const ThemeColors& GetColors() const;
    bool IsDarkTheme() const;
    
    void ApplyToWindow(HWND hwnd);
    void RefreshSystemTheme();
    
    void LoadFromRegistry();
    void SaveToRegistry();
    
private:
    ThemeManager();
    ~ThemeManager() = default;
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;
    
    void UpdateColors();
    bool IsSystemDarkMode() const;
    void EnableDarkModeForWindow(HWND hwnd);
    
    Theme m_theme;
    Theme m_effectiveTheme;
    ThemeColors m_colors;
    bool m_systemSupportsThemes;
};

} // namespace debugviewpp
} // namespace fusion