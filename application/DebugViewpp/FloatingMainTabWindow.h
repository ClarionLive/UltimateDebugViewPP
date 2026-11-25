// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "atleverything.h"
#include <atlctrls.h>
#include <memory>
#include <string>
#include "resource.h"

namespace fusion {
namespace debugviewpp {

class CLogViewTabItem2;
class CMainFrame;

// Floating window that hosts a complete main tab (LogView + detail pane)
class CFloatingMainTabWindow : public CFrameWindowImpl<CFloatingMainTabWindow>
{
public:
    DECLARE_WND_CLASS(_T("DebugViewPP_FloatingMainTab"))

    // Constructor
    CFloatingMainTabWindow(CMainFrame* mainFrame, CLogViewTabItem2* tabItem, int originalIndex);
    ~CFloatingMainTabWindow();

    BEGIN_MSG_MAP(CFloatingMainTabWindow)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
        MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
        COMMAND_ID_HANDLER(ID_FLOATING_MAIN_POPIN, OnPopIn)
        COMMAND_ID_HANDLER(ID_FLOATING_STAY_ON_TOP, OnStayOnTop)
        CHAIN_MSG_MAP(CFrameWindowImpl<CFloatingMainTabWindow>)
    END_MSG_MAP()

    // Message handlers
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPopIn(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnStayOnTop(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    // Get the tab item
    CLogViewTabItem2* GetTabItem() { return m_tabItem; }
    int GetOriginalIndex() const { return m_originalIndex; }
    std::wstring GetTabName() const { return m_tabName; }
    
    // Theme support
    void RefreshTheme();

private:
    void CreateMenuBar();
    void SaveWindowState();
    void RestoreWindowState();
    void LoadWindowStateFromRegistry();
    void SaveWindowStateToRegistry();
    std::wstring GetRegistryKey() const;

    // Window state tracking
    struct WindowState
    {
        RECT rect{100, 100, 900, 700};
        bool maximized{false};
        bool valid{false};
    };

    // Constants
    static const int MIN_WIDTH = 400;
    static const int MIN_HEIGHT = 300;
    static const int TOOLBAR_HEIGHT = 24;

    CMainFrame* m_mainFrame;
    CLogViewTabItem2* m_tabItem;
    int m_originalIndex;
    std::wstring m_tabName;
    HWND m_originalParent;
    WindowState m_windowState;
    
    // Toolbar
    CToolBarCtrl m_toolbar;
    bool m_stayOnTop = true;  // Default to Stay On Top for better visibility
};

// Custom message for main tab floating window closing
#ifndef WM_FLOATING_MAIN_WINDOW_CLOSING
#define WM_FLOATING_MAIN_WINDOW_CLOSING (WM_USER + 1001)
#endif

} // namespace debugviewpp
} // namespace fusion