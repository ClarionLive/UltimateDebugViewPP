// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "atleverything.h"
#include "FloatingMainTabWindow.h"
#include "CLogViewTabItem2.h"
#include "MainFrame.h"
#include "LogView.h"
#include "DebugViewppLib/ThemeManager.h"
#include "resource.h"

namespace fusion {
namespace debugviewpp {

CFloatingMainTabWindow::CFloatingMainTabWindow(CMainFrame* mainFrame, CLogViewTabItem2* tabItem, int originalIndex)
    : m_mainFrame(mainFrame)
    , m_tabItem(tabItem)
    , m_originalIndex(originalIndex)
{
    LoadWindowStateFromRegistry();
}

CFloatingMainTabWindow::~CFloatingMainTabWindow()
{
    SaveWindowStateToRegistry();
}

LRESULT CFloatingMainTabWindow::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // Set window icon
    SetIcon(AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON)));
    SetIcon(AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON)), FALSE);
    
    // Create toolbar with proper style matching the main application
    m_toolbar.Create(m_hWnd, nullptr, nullptr, 
        ATL_SIMPLE_TOOLBAR_PANE_STYLE | TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
        0, ATL_IDW_TOOLBAR);
    
    // Set button structure size
    m_toolbar.SetButtonStructSize(sizeof(TBBUTTON));
    
    // Add toolbar button with text and proper sizing
    TBBUTTON buttons[] = {
        { I_IMAGENONE, ID_FLOATING_STAY_ON_TOP, TBSTATE_ENABLED, BTNS_CHECK | BTNS_SHOWTEXT | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)L"Stay On Top" }
    };
    
    m_toolbar.AddButtons(1, buttons);
    
    // Size the toolbar to fit content
    m_toolbar.AutoSize();
    
    // Position the toolbar at the top
    RECT rcToolbar;
    m_toolbar.GetWindowRect(&rcToolbar);
    int toolbarHeight = rcToolbar.bottom - rcToolbar.top;
    ::SetWindowPos(m_toolbar, nullptr, 0, 0, 0, toolbarHeight, SWP_NOSIZE | SWP_NOZORDER);
    
    // Create menu bar with Pop In option
    CreateMenuBar();
    
    // Get the tab name before reparenting
    if (m_tabItem)
    {
        m_tabName = m_tabItem->GetTitle();
        
        // Store the original parent
        m_originalParent = m_tabItem->GetLogViewParent();
        
        // Get the entire tab content window (which contains splitter, LogView, and detail pane)
        HWND hTabContent = m_tabItem->GetLogViewParent();
        if (hTabContent && ::IsWindow(hTabContent))
        {
            // Reparent the entire tab content to this floating window
            ::SetParent(hTabContent, m_hWnd);
            
            // Get client area and resize the tab content to fit (accounting for toolbar)
            RECT rcClient;
            GetClientRect(&rcClient);
            
            // Position the tab content below the toolbar
            ::SetWindowPos(hTabContent, nullptr, 0, TOOLBAR_HEIGHT, 
                          rcClient.right - rcClient.left, 
                          rcClient.bottom - rcClient.top - TOOLBAR_HEIGHT,
                          SWP_NOZORDER | SWP_SHOWWINDOW);
            
            // Make sure everything is visible
            ::ShowWindow(hTabContent, SW_SHOW);
            
            // Force a complete refresh of the window hierarchy
            ::InvalidateRect(hTabContent, nullptr, TRUE);
            ::UpdateWindow(hTabContent);
            
            // Refresh all child windows recursively
            auto refreshChildren = [](HWND hChild, LPARAM) -> BOOL
            {
                ::ShowWindow(hChild, SW_SHOW);
                ::InvalidateRect(hChild, nullptr, TRUE);
                ::UpdateWindow(hChild);
                return TRUE;
            };
            ::EnumChildWindows(hTabContent, refreshChildren, 0);
            
            // Force LogView to refresh its content
            CLogView& logView = m_tabItem->GetView();
            logView.Invalidate();
            logView.UpdateWindow();
            
            // If detail pane is visible, make sure it's refreshed too
            if (m_tabItem->IsDetailPaneVisible())
            {
                m_tabItem->RestoreDetailContent();
            }
        }
        
        // Set window title
        std::wstring title = L"DebugView++ - " + m_tabName;
        SetWindowText(title.c_str());
    }
    
    // Restore window state
    RestoreWindowState();

    // Apply Stay On Top by default
    if (m_stayOnTop)
    {
        SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        if (m_toolbar.IsWindow())
        {
            m_toolbar.CheckButton(ID_FLOATING_STAY_ON_TOP, TRUE);
        }
    }

    return 0;
}

LRESULT CFloatingMainTabWindow::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    int cx = LOWORD(lParam);
    int cy = HIWORD(lParam);
    
    // Resize toolbar
    if (m_toolbar.IsWindow())
    {
        m_toolbar.SetWindowPos(nullptr, 0, 0, cx, TOOLBAR_HEIGHT, SWP_NOZORDER);
    }
    
    // Resize tab content (accounting for toolbar)
    if (m_tabItem)
    {
        HWND hTabContent = m_tabItem->GetLogViewParent();
        if (hTabContent && ::IsWindow(hTabContent))
        {
            ::SetWindowPos(hTabContent, nullptr, 0, TOOLBAR_HEIGHT, cx, cy - TOOLBAR_HEIGHT, 
                          SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    
    return 0;
}

LRESULT CFloatingMainTabWindow::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // Save window state before closing
    SaveWindowState();
    
    // Post a message to the main frame for deferred cleanup
    if (m_mainFrame && m_tabItem)
    {
        // Send the window pointer and tab info to main frame for cleanup
        ::PostMessage(m_mainFrame->m_hWnd, WM_FLOATING_MAIN_WINDOW_CLOSING, 
                      reinterpret_cast<WPARAM>(this), 0);
    }
    
    // Hide the window immediately for better user experience
    ShowWindow(SW_HIDE);
    
    // Don't destroy here - let the main frame handle it
    return 0;
}

LRESULT CFloatingMainTabWindow::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // The tab item will be managed by the main frame when reattached
    // Don't destroy it here
    return 0;
}

LRESULT CFloatingMainTabWindow::OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
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

LRESULT CFloatingMainTabWindow::OnSysCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Handle minimize to save state
    if ((wParam & 0xFFF0) == SC_MINIMIZE || (wParam & 0xFFF0) == SC_MAXIMIZE || (wParam & 0xFFF0) == SC_RESTORE)
    {
        SaveWindowState();
    }
    
    bHandled = FALSE;
    return 0;
}

LRESULT CFloatingMainTabWindow::OnGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    MINMAXINFO* pMMI = (MINMAXINFO*)lParam;
    pMMI->ptMinTrackSize.x = MIN_WIDTH;
    pMMI->ptMinTrackSize.y = MIN_HEIGHT;
    return 0;
}

LRESULT CFloatingMainTabWindow::OnPopIn(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // Same as closing - reattach the tab
    PostMessage(WM_CLOSE);
    return 0;
}

void CFloatingMainTabWindow::CreateMenuBar()
{
    CMenuHandle menu;
    menu.CreateMenu();
    
    CMenuHandle viewMenu;
    viewMenu.CreatePopupMenu();
    viewMenu.AppendMenu(MF_STRING, ID_FLOATING_MAIN_POPIN, L"&Pop In\tCtrl+Shift+P");
    
    menu.AppendMenu(MF_POPUP, (UINT_PTR)viewMenu.m_hMenu, L"&View");
    
    SetMenu(menu);
}

void CFloatingMainTabWindow::SaveWindowState()
{
    if (!m_hWnd || !::IsWindow(m_hWnd))
        return;
    
    // Get window placement
    WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
    if (GetWindowPlacement(&wp))
    {
        m_windowState.rect = wp.rcNormalPosition;
        m_windowState.maximized = (wp.showCmd == SW_SHOWMAXIMIZED);
        m_windowState.valid = true;
    }
}

void CFloatingMainTabWindow::RestoreWindowState()
{
    if (!m_windowState.valid)
        return;
    
    // Ensure the window is visible on screen
    RECT rcWork;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);
    
    RECT& rc = m_windowState.rect;
    if (rc.right <= rcWork.left || rc.left >= rcWork.right ||
        rc.bottom <= rcWork.top || rc.top >= rcWork.bottom)
    {
        // Window is off-screen, use default position
        rc = {100, 100, 900, 700};
    }
    
    SetWindowPos(nullptr, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 
                 SWP_NOZORDER | SWP_NOACTIVATE);
    
    if (m_windowState.maximized)
    {
        ShowWindow(SW_SHOWMAXIMIZED);
    }
}

void CFloatingMainTabWindow::RefreshTheme()
{
    if (m_tabItem)
    {
        m_tabItem->RefreshTheme();
        
        // Also refresh the LogView
        CLogView& logView = m_tabItem->GetView();
        logView.Invalidate();
        logView.UpdateWindow();
    }
    
    // Refresh window background
    if (m_hWnd)
    {
        Invalidate();
        UpdateWindow();
    }
}

void CFloatingMainTabWindow::LoadWindowStateFromRegistry()
{
    // Load from registry if available
    // For now, use default values - could be enhanced to save/restore per-tab window states
    m_windowState.rect = {100, 100, 900, 700};
    m_windowState.maximized = false;
    m_windowState.valid = true;
}

void CFloatingMainTabWindow::SaveWindowStateToRegistry()
{
    // Save to registry if needed
    // For now, just ensure state is saved in memory
    SaveWindowState();
}

std::wstring CFloatingMainTabWindow::GetRegistryKey() const
{
    return L"FloatingMainWindows\\" + m_tabName;
}

LRESULT CFloatingMainTabWindow::OnStayOnTop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // Toggle stay on top state
    m_stayOnTop = !m_stayOnTop;
    
    // Update window z-order
    SetWindowPos(m_stayOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 
                 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    
    // Update toolbar button state
    if (m_toolbar.IsWindow())
    {
        m_toolbar.CheckButton(ID_FLOATING_STAY_ON_TOP, m_stayOnTop);
    }
    
    return 0;
}

} // namespace debugviewpp
} // namespace fusion