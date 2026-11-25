// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "atleverything.h"
#include <atlcom.h>
#include <atlctrls.h>
#include <richedit.h>
#include "resource.h"
#include "DebugViewppLib/ThemeManager.h"
#include "DebugViewppLib/SyntaxHighlighter.h"
#include "ScintillaWrapper.h"
#include "DebugViewppLib/ClarionProcedureTracker.h"
#include "CQueueGridView.h"
#include "ClarionProcedureTimelineView.h"

// Define TCN_RCLICK if not already defined
#ifndef TCN_RCLICK
#define TCN_RCLICK (TCN_FIRST - 3)
#endif

// Custom message for deferred window cleanup
#ifndef WM_FLOATING_WINDOW_CLOSING
#define WM_FLOATING_WINDOW_CLOSING (WM_USER + 1000)
#endif

#pragma warning(push, 3)
#pragma warning(disable : 4838)

#include "CustomTabCtrl.h"
#include "DotNetTabCtrl.h"
#include "TabbedFrame.h"
#pragma warning(pop)

#include <memory>

namespace fusion {
namespace debugviewpp {

class CLogView;
class CMainFrame;
class ClarionProcedureTracker;
class CFloatingTabWindow;
class CProcedureFlowTabView;
class COpenProceduresTabView;
class CProcedureTimelineTabView;
class CQueueGridView;
class CQueueTabView;
class CHelpTabView;

// Forward declarations for Queue support
struct QueueData;

// Base interface for detail tab views to ensure consistent management
class IDetailTabView
{
public:
    virtual ~IDetailTabView() = default;
    virtual void Create(HWND parent, const RECT& rc) = 0;
    virtual HWND GetHWND() = 0;
    virtual void SetVisible(bool visible) = 0;
    virtual void Resize(const RECT& rc) = 0;
    virtual void RefreshTheme() = 0;
    virtual std::wstring GetTabName() const = 0;
};

class CLogViewTabItem : public CTabViewTabItem
{
public:
    void SetView(std::shared_ptr<CLogView> pView);
    CLogView& GetView();

    void Create(HWND parent) { m_parent = parent; }
    HWND GetLogViewParent() { return m_parent; }

private:
    std::shared_ptr<CLogView> m_pView;
    HWND m_parent = HWND();
};

class CMyPaneContainer : public CPaneContainerImpl<CMyPaneContainer>
{
public:
    DECLARE_WND_CLASS_EX(_T("MY_PaneContainer"), CS_HREDRAW | CS_VREDRAW, COLOR_WINDOW)

    BEGIN_MSG_MAP(CMyPaneContainerImpl)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        REFLECT_NOTIFICATIONS()
        CHAIN_MSG_MAP(CPaneContainerImpl<CMyPaneContainer>)
    END_MSG_MAP()
    
    LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CDCHandle dc((HDC)wParam);
        RECT rect;
        GetClientRect(&rect);
        dc.FillRect(&rect, COLOR_WINDOW + 1);
        return 1;
    }
    
    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        bHandled = FALSE;
        Invalidate();
        return 0;
    }
};


// Modern styled button for detail toolbar
class CDetailToolbarButton : public CWindowImpl<CDetailToolbarButton, CButton>
{
public:
    DECLARE_WND_SUPERCLASS(NULL, CButton::GetWndClassName())
    
    BEGIN_MSG_MAP(CDetailToolbarButton)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    END_MSG_MAP()
    
    void SetText(const std::wstring& text) { m_text = text; }
    void SetIcon(HICON hIcon) { m_hIcon = hIcon; }
    
    virtual LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CPaintDC dc(m_hWnd);
        CRect rcClient;
        GetClientRect(&rcClient);

        // Check if button is enabled
        bool isEnabled = IsWindowEnabled();

        // Get current theme
        bool isDarkTheme = fusion::debugviewpp::ThemeManager::Instance().IsDarkTheme();
        
        // Fill with parent's background color
        CBrush bgBrush;
        if (isDarkTheme)
        {
            bgBrush.CreateSolidBrush(RGB(45, 45, 45));  // Dark toolbar
        }
        else
        {
            bgBrush.CreateSolidBrush(RGB(245, 245, 245));  // Light toolbar
        }
        dc.FillRect(&rcClient, bgBrush);
        
        // Only draw hover/press effects as overlay if enabled
        if (isEnabled && (m_isHovered || m_isPressed))
        {
            // Draw a subtle highlight/shadow overlay
            COLORREF overlayColor;
            if (isDarkTheme)
            {
                overlayColor = m_isPressed ? RGB(30, 30, 30) : RGB(60, 60, 60);
            }
            else
            {
                overlayColor = m_isPressed ? RGB(200, 200, 200) : RGB(225, 225, 225);
            }
            CBrush overlayBrush;
            overlayBrush.CreateSolidBrush(overlayColor);
            
            // Create rounded rect effect
            CRect rcOverlay = rcClient;
            rcOverlay.DeflateRect(1, 1);
            dc.FillRect(&rcOverlay, overlayBrush);
        }
        
        // Calculate positions
        int iconSize = 16;
        int padding = 4;
        int totalWidth = 0;
        
        if (m_hIcon)
            totalWidth += iconSize;
        if (!m_text.empty())
        {
            if (m_hIcon) totalWidth += padding;
            CSize textSize;
            dc.GetTextExtent(m_text.c_str(), -1, &textSize);
            totalWidth += textSize.cx;
        }
        
        int x = (rcClient.Width() - totalWidth) / 2;
        int y = (rcClient.Height() - iconSize) / 2;
        
        // Draw icon (grayed out if disabled)
        if (m_hIcon)
        {
            if (isEnabled)
            {
                dc.DrawIconEx(x, y, m_hIcon, iconSize, iconSize, 0, NULL, DI_NORMAL);
            }
            else
            {
                // Draw disabled icon (grayed out)
                dc.DrawState(CPoint(x, y), CSize(iconSize, iconSize), m_hIcon,
                            DSS_DISABLED | DST_ICON, (HBRUSH)NULL);
            }
            x += iconSize + padding;
        }

        // Draw text
        if (!m_text.empty())
        {
            dc.SetBkMode(TRANSPARENT);

            // Set text color based on enabled state
            if (!isEnabled)
            {
                // Disabled text color
                if (isDarkTheme)
                {
                    dc.SetTextColor(RGB(100, 100, 100));  // Gray text for disabled in dark theme
                }
                else
                {
                    dc.SetTextColor(RGB(160, 160, 160));  // Gray text for disabled in light theme
                }
            }
            else if (isDarkTheme)
            {
                dc.SetTextColor(RGB(220, 220, 220));  // Light text for dark theme
            }
            else
            {
                dc.SetTextColor(RGB(60, 60, 60));  // Dark text for light theme
            }
            
            CFont font;
            font.CreatePointFont(90, L"Segoe UI");
            CFontHandle oldFont = dc.SelectFont(font);
            
            CRect rcText = rcClient;
            rcText.left = x;
            dc.DrawText(m_text.c_str(), -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            dc.SelectFont(oldFont);
        }
        
        return 0;
    }
    
    LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        return 1;
    }
    
    LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        // Only track hover if enabled
        if (IsWindowEnabled() && !m_isHovered)
        {
            m_isHovered = true;
            TRACKMOUSEEVENT tme = {0};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = m_hWnd;
            TrackMouseEvent(&tme);
            Invalidate();
        }
        return 0;
    }
    
    LRESULT OnMouseLeave(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        m_isHovered = false;
        m_isPressed = false;
        Invalidate();
        return 0;
    }
    
    LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        if (IsWindowEnabled())
        {
            m_isPressed = true;
            Invalidate();
        }
        bHandled = FALSE;
        return 0;
    }
    
    LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        m_isPressed = false;
        Invalidate();
        bHandled = FALSE;
        return 0;
    }
    
protected:  // Changed to protected so derived class can access
    std::wstring m_text;
    HICON m_hIcon = nullptr;
    bool m_isHovered = false;
    bool m_isPressed = false;
};

// Toggle button for detail toolbar (extends CDetailToolbarButton)
class CDetailToolbarToggleButton : public CDetailToolbarButton
{
public:
    DECLARE_WND_SUPERCLASS(L"DetailToolbarToggleButton", CButton::GetWndClassName())
    
    BEGIN_MSG_MAP(CDetailToolbarToggleButton)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        CHAIN_MSG_MAP(CDetailToolbarButton)
    END_MSG_MAP()
    
    void SetToggled(bool toggled) 
    { 
        m_isToggled = toggled; 
        if (m_hWnd)
            Invalidate();
    }
    
    bool GetToggled() const { return m_isToggled; }
    
    void Toggle() 
    { 
        m_isToggled = !m_isToggled;
        if (m_hWnd)
            Invalidate();
    }
    
    LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CPaintDC dc(m_hWnd);
        CRect rcClient;
        GetClientRect(&rcClient);

        // Check if button is enabled
        bool isEnabled = IsWindowEnabled();

        // Get current theme
        bool isDarkTheme = fusion::debugviewpp::ThemeManager::Instance().IsDarkTheme();

        // Fill with parent's background color
        CBrush bgBrush;
        if (isDarkTheme)
        {
            bgBrush.CreateSolidBrush(RGB(45, 45, 45));  // Dark toolbar
        }
        else
        {
            bgBrush.CreateSolidBrush(RGB(245, 245, 245));  // Light toolbar
        }
        dc.FillRect(&rcClient, bgBrush);
        
        // Draw toggle state background (only if enabled)
        if (isEnabled && m_isToggled)
        {
            CBrush toggleBrush;
            if (isDarkTheme)
            {
                toggleBrush.CreateSolidBrush(RGB(70, 90, 110));  // Dark blue for dark theme
            }
            else
            {
                toggleBrush.CreateSolidBrush(RGB(201, 224, 247));  // Light blue for light theme
            }
            CRect rcToggle = rcClient;
            rcToggle.DeflateRect(1, 1);
            dc.FillRect(&rcToggle, toggleBrush);
        }
        // Draw hover/press effects as overlay (only if enabled)
        else if (isEnabled && (m_isHovered || m_isPressed))
        {
            COLORREF overlayColor;
            if (isDarkTheme)
            {
                overlayColor = m_isPressed ? RGB(30, 30, 30) : RGB(60, 60, 60);
            }
            else
            {
                overlayColor = m_isPressed ? RGB(200, 200, 200) : RGB(225, 225, 225);
            }
            CBrush overlayBrush;
            overlayBrush.CreateSolidBrush(overlayColor);
            
            CRect rcOverlay = rcClient;
            rcOverlay.DeflateRect(1, 1);
            dc.FillRect(&rcOverlay, overlayBrush);
        }
        
        // Calculate positions for icon and text
        int iconSize = 16;
        int padding = 4;
        int totalWidth = 0;
        
        if (m_hIcon)
            totalWidth += iconSize;
        if (!m_text.empty())
        {
            if (m_hIcon) totalWidth += padding;
            CSize textSize;
            dc.GetTextExtent(m_text.c_str(), -1, &textSize);
            totalWidth += textSize.cx;
        }
        
        int x = (rcClient.Width() - totalWidth) / 2;
        int y = (rcClient.Height() - iconSize) / 2;
        
        // Draw icon (grayed out if disabled)
        if (m_hIcon)
        {
            if (isEnabled)
            {
                dc.DrawIconEx(x, y, m_hIcon, iconSize, iconSize, 0, NULL, DI_NORMAL);
            }
            else
            {
                // Draw disabled icon (grayed out)
                dc.DrawState(CPoint(x, y), CSize(iconSize, iconSize), m_hIcon,
                            DSS_DISABLED | DST_ICON, (HBRUSH)NULL);
            }
            x += iconSize + padding;
        }

        // Draw text
        if (!m_text.empty())
        {
            dc.SetBkMode(TRANSPARENT);

            // Set text color based on enabled state
            if (!isEnabled)
            {
                // Disabled text color
                if (isDarkTheme)
                {
                    dc.SetTextColor(RGB(100, 100, 100));  // Gray text for disabled in dark theme
                }
                else
                {
                    dc.SetTextColor(RGB(160, 160, 160));  // Gray text for disabled in light theme
                }
            }
            else if (isDarkTheme)
            {
                dc.SetTextColor(RGB(220, 220, 220));  // Light text for dark theme
            }
            else
            {
                dc.SetTextColor(RGB(60, 60, 60));  // Dark text for light theme
            }
            
            CFont font;
            font.CreatePointFont(90, L"Segoe UI");
            CFontHandle oldFont = dc.SelectFont(font);
            
            CRect rcText = rcClient;
            rcText.left = x;
            dc.DrawText(m_text.c_str(), -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            dc.SelectFont(oldFont);
        }
        
        return 0;
    }
    
protected:
    bool m_isToggled = false;
};

// Custom checkbox with transparent background (keeping for compatibility)
class CTransparentCheckBox : public CWindowImpl<CTransparentCheckBox, CButton>
{
public:
    DECLARE_WND_SUPERCLASS(NULL, CButton::GetWndClassName())
    
    BEGIN_MSG_MAP(CTransparentCheckBox)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        REFLECTED_COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked)
    END_MSG_MAP()
    
    LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        return 1;
    }
    
    LRESULT OnClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
    {
        bHandled = FALSE;
        Invalidate();
        return 0;
    }
};

// Detail view wrapper window
class CDetailView : public CWindowImpl<CDetailView>
{
public:
    DECLARE_WND_CLASS_EX(_T("DetailView"), 0, -1)  // -1 means no default background
    
    BEGIN_MSG_MAP(CDetailView)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_SHOWWINDOW, OnShowWindow)
        MESSAGE_HANDLER(WM_USER + 100, OnSetInitialText)
        MESSAGE_HANDLER(WM_CTLCOLORBTN, OnCtlColorBtn)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        COMMAND_HANDLER(IDC_DETAIL_FIND, BN_CLICKED, OnFind)
        COMMAND_HANDLER(IDC_DETAIL_FILTER, BN_CLICKED, OnFilter)
        COMMAND_HANDLER(IDC_DETAIL_WORDWRAP, BN_CLICKED, OnWordWrap)
        COMMAND_HANDLER(IDC_DETAIL_SYNTAX, BN_CLICKED, OnSyntaxHighlight)
        COMMAND_HANDLER(IDC_DETAIL_FORMAT, BN_CLICKED, OnFormat)
        COMMAND_HANDLER(IDC_DETAIL_LINENUMBERS, BN_CLICKED, OnLineNumbers)
        COMMAND_HANDLER(IDC_DETAIL_UNPARSED, BN_CLICKED, OnShowUnparsed)
        COMMAND_HANDLER(IDC_DETAIL_COPY, BN_CLICKED, OnCopy)
        COMMAND_HANDLER(IDC_DETAIL_FONT, BN_CLICKED, OnFont)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()
    
    // Controls
    CScintillaWrapper m_edit;  // Scintilla editor control
    CQueueGridView m_queueGrid;  // Queue grid control
    CStatic m_statusLine;
    CDetailToolbarButton m_btnFind;
    CDetailToolbarButton m_btnFilter;
    CDetailToolbarButton m_btnCopy;
    CDetailToolbarButton m_btnFont;
    CDetailToolbarToggleButton m_btnWordWrap;
    CDetailToolbarToggleButton m_btnSyntax;
    CDetailToolbarToggleButton m_btnFormat;
    CDetailToolbarToggleButton m_btnLineNumbers;
    CDetailToolbarToggleButton m_btnUnparsed;
    
    // Icons
    CImageList m_imageList;
    
    // State (default to on for new views)
    bool m_wordWrap = true;
    bool m_syntaxHighlight = true;
    bool m_formatContent = true;
    bool m_showLineNumbers = true;
    bool m_showUnparsed = false;  // Default to showing parsed message
    bool m_showingQueue = false;  // Track whether showing Queue grid or text
    std::wstring m_originalText;  // Store original unformatted text
    std::wstring m_unparsedText;  // Store unparsed text (with Clarion metadata)
    std::wstring m_highlightText;  // Current highlighted text
    std::shared_ptr<QueueData> m_currentQueueData;  // Current queue data being displayed
    CMainFrame* m_pMainFrame = nullptr;
    CBrush m_bgBrush;  // Background brush for edit control
    CBrush m_statusBrush;  // Background brush for status line
    CFont m_font;  // Current font for detail pane
    std::unique_ptr<SyntaxHighlighter> m_highlighter;  // Syntax highlighter
    
    // Layout constants
    static const int TOOLBAR_HEIGHT = 30;
    static const int STATUS_HEIGHT = 22;
    static const int BUTTON_WIDTH = 90;  // Increased to accommodate icon + text
    static const int BUTTON_HEIGHT = 24;
    static const int BUTTON_SPACING = 5;

    void SetMainFrame(CMainFrame* pMainFrame) {
        FILE* log = fopen("C:\\Dev\\UltimateDebugViewPP\\prefs_debug.log", "a");
        if (log) {
            fprintf(log, "CDetailTabContainer::SetMainFrame called with %p\n", pMainFrame);
            fclose(log);
        }
        m_pMainFrame = pMainFrame;
        // Update existing Details tabs with the MainFrame pointer
        // This would need to be done after tabs are created, not in the header
        // for (auto& tabView : m_tabViews)
        // {
        //     if (tabView && tabView->GetTabName() == L"Details")
        //     {
        //         // Would need forward declaration or implementation in .cpp
        //     }
        // }
    }
    void InitWordWrapState();
    void InitSyntaxHighlightState();
    void InitFormatState();
    void InitLineNumbersState();
    void InitUnparsedState();
    
    // Getters for current settings
    bool GetWordWrap() const { return m_wordWrap; }
    bool GetSyntaxHighlight() const { return m_syntaxHighlight; }
    bool GetFormatContent() const { return m_formatContent; }
    bool GetLineNumbers() const { return m_showLineNumbers; }
    
    // Setters for initial settings (used when loading from registry)
    void SetWordWrap(bool value) 
    { 
        m_wordWrap = value; 
        if (m_btnWordWrap.m_hWnd)
            m_btnWordWrap.SetToggled(value);
        if (m_edit.m_hWnd)
            m_edit.SetWordWrap(value);
    }
    
    void SetSyntaxHighlight(bool value) 
    { 
        m_syntaxHighlight = value;
        if (m_btnSyntax.m_hWnd)
            m_btnSyntax.SetToggled(value);
    }
    
    void SetFormatContent(bool value)
    {
        m_formatContent = value;
        if (m_btnFormat.m_hWnd)
            m_btnFormat.SetToggled(value);
    }
    
    void SetLineNumbers(bool value)
    {
        m_showLineNumbers = value;
        if (m_btnLineNumbers.m_hWnd)
            m_btnLineNumbers.SetToggled(value);
        if (m_edit.m_hWnd)
            m_edit.ShowLineNumbers(value);
    }
    
    void SetDetailFont(HFONT hFont)
    {
        if (hFont)
        {
            m_font.DeleteObject();
            m_font.Attach(hFont);
            if (m_edit.m_hWnd)
            {
                m_edit.SetFont(m_font);
            }
            // Apply same font to Queue grid
            if (m_queueGrid.m_hWnd)
            {
                m_queueGrid.SetFont(m_font);
            }
        }
    }
    
    LOGFONT GetDetailFont() const
    {
        LOGFONT lf = {0};
        if (m_font.m_hFont)
        {
            m_font.GetLogFont(&lf);
        }
        else
        {
            // Default to Consolas 9pt
            lf.lfHeight = -12;
            wcscpy_s(lf.lfFaceName, L"Consolas");
        }
        return lf;
    }
    
    void RefreshTheme()
    {
        // Delete brushes - they will be recreated in OnCtlColor handlers
        if (m_bgBrush.m_hBrush)
            m_bgBrush.DeleteObject();
        if (m_statusBrush.m_hBrush)
            m_statusBrush.DeleteObject();
        
        // Apply theme colors to Scintilla control
        if (m_edit.m_hWnd)
        {
            bool isDarkTheme = fusion::debugviewpp::ThemeManager::Instance().IsDarkTheme();
            m_edit.ApplyTheme(isDarkTheme);
            
            // Re-apply syntax highlighting if enabled
            if (!m_originalText.empty())
            {
                SetText(m_originalText);
            }
        }
        
        // Force complete redraw including all child windows
        if (m_hWnd)
        {
            InvalidateRect(nullptr, TRUE);
            
            if (m_edit.m_hWnd)
            {
                m_edit.InvalidateRect(nullptr, TRUE);
            }
            if (m_statusLine.m_hWnd)
            {
                m_statusLine.InvalidateRect(nullptr, TRUE);
                m_statusLine.UpdateWindow();
            }
            
            // Force buttons to redraw
            m_btnFind.InvalidateRect(nullptr, TRUE);
            m_btnFilter.InvalidateRect(nullptr, TRUE);
            m_btnCopy.InvalidateRect(nullptr, TRUE);
            m_btnWordWrap.InvalidateRect(nullptr, TRUE);
            
            // Trigger redraw
            RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
        }
    }
    
    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        // Load toolbar icons from mainframe resources
        m_imageList.Create(16, 16, ILC_COLOR32 | ILC_MASK, 8, 4);
        
        // Load main toolbar bitmap and extract individual icons
        CBitmap bmp;
        if (bmp.LoadBitmap(IDR_MAINFRAME))
        {
            m_imageList.Add(bmp, RGB(192, 192, 192));
        }
        
        // Create toolbar buttons grouped on the left
        int x = 8;  // Start with some padding
        
        // Find button - try different icon indices
        RECT rcBtn = {x, 3, x + BUTTON_WIDTH, 3 + BUTTON_HEIGHT};
        m_btnFind.Create(m_hWnd, rcBtn, nullptr, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         0, IDC_DETAIL_FIND);
        m_btnFind.SetText(L"Find");
        HICON hIcon = m_imageList.GetIcon(8);  // Try index 8 for Find (was 18)
        if (hIcon)
            m_btnFind.SetIcon(hIcon);
        x += BUTTON_WIDTH + 2;  // Small spacing between grouped buttons
        
        // Filter button - use Filter icon from toolbar
        rcBtn = {x, 3, x + BUTTON_WIDTH, 3 + BUTTON_HEIGHT};
        m_btnFilter.Create(m_hWnd, rcBtn, nullptr, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                           0, IDC_DETAIL_FILTER);
        m_btnFilter.SetText(L"Filter");
        hIcon = m_imageList.GetIcon(10);  // Try index 10 for Filter (was 19)
        if (hIcon)
            m_btnFilter.SetIcon(hIcon);
        x += BUTTON_WIDTH + 2;
        
        // Copy All button - use same icon as main Copy button (index 2)
        rcBtn = {x, 3, x + BUTTON_WIDTH, 3 + BUTTON_HEIGHT};
        m_btnCopy.Create(m_hWnd, rcBtn, nullptr, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         0, IDC_DETAIL_COPY);
        m_btnCopy.SetText(L"Copy All");
        hIcon = m_imageList.GetIcon(2);  // Use same copy icon as main toolbar (index 2)
        if (hIcon)
            m_btnCopy.SetIcon(hIcon);
        x += BUTTON_WIDTH + 2;
        
        // Font button - use the font/text icon (index 6 which looks like an 'A')
        rcBtn = {x, 3, x + BUTTON_WIDTH, 3 + BUTTON_HEIGHT};
        m_btnFont.Create(m_hWnd, rcBtn, nullptr, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         0, IDC_DETAIL_FONT);
        m_btnFont.SetText(L"Font");
        hIcon = m_imageList.GetIcon(6);  // Use the 'A' text/font icon
        if (hIcon)
            m_btnFont.SetIcon(hIcon);
        
        // Toggle buttons on the right (will be positioned in OnSize)
        RECT rcClient;
        GetClientRect(&rcClient);
        
        // Calculate button positions from right edge
        int rightX = rcClient.right - 8;  // Start from right edge with padding
        
        // Word Wrap toggle button (rightmost) - needs more width for icon + text
        const int WORDWRAP_WIDTH = 85;  // Wider to accommodate icon + "Word Wrap" text
        rightX -= WORDWRAP_WIDTH;
        rcBtn = {rightX, 3, rightX + WORDWRAP_WIDTH, 3 + BUTTON_HEIGHT};
        m_btnWordWrap.Create(m_hWnd, rcBtn, nullptr, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                             0, IDC_DETAIL_WORDWRAP);
        m_btnWordWrap.SetText(L"Word Wrap");
        HICON hWordWrapIcon = (HICON)::LoadImage(::GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_DETAIL_WORDWRAP), 
                                                  IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        if (hWordWrapIcon)
            m_btnWordWrap.SetIcon(hWordWrapIcon);
        
        // Syntax Highlight toggle button
        rightX -= (BUTTON_WIDTH + 5);  // Increased spacing from 2 to 5
        rcBtn = {rightX, 3, rightX + BUTTON_WIDTH, 3 + BUTTON_HEIGHT};
        m_btnSyntax.Create(m_hWnd, rcBtn, nullptr, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                          0, IDC_DETAIL_SYNTAX);
        m_btnSyntax.SetText(L"Syntax");
        HICON hSyntaxIcon = (HICON)::LoadImage(::GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_DETAIL_SYNTAX), 
                                                IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        if (hSyntaxIcon)
            m_btnSyntax.SetIcon(hSyntaxIcon);
        
        // Line Numbers toggle button
        rightX -= (BUTTON_WIDTH + 5);  // Increased spacing from 2 to 5
        rcBtn = {rightX, 3, rightX + BUTTON_WIDTH, 3 + BUTTON_HEIGHT};
        m_btnLineNumbers.Create(m_hWnd, rcBtn, nullptr, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                               0, IDC_DETAIL_LINENUMBERS);
        m_btnLineNumbers.SetText(L"Line #");
        HICON hLineNumIcon = (HICON)::LoadImage(::GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_DETAIL_LINENUMBERS), 
                                                 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        if (hLineNumIcon)
            m_btnLineNumbers.SetIcon(hLineNumIcon);
        
        // Show Unparsed toggle button
        rightX -= (BUTTON_WIDTH + 5);
        rcBtn = {rightX, 3, rightX + BUTTON_WIDTH, 3 + BUTTON_HEIGHT};
        m_btnUnparsed.Create(m_hWnd, rcBtn, nullptr, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                            0, IDC_DETAIL_UNPARSED);
        m_btnUnparsed.SetText(L"Unparsed");
        // Use same icon as Format for now (or create a new icon)
        HICON hUnparsedIcon = (HICON)::LoadImage(::GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_DETAIL_FORMAT), 
                                                 IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        if (hUnparsedIcon)
            m_btnUnparsed.SetIcon(hUnparsedIcon);
        
        // Format toggle button
        rightX -= (BUTTON_WIDTH + 5);  // Increased spacing from 2 to 5
        rcBtn = {rightX, 3, rightX + BUTTON_WIDTH, 3 + BUTTON_HEIGHT};
        m_btnFormat.Create(m_hWnd, rcBtn, nullptr, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                          0, IDC_DETAIL_FORMAT);
        m_btnFormat.SetText(L"Format");
        HICON hFormatIcon = (HICON)::LoadImage(::GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_DETAIL_FORMAT), 
                                                IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        if (hFormatIcon)
            m_btnFormat.SetIcon(hFormatIcon);
        
        // Create status line below toolbar
        GetClientRect(&rcClient);
        RECT rcStatus = {0, TOOLBAR_HEIGHT, rcClient.right, TOOLBAR_HEIGHT + STATUS_HEIGHT};
        m_statusLine.Create(m_hWnd, rcStatus, nullptr, 
                           WS_CHILD | WS_VISIBLE | SS_LEFT | SS_SUNKEN | SS_CENTERIMAGE);
        
        // Set status line font
        m_statusLine.SetFont(AtlGetDefaultGuiFont());
        
        // Create edit control below status line
        CreateEditControl();

        // Create Queue grid control in same position as edit control
        RECT rcGrid;
        GetClientRect(&rcGrid);
        rcGrid.top = TOOLBAR_HEIGHT + STATUS_HEIGHT;
        m_queueGrid.Create(m_hWnd, rcGrid);
        m_queueGrid.ShowWindow(SW_HIDE);  // Initially hidden

        // Apply font to Queue grid if we have one
        if (m_font.m_hFont)
        {
            m_queueGrid.SetFont(m_font);
        }

        return 0;
    }
    
    LRESULT OnShowWindow(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        if (wParam) // Window is being shown
        {
            // Apply theme when window becomes visible
            if (m_edit.m_hWnd)
            {
                bool isDarkTheme = fusion::debugviewpp::ThemeManager::Instance().IsDarkTheme();
                m_edit.ApplyTheme(isDarkTheme);
                m_edit.ConfigurePlainText(isDarkTheme);
                
                // Force refresh
                m_edit.InvalidateRect(nullptr, TRUE);
                m_edit.UpdateWindow();
            }
        }
        return 0;
    }
    
    LRESULT OnSetInitialText(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        // Set the initial text after all initialization is complete
        SetText(L"Select a line to view complete message details...");
        return 0;
    }
    
    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
    {
        int cx = LOWORD(lParam);
        int cy = HIWORD(lParam);
        
        // Reposition toggle buttons to stay on the right
        int rightX = cx - 8;  // Start from right edge with padding
        
        // Word Wrap toggle button (rightmost) - uses wider width
        const int WORDWRAP_WIDTH = 85;  // Same as in OnCreate
        if (m_btnWordWrap.m_hWnd)
        {
            rightX -= WORDWRAP_WIDTH;
            m_btnWordWrap.SetWindowPos(nullptr, rightX, 3, WORDWRAP_WIDTH, BUTTON_HEIGHT,
                                       SWP_NOZORDER | SWP_NOACTIVATE);
            m_btnWordWrap.Invalidate();  // Force repaint after repositioning
        }
        
        // Syntax Highlight toggle button
        if (m_btnSyntax.m_hWnd)
        {
            rightX -= (BUTTON_WIDTH + 5);  // Increased spacing from 2 to 5
            m_btnSyntax.SetWindowPos(nullptr, rightX, 3, BUTTON_WIDTH, BUTTON_HEIGHT,
                                    SWP_NOZORDER | SWP_NOACTIVATE);
            m_btnSyntax.Invalidate();  // Force repaint after repositioning
        }
        
        // Line Numbers toggle button
        if (m_btnLineNumbers.m_hWnd)
        {
            rightX -= (BUTTON_WIDTH + 5);  // Increased spacing from 2 to 5
            m_btnLineNumbers.SetWindowPos(nullptr, rightX, 3, BUTTON_WIDTH, BUTTON_HEIGHT,
                                         SWP_NOZORDER | SWP_NOACTIVATE);
            m_btnLineNumbers.Invalidate();  // Force repaint after repositioning
        }
        
        // Unparsed toggle button
        if (m_btnUnparsed.m_hWnd)
        {
            rightX -= (BUTTON_WIDTH + 5);
            m_btnUnparsed.SetWindowPos(nullptr, rightX, 3, BUTTON_WIDTH, BUTTON_HEIGHT,
                                      SWP_NOZORDER | SWP_NOACTIVATE);
            m_btnUnparsed.Invalidate();
        }
        
        // Format toggle button
        if (m_btnFormat.m_hWnd)
        {
            rightX -= (BUTTON_WIDTH + 5);  // Increased spacing from 2 to 5
            m_btnFormat.SetWindowPos(nullptr, rightX, 3, BUTTON_WIDTH, BUTTON_HEIGHT,
                                    SWP_NOZORDER | SWP_NOACTIVATE);
            m_btnFormat.Invalidate();  // Force repaint after repositioning
        }
        
        if (m_statusLine.m_hWnd)
        {
            // Position status line below toolbar
            m_statusLine.SetWindowPos(nullptr, 0, TOOLBAR_HEIGHT, cx, STATUS_HEIGHT,
                                     SWP_NOZORDER | SWP_NOACTIVATE);
        }
        
        if (m_edit.m_hWnd)
        {
            // Position Scintilla control below status line
            int editTop = TOOLBAR_HEIGHT + STATUS_HEIGHT;
            m_edit.SetWindowPos(nullptr, 0, editTop, cx, cy - editTop,
                               SWP_NOZORDER | SWP_NOACTIVATE);
        }

        if (m_queueGrid.m_hWnd)
        {
            // Position Queue grid in same space as edit control
            int gridTop = TOOLBAR_HEIGHT + STATUS_HEIGHT;
            m_queueGrid.SetWindowPos(nullptr, 0, gridTop, cx, cy - gridTop,
                                    SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;
    }
    
    LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CDCHandle dc((HDC)wParam);
        RECT rect;
        GetClientRect(&rect);
        
        // Fill with theme background
        const auto& colors = fusion::debugviewpp::ThemeManager::Instance().GetColors();
        CBrush brush;
        brush.CreateSolidBrush(colors.controlBackground);
        dc.FillRect(&rect, brush);
        return 1;
    }
    
    LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CPaintDC dc(m_hWnd);
        RECT rcClient;
        GetClientRect(&rcClient);
        
        // Use theme-aware toolbar background
        bool isDarkTheme = fusion::debugviewpp::ThemeManager::Instance().IsDarkTheme();
        
        RECT rcToolbar = {0, 0, rcClient.right, TOOLBAR_HEIGHT};
        CBrush brushToolbar;
        if (isDarkTheme)
        {
            brushToolbar.CreateSolidBrush(RGB(45, 45, 45));  // Dark toolbar background
        }
        else
        {
            brushToolbar.CreateSolidBrush(RGB(245, 245, 245));  // Light gray toolbar
        }
        dc.FillRect(&rcToolbar, brushToolbar);
        
        // Draw a subtle separator line below toolbar
        CPen pen;
        if (isDarkTheme)
        {
            pen.CreatePen(PS_SOLID, 1, RGB(60, 60, 60));  // Darker separator for dark theme
        }
        else
        {
            pen.CreatePen(PS_SOLID, 1, RGB(200, 200, 200));  // Light separator
        }
        CPenHandle oldPen = dc.SelectPen(pen);
        dc.MoveTo(0, TOOLBAR_HEIGHT - 1);
        dc.LineTo(rcClient.right, TOOLBAR_HEIGHT - 1);
        dc.SelectPen(oldPen);
        
        return 0;
    }
    
    void SetText(const std::wstring& text);
    void SetUnparsedText(const std::wstring& unparsed);
    void ApplySyntaxHighlighting(const std::wstring& text);
    void SetHighlightText(const std::wstring& text);  // Set text to highlight in detail pane
    void Refresh();  // Refresh the view with current text and new formatting preferences
    
    void SetStatus(const std::wstring& status)
    {
        if (m_statusLine.m_hWnd)
        {
            // Add padding for better appearance
            std::wstring paddedStatus = L"  " + status;
            m_statusLine.SetWindowText(paddedStatus.c_str());
        }
    }
    
    void SetContent(const std::wstring& status, const std::wstring& content)
    {
        SetStatus(status);
        SetText(content);
    }

    // Queue support
    void SetQueueData(std::shared_ptr<QueueData> queueData);
    void ShowQueueGrid(bool show);
    void UpdateButtonStates();
    bool IsShowingQueue() const { return m_showingQueue; }
    std::shared_ptr<QueueData> GetQueueData() const { return m_currentQueueData; }
    
    LRESULT OnCtlColorBtn(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        // Not needed for toggle buttons
        return 0;
    }
    
    LRESULT OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
    {
        CDCHandle dc((HDC)wParam);
        const auto& colors = fusion::debugviewpp::ThemeManager::Instance().GetColors();
        
        // Handle status line
        if ((HWND)lParam == m_statusLine.m_hWnd)
        {
            dc.SetTextColor(colors.statusBarText);
            dc.SetBkColor(colors.statusBarBackground);
            
            // Delete and recreate status brush to ensure it has correct color
            if (m_statusBrush.m_hBrush)
                m_statusBrush.DeleteObject();
            m_statusBrush.CreateSolidBrush(colors.statusBarBackground);
            
            return (LRESULT)m_statusBrush.m_hBrush;
        }
        
        return 0;
    }
    
    // Button handlers (declarations - implementations in .cpp)
    LRESULT OnFind(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFilter(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnWordWrap(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSyntaxHighlight(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFormat(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnLineNumbers(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnShowUnparsed(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFont(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
    std::wstring ParseErrorContent(const std::wstring& content);
    void CreateEditControl()
    {
        RECT rcClient;
        GetClientRect(&rcClient);
        rcClient.top = TOOLBAR_HEIGHT + STATUS_HEIGHT;  // Edit below both toolbar and status
        
        // Create Scintilla control
        DWORD editStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL;
        m_edit.Create(m_hWnd, rcClient, nullptr, editStyle, WS_EX_CLIENTEDGE);
        
        // Configure Scintilla settings
        m_edit.SetWordWrap(m_wordWrap);
        m_edit.ShowLineNumbers(m_showLineNumbers);
        m_edit.SetReadOnly(false);
        
        // Set font from stored or fallback to Consolas
        if (m_font.m_hFont)
        {
            m_edit.SetFont(m_font);
        }
        else
        {
            // Fallback to Consolas - MainFrame will set the actual font later
            CFont font;
            font.CreatePointFont(90, L"Consolas");
            m_font = std::move(font);
            m_edit.SetFont(m_font);
        }
        
        // Apply the current theme to the Scintilla control
        bool isDarkTheme = fusion::debugviewpp::ThemeManager::Instance().IsDarkTheme();
        m_edit.ApplyTheme(isDarkTheme);
        
        // Configure default plain text style
        m_edit.ConfigurePlainText(isDarkTheme);
        
        // Force a visual refresh to ensure theme is applied immediately
        m_edit.InvalidateRect(nullptr, TRUE);
        m_edit.UpdateWindow();
        
        // Ensure the background is painted immediately  
        m_edit.SendEditor(SCI_COLOURISE, 0, -1);
    }
    
    void RecreateEditControl()
    {
        // Save current text
        std::string currentText = m_edit.GetText();
        
        // Destroy and recreate
        if (m_edit.m_hWnd)
            m_edit.DestroyWindow();
        
        CreateEditControl();
        
        // Restore text
        m_edit.SetText(currentText);
    }
};

// Detail tab container that manages multiple tabbed views
class CDetailTabContainer : public CWindowImpl<CDetailTabContainer>
{
public:
    DECLARE_WND_CLASS(_T("DetailTabContainer"))
    
    CDetailTabContainer();
    ~CDetailTabContainer();
    
    BEGIN_MSG_MAP(CDetailTabContainer)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_FLOATING_WINDOW_CLOSING, OnFloatingWindowClosing)
        NOTIFY_CODE_HANDLER(TCN_SELCHANGE, OnTabSelChange)
        NOTIFY_CODE_HANDLER(TCN_RCLICK, OnTabRightClick)
        COMMAND_ID_HANDLER(ID_TAB_POPOUT, OnPopOut)
        COMMAND_ID_HANDLER(ID_TAB_POPIN, OnPopIn)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()
    
    // Tab management
    int AddTab(std::shared_ptr<IDetailTabView> tabView);
    void RemoveTab(int index);
    void SetActiveTab(int index);
    int GetActiveTab() const { return m_activeTab; }
    int GetTabCount() const { return static_cast<int>(m_tabViews.size()); }
    
    // Access to specific tabs
    std::shared_ptr<IDetailTabView> GetTab(int index) const;
    std::shared_ptr<IDetailTabView> GetActiveTabView() const;
    
    // Access to procedure tabs specifically
    std::shared_ptr<CProcedureFlowTabView> GetProcedureFlowTab() const;
    std::shared_ptr<COpenProceduresTabView> GetOpenProceduresTab() const;
    std::shared_ptr<CProcedureTimelineTabView> GetProcedureTimelineTab() const;
    
    // Set the parent LogView (needed for tracker access)
    void SetParentLogView(std::shared_ptr<CLogView> view) { m_parentLogView = view; }
    ClarionProcedureTracker* GetTracker() const;
    
    // Pass-through methods to active tab (for compatibility with existing CDetailView interface)
    void SetText(const std::wstring& text);
    void SetUnparsedText(const std::wstring& unparsed);
    void SetContent(const std::wstring& status, const std::wstring& content);
    void SetHighlightText(const std::wstring& text);
    void RefreshTheme();
    void Refresh();  // Refresh with current content and new formatting preferences
    void SetMainFrame(CMainFrame* pMainFrame);
    
    // Settings access (delegated to Details tab)
    bool GetWordWrap() const;
    bool GetSyntaxHighlight() const;
    bool GetFormatContent() const;
    bool GetLineNumbers() const;
    void SetWordWrap(bool value);
    void SetSyntaxHighlight(bool value);
    void SetFormatContent(bool value);
    void SetLineNumbers(bool value);
    void SetDetailFont(HFONT hFont);
    
    // Initialize states (delegated to Details tab)
    void InitWordWrapState();
    void InitSyntaxHighlightState();
    void InitFormatState();
    void InitLineNumbersState();
    void InitUnparsedState();
    
    // Pop-out functionality
    void PopOutTab(int index);
    void ReattachTab(std::shared_ptr<IDetailTabView> tabView, int originalIndex);
    void CloseFloatingWindow(CFloatingTabWindow* window);
    bool IsTabPoppedOut(int index) const;
    
    // Update floating windows with new tracker when view changes
    void UpdateFloatingWindowTrackers();
    
    // Update only floating windows in real-time
    void UpdateFloatingWindows();
    void CleanupResources();
    
protected:
    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
    LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
    LRESULT OnFloatingWindowClosing(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnTabSelChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
    LRESULT OnTabRightClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
    LRESULT OnPopOut(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnPopIn(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    
private:
    CTabCtrl m_tabCtrl;
    std::vector<std::shared_ptr<IDetailTabView>> m_tabViews;
    int m_activeTab;
    CMainFrame* m_pMainFrame;
    std::shared_ptr<CLogView> m_parentLogView;
    HFONT m_detailFont = nullptr;  // Store the detail font to apply to tabs when they're created
    
    // Pop-out support
    struct FloatingTabInfo {
        std::shared_ptr<IDetailTabView> tabView;
        std::unique_ptr<CFloatingTabWindow> window;
        int originalIndex;
        std::wstring tabText;  // Store the tab text for restoration
    };
    std::vector<FloatingTabInfo> m_floatingTabs;
    int m_contextMenuTabIndex;  // Tab index for which context menu was shown
    
    void UpdateTabLayout();
    void ShowActiveTab();
    static void RefreshChildWindows(HWND hWnd);
};

// Adapter to make CDetailView compatible with IDetailTabView interface
class CDetailViewTabAdapter : public IDetailTabView
{
public:
    CDetailViewTabAdapter() = default;
    ~CDetailViewTabAdapter() override = default;

    // IDetailTabView interface
    void Create(HWND parent, const RECT& rc) override;
    HWND GetHWND() override { return m_detailView.m_hWnd; }
    void SetVisible(bool visible) override;
    void Resize(const RECT& rc) override;
    void RefreshTheme() override { m_detailView.RefreshTheme(); }
    std::wstring GetTabName() const override { return L"Details"; }

    // Access to the actual detail view for existing functionality
    CDetailView& GetDetailView() { return m_detailView; }
    const CDetailView& GetDetailView() const { return m_detailView; }

private:
    CDetailView m_detailView;
};
class CProcedureFlowTabView : public IDetailTabView, public CWindowImpl<CProcedureFlowTabView>
{
public:
    DECLARE_WND_CLASS(_T("ProcedureFlowTabView"))
    
    CProcedureFlowTabView() : m_tracker(nullptr), m_highlightedRow(-1), m_selectedRow(-1) {}
    ~CProcedureFlowTabView() override = default;
    
    void Create(HWND parent, const RECT& rc) override;
    HWND GetHWND() override { return m_hWnd; }
    void SetVisible(bool visible) override;
    void Resize(const RECT& rc) override;
    void RefreshTheme() override;
    std::wstring GetTabName() const override { return L"Procedure Flow"; }
    
    // Set the tracker to display data from
    void SetTracker(ClarionProcedureTracker* tracker);
    ClarionProcedureTracker* GetTracker() const { return m_tracker; }
    
    // Update display
    void UpdateDisplay();
    
    // Recreate ListView if it was destroyed
    void RecreateListView();
    
    BEGIN_MSG_MAP(CProcedureFlowTabView)
        NOTIFY_CODE_HANDLER(NM_CLICK, OnListClick)
        NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, OnCustomDraw)
        NOTIFY_CODE_HANDLER(LVN_COLUMNCLICK, OnColumnClick)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()
    
private:
    CListViewCtrl m_listView;
    ClarionProcedureTracker* m_tracker;
    int m_splitterPosPct = 50;
    CMainFrame* m_pMainFrame = nullptr;

    void ApplySplitterPos();
    CImageList m_imageList;
    int m_highlightedRow;
    int m_selectedRow;
    
    void InitializeListView();
    void PopulateFromTracker();
    
    LRESULT OnListClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnColumnClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    enum { 
        ID_PROCEDURE_CLEAR = 30001,
        ID_PROCEDURE_EXPAND_ALL = 30002, 
        ID_PROCEDURE_COLLAPSE_ALL = 30003
    };
};

class COpenProceduresTabView : public IDetailTabView, public CWindowImpl<COpenProceduresTabView>
{
public:
    DECLARE_WND_CLASS(_T("OpenProceduresTabView"))

    COpenProceduresTabView() : m_tracker(nullptr) {}
    ~COpenProceduresTabView() override = default;

    void Create(HWND parent, const RECT& rc) override;
    HWND GetHWND() override { return m_hWnd; }
    void SetVisible(bool visible) override;
    void SetMainFrame(CMainFrame* mainFrame);
    void SetSplitterPosPct(int pct);
    int GetSplitterPosPct() const;
    void Resize(const RECT& rc) override;
    void RefreshTheme() override;
    std::wstring GetTabName() const override { return L"Procedure Stats"; }

    // Set the tracker to display data from
    void SetTracker(ClarionProcedureTracker* tracker);
    ClarionProcedureTracker* GetTracker() const { return m_tracker; }

    // Update display
    void UpdateDisplay();

    // Recreate ListView if it was destroyed
    void RecreateListView();

    BEGIN_MSG_MAP(COpenProceduresTabView)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

private:
    CSplitterWindow m_splitter;           // Vertical splitter
    CListViewCtrl m_currentlyOpenView;    // Top grid: currently open procedures
    CListViewCtrl m_procedureStatsView;   // Bottom grid: procedure statistics
    ClarionProcedureTracker* m_tracker;
    int m_splitterPosPct = 50;
    CMainFrame* m_pMainFrame = nullptr;

    // Sorting state
    int m_currentlyOpenSortColumn = 0;
    bool m_currentlyOpenSortAscending = true;
    int m_procedureStatsSortColumn = 0;
    bool m_procedureStatsSortAscending = true;

    void InitializeViews();
    void PopulateFromTracker();
    void SortCurrentlyOpenView(int column);
    void SortProcedureStatsView(int column);
    void ApplySplitterPos();
};
class CProcedureTimelineTabView : public IDetailTabView, public CWindowImpl<CProcedureTimelineTabView>
{
public:
    DECLARE_WND_CLASS(_T("ProcedureTimelineTabView"))

    CProcedureTimelineTabView() : m_tracker(nullptr) {}
    ~CProcedureTimelineTabView() override = default;

    void Create(HWND parent, const RECT& rc) override;
    HWND GetHWND() override { return m_hWnd; }
    void SetVisible(bool visible) override;
    void Resize(const RECT& rc) override;
    void RefreshTheme() override;
    std::wstring GetTabName() const override { return L"Timeline"; }

    // Set the tracker to display data from
    void SetTracker(ClarionProcedureTracker* tracker);
    ClarionProcedureTracker* GetTracker() const { return m_tracker; }

    // Update display
    void UpdateDisplay();

    // Recreate TimelineView if it was destroyed
    void RecreateTimelineView();

    BEGIN_MSG_MAP(CProcedureTimelineTabView)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

private:
    ::debugviewpp::ClarionProcedureTimelineView m_timelineView;
    ClarionProcedureTracker* m_tracker;

    void InitializeTimelineView();
    void PopulateFromTracker();
};

class CLogViewTabItem2 : public CTabViewTabItem
{
public:
    ~CLogViewTabItem2() override;
    void SetView(std::shared_ptr<CLogView> pView);
    CLogView& GetView();
    void Cleanup();  // Call before deletion to properly clean up windows

    void Create(HWND parent);
    std::wstring GetTitle() const;
    void SetMainFrame(CMainFrame* pMainFrame) { 
        m_pMainFrame = pMainFrame;
        m_detailTabContainer.SetMainFrame(pMainFrame); 
        m_detailTabContainer.InitWordWrapState();
        m_detailTabContainer.InitSyntaxHighlightState();
        m_detailTabContainer.InitFormatState();
        m_detailTabContainer.InitLineNumbersState();
        m_detailTabContainer.InitUnparsedState();
    }
    HWND GetLogViewParent() { return m_top; }
    
    void RefreshTheme()
    {
        // Refresh detail view theme
        m_detailTabContainer.RefreshTheme();
    }
    
    void SetDetailFont(HFONT hFont)
    {
        // Pass font to detail view
        m_detailTabContainer.SetDetailFont(hFont);
    }
    
    void ShowDetailPane(bool show);
    bool IsDetailPaneVisible() const { return m_detailPaneVisible; }
    void UpdateDetailView(const std::wstring& text);
    void UpdateDetailView(const std::wstring& status, const std::wstring& content, const std::wstring& unparsed = L"");
    void StoreDetailContent(const std::wstring& status, const std::wstring& content);
    void RestoreDetailContent();
    void RefreshDetailView();  // Refresh detail view with current content and new formatting preferences
    
    int GetSplitterPos() const { return m_splitterPos; }
    void SetSplitterPos(int pos);
    void UpdateSplitterPos();
    
    // Getters for detail pane settings
    bool GetDetailWordWrap() const { return m_detailTabContainer.GetWordWrap(); }
    bool GetDetailSyntaxHighlight() const { return m_detailTabContainer.GetSyntaxHighlight(); }
    bool GetDetailFormatContent() const { return m_detailTabContainer.GetFormatContent(); }
    bool GetDetailLineNumbers() const { return m_detailTabContainer.GetLineNumbers(); }
    
    // Setters for detail pane settings
    void SetDetailWordWrap(bool value) { m_detailTabContainer.SetWordWrap(value); }
    void SetDetailSyntaxHighlight(bool value) { m_detailTabContainer.SetSyntaxHighlight(value); }
    void SetDetailFormatContent(bool value) { m_detailTabContainer.SetFormatContent(value); }
    void SetDetailLineNumbers(bool value) { m_detailTabContainer.SetLineNumbers(value); }
    
    // Clear highlighting in detail pane
    void ClearDetailHighlight() { m_detailTabContainer.SetHighlightText(L""); }
    
    // Access to detail tab container for advanced functionality
    CDetailTabContainer& GetDetailTabContainer() { return m_detailTabContainer; }
    const CDetailTabContainer& GetDetailTabContainer() const { return m_detailTabContainer; }
    
    // Update procedure tabs with current view's tracker
    void UpdateProcedureTabs();
    
    // Update floating windows with new tracker when view changes
    void UpdateFloatingWindowTrackers();

    // Update only floating windows in real-time
    void UpdateFloatingWindows();
    // Update Queue tab with queue data
    void UpdateQueueView(std::shared_ptr<QueueData> queueData);

private:
    CMainFrame* m_pMainFrame = nullptr;
    std::shared_ptr<CLogView> m_pView;
    CHorSplitterWindow m_split;
    CMyPaneContainer m_top;
    CMyPaneContainer m_bottom;
    
    // Detail view components - replaced with tabbed container
    CDetailTabContainer m_detailTabContainer;
    std::shared_ptr<CDetailViewTabAdapter> m_detailViewAdapter;  // Access to Details tab
    std::shared_ptr<CHelpTabView> m_helpTabView;  // Permanent Help tab
    bool m_detailPaneVisible = false;  // Default to hidden on first run
    int m_splitterPos = 75; // percentage
    
    // Store last detail content for this tab
    std::wstring m_lastDetailStatus;
    std::wstring m_lastDetailContent;
};


} // namespace debugviewpp
} // namespace fusion









