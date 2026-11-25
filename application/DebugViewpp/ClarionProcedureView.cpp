// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "ClarionProcedureView.h"
#include "DebugViewppLib/LogFile.h"
#include "ProcedureStatsSplitter.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <atlstr.h>
#include <atlmisc.h>
#include <windowsx.h>

namespace fusion {
namespace debugviewpp {


CClarionProcedureView::CClarionProcedureView()
    : m_tracker(nullptr)
    , m_currentlyOpenView(nullptr)
    , m_procedureStatsView(nullptr)
{
}

CClarionProcedureView::~CClarionProcedureView()
{
}

void CClarionProcedureView::SetTracker(ClarionProcedureTracker* tracker)
{
    m_tracker = tracker;
    
    // Refresh the display with the new tracker's data
    if (m_procedureFlowView.IsWindow())
    {
        PopulateFlowFromTracker();
    }
    if (m_procedureStatsSplitter.IsWindow())
    {
        // Ensure we have valid pointers to the list views
        if (!m_currentlyOpenView && m_currentlyOpenPanel.IsListViewCreated()) {
            m_currentlyOpenView = m_currentlyOpenPanel.GetListViewPtr();
        }
        if (!m_procedureStatsView && m_procedureStatsPanel.IsListViewCreated()) {
            m_procedureStatsView = m_procedureStatsPanel.GetListViewPtr();
        }
        PopulateProcedureStatsFromTracker();
    }
    // Pass the tracker to the timeline view (convert to shared_ptr with no-op deleter)
    if (m_timelineView.IsWindow() && tracker)
    {
        m_timelineView.SetTracker(std::shared_ptr<ClarionProcedureTracker>(tracker, [](ClarionProcedureTracker*) {}));
    }
}

void CClarionProcedureView::SetProcedureStatsSplitterPos(int pct)
{
    m_procedureStatsSplitterPos = ClampProcedureStatsSplitPct(pct);
    if (m_procedureStatsSplitter.IsWindow())
    {
        m_procedureStatsSplitter.SetSplitterPosPct(m_procedureStatsSplitterPos);
    }
}

int CClarionProcedureView::GetProcedureStatsSplitterPos() const
{
    if (m_procedureStatsSplitter.IsWindow())
    {
        return ClampProcedureStatsSplitPct(m_procedureStatsSplitter.GetSplitterPosPct());
    }
    return ClampProcedureStatsSplitPct(m_procedureStatsSplitterPos);
}


LRESULT CClarionProcedureView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // Create tab control
    m_tabCtrl.Create(m_hWnd, rcDefault, NULL, 
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_TABS);
    
    // Add tabs
    TCITEM item = {0};
    item.mask = TCIF_TEXT;
    TCHAR procFlowText[] = _T("Procedure Flow");
    item.pszText = procFlowText;
    m_tabCtrl.InsertItem(0, &item);

    TCHAR procStatsText[] = _T("Procedure Stats");
    item.pszText = procStatsText;
    m_tabCtrl.InsertItem(1, &item);

    TCHAR timelineText[] = _T("Timeline View");
    item.pszText = timelineText;
    m_tabCtrl.InsertItem(2, &item);
    
    // Create child controls
    CreateTabControls();
    
    // Initialize views
    InitializeProcedureFlowView();
    InitializeProcedureStatsViews();

    // Show first tab
    m_procedureFlowView.ShowWindow(SW_SHOW);
    m_procedureStatsSplitter.ShowWindow(SW_HIDE);
    
    // Populate flow initially (might be empty but sets up structure)
    PopulateFlowFromTracker();
    
    return 0;
}

LRESULT CClarionProcedureView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    CRect rect;
    GetClientRect(&rect);
    
    // Size tab control
    m_tabCtrl.SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height(), SWP_NOZORDER);
    
    // Get display area for tab pages
    CRect tabRect;
    m_tabCtrl.GetClientRect(&tabRect);
    m_tabCtrl.AdjustRect(FALSE, &tabRect);
    
    // Size child controls
    m_procedureFlowView.SetWindowPos(NULL, tabRect.left, tabRect.top,
        tabRect.Width(), tabRect.Height(), SWP_NOZORDER);
    m_procedureStatsSplitter.SetWindowPos(NULL, tabRect.left, tabRect.top,
        tabRect.Width(), tabRect.Height(), SWP_NOZORDER);
    m_timelineView.SetWindowPos(NULL, tabRect.left, tabRect.top,
        tabRect.Width(), tabRect.Height(), SWP_NOZORDER);

    return 0;
}

LRESULT CClarionProcedureView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    return 0;
}

LRESULT CClarionProcedureView::OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    // Handle ESC key to clear highlighting
    if (wParam == VK_ESCAPE) {
        if (m_selectedRow >= 0 || m_highlightedRow >= 0) {
            // Clear highlighting state
            m_selectedRow = -1;
            m_highlightedRow = -1;
            
            // Force a complete redraw of the list view
            m_procedureFlowView.Invalidate();
            m_procedureFlowView.UpdateWindow();
        }
        return 0;
    }
    
    bHandled = FALSE;
    return 0;
}

LRESULT CClarionProcedureView::OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    
    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, ID_PROCEDURE_EXPAND_ALL, _T("Expand All"));
    menu.AppendMenu(MF_STRING, ID_PROCEDURE_COLLAPSE_ALL, _T("Collapse All"));
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, ID_PROCEDURE_CLEAR, _T("Clear"));
    
    menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
    
    return 0;
}

LRESULT CClarionProcedureView::OnNotify(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LPNMHDR pnmh = (LPNMHDR)lParam;
    UINT_PTR idFrom = (UINT_PTR)wParam;  // Get the control ID

    // Check if this is a column click from one of our list views
    if (pnmh->code == LVN_COLUMNCLICK) {
        LPNMLISTVIEW pnmv = (LPNMLISTVIEW)pnmh;

        // Check which list view sent the notification (by control ID or window handle)
        if (idFrom == IDC_CURRENTLY_OPEN_VIEW || (m_currentlyOpenView && pnmh->hwndFrom == m_currentlyOpenView->m_hWnd)) {
            SortCurrentlyOpenView(pnmv->iSubItem);
            return 0;
        } else if (idFrom == IDC_PROCEDURE_STATS_VIEW || (m_procedureStatsView && pnmh->hwndFrom == m_procedureStatsView->m_hWnd)) {
            SortProcedureStatsView(pnmv->iSubItem);
            return 0;
        }
    }

    // Let other handlers process this notification
    bHandled = FALSE;
    return 0;
}

LRESULT CClarionProcedureView::OnTabSelChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
    int sel = m_tabCtrl.GetCurSel();

    // Save splitter position when leaving Procedure Stats tab
    if (m_procedureStatsSplitter.IsWindow() && m_procedureStatsSplitter.IsWindowVisible()) {
        m_procedureStatsSplitterPos = ClampProcedureStatsSplitPct(m_procedureStatsSplitter.GetSplitterPosPct());
    }

    if (sel == 0) {
        m_procedureFlowView.ShowWindow(SW_SHOW);
        m_procedureStatsSplitter.ShowWindow(SW_HIDE);
        m_timelineView.ShowWindow(SW_HIDE);
        PopulateFlowFromTracker();  // Update the flow when switching to it
    } else if (sel == 1) {
        m_procedureFlowView.ShowWindow(SW_HIDE);
        m_procedureStatsSplitter.ShowWindow(SW_SHOW);
        m_timelineView.ShowWindow(SW_HIDE);

        // Restore the saved splitter position, enforcing a sensible minimum width
        int desiredPos = ClampProcedureStatsSplitPct(m_procedureStatsSplitterPos);
        m_procedureStatsSplitterPos = desiredPos;
        m_procedureStatsSplitter.SetSplitterPosPct(desiredPos);

        // Ensure we have valid pointers to the list views
        if (!m_currentlyOpenView && m_currentlyOpenPanel.IsListViewCreated()) {
            m_currentlyOpenView = m_currentlyOpenPanel.GetListViewPtr();
        }
        if (!m_procedureStatsView && m_procedureStatsPanel.IsListViewCreated()) {
            m_procedureStatsView = m_procedureStatsPanel.GetListViewPtr();
        }

        PopulateProcedureStatsFromTracker();
    } else if (sel == 2) {
        m_procedureFlowView.ShowWindow(SW_HIDE);
        m_procedureStatsSplitter.ShowWindow(SW_HIDE);
        m_timelineView.ShowWindow(SW_SHOW);
        m_timelineView.RefreshView();  // Update timeline view when switching to it
    }
    
    return 0;
}

LRESULT CClarionProcedureView::OnColumnClick(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
    LPNMLISTVIEW pnmv = (LPNMLISTVIEW)pnmh;

    // Check which list view sent the notification (by control ID or window handle)
    if (idCtrl == IDC_CURRENTLY_OPEN_VIEW || (m_currentlyOpenView && pnmv->hdr.hwndFrom == m_currentlyOpenView->m_hWnd)) {
        SortCurrentlyOpenView(pnmv->iSubItem);
    } else if (idCtrl == IDC_PROCEDURE_STATS_VIEW || (m_procedureStatsView && pnmv->hdr.hwndFrom == m_procedureStatsView->m_hWnd)) {
        SortProcedureStatsView(pnmv->iSubItem);
    }

    return 0;
    
    /*
    LPNMLISTVIEW pnmv = (LPNMLISTVIEW)pnmh;
    
    // Only handle clicks on the procedure flow view
    if (pnmv->hdr.hwndFrom != m_procedureFlowView.m_hWnd)
        return 0;
        
    // Check which column was clicked
    if (pnmv->iSubItem == 0) {  // Thread column
        // Toggle to Thread/Time sorting
        m_sortMode = SortMode::ThreadThenTime;
    } else if (pnmv->iSubItem == 1) {  // Time column
        // Toggle to Time only sorting
        m_sortMode = SortMode::Time;
    }
    
    // Repopulate with new sort order
    PopulateFlowFromTracker();
    
    return 0;
    */
}

LRESULT CClarionProcedureView::OnListClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
    LPNMITEMACTIVATE pnmia = (LPNMITEMACTIVATE)pnmh;
    
    // Only handle clicks on the procedure flow view
    if (pnmia->hdr.hwndFrom != m_procedureFlowView.m_hWnd)
        return 0;
    
    // Set focus to the list view to enable keyboard handling
    m_procedureFlowView.SetFocus();
    
    // Get the clicked item
    int clickedRow = pnmia->iItem;
    if (clickedRow < 0)
        return 0;
    
    // Get the procedure name and status from the clicked row
    TCHAR buffer[256];
    m_procedureFlowView.GetItemText(clickedRow, 2, buffer, 256);  // Procedure column
    CString procedureName(buffer);
    m_procedureFlowView.GetItemText(clickedRow, 4, buffer, 256);  // Status column
    CString status(buffer);
    m_procedureFlowView.GetItemText(clickedRow, 0, buffer, 256);  // Thread column
    CString thread(buffer);
    
    
    // Remove indentation and open/close markers to get clean procedure name
    // First remove all dots (indentation)
    while (procedureName.Find(_T("..")) >= 0) {
        procedureName.Replace(_T(".."), _T(""));
    }
    
    // Find the position of '>' or '<' and extract everything after it
    int markerPos = procedureName.Find(_T('>'));
    if (markerPos < 0) {
        markerPos = procedureName.Find(_T('<'));
    }
    
    if (markerPos >= 0) {
        // Get everything after the marker (skip the marker and the space after it)
        procedureName = procedureName.Mid(markerPos + 1);
        procedureName.TrimLeft();  // Remove any spaces after the marker
    }
    
    procedureName.TrimRight();  // Remove trailing spaces
    
    // Find the matching procedure (open finds close, close finds open)
    bool isOpening = (status == _T("Opening"));
    int matchingRow = -1;
    
    // Search for matching procedure in the same thread
    int itemCount = m_procedureFlowView.GetItemCount();
    if (isOpening) {
        // Search forward for the closing
        for (int i = clickedRow + 1; i < itemCount; i++) {
            TCHAR itemBuffer[256];
            m_procedureFlowView.GetItemText(i, 0, itemBuffer, 256);
            CString itemThread(itemBuffer);
            m_procedureFlowView.GetItemText(i, 2, itemBuffer, 256);
            CString itemProc(itemBuffer);
            m_procedureFlowView.GetItemText(i, 4, itemBuffer, 256);
            CString itemStatus(itemBuffer);
            
            // Clean the procedure name for comparison
            // First remove all dots (indentation)
            while (itemProc.Find(_T("..")) >= 0) {
                itemProc.Replace(_T(".."), _T(""));
            }
            
            // Find the position of '>' or '<' and extract everything after it
            int itemMarkerPos = itemProc.Find(_T('>'));
            if (itemMarkerPos < 0) {
                itemMarkerPos = itemProc.Find(_T('<'));
            }
            
            if (itemMarkerPos >= 0) {
                // Get everything after the marker (skip the marker and the space after it)
                itemProc = itemProc.Mid(itemMarkerPos + 1);
                itemProc.TrimLeft();  // Remove any spaces after the marker
            }
            
            itemProc.TrimRight();  // Remove trailing spaces
            
            if (itemThread == thread && itemProc == procedureName && itemStatus == _T("Closing")) {
                matchingRow = i;
                break;
            }
        }
    } else {
        // Search backward for the opening
        for (int i = clickedRow - 1; i >= 0; i--) {
            TCHAR itemBuffer[256];
            m_procedureFlowView.GetItemText(i, 0, itemBuffer, 256);
            CString itemThread(itemBuffer);
            m_procedureFlowView.GetItemText(i, 2, itemBuffer, 256);
            CString itemProc(itemBuffer);
            m_procedureFlowView.GetItemText(i, 4, itemBuffer, 256);
            CString itemStatus(itemBuffer);
            
            // Clean the procedure name for comparison
            // First remove all dots (indentation)
            while (itemProc.Find(_T("..")) >= 0) {
                itemProc.Replace(_T(".."), _T(""));
            }
            
            // Find the position of '>' or '<' and extract everything after it
            int itemMarkerPos = itemProc.Find(_T('>'));
            if (itemMarkerPos < 0) {
                itemMarkerPos = itemProc.Find(_T('<'));
            }
            
            if (itemMarkerPos >= 0) {
                // Get everything after the marker (skip the marker and the space after it)
                itemProc = itemProc.Mid(itemMarkerPos + 1);
                itemProc.TrimLeft();  // Remove any spaces after the marker
            }
            
            itemProc.TrimRight();  // Remove trailing spaces
            
            if (itemThread == thread && itemProc == procedureName && itemStatus == _T("Opening")) {
                matchingRow = i;
                break;
            }
        }
    }
    
    // Clear all selections first
    for (int i = 0; i < m_procedureFlowView.GetItemCount(); i++) {
        m_procedureFlowView.SetItemState(i, 0, LVIS_SELECTED | LVIS_FOCUSED);
    }
    
    // Update highlighting state
    m_selectedRow = clickedRow;
    m_highlightedRow = matchingRow;
    
    // Select the clicked row
    if (m_selectedRow >= 0) {
        m_procedureFlowView.SetItemState(m_selectedRow, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
    
    // Also select the matching row to highlight it
    if (m_highlightedRow >= 0) {
        m_procedureFlowView.SetItemState(m_highlightedRow, LVIS_SELECTED, LVIS_SELECTED);
    }
    
    // Force a complete redraw of the list view
    m_procedureFlowView.Invalidate();
    m_procedureFlowView.UpdateWindow();
    
    // If we found a match, ensure it's visible
    if (matchingRow >= 0) {
        m_procedureFlowView.EnsureVisible(matchingRow, FALSE);
    }
    
    return 0;
}

LRESULT CClarionProcedureView::OnCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
    LPNMLVCUSTOMDRAW lpnmcd = (LPNMLVCUSTOMDRAW)pnmh;
    
    // Only process for procedure flow view
    if (lpnmcd->nmcd.hdr.hwndFrom != m_procedureFlowView.m_hWnd) {
        return CDRF_DODEFAULT;
    }
    
    switch (lpnmcd->nmcd.dwDrawStage) {
        case CDDS_PREPAINT:
            // Request item-specific notifications
            return CDRF_NOTIFYITEMDRAW;
            
        case CDDS_ITEMPREPAINT:
            {
                int row = static_cast<int>(lpnmcd->nmcd.dwItemSpec);
                
                // Check if this row should be highlighted
                if (row == m_highlightedRow && m_highlightedRow >= 0) {
                    // Highlight the matching procedure with a light green background
                    lpnmcd->clrTextBk = RGB(144, 238, 144);  // Light green
                    lpnmcd->clrText = RGB(0, 0, 0);  // Black text
                    return CDRF_NEWFONT;
                } else if (row == m_selectedRow && m_selectedRow >= 0) {
                    // Highlight the selected procedure with a light blue background
                    lpnmcd->clrTextBk = RGB(173, 216, 230);  // Light blue
                    lpnmcd->clrText = RGB(0, 0, 0);  // Black text
                    return CDRF_NEWFONT;
                }
                
                return CDRF_DODEFAULT;
            }
            
        default:
            return CDRF_DODEFAULT;
    }
}

LRESULT CClarionProcedureView::OnClear(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (m_tracker)
    {
        m_tracker->Clear();
    }
    m_procedureFlowView.DeleteAllItems();
    if (m_currentlyOpenView)
        m_currentlyOpenView->DeleteAllItems();
    if (m_procedureStatsView)
        m_procedureStatsView->DeleteAllItems();
    return 0;
}

LRESULT CClarionProcedureView::OnExpandAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // Not needed for list view
    return 0;
}

LRESULT CClarionProcedureView::OnCollapseAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // Not needed for list view
    return 0;
}

LRESULT CClarionProcedureView::OnSplitterMoved(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/)
{
    // The splitter position is sent in the LPARAM (passed as hWndCtl)
    int newPos = (int)(LPARAM)hWndCtl;
    m_procedureStatsSplitterPos = ClampProcedureStatsSplitPct(newPos);

    // Forward to MainFrame to save the position
    HWND hWndParent = GetParent();
    if (hWndParent)
    {
        ::SendMessage(hWndParent, WM_COMMAND, MAKEWPARAM(ID_PROCEDURE_SPLITTER_MOVED, 0), (LPARAM)m_procedureStatsSplitterPos);
    }

    return 0;
}

void CClarionProcedureView::CreateTabControls()
{
    CRect tabRect;
    m_tabCtrl.GetClientRect(&tabRect);
    m_tabCtrl.AdjustRect(FALSE, &tabRect);
    
    // Create list view for procedure flow - remove LVS_SINGLESEL to allow multiple selections
    m_procedureFlowView.Create(m_tabCtrl, tabRect, NULL,
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | WS_TABSTOP,
        WS_EX_CLIENTEDGE);

    // Create splitter window for Procedure Stats tab
    m_procedureStatsSplitter.Create(m_tabCtrl, tabRect, NULL,
        WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    m_procedureStatsSplitter.m_bFullDrag = true;

    // Create container panels with titles
    // IMPORTANT: Set title and control ID before Create()
    m_currentlyOpenPanel.SetTitle(L"Open Procedures");
    m_currentlyOpenPanel.SetControlId(IDC_CURRENTLY_OPEN_VIEW);
    m_currentlyOpenPanel.Create(m_procedureStatsSplitter, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

    m_procedureStatsPanel.SetTitle(L"Open/Close Count");
    m_procedureStatsPanel.SetControlId(IDC_PROCEDURE_STATS_VIEW);
    m_procedureStatsPanel.Create(m_procedureStatsSplitter, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

    // Set the splitter panes to use the container panels
    m_procedureStatsSplitter.SetSplitterPanes(m_currentlyOpenPanel, m_procedureStatsPanel);

    // Set splitter position AFTER setting panes (order matters!)
    // Use saved percentage position (default 50% = centered) with minimum width enforced
    int initialPos = ClampProcedureStatsSplitPct(m_procedureStatsSplitterPos);
    m_procedureStatsSplitterPos = initialPos;
    m_procedureStatsSplitter.SetSplitterPosPct(initialPos);

    // Get pointers to the list views in the panels
    if (m_currentlyOpenPanel.IsListViewCreated()) {
        m_currentlyOpenView = m_currentlyOpenPanel.GetListViewPtr();
    }
    if (m_procedureStatsPanel.IsListViewCreated()) {
        m_procedureStatsView = m_procedureStatsPanel.GetListViewPtr();
    }

    // Create timeline view
    m_timelineView.Create(m_tabCtrl, tabRect, NULL,
        WS_CHILD | WS_TABSTOP | WS_BORDER,
        0);
}

void CClarionProcedureView::InitializeProcedureFlowView()
{
    m_procedureFlowView.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    
    // Add columns for procedure flow
    m_procedureFlowView.InsertColumn(0, _T("Thread"), LVCFMT_LEFT, 60);
    m_procedureFlowView.InsertColumn(1, _T("Time"), LVCFMT_LEFT, 80);
    m_procedureFlowView.InsertColumn(2, _T("Procedure"), LVCFMT_LEFT, 200);  // Will include indentation
    m_procedureFlowView.InsertColumn(3, _T("Module"), LVCFMT_LEFT, 120);
    m_procedureFlowView.InsertColumn(4, _T("Status"), LVCFMT_LEFT, 70);
    m_procedureFlowView.InsertColumn(5, _T("Duration"), LVCFMT_RIGHT, 80);
}

void CClarionProcedureView::InitializeProcedureStatsViews()
{
    // Ensure we have valid pointers to the list views
    if (!m_currentlyOpenView && m_currentlyOpenPanel.IsListViewCreated()) {
        m_currentlyOpenView = m_currentlyOpenPanel.GetListViewPtr();
    }
    if (!m_procedureStatsView && m_procedureStatsPanel.IsListViewCreated()) {
        m_procedureStatsView = m_procedureStatsPanel.GetListViewPtr();
    }

    // Initialize Currently Open view (left grid)
    if (m_currentlyOpenView) {
        m_currentlyOpenView->SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        m_currentlyOpenView->InsertColumn(0, _T("Procedure"), LVCFMT_LEFT, 300);
        m_currentlyOpenView->InsertColumn(1, _T("Count"), LVCFMT_RIGHT, 100);
    }

    // Initialize Procedure Stats view (right grid)
    if (m_procedureStatsView) {
        m_procedureStatsView->SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        m_procedureStatsView->InsertColumn(0, _T("Procedure"), LVCFMT_LEFT, 250);
        m_procedureStatsView->InsertColumn(1, _T("Times Opened"), LVCFMT_RIGHT, 100);
        m_procedureStatsView->InsertColumn(2, _T("Times Closed"), LVCFMT_RIGHT, 100);
        m_procedureStatsView->InsertColumn(3, _T("Net"), LVCFMT_RIGHT, 80);
    }
}

void CClarionProcedureView::UpdateProcedureTree()
{
    // Update the currently visible tab
    int sel = m_tabCtrl.GetCurSel();
    if (sel == 0) {
        PopulateFlowFromTracker();  // Update Procedure Flow tab
    } else if (sel == 1) {
        PopulateProcedureStatsFromTracker();  // Update Procedure Stats tab
    } else if (sel == 2) {
        m_timelineView.RefreshView();  // Update Timeline View tab
    }
}

void CClarionProcedureView::AddProcedureMessage(const Message& /*msg*/)
{
    // This method is no longer used - procedure tracking is now done per-view
    // and the display is refreshed when switching tabs via SetTracker
    return;
}

void CClarionProcedureView::PopulateFlowFromTracker()
{
    // Clear existing items
    m_procedureFlowView.DeleteAllItems();
    
    // Force the list view to update after clearing
    m_procedureFlowView.InvalidateRect(nullptr);
    m_procedureFlowView.UpdateWindow();
    
    if (!m_tracker)
    {
        return;
    }
    
    // Get events from tracker
    auto events = m_tracker->GetChronologicalEvents();
    
    // For now, disable sorting to avoid duplicate issue
    // TODO: Fix sorting implementation
    
    // Display all events in chronological order
    int index = 0;
    for (const auto& event : events) {
        // Format procedure name with indentation
        CString procedureName;
        
        // Calculate indentation - ensure depth is at least 1
        int actualDepth = (event.depth > 0) ? event.depth : 1;
        CString indent;
        
        // Use non-breaking spaces (Unicode 0xA0) or dots for indentation
        // Since regular spaces get trimmed by list view
        int numIndents = (actualDepth - 1);
        for (int i = 0; i < numIndents; ++i) {
            indent += _T("  "); // Two spaces per level
        }
        
        // Use dots for visual indentation - this was working before
        CString prefix;
        if (numIndents > 0) {
            // Use dots to show indentation level visually  
            for (int i = 0; i < numIndents; ++i) {
                prefix += _T("..");  // Two dots per level
            }
        }
        
        // Sanitize the procedure name - remove any tabs that could break column layout
        CString cleanProcName(event.procedureName.c_str());
        cleanProcName.Replace(_T("\t"), _T(" ")); // Replace tabs with spaces
        cleanProcName.TrimLeft();  // Remove leading whitespace
        cleanProcName.TrimRight(); // Remove trailing whitespace
        
        // Add open/close indicator and procedure name
        if (event.isOpen) {
            procedureName.Format(_T("%s> %s"), (LPCTSTR)prefix, (LPCTSTR)cleanProcName);
        } else {
            procedureName.Format(_T("%s< %s"), (LPCTSTR)prefix, (LPCTSTR)cleanProcName);
        }
        
        
        // Insert row
        CString threadStr(event.threadId.c_str());
        int item = m_procedureFlowView.InsertItem(index, threadStr);
        
        // Format time
        CString timeStr;
        timeStr.Format(_T("%.3f"), event.timestamp);
        m_procedureFlowView.SetItemText(item, 1, timeStr);
        
        // Procedure name with indentation
        m_procedureFlowView.SetItemText(item, 2, procedureName);
        
        // Module
        CString moduleStr(event.module.c_str());
        m_procedureFlowView.SetItemText(item, 3, moduleStr);
        
        // Status
        CString statusStr = event.isOpen ? _T("Opening") : _T("Closing");
        m_procedureFlowView.SetItemText(item, 4, statusStr);
        
        // Duration (only for close events)
        if (!event.isOpen && event.duration > 0) {
            CString durationStr;
            durationStr.Format(_T("%.3f s"), event.duration);
            m_procedureFlowView.SetItemText(item, 5, durationStr);
        }
        
        index++;
    }
}

void CClarionProcedureView::PopulateProcedureStatsFromTracker()
{
    if (m_currentlyOpenView)
        m_currentlyOpenView->DeleteAllItems();
    if (m_procedureStatsView)
        m_procedureStatsView->DeleteAllItems();

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
        if (m_currentlyOpenView) {
            int item = m_currentlyOpenView->InsertItem(index, procName);
            CString countStr;
            countStr.Format(_T("%d"), count);
            m_currentlyOpenView->SetItemText(item, 1, countStr);
            index++;
        }
    }

    // Populate Procedure Stats view
    std::vector<std::tuple<CString, int, int, int>> statsItems;
    for (const auto& [procName, stats] : lifetimeStats) {
        int net = stats.openCount - stats.closeCount;
        statsItems.push_back({CString(procName.c_str()), stats.openCount, stats.closeCount, net});
    }

    // Sort based on the selected column
    if (m_procedureStatsSortColumn == 0) {
        // Sort by procedure name
        std::sort(statsItems.begin(), statsItems.end(),
            [this](const auto& a, const auto& b) {
                int cmp = std::get<0>(a).CompareNoCase(std::get<0>(b));
                return m_procedureStatsSortAscending ? (cmp < 0) : (cmp > 0);
            });
    } else if (m_procedureStatsSortColumn == 1) {
        // Sort by times opened with secondary sort by procedure name
        std::sort(statsItems.begin(), statsItems.end(),
            [this](const auto& a, const auto& b) {
                if (std::get<1>(a) != std::get<1>(b)) {
                    return m_procedureStatsSortAscending ?
                        (std::get<1>(a) < std::get<1>(b)) : (std::get<1>(a) > std::get<1>(b));
                }
                // Secondary sort by procedure name (always ascending)
                return std::get<0>(a).CompareNoCase(std::get<0>(b)) < 0;
            });
    } else if (m_procedureStatsSortColumn == 2) {
        // Sort by times closed with secondary sort by procedure name
        std::sort(statsItems.begin(), statsItems.end(),
            [this](const auto& a, const auto& b) {
                if (std::get<2>(a) != std::get<2>(b)) {
                    return m_procedureStatsSortAscending ?
                        (std::get<2>(a) < std::get<2>(b)) : (std::get<2>(a) > std::get<2>(b));
                }
                // Secondary sort by procedure name (always ascending)
                return std::get<0>(a).CompareNoCase(std::get<0>(b)) < 0;
            });
    } else if (m_procedureStatsSortColumn == 3) {
        // Sort by net with secondary sort by procedure name
        std::sort(statsItems.begin(), statsItems.end(),
            [this](const auto& a, const auto& b) {
                if (std::get<3>(a) != std::get<3>(b)) {
                    return m_procedureStatsSortAscending ?
                        (std::get<3>(a) < std::get<3>(b)) : (std::get<3>(a) > std::get<3>(b));
                }
                // Secondary sort by procedure name (always ascending)
                return std::get<0>(a).CompareNoCase(std::get<0>(b)) < 0;
            });
    }

    // Add items to Procedure Stats view
    index = 0;
    for (const auto& [procName, opened, closed, net] : statsItems) {
        if (m_procedureStatsView) {
            int item = m_procedureStatsView->InsertItem(index, procName);
            CString openedStr, closedStr, netStr;
            openedStr.Format(_T("%d"), opened);
            closedStr.Format(_T("%d"), closed);
            netStr.Format(_T("%d"), net);
            m_procedureStatsView->SetItemText(item, 1, openedStr);
            m_procedureStatsView->SetItemText(item, 2, closedStr);
            m_procedureStatsView->SetItemText(item, 3, netStr);
            index++;
        }
    }
}

void CClarionProcedureView::AddProcedureEvent(const ProcedureEvent& event)
{
    // Add a single event to the flow view
    int index = m_procedureFlowView.GetItemCount();
    
    // Format procedure name with indentation
    CString procedureName;
    
    // Calculate indentation - ensure depth is at least 1
    int actualDepth = (event.depth > 0) ? event.depth : 1;
    int numIndents = (actualDepth - 1);
    
    // Use dots for visual indentation - same as PopulateFlowFromTracker
    CString prefix;
    if (numIndents > 0) {
        for (int i = 0; i < numIndents; ++i) {
            prefix += _T("..");  // Two dots per level
        }
    }
    
    // Sanitize the procedure name - remove any tabs that could break column layout
    CString cleanProcName(event.procedureName.c_str());
    cleanProcName.Replace(_T("\t"), _T(" ")); // Replace tabs with spaces
    cleanProcName.TrimLeft();  // Remove leading whitespace
    cleanProcName.TrimRight(); // Remove trailing whitespace
    
    // Add open/close indicator and procedure name
    if (event.isOpen) {
        procedureName.Format(_T("%s> %s"), (LPCTSTR)prefix, (LPCTSTR)cleanProcName);
    } else {
        procedureName.Format(_T("%s< %s"), (LPCTSTR)prefix, (LPCTSTR)cleanProcName);
    }
    
    // Insert row
    CString threadStr(event.threadId.c_str());
    int item = m_procedureFlowView.InsertItem(index, threadStr);
    
    // Format time
    CString timeStr;
    timeStr.Format(_T("%.3f"), event.timestamp);
    m_procedureFlowView.SetItemText(item, 1, timeStr);
    
    // Procedure name with indentation
    m_procedureFlowView.SetItemText(item, 2, procedureName);
    
    // Module
    CString moduleStr(event.module.c_str());
    m_procedureFlowView.SetItemText(item, 3, moduleStr);
    
    // Status
    CString statusStr = event.isOpen ? _T("Opening") : _T("Closing");
    m_procedureFlowView.SetItemText(item, 4, statusStr);
    
    // Duration (only for close events)
    if (!event.isOpen && event.duration > 0) {
        CString durationStr;
        durationStr.Format(_T("%.3f s"), event.duration);
        m_procedureFlowView.SetItemText(item, 5, durationStr);
    }
}

// CProcedureStatsDialog implementation

LRESULT CProcedureStatsDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CenterWindow();
    
    // Display statistics
    CString text;
    text.Format(_T("Total Procedures Called: %d\n")
                _T("Currently Open: %d\n")
                _T("Maximum Call Depth: %d"),
                m_stats.totalProceduresCalled,
                m_stats.currentlyOpenProcedures,
                m_stats.maxCallDepth);
    
    // SetDlgItemText(IDC_STATS_TEXT, text); // TODO: Add proper resource
    
    return TRUE;
}

LRESULT CProcedureStatsDialog::OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(IDOK);
    return 0;
}

LRESULT CProcedureStatsDialog::OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(IDCANCEL);
    return 0;
}

void CProcedureStatsDialog::SetStatistics(const ClarionProcedureTracker::Statistics& stats)
{
    m_stats = stats;
}

void CClarionProcedureView::SortCurrentlyOpenView(int column)
{
    if (column == m_currentlyOpenSortColumn) {
        // Toggle sort order
        m_currentlyOpenSortAscending = !m_currentlyOpenSortAscending;
    } else {
        // New column, default to ascending
        m_currentlyOpenSortColumn = column;
        m_currentlyOpenSortAscending = true;
    }

    // Repopulate to apply new sort
    PopulateProcedureStatsFromTracker();
}

void CClarionProcedureView::SortProcedureStatsView(int column)
{
    if (column == m_procedureStatsSortColumn) {
        // Toggle sort order
        m_procedureStatsSortAscending = !m_procedureStatsSortAscending;
    } else {
        // New column, default to ascending
        m_procedureStatsSortColumn = column;
        m_procedureStatsSortAscending = true;
    }

    // Repopulate to apply new sort
    PopulateProcedureStatsFromTracker();
}

} // namespace debugviewpp
} // namespace fusion




