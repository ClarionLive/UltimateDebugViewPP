// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "atleverything.h"
#include <atlctrls.h>
#include <atlctrlx.h>
#include <vector>
#include <memory>

namespace fusion {
namespace debugviewpp {

// Modern toolbar button with icon and text label
class CModernToolbarButton : public CWindowImpl<CModernToolbarButton, CButton>
{
public:
    DECLARE_WND_SUPERCLASS(NULL, CButton::GetWndClassName())
    
    BEGIN_MSG_MAP(CModernToolbarButton)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    END_MSG_MAP()
    
    void SetIcon(HICON hIcon) { m_hIcon = hIcon; }
    void SetText(const std::wstring& text) { m_text = text; }
    void SetTooltip(const std::wstring& tooltip) { m_tooltip = tooltip; }
    void SetDropdown(bool hasDropdown) { m_hasDropdown = hasDropdown; }
    
    LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnMouseLeave(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    
    static const int ICON_SIZE = 24;
    static const int BUTTON_HEIGHT = 64;
    static const int BUTTON_WIDTH = 72;
    static const int PADDING = 4;
    
private:
    HICON m_hIcon = nullptr;
    std::wstring m_text;
    std::wstring m_tooltip;
    bool m_hasDropdown = false;
    bool m_isHovered = false;
    bool m_isPressed = false;
};

// Modern toolbar group that contains related buttons
class CModernToolbarGroup : public CWindowImpl<CModernToolbarGroup>
{
public:
    DECLARE_WND_CLASS_EX(_T("ModernToolbarGroup"), 0, COLOR_WINDOW)
    
    BEGIN_MSG_MAP(CModernToolbarGroup)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    END_MSG_MAP()
    
    void SetTitle(const std::wstring& title) { m_title = title; }
    void AddButton(UINT nID, const std::wstring& text, HICON hIcon, const std::wstring& tooltip = L"");
    void AddSeparator();
    
    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    
private:
    struct ButtonInfo
    {
        UINT nID;
        std::wstring text;
        HICON hIcon;
        std::wstring tooltip;
        bool isSeparator;
    };
    
    std::wstring m_title;
    std::vector<ButtonInfo> m_buttons;
    std::vector<std::unique_ptr<CModernToolbarButton>> m_buttonControls;
    
    static const int TITLE_HEIGHT = 20;
    static const int GROUP_PADDING = 8;
};

// Modern toolbar with tabbed interface
class CModernToolbar : public CWindowImpl<CModernToolbar>
{
public:
    DECLARE_WND_CLASS_EX(_T("ModernToolbar"), 0, COLOR_WINDOW)
    
    BEGIN_MSG_MAP(CModernToolbar)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        COMMAND_CODE_HANDLER(BN_CLICKED, OnButtonClicked)
        NOTIFY_CODE_HANDLER(TCN_SELCHANGE, OnTabSelChange)
    END_MSG_MAP()
    
    // Tab management
    void AddTab(const std::wstring& name);
    void SetActiveTab(int index);
    int GetActiveTab() const { return m_activeTab; }
    
    // Group management for current tab
    CModernToolbarGroup* AddGroup(const std::wstring& title);
    
    // Quick access toolbar
    void AddQuickAccessButton(UINT nID, HICON hIcon, const std::wstring& tooltip);
    
    // Event handlers
    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnButtonClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnTabSelChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
    
    // Configuration
    void SetMinimized(bool minimized);
    bool IsMinimized() const { return m_isMinimized; }
    
    // Height calculation
    int GetHeight() const;
    
private:
    struct TabInfo
    {
        std::wstring name;
        std::vector<std::unique_ptr<CModernToolbarGroup>> groups;
    };
    
    CTabCtrl m_tabCtrl;
    std::vector<TabInfo> m_tabs;
    int m_activeTab = 0;
    bool m_isMinimized = false;
    
    // Quick access toolbar
    struct QuickAccessButton
    {
        UINT nID;
        HICON hIcon;
        std::wstring tooltip;
        std::unique_ptr<CButton> button;
    };
    std::vector<QuickAccessButton> m_quickAccessButtons;
    
    static const int TAB_HEIGHT = 28;
    static const int TOOLBAR_HEIGHT = 100;
    static const int MINIMIZED_HEIGHT = 32;
    static const int QAT_BUTTON_SIZE = 24;
    
    void LayoutGroups();
    void ShowTab(int index);
};

// Helper class to manage modern toolbar in MainFrame
class CModernToolbarManager
{
public:
    CModernToolbarManager(HWND hWndParent);
    ~CModernToolbarManager();
    
    // Initialize the modern toolbar
    bool Create();
    
    // Setup default toolbar configuration
    void SetupDefaultLayout();
    
    // Get the toolbar window
    CModernToolbar& GetToolbar() { return m_toolbar; }
    
    // Load/Save toolbar settings
    void LoadSettings();
    void SaveSettings();
    
private:
    HWND m_hWndParent;
    CModernToolbar m_toolbar;
    
    // Icon management
    std::vector<HICON> m_icons;
    HICON LoadIcon(UINT nID, int size = 24);
};

} // namespace debugviewpp
} // namespace fusion