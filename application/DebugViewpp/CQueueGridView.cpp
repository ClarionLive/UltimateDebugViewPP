#include "CQueueGridView.h"
#include <algorithm>
#include <sstream>
#include <fstream>

namespace fusion {
namespace debugviewpp {

CQueueGridView::CQueueGridView()
    : m_sortColumn(-1)
    , m_sortAscending(true)
{
}

CQueueGridView::~CQueueGridView()
{
}

LRESULT CQueueGridView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CreateListView();
    return 0;
}

LRESULT CQueueGridView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    if (m_listView.IsWindow())
    {
        m_listView.MoveWindow(0, 0, LOWORD(lParam), HIWORD(lParam));
    }
    return 0;
}

LRESULT CQueueGridView::OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // TODO: Implement context menu with Copy, Export options
    return 0;
}

LRESULT CQueueGridView::OnListViewKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
    LPNMLVKEYDOWN pnkd = reinterpret_cast<LPNMLVKEYDOWN>(pnmh);

    if (pnkd->wVKey == 'C' && ::GetKeyState(VK_CONTROL) < 0)
    {
        CopyToClipboard();
        return 0;
    }

    bHandled = FALSE;
    return 0;
}

LRESULT CQueueGridView::OnColumnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
    LPNMLISTVIEW pnmv = reinterpret_cast<LPNMLISTVIEW>(pnmh);
    SortByColumn(pnmv->iSubItem);
    return 0;
}

void CQueueGridView::CreateListView()
{
    m_listView.Create(m_hWnd, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
        WS_EX_CLIENTEDGE);

    m_listView.SetExtendedListViewStyle(
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
}

void CQueueGridView::SetQueueData(std::shared_ptr<QueueData> queueData)
{
    // Only update if the queue data has changed
    if (m_queueData != queueData)
    {
        // Check if it's the same queue ID and same data size
        if (m_queueData && queueData &&
            m_queueData->queueId == queueData->queueId &&
            m_queueData->headers.size() == queueData->headers.size() &&
            m_queueData->rows.size() == queueData->rows.size())
        {
            // Same queue, same size - likely clicking on different messages from same queue
            // Don't repopulate the grid
            return;
        }

        m_queueData = queueData;
        PopulateGrid();
    }
}

void CQueueGridView::Clear()
{
    if (m_listView.IsWindow())
    {
        m_listView.DeleteAllItems();

        // Delete all columns
        int columnCount = m_listView.GetHeader().GetItemCount();
        for (int i = columnCount - 1; i >= 0; i--)
        {
            m_listView.DeleteColumn(i);
        }
    }
    m_queueData.reset();
}

void CQueueGridView::PopulateGrid()
{
    if (!m_listView.IsWindow() || !m_queueData)
        return;

    // Clear existing data
    m_listView.DeleteAllItems();

    // Delete existing columns
    int columnCount = m_listView.GetHeader().GetItemCount();
    for (int i = columnCount - 1; i >= 0; i--)
    {
        m_listView.DeleteColumn(i);
    }

    // Add columns from headers
    for (size_t i = 0; i < m_queueData->headers.size(); ++i)
    {
        CString header(m_queueData->headers[i].c_str());
        m_listView.InsertColumn(static_cast<int>(i), header, LVCFMT_LEFT, 150);
    }

    // Add rows
    for (size_t i = 0; i < m_queueData->rows.size(); ++i)
    {
        const auto& row = m_queueData->rows[i];

        // Insert item with first column value
        CString firstValue;
        if (!row.empty())
        {
            firstValue = CString(row[0].c_str());
        }
        int itemIndex = m_listView.InsertItem(static_cast<int>(i), firstValue);

        // Set remaining subitems (starting from column 1)
        for (size_t j = 1; j < row.size() && j < m_queueData->headers.size(); ++j)
        {
            CString value(row[j].c_str());
            m_listView.SetItemText(itemIndex, static_cast<int>(j), value);
        }
    }

    // Auto-size columns
    for (size_t i = 0; i < m_queueData->headers.size(); ++i)
    {
        m_listView.SetColumnWidth(static_cast<int>(i), LVSCW_AUTOSIZE_USEHEADER);
    }
}

void CQueueGridView::SortByColumn(int columnIndex)
{
    if (!m_queueData || columnIndex < 0 || columnIndex >= static_cast<int>(m_queueData->headers.size()))
        return;

    // Toggle sort order if clicking same column
    if (m_sortColumn == columnIndex)
    {
        m_sortAscending = !m_sortAscending;
    }
    else
    {
        m_sortColumn = columnIndex;
        m_sortAscending = true;
    }

    // Sort the data
    std::sort(m_queueData->rows.begin(), m_queueData->rows.end(),
        [this, columnIndex](const std::vector<std::string>& a, const std::vector<std::string>& b) {
            if (columnIndex >= static_cast<int>(a.size()) || columnIndex >= static_cast<int>(b.size()))
                return false;

            int cmp = a[columnIndex].compare(b[columnIndex]);
            return m_sortAscending ? (cmp < 0) : (cmp > 0);
        });

    // Refresh the display
    PopulateGrid();
}

void CQueueGridView::CopyToClipboard()
{
    if (!m_listView.IsWindow() || !m_queueData)
        return;

    std::wstringstream ss;

    // Add headers
    for (size_t i = 0; i < m_queueData->headers.size(); ++i)
    {
        if (i > 0) ss << L"\t";
        ss << std::wstring(m_queueData->headers[i].begin(), m_queueData->headers[i].end());
    }
    ss << L"\r\n";

    // Add selected rows (or all if none selected)
    int selectedCount = m_listView.GetSelectedCount();

    if (selectedCount == 0)
    {
        // Copy all rows
        for (const auto& row : m_queueData->rows)
        {
            for (size_t i = 0; i < row.size(); ++i)
            {
                if (i > 0) ss << L"\t";
                ss << std::wstring(row[i].begin(), row[i].end());
            }
            ss << L"\r\n";
        }
    }
    else
    {
        // Copy selected rows
        int item = -1;
        while ((item = m_listView.GetNextItem(item, LVNI_SELECTED)) != -1)
        {
            if (item < static_cast<int>(m_queueData->rows.size()))
            {
                const auto& row = m_queueData->rows[item];
                for (size_t i = 0; i < row.size(); ++i)
                {
                    if (i > 0) ss << L"\t";
                    ss << std::wstring(row[i].begin(), row[i].end());
                }
                ss << L"\r\n";
            }
        }
    }

    // Copy to clipboard
    std::wstring text = ss.str();
    if (!text.empty())
    {
        if (::OpenClipboard(m_hWnd))
        {
            ::EmptyClipboard();

            size_t size = (text.length() + 1) * sizeof(wchar_t);
            HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, size);
            if (hGlobal)
            {
                void* pData = ::GlobalLock(hGlobal);
                if (pData)
                {
                    memcpy(pData, text.c_str(), size);
                    ::GlobalUnlock(hGlobal);
                    ::SetClipboardData(CF_UNICODETEXT, hGlobal);
                }
            }
            ::CloseClipboard();
        }
    }
}

void CQueueGridView::ExportToCsv(const std::wstring& filename)
{
    if (!m_queueData)
        return;

    std::wofstream file(filename);
    if (!file.is_open())
        return;

    // Write headers
    for (size_t i = 0; i < m_queueData->headers.size(); ++i)
    {
        if (i > 0) file << L",";

        // Quote if contains comma
        std::string header = m_queueData->headers[i];
        if (header.find(',') != std::string::npos)
        {
            file << L"\"" << std::wstring(header.begin(), header.end()) << L"\"";
        }
        else
        {
            file << std::wstring(header.begin(), header.end());
        }
    }
    file << L"\n";

    // Write rows
    for (const auto& row : m_queueData->rows)
    {
        for (size_t i = 0; i < row.size(); ++i)
        {
            if (i > 0) file << L",";

            // Quote if contains comma
            if (row[i].find(',') != std::string::npos)
            {
                file << L"\"" << std::wstring(row[i].begin(), row[i].end()) << L"\"";
            }
            else
            {
                file << std::wstring(row[i].begin(), row[i].end());
            }
        }
        file << L"\n";
    }

    file.close();
}

std::wstring CQueueGridView::GetCellText(int row, int col)
{
    if (!m_queueData || row < 0 || row >= static_cast<int>(m_queueData->rows.size()) ||
        col < 0 || col >= static_cast<int>(m_queueData->headers.size()))
    {
        return L"";
    }

    const auto& rowData = m_queueData->rows[row];
    if (col < static_cast<int>(rowData.size()))
    {
        return std::wstring(rowData[col].begin(), rowData[col].end());
    }

    return L"";
}

void CQueueGridView::SetFont(HFONT hFont)
{
    if (m_listView.IsWindow() && hFont)
    {
        m_listView.SetFont(hFont);
        // Force a redraw to apply the new font
        m_listView.Invalidate();
    }
}

} // namespace debugviewpp
} // namespace fusion