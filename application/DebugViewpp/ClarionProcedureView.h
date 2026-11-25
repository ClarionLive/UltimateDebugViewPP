// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlctrls.h>
#include <atlsplit.h>
#include <atlctrlx.h>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include "DebugViewppLib/ClarionProcedureTracker.h"
#include "DebugViewppLib/LogFile.h"
#include "ClarionProcedureTimelineView.h"

namespace fusion {
namespace debugviewpp {

// Custom splitter that notifies parent when position changes
class CNotifySplitterWindow : public CSplitterWindowImpl<CNotifySplitterWindow>
{
public:
    DECLARE_WND_CLASS_EX(_T("NotifySplitterWindow"), 0, -1)

    BEGIN_MSG_MAP(CNotifySplitterWindow)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
        CHAIN_MSG_MAP(CSplitterWindowImpl<CNotifySplitterWindow>)
    END_MSG_MAP()

    LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        // Let the base class handle the message first
        bHandled = FALSE;

        // If we were dragging, notify parent that position has changed
        if (::GetCapture() == m_hWnd)
        {
            // Post a message to parent after the drag is complete
            // We use PostMessage to ensure the splitter position is updated first
            ::PostMessage(GetParent(), WM_COMMAND, MAKEWPARAM(ID_PROCEDURE_SPLITTER_MOVED, 0), (LPARAM)GetSplitterPosPct());
        }

        return 0;
    }

    enum { ID_PROCEDURE_SPLITTER_MOVED = 30012 };
};

// Custom list view control that forwards keyboard messages to parent
class CProcedureListViewCtrl : public CWindowImpl<CProcedureListViewCtrl, CListViewCtrl>
{
public:
    DECLARE_WND_SUPERCLASS(NULL, CListViewCtrl::GetWndClassName())
    
    BEGIN_MSG_MAP(CProcedureListViewCtrl)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
        REFLECTED_NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, OnCustomDraw)
        DEFAULT_REFLECTION_HANDLER()
    END_MSG_MAP()
    
    LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        // Forward ESC key to parent
        if (wParam == VK_ESCAPE) {
            ::SendMessage(GetParent(), WM_KEYDOWN, wParam, lParam);
            return 0;
        }
        bHandled = FALSE;
        return 0;
    }
    
    LRESULT OnGetDlgCode(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        // Allow the control to receive all keyboard input
        return DLGC_WANTALLKEYS;
    }
    
    LRESULT OnCustomDraw(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
    {
        // Don't handle - let parent handle it
        bHandled = FALSE;
        return 0;
    }
};

// Container panel for procedure statistics grids with titles
class CProcedureStatsPanel : public CWindowImpl<CProcedureStatsPanel>
{
public:
    DECLARE_WND_CLASS(_T("ProcedureStatsPanel"))

    CProcedureStatsPanel() : m_controlId(0), m_listView(NULL) {}

    BEGIN_MSG_MAP(CProcedureStatsPanel)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        DEFAULT_REFLECTION_HANDLER()
    END_MSG_MAP()

    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        // Create static label for title
        m_label.Create(m_hWnd, rcDefault, m_title.empty() ? L"" : m_title.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
            0);

        // Set font for label (use default GUI font)
        m_label.SetFont(AtlGetDefaultGuiFont());

        // Create list view
        m_listView.Create(m_hWnd, rcDefault, NULL,
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | WS_TABSTOP | WS_CLIPSIBLINGS,
            WS_EX_CLIENTEDGE, m_controlId);

        return 0;
    }

    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);

        // Position label at top (20 pixels height)
        const int labelHeight = 20;
        if (m_label.IsWindow())
            m_label.SetWindowPos(NULL, 0, 0, width, labelHeight, SWP_NOZORDER);

        // Position list view below label
        if (m_listView.IsWindow())
            m_listView.SetWindowPos(NULL, 0, labelHeight, width, height - labelHeight, SWP_NOZORDER);

        return 0;
    }

    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        // Forward notification to parent window for processing
        bHandled = FALSE;
        return ::SendMessage(GetParent(), uMsg, wParam, lParam);
    }

    LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CDCHandle dc((HDC)wParam);
        CRect rect;
        GetClientRect(&rect);
        dc.FillRect(&rect, COLOR_BTNFACE);
        return 1;
    }

    void SetTitle(const std::wstring& title) { m_title = title; }
    void SetControlId(UINT id) { m_controlId = id; }
    CListViewCtrl& GetListView() { return m_listView; }
    CListViewCtrl* GetListViewPtr() { return &m_listView; }
    HWND GetListViewHwnd() const { return m_listView.m_hWnd; }
    bool IsListViewCreated() const { return m_listView.m_hWnd != NULL; }

private:
    CStatic m_label;
    CListViewCtrl m_listView;
    std::wstring m_title;
    UINT m_controlId;
};

class CClarionProcedureView : public CWindowImpl<CClarionProcedureView>
{
public:
    DECLARE_WND_CLASS(_T("ClarionProcedureView"))

    CClarionProcedureView();
    ~CClarionProcedureView();

    // Set the tracker to display (called when view switches)
    void SetTracker(ClarionProcedureTracker* tracker);
    void SetProcedureStatsSplitterPos(int pct);
    int GetProcedureStatsSplitterPos() const;

    BEGIN_MSG_MAP(CClarionProcedureView)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        NOTIFY_CODE_HANDLER(TCN_SELCHANGE, OnTabSelChange)
        NOTIFY_CODE_HANDLER(LVN_COLUMNCLICK, OnColumnClick)
        NOTIFY_CODE_HANDLER(NM_CLICK, OnListClick)
        NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, OnCustomDraw)
        COMMAND_ID_HANDLER(ID_PROCEDURE_CLEAR, OnClear)
        COMMAND_ID_HANDLER(ID_PROCEDURE_EXPAND_ALL, OnExpandAll)
        COMMAND_ID_HANDLER(ID_PROCEDURE_COLLAPSE_ALL, OnCollapseAll)
        COMMAND_ID_HANDLER(ID_PROCEDURE_SPLITTER_MOVED, OnSplitterMoved)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()
    
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTabSelChange(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnColumnClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnListClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnClear(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnExpandAll(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCollapseAll(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSplitterMoved(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    void UpdateProcedureTree();
    void AddProcedureMessage(const Message& msg);
    
private:
    enum {
        ID_PROCEDURE_CLEAR = 30001,
        ID_PROCEDURE_EXPAND_ALL = 30002,
        ID_PROCEDURE_COLLAPSE_ALL = 30003,
        IDC_CURRENTLY_OPEN_VIEW = 30010,
        IDC_PROCEDURE_STATS_VIEW = 30011,
        ID_PROCEDURE_SPLITTER_MOVED = 30012
    };
    
    CTabCtrl m_tabCtrl;
    CProcedureListViewCtrl m_procedureFlowView;  // Changed from tree to list for procedure flow

    // Procedure Stats tab components
    CNotifySplitterWindow m_procedureStatsSplitter;  // Vertical splitter for the two grids with notifications
    CProcedureStatsPanel m_currentlyOpenPanel;  // Left panel with title
    CProcedureStatsPanel m_procedureStatsPanel; // Right panel with title
    CListViewCtrl* m_currentlyOpenView;         // Pointer to left grid: currently open procedures
    CListViewCtrl* m_procedureStatsView;        // Pointer to right grid: procedure statistics

    ::debugviewpp::ClarionProcedureTimelineView m_timelineView;  // New timeline grid view
    CImageList m_imageList;
    
    // Procedure flow event structure
    struct ProcedureEvent {
        std::string threadId;
        std::string procedureName;
        std::string module;
        std::string app;
        double timestamp;
        bool isOpen;  // true for open, false for close
        int depth;
        double duration;  // for close events
    };
    std::vector<ProcedureEvent> m_procedureEvents;
    
    void CreateTabControls();
    void InitializeProcedureFlowView();
    void InitializeProcedureStatsViews();
    void PopulateFlowFromTracker();
    void PopulateProcedureStatsFromTracker();
    void AddProcedureEvent(const ProcedureEvent& event);

    // Sorting support for statistics grids
    void SortCurrentlyOpenView(int column);
    void SortProcedureStatsView(int column);
    int m_currentlyOpenSortColumn = 0;
    bool m_currentlyOpenSortAscending = true;
    int m_procedureStatsSortColumn = 0;
    bool m_procedureStatsSortAscending = true;
    
    ClarionProcedureTracker* m_tracker;

    // Splitter position (as percentage)
    int m_procedureStatsSplitterPos = 50;  // Default to 50% (centered)

    // Sorting state
    enum class SortMode {
        Time,           // Sort by time only
        ThreadThenTime  // Sort by thread, then time
    };
    SortMode m_sortMode = SortMode::Time;
    
    // Highlighting state for matching procedures
    int m_highlightedRow = -1;  // Row index of highlighted matching procedure
    int m_selectedRow = -1;     // Currently selected row
};

// Dialog for displaying procedure statistics
class CProcedureStatsDialog : public CDialogImpl<CProcedureStatsDialog>
{
public:
    enum { IDD = 4000 }; // Temporary ID, should be defined in resource.h
    
    BEGIN_MSG_MAP(CProcedureStatsDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    END_MSG_MAP()
    
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    void SetStatistics(const ClarionProcedureTracker::Statistics& stats);
    
private:
    ClarionProcedureTracker::Statistics m_stats;
};

} // namespace debugviewpp
} // namespace fusion
