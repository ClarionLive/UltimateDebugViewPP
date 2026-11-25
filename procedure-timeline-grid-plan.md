# Procedure Timeline Grid Visualization - Implementation Plan

## Overview
Enhancement to UltimateDebugViewPP to add a grid-based timeline visualization for procedure flow, showing thread execution patterns over time.

## Current State Analysis

### Existing Implementation
- **Location**: `application\DebugViewpp\ClarionProcedureView.cpp/h`
- **Current View**: Linear list showing chronological procedure events
- **Data Structure**: `ClarionProcedureTracker` maintains event-based procedure tracking
- **Display Features**:
  - Thread, Time, Procedure, Module, Status, Duration columns
  - Indentation using dots (..) to show call depth
  - Open/close indicators (> and <)
  - Click highlighting for matching open/close pairs

### Limitations of Current View
- Hard to visualize parallel execution across threads
- Difficult to see timing relationships
- Linear view doesn't show thread interactions clearly

## Proposed Grid Timeline Visualization

### Concept
A grid where:
- **Y-axis (Rows)**: Time intervals
- **X-axis (Columns)**: Thread IDs
- **Cells**: Show procedures executing at that time/thread intersection

### Visual Design
```
Time     | Thread 1      | Thread 2      | Thread 3
---------|---------------|---------------|---------------
0.000s   | Main()        |               |
0.100s   | ├─Init()      | Worker()      |
0.200s   | ├─Load()      | ├─Process()   | Monitor()
0.300s   | │             | ├─Calculate() | ├─Check()
0.400s   | └─Display()   | └─Save()      | └─Report()
```

### Key Features
1. **Color-coded procedures** - Same procedure gets same color across threads
2. **Vertical bars** - Show procedure duration visually
3. **Nested display** - Stack procedures within cells for call hierarchy
4. **Interactive** - Click to highlight, hover for details
5. **Scalable** - Zoom in/out on time axis

## Implementation Architecture

### 1. New Files to Create
- `application\DebugViewpp\ClarionProcedureTimelineView.h`
- `application\DebugViewpp\ClarionProcedureTimelineView.cpp`

### 2. Core Components

#### CProcedureTimelineGrid Class
```cpp
class CProcedureTimelineGrid : public CWindowImpl<CProcedureTimelineGrid>
{
    // Grid data structure
    struct TimeSlice {
        double startTime;
        double endTime;
        map<string, vector<ProcedureInfo>> threadProcedures;
    };

    // Rendering
    void DrawGrid(CDC& dc);
    void DrawTimeAxis(CDC& dc);
    void DrawThreadHeaders(CDC& dc);
    void DrawProcedureBlock(CDC& dc, const ProcedureInfo& proc);

    // Interaction
    void OnCellClick(int row, int col);
    void OnZoom(int delta);
    void SetTimeScale(double pixelsPerSecond);
};
```

#### Data Processing
- Convert event stream to time-slice representation
- Calculate optimal time intervals based on data density
- Maintain procedure stack state per thread per time slice

### 3. Integration Points

#### Modify ClarionProcedureView
- Add third tab "Timeline View"
- Create and manage timeline grid control
- Share ClarionProcedureTracker instance

#### Enhance ClarionProcedureTracker
- Add method: `GetProceduresAtTime(double time)`
- Add method: `GetTimeRange()` for min/max timestamps
- Add method: `GetProceduresByTimeSlice(double start, double end)`

### 4. Rendering Strategy

#### Double Buffering
- Create off-screen bitmap
- Render complete frame
- Blit to screen to prevent flicker

#### Virtual Scrolling
- Only render visible time range
- Calculate visible rows based on scroll position
- Lazy-load procedure data as needed

#### Cell Rendering
```cpp
void DrawProcedureCell(CDC& dc, int x, int y, int width, int height,
                       const vector<ProcedureInfo>& procedures)
{
    // Draw background
    // Stack procedures vertically within cell
    // Draw procedure name (abbreviated if needed)
    // Add depth indicators
    // Apply color coding
}
```

### 5. User Interaction Features

#### Mouse Operations
- **Click**: Select and highlight procedure across all occurrences
- **Double-click**: Navigate to procedure in main log view
- **Right-click**: Context menu (copy, export, etc.)
- **Wheel scroll**: Zoom time scale
- **Drag**: Pan timeline

#### Keyboard Shortcuts
- `+/-`: Zoom in/out
- `Arrow keys`: Navigate cells
- `Home/End`: Jump to start/end of timeline
- `Ctrl+F`: Find procedure
- `Esc`: Clear selection

### 6. Performance Optimizations

#### Caching
- Cache rendered cells
- Store calculated time slices
- Pre-compute procedure colors

#### Rendering Optimizations
- Clip drawing to visible region
- Use GDI+ for anti-aliased text
- Batch similar draw operations

#### Data Structure Optimizations
- Use interval tree for time-based queries
- Index procedures by thread and time
- Lazy evaluation of time slices

## Implementation Phases

### Phase 1: Basic Grid (2-3 days)
- Create timeline view class
- Add new tab to procedure view
- Implement basic grid rendering
- Display thread columns and time rows

### Phase 2: Procedure Display (2-3 days)
- Convert events to time slices
- Render procedure blocks in cells
- Implement color coding
- Show procedure names

### Phase 3: Interactivity (2 days)
- Add mouse click handling
- Implement selection highlighting
- Add zoom functionality
- Create context menu

### Phase 4: Polish (2 days)
- Optimize rendering performance
- Add tooltips with procedure details
- Implement keyboard navigation
- Add export functionality

### Phase 5: Testing & Refinement (1-2 days)
- Test with large datasets
- Handle edge cases
- Performance profiling
- UI polish

## Technical Considerations

### Threading
- Ensure thread-safe access to ClarionProcedureTracker
- Use locks when updating shared data
- Consider background processing for large datasets

### Memory Management
- Limit cached time slices
- Use smart pointers for procedure data
- Implement LRU cache for rendered cells

### Scalability
- Support datasets with 1000+ procedures
- Handle timespans from milliseconds to hours
- Manage 10+ concurrent threads

## Testing Strategy

### Unit Tests
- Time slice calculation
- Procedure stacking logic
- Color assignment algorithm

### Integration Tests
- Tab switching
- Data synchronization
- Event handling

### Performance Tests
- Large dataset rendering
- Scroll performance
- Memory usage

## Alternative Approaches Considered

### 1. Using External Grid Control
- **Pros**: Feature-rich, tested
- **Cons**: Licensing, integration complexity

### 2. HTML/JavaScript View
- **Pros**: Rich visualization libraries
- **Cons**: Complex integration with C++ app

### 3. Direct2D Rendering
- **Pros**: Hardware acceleration
- **Cons**: More complex implementation

## Conclusion
The grid timeline visualization will provide significant value for understanding multi-threaded procedure execution patterns. The implementation is feasible within the existing architecture and can be developed incrementally.