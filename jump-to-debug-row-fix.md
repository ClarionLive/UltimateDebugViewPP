# Fix for "Jump to Debug Row" Feature

## Problem Description
The timeline's "Jump to Debug Row" context menu option doesn't work correctly. When right-clicking on a procedure event in the timeline and selecting "Jump to Debug Row", it should jump to the original log line where that procedure event occurred, but instead it shows a "Feature Not Yet Available" message or jumps to the wrong row.

## Root Cause
The line index information from the original debug log is not being tracked when procedure events (OPEN/CLOSE) are processed. The system processes messages and creates procedure events but doesn't maintain the connection to the original line numbers.

## Current Code Flow
```
LogView parsing loop (has line index i)
  → ParseClarionMessage (doesn't receive line index)
    → ParseClarionFormat (doesn't receive line index)
      → HandleProcedureOpen/Close (can't store line index)
        → Event creation (no line index available)
          → Timeline display (no line index to use)
            → Jump to debug row (sends -1 instead of actual line)
```

## Solution Overview
Add line index tracking throughout the procedure event processing chain, from initial message parsing through to timeline display and jump functionality.

## Implementation Details

### 1. Data Structure Updates

#### ClarionProcedureTracker.h
- Add `int lineIndex` field to the `Event` structure
- Update constructor to accept line index parameter (default -1)
- Add line index parameter to `HandleProcedureOpen` and `HandleProcedureClose`

#### ProcedureNode structure
- Add `int lineIndex` field to track where procedure was opened
- Initialize with -1 for backward compatibility

### 2. Function Signature Changes

#### ClarionProcedureTracker.cpp
```cpp
// Update function signatures
int HandleProcedureOpen(const std::string& threadId, const std::string& procedureName,
                        const std::string& module, const std::string& app,
                        const std::string& processName, double timestamp,
                        int lineIndex = -1);

int HandleProcedureClose(const std::string& threadId, const std::string& procedureName,
                         const std::string& processName, double timestamp,
                         int lineIndex = -1);
```

### 3. ParseClarionFormat Updates

#### LogFile.cpp
- Add optional `int lineIndex = -1` parameter to `ParseClarionFormat`
- Pass line index to `HandleProcedureOpen` and `HandleProcedureClose` calls
- Update all call sites to pass line index where available

### 4. LogView Integration

#### LogView.cpp
- Find where `ParseClarionMessage` is called in the message processing loop
- Pass the current line index `i` as an additional parameter
- Ensure the line index flows through to procedure tracking

### 5. Timeline Context Menu Fix

#### ClarionProcedureTimelineView.cpp
```cpp
LRESULT ClarionProcedureTimelineView::OnTimelineJumpToRow(...)
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
        // Use actual line index from the event instead of -1
        ::SendMessage(hMainFrame, WM_USER_JUMP_TO_DEBUG_ROW,
                      static_cast<WPARAM>(m_contextMenuEvent->lineIndex),
                      0);
    }

    return 0;
}
```

## Files to Modify
1. `application\include\DebugViewppLib\ClarionProcedureTracker.h` - Add lineIndex to Event struct
2. `application\DebugViewppLib\ClarionProcedureTracker.cpp` - Implement line index storage
3. `application\DebugViewppLib\LogFile.cpp` - Pass line indices through parsing
4. `application\DebugViewpp\LogView.cpp` - Provide line index during message processing
5. `application\DebugViewpp\ClarionProcedureTimelineView.cpp` - Use actual line index

## Testing
1. Open a debug log with Clarion procedure events
2. Open the procedure timeline view
3. Right-click on any procedure event
4. Select "Jump to Debug Row"
5. Verify it jumps to the correct line in the main log view

## Backward Compatibility
- All line index parameters default to -1
- Existing code without line indices will continue to work
- The feature gracefully handles missing line index information