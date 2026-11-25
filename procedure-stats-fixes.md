# Procedure Stats Tab Fixes

## Issues to Fix

### 1. Splitter Position Issue
**Problem**: The vertical splitter in the Procedure Stats tab starts all the way to the left, making the left pane invisible.
**Root Cause**: The code incorrectly uses `tabRect.Height() / 2` instead of `tabRect.Width() / 2` for a vertical splitter.
**Location**: `ClarionProcedureView.cpp`, line 473

### 2. Missing Grid Titles
**Problem**: No descriptive titles above the two grids in the Procedure Stats tab
**Requirement**:
- Left grid should have title: "Open Procedures"
- Right grid should have title: "Open/Close Count"

### 3. Column Header Sorting Not Working
**Problem**: Clicking column headers in the Procedure Stats tab doesn't trigger sorting
**Root Cause**: WM_NOTIFY messages from list views inside the splitter window aren't properly reaching the parent window handler

## Solution Architecture

### Create Container Panels
Instead of placing list views directly in the splitter, create container panels:

```cpp
// New container classes needed
class CProcedureStatsPanel : public CWindowImpl<CProcedureStatsPanel> {
    // Contains a static text label and a list view
    CStatic m_titleLabel;
    CListViewCtrl m_listView;
};
```

### Implementation Steps

1. **Create Container Windows**
   - Create two container panel classes (left and right)
   - Each container has a static text control at top and list view below
   - Containers forward WM_NOTIFY messages to parent

2. **Fix Splitter Position**
   - Change: `m_procedureStatsSplitter.SetSplitterPos(tabRect.Height() / 2);`
   - To: `m_procedureStatsSplitter.SetSplitterPos(tabRect.Width() / 2);`

3. **Add Title Labels**
   - In left container: Add static text "Open Procedures"
   - In right container: Add static text "Open/Close Count"
   - Position labels at top with ~20 pixel height
   - Position list views below labels

4. **Fix Notification Routing**
   - Ensure containers forward WM_NOTIFY messages
   - Preserve control IDs through the notification chain
   - Handle LVN_COLUMNCLICK properly in parent

## File Changes Required

### ClarionProcedureView.h
- Add container panel class declarations
- Replace direct CListViewCtrl members with container panels

### ClarionProcedureView.cpp
- Implement container panel classes
- Fix splitter position calculation (line 473)
- Update initialization to create containers instead of raw list views
- Ensure proper notification forwarding

## Testing Requirements
1. Verify splitter appears centered when tab first opens
2. Verify titles appear above both grids
3. Test all column header clicks trigger proper sorting:
   - Procedure name (alphabetical)
   - Times Opened/Closed/Net (numerical with secondary alphabetical sort)
   - Toggle between ascending/descending on repeated clicks