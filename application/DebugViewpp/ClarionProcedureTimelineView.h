#pragma once

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlctrls.h>
#include <atlmisc.h>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include "DebugViewppLib/ClarionProcedureTracker.h"
#include "resource.h"

namespace debugviewpp {

using fusion::debugviewpp::ClarionProcedureTracker;

class ClarionProcedureTimelineView : public CWindowImpl<ClarionProcedureTimelineView>
{
public:
    DECLARE_WND_CLASS(_T("ClarionProcedureTimelineView"))

    ClarionProcedureTimelineView();
    virtual ~ClarionProcedureTimelineView();

    BEGIN_MSG_MAP(ClarionProcedureTimelineView)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
        MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
        MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
        MESSAGE_HANDLER(WM_VSCROLL, OnVScroll)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        COMMAND_ID_HANDLER(ID_TIMELINE_CREATE_VIEW, OnTimelineCreateView)
        COMMAND_ID_HANDLER(ID_TIMELINE_JUMP_TO_ROW, OnTimelineJumpToRow)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    void SetTracker(std::shared_ptr<ClarionProcedureTracker> tracker);
    void RefreshView();
    void Clear();

protected:
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTimelineCreateView(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnTimelineJumpToRow(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:
    struct TimeSlice
    {
        double startTime;
        double endTime;
        std::map<std::string, std::vector<ClarionProcedureTracker::Event>> threadProcedures;
    };

    struct GridCell
    {
        int row;
        int col;
        RECT bounds;
        std::vector<ClarionProcedureTracker::Event> procedures;
    };

    struct ViewMetrics
    {
        int headerHeight;
        int timeAxisWidth;
        int cellHeight;
        int cellWidth;
        int visibleRows;
        int visibleCols;
        double pixelsPerSecond;
        double startTime;
        double endTime;
        double timeRange;
    };

    void ProcessTrackerData();
    void CalculateTimeSlices();
    void DrawGrid(HDC hdc);
    void DrawTimeAxis(HDC hdc, const RECT& rect);
    void DrawThreadHeaders(HDC hdc, const RECT& rect);
    void DrawGridCells(HDC hdc, const RECT& rect);
    void DrawProcedureBlock(HDC hdc, const RECT& cellRect, const ClarionProcedureTracker::Event& proc);
    void UpdateScrollBars();
    void UpdateViewMetrics();
    void CalculateColumnWidths();
    int CalculateTextWidth(const std::string& text, HDC hdc);
    GridCell* GetCellAtPoint(const POINT& pt);
    COLORREF GetProcedureColor(const std::string& procedureName);
    std::string FormatTimeLabel(double time);
    std::string TruncateText(const std::string& text, int maxWidth, HDC hdc);
    void CreateDoubleBuffer();
    void DestroyDoubleBuffer();
    void ShowTooltip(const POINT& pt, const std::string& text);
    void HideTooltip();

private:
    std::shared_ptr<ClarionProcedureTracker> m_tracker;
    std::vector<TimeSlice> m_timeSlices;
    std::vector<std::string> m_threadIds;
    std::map<std::string, int> m_threadIndexMap;
    std::map<std::string, COLORREF> m_procedureColors;
    std::vector<GridCell> m_gridCells;
    std::vector<int> m_columnWidths;

    ViewMetrics m_metrics;
    int m_scrollX;
    int m_scrollY;
    double m_zoomLevel;
    bool m_isDragging;
    POINT m_dragStart;
    POINT m_dragOffset;

    GridCell* m_selectedCell;
    GridCell* m_hoveredCell;
    std::string m_selectedProcedure;
    std::unique_ptr<ClarionProcedureTracker::Event> m_contextMenuEvent;  // Use pointer to avoid default constructor requirement

    CScrollBar m_hScrollBar;
    CScrollBar m_vScrollBar;
    CToolTipCtrl m_tooltip;

    HDC m_hdcMem;
    HBITMAP m_hbmMem;
    HBITMAP m_hbmOld;
    int m_bufferWidth;
    int m_bufferHeight;

    HFONT m_fontNormal;
    HFONT m_fontBold;
    HPEN m_penGrid;
    HPEN m_penSelection;
    HBRUSH m_brushBackground;

    static const int TIMER_REFRESH = 1001;
    static const int HEADER_HEIGHT = 30;
    static const int TIME_AXIS_WIDTH = 140;  // Generous width for HH:MM:SS.mmm format
    static const int MIN_CELL_HEIGHT = 20;
    static const int MIN_CELL_WIDTH = 100;
    static const int MAX_CELL_WIDTH = 300;
    static const int MIN_COLUMN_WIDTH = 200;  // Increased to prevent truncation
    static const int MAX_COLUMN_WIDTH = 500;  // Increased for longer procedure names
    static const int COLUMN_PADDING = 20;
    static const int SCROLLBAR_SIZE = 17;
    static const double MIN_ZOOM;
    static const double MAX_ZOOM;
    static const double ZOOM_STEP;
};

} // namespace debugviewpp