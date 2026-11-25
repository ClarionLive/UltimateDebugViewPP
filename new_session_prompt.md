# Task: Add Comma Alignment Feature to SQL Formatter

## Context
Please implement the comma alignment feature for the SQL formatter in UltimateDebugViewPP. The complete specification is in the file `C:\Dev\UltimateDebugViewPP\comma_alignment_feature.md`.

## Background
- The project is located at `C:\Dev\UltimateDebugViewPP`
- The SQL formatter preferences dialog was recently updated to use fsqlf.dll for preview formatting
- The spin control for indentation was just added and is working correctly
- The fsqlf.dll library supports leading commas via the `kw_comma` spacing configuration

## Task Overview
Implement the comma alignment feature as specified in the markdown file. This involves:
1. Adding a new "Comma Alignment" group box with three radio buttons to the SQL Options tab
2. Updating the data structures to use an enum instead of a boolean
3. Modifying the SQL formatter to handle the three alignment modes
4. Updating the preferences save/load functionality

## Important Instructions

### 1. Use Subagents Where Appropriate
**Use general-purpose agents** for:
- Searching for all occurrences of `alignCommas` across the codebase
- Understanding how the Keyword Case radio buttons are implemented
- Finding all places where comma formatting is currently handled
- Performing complex multi-file modifications

### 2. Build and Test Commands
- Main app: `"C:\Program Files\CMake\bin\cmake.exe" --build build --config Debug --target UltimateDebugviewpp`
- fsqlf.dll if needed: `"C:\Program Files\CMake\bin\cmake.exe" --build build --config Debug --target fsqlf`

### 3. Maintain Consistency
- The new Comma Alignment group should look and behave like the Keyword Case group
- Use the `_NEW` suffix for control IDs as per existing convention
- Follow the existing message handler patterns in PreferencesDlg

### 4. Testing Requirements
- Verify the preview updates correctly for each option
- Ensure settings persist after closing and reopening preferences
- Check that the Details pane uses the correct formatting
- Test with SELECT, INSERT, UPDATE, ORDER BY, and GROUP BY statements

### 5. Key Technical Details
The fsqlf_set_keyword_spacing parameters are: (keyword, before_newlines, before_indent, after_newlines, after_indent)
- **Leading commas**: `(1, 1, 0, 0)` - newline and indent before comma
- **Trailing commas**: `(0, 0, 1, 1)` - newline and indent after comma
- **Inline**: `(0, 0, 0, 0)` - no newlines

## Implementation Steps
1. **Start by reading** `comma_alignment_feature.md` to understand the full specification
2. **Use an agent** to find all current uses of `alignCommas` boolean
3. **Use an agent** to study how Keyword Case radio buttons work
4. **Implement** the changes as specified in the markdown
5. **Build and test** thoroughly

## Files to Modify
- `application\include\DebugViewppLib\SqlFormatter.h`
- `application\DebugViewpp\DebugView++.rc`
- `application\DebugViewpp\resource.h`
- `application\DebugViewpp\PreferencesDlg.h`
- `application\DebugViewpp\PreferencesDlg.cpp`
- `application\DebugViewppLib\SqlFormatter.cpp`
- `application\DebugViewpp\MainFrame.cpp`

Start by reading the comma_alignment_feature.md file to understand the full specification, then proceed with the implementation using agents where appropriate.