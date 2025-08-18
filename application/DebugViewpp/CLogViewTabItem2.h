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
        
        // Only draw hover/press effects as overlay
        if (m_isHovered || m_isPressed)
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
        
        // Draw icon
        if (m_hIcon)
        {
            dc.DrawIconEx(x, y, m_hIcon, iconSize, iconSize, 0, NULL, DI_NORMAL);
            x += iconSize + padding;
        }
        
        // Draw text
        if (!m_text.empty())
        {
            dc.SetBkMode(TRANSPARENT);
            if (isDarkTheme)
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
        if (!m_isHovered)
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
        m_isPressed = true;
        Invalidate();
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
        
        // Draw toggle state background
        if (m_isToggled)
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
        // Draw hover/press effects as overlay
        else if (m_isHovered || m_isPressed)
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
        
        // Draw icon
        if (m_hIcon)
        {
            dc.DrawIconEx(x, y, m_hIcon, iconSize, iconSize, 0, NULL, DI_NORMAL);
            x += iconSize + padding;
        }
        
        // Draw text
        if (!m_text.empty())
        {
            dc.SetBkMode(TRANSPARENT);
            if (isDarkTheme)
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
        COMMAND_HANDLER(IDC_DETAIL_COPY, BN_CLICKED, OnCopy)
        COMMAND_HANDLER(IDC_DETAIL_FONT, BN_CLICKED, OnFont)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()
    
    // Controls
    CScintillaWrapper m_edit;  // Scintilla editor control
    CStatic m_statusLine;
    CDetailToolbarButton m_btnFind;
    CDetailToolbarButton m_btnFilter;
    CDetailToolbarButton m_btnCopy;
    CDetailToolbarButton m_btnFont;
    CDetailToolbarToggleButton m_btnWordWrap;
    CDetailToolbarToggleButton m_btnSyntax;
    CDetailToolbarToggleButton m_btnFormat;
    CDetailToolbarToggleButton m_btnLineNumbers;
    
    // Icons
    CImageList m_imageList;
    
    // State (default to on for new views)
    bool m_wordWrap = true;
    bool m_syntaxHighlight = true;
    bool m_formatContent = true;
    bool m_showLineNumbers = true;
    std::wstring m_originalText;  // Store original unformatted text
    std::wstring m_highlightText;  // Current highlighted text
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
    
    void SetMainFrame(CMainFrame* pMainFrame) { m_pMainFrame = pMainFrame; }
    void InitWordWrapState();
    void InitSyntaxHighlightState();
    void InitFormatState();
    void InitLineNumbersState();
    
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
        const auto& colors = fusion::debugviewpp::ThemeManager::Instance().GetColors();
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
    void ApplySyntaxHighlighting(const std::wstring& text);
    void SetHighlightText(const std::wstring& text);  // Set text to highlight in detail pane
    
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
    LRESULT OnCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFont(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
private:
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

class CLogViewTabItem2 : public CTabViewTabItem
{
public:
    ~CLogViewTabItem2() override;
    void SetView(std::shared_ptr<CLogView> pView);
    CLogView& GetView();

    void Create(HWND parent);
    void SetMainFrame(CMainFrame* pMainFrame) { 
        m_pMainFrame = pMainFrame;
        m_detailView.SetMainFrame(pMainFrame); 
        m_detailView.InitWordWrapState();
        m_detailView.InitSyntaxHighlightState();
        m_detailView.InitFormatState();
        m_detailView.InitLineNumbersState();
    }
    HWND GetLogViewParent() { return m_top; }
    
    void RefreshTheme()
    {
        // Refresh detail view theme
        m_detailView.RefreshTheme();
    }
    
    void SetDetailFont(HFONT hFont)
    {
        // Pass font to detail view
        m_detailView.SetDetailFont(hFont);
    }
    
    void ShowDetailPane(bool show);
    bool IsDetailPaneVisible() const { return m_detailPaneVisible; }
    void UpdateDetailView(const std::wstring& text);
    void UpdateDetailView(const std::wstring& status, const std::wstring& content);
    void StoreDetailContent(const std::wstring& status, const std::wstring& content);
    void RestoreDetailContent();
    
    int GetSplitterPos() const { return m_splitterPos; }
    void SetSplitterPos(int pos);
    void UpdateSplitterPos();
    
    // Getters for detail pane settings
    bool GetDetailWordWrap() const { return m_detailView.GetWordWrap(); }
    bool GetDetailSyntaxHighlight() const { return m_detailView.GetSyntaxHighlight(); }
    bool GetDetailFormatContent() const { return m_detailView.GetFormatContent(); }
    bool GetDetailLineNumbers() const { return m_detailView.GetLineNumbers(); }
    
    // Setters for detail pane settings
    void SetDetailWordWrap(bool value) { m_detailView.SetWordWrap(value); }
    void SetDetailSyntaxHighlight(bool value) { m_detailView.SetSyntaxHighlight(value); }
    void SetDetailFormatContent(bool value) { m_detailView.SetFormatContent(value); }
    void SetDetailLineNumbers(bool value) { m_detailView.SetLineNumbers(value); }
    
    // Clear highlighting in detail pane
    void ClearDetailHighlight() { m_detailView.SetHighlightText(L""); }

private:
    CMainFrame* m_pMainFrame = nullptr;
    std::shared_ptr<CLogView> m_pView;
    CHorSplitterWindow m_split;
    CMyPaneContainer m_top;
    CMyPaneContainer m_bottom;
    
    // Detail view components
    CDetailView m_detailView;
    bool m_detailPaneVisible = false;  // Default to hidden on first run
    int m_splitterPos = 75; // percentage
    
    // Store last detail content for this tab
    std::wstring m_lastDetailStatus;
    std::wstring m_lastDetailContent;
};


} // namespace debugviewpp
} // namespace fusion
