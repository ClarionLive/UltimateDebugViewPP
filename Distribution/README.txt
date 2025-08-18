=========================================
    ULTIMATE DEBUGVIEW++ - ENHANCED EDITION
=========================================

QUICK START
-----------
1. Run UltimateDebugviewpp.exe
2. No installation required - just run!
3. fsqlf.dll must be in the same folder for SQL formatting
4. First run? Import your DebugView++ settings automatically!

NEW FEATURES
------------
✓ Settings Import - Seamlessly import settings from DebugView++
✓ Dark Mode - View → Dark Mode (works with Windows 10/11)
✓ Enhanced Detail Pane - Press Ctrl+D to toggle (hidden by default)
✓ Syntax Highlighting - JSON, XML, SQL, Log Levels, Stack Traces
✓ SQL Formatting - Professional SQL statement formatting
✓ Clarion Debug Support - Native support for Clarion debug format
✓ Auto-Create Views - Apps can create their own views with @@NEWDEBUGVIEW@@
✓ Column Layouts - Save/restore up to 10 column configurations
✓ Process Filter Dropdown - Auto-populated process list in filters
✓ Tab-Specific Settings - Each tab remembers its own configuration
✓ Expanded Toolbar Option - Text labels for toolbar buttons

KEY SHORTCUTS
-------------
Ctrl+D    - Toggle Detail Pane
Ctrl+F    - Find
Ctrl+L    - Filter
Ctrl+X    - Clear View
Ctrl+C    - Copy Selection
Ctrl+A    - Select All
Ctrl+E    - Auto Scroll
Space     - Pause/Resume
Ctrl+F2   - Toggle Bookmark
F2        - Next Bookmark
Shift+F2  - Previous Bookmark
Ctrl+N    - New Tab
Ctrl+W    - Close Tab
Ctrl+S    - Save Log

COLUMN MANAGEMENT
-----------------
• Right-click column header for options
• Drag headers to reorder (except Bookmark)
• Save/Load layouts via right-click menu
• "Reset Layout" to restore defaults
• Clarion columns hidden by default

DETAIL PANE FEATURES
--------------------
• Scintilla-powered editor
• Automatic format detection
• Line numbers & word wrap
• Independent font settings
• Professional SQL formatting
• Theme-aware colors
• Per-tab settings

SETTINGS IMPORT FROM DEBUGVIEW++
---------------------------------
• First run: Automatically prompts to import existing settings
• Manual import: Options → Import DebugView++ Settings...
• Imports: Views, filters, column layouts, fonts, preferences
• Registry: Settings stored in Software\ClarionLive\UltimateDebugView++
• No conflicts: Can run alongside original DebugView++

AUTO-VIEW CREATION
------------------
Applications can auto-create their own debug view by sending:
    OutputDebugString("@@NEWDEBUGVIEW@@")

This will create or switch to a dedicated view for that process.

TIPS
----
• Enable Process Colors for visual separation
• Link Views to synchronize scrolling
• Each tab has independent detail pane settings
• Detail pane shows format type when detected
• SQL formatting requires fsqlf.dll
• Dark mode integrates with Windows theme

USER MANUAL
-----------
Open UserManual.html in your browser for the complete guide
with detailed feature descriptions and examples.

SUPPORT
-------
For issues or feature requests, please refer to the 
project repository documentation.

=========================================
Enjoy debugging with Ultimate DebugView++!
=========================================