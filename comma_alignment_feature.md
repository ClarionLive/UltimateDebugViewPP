# Add Comma Alignment Feature to SQL Formatter

## Overview
Add a "Comma Alignment" feature to the SQL Options preferences dialog that allows users to choose between inline commas, trailing commas (end of line), or leading commas (beginning of line) when formatting SQL.

## Current State
- The application currently has a boolean "Align commas" checkbox that only supports trailing commas or inline
- The fsqlf.dll library DOES support leading commas via the `kw_comma` spacing configuration
- The preferences dialog has successfully been updated to use fsqlf.dll for preview formatting

## Implementation Plan

### 1. Update Data Structures

#### File: `application\include\DebugViewppLib\SqlFormatter.h`
- Add new enum:
```cpp
enum class SqlCommaAlignment {
    Inline,     // No line breaks, commas inline
    Trailing,   // Commas at end of line (default)
    Leading     // Commas at beginning of line
};
```
- In `SqlFormatOptions` struct, replace:
  - `bool alignCommas` with `SqlCommaAlignment commaAlignment = SqlCommaAlignment::Trailing`

### 2. Update Dialog Resource

#### File: `application\DebugViewpp\DebugView++.rc`
- Add a new group box "Comma Alignment" positioned appropriately (perhaps at position 330,7,150,80)
- Add three radio buttons inside the group box:
  - "Inline" (IDC_SQL_COMMA_INLINE_NEW)
  - "Trailing (end of line)" (IDC_SQL_COMMA_TRAILING_NEW)
  - "Leading (start of line)" (IDC_SQL_COMMA_LEADING_NEW)
- Remove the old "Align commas" checkbox (IDC_SQL_ALIGN_COMMAS_NEW) from Formatting Options
- Adjust the layout as needed to accommodate the new group box

### 3. Update Resource IDs

#### File: `application\DebugViewpp\resource.h`
- Add new control IDs:
```cpp
#define IDC_SQL_COMMA_INLINE_NEW    831
#define IDC_SQL_COMMA_TRAILING_NEW  832
#define IDC_SQL_COMMA_LEADING_NEW   833
```
- Remove or comment out `IDC_SQL_ALIGN_COMMAS_NEW`

### 4. Update Preferences Dialog Handler

#### File: `application\DebugViewpp\PreferencesDlg.h`
- In `CSqlOptionsPage` class, add message handlers for the new radio buttons:
```cpp
COMMAND_ID_HANDLER_EX(IDC_SQL_COMMA_INLINE_NEW, OnFormatOptionChange)
COMMAND_ID_HANDLER_EX(IDC_SQL_COMMA_TRAILING_NEW, OnFormatOptionChange)
COMMAND_ID_HANDLER_EX(IDC_SQL_COMMA_LEADING_NEW, OnFormatOptionChange)
```
- Remove the handler for IDC_SQL_ALIGN_COMMAS_NEW

#### File: `application\DebugViewpp\PreferencesDlg.cpp`
- In `LoadFromDefaults()`, set the appropriate radio button based on `commaAlignment`:
```cpp
switch (defaults.sqlCommaAlignment)
{
case SqlCommaAlignment::Inline:
    CheckDlgButton(IDC_SQL_COMMA_INLINE_NEW, BST_CHECKED);
    break;
case SqlCommaAlignment::Trailing:
    CheckDlgButton(IDC_SQL_COMMA_TRAILING_NEW, BST_CHECKED);
    break;
case SqlCommaAlignment::Leading:
    CheckDlgButton(IDC_SQL_COMMA_LEADING_NEW, BST_CHECKED);
    break;
}
```

- In `SaveToDefaults()`, read the radio button states:
```cpp
if (IsDlgButtonChecked(IDC_SQL_COMMA_INLINE_NEW))
    defaults.sqlCommaAlignment = SqlCommaAlignment::Inline;
else if (IsDlgButtonChecked(IDC_SQL_COMMA_TRAILING_NEW))
    defaults.sqlCommaAlignment = SqlCommaAlignment::Trailing;
else if (IsDlgButtonChecked(IDC_SQL_COMMA_LEADING_NEW))
    defaults.sqlCommaAlignment = SqlCommaAlignment::Leading;
```

- In `UpdatePreview()`, update the options setting:
```cpp
// Get comma alignment
if (IsDlgButtonChecked(IDC_SQL_COMMA_INLINE_NEW))
    options.commaAlignment = SqlCommaAlignment::Inline;
else if (IsDlgButtonChecked(IDC_SQL_COMMA_TRAILING_NEW))
    options.commaAlignment = SqlCommaAlignment::Trailing;
else if (IsDlgButtonChecked(IDC_SQL_COMMA_LEADING_NEW))
    options.commaAlignment = SqlCommaAlignment::Leading;
```

### 5. Update SQL Formatter

#### File: `application\DebugViewppLib\SqlFormatter.cpp`
- Replace the comma alignment section (around line 281-298) with:
```cpp
// Configure comma alignment
switch (options.commaAlignment) {
case SqlCommaAlignment::Leading:
    LogDebug(L"Setting comma alignment to leading (start of line)");
    // Newline and indent BEFORE comma
    s_fsqlf_set_keyword_spacing("kw_comma", 1, 1, 0, 0);
    s_fsqlf_set_keyword_spacing("kw_comma_ins", 1, 1, 0, 0);
    s_fsqlf_set_keyword_spacing("kw_comma_set", 1, 1, 0, 0);
    s_fsqlf_set_keyword_spacing("kw_comma_grpby", 1, 1, 0, 0);
    s_fsqlf_set_keyword_spacing("kw_comma_ordby", 1, 1, 0, 0);
    break;

case SqlCommaAlignment::Trailing:
    LogDebug(L"Setting comma alignment to trailing (end of line)");
    // Newline and indent AFTER comma
    s_fsqlf_set_keyword_spacing("kw_comma", 0, 0, 1, 1);
    s_fsqlf_set_keyword_spacing("kw_comma_ins", 0, 0, 1, 1);
    s_fsqlf_set_keyword_spacing("kw_comma_set", 0, 0, 1, 1);
    s_fsqlf_set_keyword_spacing("kw_comma_grpby", 0, 0, 1, 1);
    s_fsqlf_set_keyword_spacing("kw_comma_ordby", 0, 0, 1, 1);
    break;

case SqlCommaAlignment::Inline:
default:
    LogDebug(L"Setting comma alignment to inline (no newlines)");
    // No newlines, just inline commas
    s_fsqlf_set_keyword_spacing("kw_comma", 0, 0, 0, 0);
    s_fsqlf_set_keyword_spacing("kw_comma_ins", 0, 0, 0, 0);
    s_fsqlf_set_keyword_spacing("kw_comma_set", 0, 0, 0, 0);
    s_fsqlf_set_keyword_spacing("kw_comma_grpby", 0, 0, 0, 0);
    s_fsqlf_set_keyword_spacing("kw_comma_ordby", 0, 0, 0, 0);
    break;
}
```

### 6. Update View Defaults

#### File: `application\DebugViewpp\PreferencesDlg.h`
- In `NewViewDefaults` struct, replace:
  - `bool sqlAlignCommas` with `SqlCommaAlignment sqlCommaAlignment = SqlCommaAlignment::Trailing`

#### File: `application\DebugViewpp\MainFrame.cpp`
- Update `LoadDefaults()` to load the comma alignment from registry:
```cpp
DWORD sqlCommaAlignment = 1; // Default to Trailing
regDefaults.QueryDWORDValue(L"SqlCommaAlignment", sqlCommaAlignment);
m_newViewDefaults.sqlCommaAlignment = static_cast<SqlCommaAlignment>(sqlCommaAlignment);
```

- Update `SaveDefaults()` to save the comma alignment:
```cpp
regDefaults.SetDWORDValue(L"SqlCommaAlignment", static_cast<DWORD>(m_newViewDefaults.sqlCommaAlignment));
```

- Remove the old `SqlAlignCommas` registry entries

### 7. Update Tab Item Usage
- Check `CLogViewTabItem2.cpp` to ensure it uses the updated `SqlFormatOptions` correctly when calling the formatter

## Testing Plan
1. Open preferences dialog and verify new Comma Alignment group box appears
2. Test each radio button option and verify the preview updates correctly:
   - Inline: all on one line or with minimal breaks
   - Trailing: commas at end of lines with next item on new line
   - Leading: commas at beginning of lines before each item
3. Apply settings and verify the Details pane formats SQL correctly
4. Save preferences and verify they persist after restart
5. Test with various SQL statements to ensure all comma types are handled (SELECT, INSERT, ORDER BY, GROUP BY)

## Key Files Modified
- `application\include\DebugViewppLib\SqlFormatter.h`
- `application\DebugViewpp\DebugView++.rc`
- `application\DebugViewpp\resource.h`
- `application\DebugViewpp\PreferencesDlg.h`
- `application\DebugViewpp\PreferencesDlg.cpp`
- `application\DebugViewppLib\SqlFormatter.cpp`
- `application\DebugViewpp\MainFrame.cpp`

## Notes
- The fsqlf.dll already supports this feature via the `fsqlf_set_keyword_spacing` function
- The parameters are: (keyword, before_newlines, before_indent, after_newlines, after_indent)
- The implementation follows the same pattern as the existing "Keyword Case" group box for consistency