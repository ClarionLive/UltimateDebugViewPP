# Prompt for New Session - Timeline View Fixes

Please implement the improvements to the Timeline View in UltimateDebugViewPP as documented in `timeline-view-improvements.md`.

## Context
The Timeline View tab has been added to the detail pane in UltimateDebugViewPP. It displays procedure execution as a grid where columns are threads and rows are time intervals. The implementation is working but needs several usability improvements.

## Main Tasks

### Task 1: Fix Time Display Format
The timeline currently shows relative time in seconds (e.g., "0.123s"). Change this to show actual timestamps in HH:MM:SS.mmm format.

**Use a general-purpose agent to**:
- Analyze how timestamps are stored in ClarionProcedureTracker events
- Modify the `FormatTimeLabel()` function in `ClarionProcedureTimelineView.cpp` to properly format timestamps
- Ensure the time axis shows readable time values like "16:14:23.210"

### Task 2: Implement Event-Based Rows
Currently, the timeline creates fixed time slices resulting in many empty rows. Change this to only create rows when events actually occur.

**Use a general-purpose agent to**:
- Rewrite `CalculateTimeSlices()` in `ClarionProcedureTimelineView.cpp`
- Collect unique timestamps from all events
- Create one row per timestamp where something happens
- Update the drawing logic to work with event-based rows

### Task 3: Add Open/Close Indicators
Add clear visual indicators to show whether a procedure is opening or closing.

Implement in `DrawProcedureBlock()`:
- Add ">" prefix for open events and "<" prefix for close events
- Use different background colors (lighter for open, darker for closed)
- Ensure the indicators are clearly visible

### Task 4: Auto-size Column Width
Columns currently have fixed width causing procedure names to be truncated.

**Use a general-purpose agent to**:
- Calculate the maximum text width needed for each thread column
- Implement dynamic column sizing in `UpdateViewMetrics()`
- Set reasonable min (150px) and max (400px) limits
- Ensure thread headers and procedure names fit properly

### Task 5: Enhance Tooltips
Improve the tooltip information shown on hover.

Update the tooltip in `OnMouseMove()` to show:
- Thread ID
- Procedure name and module
- Event type (Open/Close)
- Timestamp
- Start time (for close events)
- Elapsed time (for close events)
- Call depth

### Task 6: Visual Polish (Optional)
If time permits, add visual improvements:
- Alternating row colors
- Row highlighting on hover
- Better grid lines
- Consistent color scheme

## Important Files
- **Main implementation**: `application\DebugViewpp\ClarionProcedureTimelineView.cpp`
- **Header file**: `application\DebugViewpp\ClarionProcedureTimelineView.h`
- **Documentation**: `timeline-view-improvements.md`

## Build Instructions
After making changes:
```powershell
cd C:\Dev\UltimateDebugViewPP
"C:\Program Files\CMake\bin\cmake.exe" --build build --config Debug --target UltimateDebugviewpp
```

The executable will be at: `C:\Dev\UltimateDebugViewPP\bin\Debug\UltimateDebugviewpp.exe`

## Testing
1. Run the application
2. Enable the Detail Pane (View menu → Detail Pane)
3. Click on the Timeline tab at the bottom
4. Load or generate some procedure tracking data
5. Verify:
   - Times show as actual timestamps
   - Only rows with events are shown
   - Open/close events are clearly indicated
   - Column widths accommodate full procedure names
   - Tooltips show comprehensive information

## Notes
- Use agents for complex analysis and implementation tasks
- The existing implementation provides a good foundation - focus on improving the specific issues
- Maintain compatibility with existing zoom and scroll functionality
- Test with various data sizes to ensure performance remains good