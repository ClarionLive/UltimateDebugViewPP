// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "ClarionProcedureTimelineView.h"
#include "MainFrame.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <set>
#include <atlstr.h>
#include <atlmisc.h>
#include <windowsx.h>

namespace debugviewpp {

// Static member definitions
const double ClarionProcedureTimelineView::MIN_ZOOM = 0.1;
const double ClarionProcedureTimelineView::MAX_ZOOM = 10.0;
const double ClarionProcedureTimelineView::ZOOM_STEP = 1.2;

ClarionProcedureTimelineView::ClarionProcedureTimelineView()
    : m_scrollX(0)
    , m_scrollY(0)
    , m_zoomLevel(1.0)
    , m_isDragging(false)
    , m_selectedCell(nullptr)
    , m_hoveredCell(nullptr)
    , m_contextMenuEvent(nullptr)
    , m_hdcMem(nullptr)
    , m_hbmMem(nullptr)
    , m_hbmOld(nullptr)
    , m_bufferWidth(0)
    , m_bufferHeight(0)
    , m_fontNormal(nullptr)
    , m_fontBold(nullptr)
    , m_penGrid(nullptr)
    , m_penSelection(nullptr)
    , m_brushBackground(nullptr)
{
    // Initialize metrics with defaults
    m_metrics.headerHeight = HEADER_HEIGHT;
    m_metrics.timeAxisWidth = TIME_AXIS_WIDTH;
    m_metrics.cellHeight = MIN_CELL_HEIGHT;  // Will be updated based on font
    m_metrics.cellWidth = MIN_CELL_WIDTH;
    m_metrics.visibleRows = 0;
    m_metrics.visibleCols = 0;
    m_metrics.pixelsPerSecond = 100.0;
    m_metrics.startTime = 0.0;
    m_metrics.endTime = 10.0;
    m_metrics.timeRange = 10.0;

    m_dragStart.x = m_dragStart.y = 0;
    m_dragOffset.x = m_dragOffset.y = 0;
}

ClarionProcedureTimelineView::~ClarionProcedureTimelineView()
{
    DestroyDoubleBuffer();

    if (m_fontNormal) DeleteObject(m_fontNormal);
    if (m_fontBold) DeleteObject(m_fontBold);
    if (m_penGrid) DeleteObject(m_penGrid);
    if (m_penSelection) DeleteObject(m_penSelection);
    if (m_brushBackground) DeleteObject(m_brushBackground);
}

void ClarionProcedureTimelineView::SetTracker(std::shared_ptr<ClarionProcedureTracker> tracker)
{
    m_tracker = tracker;
    ProcessTrackerData();
    RefreshView();
}

void ClarionProcedureTimelineView::RefreshView()
{
    if (IsWindow())
    {
        ProcessTrackerData();
        UpdateViewMetrics();
        UpdateScrollBars();
        Invalidate();
    }
}

void ClarionProcedureTimelineView::Clear()
{
    m_timeSlices.clear();
    m_threadIds.clear();
    m_threadIndexMap.clear();
    m_gridCells.clear();
    m_columnWidths.clear();
    m_selectedCell = nullptr;
    m_hoveredCell = nullptr;
    m_selectedProcedure.clear();

    if (IsWindow())
    {
        Invalidate();
    }
}

LRESULT ClarionProcedureTimelineView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // Create scrollbars
    m_hScrollBar.Create(m_hWnd, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | SBS_HORZ, 0, 100);
    m_vScrollBar.Create(m_hWnd, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | SBS_VERT, 0, 101);

    // Create tooltip control
    m_tooltip.Create(m_hWnd, NULL, NULL, TTS_NOPREFIX | TTS_ALWAYSTIP);
    m_tooltip.SetMaxTipWidth(500);
    m_tooltip.Activate(TRUE);

    // Add tool for the entire window
    CToolInfo ti(TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE, m_hWnd);
    m_tooltip.AddTool(&ti);

    // Create the configured font (Consolas 12pt or whatever is configured)
    LOGFONT lf = {0};

    // Convert point size to logical units
    // For 12pt font at 96 DPI: height = -12 * 96 / 72 = -16
    HDC hdc = GetDC();
    int dpi = GetDeviceCaps(hdc, LOGPIXELSY);

    // Configure for Consolas 12pt (or could be made configurable)
    lf.lfHeight = -MulDiv(12, dpi, 72);  // 12 point size
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;
    lstrcpy(lf.lfFaceName, _T("Consolas"));

    // Create normal font
    m_fontNormal = CreateFontIndirect(&lf);

    // Create bold version
    lf.lfWeight = FW_BOLD;
    m_fontBold = CreateFontIndirect(&lf);

    // Calculate row height based on font size
    HFONT oldFont = (HFONT)SelectObject(hdc, m_fontNormal);
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);

    // Set cell height with generous padding for breathing room
    m_metrics.cellHeight = tm.tmHeight + tm.tmExternalLeading + 8;  // More vertical padding

    // Calculate time axis width based on font - needs room for "HH:MM:SS.mmm"
    SIZE textSize;
    CString sampleTime(_T("00:00:00.000"));
    GetTextExtentPoint32(hdc, sampleTime, sampleTime.GetLength(), &textSize);
    m_metrics.timeAxisWidth = textSize.cx + 40;  // Add generous padding for visibility

    SelectObject(hdc, oldFont);
    ReleaseDC(hdc);

    // Create drawing objects
    m_penGrid = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    m_penSelection = CreatePen(PS_SOLID, 2, RGB(51, 153, 255));
    m_brushBackground = CreateSolidBrush(RGB(255, 255, 255));

    // Initialize view
    UpdateViewMetrics();
    CreateDoubleBuffer();

    // Start refresh timer
    SetTimer(TIMER_REFRESH, 1000);

    return 0;
}

LRESULT ClarionProcedureTimelineView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    int width = LOWORD(lParam);
    int height = HIWORD(lParam);

    if (width > 0 && height > 0)
    {
        // Position scrollbars
        m_hScrollBar.SetWindowPos(NULL, 0, height - SCROLLBAR_SIZE,
            width - SCROLLBAR_SIZE, SCROLLBAR_SIZE, SWP_NOZORDER);
        m_vScrollBar.SetWindowPos(NULL, width - SCROLLBAR_SIZE, 0,
            SCROLLBAR_SIZE, height - SCROLLBAR_SIZE, SWP_NOZORDER);

        UpdateViewMetrics();
        CreateDoubleBuffer();
        UpdateScrollBars();
    }

    return 0;
}

LRESULT ClarionProcedureTimelineView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    KillTimer(TIMER_REFRESH);
    DestroyDoubleBuffer();
    return 0;
}

LRESULT ClarionProcedureTimelineView::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(&ps);

    if (m_hdcMem)
    {
        DrawGrid(m_hdcMem);
        BitBlt(hdc, 0, 0, m_bufferWidth, m_bufferHeight, m_hdcMem, 0, 0, SRCCOPY);
    }
    else
    {
        DrawGrid(hdc);
    }

    EndPaint(&ps);
    return 0;
}

LRESULT ClarionProcedureTimelineView::OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // Don't erase background to prevent flicker
    return TRUE;
}

LRESULT ClarionProcedureTimelineView::OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

    m_isDragging = true;
    m_dragStart = pt;
    m_dragOffset.x = m_scrollX;
    m_dragOffset.y = m_scrollY;

    SetCapture();

    // Check if we clicked on a cell
    GridCell* cell = GetCellAtPoint(pt);
    if (cell != m_selectedCell)
    {
        m_selectedCell = cell;
        if (cell && !cell->procedures.empty())
        {
            m_selectedProcedure = cell->procedures[0].procedureName;
        }
        else
        {
            m_selectedProcedure.clear();
        }
        Invalidate();
    }

    return 0;
}

LRESULT ClarionProcedureTimelineView::OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (m_isDragging)
    {
        m_isDragging = false;
        ReleaseCapture();
    }
    return 0;
}

LRESULT ClarionProcedureTimelineView::OnRButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    GridCell* cell = GetCellAtPoint(pt);

    if (cell)
    {
        m_selectedCell = cell;
        Invalidate();
    }

    return 0;
}

LRESULT ClarionProcedureTimelineView::OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

    if (m_isDragging)
    {
        // Calculate drag offset and update scroll position
        int deltaX = pt.x - m_dragStart.x;
        int deltaY = pt.y - m_dragStart.y;

        m_scrollX = std::max(0, static_cast<int>(m_dragOffset.x - deltaX));
        m_scrollY = std::max(0, static_cast<int>(m_dragOffset.y - deltaY));

        UpdateScrollBars();
        Invalidate();
    }
    else
    {
        // Handle hover
        GridCell* cell = GetCellAtPoint(pt);
        if (cell != m_hoveredCell)
        {
            m_hoveredCell = cell;

            if (cell && !cell->procedures.empty())
            {
                // Show enhanced tooltip with comprehensive information
                const auto& proc = cell->procedures[0];
                std::ostringstream tooltip;

                // Thread information
                tooltip << "Thread: " << proc.threadId << "\n";

                // Procedure name and module
                tooltip << "Procedure: " << proc.procedureName << "\n";
                tooltip << "Module: " << proc.module << "\n";

                // Event type
                tooltip << "Event: " << (proc.isOpen ? "Open" : "Close") << "\n";

                // Timestamp in HH:MM:SS.mmm format
                tooltip << "Time: " << FormatTimeLabel(proc.timestamp) << "\n";

                // For close events, show elapsed time
                if (!proc.isOpen && proc.duration > 0)
                {
                    tooltip << "Elapsed: " << std::fixed << std::setprecision(3)
                           << proc.duration << "ms\n";
                }

                // Call depth and caller information
                tooltip << "Depth: " << proc.depth;
                if (proc.depth > 0)
                {
                    tooltip << " (Called from nested context)";
                }

                ShowTooltip(pt, tooltip.str());
            }
            else
            {
                HideTooltip();
            }
        }
    }

    return 0;
}

LRESULT ClarionProcedureTimelineView::OnMouseWheel(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    int delta = GET_WHEEL_DELTA_WPARAM(wParam);
    bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

    if (ctrlPressed)
    {
        // Zoom
        double factor = (delta > 0) ? ZOOM_STEP : (1.0 / ZOOM_STEP);
        double newZoom = m_zoomLevel * factor;

        if (newZoom >= MIN_ZOOM && newZoom <= MAX_ZOOM)
        {
            m_zoomLevel = newZoom;
            UpdateViewMetrics();
            UpdateScrollBars();
            Invalidate();
        }
    }
    else
    {
        // Scroll vertically
        int scrollDelta = (delta > 0) ? -3 : 3;
        m_scrollY = std::max(0, m_scrollY + scrollDelta * m_metrics.cellHeight);
        UpdateScrollBars();
        Invalidate();
    }

    return 0;
}

LRESULT ClarionProcedureTimelineView::OnHScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    SCROLLINFO si = {0};
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    m_hScrollBar.GetScrollInfo(&si);

    int oldPos = si.nPos;

    switch (LOWORD(wParam))
    {
    case SB_LEFT:
        si.nPos = si.nMin;
        break;
    case SB_RIGHT:
        si.nPos = si.nMax;
        break;
    case SB_LINELEFT:
        {
            int scrollStep = m_columnWidths.empty() ? m_metrics.cellWidth :
                           m_columnWidths[0]; // Use first column width as step
            si.nPos = std::max(si.nMin, si.nPos - scrollStep);
        }
        break;
    case SB_LINERIGHT:
        {
            int scrollStep = m_columnWidths.empty() ? m_metrics.cellWidth :
                           m_columnWidths[0]; // Use first column width as step
            si.nPos = std::min(si.nMax, si.nPos + scrollStep);
        }
        break;
    case SB_PAGELEFT:
        si.nPos = std::max(si.nMin, si.nPos - static_cast<int>(si.nPage));
        break;
    case SB_PAGERIGHT:
        si.nPos = std::min(si.nMax, si.nPos + static_cast<int>(si.nPage));
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        si.nPos = si.nTrackPos;
        break;
    }

    if (si.nPos != oldPos)
    {
        m_scrollX = si.nPos;
        m_hScrollBar.SetScrollPos(si.nPos);
        Invalidate();
    }

    return 0;
}

LRESULT ClarionProcedureTimelineView::OnVScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    SCROLLINFO si = {0};
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    m_vScrollBar.GetScrollInfo(&si);

    int oldPos = si.nPos;

    switch (LOWORD(wParam))
    {
    case SB_TOP:
        si.nPos = si.nMin;
        break;
    case SB_BOTTOM:
        si.nPos = si.nMax;
        break;
    case SB_LINEUP:
        si.nPos = std::max(si.nMin, si.nPos - static_cast<int>(m_metrics.cellHeight));
        break;
    case SB_LINEDOWN:
        si.nPos = std::min(si.nMax, si.nPos + static_cast<int>(m_metrics.cellHeight));
        break;
    case SB_PAGEUP:
        si.nPos = std::max(si.nMin, si.nPos - static_cast<int>(si.nPage));
        break;
    case SB_PAGEDOWN:
        si.nPos = std::min(si.nMax, si.nPos + static_cast<int>(si.nPage));
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        si.nPos = si.nTrackPos;
        break;
    }

    if (si.nPos != oldPos)
    {
        m_scrollY = si.nPos;
        m_vScrollBar.SetScrollPos(si.nPos);
        Invalidate();
    }

    return 0;
}

LRESULT ClarionProcedureTimelineView::OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    // Hide any visible tooltip first
    HideTooltip();

    POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

    // Convert screen coordinates to client coordinates
    POINT clientPt = pt;
    ScreenToClient(&clientPt);

    // Get the cell at this point
    GridCell* cell = GetCellAtPoint(clientPt);
    if (!cell || cell->procedures.empty())
    {
        m_contextMenuEvent.reset();
        return 0;
    }

    // Store the selected procedure info for menu handlers
    m_contextMenuEvent = std::make_unique<ClarionProcedureTracker::Event>(cell->procedures[0]);

    // Create popup menu
    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, ID_TIMELINE_CREATE_VIEW, _T("Create View for Procedure"));
    menu.AppendMenu(MF_STRING, ID_TIMELINE_JUMP_TO_ROW, _T("Jump to Debug Row"));

    // Show menu at screen coordinates
    menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

    return 0;
}

LRESULT ClarionProcedureTimelineView::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (wParam == TIMER_REFRESH)
    {
        RefreshView();
    }
    return 0;
}

void ClarionProcedureTimelineView::ProcessTrackerData()
{
    if (!m_tracker)
    {
        Clear();
        return;
    }

    // Get all events from tracker
    auto events = m_tracker->GetChronologicalEvents();
    if (events.empty())
    {
        Clear();
        return;
    }

    // Extract thread IDs
    std::set<std::string> threadSet;
    for (const auto& event : events)
    {
        threadSet.insert(event.threadId);
    }

    m_threadIds.assign(threadSet.begin(), threadSet.end());
    std::sort(m_threadIds.begin(), m_threadIds.end());

    // Build thread index map
    m_threadIndexMap.clear();
    for (size_t i = 0; i < m_threadIds.size(); ++i)
    {
        m_threadIndexMap[m_threadIds[i]] = static_cast<int>(i);
    }

    // Calculate time range
    m_metrics.startTime = events.front().timestamp;
    m_metrics.endTime = events.back().timestamp;
    m_metrics.timeRange = m_metrics.endTime - m_metrics.startTime;

    if (m_metrics.timeRange <= 0)
        m_metrics.timeRange = 1.0;

    CalculateTimeSlices();
}

void ClarionProcedureTimelineView::CalculateTimeSlices()
{
    if (!m_tracker || m_threadIds.empty())
        return;

    m_timeSlices.clear();

    // Get all events from tracker
    auto events = m_tracker->GetChronologicalEvents();
    if (events.empty())
        return;

    // Collect all unique timestamps where events occur
    std::set<double> uniqueTimestamps;
    for (const auto& event : events)
    {
        uniqueTimestamps.insert(event.timestamp);
    }

    // Create time slices only for timestamps where events occur
    std::vector<double> sortedTimestamps(uniqueTimestamps.begin(), uniqueTimestamps.end());

    for (size_t i = 0; i < sortedTimestamps.size(); ++i)
    {
        TimeSlice slice;
        slice.startTime = sortedTimestamps[i];

        // Set end time to the next timestamp, or slightly after current timestamp if it's the last one
        if (i + 1 < sortedTimestamps.size())
        {
            slice.endTime = sortedTimestamps[i + 1];
        }
        else
        {
            // For the last timestamp, set a minimal end time
            slice.endTime = slice.startTime + 0.001;
        }

        m_timeSlices.push_back(slice);
    }

    // Fill time slices with procedure events
    // Since we created slices based on exact timestamps, we can directly match events
    for (const auto& event : events)
    {
        // Find the time slice for this event's exact timestamp
        auto it = std::lower_bound(m_timeSlices.begin(), m_timeSlices.end(), event.timestamp,
            [](const TimeSlice& slice, double time) {
                return slice.startTime < time;
            });

        if (it != m_timeSlices.end() && it->startTime == event.timestamp)
        {
            it->threadProcedures[event.threadId].push_back(event);
        }
    }

    // Build grid cells
    m_gridCells.clear();
    for (size_t row = 0; row < m_timeSlices.size(); ++row)
    {
        for (size_t col = 0; col < m_threadIds.size(); ++col)
        {
            GridCell cell;
            cell.row = static_cast<int>(row);
            cell.col = static_cast<int>(col);

            const std::string& threadId = m_threadIds[col];
            const auto& slice = m_timeSlices[row];

            auto threadIt = slice.threadProcedures.find(threadId);
            if (threadIt != slice.threadProcedures.end())
            {
                cell.procedures = threadIt->second;
            }

            m_gridCells.push_back(cell);
        }
    }
}

void ClarionProcedureTimelineView::DrawGrid(HDC hdc)
{
    RECT clientRect;
    GetClientRect(&clientRect);

    // Fill background
    FillRect(hdc, &clientRect, m_brushBackground);

    // Calculate visible area
    RECT timeAxisRect = {0, 0, m_metrics.timeAxisWidth, clientRect.bottom - SCROLLBAR_SIZE};
    RECT headerRect = {m_metrics.timeAxisWidth, 0, clientRect.right - SCROLLBAR_SIZE, m_metrics.headerHeight};
    RECT gridRect = {m_metrics.timeAxisWidth, m_metrics.headerHeight,
                     clientRect.right - SCROLLBAR_SIZE, clientRect.bottom - SCROLLBAR_SIZE};

    // Draw components
    DrawTimeAxis(hdc, timeAxisRect);
    DrawThreadHeaders(hdc, headerRect);
    DrawGridCells(hdc, gridRect);

    // Draw selection
    if (m_selectedCell)
    {
        HPEN oldPen = (HPEN)SelectObject(hdc, m_penSelection);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

        Rectangle(hdc, m_selectedCell->bounds.left, m_selectedCell->bounds.top,
                  m_selectedCell->bounds.right, m_selectedCell->bounds.bottom);

        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
    }
}

void ClarionProcedureTimelineView::DrawTimeAxis(HDC hdc, const RECT& rect)
{
    HFONT oldFont = (HFONT)SelectObject(hdc, m_fontNormal);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));

    // Draw time labels
    int labelHeight = 20;
    int visibleHeight = rect.bottom - rect.top - m_metrics.headerHeight;
    int totalRows = static_cast<int>(m_timeSlices.size());

    for (int i = 0; i < totalRows; ++i)
    {
        int y = m_metrics.headerHeight + i * m_metrics.cellHeight - m_scrollY;

        if (y + m_metrics.cellHeight < m_metrics.headerHeight || y > rect.bottom)
            continue;

        if (i < static_cast<int>(m_timeSlices.size()))
        {
            std::string timeStr = FormatTimeLabel(m_timeSlices[i].startTime);
            RECT labelRect = {rect.left + 5, y, rect.right - 5, y + m_metrics.cellHeight};

            CString text(timeStr.c_str());
            DrawText(hdc, text, -1, &labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
    }

    SelectObject(hdc, oldFont);
}

void ClarionProcedureTimelineView::DrawThreadHeaders(HDC hdc, const RECT& rect)
{
    HFONT oldFont = (HFONT)SelectObject(hdc, m_fontBold);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));

    // Draw thread column headers
    int currentX = 0;
    for (size_t i = 0; i < m_threadIds.size(); ++i)
    {
        int columnWidth = (i < m_columnWidths.size() && !m_columnWidths.empty()) ?
                         m_columnWidths[i] : m_metrics.cellWidth;
        int x = currentX - m_scrollX;

        if (x + columnWidth < 0 || x > rect.right - rect.left)
        {
            currentX += columnWidth;
            continue;
        }

        RECT headerRect = {rect.left + x, rect.top,
                          rect.left + x + columnWidth, rect.bottom};

        // Draw header background
        FillRect(hdc, &headerRect, (HBRUSH)GetStockObject(LTGRAY_BRUSH));

        // Draw header border
        HPEN oldPen = (HPEN)SelectObject(hdc, m_penGrid);
        MoveToEx(hdc, headerRect.right - 1, headerRect.top, NULL);
        LineTo(hdc, headerRect.right - 1, headerRect.bottom);
        SelectObject(hdc, oldPen);

        // Draw thread ID
        std::string threadText = "Thread " + m_threadIds[i];
        std::string truncatedText = TruncateText(threadText, columnWidth - 10, hdc);

        RECT textRect = headerRect;
        InflateRect(&textRect, -5, 0);

        CString text(truncatedText.c_str());
        DrawText(hdc, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        currentX += columnWidth;
    }

    SelectObject(hdc, oldFont);
}

void ClarionProcedureTimelineView::DrawGridCells(HDC hdc, const RECT& rect)
{
    HPEN oldPen = (HPEN)SelectObject(hdc, m_penGrid);

    // Calculate column positions
    std::vector<int> columnPositions;
    columnPositions.push_back(0);
    for (size_t i = 0; i < m_columnWidths.size(); ++i)
    {
        columnPositions.push_back(columnPositions.back() + m_columnWidths[i]);
    }

    // Update cell bounds and draw cells
    for (auto& cell : m_gridCells)
    {
        int columnWidth = (cell.col < static_cast<int>(m_columnWidths.size()) && !m_columnWidths.empty()) ?
                         m_columnWidths[cell.col] : m_metrics.cellWidth;
        int x = (cell.col < static_cast<int>(columnPositions.size())) ?
                columnPositions[cell.col] : cell.col * m_metrics.cellWidth;
        x -= m_scrollX;
        int y = cell.row * m_metrics.cellHeight - m_scrollY;

        cell.bounds.left = rect.left + x;
        cell.bounds.top = rect.top + y;
        cell.bounds.right = cell.bounds.left + columnWidth;
        cell.bounds.bottom = cell.bounds.top + m_metrics.cellHeight;

        // Skip if not visible
        if (cell.bounds.right < rect.left || cell.bounds.left > rect.right ||
            cell.bounds.bottom < rect.top || cell.bounds.top > rect.bottom)
            continue;

        // Draw cell border
        Rectangle(hdc, cell.bounds.left, cell.bounds.top,
                  cell.bounds.right, cell.bounds.bottom);

        // Draw procedure blocks in cell
        if (!cell.procedures.empty())
        {
            // Sort procedures by depth for proper layering
            auto procedures = cell.procedures;
            std::sort(procedures.begin(), procedures.end(),
                [](const fusion::debugviewpp::ClarionProcedureTracker::Event& a,
                   const fusion::debugviewpp::ClarionProcedureTracker::Event& b) {
                    return a.depth < b.depth;
                });

            for (const auto& proc : procedures)
            {
                DrawProcedureBlock(hdc, cell.bounds, proc);
            }
        }
    }

    SelectObject(hdc, oldPen);
}

void ClarionProcedureTimelineView::DrawProcedureBlock(HDC hdc, const RECT& cellRect,
    const fusion::debugviewpp::ClarionProcedureTracker::Event& proc)
{
    // Calculate block position within cell based on depth
    RECT blockRect = cellRect;
    InflateRect(&blockRect, -2, -2);

    // Check if this looks like a truly nested call (depth > 1)
    // Depth 0 = top level, Depth 1 = also treat as top level for display
    bool isVisuallyNested = proc.depth > 1;

    // Only indent truly nested calls
    if (isVisuallyNested)
    {
        int adjustedDepth = proc.depth - 1;  // Subtract 1 for visual purposes
        int depthOffset = adjustedDepth * 15;  // 15 pixels per depth level
        blockRect.left += depthOffset;
    }

    if (blockRect.left >= blockRect.right || blockRect.top >= blockRect.bottom)
        return;

    // Draw connector line ONLY for visually nested procedures
    if (isVisuallyNested && proc.isOpen)
    {
        HPEN connectorPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
        HPEN oldPen = (HPEN)SelectObject(hdc, connectorPen);

        // Draw L-shaped connector: vertical line then horizontal with arrow
        int adjustedDepth = proc.depth - 1;
        int lineX = cellRect.left + 2 + (adjustedDepth - 1) * 15 + 10;
        int lineY = blockRect.top + (blockRect.bottom - blockRect.top) / 2;

        // Vertical line from previous level
        MoveToEx(hdc, lineX, cellRect.top, NULL);
        LineTo(hdc, lineX, lineY);

        // Horizontal line to current block
        LineTo(hdc, blockRect.left - 2, lineY);

        // Draw arrow head (>)
        MoveToEx(hdc, blockRect.left - 6, lineY - 3, NULL);
        LineTo(hdc, blockRect.left - 2, lineY);
        LineTo(hdc, blockRect.left - 6, lineY + 3);

        SelectObject(hdc, oldPen);
        DeleteObject(connectorPen);
    }

    // Get procedure color - use same color for both open and close
    COLORREF color = GetProcedureColor(proc.procedureName);
    HBRUSH brush = CreateSolidBrush(color);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

    // Fill block
    FillRect(hdc, &blockRect, brush);

    // Draw border with different style for open/close
    HPEN borderPen;
    if (proc.isOpen)
    {
        // Solid border for open events
        borderPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    }
    else
    {
        // Dashed border for close events
        borderPen = CreatePen(PS_DOT, 1, RGB(0, 0, 0));
    }

    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    HBRUSH hollowBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, blockRect.left, blockRect.top, blockRect.right, blockRect.bottom);
    SelectObject(hdc, hollowBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);

    // Draw procedure name with open/close indicator
    HFONT oldFont = (HFONT)SelectObject(hdc, m_fontNormal);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));

    // Add prefix indicator
    std::string indicator = proc.isOpen ? "> " : "< ";
    std::string displayName = indicator + proc.procedureName;

    std::string truncatedName = TruncateText(displayName,
        blockRect.right - blockRect.left - 4, hdc);

    RECT textRect = blockRect;
    InflateRect(&textRect, -2, 0);

    CString text(truncatedName.c_str());
    DrawText(hdc, text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
    SelectObject(hdc, oldBrush);
    DeleteObject(brush);
}

void ClarionProcedureTimelineView::UpdateScrollBars()
{
    RECT clientRect;
    GetClientRect(&clientRect);

    int clientWidth = clientRect.right - m_metrics.timeAxisWidth - SCROLLBAR_SIZE;
    int clientHeight = clientRect.bottom - m_metrics.headerHeight - SCROLLBAR_SIZE;

    // Calculate total width using dynamic column widths
    int totalWidth = 0;
    for (int width : m_columnWidths)
    {
        totalWidth += width;
    }
    // Fallback to old calculation if no dynamic widths
    if (totalWidth == 0)
    {
        totalWidth = static_cast<int>(m_threadIds.size()) * m_metrics.cellWidth;
    }

    int totalHeight = static_cast<int>(m_timeSlices.size()) * m_metrics.cellHeight;

    // Update horizontal scrollbar
    SCROLLINFO si = {0};
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    si.nMin = 0;
    si.nMax = std::max(0, totalWidth - clientWidth);
    si.nPage = clientWidth;
    si.nPos = m_scrollX;
    m_hScrollBar.SetScrollInfo(&si);

    // Update vertical scrollbar
    si.nMax = std::max(0, totalHeight - clientHeight);
    si.nPage = clientHeight;
    si.nPos = m_scrollY;
    m_vScrollBar.SetScrollInfo(&si);
}

void ClarionProcedureTimelineView::UpdateViewMetrics()
{
    RECT clientRect;
    GetClientRect(&clientRect);

    // Calculate dynamic column widths first
    CalculateColumnWidths();

    // Keep the font-based cell height (don't override it)
    // Cell height was already set based on font metrics in OnCreate

    m_metrics.cellWidth = static_cast<int>(MIN_CELL_WIDTH * m_zoomLevel);
    m_metrics.cellWidth = std::max(static_cast<int>(MIN_CELL_WIDTH),
                                  std::min(m_metrics.cellWidth, static_cast<int>(MAX_CELL_WIDTH)));

    m_metrics.pixelsPerSecond = m_metrics.cellWidth / (m_metrics.timeRange /
                                std::max(1.0, static_cast<double>(m_timeSlices.size())));

    // Calculate total width of all columns for scroll calculations
    int totalColumnsWidth = 0;
    for (int width : m_columnWidths)
    {
        totalColumnsWidth += width;
    }

    int avgColumnWidth = m_metrics.cellWidth;
    if (!m_columnWidths.empty() && totalColumnsWidth > 0)
    {
        avgColumnWidth = totalColumnsWidth / static_cast<int>(m_columnWidths.size());
    }
    m_metrics.visibleCols = (clientRect.right - m_metrics.timeAxisWidth) / avgColumnWidth + 1;
    m_metrics.visibleRows = (clientRect.bottom - m_metrics.headerHeight) / m_metrics.cellHeight + 1;
}

void ClarionProcedureTimelineView::CalculateColumnWidths()
{
    if (m_threadIds.empty())
    {
        m_columnWidths.clear();
        return;
    }

    // Resize column widths vector to match number of threads
    m_columnWidths.resize(m_threadIds.size());

    // Get HDC for text measurement
    HDC hdc = GetDC();
    if (!hdc)
        return;

    // Select font for measurement (if available)
    HFONT oldFont = nullptr;
    if (m_fontNormal)
    {
        oldFont = (HFONT)SelectObject(hdc, m_fontNormal);
    }

    // Get font metrics to scale column widths appropriately
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    int fontScale = std::max(1, static_cast<int>(tm.tmHeight / 12));  // Scale factor based on font height

    for (size_t threadIndex = 0; threadIndex < m_threadIds.size(); ++threadIndex)
    {
        // Scale minimum width based on font size
        int minWidth = MIN_COLUMN_WIDTH + (fontScale - 1) * 30;
        int maxWidth = minWidth;
        const std::string& threadId = m_threadIds[threadIndex];

        // Calculate width for thread header text
        std::string threadHeaderText = "Thread " + threadId;
        int headerWidth = CalculateTextWidth(threadHeaderText, hdc) + 40;  // More padding
        maxWidth = std::max(maxWidth, headerWidth);

        // Find all procedure names for this thread
        for (const auto& slice : m_timeSlices)
        {
            auto threadIt = slice.threadProcedures.find(threadId);
            if (threadIt != slice.threadProcedures.end())
            {
                for (const auto& proc : threadIt->second)
                {
                    // Calculate width for procedure name with prefix indicator and indentation
                    std::string indicator = proc.isOpen ? "> " : "< ";
                    std::string displayName = indicator + proc.procedureName;

                    // Account for indentation in width calculation
                    int indentWidth = (proc.depth > 0) ? proc.depth * 15 : 0;
                    int textWidth = CalculateTextWidth(displayName, hdc) + indentWidth + 50;  // More generous padding
                    maxWidth = std::max(maxWidth, textWidth);
                }
            }
        }

        // Apply scaled min/max limits with more generous max
        int scaledMaxWidth = MAX_COLUMN_WIDTH + (fontScale - 1) * 60;
        maxWidth = std::max(minWidth, std::min(maxWidth, scaledMaxWidth));
        m_columnWidths[threadIndex] = maxWidth;
    }

    if (oldFont)
    {
        SelectObject(hdc, oldFont);
    }
    ReleaseDC(hdc);
}

int ClarionProcedureTimelineView::CalculateTextWidth(const std::string& text, HDC hdc)
{
    if (text.empty())
        return 0;

    CString ctext(text.c_str());
    SIZE textSize;
    if (GetTextExtentPoint32(hdc, ctext, ctext.GetLength(), &textSize))
    {
        return textSize.cx;
    }
    return 0;
}

ClarionProcedureTimelineView::GridCell* ClarionProcedureTimelineView::GetCellAtPoint(const POINT& pt)
{
    for (auto& cell : m_gridCells)
    {
        if (PtInRect(&cell.bounds, pt))
        {
            return &cell;
        }
    }
    return nullptr;
}

COLORREF ClarionProcedureTimelineView::GetProcedureColor(const std::string& procedureName)
{
    auto it = m_procedureColors.find(procedureName);
    if (it != m_procedureColors.end())
    {
        return it->second;
    }

    // Generate a color based on procedure name hash
    size_t hash = std::hash<std::string>{}(procedureName);
    COLORREF colors[] = {
        RGB(255, 200, 200), RGB(200, 255, 200), RGB(200, 200, 255),
        RGB(255, 255, 200), RGB(255, 200, 255), RGB(200, 255, 255),
        RGB(255, 220, 180), RGB(220, 255, 180), RGB(220, 180, 255),
        RGB(180, 255, 220), RGB(180, 220, 255), RGB(255, 180, 220)
    };

    COLORREF color = colors[hash % (sizeof(colors) / sizeof(colors[0]))];
    m_procedureColors[procedureName] = color;
    return color;
}

std::string ClarionProcedureTimelineView::FormatTimeLabel(double time)
{
    // Convert the relative time to HH:MM:SS.mmm format
    // Assuming 'time' is seconds since the start of logging

    // Get the total seconds and split into hours, minutes, seconds, and milliseconds
    double totalSeconds = time;

    // Extract hours, minutes, and seconds
    int hours = static_cast<int>(totalSeconds) / 3600;
    int minutes = (static_cast<int>(totalSeconds) % 3600) / 60;
    int seconds = static_cast<int>(totalSeconds) % 60;

    // Extract milliseconds from the fractional part
    int milliseconds = static_cast<int>((totalSeconds - static_cast<int>(totalSeconds)) * 1000);

    // Format as HH:MM:SS.mmm
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << hours << ":"
        << std::setw(2) << minutes << ":"
        << std::setw(2) << seconds << "."
        << std::setw(3) << milliseconds;

    return oss.str();
}

std::string ClarionProcedureTimelineView::TruncateText(const std::string& text, int maxWidth, HDC hdc)
{
    if (text.empty()) return text;

    CString ctext(text.c_str());
    SIZE textSize;
    GetTextExtentPoint32(hdc, ctext, ctext.GetLength(), &textSize);

    if (textSize.cx <= maxWidth)
        return text;

    // Binary search for best fit
    int left = 0, right = static_cast<int>(text.length());
    std::string result = text;

    while (left < right)
    {
        int mid = (left + right + 1) / 2;
        std::string testText = text.substr(0, mid) + "...";
        CString ctestText(testText.c_str());

        GetTextExtentPoint32(hdc, ctestText, ctestText.GetLength(), &textSize);

        if (textSize.cx <= maxWidth)
        {
            result = testText;
            left = mid;
        }
        else
        {
            right = mid - 1;
        }
    }

    return result;
}

void ClarionProcedureTimelineView::CreateDoubleBuffer()
{
    DestroyDoubleBuffer();

    RECT clientRect;
    GetClientRect(&clientRect);

    m_bufferWidth = clientRect.right - clientRect.left;
    m_bufferHeight = clientRect.bottom - clientRect.top;

    if (m_bufferWidth > 0 && m_bufferHeight > 0)
    {
        HDC hdc = GetDC();
        m_hdcMem = CreateCompatibleDC(hdc);
        m_hbmMem = CreateCompatibleBitmap(hdc, m_bufferWidth, m_bufferHeight);
        m_hbmOld = (HBITMAP)SelectObject(m_hdcMem, m_hbmMem);
        ReleaseDC(hdc);
    }
}

void ClarionProcedureTimelineView::DestroyDoubleBuffer()
{
    if (m_hdcMem)
    {
        if (m_hbmOld)
        {
            SelectObject(m_hdcMem, m_hbmOld);
            m_hbmOld = nullptr;
        }
        if (m_hbmMem)
        {
            DeleteObject(m_hbmMem);
            m_hbmMem = nullptr;
        }
        DeleteDC(m_hdcMem);
        m_hdcMem = nullptr;
    }
}

void ClarionProcedureTimelineView::ShowTooltip(const POINT& pt, const std::string& text)
{
    if (m_tooltip.IsWindow())
    {
        // Convert text to wide string
        CString ctext(text.c_str());

        // Update tooltip text
        CToolInfo ti(TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE, m_hWnd, 0, 0, const_cast<LPTSTR>((LPCTSTR)ctext));
        m_tooltip.SetToolInfo(&ti);

        // Activate tracking
        m_tooltip.TrackActivate(&ti, TRUE);

        // Position tooltip near cursor
        POINT screenPt = pt;
        ClientToScreen(&screenPt);
        m_tooltip.TrackPosition(screenPt.x + 15, screenPt.y + 15);

        // Force tooltip to update
        m_tooltip.UpdateTipText((LPCTSTR)ctext, m_hWnd, 0);

        // Ensure tooltip appears above "Stay On Top" windows
        // Check if our parent window is topmost
        HWND hParent = GetParent();
        while (hParent && ::GetParent(hParent))
        {
            hParent = ::GetParent(hParent);
        }

        if (hParent)
        {
            LONG_PTR exStyle = ::GetWindowLongPtr(hParent, GWL_EXSTYLE);
            if (exStyle & WS_EX_TOPMOST)
            {
                // If parent is topmost, make tooltip topmost too
                ::SetWindowPos(m_tooltip.m_hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                              SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }
        }
    }
}

void ClarionProcedureTimelineView::HideTooltip()
{
    if (m_tooltip.IsWindow())
    {
        CToolInfo ti(TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE, m_hWnd);
        m_tooltip.TrackActivate(&ti, FALSE);

        // Reset tooltip z-order to normal
        ::SetWindowPos(m_tooltip.m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

LRESULT ClarionProcedureTimelineView::OnTimelineCreateView(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (!m_contextMenuEvent)
        return 0;

    // Find the top-level parent window (MainFrame)
    HWND hMainFrame = GetParent();
    while (hMainFrame && ::GetParent(hMainFrame))
    {
        hMainFrame = ::GetParent(hMainFrame);
    }

    if (hMainFrame)
    {
        // Send a message to MainFrame to create the procedure view
        // Pass procedure name as wParam, process name as lParam for filtering
        ::SendMessage(hMainFrame, WM_USER_CREATE_PROCEDURE_VIEW,
                      reinterpret_cast<WPARAM>(m_contextMenuEvent->procedureName.c_str()),
                      reinterpret_cast<LPARAM>(m_contextMenuEvent->processName.c_str()));
    }

    return 0;
}

LRESULT ClarionProcedureTimelineView::OnTimelineJumpToRow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (!m_contextMenuEvent)
        return 0;

    // Find the top-level parent window (MainFrame)
    HWND hMainFrame = GetParent();
    while (hMainFrame && ::GetParent(hMainFrame))
    {
        hMainFrame = ::GetParent(hMainFrame);
    }

    if (hMainFrame)
    {
        // Use actual line index from the event
        ::SendMessage(hMainFrame, WM_USER_JUMP_TO_DEBUG_ROW,
                      static_cast<WPARAM>(m_contextMenuEvent->lineIndex),  // Use stored line index
                      0);  // No view name for now
    }

    return 0;
}

} // namespace debugviewpp