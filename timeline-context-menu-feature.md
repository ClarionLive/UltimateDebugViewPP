# Timeline Context Menu Feature Implementation

## Overview
Add a context menu to the Timeline view that appears when right-clicking on procedure entries. The menu will have two powerful features:
1. **Create View** - Creates a new filtered view for the selected procedure
2. **Jump to debug row** - Navigate to the source row in the debug grid where the procedure open/close originated

## Current Infrastructure Analysis

### Existing Components
- **ClarionProcedureTimelineView**: Already has right-click handling (`OnRButtonDown`, `OnContextMenu` stub at line 480)
- **MainFrame**: Has `AddFilterView()` methods for programmatic view creation
- **LogFilter**: Supports both `procedureFilters` and `processFilters`
- **CLogView**: Has `SetFocusLine()` and `ScrollToIndex()` for navigation
- **ClarionProcedureTracker**: Tracks procedure events with metadata

## Implementation Plan

### Phase 1: Enhanced Event Tracking
**Goal**: Track the source location of each procedure open/close event

#### 1.1 Extend ClarionProcedureTracker::Event
Add fields to track source information:
```cpp
struct Event {
    // ... existing fields ...
    std::string sourceViewName;  // Name of the view that generated this event
    int sourceLineIndex;         // Line number in the LogFile
    int sourceRowIndex;          // Row index in the source view
};
```

#### 1.2 Update LogFile::Add method
- Pass view context when detecting procedure events
- Store the line index where procedures are detected
- Modify calls to `HandleProcedureOpen/Close` to include source info

### Phase 2: Context Menu Implementation

#### 2.1 Implement OnContextMenu in ClarionProcedureTimelineView
```cpp
LRESULT ClarionProcedureTimelineView::OnContextMenu(UINT, WPARAM, LPARAM lParam, BOOL&)
{
    POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

    // Get the procedure event at this point
    GridCell* cell = GetCellAtPoint(pt);
    if (!cell || cell->procedures.empty())
        return 0;

    // Create popup menu
    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, ID_TIMELINE_CREATE_VIEW, _T("Create View"));
    menu.AppendMenu(MF_STRING, ID_TIMELINE_JUMP_TO_ROW, _T("Jump to debug row"));

    // Store selected procedure info for menu handlers
    m_selectedProcedureEvent = cell->procedures[0];

    // Show menu
    menu.TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, m_hWnd);
    return 0;
}
```

### Phase 3: "Create View" Feature

#### 3.1 Add method to MainFrame
```cpp
void CMainFrame::CreateProcedureView(const std::string& procedureName,
                                     const std::string& processName)
{
    LogFilter filter;

    // Add procedure filter
    filter.procedureFilters.push_back(
        Filter(procedureName, MatchType::Simple, FilterType::Include));

    // Add process filter if specified
    if (!processName.empty()) {
        filter.processFilters.push_back(
            Filter(processName, MatchType::Simple, FilterType::Include));
    }

    // Create view with descriptive name
    std::wstring viewName = L"Proc: " + Utf8ToUtf16(procedureName);
    AddFilterView(viewName, filter);
}
```

#### 3.2 Handle menu command in Timeline view
- Get selected procedure info
- Call MainFrame's `CreateProcedureView()`
- Optionally switch to the new view

### Phase 4: "Jump to Debug Row" Feature

#### 4.1 Enhanced tracking mechanism
- Create a map in LogFile to track event sources:
```cpp
std::map<size_t, SourceLocation> m_eventSourceMap;
struct SourceLocation {
    std::string viewName;
    int lineIndex;
    int viewRowIndex;
};
```

#### 4.2 Implement jump functionality
```cpp
void CMainFrame::JumpToEventSource(const SourceLocation& location)
{
    // Find the view by name
    int viewIndex = FindViewByName(location.viewName);
    if (viewIndex < 0)
        return; // View no longer exists

    // Switch to the view
    SetCurSel(viewIndex);

    // Jump to the line
    CLogView& view = GetView(viewIndex);
    view.SetFocusLine(location.lineIndex);

    // Optionally highlight the row
    view.SetItemState(location.viewRowIndex,
                      LVIS_SELECTED | LVIS_FOCUSED,
                      LVIS_SELECTED | LVIS_FOCUSED);
}
```

### Phase 5: Polish & Edge Cases

#### 5.1 UI Enhancements
- Add icons to menu items (optional)
- Show procedure details in menu item text
- Add keyboard shortcuts (Ctrl+N for new view, Ctrl+J for jump)

#### 5.2 Error Handling
- Handle case where source view has been closed
- Handle case where source row has been filtered out
- Handle case where source data is from a file (not live capture)
- Gracefully disable "Jump to row" if source info unavailable

#### 5.3 Performance Considerations
- Limit source tracking to avoid memory bloat
- Consider using weak references or IDs instead of storing full paths
- Implement cleanup when views are closed

## Technical Challenges

### Challenge 1: Source Tracking
**Problem**: Messages flow through multiple layers before reaching Timeline
**Solution**: Pass view context through the message pipeline, possibly using a context ID

### Challenge 2: View Synchronization
**Problem**: Source view may have changed (filtered, scrolled, cleared)
**Solution**: Store both line index (stable) and view row (for quick jump), fallback to line search

### Challenge 3: Memory Management
**Problem**: Storing source info for all events could use significant memory
**Solution**:
- Store only for visible/recent events
- Use circular buffer with configurable size
- Store references instead of duplicating data

## Testing Scenarios

1. **Basic Functionality**
   - Right-click on procedure in timeline
   - Create view for procedure
   - Jump to source row

2. **Edge Cases**
   - Create view for procedure that's already filtered
   - Jump to row when source view is closed
   - Jump to row after clearing log
   - Multiple procedures with same name

3. **Performance**
   - Test with thousands of procedures
   - Test with long-running sessions
   - Memory usage monitoring

## Future Enhancements

1. **Multi-select**: Select multiple procedures to create combined filter
2. **Statistics**: Show procedure call count, total time in context menu
3. **Export**: Export procedure timeline to CSV/JSON
4. **Bookmarks**: Bookmark specific procedure instances
5. **Diff View**: Compare two procedure execution paths

## Implementation Priority

1. **High Priority** (Core functionality)
   - Basic context menu
   - Create View feature
   - Simple jump to row (without source tracking)

2. **Medium Priority** (Enhanced usability)
   - Full source tracking
   - Accurate jump to original row
   - Error handling for edge cases

3. **Low Priority** (Nice to have)
   - Icons and keyboard shortcuts
   - Advanced filtering options
   - Statistics in menu

## Success Criteria

- [ ] Context menu appears on right-click in Timeline
- [ ] "Create View" creates correctly filtered view
- [ ] "Jump to debug row" navigates to source (when available)
- [ ] No memory leaks or performance degradation
- [ ] Graceful handling of edge cases
- [ ] User-friendly error messages when operations fail