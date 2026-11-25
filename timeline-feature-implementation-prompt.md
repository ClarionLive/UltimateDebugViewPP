# Implementation Prompt for Timeline Context Menu Feature

## Task Overview
Please implement the Timeline context menu feature for UltimateDebugViewPP as described in the `timeline-context-menu-feature.md` file. This feature adds a right-click context menu to the Timeline view with two options:
1. "Create View" - Creates a new filtered view for the selected procedure
2. "Jump to debug row" - Navigates to the source row where the procedure event originated

## Implementation Instructions

### Step 1: Review Documentation
First, read the `timeline-context-menu-feature.md` file to understand the full feature specification and implementation plan.

### Step 2: Use Subagents for Complex Tasks
**IMPORTANT**: Use the Task tool with appropriate subagents for the following complex tasks:

1. **For source tracking implementation** (Phase 1):
   - Use a general-purpose agent to search for and understand how messages flow from LogFile to the Timeline view
   - Ask it to trace the data flow and identify the best points to inject source tracking information

2. **For view creation logic** (Phase 3):
   - Use a general-purpose agent to analyze the existing view creation patterns in MainFrame
   - Ask it to find all places where AddFilterView is called and understand the filter setup patterns

3. **For navigation implementation** (Phase 4):
   - Use a general-purpose agent to search for existing code that switches between views and jumps to specific lines
   - Ask it to find examples of SetFocusLine, ScrollToIndex usage

### Step 3: Implementation Order

Implement the features in this order for best results:

1. **Start with the context menu UI** (Phase 2)
   - Add the context menu to ClarionProcedureTimelineView::OnContextMenu
   - Add menu command IDs to resource.h
   - Create placeholder handlers for the menu items

2. **Implement "Create View" feature** (Phase 3)
   - This can work without source tracking
   - Add CreateProcedureView method to MainFrame
   - Connect the menu handler to create the filtered view

3. **Basic "Jump to debug row"** (Phase 4 simplified)
   - Initially, just jump to the approximate time in the current view
   - This provides immediate value without complex tracking

4. **Enhanced source tracking** (Phase 1 & 4 complete)
   - Only after basic features work
   - Implement full source location tracking
   - Upgrade jump feature to use exact source location

### Step 4: Testing

After each phase, test the implementation:
1. Build the project using: `cmake --build build --config Debug --target UltimateDebugviewpp`
2. Run the application and test with Clarion debug output
3. Verify the context menu appears and functions work

### Step 5: Code Quality

Ensure:
- All new code follows the existing code style (check neighboring files)
- No memory leaks (use smart pointers where appropriate)
- Proper error handling for edge cases
- Comments for complex logic

## Key Files to Modify

1. `application\DebugViewpp\ClarionProcedureTimelineView.cpp` - Main implementation
2. `application\DebugViewpp\ClarionProcedureTimelineView.h` - Add member variables and methods
3. `application\DebugViewpp\MainFrame.cpp` - Add CreateProcedureView method
4. `application\DebugViewpp\MainFrame.h` - Declare new methods
5. `application\include\DebugViewppLib\ClarionProcedureTracker.h` - Extend Event structure
6. `application\DebugViewppLib\LogFile.cpp` - Add source tracking (if implementing full feature)
7. `application\DebugViewpp\resource.h` - Add menu command IDs

## Important Notes

- The Timeline feature is new, so the context menu is currently empty (TODO comment at line 480)
- Use the existing Filter and LogFilter structures - don't create new ones
- The application uses WTL for UI, not MFC
- Test with actual Clarion debug output that contains procedure open/close messages
- If you encounter any Clarion-specific parsing issues, check LogFile::Add for the parsing logic

## Success Criteria

You've successfully completed the task when:
- [ ] Right-clicking on a procedure in Timeline shows a context menu
- [ ] "Create View" creates a new tab with the procedure filtered
- [ ] "Jump to debug row" navigates to the source (or shows appropriate message if unavailable)
- [ ] The implementation handles edge cases gracefully
- [ ] The code compiles without warnings
- [ ] The feature works reliably with real Clarion debug output

Start by reading the feature specification, then use subagents to understand the codebase better, and finally implement the features incrementally. Good luck!