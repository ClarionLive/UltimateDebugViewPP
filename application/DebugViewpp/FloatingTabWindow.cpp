// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "atleverything.h"
#include "FloatingTabWindow.h"
#include "CLogViewTabItem2.h"
#include "DebugViewppLib/ThemeManager.h"
#include "resource.h"

namespace fusion {
namespace debugviewpp {

CFloatingTabWindow::CFloatingTabWindow(CDetailTabContainer* parent, std::shared_ptr<IDetailTabView> tabView, int originalIndex, ClarionProcedureTracker* tracker, const std::wstring& viewName)
    : m_parentContainer(parent)
    , m_tabView(tabView)
    , m_originalIndex(originalIndex)
    , m_tracker(tracker)
    , m_viewName(viewName)
{
    LoadWindowStateFromRegistry();
}

CFloatingTabWindow::~CFloatingTabWindow()
{
    SaveWindowStateToRegistry();
}

LRESULT CFloatingTabWindow::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
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
    
    // Reparent the existing tab view to this floating window
    if (m_tabView && m_tabView->GetHWND())
    {
        // Reparent the existing window to this floating window
        HWND hTabWnd = m_tabView->GetHWND();
        ::SetParent(hTabWnd, m_hWnd);
        
        // Get client area and resize the tab view to fit (accounting for toolbar)
        RECT rcClient;
        GetClientRect(&rcClient);
        rcClient.top += TOOLBAR_HEIGHT;
        m_tabView->Resize(rcClient);
        
        // Set window title to include view name and tab name
        std::wstring title = L"DebugView++ - " + m_tabView->GetTabName();
        if (!m_viewName.empty())
        {
            title += L" [" + m_viewName + L"]";
        }
        SetWindowText(title.c_str());
        
        // Post a message to handle visibility and content update after window creation completes
        PostMessage(WM_FLOATING_WINDOW_SHOW_CONTENT);
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

LRESULT CFloatingTabWindow::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    int cx = LOWORD(lParam);
    int cy = HIWORD(lParam);
    
    // Resize toolbar
    if (m_toolbar.IsWindow())
    {
        m_toolbar.SetWindowPos(nullptr, 0, 0, cx, TOOLBAR_HEIGHT, SWP_NOZORDER);
    }
    
    // Resize tab view (accounting for toolbar)
    if (m_tabView && m_tabView->GetHWND())
    {
        RECT rcTab = {0, TOOLBAR_HEIGHT, cx, cy};
        m_tabView->Resize(rcTab);
    }
    
    return 0;
}

LRESULT CFloatingTabWindow::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // Save window state before closing
    SaveWindowState();
    
    // Post a message to the parent for deferred cleanup
    // This avoids destroying the window while we're still in its message handler
    if (m_parentContainer && m_tabView)
    {
        // Send the window pointer and tab info to parent for cleanup
        ::PostMessage(m_parentContainer->m_hWnd, WM_FLOATING_WINDOW_CLOSING, 
                      reinterpret_cast<WPARAM>(this), 0);
    }
    
    // Hide the window immediately for better user experience
    ShowWindow(SW_HIDE);
    
    // Don't destroy here - let the parent handle it
    return 0;
}

LRESULT CFloatingTabWindow::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // The tab view will be managed by the parent container when reattached
    // Don't destroy it here
    return 0;
}

LRESULT CFloatingTabWindow::OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
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

LRESULT CFloatingTabWindow::OnSysCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Handle minimize to save state
    if ((wParam & 0xFFF0) == SC_MINIMIZE || (wParam & 0xFFF0) == SC_MAXIMIZE || (wParam & 0xFFF0) == SC_RESTORE)
    {
        SaveWindowState();
    }
    
    bHandled = FALSE;
    return 0;
}

LRESULT CFloatingTabWindow::OnGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    MINMAXINFO* pMMI = (MINMAXINFO*)lParam;
    pMMI->ptMinTrackSize.x = MIN_WIDTH;
    pMMI->ptMinTrackSize.y = MIN_HEIGHT;
    return 0;
}

LRESULT CFloatingTabWindow::OnShowContent(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (!m_tabView || !m_tabView->GetHWND())
        return 0;
        
    HWND hTabWnd = m_tabView->GetHWND();
    
    // Force the tab view and all its children to be visible
    ::ShowWindow(hTabWnd, SW_SHOW);
    m_tabView->SetVisible(true);
    
    // Force all child windows to be visible
    ::EnumChildWindows(hTabWnd, [](HWND hChild, LPARAM) -> BOOL {
        ::ShowWindow(hChild, SW_SHOW);
        ::InvalidateRect(hChild, nullptr, TRUE);
        ::UpdateWindow(hChild);
        return TRUE;
    }, 0);
    
    // Handle procedure tabs specially
    std::wstring tabName = m_tabView->GetTabName();
    if (tabName == L"Procedure Flow" || tabName == L"Procedure Stats")
    {
        if (auto* procFlow = dynamic_cast<CProcedureFlowTabView*>(m_tabView.get()))
        {
            // Set the tracker if we have one
            if (m_tracker)
            {
                procFlow->SetTracker(m_tracker);
            }
            // Force update and show
            procFlow->UpdateDisplay();
            
            // Explicitly show the ListView
            HWND hListView = ::GetWindow(hTabWnd, GW_CHILD);
            if (hListView)
            {
                ::ShowWindow(hListView, SW_SHOW);
                ::InvalidateRect(hListView, nullptr, TRUE);
                ::UpdateWindow(hListView);
            }
        }
        else if (auto* openProc = dynamic_cast<COpenProceduresTabView*>(m_tabView.get()))
        {
            if (m_tracker)
            {
                openProc->SetTracker(m_tracker);
            }
            openProc->UpdateDisplay();
            
            // Explicitly show the ListView
            HWND hListView = ::GetWindow(hTabWnd, GW_CHILD);
            if (hListView)
            {
                ::ShowWindow(hListView, SW_SHOW);
                ::InvalidateRect(hListView, nullptr, TRUE);
                ::UpdateWindow(hListView);
            }
        }
    }
    
    // Final refresh
    ::RedrawWindow(hTabWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    
    return 0;
}

LRESULT CFloatingTabWindow::OnPopIn(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // Same as closing - reattach the tab
    PostMessage(WM_CLOSE);
    return 0;
}

LRESULT CFloatingTabWindow::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (!m_tabView)
    {
        return 0;
    }
    
    std::wstring tabName = m_tabView->GetTabName();
    
    // Check if this is a Procedure Flow or Procedure Stats tab
    if (tabName == L"Procedure Flow" || tabName == L"Procedure Stats")
    {
        // Cast to the specific tab type and force update
        if (auto* procFlow = dynamic_cast<CProcedureFlowTabView*>(m_tabView.get()))
        {
            // Force the display to update
            procFlow->UpdateDisplay();
            
            // Also force the window to repaint
            if (procFlow->GetHWND())
            {
                ::InvalidateRect(procFlow->GetHWND(), nullptr, TRUE);
                ::UpdateWindow(procFlow->GetHWND());
                
                // Enumerate and refresh child windows (ListView)
                auto refreshChild = [](HWND hChild, LPARAM) -> BOOL
                {
                    ::InvalidateRect(hChild, nullptr, TRUE);
                    ::UpdateWindow(hChild);
                    return TRUE;
                };
                ::EnumChildWindows(procFlow->GetHWND(), refreshChild, 0);
            }
        }
        else if (auto* openProc = dynamic_cast<COpenProceduresTabView*>(m_tabView.get()))
        {
            // Force the display to update
            openProc->UpdateDisplay();
            
            // Also force the window to repaint
            if (openProc->GetHWND())
            {
                ::InvalidateRect(openProc->GetHWND(), nullptr, TRUE);
                ::UpdateWindow(openProc->GetHWND());
                
                // Enumerate and refresh child windows (ListView)
                auto refreshChild = [](HWND hChild, LPARAM) -> BOOL
                {
                    ::InvalidateRect(hChild, nullptr, TRUE);
                    ::UpdateWindow(hChild);
                    return TRUE;
                };
                ::EnumChildWindows(openProc->GetHWND(), refreshChild, 0);
            }
        }
    }
    else
    {
        // For other tabs, just refresh the view
        if (m_tabView->GetHWND())
        {
            m_tabView->RefreshTheme();
            ::InvalidateRect(m_tabView->GetHWND(), nullptr, TRUE);
            ::UpdateWindow(m_tabView->GetHWND());
        }
    }
    
    return 0;
}

void CFloatingTabWindow::CreateMenuBar()
{
    CMenuHandle menu;
    menu.CreateMenu();
    
    CMenuHandle viewMenu;
    viewMenu.CreatePopupMenu();
    viewMenu.AppendMenu(MF_STRING, ID_FLOATING_REFRESH, L"&Refresh\tF5");
    viewMenu.AppendMenu(MF_SEPARATOR);
    viewMenu.AppendMenu(MF_STRING, ID_FLOATING_POPIN, L"&Pop In\tCtrl+P");
    
    menu.AppendMenu(MF_POPUP, (UINT_PTR)viewMenu.m_hMenu, L"&View");
    
    SetMenu(menu);
}

void CFloatingTabWindow::SaveWindowState()
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

void CFloatingTabWindow::RestoreWindowState()
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
        rc = {100, 100, 800, 600};
    }
    
    SetWindowPos(nullptr, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 
                 SWP_NOZORDER | SWP_NOACTIVATE);
    
    if (m_windowState.maximized)
    {
        ShowWindow(SW_SHOWMAXIMIZED);
    }
}

void CFloatingTabWindow::RefreshTheme()
{
    if (m_tabView)
    {
        m_tabView->RefreshTheme();
    }
    
    // Refresh window background
    if (m_hWnd)
    {
        Invalidate();
        UpdateWindow();
    }
}

void CFloatingTabWindow::SetViewName(const std::wstring& viewName)
{
    m_viewName = viewName;
    
    // Update window title
    if (m_hWnd && m_tabView)
    {
        std::wstring title = L"DebugView++ - " + m_tabView->GetTabName();
        if (!m_viewName.empty())
        {
            title += L" [" + m_viewName + L"]";
        }
        SetWindowText(title.c_str());
    }
}

void CFloatingTabWindow::LoadWindowStateFromRegistry()
{
    // Load from registry if available
    // For now, use default values - could be enhanced to save/restore per-tab window states
    m_windowState.rect = {100, 100, 800, 600};
    m_windowState.maximized = false;
    m_windowState.valid = true;
}

void CFloatingTabWindow::SaveWindowStateToRegistry()
{
    // Save to registry if needed
    // For now, just ensure state is saved in memory
    SaveWindowState();
}

std::wstring CFloatingTabWindow::GetRegistryKey() const
{
    if (m_tabView)
    {
        return L"FloatingWindows\\" + m_tabView->GetTabName();
    }
    return L"FloatingWindows\\Default";
}

void CFloatingTabWindow::RefreshChildWindows(HWND hWnd)
{
    // First, show and refresh the parent window
    ::ShowWindow(hWnd, SW_SHOW);
    ::InvalidateRect(hWnd, nullptr, TRUE);
    ::UpdateWindow(hWnd);
    
    // Lambda function to refresh a child window
    auto refreshChild = [](HWND hChild, LPARAM) -> BOOL
    {
        // Show the child window
        ::ShowWindow(hChild, SW_SHOW);
        
        // Force a complete refresh
        ::InvalidateRect(hChild, nullptr, TRUE);
        ::UpdateWindow(hChild);
        
        // Continue enumeration
        return TRUE;
    };
    
    // Enumerate and refresh all child windows recursively
    ::EnumChildWindows(hWnd, refreshChild, 0);
}

LRESULT CFloatingTabWindow::OnStayOnTop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
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