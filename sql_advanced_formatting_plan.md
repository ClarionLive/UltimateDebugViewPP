# SQL Advanced Formatting Features Implementation Plan

## Overview
This document outlines the implementation of advanced SQL formatting options for UltimateDebugViewPP, leveraging the capabilities of fsqlf.dll to provide users with fine-grained control over SQL formatting.

## Current State
- The comma alignment feature has been successfully implemented with three options (Inline, Trailing, Leading)
- The "Preserve Comments" checkbox exists but is non-functional with fsqlf.dll
- fsqlf.dll supports keyword-specific spacing control through `fsqlf_set_keyword_spacing()`

## fsqlf Capabilities Discovered

### Keyword-Specific Spacing Control
Each SQL keyword/token can have customized spacing with these parameters:
- `global_indent_change`: Change indentation level
- `new_line`: Number of newlines before/after
- `indent`: Indentation levels before/after
- `space`: Spaces before/after

### Context-Aware Token Handling
fsqlf distinguishes between different uses of the same character:

**Parentheses contexts:**
- `kw_left_p` / `kw_right_p` - General parentheses
- `kw_left_p_sub` / `kw_right_p_sub` - Subquery parentheses
- `kw_left_p_create` / `kw_right_p_create` - CREATE statement parentheses
- `kw_left_p_ins` / `kw_right_p_ins` - INSERT statement parentheses

**Comma contexts:**
- `kw_comma` - General commas in SELECT lists
- `kw_comma_ins` - Commas in INSERT statements
- `kw_comma_grpby` - Commas in GROUP BY
- `kw_comma_ordby` - Commas in ORDER BY
- `kw_comma_join` - Commas in JOIN conditions
- `kw_comma_set` - Commas in UPDATE SET
- `kw_comma_create` - Commas in CREATE TABLE

## Features to Implement

### 1. Compact vs Expanded Formatting
**Purpose:** Allow users to control overall SQL density

**Options:**
- **Compact**: Minimize newlines for dense output
- **Normal**: Current default formatting
- **Expanded**: Extra newlines between major clauses for readability

**Implementation:**
- Add `SqlFormatStyle` enum with three values
- Control newline settings for major keywords (SELECT, FROM, WHERE, etc.)
- Compact: 0 newlines before major keywords
- Normal: 1 newline before major keywords
- Expanded: 2 newlines before major keywords

### 2. Parentheses Handling for Subqueries
**Purpose:** Control how subqueries are formatted for better readability

**Options:**
- **Same line**: No newlines around parentheses
- **New line**: Parentheses on separate lines
- **Indent content**: Additional indentation inside parentheses

**Implementation:**
- Add `SqlParenthesesStyle` enum
- Control `kw_left_p_sub` and `kw_right_p_sub` spacing:
  - Same line: (0,0,0,0) for both
  - New line: (1,0,0,0) before left, (0,0,1,0) after right
  - Indent content: (1,1,0,0) before left, (-1,0,1,0) after right

### 3. Function Formatting
**Purpose:** Control how SQL functions are formatted

**Options:**
- **Compact**: `COUNT(*)` - no spaces
- **Expanded**: `COUNT( * )` - with spaces
- **Multi-line arguments**: For functions with multiple arguments, each on a new line

**Implementation:**
- Add `SqlFunctionStyle` enum
- Control `kw_left_p` and `kw_right_p` when preceded by function names
- For multi-line: control `kw_comma` within function parentheses

### 4. Alias Alignment
**Purpose:** Vertically align column aliases for better readability

**Options:**
- Checkbox: "Align column aliases"

**Implementation:**
- When enabled, dynamically adjust spacing before `kw_as` keyword
- Calculate maximum column name length in SELECT list
- Add appropriate spacing to align all AS keywords vertically

## Implementation Steps

### Phase 1: Remove Non-Functional Features
1. Remove "Preserve Comments" checkbox (IDC_SQL_PRESERVE_COMMENTS_NEW)
2. Remove `preserveComments` from `SqlFormatOptions` struct
3. Remove `sqlPreserveComments` from `NewViewDefaults` struct
4. Clean up all related code references

### Phase 2: Add Format Style (Easiest)
1. Update `SqlFormatter.h`:
   ```cpp
   enum class SqlFormatStyle {
       Compact,
       Normal,
       Expanded
   };
   ```
2. Add to `SqlFormatOptions` struct
3. Add UI group box with 3 radio buttons
4. Implement spacing logic in `SqlFormatter.cpp`

### Phase 3: Add Parentheses Handling
1. Add `SqlParenthesesStyle` enum
2. Add UI controls (radio buttons or checkboxes)
3. Implement logic for `kw_left_p_sub` and `kw_right_p_sub`

### Phase 4: Add Function Formatting
1. Add `SqlFunctionStyle` enum
2. Add UI controls
3. Implement function detection logic
4. Apply appropriate spacing to function parentheses

### Phase 5: Add Alias Alignment (Most Complex)
1. Add checkbox for alias alignment
2. Implement pre-processing to calculate column widths
3. Dynamically adjust `kw_as` spacing

## Files to Modify

### Core Files
- `application\include\DebugViewppLib\SqlFormatter.h` - Add new enums and options
- `application\DebugViewppLib\SqlFormatter.cpp` - Implement formatting logic
- `application\DebugViewpp\resource.h` - Add new control IDs
- `application\DebugViewpp\DebugView++.rc` - Add UI elements

### Dialog Files
- `application\DebugViewpp\PreferencesDlg.h` - Add message handlers and struct fields
- `application\DebugViewpp\PreferencesDlg.cpp` - Handle new controls

### Settings Persistence
- `application\DebugViewpp\MainFrame.cpp` - Registry save/load for new settings

## UI Layout Suggestion

```
SQL Options Tab:
┌─────────────────────────────────────────────────────────┐
│ [Keyword Case]  [Indentation]  [Comma Alignment]        │
│                                                          │
│ ┌─Format Style──────┐ ┌─Subquery Parentheses─────────┐ │
│ │ ○ Compact         │ │ ○ Same line                  │ │
│ │ ● Normal          │ │ ○ New line                   │ │
│ │ ○ Expanded        │ │ ○ Indent content             │ │
│ └───────────────────┘ └──────────────────────────────┘ │
│                                                          │
│ ┌─Function Format──┐  ┌─Additional Options──────────┐  │
│ │ ○ Compact        │  │ ☐ Align column aliases      │  │
│ │ ● Expanded       │  │ ☐ New line before keywords  │  │
│ │ ○ Multi-line     │  └──────────────────────────────┘ │
│ └──────────────────┘                                    │
│                                                          │
│ [Preview]                                                │
│ ┌────────────────────────────────────────────────────┐  │
│ │                                                    │  │
│ └────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

## Testing Requirements

### Test Cases
1. **Format Style Testing:**
   - Verify compact removes unnecessary newlines
   - Verify expanded adds extra spacing
   - Ensure settings persist across sessions

2. **Parentheses Testing:**
   - Test with nested subqueries
   - Verify indentation changes work correctly
   - Test with different parentheses contexts

3. **Function Testing:**
   - Test with single-argument functions (COUNT(*))
   - Test with multi-argument functions (SUBSTRING, COALESCE)
   - Verify spacing is consistent

4. **Alias Alignment Testing:**
   - Test with varying column name lengths
   - Verify alignment works with complex expressions
   - Test performance with large SELECT lists

### SQL Test Statements
```sql
-- Test for Format Style and Parentheses
SELECT customer_id, first_name, last_name,
       (SELECT COUNT(*) FROM orders WHERE orders.customer_id = c.customer_id) as order_count
FROM customers c
WHERE country = 'USA'
  AND state IN (SELECT DISTINCT state FROM regions WHERE active = 1)
ORDER BY last_name, first_name;

-- Test for Function Formatting
SELECT COUNT(*) as total,
       MAX(order_date) as latest_order,
       SUBSTRING(description, 1, 50) as short_desc,
       COALESCE(email, phone, 'No Contact') as contact_info
FROM orders;

-- Test for Alias Alignment
SELECT c.customer_id AS id,
       c.first_name AS fname,
       c.last_name AS lname,
       c.email_address AS email,
       COUNT(o.order_id) AS total_orders
FROM customers c
LEFT JOIN orders o ON c.customer_id = o.customer_id
GROUP BY c.customer_id, c.first_name, c.last_name, c.email_address;
```

## Notes
- All features should be independently toggleable
- Settings must persist in Windows registry
- Preview pane should update in real-time as options change
- Build with Debug configuration for testing: `cmake --build build --config Debug --target UltimateDebugviewpp`