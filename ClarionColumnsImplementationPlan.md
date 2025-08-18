# Clarion Columns Implementation Plan for Ultimate DebugView++

## Overview
This document outlines the implementation plan for adding Clarion-formatted message support with additional columns to Ultimate DebugView++. The implementation is divided into three phases to ensure systematic progress and early issue detection.

## Phase 1: Add New Columns Infrastructure

### 1. Define New Column Types
**File:** `application/DebugViewpp/LogView.h`

Add new column enums in the `Column` struct after `Process` but before `Message`:
```cpp
struct Column
{
    enum type
    {
        Bookmark = 0,
        Line,
        Date,
        Time,
        Pid,
        Process,
        Thread,      // NEW
        Module,      // NEW
        Function,    // NEW
        Category,    // NEW
        Level,       // NEW
        Message,     // Keep as last
        Count
    };
};
```

### 2. Update Resource IDs
**File:** `application/DebugViewpp/resource.h`

Add new column command IDs:
```cpp
#define ID_VIEW_COLUMN_THREAD    32833
#define ID_VIEW_COLUMN_MODULE    32834
#define ID_VIEW_COLUMN_FUNCTION  32835
#define ID_VIEW_COLUMN_CATEGORY  32836
#define ID_VIEW_COLUMN_LEVEL     32837
#define ID_VIEW_COLUMN_LAST      32837  // Update to new last ID
```

### 3. Update Header Context Menu
**File:** `application/DebugViewpp/DebugView++.rc`

Add menu items for new columns (excluding Message to prevent hiding):
```rc
IDR_HEADER_CONTEXTMENU MENU
BEGIN
    POPUP "Header Context Menu"
    BEGIN
        MENUITEM "Line",                        ID_VIEW_COLUMN_LINE
        MENUITEM "Date",                        ID_VIEW_COLUMN_DATE
        MENUITEM "Time",                        ID_VIEW_COLUMN_TIME
        MENUITEM "PID",                         ID_VIEW_COLUMN_PID
        MENUITEM "Process",                     ID_VIEW_COLUMN_PROCESS
        MENUITEM "Thread",                       ID_VIEW_COLUMN_THREAD
        MENUITEM "Module",                       ID_VIEW_COLUMN_MODULE
        MENUITEM "Function",                     ID_VIEW_COLUMN_FUNCTION
        MENUITEM "Category",                     ID_VIEW_COLUMN_CATEGORY
        MENUITEM "Level",                        ID_VIEW_COLUMN_LEVEL
        // Note: Message column intentionally omitted
    END
END
```

### 4. Initialize Columns
**File:** `application/DebugViewpp/LogView.cpp`

In `OnCreate()` method, add initialization for new columns:
```cpp
m_columns.push_back(MakeColumn(Column::Bookmark, L"", LVCFMT_RIGHT, 20));
m_columns.push_back(MakeColumn(Column::Line, L"Line", LVCFMT_RIGHT, 60));
m_columns.push_back(MakeColumn(Column::Date, L"Date", LVCFMT_RIGHT, 90));
m_columns.back().enable = false; // Default no Date column
m_columns.push_back(MakeColumn(Column::Time, L"Time", LVCFMT_RIGHT, 90));
m_columns.push_back(MakeColumn(Column::Pid, L"PID", LVCFMT_RIGHT, 60));
m_columns.push_back(MakeColumn(Column::Process, L"Process", LVCFMT_LEFT, 140));
// NEW COLUMNS
m_columns.push_back(MakeColumn(Column::Thread, L"Thread", LVCFMT_RIGHT, 80));
m_columns.back().enable = false; // Default hidden
m_columns.push_back(MakeColumn(Column::Module, L"Module", LVCFMT_LEFT, 120));
m_columns.back().enable = false; // Default hidden
m_columns.push_back(MakeColumn(Column::Function, L"Function", LVCFMT_LEFT, 150));
m_columns.back().enable = false; // Default hidden
m_columns.push_back(MakeColumn(Column::Category, L"Category", LVCFMT_LEFT, 100));
m_columns.back().enable = false; // Default hidden
m_columns.push_back(MakeColumn(Column::Level, L"Level", LVCFMT_LEFT, 80));
m_columns.back().enable = false; // Default hidden
// Message column must be last
m_columns.push_back(MakeColumn(Column::Message, L"Message", LVCFMT_LEFT, 1500));
```

### 5. Extend Data Structures
**File:** `application/DebugViewpp/LogView.h`

Update `ItemData` structure array size to match new Column::Count.

### 6. Implement GetColumnText for New Columns
**File:** `application/DebugViewpp/LogView.cpp`

Add cases in `GetColumnText()` method:
```cpp
case Column::Thread: return L""; // Will be populated after parsing
case Column::Module: return L"";
case Column::Function: return L"";
case Column::Category: return L"";
case Column::Level: return L"";
```

### 7. Fix Column Visibility Issues
**File:** `application/DebugViewpp/LogView.cpp`

Modify `OnViewColumn()` to prevent Message column from being hidden:
```cpp
void CLogView::OnViewColumn(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
    // Prevent Message column from being hidden
    if (nID - ID_VIEW_COLUMN_FIRST == Column::Message)
        return;
    
    auto& column = m_columns[nID - ID_VIEW_COLUMN_FIRST];
    column.enable = !column.enable;
    UpdateColumns();
}
```

## Phase 2: Test Column Infrastructure

### Testing Checklist
- [ ] Build the application successfully
- [ ] All original columns still function correctly
- [ ] New columns appear in the header context menu
- [ ] New columns can be shown via context menu
- [ ] New columns can be hidden via context menu
- [ ] Message column does NOT appear in context menu
- [ ] Message column cannot be hidden
- [ ] Message column always remains as the last column
- [ ] Column settings persist across application restarts
- [ ] No crashes or unexpected behavior when toggling columns

## Phase 3: Implement Clarion Format Parsing

### 1. Define Clarion Message Format
Create a specification for the Clarion message format that will be parsed. Example:
```
[CLARION]|Thread:1234|Module:MyModule|Function:ProcessData|Category:Debug|Level:Info|Message text here
```

### 2. Create Clarion Parser
**New File:** `application/DebugViewppLib/ClarionParser.cpp/h`

Implement parser class:
```cpp
class ClarionParser
{
public:
    struct ClarionData
    {
        std::wstring thread;
        std::wstring module;
        std::wstring function;
        std::wstring category;
        std::wstring level;
        std::wstring message;
    };
    
    static bool IsClarionFormat(const std::wstring& text);
    static ClarionData Parse(const std::wstring& text);
};
```

### 3. Integration with Message Processing
**File:** `application/DebugViewppLib/LogFile.cpp`

Detect and parse Clarion messages when they arrive:
- Check if message starts with `[CLARION]|`
- Parse the formatted fields
- Store parsed data in message structure

### 4. Display Parsed Data in Columns
**File:** `application/DebugViewpp/LogView.cpp`

Update `GetColumnText()` to return parsed Clarion data:
```cpp
case Column::Thread: return msg.clarionData.thread;
case Column::Module: return msg.clarionData.module;
case Column::Function: return msg.clarionData.function;
case Column::Category: return msg.clarionData.category;
case Column::Level: return msg.clarionData.level;
```

## Known Issues from Previous Attempt

The previous implementation failed because new columns couldn't be shown. Likely causes:
1. **Column range validation**: Ensure ID_VIEW_COLUMN_LAST is updated
2. **Array bounds**: Verify m_columns vector size matches Column::Count
3. **Command routing**: Check that COMMAND_RANGE_HANDLER_EX covers new IDs
4. **Initialization order**: Ensure columns are initialized before UpdateColumns()
5. **Registry persistence**: May need to handle migration of saved column settings

## Implementation Notes

### Critical Requirements
1. **Message column must NEVER be hideable** - This is enforced in OnViewColumn()
2. **Message column must ALWAYS be last** - Enforced by initialization order
3. **New columns default to hidden** - To avoid cluttering existing users' views
4. **Backward compatibility** - Existing saved configurations must continue to work

### Testing Strategy
1. Test with non-Clarion messages first (ensure no regression)
2. Test with sample Clarion messages
3. Test column persistence across restarts
4. Test with mixed Clarion and non-Clarion messages

## Success Criteria
- All existing functionality remains intact
- New columns can be shown/hidden (except Message)
- Message column cannot be hidden and stays last
- Clarion messages are properly parsed and displayed
- Performance is not significantly impacted
- Settings persist correctly

## Rollback Plan
If issues arise:
1. Revert Column enum changes
2. Revert resource.h changes
3. Revert menu changes
4. Revert initialization code
5. Clean rebuild

This phased approach ensures we can identify and fix issues early, particularly the column visibility problem from the previous attempt.