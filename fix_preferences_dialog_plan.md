# Fix Preferences Dialog - Implementation Plan

## Problem Summary
The Preferences dialog in UltimateDebugViewPP is failing with a debug assertion at line 965 in atlwin.h. The dialog tabs show but contain no controls. This broke after attempting to modify SQL preferences.

## Root Cause
The tab pages (CNewViewDefaultsPage, CPrefixDefaultsPage, CSqlOptionsPage) inherit from `CWindowImpl` and attempt to create windows programmatically. This approach is failing because:
1. CWindowImpl::Create expects a registered window class but doesn't have one
2. The window handle (m_hWnd) assertion fails during creation
3. Controls created programmatically in OnCreate() never get called

## Solution: Convert to Dialog-Based Tab Pages

### Why This Solution
- FilterDlg uses this pattern successfully with CFilterPage inheriting from CDialogImpl
- Dialog resources are more maintainable than programmatic control creation
- This follows established ATL/WTL patterns
- Will completely eliminate the assertion failure

### Implementation Steps

#### Step 1: Create Dialog Resources
Add three new dialog resources to DebugView++.rc:

1. **IDD_PREFERENCES_NEWVIEW** - New View Defaults tab
   - Checkbox: "Show detail pane by default" (IDC_SHOW_DETAIL_PANE)
   - Edit: "Default splitter position (%)" (IDC_SPLITTER_POS)
   - Checkbox: "Word wrap" (IDC_DETAIL_WORDWRAP)
   - Checkbox: "Syntax highlighting" (IDC_DETAIL_SYNTAX_HIGHLIGHT)
   - Checkbox: "Format content" (IDC_DETAIL_FORMAT_CONTENT)
   - Checkbox: "Show line numbers" (IDC_DETAIL_LINE_NUMBERS)
   - Button: "Font..." (IDC_DETAIL_FONT_BTN)
   - Static: Font preview (IDC_DETAIL_FONT_PREVIEW)
   - Radio buttons: Theme selection (IDC_THEME_SYSTEM, IDC_THEME_LIGHT, IDC_THEME_DARK)

2. **IDD_PREFERENCES_PREFIX** - Prefix Defaults tab
   - Static: "System Prefix Filters"
   - Grid control: System filters (IDC_SYSTEM_PREFIX_GRID)
   - Static: "Custom Prefix Filters"
   - Grid control: Custom filters (IDC_CUSTOM_PREFIX_GRID)
   - Button: "Reset to Defaults" (IDC_RESET_PREFIX_DEFAULTS)

3. **IDD_PREFERENCES_SQL** - SQL Options tab
   - Group: "Keyword Case"
   - Radio: "UPPERCASE" (IDC_SQL_CASE_UPPER)
   - Radio: "lowercase" (IDC_SQL_CASE_LOWER)
   - Radio: "Capitalize" (IDC_SQL_CASE_CAPITALIZE)
   - Group: "Indentation"
   - Radio: "Spaces" (IDC_SQL_INDENT_SPACES)
   - Radio: "Tab" (IDC_SQL_INDENT_TAB)
   - Edit: "Number of spaces" (IDC_SQL_INDENT_SPACES_EDIT)
   - Checkbox: "Align commas" (IDC_SQL_ALIGN_COMMAS)
   - Checkbox: "New line before keywords" (IDC_SQL_NEWLINE_KEYWORDS)
   - Checkbox: "Preserve comments" (IDC_SQL_PRESERVE_COMMENTS)
   - Edit: Preview area (IDC_SQL_PREVIEW)

#### Step 2: Update resource.h
Add resource IDs for the new dialogs and all controls listed above.

#### Step 3: Convert Classes to CDialogImpl

In PreferencesDlg.h, change:
```cpp
class CNewViewDefaultsPage : public CWindowImpl<CNewViewDefaultsPage>
```
to:
```cpp
class CNewViewDefaultsPage : public CDialogImpl<CNewViewDefaultsPage>
```

Add `enum { IDD = IDD_PREFERENCES_NEWVIEW };` to each class.

Do the same for CPrefixDefaultsPage and CSqlOptionsPage.

#### Step 4: Update Implementation

1. Remove OnCreate() methods from all three page classes
2. Add OnInitDialog() methods instead
3. Move control initialization from programmatic creation to GetDlgItem() calls
4. Update LoadFromDefaults() and SaveToDefaults() to work with dialog controls

#### Step 5: Update CreateTabPages()

Change from:
```cpp
m_newViewDefaultsPage.Create(m_hWnd, pageRect, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0);
```
to:
```cpp
m_newViewDefaultsPage.Create(m_hWnd);
m_newViewDefaultsPage.MoveWindow(&displayRect);
m_newViewDefaultsPage.ShowWindow(SW_SHOW);
```

#### Step 6: Test Each Tab
- Verify New View Defaults loads and saves settings
- Verify Prefix Defaults shows grids and allows editing
- Verify SQL Options updates preview and saves preferences

### Files to Modify
1. `application\DebugViewpp\DebugView++.rc` - Add dialog resources
2. `application\DebugViewpp\resource.h` - Add resource IDs
3. `application\DebugViewpp\PreferencesDlg.h` - Update class inheritance
4. `application\DebugViewpp\PreferencesDlg.cpp` - Update implementation

### Testing Plan
1. Build in Debug mode
2. Open Preferences dialog - should open without assertion
3. Check each tab displays controls properly
4. Test saving and loading preferences
5. Verify SQL formatting preview works
6. Test font selection dialog
7. Test prefix filter grids

## Risks and Mitigations
- **Risk**: Control IDs might conflict with existing ones
  - **Mitigation**: Check resource.h for unused ID ranges
- **Risk**: Dialog layout might not match programmatic layout exactly
  - **Mitigation**: Use dialog editor to match original layout
- **Risk**: Some dynamic behavior might be lost
  - **Mitigation**: Implement in OnInitDialog() and message handlers

## Success Criteria
- No assertion failures when opening Preferences dialog
- All three tabs display their controls correctly
- All settings can be modified and saved
- SQL preview updates in real-time
- Font selection works
- Filter grids are editable