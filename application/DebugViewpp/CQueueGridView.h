#pragma once

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlctrls.h>
#include <atlmisc.h>
#include "DebugViewppLib/LogFile.h"
#include "DebugViewppLib/ClarionQueueManager.h"
#include <memory>

namespace fusion {
namespace debugviewpp {

class CQueueGridView : public CWindowImpl<CQueueGridView>
{
public:
    DECLARE_WND_CLASS(NULL)

    CQueueGridView();
    virtual ~CQueueGridView();

    // Set the Queue data to display
    void SetQueueData(std::shared_ptr<QueueData> queueData);

    // Clear the grid
    void Clear();

    // Copy selected rows to clipboard
    void CopyToClipboard();

    // Export to CSV file
    void ExportToCsv(const std::wstring& filename);

    // Set font for the grid
    void SetFont(HFONT hFont);

    BEGIN_MSG_MAP(CQueueGridView)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        NOTIFY_CODE_HANDLER(LVN_KEYDOWN, OnListViewKeyDown)
        NOTIFY_CODE_HANDLER(LVN_COLUMNCLICK, OnColumnClick)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

private:
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnListViewKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnColumnClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    void CreateListView();
    void PopulateGrid();
    void SortByColumn(int columnIndex);
    std::wstring GetCellText(int row, int col);

    CListViewCtrl m_listView;
    std::shared_ptr<QueueData> m_queueData;
    int m_sortColumn;
    bool m_sortAscending;
};

} // namespace debugviewpp
} // namespace fusion