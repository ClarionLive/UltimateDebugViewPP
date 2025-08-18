# DebugView++ Enhancements

## Overview
This document describes the enhancements made to DebugView++ to improve user experience, including dark mode support, enhanced detail pane functionality, and improved readability features.

## Features Implemented

### 1. Dark Mode Support
A comprehensive dark mode theme has been implemented that can be toggled from the View menu.

#### Key Components:
- **ThemeManager**: Central singleton class for managing themes (Light/Dark/System)
- **Theme Integration**: Integration with Windows 10/11 dark mode APIs for native title bar theming
- **Dynamic Colors**: All UI colors are now dynamic and update when switching themes
- **Persistent Settings**: Theme preference is saved to registry and restored on startup

#### Files Modified/Created:
- `application/DebugViewppLib/ThemeManager.h/cpp` - Core theme management system
- `application/include/DebugViewppLib/Colors.h` - Updated to support dynamic colors
- `application/DebugViewppLib/Colors.cpp` - Implementation of theme-aware colors
- `MainFrame.cpp` - Added dark mode menu handler and theme application

#### Dark Mode Colors:
- Background: RGB(30, 30, 30)
- Text: RGB(220, 220, 220)
- Selection: RGB(51, 153, 255)
- Highlight: RGB(96, 96, 96)

### 2. Process Colors Contrast Fix
Fixed an issue where Process Colors feature would display unreadable text (white on light backgrounds) in dark mode.

#### Solution:
- Added `GetContrastingTextColor()` function that calculates appropriate text color based on background luminance
- Uses WCAG 2.0 luminance formula for optimal contrast
- Automatically selects black text for light backgrounds, white text for dark backgrounds

#### Files Modified:
- `application/include/DebugViewppLib/Colors.h` - Added GetContrastingTextColor declaration
- `application/DebugViewppLib/Colors.cpp` - Implemented contrast calculation
- `application/DebugViewpp/LogView.cpp` - Updated GetTextColor to use contrast-aware colors

### 3. Detail Pane Enhancements

#### 3.1 RichEdit Control Implementation
Replaced standard Edit control with RichEdit for better formatting support in dark mode.

- Supports proper background and text color customization
- Better text rendering in dark mode
- Maintains monospace font for code readability

#### 3.2 Detail Pane Toolbar
Enhanced the detail pane toolbar to match main application styling:
- Toolbar remains light gray in all themes (consistent with main toolbar)
- Buttons are transparent/flat style matching main toolbar
- Added proper hover effects

#### 3.3 Font Button
Added a dedicated Font button to the detail pane toolbar allowing users to customize the font independently from the main list.

**Features:**
- Separate font storage for detail pane
- Defaults to Consolas 9pt (optimal for code/structured data)
- Font settings persist across sessions
- Saved to registry as `DetailFontName` and `DetailFontSize`

#### Files Modified:
- `CLogViewTabItem2.h/cpp` - Added Font button, handlers, and font management
- `MainFrame.h/cpp` - Added detail font storage and persistence
- `resource.h` - Added IDC_DETAIL_FONT resource ID

### 4. Icon Corrections
Fixed icon assignments in detail pane toolbar:
- Copy All button: Now uses the standard copy icon (index 2) matching main toolbar
- Font button: Uses the 'A' text/font icon (index 6)
- Find button: Uses magnifying glass icon (index 8)
- Filter button: Uses filter/funnel icon (index 10)

### 5. Syntax Highlighting System
Implemented comprehensive syntax highlighting for the detail pane to improve readability of structured debug output.

#### 5.1 Architecture
- **RTF Generation**: Uses Rich Text Format generation instead of range-based formatting for reliable colorization
- **Format Detection**: Automatic detection of content type (JSON, XML, SQL, Log Levels, Stack Traces)
- **Theme Support**: Different color schemes for light and dark themes
- **Font Preservation**: Respects user's selected font while applying syntax colors

#### 5.2 Supported Formats

##### JSON Highlighting
- **Properties/Keys**: Blue (light theme) / Light blue (dark theme)
- **Strings**: Green (light theme) / Light brown (dark theme)
- **Numbers**: Orange (light theme) / Light green (dark theme)
- **Booleans**: Purple (light theme) / Light purple (dark theme)
- **Null values**: Gray
- Handles nested objects and arrays with proper indentation

##### XML Highlighting
- **Tags**: Blue with bold
- **Attributes**: Orange
- **Attribute values**: Green
- **Comments**: Green-gray (preserves content even with special characters like `>`)
- **CDATA sections**: Purple
- **XML declarations**: Gray
- Properly handles malformed XML and single-quoted attributes

##### SQL Highlighting & Formatting
- **Keywords**: Blue (SELECT, INSERT, UPDATE, DELETE, CREATE, etc.)
- **Functions**: Purple (COUNT, MAX, MIN, AVG, etc.)
- **Strings**: Green
- **Numbers**: Orange
- **Comments**: Gray (both `--` and `/* */` styles)
- **Professional SQL Formatting**: Integrated fsqlf library for proper SQL formatting
  - Automatic indentation of subqueries
  - Proper JOIN alignment and formatting
  - Keyword alignment and spacing
  - Comment preservation
  - Handles complex multi-table queries

##### Log Level Highlighting
Supports both formats: `ERROR:` and `[ERROR]`
- **Error levels** (ERROR, FATAL, CRITICAL): Red/Light red with bold
- **Warning levels** (WARN, WARNING): Orange/Light orange with bold
- **Info level** (INFO): Blue/Light blue with bold
- **Debug level** (DEBUG): Green/Light green with bold
- **Trace level** (TRACE): Gray/Light gray with bold

##### Stack Trace Highlighting
- **Exception names**: Red with bold
- **Method/Class names**: Blue
- **File paths**: Green
- **Line numbers**: Orange
- **Keywords** ("at", "Inner Exception"): Gray
- Detects .NET/Java style traces, GDB format, and explicit "Stack trace" text

#### 5.3 Detection Priority
Format detection follows a specific order to handle overlapping patterns:
1. **SQL** - Checked first to prevent false JSON detection due to parentheses/brackets
2. **JSON/XML** - Specific structured formats
3. **Stack Traces** - Before log levels to catch "ERROR: Stack trace..." patterns
4. **Log Levels** - Common in debug output

#### 5.4 Implementation Details
- **Files Created/Modified**:
  - `application/DebugViewppLib/SyntaxHighlighter.h/cpp` - Core syntax highlighting engine
  - `application/include/DebugViewppLib/ThemeManager.h/cpp` - Theme integration
  - `CLogViewTabItem2.cpp` - RTF streaming implementation using EM_STREAMIN
  - `application/DebugViewppLib/SqlFormatter.cpp` - SQL formatting with fsqlf integration
  - `fsqlf_dll/` - Build configuration for fsqlf SQL formatter DLL

- **Technical Approach**:
  - State machine parsing for complex formats like XML
  - RTF generation with embedded color tables
  - Font settings preserved through RTF font table
  - Efficient character-by-character processing for malformed content
  - Dynamic DLL loading for SQL formatting with fallback

### 6. Professional SQL Formatting Integration

#### 6.1 Overview
Integrated the Free SQL Formatter (fsqlf) library as a dynamically loaded DLL to provide professional SQL formatting capabilities.

#### 6.2 Features
- **Automatic SQL Detection**: Identifies SQL statements in debug output with improved detection priority
- **Professional Formatting**: 
  - Proper keyword alignment (SELECT, FROM, WHERE, JOIN, etc.)
  - Intelligent indentation for subqueries
  - Correct handling of complex JOINs (LEFT JOIN, INNER JOIN, etc.)
  - Preservation of inline and block comments
  - Configurable formatting rules
- **Dynamic Loading**: fsqlf.dll loads at runtime with graceful fallback
- **Wide String Support**: Custom wrapper for Windows wide character support

#### 6.3 Implementation
- **fsqlf Library**: Open-source SQL formatter (LGPL v3) from https://github.com/dnsmkl/fsqlf
- **Build Process**:
  - Downloaded win_flex to generate lexer from .lex files
  - Created DLL wrapper with Windows-specific adaptations
  - Built as separate DLL to maintain license independence
- **Integration Points**:
  - `SqlFormatter::Format()` - Attempts to load and use fsqlf.dll
  - Falls back to simple formatter if DLL unavailable
  - Logs all operations for debugging

#### 6.4 Files Added/Modified
- **New Files**:
  - `fsqlf_dll/` - Build directory for fsqlf DLL
  - `application/DebugViewppLib/SqlFormatter.cpp/h` - SQL formatting wrapper
  - `application/include/DebugViewppLib/fsqlf_dll.h` - DLL interface definitions
  - `FSQLF_ATTRIBUTION.md` - License compliance documentation
- **Modified Files**:
  - `application/DebugViewppLib/SyntaxHighlighter.cpp` - Integrated SQL formatter
  - `application/DebugViewppLib/CMakeLists.txt` - Added SqlFormatter.cpp

#### 6.5 Benefits
- Professional SQL formatting without writing a complex parser
- Handles edge cases like comments, strings, and nested queries
- Maintained as separate DLL for license compliance
- Can be reused in other projects
- Users can update fsqlf.dll independently

## Build Instructions

### Prerequisites
- Visual Studio 2022
- CMake 3.27.7 or later (included in repository)
- Windows SDK

### Building the Project

1. **Generate Visual Studio Solution:**
   ```bash
   ./cmake-3.27.7-windows-x86_64/bin/cmake.exe -DCMAKE_INSTALL_PREFIX=install -G "Visual Studio 17 2022" -B vs2022 .
   ```

2. **Build the Application:**
   ```bash
   ./cmake-3.27.7-windows-x86_64/bin/cmake.exe --build vs2022 --config Release --target UltimateDebugviewpp
   ```

   Or for the standard version:
   ```bash
   ./cmake-3.27.7-windows-x86_64/bin/cmake.exe --build vs2022 --config Release --target Debugviewpp
   ```

### Running the Application

**Standard Version:**
```bash
start C:/Dev/DebugViewPP/bin/Release/Debugviewpp.exe
```

**Ultimate Version:**
```bash
start C:/Dev/DebugViewPP/bin/Release/UltimateDebugviewpp.exe
```

Or simply navigate to `bin\Release\` and double-click the executable.

## Taking Screenshots

### Using Windows Snipping Tool
1. Press `Win + Shift + S` to open Snipping Tool
2. Select the area to capture
3. The screenshot will be copied to clipboard

### Using MCP WindowsSnapIt Tool (if available)
```bash
# Capture by process name
mcp__windowssnapit-mcp__take_screenshot --processName UltimateDebugviewpp

# Capture by window title
mcp__windowssnapit-mcp__take_screenshot --windowTitle "Ultimate Debugview ++"
```

## Testing the Features

### Dark Mode
1. Launch the application
2. Go to View menu → Dark Mode
3. The entire application should switch to dark theme
4. Check mark should appear next to Dark Mode menu item
5. Theme preference should persist after restart

### Process Colors
1. Enable View → Process Colors
2. Each process should get a unique background color
3. Text should automatically be black on light colors, white on dark colors
4. Should work correctly in both light and dark modes

### Detail Pane
1. Enable View → Detail Pane
2. Select any log line to see full message in detail pane
3. Detail pane should respect dark mode setting
4. Use buttons in detail pane toolbar:
   - **Find**: Search for selected text in main view
   - **Filter**: Open filter dialog with selected text
   - **Copy All**: Copy entire detail text to clipboard
   - **Font**: Customize detail pane font
   - **Word Wrap**: Toggle word wrapping

### Font Customization
1. Main list font: Options → Font
2. Detail pane font: Click Font button in detail pane toolbar
3. Both fonts are saved independently and persist across sessions

### Syntax Highlighting
1. Enable View → Detail Pane
2. Select any log line containing structured data
3. The detail pane will automatically detect and highlight:
   - **JSON**: Object properties, strings, numbers, booleans
   - **XML**: Tags, attributes, comments, CDATA sections
   - **SQL**: Keywords, functions, strings, comments
   - **Log Levels**: ERROR, WARN, INFO, DEBUG, TRACE (both `ERROR:` and `[ERROR]` formats)
   - **Stack Traces**: Exception names, method calls, file paths
4. Toggle syntax highlighting with the Syntax button in detail pane toolbar
5. Colors automatically adjust for dark/light theme

## Registry Settings

Settings are stored in: `HKEY_CURRENT_USER\Software\CobaltFusion\DebugView++`

Key settings:
- `Theme`: Current theme (0=Light, 1=Dark, 2=System)
- `FontName` / `FontSize`: Main list font
- `DetailFontName` / `DetailFontSize`: Detail pane font
- `DetailWordWrap`: Word wrap state for detail pane
- `DetailSyntaxHighlight`: Syntax highlighting state for detail pane
- `ShowProcessColors`: Process colors enabled state

## Known Issues Resolved

1. **White text on light backgrounds with Process Colors** - Fixed with contrast calculation
2. **Detail pane not respecting dark mode** - Fixed with RichEdit implementation
3. **Dark mode text invisible on startup** - Fixed with proper theme initialization
4. **Wrong icons on detail pane buttons** - Fixed with correct icon indices
5. **XML syntax highlighting broken with `>` in comments** - Fixed with state machine parser
6. **Log messages incorrectly detected as SQL** - Fixed with stricter SQL detection and proper format priority
7. **Bracketed log levels `[ERROR]` not highlighted** - Added support for both bracket and colon formats

## Architecture Notes

### Theme System
- ThemeManager uses singleton pattern for centralized theme management
- Colors are mutable and update globally when theme changes
- Windows dark mode APIs used for native title bar theming
- All views refresh when theme changes

### Detail Pane Architecture
- Uses WTL (Windows Template Library) for UI components
- RichEdit control for better text formatting
- Custom button controls with hover effects
- Separate font management from main list view

### 7. Scintilla Integration for Enhanced Detail Pane (NEW)

#### 7.1 Overview
Replaced the RichEdit control in the detail pane with Scintilla text editor for professional-grade syntax highlighting and text editing capabilities.

#### 7.2 Key Features Implemented
- **Native Syntax Highlighting**: Using Scintilla's Lexilla lexer library for accurate syntax coloring
- **Line Numbers**: Optional line number display with theme-aware colors
- **Word Wrap**: Toggle word wrapping for long lines
- **Format Auto-Detection**: Automatic detection and display of content format (SQL, JSON, XML, Plain Text)
- **Professional SQL Formatting**: Integration with fsqlf.dll for SQL beautification
- **Theme Support**: Full dark/light theme support with proper initialization

#### 7.3 Technical Implementation

##### Scintilla/Lexilla Integration
- **Scintilla Control**: Professional text editor control used by Notepad++, SciTE, and many IDEs
- **Lexilla Library**: Modular lexer library providing syntax analysis for 100+ languages
- **Direct Function Access**: Using Scintilla's direct function pointer for optimal performance
- **Lexer Configuration**: Custom configuration for SQL (MSSQL), JSON, and XML lexers

##### Files Created/Modified
- **New Files**:
  - `application/DebugViewpp/ScintillaWrapper.h/cpp` - Wrapper class for Scintilla control
  - `scintilla/` - Scintilla 5.x library source
  - `lexilla/` - Lexilla lexer library source
- **Modified Files**:
  - `CLogViewTabItem2.h/cpp` - Integration of Scintilla control
  - `CMakeLists.txt` - Build configuration for Scintilla/Lexilla

#### 7.4 Bugs Fixed
1. **Line Numbers Invisible in Dark Mode**: Created ApplyLineNumberColors helper to ensure proper theme colors
2. **SQL Formatting Not Using fsqlf.dll**: Fixed method routing to use the professional formatter
3. **Syntax Highlighting Not Working**: Implemented SetLexer method with proper Lexilla integration
4. **XML Misidentified as JSON**: Reordered format detection to check XML before JSON
5. **Plain Text Misidentified as JSON**: Made JSON detection more strict to avoid false positives
6. **Line Numbers Not Showing on Startup**: Fixed initialization to apply saved settings
7. **Dark Theme Not Applied Initially**: Added theme application during control creation
8. **Debug Output Flooding Viewer**: Removed all debug statements after testing

#### 7.5 Format Detection Display
Added status bar in detail pane showing the detected format:
- Displays "Format: SQL/JSON/XML/Plain Text" when syntax highlighting is enabled
- Shows "(detected)" suffix when highlighting is off but format is recognized
- Helps users understand why certain highlighting is applied

#### 7.6 User Interface Enhancements
- **Toolbar Buttons**: 
  - Find: Search for selected text in main view
  - Filter: Create filter from selected text
  - Word Wrap: Toggle word wrapping
  - Syntax: Toggle syntax highlighting
  - Format: Toggle content formatting (JSON/XML/SQL beautification)
  - Line Numbers: Toggle line number display
  - Copy: Copy all text to clipboard
  - Font: Select custom font
- **Persistent Settings**: All toolbar button states saved and restored
- **Theme-Aware**: All controls properly respect dark/light theme

### 8. Expanded Toolbar Option

#### 8.1 Overview
Added an "Expanded Toolbar" option under the Options menu that allows users to toggle between a compact toolbar (icons only) and an expanded toolbar (icons with text labels) for improved usability and accessibility.

#### 8.2 Features
- **Toggle Option**: Options → Expanded Toolbar menu item with checkmark indicator
- **Text Labels**: When enabled, displays descriptive text labels below each toolbar icon:
  - Open, Save Log, Copy, Clear, Auto Scroll, Clock Time, Font, Filter
  - Link Views, Bookmark, Previous, Next, Clear, Pause
- **Persistent Setting**: User preference saved to registry and restored on startup
- **Proper Layout**: Toolbar appears on its own line below the menu bar for cleaner interface
- **Seamless Switching**: Smooth transition between compact and expanded modes without visual glitches

#### 8.3 Technical Implementation
- **Dynamic Toolbar Creation**: Uses `TBSTYLE_LIST` and `TBSTYLE_EX_MIXEDBUTTONS` for text labels
- **Band Management**: Proper rebar band handling to prevent duplicate toolbars
- **Window Cleanup**: Correctly destroys old toolbar windows when switching modes
- **Registry Storage**: Settings saved to `HKEY_CURRENT_USER\Software\CobaltFusion\DebugView++\ExpandedToolbar`

#### 8.4 Files Modified
- `MainFrame.cpp`: Added `ApplyExpandedToolbarState()` and `OnExpandedToolbar()` methods
- `MainFrame.h`: Added `m_expandedToolbar` member variable
- `resource.h`: Added `ID_OPTIONS_EXPANDED_TOOLBAR` resource ID
- `DebugView++.rc`: Added menu item for Expanded Toolbar option

### 9. Process Filter Dropdown Enhancement

#### 9.1 Overview
Enhanced the Filter window's Processes tab with a dropdown list showing all processes that have appeared in the debug grid. This eliminates the need to manually type process names and reduces errors from typos.

#### 9.2 Features
- **Smart Dropdown**: Automatically populated with all unique process names from the debug log
- **Auto-Complete**: Select from existing processes or type a custom name
- **Sorted List**: Process names are alphabetically sorted for easy navigation
- **Dynamic Updates**: The list includes all processes that have appeared since DebugView++ was started
- **Backwards Compatible**: Still allows manual entry of process names not yet in the log

#### 9.3 Technical Implementation
- **Process Collection**: `LogFile::GetUniqueProcesses()` method extracts all unique process names from the message log
- **Filter Dialog Enhancement**: `CFilterDlg` now accepts a LogFile reference to access process information
- **Dynamic UI**: Process filter page uses `CPropertyListItem` dropdown when processes are available, falls back to text edit otherwise
- **Stable Storage**: Process list strings are stored in member variables to avoid dangling pointers

#### 9.4 User Benefits
- **Eliminate Typos**: No more typing errors when entering process names
- **Discoverability**: See all available processes at a glance
- **Faster Filtering**: Quick selection from dropdown instead of typing
- **Flexibility**: Can still type custom process names for future processes

#### 9.5 Files Modified
- `LogFile.h/cpp`: Added `GetUniqueProcesses()` method to extract unique process list
- `FilterDlg.h/cpp`: Modified to accept LogFile reference and pass to FilterPage
- `FilterPage.h/cpp`: Added dropdown support for process filters with `SetAvailableProcesses()`
- `MainFrame.cpp`: Updated all CFilterDlg instantiations to pass LogFile reference

### 10. Auto-Create Process Views (@@NEWDEBUGVIEW@@)

#### 10.1 Overview
Added automatic view creation and management when processes send a special `@@NEWDEBUGVIEW@@` message. This feature automatically creates dedicated filtered views for each process, eliminating manual setup and improving debugging workflow.

#### 10.2 Features
- **Special Message Trigger**: Processes send `@@NEWDEBUGVIEW@@` to trigger auto-view creation
- **Smart View Management**: 
  - Creates new view if none exists for the process
  - Switches to existing view and clears it if already created
- **Automatic Filtering**: Sets up process include filter automatically
- **Existing Message Population**: New views are populated with all existing messages from that process
- **Seamless Integration**: Works transparently without user intervention

#### 10.3 How It Works
1. Process sends `OutputDebugString("@@NEWDEBUGVIEW@@")`
2. DebugView++ intercepts this special message
3. Checks if a view already exists for that process
4. Either creates new view or switches to existing one
5. Clears the view for a fresh debugging session
6. All subsequent messages from that process appear in its dedicated view

#### 10.4 Technical Implementation
- **Message Interception**: Special message detected in `CMainFrame::AddMessage()`
- **View Discovery**: `FindViewWithProcessFilter()` locates existing process views
- **View Creation**: `CreateProcessView()` sets up new filtered view
- **View Switching**: `SwitchToView()` activates the appropriate tab
- **Message Population**: New views receive all existing messages from their process

#### 10.5 User Benefits
- **Zero Configuration**: No manual filter setup required
- **Automatic Isolation**: Each app gets its own view automatically
- **Clean Restart**: Views clear when app restarts (sends message again)
- **Focus Management**: Automatically switches to relevant view
- **Time Saving**: Eliminates repetitive manual view creation

#### 10.6 Files Modified
- `MainFrame.h/cpp`: Added helper methods and message handling logic
- `DebugViewTestMessages.cpp`: Added test for auto-view feature

### 11. Tab-Specific Detail Pane

#### 11.1 Overview
Enhanced the detail pane to show only content from the currently active tab, preventing confusion from cross-tab message updates. Each tab now maintains its own detail view state independently.

#### 11.2 Problem Solved
Previously, selecting a message in any tab would update the detail pane of the active tab, causing:
- Confusion when background tabs updated the visible detail pane
- Loss of context when switching between tabs
- Difficulty tracking messages across multiple debug sessions

#### 11.3 Features
- **Tab Isolation**: Detail pane only updates from the active tab's selections
- **State Persistence**: Each tab remembers its last selected detail content
- **Automatic Restoration**: Switching tabs restores that tab's detail view
- **Clean Separation**: No cross-contamination between tab contexts
- **Per-View Splitter Position**: Each view maintains its own detail pane height preference

#### 11.4 Technical Implementation
- **View Identification**: LogView passes its pointer when updating details
- **View Matching**: MainFrame checks if calling view matches active tab
- **Content Storage**: Each tab stores its last detail status and content
- **Tab Switch Handling**: OnChangeTab restores detail content for activated tab
- **Splitter Position Persistence**: Each view's detail pane splitter position is saved to registry

#### 11.5 User Benefits
- **Clear Context**: Detail pane always shows content from visible tab
- **Persistent State**: Tab switching preserves detail selections
- **No Confusion**: Background tab activity doesn't affect visible details
- **Better Multi-Session Debugging**: Each debug session maintains its own context
- **Custom Layouts**: Each view can have different detail pane heights based on content needs

#### 11.6 Files Modified
- `MainFrame.h/cpp`: Modified UpdateDetailView to accept view identifier, added splitter position save/load
- `LogView.cpp`: Updated to pass 'this' pointer when updating details
- `CLogViewTabItem2.h/cpp`: Added detail content storage, restoration, and splitter position management
- Tab switching handler updated to restore detail content

## Known Issues Resolved

1. **White text on light backgrounds with Process Colors** - Fixed with contrast calculation
2. **Detail pane not respecting dark mode** - Fixed with RichEdit implementation
3. **Dark mode text invisible on startup** - Fixed with proper theme initialization
4. **Wrong icons on detail pane buttons** - Fixed with correct icon indices
5. **XML syntax highlighting broken with `>` in comments** - Fixed with state machine parser
6. **Log messages incorrectly detected as SQL** - Fixed with stricter SQL detection and proper format priority
7. **Bracketed log levels `[ERROR]` not highlighted** - Added support for both bracket and colon formats
8. **Line numbers invisible in dark mode** - Fixed with theme-aware colors
9. **SQL formatting not using professional formatter** - Fixed method routing
10. **Syntax highlighting completely broken** - Fixed with Lexilla integration
11. **XML incorrectly detected as JSON** - Fixed detection order
12. **Plain text with brackets detected as JSON** - Fixed with stricter pattern matching
13. **Line numbers not showing on startup** - Fixed initialization sequence
14. **Scintilla not in dark mode initially** - Fixed with proper theme application
15. **Expanded Toolbar toggle creating duplicate/missing toolbars** - Fixed with proper toolbar band management

### 12. Per-View Detail Pane Splitter Position Persistence

#### 12.1 Overview
Added persistent storage of the detail pane splitter position for each view, allowing users to customize the detail pane height based on the content type of each view. The splitter position is now saved per-view and restored when the application restarts.

#### 12.2 Problem Solved
Previously, the detail pane splitter position would reset to 75% on every application restart, and all views shared the same temporary splitter position during a session. This was inconvenient for users who wanted different detail pane sizes for different types of content.

#### 12.3 Features
- **Per-View Storage**: Each view maintains its own preferred detail pane height
- **Persistent Settings**: Splitter positions are saved to the registry and restored on startup
- **Smart Updates**: Position is updated when saving settings to capture the latest user preference
- **Default Position**: New views start with a sensible 75% default position

#### 12.4 Technical Implementation
- **Storage**: Splitter position stored as `DetailSplitterPos` DWORD in each view's registry key
- **Getter/Setter Methods**: Added `GetSplitterPos()`, `SetSplitterPos()`, and `UpdateSplitterPos()` to CLogViewTabItem2
- **Save Process**: MainFrame calls `UpdateSplitterPos()` before saving to capture current position
- **Load Process**: Position loaded from registry when views are restored at startup

#### 12.5 User Benefits
- **Customizable Layouts**: Different views can have different detail pane sizes
- **Content-Specific Sizing**: SQL/JSON views can use larger detail panes while simple logs use smaller ones
- **Persistent Preferences**: Layout preferences survive application restarts
- **Seamless Experience**: No need to manually adjust splitter positions after restart

#### 12.6 Files Modified
- `CLogViewTabItem2.h/cpp`: Added splitter position management methods and storage
- `MainFrame.cpp`: Added splitter position save/load logic in LoadSettings and SaveSettings

### 13. Detail Pane Toggle Buttons

#### 13.1 Overview
Replaced the checkbox controls in the detail pane toolbar with toggle buttons that match the visual style of the main toolbar, providing a more consistent and professional user interface.

#### 13.2 Features
- **Toggle Button Style**: Replaced checkboxes (Format, Line Numbers, Syntax Highlight, Word Wrap) with toggle buttons
- **Visual Feedback**: Toggle buttons show light blue highlight (RGB 201, 224, 247) when enabled, matching the main toolbar style
- **Hover Effects**: Buttons show subtle hover and press effects for better user feedback
- **Text-Only Buttons**: Clean text labels without icons (ready for future icon additions)
- **Consistent Appearance**: All toolbar buttons now use the same visual style throughout the application

#### 13.3 Technical Implementation
- **CDetailToolbarToggleButton Class**: New class extending CDetailToolbarButton with toggle state functionality
- **Message Map Override**: Proper WTL message map handling to ensure OnPaint override works correctly
- **State Management**: Toggle state preserved and properly reflected in UI

#### 13.4 Files Modified
- `CLogViewTabItem2.h`: Added CDetailToolbarToggleButton class, replaced checkbox declarations
- `CLogViewTabItem2.cpp`: Updated event handlers to work with toggle buttons

### 14. Per-View Detail Pane Settings

#### 14.1 Overview
Detail pane settings (Word Wrap, Syntax Highlight, Format, Line Numbers) are now saved per-view instead of globally, allowing each tab to maintain its own independent configuration.

#### 14.2 Problem Solved
Previously, detail pane settings were stored globally in MainFrame, causing settings to appear to change when switching between tabs. This was confusing as each tab appeared to share the same settings.

#### 14.3 Features
- **Independent Settings**: Each view/tab maintains its own detail pane configuration
- **Persistent Storage**: Settings saved to registry per-view and restored on application restart
- **Default Values**: New views default to having all four options enabled (Word Wrap, Syntax, Format, Line Numbers)
- **Seamless Experience**: Settings remain consistent when switching between tabs

#### 14.4 Technical Implementation
- **Local Storage**: Settings stored in CDetailView instance instead of MainFrame
- **Registry Keys**: Each view saves its settings to its own registry section:
  - `DetailWordWrap` (default: 1)
  - `DetailSyntaxHighlight` (default: 1)
  - `DetailFormatContent` (default: 1)
  - `DetailLineNumbers` (default: 1)
- **Getter/Setter Methods**: Added methods in CDetailView and CLogViewTabItem2 to access settings
- **Initialization**: Settings loaded from registry when view is created, applied to buttons and editor

#### 14.5 User Benefits
- **Customized Views**: Different views can have different detail pane configurations
- **Context Preservation**: Each debugging session maintains its preferred settings
- **No Confusion**: Settings don't appear to randomly change when switching tabs
- **Smart Defaults**: New views start with all features enabled for best experience

#### 14.6 Files Modified
- `CLogViewTabItem2.h`: Added getter/setter methods for detail settings
- `CLogViewTabItem2.cpp`: Updated initialization and event handlers
- `MainFrame.h/cpp`: Removed global detail settings, added per-view save/load
- All settings now stored locally per-view with proper persistence

### 15. Clarion Debug Format Support

#### 15.1 Overview
Added comprehensive support for Clarion-formatted debug messages with dedicated columns for structured data display. This feature allows Clarion applications to send formatted debug messages that are automatically parsed and displayed in appropriate columns.

#### 15.2 Features Implemented

##### New Columns Added
- **Thread** - Displays thread ID from Clarion messages (T:<threadId>)
- **App** - Shows the application name (A:<appName>)
- **Module** - Shows the module/source file name (M:<moduleName>)
- **Procedure** - Displays the procedure/function name (P:<procedureName>)
- **Prefix** - Shows optional filter prefix (PRE:<prefix>)

##### Clarion Message Format
Messages following this format are automatically parsed:
```
[||]X,T:<threadId>,P:<procedureName>,M:<moduleName>,A:<appName>,PRE:<prefix>,MSG:<message>
```

Example:
```
[||]X,T:1234,P:InitDatabase,M:MainModule.clw,A:MyApp.exe,PRE:DB,MSG:Connecting to database...
```

##### Key Features
- **Automatic Format Detection**: Messages starting with `[||]X,` are automatically detected as Clarion format
- **Smart Parsing**: Key:value pairs are extracted and displayed in appropriate columns
- **Clean Message Display**: Only the actual message text (after MSG:) is shown in the Message column
- **Backward Compatible**: Regular non-Clarion messages continue to work normally with empty Clarion columns
- **Column Visibility Control**: All new columns can be shown/hidden via right-click on column headers
- **Protected Message Column**: Message column cannot be hidden and always remains as the last column
- **Persistent Settings**: Column visibility and widths are saved and restored across sessions

#### 15.3 Technical Implementation

##### Files Modified/Created
- **LogView.h**: Added new column enums (Thread, App, Module, Procedure, Prefix)
- **LogView.cpp**: 
  - Fixed `GetItemData()` to populate all columns using `GetColumnText()`
  - Updated column initialization with proper defaults (hidden by default)
  - Added protection to prevent Message column from being hidden
  - Fixed checkmark display in column context menu
- **LogFile.h**: Added Clarion-specific fields to Message struct
- **LogFile.cpp**: Implemented `ParseClarionFormat()` function for parsing messages
- **resource.h**: Added resource IDs for new columns
- **DebugView++.rc**: Added new columns to header context menu

##### Bug Fixes
- **Column visibility checkmarks**: Fixed issue where checkmarks weren't showing in context menu
- **Owner-draw column display**: Fixed critical bug where new columns weren't being drawn due to `GetItemData()` not populating them
- **Message storage**: Ensured original message format is preserved for parsing on retrieval

#### 15.4 Testing
Created test programs to verify functionality:
- **TestClarionMessages.exe**: Sends various Clarion-formatted test messages
- **TestDebugClarion.exe**: Simple debug test for format verification

#### 15.5 Usage
1. Start UltimateDebugviewpp.exe
2. Right-click on column headers to show Clarion columns (Thread, App, Module, Procedure, Prefix)
3. Run your Clarion application that sends formatted debug messages
4. Messages are automatically parsed and displayed in appropriate columns
5. Use column visibility controls to customize your view

#### 15.6 Benefits
- **Better Organization**: Debug information is automatically organized into logical columns
- **Improved Filtering**: Can filter by thread, module, procedure, etc.
- **Cleaner Display**: Message column shows only the actual debug message, not the format metadata
- **Enhanced Debugging**: Easier to track execution flow across procedures and modules
- **Professional Presentation**: Structured data display similar to modern IDEs

### 16. Column Drag-and-Drop Reordering

#### 16.1 Overview
Implemented drag-and-drop column reordering for the Debug Grid, allowing users to customize column arrangement by dragging column headers with the mouse.

#### 16.2 Features
- **Drag-and-Drop Support**: Click and hold any column header to drag it to a new position
- **Protected Bookmark Column**: First column (bookmark) cannot be moved, maintaining UI consistency
- **Visual Feedback**: Standard Windows drag cursor shows during column dragging
- **Per-View Persistence**: Each tab/view maintains its own column arrangement
- **Automatic Saving**: Column order is saved to registry when application exits
- **Restoration on Startup**: Column order is restored when views are loaded

#### 16.3 Technical Implementation
- **Extended ListView Style**: Explicitly set `LVS_EX_HEADERDRAGDROP` in OnCreate to enable drag-drop
- **Notification Handlers**: 
  - `HDN_BEGINDRAG`: Blocks dragging of column 0, allows others
  - `HDN_ENDDRAG`: Updates internal column order after drag completes
- **Column Order Tracking**: `GetColumnOrderArray`/`SetColumnOrderArray` APIs manage order
- **Registry Persistence**: Column order saved as "Order" DWORD for each column in registry
- **Order Application**: UpdateColumns() correctly applies saved column order when displaying columns

#### 16.4 Files Modified
- `LogView.h`: Added HDN_BEGINDRAG/HDN_ENDDRAG handlers
- `LogView.cpp`: 
  - Added explicit LVS_EX_HEADERDRAGDROP setting
  - Implemented drag notification handlers
  - Updated SaveSettings to capture current column order
  - Fixed UpdateColumns to properly apply saved order

#### 16.5 User Benefits
- **Customizable Layout**: Arrange columns in preferred order for workflow
- **Persistent Preferences**: Column arrangement survives application restarts
- **Per-View Customization**: Different tabs can have different column orders
- **Intuitive Interface**: Standard Windows drag-and-drop behavior
- **No Configuration Required**: Works immediately with existing saved views

### 17. Column Layout Save/Restore System

#### 17.1 Overview
Implemented a comprehensive column layout management system that allows users to save, restore, and manage multiple column configurations. This feature enables quick switching between different debugging scenarios and recovery from column display issues.

#### 17.2 Key Features

##### Save and Restore Layouts
- **Save Current Layout**: Right-click column header → "Save Current Layout..." to save the current column configuration with a custom name
- **Quick Restore**: Saved layouts appear in the column header context menu with keyboard accelerators (1-9, 0)
- **Layout Storage**: Up to 10 layouts can be saved for quick access
- **Overwrite Protection**: Prompts for confirmation when overwriting existing layouts

##### Manage Layouts Dialog
- **Centralized Management**: "Manage Layouts..." option opens a dedicated dialog for layout organization
- **Available Actions**:
  - **Load**: Apply selected layout immediately
  - **Rename**: Change the name of existing layouts
  - **Delete**: Remove unwanted layouts with confirmation
- **Double-Click Loading**: Double-click any layout in the list to load it instantly
- **User-Friendly Interface**: Clean dialog with list view and action buttons

##### Reset Layout
- **Factory Reset**: "Reset Layout" option restores all columns to their default configuration
- **Issue Recovery**: Fixes column ordering problems and restores missing columns
- **Confirmation Dialog**: Prevents accidental resets with confirmation prompt
- **Default Configuration**:
  - Visible: Bookmark, Line, Time, PID, Process, Message
  - Hidden: Date, Thread, App, Module, Procedure, Prefix (Clarion columns)

#### 17.3 Technical Implementation

##### Architecture
- **Registry Storage**: Layouts stored in `HKEY_CURRENT_USER\Software\CobaltFusion\DebugView++\ColumnLayouts`
- **Per-Layout Data**: Each layout saves column visibility, width, and order for all columns
- **Dynamic Menu Population**: Saved layouts automatically appear in context menu
- **Column Order Fix**: Improved `UpdateColumns()` function to properly handle column ordering

##### Files Created/Modified
- **New Files**:
  - `ManageLayoutsDlg.h/cpp`: Dialog for managing saved layouts
- **Modified Files**:
  - `LogView.h/cpp`: Added layout management methods and improved column handling
  - `resource.h`: Added resource IDs for new menu items and dialog
  - `DebugView++.rc`: Added menu items and dialog resource
  - `CMakeLists.txt`: Added ManageLayoutsDlg.cpp to build

##### Key Methods
- `SaveColumnLayout()`: Captures current column state and saves to registry
- `LoadColumnLayout()`: Restores column configuration from saved layout
- `DeleteColumnLayout()`: Removes layout from registry
- `GetSavedLayouts()`: Returns list of available layouts
- `PopulateLayoutMenu()`: Adds saved layouts to context menu
- `OnColumnResetLayout()`: Restores factory default column configuration

#### 17.4 Bug Fixes
- **Column Order Corruption**: Fixed issue where bookmark column could appear in wrong position
- **Missing Columns**: Reset Layout now properly restores all missing columns
- **Column Toggle Issues**: Fixed problem where enabling/disabling columns would corrupt the display
- **UpdateColumns Logic**: Simplified and fixed the column ordering algorithm

#### 17.5 User Workflow

##### Saving a Layout
1. Arrange columns as desired (show/hide, reorder, resize)
2. Right-click any column header
3. Select "Save Current Layout..."
4. Enter a descriptive name
5. Layout is saved and immediately available in menu

##### Restoring a Layout
1. Right-click any column header
2. Select the desired layout from the menu (or use keyboard shortcut 1-9, 0)
3. Columns instantly rearrange to saved configuration

##### Managing Layouts
1. Right-click column header → "Manage Layouts..."
2. Select layout from list
3. Use buttons to Load, Rename, or Delete
4. Double-click to quickly load a layout

#### 17.6 User Benefits
- **Quick Configuration Switching**: Switch between different debugging scenarios instantly
- **Team Collaboration**: Share column configurations with team members
- **Error Recovery**: Easily fix column display issues with Reset Layout
- **Workflow Optimization**: Save layouts for specific debugging tasks
- **Persistent Preferences**: All layouts survive application restarts
- **No Lost Columns**: Reset Layout ensures all columns are accessible

## Future Enhancements

Potential improvements for future versions:
1. Custom theme colors (user-defined themes)
2. Per-process custom colors
3. ~~Syntax highlighting in detail pane for known formats (JSON, XML, etc.)~~ ✅ COMPLETED with Scintilla
4. Export themes as shareable files
5. More toolbar customization options
6. Additional syntax highlighting formats:
   - YAML configuration files
   - INI/Properties files
   - Python tracebacks
   - Regular expression patterns
   - HTTP request/response headers
   - C/C++ code
   - JavaScript/TypeScript
   - PowerShell scripts
7. Customizable syntax highlighting colors
8. Syntax highlighting for the main list view (not just detail pane)
9. Export highlighted text as HTML/RTF
10. Smart format detection improvements (e.g., detect partial JSON in larger text)
11. Code folding support in detail pane
12. Search and replace in detail pane
13. Regular expression search in detail pane
14. ~~Clarion debug format support~~ ✅ COMPLETED
15. Support for additional structured debug formats (similar to Clarion)
16. Column profiles for quick switching between different debug scenarios
17. Export column data to CSV/Excel format