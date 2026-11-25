# Prompt for New Session - Procedure Timeline Grid Implementation

Please implement a new Timeline Grid visualization for the Procedure Flow feature in UltimateDebugViewPP. The complete analysis and plan is in `procedure-timeline-grid-plan.md`.

## Task Overview
Create a grid-based timeline view where:
- Rows represent time intervals
- Columns represent threads
- Cells show procedures executing at that time/thread intersection

## Key Requirements
1. Add a third tab "Timeline View" to the existing Procedure Flow panel
2. Create a custom grid control that visualizes procedure execution over time
3. Support interactive features like zoom, selection, and navigation
4. Maintain good performance with large datasets

## Implementation Instructions

### Step 1: Create the Timeline View Classes
Use a **general-purpose agent** to:
- Create `ClarionProcedureTimelineView.h` and `.cpp` files
- Implement a custom-drawn grid control based on CWindowImpl
- Model the implementation after the existing `ClarionProcedureView` and `CQueueGridView` classes for consistency

### Step 2: Integrate with Existing Code
- Modify `ClarionProcedureView.cpp` to add the new Timeline tab
- Share the existing `ClarionProcedureTracker` data
- Ensure the new tab appears alongside "Procedure Flow" and "Open Procedures"

### Step 3: Implement Core Grid Rendering
Use a **general-purpose agent** to research and implement:
- Grid drawing with time axis (rows) and thread headers (columns)
- Cell rendering to show procedure blocks
- Color coding for different procedures
- Double buffering to prevent flicker

### Step 4: Data Processing
- Convert the event-based data from `ClarionProcedureTracker::GetChronologicalEvents()` into time-slice representation
- Calculate appropriate time intervals based on the data range
- Group procedures by thread and time slice

### Step 5: Add Interactivity
- Click to select/highlight procedures
- Mouse wheel zoom for time scale adjustment
- Tooltips showing procedure details on hover
- Context menu for additional operations

### Step 6: Test and Optimize
- Test with the existing procedure tracking data
- Ensure smooth scrolling and rendering
- Profile and optimize any performance bottlenecks

## Important Context
- The project uses WTL (Windows Template Library) for UI
- Follow the existing code patterns in `ClarionProcedureView.cpp`
- The build system uses CMake - remember to add new files to `CMakeLists.txt`
- Use Visual Studio 2022 for building

## File Locations
- Main procedure view: `application\DebugViewpp\ClarionProcedureView.cpp/h`
- Data tracker: `application\include\DebugViewppLib\ClarionProcedureTracker.h`
- Example grid view: `application\DebugViewpp\CQueueGridView.h`

Start by reviewing the existing code structure, then proceed with implementing the timeline grid following the plan in `procedure-timeline-grid-plan.md`. Use agents where appropriate for complex research or implementation tasks.