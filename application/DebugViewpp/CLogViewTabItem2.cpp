// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "CLogViewTabItem2.h"
#include "LogView.h"
#include "MainFrame.h"
#include "ProcedureStatsSplitter.h"
#include "FloatingTabWindow.h"
#include "HelpTabView.h"
#include "resource.h"
#include "DebugViewppLib/ClarionProcedureTracker.h"
#include "CQueueGridView.h"
#include "DebugViewppLib/LogFile.h"
#include <atldlgs.h>  // For CFontDialog
#include <atlconv.h>  // For CW2A, CA2W
#include <Scintilla.h>  // For SCNotification and SCN_DOUBLECLICK
#include <shlwapi.h>  // For PathRemoveFileSpec
#include <vector>
#include <algorithm>
#include <tuple>

namespace fusion {
namespace debugviewpp {

void CLogViewTabItem::SetView(std::shared_ptr<CLogView> pView)
{
    m_pView = pView;
    SetTabView(*pView);
}

CLogView& CLogViewTabItem::GetView()
{
    return *m_pView;
}




void DisablePaneHeader(CMyPaneContainer& panecontainer)
{
    panecontainer.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON, 0);
    panecontainer.m_cxyHeader = 0;
}

// CDetailTabContainer Implementation
CDetailTabContainer::CDetailTabContainer() : m_activeTab(-1), m_pMainFrame(nullptr), m_contextMenuTabIndex(-1)
{
}

CDetailTabContainer::~CDetailTabContainer()
{
    // Close all floating windows first
    for (auto& floatingInfo : m_floatingTabs)
    {
        if (floatingInfo.window && floatingInfo.window->m_hWnd)
        {
            floatingInfo.window->DestroyWindow();
        }
    }
    m_floatingTabs.clear();
    
    if (m_tabCtrl.m_hWnd)
        m_tabCtrl.DestroyWindow();
    
    for (auto& tabView : m_tabViews)
    {
        if (tabView && tabView->GetHWND())
            ::DestroyWindow(tabView->GetHWND());
    }
    m_tabViews.clear();
}

LRESULT CDetailTabContainer::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // Create the tab control
    RECT rcClient;
    GetClientRect(&rcClient);
    
    // Tab control at the top
    RECT rcTab = {0, 0, rcClient.right, 25};
    m_tabCtrl.Create(m_hWnd, rcTab, nullptr, 
                    WS_CHILD | WS_VISIBLE | TCS_TABS | TCS_SINGLELINE,
                    0, IDC_DETAIL_TABCTRL);
    
    // Set tab control font
    m_tabCtrl.SetFont(AtlGetDefaultGuiFont());
    
    return 0;
}

LRESULT CDetailTabContainer::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    int cx = LOWORD(lParam);
    // int cy = HIWORD(lParam);  // Not used in this implementation
    
    if (m_tabCtrl.m_hWnd)
    {
        // Position tab control at top
        m_tabCtrl.SetWindowPos(nullptr, 0, 0, cx, 25, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    
    // Update layout of tab views
    UpdateTabLayout();
    
    return 0;
}

LRESULT CDetailTabContainer::OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
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

LRESULT CDetailTabContainer::OnTabSelChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
    int newTab = m_tabCtrl.GetCurSel();
    
    // Check if the selected tab is popped out
    if (newTab >= 0 && IsTabPoppedOut(newTab))
    {
        // Don't allow selecting a popped-out tab
        // Restore the previous selection if possible
        if (m_activeTab >= 0 && !IsTabPoppedOut(m_activeTab))
        {
            m_tabCtrl.SetCurSel(m_activeTab);
        }
        else
        {
            // Find first non-popped tab
            for (int i = 0; i < static_cast<int>(m_tabViews.size()); ++i)
            {
                if (!IsTabPoppedOut(i))
                {
                    m_tabCtrl.SetCurSel(i);
                    SetActiveTab(i);
                    break;
                }
            }
        }
        return 0;
    }
    
    if (newTab != m_activeTab)
    {
        SetActiveTab(newTab);
    }
    return 0;
}

LRESULT CDetailTabContainer::OnTabRightClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
    // Get cursor position and tab under cursor
    POINT pt;
    GetCursorPos(&pt);
    m_tabCtrl.ScreenToClient(&pt);
    
    TCHITTESTINFO hitTest = {0};
    hitTest.pt = pt;
    int tabIndex = m_tabCtrl.HitTest(&hitTest);
    
    if (tabIndex >= 0 && tabIndex < static_cast<int>(m_tabViews.size()))
    {
        m_contextMenuTabIndex = tabIndex;
        
        // Show context menu
        CPoint screenPt;
        GetCursorPos(&screenPt);
        
        CMenu menu;
        menu.LoadMenu(IDR_TAB_POPOUT_CONTEXTMENU);
        CMenuHandle popupMenu = menu.GetSubMenu(0);
        
        // Enable/disable menu items based on current state
        if (IsTabPoppedOut(tabIndex))
        {
            popupMenu.EnableMenuItem(ID_TAB_POPOUT, MF_GRAYED);
            popupMenu.EnableMenuItem(ID_TAB_POPIN, MF_ENABLED);
        }
        else
        {
            // Enable pop out for all tabs including Details tab
            popupMenu.EnableMenuItem(ID_TAB_POPOUT, MF_ENABLED);
            popupMenu.EnableMenuItem(ID_TAB_POPIN, MF_GRAYED);
        }
        
        popupMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, screenPt.x, screenPt.y, m_hWnd);
    }
    
    return 0;
}

LRESULT CDetailTabContainer::OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    // Handle context menu for tab control
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);
    
    if (x == -1 && y == -1) // Keyboard context menu
    {
        // Use current selected tab
        m_contextMenuTabIndex = m_activeTab;
        if (m_contextMenuTabIndex >= 0)
        {
            RECT rcTab;
            m_tabCtrl.GetItemRect(m_contextMenuTabIndex, &rcTab);
            m_tabCtrl.ClientToScreen(&rcTab);
            x = rcTab.left + (rcTab.right - rcTab.left) / 2;
            y = rcTab.top + (rcTab.bottom - rcTab.top) / 2;
        }
    }
    else
    {
        // Mouse context menu - find which tab
        CPoint pt(x, y);
        m_tabCtrl.ScreenToClient(&pt);
        
        TCHITTESTINFO hitTest = {0};
        hitTest.pt = pt;
        int tabIndex = m_tabCtrl.HitTest(&hitTest);
        
        if (tabIndex >= 0 && tabIndex < static_cast<int>(m_tabViews.size()))
        {
            m_contextMenuTabIndex = tabIndex;
        }
        else
        {
            return 0; // Not over a tab
        }
    }
    
    // Show context menu
    CMenu menu;
    menu.LoadMenu(IDR_TAB_POPOUT_CONTEXTMENU);
    CMenuHandle popupMenu = menu.GetSubMenu(0);
    
    // Enable/disable menu items based on current state
    if (IsTabPoppedOut(m_contextMenuTabIndex))
    {
        popupMenu.EnableMenuItem(ID_TAB_POPOUT, MF_GRAYED);
        popupMenu.EnableMenuItem(ID_TAB_POPIN, MF_ENABLED);
    }
    else
    {
        // Enable pop out for all tabs including Details tab
        popupMenu.EnableMenuItem(ID_TAB_POPOUT, MF_ENABLED);
        popupMenu.EnableMenuItem(ID_TAB_POPIN, MF_GRAYED);
    }
    
    popupMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, m_hWnd);
    
    return 0;
}

LRESULT CDetailTabContainer::OnPopOut(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (m_contextMenuTabIndex >= 0 && m_contextMenuTabIndex < static_cast<int>(m_tabViews.size()))
    {
        PopOutTab(m_contextMenuTabIndex);
    }
    return 0;
}

LRESULT CDetailTabContainer::OnPopIn(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // Find the floating window for this tab and close it (which will trigger reattachment)
    for (auto& floatingInfo : m_floatingTabs)
    {
        if (floatingInfo.originalIndex == m_contextMenuTabIndex && floatingInfo.window)
        {
            floatingInfo.window->PostMessage(WM_CLOSE);
            break;
        }
    }
    return 0;
}

int CDetailTabContainer::AddTab(std::shared_ptr<IDetailTabView> tabView)
{
    if (!tabView)
        return -1;

    // Add tab to control
    TCITEM tcItem = {0};
    tcItem.mask = TCIF_TEXT;
    std::wstring tabName = tabView->GetTabName();
    tcItem.pszText = const_cast<LPWSTR>(tabName.c_str());

    int index = m_tabCtrl.InsertItem(static_cast<int>(m_tabViews.size()), &tcItem);

    if (index >= 0)
    {
        // Create the tab view
        RECT rcClient;
        GetClientRect(&rcClient);
        RECT rcTab = {0, 25, rcClient.right, rcClient.bottom};

        tabView->Create(m_hWnd, rcTab);
        tabView->SetVisible(false);

        // Set MainFrame pointer if this is a Details tab
        FILE* log = fopen("C:\\Dev\\UltimateDebugViewPP\\prefs_debug.log", "a");
        if (log) {
            fprintf(log, "AddTab called for '%ls', m_pMainFrame = %p\n", tabName.c_str(), m_pMainFrame);
            fclose(log);
        }
        if (m_pMainFrame && tabName == L"Details")
        {
            auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(tabView);
            if (detailsTab)
            {
                if (log = fopen("C:\\Dev\\UltimateDebugViewPP\\prefs_debug.log", "a")) {
                    fprintf(log, "Setting MainFrame on Details tab\n");
                    fclose(log);
                }
                detailsTab->GetDetailView().SetMainFrame(m_pMainFrame);
            }
        }
        
        // Apply stored font to Details tab if this is it
        if (m_detailFont && tabView->GetTabName() == L"Details")
        {
            auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(tabView);
            if (detailsTab)
                detailsTab->GetDetailView().SetDetailFont(m_detailFont);
        }
        
        m_tabViews.push_back(tabView);
        
        // If this is the first tab, make it active
        if (m_activeTab == -1)
        {
            SetActiveTab(index);
        }
    }
    
    return index;
}

void CDetailTabContainer::SetMainFrame(CMainFrame* pMainFrame)
{
    m_pMainFrame = pMainFrame;

    // Propagate to existing tabs, especially the Details tab
    for (auto& tabView : m_tabViews)
    {
        if (tabView && tabView->GetTabName() == L"Details")
        {
            auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(tabView);
            if (detailsTab)
            {
                FILE* log = fopen("C:\\Dev\\UltimateDebugViewPP\\prefs_debug.log", "a");
                if (log) {
                    fprintf(log, "CDetailTabContainer::SetMainFrame - Setting MainFrame on existing Details tab, pMainFrame = %p\n", pMainFrame);
                    fclose(log);
                }
                detailsTab->GetDetailView().SetMainFrame(pMainFrame);
            }
        }
    }
}

void CDetailTabContainer::RemoveTab(int index)
{
    if (index < 0 || index >= static_cast<int>(m_tabViews.size()))
        return;
    
    // Destroy the tab view
    if (m_tabViews[index] && m_tabViews[index]->GetHWND())
    {
        ::DestroyWindow(m_tabViews[index]->GetHWND());
    }
    
    // Remove from collections
    m_tabViews.erase(m_tabViews.begin() + index);
    m_tabCtrl.DeleteItem(index);
    
    // Adjust active tab
    if (index == m_activeTab)
    {
        if (m_tabViews.empty())
        {
            m_activeTab = -1;
        }
        else
        {
            SetActiveTab(std::min(index, static_cast<int>(m_tabViews.size()) - 1));
        }
    }
    else if (index < m_activeTab)
    {
        m_activeTab--;
    }
}

void CDetailTabContainer::SetActiveTab(int index)
{
    if (index < 0 || index >= static_cast<int>(m_tabViews.size()))
        return;
    
    // Hide current tab
    if (m_activeTab >= 0 && m_activeTab < static_cast<int>(m_tabViews.size()))
    {
        m_tabViews[m_activeTab]->SetVisible(false);
    }
    
    // Show new tab
    m_activeTab = index;
    m_tabCtrl.SetCurSel(index);
    
    if (m_activeTab >= 0 && m_activeTab < static_cast<int>(m_tabViews.size()))
    {
        m_tabViews[m_activeTab]->SetVisible(true);
        
        // Ensure all child windows are visible (important after pop-in)
        HWND hTabWnd = m_tabViews[m_activeTab]->GetHWND();
        if (hTabWnd && ::IsWindow(hTabWnd))
        {
            // Force the tab window and all its children to be visible
            ::ShowWindow(hTabWnd, SW_SHOW);
            
            // Enumerate and show all child windows
            ::EnumChildWindows(hTabWnd, [](HWND hChild, LPARAM) -> BOOL {
                ::ShowWindow(hChild, SW_SHOW);
                ::InvalidateRect(hChild, nullptr, TRUE);
                ::UpdateWindow(hChild);
                return TRUE;
            }, 0);
            
            // Force a complete refresh of the tab window
            ::InvalidateRect(hTabWnd, nullptr, TRUE);
            ::UpdateWindow(hTabWnd);
        }
        
        // Force update for procedure tabs when they become active
        // This ensures they have the correct tracker data
        if (index == 1 || index == 2)  // Procedure Flow or Procedure Stats
        {
            if (auto* procFlow = dynamic_cast<CProcedureFlowTabView*>(m_tabViews[index].get()))
            {
                procFlow->UpdateDisplay();
            }
            else if (auto* openProc = dynamic_cast<COpenProceduresTabView*>(m_tabViews[index].get()))
            {
                openProc->UpdateDisplay();
            }
        }
        // Queue tab has been removed - Queue display is now integrated into Details tab
        
        UpdateTabLayout();
    }
}

std::shared_ptr<IDetailTabView> CDetailTabContainer::GetTab(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_tabViews.size()))
        return m_tabViews[index];
    return nullptr;
}

std::shared_ptr<IDetailTabView> CDetailTabContainer::GetActiveTabView() const
{
    return GetTab(m_activeTab);
}

void CDetailTabContainer::UpdateTabLayout()
{
    if (m_activeTab < 0 || m_activeTab >= static_cast<int>(m_tabViews.size()))
        return;
    
    RECT rcClient;
    GetClientRect(&rcClient);
    
    // Calculate tab content area (below tab control)
    RECT rcTab = {0, 25, rcClient.right, rcClient.bottom};
    
    m_tabViews[m_activeTab]->Resize(rcTab);
}

void CDetailTabContainer::ShowActiveTab()
{
    for (int i = 0; i < static_cast<int>(m_tabViews.size()); ++i)
    {
        m_tabViews[i]->SetVisible(i == m_activeTab);
    }
}

// Helper function to refresh all child windows recursively
void CDetailTabContainer::RefreshChildWindows(HWND hWnd)
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

// Pop-out functionality implementation
void CDetailTabContainer::PopOutTab(int index)
{
    if (index < 0 || index >= static_cast<int>(m_tabViews.size()))
        return;
    
    // Allow popping out all tabs including the Details tab
    // (removed the check that prevented index 0 from being popped out)
    
    // Check if tab is already popped out
    if (IsTabPoppedOut(index))
        return;
    
    // Get the tab view
    std::shared_ptr<IDetailTabView> tabView = m_tabViews[index];
    if (!tabView)
        return;
    
    // Get the tab text and mark it as popped out
    TCITEM item = {0};
    item.mask = TCIF_TEXT;
    TCHAR szText[256];
    item.pszText = szText;
    item.cchTextMax = 256;
    m_tabCtrl.GetItem(index, &item);
    std::wstring tabText = szText;
    
    // Update tab text to indicate it's popped out
    std::wstring poppedText = tabText + L" [Popped Out]";
    item.pszText = const_cast<LPWSTR>(poppedText.c_str());
    m_tabCtrl.SetItem(index, &item);
    
    // Hide the tab view content in the main container
    tabView->SetVisible(false);
    
    // Get the tracker and view name from parent LogView (if available)
    ClarionProcedureTracker* tracker = nullptr;
    std::wstring viewName;
    if (m_parentLogView)
    {
        tracker = &m_parentLogView->GetProcedureTracker();
        viewName = m_parentLogView->GetName();
    }
    
    // Create floating window with tracker and view name
    auto floatingWindow = std::make_unique<CFloatingTabWindow>(this, tabView, index, tracker, viewName);
    
    // Create and show the floating window
    RECT rcFloat = {100, 100, 800, 600}; // Default size
    floatingWindow->Create(nullptr, rcFloat, nullptr, 
                          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          WS_EX_TOOLWINDOW);
    
    floatingWindow->ShowWindow(SW_SHOW);
    floatingWindow->RefreshTheme();
    
    // Store floating window info
    FloatingTabInfo info;
    info.tabView = tabView;
    info.window = std::move(floatingWindow);
    info.originalIndex = index;
    info.tabText = tabText;
    m_floatingTabs.push_back(std::move(info));
    
    // If this was the active tab, switch to another tab
    if (index == m_activeTab)
    {
        // Find the first non-popped tab
        for (int i = 0; i < static_cast<int>(m_tabViews.size()); ++i)
        {
            if (i != index && !IsTabPoppedOut(i))
            {
                m_tabCtrl.SetCurSel(i);
                SetActiveTab(i);
                break;
            }
        }
    }
    
    // Update tab layout
    UpdateTabLayout();
}

void CDetailTabContainer::ReattachTab(std::shared_ptr<IDetailTabView> tabView, int originalIndex)
{
    // Find and remove from floating tabs
    auto it = std::find_if(m_floatingTabs.begin(), m_floatingTabs.end(),
        [tabView](const FloatingTabInfo& info) {
            return info.tabView == tabView;
        });
    
    if (it != m_floatingTabs.end())
    {
        // Remove from floating tabs (window will be destroyed automatically)
        m_floatingTabs.erase(it);
        
        // Reattach the tab view to the main container
        if (tabView && originalIndex >= 0 && originalIndex < static_cast<int>(m_tabViews.size()))
        {
            // Get current client area for tab content
            RECT rcClient;
            GetClientRect(&rcClient);
            RECT rcTab = {0, 25, rcClient.right, rcClient.bottom};
            
            // Reparent the tab view
            if (tabView->GetHWND())
            {
                ::SetParent(tabView->GetHWND(), m_hWnd);
                tabView->Resize(rcTab);
            }
            
            // Reconnect the tracker for procedure tabs
            if (tabView->GetTabName() == L"Procedure Flow" || tabView->GetTabName() == L"Procedure Stats")
            {
                if (m_parentLogView)
                {
                    ClarionProcedureTracker* tracker = &m_parentLogView->GetProcedureTracker();
                    
                    if (auto* procFlow = dynamic_cast<CProcedureFlowTabView*>(tabView.get()))
                    {
                        procFlow->SetTracker(tracker);
                        procFlow->UpdateDisplay();
                    }
                    else if (auto* openProc = dynamic_cast<COpenProceduresTabView*>(tabView.get()))
                    {
                        openProc->SetTracker(tracker);
                        openProc->UpdateDisplay();
                    }
                }
            }
            
            // Make this the active tab
            SetActiveTab(originalIndex);
        }
        
        UpdateTabLayout();
    }
}

void CDetailTabContainer::CloseFloatingWindow(CFloatingTabWindow* window)
{
    // Find and remove the floating window
    auto it = std::find_if(m_floatingTabs.begin(), m_floatingTabs.end(),
        [window](const FloatingTabInfo& info) {
            return info.window.get() == window;
        });
    
    if (it != m_floatingTabs.end())
    {
        // Get tab info before removing
        auto tabView = it->tabView;
        int originalIndex = it->originalIndex;
        
        // Remove from floating tabs
        m_floatingTabs.erase(it);
        
        // Reattach the tab
        ReattachTab(tabView, originalIndex);
    }
}

LRESULT CDetailTabContainer::OnFloatingWindowClosing(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // Handle deferred cleanup of floating window
    CFloatingTabWindow* window = reinterpret_cast<CFloatingTabWindow*>(wParam);
    if (!window)
        return 0;
    
    // Find the floating window in our list
    auto it = std::find_if(m_floatingTabs.begin(), m_floatingTabs.end(),
        [window](const FloatingTabInfo& info) {
            return info.window.get() == window;
        });
    
    if (it != m_floatingTabs.end())
    {
        // Get tab info before cleanup
        auto tabView = it->tabView;
        int originalIndex = it->originalIndex;
        std::wstring tabText = it->tabText;
        
        // IMPORTANT: Reparent the tab view BEFORE destroying the window
        // This preserves child controls like the ListView
        if (tabView && tabView->GetHWND())
        {
            HWND hTabWnd = tabView->GetHWND();
            ::SetParent(hTabWnd, m_hWnd);
        }
        
        // Now destroy the floating window (it's hidden already)
        if (window->IsWindow())
        {
            window->DestroyWindow();
        }
        
        // Reconnect the tracker for procedure tabs
        if (tabView && (tabView->GetTabName() == L"Procedure Flow" || tabView->GetTabName() == L"Procedure Stats"))
        {
            if (m_parentLogView)
            {
                ClarionProcedureTracker* tracker = &m_parentLogView->GetProcedureTracker();
                
                if (auto* procFlow = dynamic_cast<CProcedureFlowTabView*>(tabView.get()))
                {
                    procFlow->SetTracker(tracker);
                    procFlow->UpdateDisplay();
                }
                else if (auto* openProc = dynamic_cast<COpenProceduresTabView*>(tabView.get()))
                {
                    openProc->SetTracker(tracker);
                    openProc->UpdateDisplay();
                }
            }
        }
        
        // Restore the original tab text (remove "[Popped Out]" suffix)
        TCITEM tcItem = {0};
        tcItem.mask = TCIF_TEXT;
        tcItem.pszText = const_cast<LPWSTR>(tabText.c_str());
        m_tabCtrl.SetItem(originalIndex, &tcItem);
        
        // Now safely remove from our floating list
        m_floatingTabs.erase(it);
        
        // Select the restored tab and properly refresh it
        m_tabCtrl.SetCurSel(originalIndex);
        SetActiveTab(originalIndex);
        
        UpdateTabLayout();
    }
    
    return 0;
}

bool CDetailTabContainer::IsTabPoppedOut(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_tabViews.size()))
        return false;
    
    // Check if this tab is in the floating tabs list
    for (const auto& floatingInfo : m_floatingTabs)
    {
        if (floatingInfo.originalIndex == index)
            return true;
    }
    
    return false;
}

// Pass-through methods to active tab (for compatibility)
void CDetailTabContainer::SetText(const std::wstring& text)
{
    // Delegate to Details tab (first tab)
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
        {
            detailsTab->GetDetailView().SetText(text);
        }
    }
}

void CDetailTabContainer::SetUnparsedText(const std::wstring& unparsed)
{
    // Delegate to Details tab (first tab)
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
        {
            detailsTab->GetDetailView().SetUnparsedText(unparsed);
        }
    }
}

void CDetailTabContainer::SetContent(const std::wstring& status, const std::wstring& content)
{
    // Delegate to Details tab (first tab)
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
        {
            detailsTab->GetDetailView().SetContent(status, content);
        }
    }
}

void CDetailTabContainer::SetHighlightText(const std::wstring& text)
{
    // Delegate to Details tab (first tab)
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
        {
            detailsTab->GetDetailView().SetHighlightText(text);
        }
    }
}

void CDetailTabContainer::RefreshTheme()
{
    for (auto& tabView : m_tabViews)
    {
        if (tabView)
            tabView->RefreshTheme();
    }

    // Redraw tab container
    if (m_hWnd)
    {
        InvalidateRect(nullptr, TRUE);
    }
}

void CDetailTabContainer::Refresh()
{
    // Refresh the Details tab with current content and new formatting preferences
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
        {
            detailsTab->GetDetailView().Refresh();
        }
    }
}

// Access to procedure tabs specifically
std::shared_ptr<CProcedureFlowTabView> CDetailTabContainer::GetProcedureFlowTab() const
{
    if (m_tabViews.size() > 1)
        return std::dynamic_pointer_cast<CProcedureFlowTabView>(m_tabViews[1]);
    return nullptr;
}

std::shared_ptr<COpenProceduresTabView> CDetailTabContainer::GetOpenProceduresTab() const
{
    if (m_tabViews.size() > 2)
        return std::dynamic_pointer_cast<COpenProceduresTabView>(m_tabViews[2]);
    return nullptr;
}

std::shared_ptr<CProcedureTimelineTabView> CDetailTabContainer::GetProcedureTimelineTab() const
{
    if (m_tabViews.size() > 3)
        return std::dynamic_pointer_cast<CProcedureTimelineTabView>(m_tabViews[3]);
    return nullptr;
}

// Queue tab has been removed - Queue display is now integrated into Details tab

ClarionProcedureTracker* CDetailTabContainer::GetTracker() const
{
    if (m_parentLogView)
    {
        return &m_parentLogView->GetProcedureTracker();
    }
    return nullptr;
}

// Settings access methods (delegated to Details tab)
bool CDetailTabContainer::GetWordWrap() const
{
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
            return detailsTab->GetDetailView().GetWordWrap();
    }
    return true;  // default
}

bool CDetailTabContainer::GetSyntaxHighlight() const
{
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
            return detailsTab->GetDetailView().GetSyntaxHighlight();
    }
    return true;  // default
}

bool CDetailTabContainer::GetFormatContent() const
{
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
            return detailsTab->GetDetailView().GetFormatContent();
    }
    return true;  // default
}

bool CDetailTabContainer::GetLineNumbers() const
{
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
            return detailsTab->GetDetailView().GetLineNumbers();
    }
    return true;  // default
}

void CDetailTabContainer::SetWordWrap(bool value)
{
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
            detailsTab->GetDetailView().SetWordWrap(value);
    }
}

void CDetailTabContainer::SetSyntaxHighlight(bool value)
{
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
            detailsTab->GetDetailView().SetSyntaxHighlight(value);
    }
}

void CDetailTabContainer::SetFormatContent(bool value)
{
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
            detailsTab->GetDetailView().SetFormatContent(value);
    }
}

void CDetailTabContainer::SetLineNumbers(bool value)
{
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
            detailsTab->GetDetailView().SetLineNumbers(value);
    }
}

void CDetailTabContainer::SetDetailFont(HFONT hFont)
{
    // Store the font for future tabs
    m_detailFont = hFont;
    
    // Apply to existing Details tab if it exists
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
            detailsTab->GetDetailView().SetDetailFont(hFont);
    }
}

// Initialize states (delegated to Details tab)
void CDetailTabContainer::InitWordWrapState()
{
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
            detailsTab->GetDetailView().InitWordWrapState();
    }
}

void CDetailTabContainer::InitSyntaxHighlightState()
{
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
            detailsTab->GetDetailView().InitSyntaxHighlightState();
    }
}

void CDetailTabContainer::InitFormatState()
{
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
            detailsTab->GetDetailView().InitFormatState();
    }
}

void CDetailTabContainer::InitLineNumbersState()
{
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
            detailsTab->GetDetailView().InitLineNumbersState();
    }
}

void CDetailTabContainer::InitUnparsedState()
{
    if (!m_tabViews.empty())
    {
        auto detailsTab = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_tabViews[0]);
        if (detailsTab)
            detailsTab->GetDetailView().InitUnparsedState();
    }
}

// CDetailViewTabAdapter Implementation
void CDetailViewTabAdapter::Create(HWND parent, const RECT& rc)
{
    m_detailView.Create(parent, CWindow::rcDefault, nullptr, WS_CHILD | WS_CLIPCHILDREN);
    // Position the detail view
    m_detailView.SetWindowPos(nullptr, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE);
}

void CDetailViewTabAdapter::SetVisible(bool visible)
{
    if (m_detailView.m_hWnd)
    {
        m_detailView.ShowWindow(visible ? SW_SHOW : SW_HIDE);
    }
}

void CDetailViewTabAdapter::Resize(const RECT& rc)
{
    if (m_detailView.m_hWnd)
    {
        m_detailView.SetWindowPos(nullptr, rc.left, rc.top, 
                                 rc.right - rc.left, rc.bottom - rc.top,
                                 SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

// CProcedureFlowTabView Implementation
void CProcedureFlowTabView::Create(HWND parent, const RECT& rc)
{
    // Create the window first
    RECT tempRc = rc;
    CWindowImpl<CProcedureFlowTabView>::Create(parent, tempRc, nullptr, WS_CHILD | WS_CLIPCHILDREN);
    
    // Then create list view as child
    RECT clientRect;
    GetClientRect(&clientRect);
    m_listView.Create(m_hWnd, clientRect, nullptr,
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | WS_TABSTOP,
        WS_EX_CLIENTEDGE);
    
    InitializeListView();
    PopulateFromTracker();
}

void CProcedureFlowTabView::SetVisible(bool visible)
{
    if (m_hWnd)
    {
        ShowWindow(visible ? SW_SHOW : SW_HIDE);
        
        // When making visible, ensure ListView exists and is shown
        if (visible)
        {
            // First ensure ListView exists (recreate if destroyed)
            RecreateListView();
            
            if (m_listView.m_hWnd)
            {
                m_listView.ShowWindow(SW_SHOW);
                m_listView.InvalidateRect(nullptr, TRUE);
                m_listView.UpdateWindow();
                // Force a data refresh to ensure content is displayed
                UpdateDisplay();
            }
        }
    }
}

void CProcedureFlowTabView::Resize(const RECT& rc)
{
    if (m_hWnd)
    {
        SetWindowPos(nullptr, rc.left, rc.top,
                    rc.right - rc.left, rc.bottom - rc.top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
                    
        // Resize the child list view to fill the window
        if (m_listView.m_hWnd)
        {
            RECT clientRect;
            GetClientRect(&clientRect);
            m_listView.SetWindowPos(nullptr, 0, 0,
                                  clientRect.right, clientRect.bottom,
                                  SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
}

void CProcedureFlowTabView::RefreshTheme()
{
    if (m_hWnd)
    {
        Invalidate(TRUE);
    }
    if (m_listView.m_hWnd)
    {
        m_listView.Invalidate(TRUE);
    }
}

void CProcedureFlowTabView::SetTracker(ClarionProcedureTracker* tracker)
{
    m_tracker = tracker;
    
    
    PopulateFromTracker();
}

void CProcedureFlowTabView::UpdateDisplay()
{
    // Ensure ListView exists before populating
    RecreateListView();
    PopulateFromTracker();
}

void CProcedureFlowTabView::RecreateListView()
{
    // If ListView doesn't exist or has been destroyed, recreate it
    if (!m_listView.m_hWnd || !::IsWindow(m_listView.m_hWnd))
    {
        // Get client area of this window
        RECT clientRect;
        GetClientRect(&clientRect);
        
        // Create new ListView
        m_listView.Create(m_hWnd, clientRect, nullptr,
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | WS_TABSTOP,
            WS_EX_CLIENTEDGE);
        
        if (m_listView.m_hWnd)
        {
            InitializeListView();
        }
    }
}

void CProcedureFlowTabView::InitializeListView()
{
    m_listView.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    
    // Add columns for procedure flow
    m_listView.InsertColumn(0, _T("Thread"), LVCFMT_LEFT, 60);
    m_listView.InsertColumn(1, _T("Time"), LVCFMT_LEFT, 80);
    m_listView.InsertColumn(2, _T("Procedure"), LVCFMT_LEFT, 200);  // Will include indentation
    m_listView.InsertColumn(3, _T("Module"), LVCFMT_LEFT, 120);
    m_listView.InsertColumn(4, _T("Status"), LVCFMT_LEFT, 70);
    m_listView.InsertColumn(5, _T("Duration"), LVCFMT_RIGHT, 80);
}

void CProcedureFlowTabView::PopulateFromTracker()
{
    // Clear existing items
    m_listView.DeleteAllItems();
    
    if (!m_tracker)
    {
        return;
    }
    
    // Get events from tracker
    auto events = m_tracker->GetChronologicalEvents();
    
    
    // Display all events in chronological order
    int index = 0;
    for (const auto& event : events) {
        // Format procedure name with indentation
        CString procedureName;
        
        // Calculate indentation - ensure depth is at least 1
        int actualDepth = (event.depth > 0) ? event.depth : 1;
        int numIndents = (actualDepth - 1);
        
        // Use spaces and tree characters for visual indentation
        CString prefix;
        if (numIndents > 0) {
            // Add spaces for indentation (4 spaces per level)
            for (int i = 0; i < numIndents; ++i) {
                prefix += _T("    ");  // 4 spaces per level
            }
            // Add tree connector
            prefix += _T("└─ ");
        }
        
        // Sanitize the procedure name
        CString cleanProcName(event.procedureName.c_str());
        cleanProcName.Replace(_T("\t"), _T(" ")); // Replace tabs with spaces
        cleanProcName.TrimLeft();
        cleanProcName.TrimRight();
        
        // Add open/close indicator and procedure name
        if (event.isOpen) {
            procedureName.Format(_T("%s▶ %s"), (LPCTSTR)prefix, (LPCTSTR)cleanProcName);
        } else {
            procedureName.Format(_T("%s◀ %s"), (LPCTSTR)prefix, (LPCTSTR)cleanProcName);
        }
        
        
        // Insert row
        CString threadStr(event.threadId.c_str());
        int item = m_listView.InsertItem(index, threadStr);
        
        // Format time
        CString timeStr;
        timeStr.Format(_T("%.3f"), event.timestamp);
        m_listView.SetItemText(item, 1, timeStr);
        
        // Procedure name with indentation
        m_listView.SetItemText(item, 2, procedureName);
        
        // Module
        CString moduleStr(event.module.c_str());
        m_listView.SetItemText(item, 3, moduleStr);
        
        // Status
        CString statusStr = event.isOpen ? _T("Opening") : _T("Closing");
        m_listView.SetItemText(item, 4, statusStr);
        
        // Duration (only for close events)
        if (!event.isOpen && event.duration > 0) {
            CString durationStr;
            durationStr.Format(_T("%.3f s"), event.duration);
            m_listView.SetItemText(item, 5, durationStr);
        }
        
        index++;
    }
    
    // Force a refresh and ensure visibility
    int itemCount = m_listView.GetItemCount();
    if (itemCount > 0)
    {
        // Ensure ListView is visible after populating
        m_listView.ShowWindow(SW_SHOW);
        m_listView.InvalidateRect(nullptr, TRUE);
        m_listView.UpdateWindow();
    }
}

LRESULT CProcedureFlowTabView::OnListClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
    // Similar implementation to ClarionProcedureView::OnListClick
    LPNMITEMACTIVATE pnmia = (LPNMITEMACTIVATE)pnmh;
    
    // Set focus to the list view
    m_listView.SetFocus();
    
    // Get the clicked item
    int clickedRow = pnmia->iItem;
    if (clickedRow < 0)
        return 0;
    
    // For now, just select the clicked row
    // TODO: Implement matching procedure highlighting like in ClarionProcedureView
    m_selectedRow = clickedRow;
    m_highlightedRow = -1; // Clear any existing highlight
    
    // Clear all selections first
    for (int i = 0; i < m_listView.GetItemCount(); i++) {
        m_listView.SetItemState(i, 0, LVIS_SELECTED | LVIS_FOCUSED);
    }
    
    // Select the clicked row
    if (m_selectedRow >= 0) {
        m_listView.SetItemState(m_selectedRow, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
    
    // Force redraw
    m_listView.Invalidate();
    
    return 0;
}

LRESULT CProcedureFlowTabView::OnCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
    LPNMLVCUSTOMDRAW lpnmcd = (LPNMLVCUSTOMDRAW)pnmh;
    
    switch (lpnmcd->nmcd.dwDrawStage) {
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;
            
        case CDDS_ITEMPREPAINT:
            {
                int row = static_cast<int>(lpnmcd->nmcd.dwItemSpec);
                
                // Check if this row should be highlighted
                if (row == m_highlightedRow && m_highlightedRow >= 0) {
                    lpnmcd->clrTextBk = RGB(144, 238, 144);  // Light green
                    lpnmcd->clrText = RGB(0, 0, 0);
                    return CDRF_NEWFONT;
                } else if (row == m_selectedRow && m_selectedRow >= 0) {
                    lpnmcd->clrTextBk = RGB(173, 216, 230);  // Light blue
                    lpnmcd->clrText = RGB(0, 0, 0);
                    return CDRF_NEWFONT;
                }
                
                return CDRF_DODEFAULT;
            }
            
        default:
            return CDRF_DODEFAULT;
    }
}

LRESULT CProcedureFlowTabView::OnColumnClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
    // Disabled for now - sorting needs to be fixed
    return 0;
}

LRESULT CProcedureFlowTabView::OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    
    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, ID_PROCEDURE_CLEAR, _T("Clear"));
    
    int cmd = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, m_listView);
    
    if (cmd == ID_PROCEDURE_CLEAR) {
        if (m_tracker) {
            m_tracker->Clear();
            PopulateFromTracker();
        }
    }
    
    return 0;
}

LRESULT CProcedureFlowTabView::OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // Handle ESC key to clear highlighting
    if (wParam == VK_ESCAPE) {
        if (m_selectedRow >= 0 || m_highlightedRow >= 0) {
            m_selectedRow = -1;
            m_highlightedRow = -1;
            m_listView.Invalidate();
        }
    }
    return 0;
}

// COpenProceduresTabView Implementation
void COpenProceduresTabView::Create(HWND parent, const RECT& rc)
{
    // Create the window first
    RECT tempRc = rc;
    CWindowImpl<COpenProceduresTabView>::Create(parent, tempRc, nullptr, WS_CHILD | WS_CLIPCHILDREN);

    // Create splitter window
    RECT clientRect;
    GetClientRect(&clientRect);
    m_splitter.Create(m_hWnd, clientRect, nullptr,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    m_splitter.SetSplitterPanes(NULL, NULL);
    m_splitter.m_bFullDrag = true;
    // Create the two list views
    m_currentlyOpenView.Create(m_splitter, rcDefault, nullptr,
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | WS_TABSTOP,
        WS_EX_CLIENTEDGE);

    m_procedureStatsView.Create(m_splitter, rcDefault, nullptr,
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | WS_TABSTOP,
        WS_EX_CLIENTEDGE);

    // Set the splitter panes
    m_splitter.SetSplitterPanes(m_currentlyOpenView, m_procedureStatsView);
    ApplySplitterPos();

    InitializeViews();
    PopulateFromTracker();
}

void COpenProceduresTabView::SetVisible(bool visible)
{
    if (m_hWnd)
    {
        ShowWindow(visible ? SW_SHOW : SW_HIDE);
        if (m_splitter.m_hWnd)
        {
            m_splitter.ShowWindow(visible ? SW_SHOW : SW_HIDE);
        }
        
        // When making visible, ensure splitter and views exist and are shown
        if (visible)
        {
            // First ensure splitter exists (recreate if destroyed)
            RecreateListView();

            if (m_currentlyOpenView.m_hWnd && m_procedureStatsView.m_hWnd)
            {
                m_currentlyOpenView.ShowWindow(SW_SHOW);
                m_procedureStatsView.ShowWindow(SW_SHOW);
                // Force a data refresh to ensure content is displayed
                UpdateDisplay();
                ApplySplitterPos();
            }
        }
    }
}

void COpenProceduresTabView::Resize(const RECT& rc)
{
    if (m_hWnd)
    {
        SetWindowPos(nullptr, rc.left, rc.top,
                    rc.right - rc.left, rc.bottom - rc.top,
                    SWP_NOZORDER | SWP_NOACTIVATE);

        // Resize the splitter to fill the window
        if (m_splitter.m_hWnd)
        {
            RECT clientRect;
            GetClientRect(&clientRect);
            m_splitter.SetWindowPos(nullptr, 0, 0,
                                  clientRect.right, clientRect.bottom,
                                  SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
}

void COpenProceduresTabView::RefreshTheme()
{
    if (m_hWnd)
    {
        Invalidate(TRUE);
    }
    if (m_currentlyOpenView.m_hWnd)
    {
        m_currentlyOpenView.Invalidate(TRUE);
    }
    if (m_procedureStatsView.m_hWnd)
    {
        m_procedureStatsView.Invalidate(TRUE);
    }
}

void COpenProceduresTabView::SetTracker(ClarionProcedureTracker* tracker)
{
    m_tracker = tracker;
    
    
    PopulateFromTracker();
}

void COpenProceduresTabView::UpdateDisplay()
{
    // Ensure ListView exists before populating
    RecreateListView();
    PopulateFromTracker();
}

void COpenProceduresTabView::RecreateListView()
{
    // If splitter doesn't exist or has been destroyed, recreate it
    if (!m_splitter.m_hWnd || !::IsWindow(m_splitter.m_hWnd))
    {
        // Get client area of this window
        RECT clientRect;
        GetClientRect(&clientRect);

        // Create splitter window
        m_splitter.Create(m_hWnd, clientRect, nullptr,
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
        m_splitter.SetSplitterPanes(NULL, NULL);
        m_splitter.m_bFullDrag = true;
        // Create the two list views
        m_currentlyOpenView.Create(m_splitter, rcDefault, nullptr,
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | WS_TABSTOP,
            WS_EX_CLIENTEDGE);

        m_procedureStatsView.Create(m_splitter, rcDefault, nullptr,
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | WS_TABSTOP,
            WS_EX_CLIENTEDGE);

        // Set the splitter panes
        m_splitter.SetSplitterPanes(m_currentlyOpenView, m_procedureStatsView);
        ApplySplitterPos();

        if (m_currentlyOpenView.m_hWnd && m_procedureStatsView.m_hWnd)
        {
            InitializeViews();
        }
    }
}


void COpenProceduresTabView::SetMainFrame(CMainFrame* mainFrame)
{
    m_pMainFrame = mainFrame;
    if (m_pMainFrame)
    {
        SetSplitterPosPct(m_pMainFrame->GetProcedureStatsSplitterPos());
    }
}

void COpenProceduresTabView::SetSplitterPosPct(int pct)
{
    m_splitterPosPct = ClampProcedureStatsSplitPct(pct);
    ApplySplitterPos();
}

int COpenProceduresTabView::GetSplitterPosPct() const
{
    if (m_splitter.m_hWnd)
    {
        int current = ClampProcedureStatsSplitPct(m_splitter.GetSplitterPosPct());
        const_cast<COpenProceduresTabView*>(this)->m_splitterPosPct = current;
        return current;
    }
    return ClampProcedureStatsSplitPct(m_splitterPosPct);
}

void COpenProceduresTabView::ApplySplitterPos()
{
    m_splitterPosPct = ClampProcedureStatsSplitPct(m_splitterPosPct);
    if (m_splitter.m_hWnd)
    {
        m_splitter.SetSplitterPosPct(m_splitterPosPct);
    }
}
void COpenProceduresTabView::InitializeViews()
{
    // Initialize Currently Open view (top grid)
    m_currentlyOpenView.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_currentlyOpenView.InsertColumn(0, _T("Procedure"), LVCFMT_LEFT, 300);
    m_currentlyOpenView.InsertColumn(1, _T("Count"), LVCFMT_RIGHT, 100);

    // Initialize Procedure Stats view (bottom grid)
    m_procedureStatsView.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_procedureStatsView.InsertColumn(0, _T("Procedure"), LVCFMT_LEFT, 250);
    m_procedureStatsView.InsertColumn(1, _T("Times Opened"), LVCFMT_RIGHT, 100);
    m_procedureStatsView.InsertColumn(2, _T("Times Closed"), LVCFMT_RIGHT, 100);
    m_procedureStatsView.InsertColumn(3, _T("Net"), LVCFMT_RIGHT, 80);
}

void COpenProceduresTabView::PopulateFromTracker()
{
    m_currentlyOpenView.DeleteAllItems();
    m_procedureStatsView.DeleteAllItems();

    if (!m_tracker)
        return;

    // Get aggregated statistics from tracker
    auto currentlyOpen = m_tracker->GetAggregatedOpenCounts();
    auto lifetimeStats = m_tracker->GetLifetimeStatistics();

    // Populate Currently Open view
    std::vector<std::pair<CString, int>> openItems;
    for (const auto& [procName, count] : currentlyOpen) {
        if (count > 0) {
            openItems.push_back({CString(procName.c_str()), count});
        }
    }

    // Sort if needed
    if (m_currentlyOpenSortColumn == 0) {
        // Sort by procedure name
        std::sort(openItems.begin(), openItems.end(),
            [this](const auto& a, const auto& b) {
                int cmp = a.first.CompareNoCase(b.first);
                return m_currentlyOpenSortAscending ? (cmp < 0) : (cmp > 0);
            });
    } else {
        // Sort by count
        std::sort(openItems.begin(), openItems.end(),
            [this](const auto& a, const auto& b) {
                return m_currentlyOpenSortAscending ? (a.second < b.second) : (a.second > b.second);
            });
    }

    // Add items to Currently Open view
    int index = 0;
    for (const auto& [procName, count] : openItems) {
        int item = m_currentlyOpenView.InsertItem(index, procName);
        CString countStr;
        countStr.Format(_T("%d"), count);
        m_currentlyOpenView.SetItemText(item, 1, countStr);
        index++;
    }

    // Populate Procedure Stats view
    std::vector<std::tuple<CString, int, int, int>> statsItems;
    for (const auto& [procName, stats] : lifetimeStats) {
        int net = stats.openCount - stats.closeCount;
        statsItems.push_back({CString(procName.c_str()), stats.openCount, stats.closeCount, net});
    }

    // Sort if needed
    if (m_procedureStatsSortColumn == 0) {
        // Sort by procedure name
        std::sort(statsItems.begin(), statsItems.end(),
            [this](const auto& a, const auto& b) {
                int cmp = std::get<0>(a).CompareNoCase(std::get<0>(b));
                return m_procedureStatsSortAscending ? (cmp < 0) : (cmp > 0);
            });
    } else if (m_procedureStatsSortColumn == 1) {
        // Sort by times opened
        std::sort(statsItems.begin(), statsItems.end(),
            [this](const auto& a, const auto& b) {
                return m_procedureStatsSortAscending ?
                    (std::get<1>(a) < std::get<1>(b)) : (std::get<1>(a) > std::get<1>(b));
            });
    } else if (m_procedureStatsSortColumn == 2) {
        // Sort by times closed
        std::sort(statsItems.begin(), statsItems.end(),
            [this](const auto& a, const auto& b) {
                return m_procedureStatsSortAscending ?
                    (std::get<2>(a) < std::get<2>(b)) : (std::get<2>(a) > std::get<2>(b));
            });
    } else {
        // Sort by net
        std::sort(statsItems.begin(), statsItems.end(),
            [this](const auto& a, const auto& b) {
                return m_procedureStatsSortAscending ?
                    (std::get<3>(a) < std::get<3>(b)) : (std::get<3>(a) > std::get<3>(b));
            });
    }

    // Add items to Procedure Stats view
    index = 0;
    for (const auto& [procName, opened, closed, net] : statsItems) {
        int item = m_procedureStatsView.InsertItem(index, procName);
        CString openedStr, closedStr, netStr;
        openedStr.Format(_T("%d"), opened);
        closedStr.Format(_T("%d"), closed);
        netStr.Format(_T("%d"), net);
        m_procedureStatsView.SetItemText(item, 1, openedStr);
        m_procedureStatsView.SetItemText(item, 2, closedStr);
        m_procedureStatsView.SetItemText(item, 3, netStr);
        index++;
    }
}

// CProcedureTimelineTabView Implementation
void CProcedureTimelineTabView::Create(HWND parent, const RECT& rc)
{
    // Create the window first
    RECT tempRc = rc;
    CWindowImpl<CProcedureTimelineTabView>::Create(parent, tempRc, nullptr, WS_CHILD | WS_CLIPCHILDREN);

    // Then create timeline view as child
    RECT clientRect;
    GetClientRect(&clientRect);
    m_timelineView.Create(m_hWnd, clientRect, nullptr, WS_CHILD | WS_VISIBLE);

    InitializeTimelineView();
    PopulateFromTracker();
}

void CProcedureTimelineTabView::SetVisible(bool visible)
{
    if (m_hWnd)
    {
        ShowWindow(visible ? SW_SHOW : SW_HIDE);

        // When making visible, ensure TimelineView exists and is shown
        if (visible)
        {
            // First ensure TimelineView exists (recreate if destroyed)
            RecreateTimelineView();

            if (m_timelineView.m_hWnd)
            {
                m_timelineView.ShowWindow(SW_SHOW);
                m_timelineView.InvalidateRect(nullptr, TRUE);
                m_timelineView.UpdateWindow();
                // Force a data refresh to ensure content is displayed
                UpdateDisplay();
            }
        }
    }
}

void CProcedureTimelineTabView::Resize(const RECT& rc)
{
    if (m_hWnd)
    {
        SetWindowPos(nullptr, rc.left, rc.top,
                    rc.right - rc.left, rc.bottom - rc.top,
                    SWP_NOZORDER | SWP_NOACTIVATE);

        // Resize the child timeline view to fill the window
        if (m_timelineView.m_hWnd)
        {
            RECT clientRect;
            GetClientRect(&clientRect);
            m_timelineView.SetWindowPos(nullptr, 0, 0,
                                      clientRect.right, clientRect.bottom,
                                      SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
}

void CProcedureTimelineTabView::RefreshTheme()
{
    if (m_hWnd)
    {
        Invalidate(TRUE);
    }
    if (m_timelineView.m_hWnd)
    {
        m_timelineView.Invalidate(TRUE);
    }
}

void CProcedureTimelineTabView::SetTracker(ClarionProcedureTracker* tracker)
{
    m_tracker = tracker;
    PopulateFromTracker();
}

void CProcedureTimelineTabView::UpdateDisplay()
{
    // Ensure TimelineView exists before populating
    RecreateTimelineView();
    PopulateFromTracker();
}

void CProcedureTimelineTabView::RecreateTimelineView()
{
    // If TimelineView doesn't exist or has been destroyed, recreate it
    if (!m_timelineView.m_hWnd || !::IsWindow(m_timelineView.m_hWnd))
    {
        // Get client area of this window
        RECT clientRect;
        GetClientRect(&clientRect);

        // Create the timeline view control
        m_timelineView.Create(m_hWnd, clientRect, nullptr, WS_CHILD | WS_VISIBLE);

        InitializeTimelineView();
    }
}

void CProcedureTimelineTabView::InitializeTimelineView()
{
    // Initialize the timeline view - the ClarionProcedureTimelineView handles its own initialization
    // No additional setup needed as it manages its own state
}

void CProcedureTimelineTabView::PopulateFromTracker()
{
    if (!m_timelineView.m_hWnd)
        return;

    if (m_tracker)
    {
        // Convert the raw tracker pointer to shared_ptr for the timeline view
        // Note: We need to be careful here - the timeline view expects a shared_ptr
        // but we only have a raw pointer. For now, we'll create a non-owning shared_ptr
        std::shared_ptr<ClarionProcedureTracker> sharedTracker(m_tracker, [](ClarionProcedureTracker*){});
        m_timelineView.SetTracker(sharedTracker);
        m_timelineView.RefreshView();
    }
    else
    {
        // Clear the timeline view if no tracker
        m_timelineView.Clear();
    }
}

void CLogViewTabItem2::Create(HWND parent)
{
    m_split.Create(parent, CWindow::rcDefault, nullptr, 
                  WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    SetTabView(m_split);

    m_top.Create(m_split, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    m_bottom.Create(m_split, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

    DisablePaneHeader(m_top);
    DisablePaneHeader(m_bottom);
    m_split.SetSplitterPanes(m_top, m_bottom, true);
    
    // Set splitter extended style to reduce flicker
    m_split.SetSplitterExtendedStyle(SPLIT_PROPORTIONAL | SPLIT_BOTTOMALIGNED);

    // Create the detail tab container
    m_detailTabContainer.Create(m_bottom, CWindow::rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN);
    
    // Set the detail tab container as the client of the bottom pane
    m_bottom.SetClient(m_detailTabContainer);
    
    // Create and add tabs
    FILE* log = fopen("C:\\Dev\\UltimateDebugViewPP\\prefs_debug.log", "a");
    if (log) {
        fprintf(log, "=== CLogViewTabItem2::OnInitialUpdate - Creating tabs ===\n");
        fclose(log);
    }

    m_detailViewAdapter = std::make_shared<CDetailViewTabAdapter>();
    auto procedureFlowTab = std::make_shared<CProcedureFlowTabView>();
    auto openProceduresTab = std::make_shared<COpenProceduresTabView>();
    auto procedureTimelineTab = std::make_shared<CProcedureTimelineTabView>();

    // Create permanent Help tab
    m_helpTabView = std::make_shared<CHelpTabView>(L"Help");

    // Get the executable directory and construct path to help index
    TCHAR szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);
    PathRemoveFileSpec(szPath);
    std::wstring helpPath = szPath;
    helpPath += L"\\help\\index.html";

    // Load the help index file
    m_helpTabView->SetHomeURL(helpPath);
    m_helpTabView->LoadHTMLFile(helpPath);

    m_detailTabContainer.AddTab(m_detailViewAdapter);
    m_detailTabContainer.AddTab(procedureFlowTab);
    m_detailTabContainer.AddTab(openProceduresTab);
    m_detailTabContainer.AddTab(procedureTimelineTab);
    m_detailTabContainer.AddTab(m_helpTabView);
    
    // Force the window to process its WM_CREATE message immediately
    m_detailTabContainer.UpdateWindow();
    
    // Post a message to set the initial text after all initialization is complete
    if (m_detailViewAdapter)
    {
        m_detailViewAdapter->GetDetailView().PostMessage(WM_USER + 100);  // Custom message to set initial text
    }
    
    // Force layout update
    m_split.UpdateWindow();
    
    // Set initial splitter state based on m_detailPaneVisible
    if (m_detailPaneVisible)
    {
        m_split.SetSplitterPosPct(m_splitterPos);
    }
    else
    {
        // Hide the detail pane on first run
        m_split.SetSinglePaneMode(SPLIT_PANE_TOP);
    }
    
    // Initialize procedure tabs (they start with no tracker)
    UpdateProcedureTabs();
}

CLogViewTabItem2::~CLogViewTabItem2()
{
    Cleanup();
}

void CLogViewTabItem2::Cleanup()
{
    if (m_detailTabContainer.m_hWnd && ::IsWindow(m_detailTabContainer.m_hWnd))
    {
        m_detailTabContainer.CleanupResources();
        m_detailTabContainer.DestroyWindow();
    }
    m_detailTabContainer.Detach();

    if (m_bottom.m_hWnd && ::IsWindow(m_bottom.m_hWnd))
    {
        m_bottom.DestroyWindow();
    }
    m_bottom.Detach();

    if (m_top.m_hWnd && ::IsWindow(m_top.m_hWnd))
    {
        m_top.DestroyWindow();
    }
    m_top.Detach();

    if (m_split.m_hWnd && ::IsWindow(m_split.m_hWnd))
    {
        m_split.DestroyWindow();
    }
    m_split.Detach();
}



void CLogViewTabItem2::SetView(std::shared_ptr<CLogView> pView)
{
    m_pView = pView;
    m_top.SetClient(*m_pView);
    
    // Also set the LogView on the detail tab container so it can access the tracker
    m_detailTabContainer.SetParentLogView(pView);
    
    // Update any floating windows with the new view's tracker
    UpdateFloatingWindowTrackers();
}

CLogView& CLogViewTabItem2::GetView()
{
    return *m_pView;
}

std::wstring CLogViewTabItem2::GetTitle() const
{
    // Get the tab title from the base class
    CString text = GetText();
    return std::wstring(text.GetString());
}

void CLogViewTabItem2::ShowDetailPane(bool show)
{
    if (m_detailPaneVisible == show)
        return;
        
    m_detailPaneVisible = show;
    
    if (show)
    {
        // Show the bottom pane
        m_split.SetSinglePaneMode(SPLIT_PANE_NONE);
        m_split.SetSplitterPosPct(m_splitterPos);
        
        // Force proper layout update
        m_split.UpdateSplitterLayout();
        
        // Ensure the detail view's theme is applied when it becomes visible
        m_detailTabContainer.RefreshTheme();
    }
    else
    {
        // Remember current position before hiding
        m_splitterPos = m_split.GetSplitterPosPct();
        // Hide the bottom pane
        m_split.SetSinglePaneMode(SPLIT_PANE_TOP);
    }
}

void CLogViewTabItem2::UpdateDetailView(const std::wstring& text)
{
    m_detailTabContainer.SetText(text);
    // Store for later restoration
    m_lastDetailStatus = L"";
    m_lastDetailContent = text;
}

void CLogViewTabItem2::UpdateDetailView(const std::wstring& status, const std::wstring& content, const std::wstring& unparsed)
{
    m_detailTabContainer.SetContent(status, content);
    if (!unparsed.empty())
    {
        m_detailTabContainer.SetUnparsedText(unparsed);
    }
    // Store for later restoration
    m_lastDetailStatus = status;
    m_lastDetailContent = content;
}

void CLogViewTabItem2::StoreDetailContent(const std::wstring& status, const std::wstring& content)
{
    m_lastDetailStatus = status;
    m_lastDetailContent = content;
}

void CLogViewTabItem2::RestoreDetailContent()
{
    if (!m_lastDetailContent.empty() || !m_lastDetailStatus.empty())
    {
        m_detailTabContainer.SetContent(m_lastDetailStatus, m_lastDetailContent);
    }
}

void CLogViewTabItem2::RefreshDetailView()
{
    // Refresh the detail view with current content using new formatting preferences
    // This is called when SQL formatting options are changed
    m_detailTabContainer.Refresh();
}

void CLogViewTabItem2::SetSplitterPos(int pos)
{
    m_splitterPos = pos;
    if (m_detailPaneVisible && m_split.m_hWnd)
    {
        m_split.SetSplitterPosPct(m_splitterPos);
    }
}

void CLogViewTabItem2::UpdateSplitterPos()
{
    if (m_detailPaneVisible && m_split.m_hWnd)
    {
        int currentPos = m_split.GetSplitterPosPct();
        if (currentPos != m_splitterPos)
        {
            m_splitterPos = currentPos;
        }
    }
}

// Button handler implementations for CDetailView
LRESULT CDetailView::OnFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (!m_pMainFrame)
        return 0;
        
    // Get selected text from Scintilla control
    std::string selectedText = m_edit.GetSelectedText();
    
    if (!selectedText.empty())
    {
        // Convert to wide string
        int size = MultiByteToWideChar(CP_UTF8, 0, selectedText.c_str(), -1, nullptr, 0);
        std::wstring wideText(size - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, selectedText.c_str(), -1, &wideText[0], size);
        
        // Trigger find in main view
        m_pMainFrame->FindTextInView(wideText);
    }
    return 0;
}

LRESULT CDetailView::OnFilter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (!m_pMainFrame)
        return 0;
        
    // Get selected text from Scintilla control
    std::string selectedText = m_edit.GetSelectedText();
    
    if (!selectedText.empty())
    {
        // Convert to wide string
        int size = MultiByteToWideChar(CP_UTF8, 0, selectedText.c_str(), -1, nullptr, 0);
        std::wstring wideText(size - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, selectedText.c_str(), -1, &wideText[0], size);
        
        // Open filter dialog with text as include filter
        m_pMainFrame->ShowFilterDialogWithText(wideText);
    }
    return 0;
}

void CDetailView::InitWordWrapState()
{
    // State is now initialized via SetWordWrap() from CLogViewTabItem2
    m_btnWordWrap.SetToggled(m_wordWrap);
    
    // Update Scintilla word wrap setting
    if (m_edit.m_hWnd)
    {
        m_edit.SetWordWrap(m_wordWrap);
    }
}

void CDetailView::InitSyntaxHighlightState()
{
    // State is now initialized via SetSyntaxHighlight() from CLogViewTabItem2
    m_btnSyntax.SetToggled(m_syntaxHighlight);
}

void CDetailView::InitFormatState()
{
    // State is now initialized via SetFormatContent() from CLogViewTabItem2
    m_btnFormat.SetToggled(m_formatContent);
}

void CDetailView::InitLineNumbersState()
{
    // State is now initialized via SetLineNumbers() from CLogViewTabItem2
    m_btnLineNumbers.SetToggled(m_showLineNumbers);
    
    // Apply the line numbers setting to the Scintilla control
    if (m_edit.m_hWnd)
    {
        m_edit.ShowLineNumbers(m_showLineNumbers);
    }
}

void CDetailView::InitUnparsedState()
{
    // Initialize the unparsed button state
    m_btnUnparsed.SetToggled(m_showUnparsed);
}

LRESULT CDetailView::OnWordWrap(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_wordWrap = !m_wordWrap;
    m_btnWordWrap.SetToggled(m_wordWrap);
    
    // State is now stored locally per-view
    
    // Update Scintilla word wrap setting
    if (m_edit.m_hWnd)
    {
        m_edit.SetWordWrap(m_wordWrap);
    }
    
    return 0;
}

LRESULT CDetailView::OnSyntaxHighlight(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_syntaxHighlight = !m_syntaxHighlight;
    m_btnSyntax.SetToggled(m_syntaxHighlight);
    
    // State is now stored locally per-view
    
    // Re-apply formatting with the stored original text
    // Don't get the current text from the edit control as it may already be formatted
    if (!m_originalText.empty())
    {
        SetText(m_originalText);
    }
    
    return 0;
}

LRESULT CDetailView::OnFormat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_formatContent = !m_formatContent;
    m_btnFormat.SetToggled(m_formatContent);
    
    // State is now stored locally per-view
    
    // Re-apply formatting with the stored original text
    // Don't get the current text from the edit control as it may already be formatted
    if (!m_originalText.empty())
    {
        SetText(m_originalText);
    }
    
    return 0;
}

LRESULT CDetailView::OnLineNumbers(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_showLineNumbers = !m_showLineNumbers;
    m_btnLineNumbers.SetToggled(m_showLineNumbers);
    
    // State is now stored locally per-view
    
    // Update Scintilla line numbers
    if (m_edit.m_hWnd)
    {
        m_edit.ShowLineNumbers(m_showLineNumbers);
    }
    
    return 0;
}

LRESULT CDetailView::OnShowUnparsed(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_showUnparsed = !m_showUnparsed;
    m_btnUnparsed.SetToggled(m_showUnparsed);
    
    // Switch between parsed and unparsed text
    if (!m_unparsedText.empty())
    {
        if (m_showUnparsed)
        {
            // Show unparsed text (with Clarion metadata)
            SetText(m_unparsedText);
        }
        else
        {
            // Show parsed text (original)
            SetText(m_originalText);
        }
    }
    
    return 0;
}

LRESULT CDetailView::OnCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // Handle copy based on current view mode
    if (m_showingQueue)
    {
        // Copy from Queue grid
        if (m_queueGrid.m_hWnd)
        {
            m_queueGrid.CopyToClipboard();
        }
        return 0;
    }

    // Original text copy logic
    std::string text = m_edit.GetText();
    
    if (!text.empty() && OpenClipboard())
    {
        EmptyClipboard();
        
        // Convert to wide string
        int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        std::wstring wideText(size - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wideText[0], size);
        
        size_t len = (wideText.length() + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
        if (hMem)
        {
            LPVOID pMem = GlobalLock(hMem);
            if (pMem)
            {
                memcpy(pMem, wideText.c_str(), len);
                GlobalUnlock(hMem);
                SetClipboardData(CF_UNICODETEXT, hMem);
            }
        }
        CloseClipboard();
    }
    return 0;
}

LRESULT CDetailView::OnFont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // Get current font or default
    LOGFONT lf = GetDetailFont();
    
    // Show font dialog
    CFontDialog dlg(&lf, CF_SCREENFONTS);
    if (dlg.DoModal(m_hWnd) == IDOK)
    {
        // Create and set the new font
        CFont newFont;
        newFont.CreateFontIndirect(&dlg.m_lf);
        SetDetailFont(newFont.Detach());
        
        // Notify MainFrame to save the font setting
        if (m_pMainFrame)
        {
            m_pMainFrame->SetDetailFont(dlg.m_lf);
            m_pMainFrame->SaveSettings();
        }
    }
    return 0;
}

LRESULT CDetailView::OnNotify(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
    LPNMHDR pnmh = (LPNMHDR)lParam;
    
    // Check if this is a notification from the Scintilla control
    if (pnmh && pnmh->hwndFrom == m_edit.m_hWnd)
    {
        SCNotification* pscn = (SCNotification*)lParam;
        
        // Handle double-click notification
        if (pscn->nmhdr.code == SCN_DOUBLECLICK)
        {
            // Scintilla automatically selects the word on double-click
            // We just need to get the selected text
            std::string selectedText = m_edit.GetSelectedText();
            
            if (!selectedText.empty())
            {
                // Highlight all occurrences in the detail pane
                m_edit.HighlightAllOccurrences(selectedText);
                
                // Clear the selection so only the highlight indicator is visible
                m_edit.SendEditor(SCI_SETSEL, static_cast<WPARAM>(-1), static_cast<LPARAM>(-1));  // -1, -1 clears selection
                
                // Also highlight in the main grid
                if (m_pMainFrame)
                {
                    // Convert to wide string and pass to main frame
                    CA2W wideText(selectedText.c_str());
                    std::wstring selectedWord(wideText);
                    m_highlightText = selectedWord;  // Store for later use
                    
                    // Use HighlightTextInView to just highlight without moving to next occurrence
                    m_pMainFrame->HighlightTextInView(selectedWord);
                }
            }
            
            bHandled = TRUE;
            return 0;
        }
    }
    
    bHandled = FALSE;
    return 0;
}

void CDetailView::Refresh()
{
    // Refresh the view by re-setting the original text
    // This will re-apply formatting with the current preferences
    if (!m_originalText.empty())
    {
        SetText(m_originalText);
    }
}

std::wstring CDetailView::ParseErrorContent(const std::wstring& content)
{
    // Check if this is an error message format
    // Expected format: "Message: X,Errorcode: Y,Error: Z,ErrorFile: A,FileError: B,FileErrorCode: C"

    // Look for the typical error message pattern
    if (content.find(L"Message:") == std::wstring::npos ||
        content.find(L"Errorcode:") == std::wstring::npos)
    {
        // Not an error message format, return as-is
        return content;
    }

    std::wstring result;
    std::wstring currentContent = content;

    // Parse the comma-separated key:value pairs
    std::vector<std::pair<std::wstring, std::wstring>> fields;
    size_t pos = 0;

    while (pos < currentContent.length())
    {
        // Find the colon to get the key
        size_t colonPos = currentContent.find(L':', pos);
        if (colonPos == std::wstring::npos)
            break;

        // Extract the key (trim any leading comma/space)
        std::wstring key = currentContent.substr(pos, colonPos - pos);

        // Remove leading comma and spaces from key
        size_t keyStart = key.find_first_not_of(L", \t");
        if (keyStart != std::wstring::npos)
            key = key.substr(keyStart);

        // Find the next comma or end of string for the value
        size_t valueStart = colonPos + 1;
        size_t commaPos = currentContent.find(L',', valueStart);

        // Check if the next comma is actually part of another key:value pair
        // by looking for a colon after it
        while (commaPos != std::wstring::npos)
        {
            size_t nextColonPos = currentContent.find(L':', commaPos);
            if (nextColonPos != std::wstring::npos)
            {
                // Check if this looks like a new field (has known field names)
                std::wstring potentialKey = currentContent.substr(commaPos + 1, nextColonPos - commaPos - 1);
                size_t keyStartCheck = potentialKey.find_first_not_of(L" \t");
                if (keyStartCheck != std::wstring::npos)
                {
                    potentialKey = potentialKey.substr(keyStartCheck);
                    if (potentialKey == L"Errorcode" || potentialKey == L"Error" ||
                        potentialKey == L"ErrorFile" || potentialKey == L"FileError" ||
                        potentialKey == L"FileErrorCode" || potentialKey == L"Message")
                    {
                        // This is a new field, stop here
                        break;
                    }
                }
            }
            // This comma is part of the value, look for the next one
            commaPos = currentContent.find(L',', commaPos + 1);
        }

        std::wstring value;
        if (commaPos != std::wstring::npos)
        {
            value = currentContent.substr(valueStart, commaPos - valueStart);
            pos = commaPos + 1;
        }
        else
        {
            value = currentContent.substr(valueStart);
            pos = currentContent.length();
        }

        // Trim leading/trailing spaces from value
        size_t valueStartTrim = value.find_first_not_of(L" \t");
        size_t valueEndTrim = value.find_last_not_of(L" \t");
        if (valueStartTrim != std::wstring::npos)
            value = value.substr(valueStartTrim, valueEndTrim - valueStartTrim + 1);

        fields.push_back({key, value});
    }

    // Format the fields into a two-column layout
    // Find the maximum key length for alignment
    size_t maxKeyLength = 0;
    for (const auto& field : fields)
    {
        maxKeyLength = std::max(maxKeyLength, field.first.length());
    }

    // Build the formatted output
    for (const auto& field : fields)
    {
        if (!result.empty())
            result += L"\n";

        // Create padded key
        std::wstring paddedKey = field.first + L":";
        while (paddedKey.length() < maxKeyLength + 2)  // +2 for colon and space
            paddedKey += L" ";

        result += paddedKey + field.second;
    }

    return result;
}

void CDetailView::SetText(const std::wstring& text)
{
    // Switch to text view when setting text
    ShowQueueGrid(false);
    m_currentQueueData.reset();

    // Store the original text
    m_originalText = text;

    // Check if this is an error message and parse it if so
    std::wstring processedText = ParseErrorContent(text);

    // Clear any existing highlights when new text is set
    if (m_edit.m_hWnd)
    {
        m_edit.ClearHighlights();
    }
    m_highlightText.clear();

    if (m_edit.m_hWnd)
    {
        // Ensure highlighter is initialized
        if (!m_highlighter)
        {
            m_highlighter = std::make_unique<SyntaxHighlighter>();
        }

        // Determine if we should format the text
        // Use the processed text (which may be parsed error format)
        std::wstring displayText = processedText;
        std::wstring formatName = L"Plain Text";
        ContentFormat detectedFormat = ContentFormat::PlainText;
        
        if (m_highlighter)
        {
            // Detect format ONCE on the original text
            detectedFormat = m_highlighter->DetectFormat(text);
            
            // Format the text if formatting is enabled
            if (m_formatContent)
            {
                switch (detectedFormat)
                {
                    case ContentFormat::JSON:
                        displayText = m_highlighter->FormatJSON(text);
                        break;
                    case ContentFormat::XML:
                        displayText = m_highlighter->FormatXML(text);
                        break;
                    case ContentFormat::SQL:
                        {
                            FILE* log = fopen("C:\\Dev\\UltimateDebugViewPP\\prefs_debug.log", "a");
                            if (log) {
                                fprintf(log, "ContentFormat::SQL - m_pMainFrame = %p\n", m_pMainFrame);
                                fclose(log);
                            }
                        }
                        if (m_pMainFrame)
                        {
                            auto sqlOptions = m_pMainFrame->GetSqlFormatOptions();
                            displayText = m_highlighter->FormatSQL(text, sqlOptions);
                        }
                        else
                        {
                            FILE* log = fopen("C:\\Dev\\UltimateDebugViewPP\\prefs_debug.log", "a");
                            if (log) {
                                fprintf(log, "WARNING: m_pMainFrame is NULL, using default SQL options!\n");
                                fclose(log);
                            }
                            displayText = m_highlighter->FormatSQL(text);
                        }
                        break;
                    case ContentFormat::ClarionEvent:
                        displayText = m_highlighter->FormatClarionEvent(text);
                        break;
                    case ContentFormat::ClarionGroup:
                        displayText = m_highlighter->FormatClarionGroup(text);
                        break;
                    default:
                        // No formatting for other types
                        break;
                }
            }
        }
        
        // Set the text in Scintilla
        m_edit.SetText(displayText);
        
        // Apply theme immediately after setting text to ensure dark mode is applied
        bool isDarkTheme = fusion::debugviewpp::ThemeManager::Instance().IsDarkTheme();
        m_edit.ApplyTheme(isDarkTheme);
        m_edit.ConfigurePlainText(isDarkTheme);
        
        // Force the Scintilla control to repaint with the new theme
        m_edit.SendEditor(SCI_COLOURISE, 0, -1);
        m_edit.InvalidateRect(nullptr, TRUE);
        m_edit.UpdateWindow();
        ::InvalidateRect(m_edit.m_hWnd, nullptr, TRUE);
        ::UpdateWindow(m_edit.m_hWnd);
        
        // Configure syntax highlighting if enabled
        
        if (m_syntaxHighlight && m_highlighter)
        {
            // Apply syntax highlighting based on the ORIGINALLY detected format (not re-detected on formatted text)
            switch (detectedFormat)
            {
                case ContentFormat::SQL:
                    formatName = L"SQL";
                    m_edit.ConfigureSQLLexer(isDarkTheme);
                    break;
                case ContentFormat::JSON:
                    formatName = L"JSON";
                    m_edit.ConfigureJSONLexer(isDarkTheme);
                    break;
                case ContentFormat::XML:
                    formatName = L"XML";
                    m_edit.ConfigureXMLLexer(isDarkTheme);
                    break;
                case ContentFormat::ClarionEvent:
                    formatName = L"Clarion Event";
                    m_edit.ConfigurePlainText(isDarkTheme);  // Use plain text for now
                    break;
                case ContentFormat::ClarionGroup:
                    formatName = L"Clarion Group";
                    m_edit.ConfigurePlainText(isDarkTheme);  // Use plain text for now
                    break;
                default:
                    formatName = L"Plain Text";
                    m_edit.ConfigurePlainText(isDarkTheme);
                    break;
            }
            
            // Update status line to show detected format when syntax highlighting is on
            if (m_statusLine.m_hWnd)
            {
                std::wstring currentStatus;
                int len = m_statusLine.GetWindowTextLength();
                if (len > 0)
                {
                    currentStatus.resize(len + 1);
                    m_statusLine.GetWindowText(&currentStatus[0], len + 1);
                    currentStatus.resize(len);
                }
                
                // Check if we already have format info in the status
                size_t formatPos = currentStatus.find(L"[Format:");
                if (formatPos != std::wstring::npos)
                {
                    // Replace existing format info
                    currentStatus = currentStatus.substr(0, formatPos) + L"[Format: " + formatName + L"]";
                }
                else
                {
                    // Append format info
                    if (!currentStatus.empty())
                    {
                        currentStatus += L" [Format: " + formatName + L"]";
                    }
                    else
                    {
                        currentStatus = L"Format: " + formatName;
                    }
                }
                m_statusLine.SetWindowText(currentStatus.c_str());
            }
        }
        else
        {
            // Plain text without syntax highlighting
            m_edit.ConfigurePlainText(isDarkTheme);
            
            // Still show format detection even when highlighting is off
            if (m_statusLine.m_hWnd && detectedFormat != ContentFormat::PlainText)
            {
                switch (detectedFormat)
                {
                    case ContentFormat::SQL:
                        formatName = L"SQL (detected)";
                        break;
                    case ContentFormat::JSON:
                        formatName = L"JSON (detected)";
                        break;
                    case ContentFormat::XML:
                        formatName = L"XML (detected)";
                        break;
                    case ContentFormat::ClarionEvent:
                        formatName = L"Clarion Event (detected)";
                        break;
                    case ContentFormat::ClarionGroup:
                        formatName = L"Clarion Group (detected)";
                        break;
                    default:
                        break;
                }
                
                std::wstring currentStatus;
                int len = m_statusLine.GetWindowTextLength();
                if (len > 0)
                {
                    currentStatus.resize(len + 1);
                    m_statusLine.GetWindowText(&currentStatus[0], len + 1);
                    currentStatus.resize(len);
                }
                
                // Check if we already have format info in the status
                size_t formatPos = currentStatus.find(L"[Format:");
                size_t detectedPos = currentStatus.find(L"(detected)");
                if (formatPos == std::wstring::npos && detectedPos == std::wstring::npos)
                {
                    // Append format info if not plain text
                    if (detectedFormat != ContentFormat::PlainText)
                    {
                        if (!currentStatus.empty())
                        {
                            currentStatus += L" [" + formatName + L"]";
                        }
                        else
                        {
                            currentStatus = formatName;
                        }
                        m_statusLine.SetWindowText(currentStatus.c_str());
                    }
                }
            }
        }
    }
}

void CDetailView::SetUnparsedText(const std::wstring& unparsed)
{
    // Store the unparsed text
    m_unparsedText = unparsed;
}

void CDetailView::ShowQueueGrid(bool show)
{
    m_showingQueue = show;
    if (show)
    {
        // Hide text editor, show grid
        if (m_edit.m_hWnd)
            m_edit.ShowWindow(SW_HIDE);
        if (m_queueGrid.m_hWnd)
            m_queueGrid.ShowWindow(SW_SHOW);

        // Update status to indicate Queue view
        if (m_statusLine.m_hWnd)
        {
            std::wstring status = L"  Queue View";
            if (m_currentQueueData)
            {
                status += L" - ID: " + std::wstring(m_currentQueueData->queueId.begin(),
                                                    m_currentQueueData->queueId.end());
                status += L" - Rows: " + std::to_wstring(m_currentQueueData->rows.size());
            }
            m_statusLine.SetWindowText(status.c_str());
        }
    }
    else
    {
        // Hide grid, show text editor
        if (m_queueGrid.m_hWnd)
            m_queueGrid.ShowWindow(SW_HIDE);
        if (m_edit.m_hWnd)
            m_edit.ShowWindow(SW_SHOW);
        // Status will be set by caller with actual content
    }

    UpdateButtonStates();
}

void CDetailView::SetQueueData(std::shared_ptr<QueueData> queueData)
{
    m_currentQueueData = queueData;
    if (queueData && !queueData->headers.empty())
    {
        // Set data on the grid
        if (m_queueGrid.m_hWnd)
        {
            m_queueGrid.SetQueueData(queueData);
        }
        // Show the grid view
        ShowQueueGrid(true);
    }
    else
    {
        // No valid queue data, show text view
        ShowQueueGrid(false);
    }
}

void CDetailView::UpdateButtonStates()
{
    if (m_showingQueue)
    {
        // Disable text-only buttons when showing Queue grid
        if (m_btnWordWrap.m_hWnd)
            m_btnWordWrap.EnableWindow(FALSE);
        if (m_btnSyntax.m_hWnd)
            m_btnSyntax.EnableWindow(FALSE);
        if (m_btnFormat.m_hWnd)
            m_btnFormat.EnableWindow(FALSE);
        if (m_btnLineNumbers.m_hWnd)
            m_btnLineNumbers.EnableWindow(FALSE);
        if (m_btnUnparsed.m_hWnd)
            m_btnUnparsed.EnableWindow(FALSE);

        // Keep these enabled as they work for both text and grid
        if (m_btnFont.m_hWnd)
            m_btnFont.EnableWindow(TRUE);
        if (m_btnCopy.m_hWnd)
            m_btnCopy.EnableWindow(TRUE);
        if (m_btnFind.m_hWnd)
            m_btnFind.EnableWindow(FALSE);  // Could implement grid search later
        if (m_btnFilter.m_hWnd)
            m_btnFilter.EnableWindow(FALSE);
    }
    else
    {
        // Enable all buttons for text mode
        if (m_btnWordWrap.m_hWnd)
            m_btnWordWrap.EnableWindow(TRUE);
        if (m_btnSyntax.m_hWnd)
            m_btnSyntax.EnableWindow(TRUE);
        if (m_btnFormat.m_hWnd)
            m_btnFormat.EnableWindow(TRUE);
        if (m_btnLineNumbers.m_hWnd)
            m_btnLineNumbers.EnableWindow(TRUE);
        if (m_btnUnparsed.m_hWnd)
            m_btnUnparsed.EnableWindow(TRUE);
        if (m_btnFont.m_hWnd)
            m_btnFont.EnableWindow(TRUE);
        if (m_btnCopy.m_hWnd)
            m_btnCopy.EnableWindow(TRUE);
        if (m_btnFind.m_hWnd)
            m_btnFind.EnableWindow(TRUE);
        if (m_btnFilter.m_hWnd)
            m_btnFilter.EnableWindow(TRUE);
    }
}

// These methods are no longer needed - Scintilla handles line numbers and syntax highlighting internally
void CDetailView::ApplySyntaxHighlighting(const std::wstring& /*text*/)
{
    // This method is now empty - syntax highlighting is handled in SetText() using Scintilla lexers
}

void CDetailView::SetHighlightText(const std::wstring& text)
{
    m_highlightText = text;
    
    if (m_edit.m_hWnd)
    {
        if (text.empty())
        {
            // Clear highlights
            m_edit.ClearHighlights();
        }
        else
        {
            // Convert to UTF-8 and highlight all occurrences
            CW2A utf8Text(text.c_str());
            m_edit.HighlightAllOccurrences(std::string(utf8Text));
        }
    }
}

void CLogViewTabItem2::UpdateProcedureTabs()
{
    if (!m_pView)
        return;

    // Get the procedure tabs using convenience methods
    auto procedureFlowTab = m_detailTabContainer.GetProcedureFlowTab();
    auto openProceduresTab = m_detailTabContainer.GetOpenProceduresTab();
    auto procedureTimelineTab = m_detailTabContainer.GetProcedureTimelineTab();

    // Set the tracker for all procedure tabs
    auto& tracker = m_pView->GetProcedureTracker();

    if (procedureFlowTab)
    {
        procedureFlowTab->SetTracker(&tracker);
    }

    if (openProceduresTab)
    {
        openProceduresTab->SetTracker(&tracker);
    }

    if (procedureTimelineTab)
    {
        procedureTimelineTab->SetTracker(&tracker);
    }
}

void CLogViewTabItem2::UpdateFloatingWindowTrackers()
{
    // Delegate to the detail tab container to update its floating windows
    m_detailTabContainer.UpdateFloatingWindowTrackers();
}

void CLogViewTabItem2::UpdateFloatingWindows()
{
    // Delegate to the detail tab container to update only floating windows
    m_detailTabContainer.UpdateFloatingWindows();
}

void CDetailTabContainer::UpdateFloatingWindowTrackers()
{
    // Update all floating windows with the new tracker
    for (auto& floatingInfo : m_floatingTabs)
    {
        if (!floatingInfo.window || !floatingInfo.tabView)
            continue;
            
        // Check if this is a procedure tab that needs tracker update
        std::wstring tabName = floatingInfo.tabView->GetTabName();
        if (tabName == L"Procedure Flow" || tabName == L"Procedure Stats")
        {
            ClarionProcedureTracker* tracker = nullptr;
            if (m_parentLogView)
            {
                tracker = &m_parentLogView->GetProcedureTracker();
            }
            
            if (auto* procFlow = dynamic_cast<CProcedureFlowTabView*>(floatingInfo.tabView.get()))
            {
                procFlow->SetTracker(tracker);
                procFlow->UpdateDisplay();
                
                // Update the window caption with the new view name
                if (floatingInfo.window)
                {
                    floatingInfo.window->SetViewName(m_parentLogView->GetName());
                }
            }
            else if (auto* openProc = dynamic_cast<COpenProceduresTabView*>(floatingInfo.tabView.get()))
            {
                openProc->SetTracker(tracker);
                openProc->UpdateDisplay();
                
                // Update the window caption with the new view name
                if (floatingInfo.window)
                {
                    floatingInfo.window->SetViewName(m_parentLogView->GetName());
                }
            }
        }
    }
}

void CDetailTabContainer::UpdateFloatingWindows()
{
    // Only update if we have floating windows
    if (m_floatingTabs.empty())
        return;

    // Update each floating window with current data
    for (auto& floatingInfo : m_floatingTabs)
    {
        if (!floatingInfo.window || !floatingInfo.tabView)
            continue;

        std::wstring tabName = floatingInfo.tabView->GetTabName();
        if (tabName == L"Procedure Flow" || tabName == L"Procedure Stats")
        {
            // Just call UpdateDisplay to refresh with current tracker data
            if (auto* procFlow = dynamic_cast<CProcedureFlowTabView*>(floatingInfo.tabView.get()))
            {
                procFlow->UpdateDisplay();
            }
            else if (auto* openProc = dynamic_cast<COpenProceduresTabView*>(floatingInfo.tabView.get()))
            {
                openProc->UpdateDisplay();
            }
        }
        else if (tabName == L"Queue")
        {
            // Queue tab has been removed - Queue display is now integrated into Details tab
            // Details tab already handles Queue display through SetQueueData
        }
    }
}


void CDetailTabContainer::CleanupResources()
{
    // Close floating windows so their ATL wrappers drop HWNDs before destruction
    for (auto& floatingInfo : m_floatingTabs)
    {
        if (!floatingInfo.window)
            continue;
        if (floatingInfo.window->IsWindow())
        {
            floatingInfo.window->DestroyWindow();
        }
        floatingInfo.window->Detach();
    }
    m_floatingTabs.clear();

    // Destroy tab view windows so ATL wrappers release their handles
    for (auto& tabView : m_tabViews)
    {
        if (!tabView)
            continue;
        HWND hTab = tabView->GetHWND();
        if (hTab && ::IsWindow(hTab))
        {
            ::DestroyWindow(hTab);
        }
    }
    m_tabViews.clear();

    if (m_tabCtrl.m_hWnd && ::IsWindow(m_tabCtrl.m_hWnd))
    {
        m_tabCtrl.DestroyWindow();
    }
    m_tabCtrl.Detach();
}

void CLogViewTabItem2::UpdateQueueView(std::shared_ptr<QueueData> queueData)
{
    // Update the Details tab with Queue data (no longer using separate Queue tab)
    auto detailsAdapter = std::dynamic_pointer_cast<CDetailViewTabAdapter>(m_detailTabContainer.GetTab(0));
    if (detailsAdapter)
    {
        // Update the detail view with Queue data
        if (queueData)
        {
            detailsAdapter->GetDetailView().SetQueueData(queueData);
        }

        // Stay on Details tab (index 0) - no tab switching needed
    }
}


} // namespace debugviewpp
} // namespace fusion









