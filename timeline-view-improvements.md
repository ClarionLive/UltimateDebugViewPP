# Timeline View Improvements Plan

## Current State
The Timeline View tab has been successfully integrated into UltimateDebugViewPP's detail pane. It displays procedure execution as a grid where columns represent threads and rows represent time intervals. However, several improvements are needed for better usability.

## Issues Identified

### 1. Time Display Format
**Current**: Shows relative time in seconds (e.g., "0.123s")
**Needed**: Actual timestamps in format HH:MM:SS.mmm (e.g., "16:14:23.210")

### 2. Row Creation Logic
**Current**: Creates fixed time slices regardless of events, resulting in many empty rows
**Needed**: Event-based rows - only create rows when procedures open or close

### 3. Open/Close Indication
**Current**: No visual distinction between procedure open and close events
**Needed**: Clear indicators showing whether a procedure is opening or closing

### 4. Column Width
**Current**: Fixed width (100-300px) truncates procedure names
**Needed**: Auto-sized columns based on content with reasonable min/max limits

### 5. Tooltip Information
**Current**: Basic information (thread, procedure, module, duration)
**Needed**: Comprehensive details including start time, end time, elapsed time, and event type

## Proposed Solutions

### 1. Fix Time Display Format
**Location**: `ClarionProcedureTimelineView::FormatTimeLabel()`
- Parse the timestamp value properly
- Convert to HH:MM:SS.mmm format
- Handle the timestamp format used by ClarionProcedureTracker

### 2. Event-Based Row Creation
**Location**: `ClarionProcedureTimelineView::CalculateTimeSlices()`
- Collect all unique timestamps from events
- Create one row per unique timestamp
- Sort rows by time
- Eliminate empty time slices

### 3. Add Open/Close Indicators
**Location**: `ClarionProcedureTimelineView::DrawProcedureBlock()`
- Add visual prefixes: ">" for open, "<" for close
- Use different background colors:
  - Lighter shade for open events
  - Darker shade for close events
- Consider border styles: solid for open, dashed for closed

### 4. Auto-size Column Width
**Location**: `ClarionProcedureTimelineView::UpdateViewMetrics()`
- Calculate maximum text width per thread column
- Set dynamic cell width with:
  - Minimum: 150px
  - Maximum: 400px
  - Padding: 20px
- Ensure thread headers fit properly

### 5. Enhanced Tooltips
**Location**: `ClarionProcedureTimelineView::OnMouseMove()`
- Display comprehensive information:
  ```
  Thread: [ThreadID]
  Procedure: [Name]
  Module: [Module]
  Event: [Open/Close]
  Time: [HH:MM:SS.mmm]
  Start Time: [HH:MM:SS.mmm] (for close events)
  Elapsed: [X.XXXms] (for close events)
  Depth: [N]
  ```

### 6. Visual Improvements
**Locations**: Various drawing functions
- Add alternating row colors for readability
- Highlight row under mouse cursor
- Add subtle grid lines between cells
- Use consistent color scheme for procedures

## Implementation Details

### Files to Modify
1. **ClarionProcedureTimelineView.cpp** - Main implementation changes
2. **ClarionProcedureTimelineView.h** - Add any new member variables if needed

### Key Functions to Update
- `FormatTimeLabel()` - Time formatting
- `CalculateTimeSlices()` - Event-based rows
- `DrawProcedureBlock()` - Visual indicators
- `UpdateViewMetrics()` - Column sizing
- `OnMouseMove()` - Tooltip content
- `DrawGridCells()` - Visual enhancements

### Testing Considerations
- Test with procedures that have varying name lengths
- Test with multiple threads
- Test with rapid open/close events
- Test zoom functionality still works
- Test scrolling with new row logic
- Verify performance with large datasets

## Priority Order
1. **High**: Time display format (most visible issue)
2. **High**: Event-based rows (eliminates clutter)
3. **High**: Open/close indicators (critical for understanding)
4. **Medium**: Auto-size columns (improves readability)
5. **Medium**: Enhanced tooltips (adds detail)
6. **Low**: Visual improvements (polish)