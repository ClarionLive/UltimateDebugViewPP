// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "atleverything.h"
#include <atlframe.h>
#include <atlctrls.h>
#include <memory>
#include <functional>
#include "resource.h"

namespace fusion {
namespace debugviewpp {

// Custom messages
#define WM_FLOATING_WINDOW_CLOSING (WM_USER + 1000)
#define WM_FLOATING_WINDOW_SHOW_CONTENT (WM_USER + 1001)

class IDetailTabView;
class CDetailTabContainer;
class ClarionProcedureTracker;

// Floating window that hosts a single popped-out tab
class CFloatingTabWindow : public CFrameWindowImpl<CFloatingTabWindow>
{
public:
    DECLARE_WND_CLASS(_T("DebugViewPP_FloatingTab"))

    // Constructor
    CFloatingTabWindow(CDetailTabContainer* parent, std::shared_ptr<IDetailTabView> tabView, int originalIndex, ClarionProcedureTracker* tracker = nullptr, const std::wstring& viewName = L"");
    ~CFloatingTabWindow();

    BEGIN_MSG_MAP(CFloatingTabWindow)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
        MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
        MESSAGE_HANDLER(WM_FLOATING_WINDOW_SHOW_CONTENT, OnShowContent)
        COMMAND_ID_HANDLER(ID_FLOATING_POPIN, OnPopIn)
        COMMAND_ID_HANDLER(ID_FLOATING_REFRESH, OnRefresh)
        COMMAND_ID_HANDLER(ID_FLOATING_STAY_ON_TOP, OnStayOnTop)
        CHAIN_MSG_MAP(CFrameWindowImpl<CFloatingTabWindow>)
    END_MSG_MAP()

    // Access to the hosted tab
    std::shared_ptr<IDetailTabView> GetTabView() const { return m_tabView; }
    int GetOriginalIndex() const { return m_originalIndex; }
    
    // Window state management
    void SaveWindowState();
    void RestoreWindowState();
    
    // Theme support
    void RefreshTheme();
    
    // Update view name in caption
    void SetViewName(const std::wstring& viewName);

protected:
    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
    LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnSysCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
    LRESULT OnShowContent(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnPopIn(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnStayOnTop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
    CDetailTabContainer* m_parentContainer;
    std::shared_ptr<IDetailTabView> m_tabView;
    int m_originalIndex;
    ClarionProcedureTracker* m_tracker;
    std::wstring m_viewName;
    
    // Toolbar
    CToolBarCtrl m_toolbar;
    bool m_stayOnTop = true;  // Default to Stay On Top for better visibility
    static const int TOOLBAR_HEIGHT = 24;
    
    // Window state
    struct WindowState {
        RECT rect = {100, 100, 800, 600}; // Default size and position
        bool maximized = false;
        bool valid = false;
    } m_windowState;
    
    void CreateMenuBar();
    void LoadWindowStateFromRegistry();
    void SaveWindowStateToRegistry();
    std::wstring GetRegistryKey() const;
    
    // Helper to refresh all child windows recursively
    static void RefreshChildWindows(HWND hWnd);
    
    // Constants
    static const int MIN_WIDTH = 400;
    static const int MIN_HEIGHT = 300;
    
    // Use the resource ID defined in resource.h
};

} // namespace debugviewpp
} // namespace fusion