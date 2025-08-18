# Detail Pane Toolbar Implementation Plan

## Overview
This document outlines the implementation of a toolbar for the detail pane in DebugView++. The toolbar will provide quick access to Find, Filter, Word Wrap, and Copy functions directly from the detail view.

## User Requirements
1. **Find Button** - Search for selected text in the debug list
2. **Filter Button** - Filter the debug list using selected text
3. **Word Wrap Checkbox** - Toggle word wrapping in the detail view
4. **Copy Button** - Copy entire detail text to clipboard

## Architecture Design

### Layout Structure
```
┌─────────────────────────────────────────┐
│ Detail Pane Container (CDetailView)     │
├─────────────────────────────────────────┤
│ Toolbar Panel (30px height)             │
│ ┌──────┐ ┌────────┐ ┌─────────┐ ┌─────┐│
│ │ Find │ │ Filter │ │☐ Wrap   │ │Copy ││
│ └──────┘ └────────┘ └─────────┘ └─────┘│
├─────────────────────────────────────────┤
│                                         │
│         Edit Control                    │
│         (Remaining space)               │
│                                         │
└─────────────────────────────────────────┘
```

### Component Hierarchy
```
CLogViewTabItem2
    └── CHorSplitterWindow (m_split)
        ├── CMyPaneContainer (m_top)
        │   └── CLogView
        └── CMyPaneContainer (m_bottom)
            └── CDetailView
                ├── Toolbar Controls
                │   ├── CButton m_btnFind
                │   ├── CButton m_btnFilter
                │   ├── CButton m_btnWordWrap
                │   └── CButton m_btnCopy
                └── CEdit m_edit
```

## Implementation Details

### 1. Resource IDs
```cpp
// Add to resource.h
#define IDC_DETAIL_FIND      40001
#define IDC_DETAIL_FILTER    40002
#define IDC_DETAIL_WORDWRAP  40003
#define IDC_DETAIL_COPY      40004
```

### 2. CDetailView Class Modifications

#### Member Variables
```cpp
class CDetailView : public CWindowImpl<CDetailView>
{
    // Existing
    CEdit m_edit;
    
    // New toolbar controls
    CButton m_btnFind;
    CButton m_btnFilter;
    CButton m_btnWordWrap;
    CButton m_btnCopy;
    
    // State
    bool m_wordWrap = false;
    CMainFrame* m_pMainFrame = nullptr;
    
    // Layout constants
    static const int TOOLBAR_HEIGHT = 30;
    static const int BUTTON_WIDTH = 70;
    static const int BUTTON_HEIGHT = 24;
    static const int BUTTON_SPACING = 5;
};
```

#### Message Map
```cpp
BEGIN_MSG_MAP(CDetailView)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    COMMAND_HANDLER(IDC_DETAIL_FIND, BN_CLICKED, OnFind)
    COMMAND_HANDLER(IDC_DETAIL_FILTER, BN_CLICKED, OnFilter)
    COMMAND_HANDLER(IDC_DETAIL_WORDWRAP, BN_CLICKED, OnWordWrap)
    COMMAND_HANDLER(IDC_DETAIL_COPY, BN_CLICKED, OnCopy)
END_MSG_MAP()
```

### 3. Button Implementations

#### Find Button
```cpp
LRESULT OnFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // Get selected text from edit control
    int start, end;
    m_edit.GetSel(start, end);
    
    if (start != end)
    {
        CString text;
        m_edit.GetWindowText(text);
        std::wstring selectedText = text.Mid(start, end - start);
        
        // Trigger find in main view
        if (m_pMainFrame)
        {
            m_pMainFrame->GetView().SetHighlightText(selectedText);
            m_pMainFrame->GetView().FindNext(selectedText);
        }
    }
    return 0;
}
```

#### Filter Button
```cpp
LRESULT OnFilter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // Get selected text
    int start, end;
    m_edit.GetSel(start, end);
    
    if (start != end)
    {
        CString text;
        m_edit.GetWindowText(text);
        std::wstring selectedText = text.Mid(start, end - start);
        
        // Open filter dialog with text as include filter
        if (m_pMainFrame)
        {
            m_pMainFrame->ShowFilterDialogWithText(selectedText);
        }
    }
    return 0;
}
```

#### Word Wrap Toggle
```cpp
LRESULT OnWordWrap(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_wordWrap = !m_wordWrap;
    
    // Save current text and selection
    CString currentText;
    m_edit.GetWindowText(currentText);
    int selStart, selEnd;
    m_edit.GetSel(selStart, selEnd);
    
    // Recreate edit control with new style
    RecreateEditControl();
    
    // Restore text and selection
    m_edit.SetWindowText(currentText);
    m_edit.SetSel(selStart, selEnd);
    
    return 0;
}
```

#### Copy Button
```cpp
LRESULT OnCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CString text;
    m_edit.GetWindowText(text);
    
    if (OpenClipboard())
    {
        EmptyClipboard();
        
        size_t len = (text.GetLength() + 1) * sizeof(TCHAR);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
        if (hMem)
        {
            LPVOID pMem = GlobalLock(hMem);
            memcpy(pMem, text.GetString(), len);
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
        CloseClipboard();
    }
    return 0;
}
```

### 4. Layout Management

#### OnCreate Handler
```cpp
LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // Create toolbar buttons
    int x = BUTTON_SPACING;
    
    m_btnFind.Create(m_hWnd, CRect(x, 3, x + BUTTON_WIDTH, 3 + BUTTON_HEIGHT),
                      L"Find", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                      0, IDC_DETAIL_FIND);
    x += BUTTON_WIDTH + BUTTON_SPACING;
    
    m_btnFilter.Create(m_hWnd, CRect(x, 3, x + BUTTON_WIDTH, 3 + BUTTON_HEIGHT),
                       L"Filter", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                       0, IDC_DETAIL_FILTER);
    x += BUTTON_WIDTH + BUTTON_SPACING;
    
    m_btnWordWrap.Create(m_hWnd, CRect(x, 3, x + BUTTON_WIDTH + 20, 3 + BUTTON_HEIGHT),
                         L"Word Wrap", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                         0, IDC_DETAIL_WORDWRAP);
    x += BUTTON_WIDTH + 20 + BUTTON_SPACING;
    
    m_btnCopy.Create(m_hWnd, CRect(x, 3, x + BUTTON_WIDTH, 3 + BUTTON_HEIGHT),
                     L"Copy All", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                     0, IDC_DETAIL_COPY);
    
    // Create edit control below toolbar
    CreateEditControl();
    
    return 0;
}
```

#### OnSize Handler
```cpp
LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    if (m_edit.m_hWnd)
    {
        int cx = LOWORD(lParam);
        int cy = HIWORD(lParam);
        
        // Position edit control below toolbar
        m_edit.SetWindowPos(nullptr, 0, TOOLBAR_HEIGHT, cx, cy - TOOLBAR_HEIGHT,
                           SWP_NOZORDER | SWP_NOACTIVATE);
    }
    return 0;
}
```

### 5. Helper Methods

#### CreateEditControl
```cpp
void CreateEditControl()
{
    RECT rcClient;
    GetClientRect(&rcClient);
    rcClient.top = TOOLBAR_HEIGHT;
    
    DWORD editStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | 
                      ES_READONLY | ES_AUTOVSCROLL | ES_NOHIDESEL;
    
    if (!m_wordWrap)
        editStyle |= WS_HSCROLL | ES_AUTOHSCROLL;
    
    m_edit.Create(m_hWnd, rcClient, nullptr, editStyle, WS_EX_CLIENTEDGE);
    
    // Set font
    CFont font;
    font.CreatePointFont(90, L"Consolas");
    m_edit.SetFont(font);
}
```

#### RecreateEditControl
```cpp
void RecreateEditControl()
{
    if (m_edit.m_hWnd)
        m_edit.DestroyWindow();
    
    CreateEditControl();
}
```

### 6. MainFrame Integration

#### Add to MainFrame.h
```cpp
void ShowFilterDialogWithText(const std::wstring& text);
```

#### Add to MainFrame.cpp
```cpp
void CMainFrame::ShowFilterDialogWithText(const std::wstring& text)
{
    // Get current filters
    auto filters = GetView().GetFilters();
    
    // Add new include filter with the text
    Filter newFilter;
    newFilter.text = text;
    newFilter.matchType = MatchType::Substring;
    newFilter.filterType = FilterType::Include;
    newFilter.enable = true;
    
    filters.messageFilters.push_back(newFilter);
    
    // Open filter dialog
    CFilterDlg dlg(GetView().GetName(), filters);
    if (dlg.DoModal() == IDOK)
    {
        GetView().SetFilters(dlg.GetFilters());
    }
}
```

### 7. Initialization

#### Pass MainFrame Reference
In CLogViewTabItem2::Create():
```cpp
m_detailView.SetMainFrame(&mainFrame);
```

## Testing Plan

### Functional Tests
1. **Find Button**
   - Select text in detail view
   - Click Find button
   - Verify text is highlighted in main list
   - Verify navigation to first occurrence

2. **Filter Button**
   - Select text in detail view
   - Click Filter button
   - Verify filter dialog opens with text
   - Apply filter and verify list updates

3. **Word Wrap**
   - Toggle word wrap checkbox
   - Verify horizontal scrollbar appears/disappears
   - Verify long lines wrap/unwrap correctly
   - Verify text and selection preserved

4. **Copy Button**
   - Click Copy button
   - Paste into external application
   - Verify complete text is copied

### Edge Cases
- No text selected (Find/Filter should do nothing)
- Empty detail view
- Very long text performance
- Rapid button clicks
- Window resize with toolbar

## Benefits
- **Improved Workflow**: Quick access to common operations
- **Better Analysis**: Easy to search/filter from detail view
- **Enhanced Readability**: Word wrap for long lines
- **Convenience**: One-click copy of full message

## Future Enhancements
- Syntax highlighting in detail view
- Export to file button
- Font size adjustment
- Find/Replace within detail view
- Regular expression support for find