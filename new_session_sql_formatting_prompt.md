# Task: Implement Advanced SQL Formatting Features

## Context
You are working on the UltimateDebugViewPP project located at `C:\Dev\UltimateDebugViewPP`. The project recently had a comma alignment feature added for SQL formatting (Inline, Trailing, Leading options). Now you need to implement additional advanced SQL formatting features.

## Background
- The project uses fsqlf.dll for SQL formatting
- The SQL formatter preferences dialog was recently updated
- The fsqlf.dll library supports keyword-specific spacing configuration via `fsqlf_set_keyword_spacing()`
- The complete implementation plan is in: `C:\Dev\UltimateDebugViewPP\sql_advanced_formatting_plan.md`

## Your Task
Implement the four advanced SQL formatting features described in the plan:

1. **Format Style** (Compact/Normal/Expanded) - Control overall SQL density
2. **Parentheses Handling** - Control subquery parentheses formatting
3. **Function Formatting** - Control how SQL functions are formatted
4. **Alias Alignment** - Vertically align column aliases

## Implementation Instructions

### Phase 1: Clean Up
First, remove the non-functional "Preserve Comments" feature:
- Remove the checkbox from the UI
- Remove `preserveComments` field from structs
- Clean up all code references

### Phase 2: Implement Features
Follow the order in the plan (easiest to hardest):
1. Format Style (Compact/Normal/Expanded)
2. Parentheses Handling for Subqueries
3. Function Formatting
4. Alias Alignment

### Important Guidelines

#### Use Subagents
**USE GENERAL-PURPOSE AGENTS** for:
- Searching for all occurrences of `preserveComments` across the codebase
- Understanding how the existing radio button groups are implemented
- Finding all places where fsqlf spacing is configured
- Analyzing the current UI layout in the .rc file
- Performing complex multi-file modifications

#### Build and Test
- Build command: `"C:\Program Files\CMake\bin\cmake.exe" --build build --config Debug --target UltimateDebugviewpp`
- Test each feature thoroughly in the Preview pane
- Ensure settings persist after closing and reopening preferences

#### Code Patterns to Follow
- Use the existing Keyword Case implementation as a pattern for new radio button groups
- Follow the `_NEW` suffix convention for control IDs
- Use the existing message handler pattern (`OnFormatOptionChange`)
- Ensure all enums are properly defined as `enum class`

#### Key Files to Modify
1. `application\include\DebugViewppLib\SqlFormatter.h` - Add enums and options
2. `application\DebugViewpp\resource.h` - Add control IDs
3. `application\DebugViewpp\DebugView++.rc` - Add UI elements
4. `application\DebugViewpp\PreferencesDlg.h/cpp` - Handle controls
5. `application\DebugViewppLib\SqlFormatter.cpp` - Implement logic
6. `application\DebugViewpp\MainFrame.cpp` - Registry persistence

## fsqlf Keyword Reference
The fsqlf.dll recognizes these keyword contexts for spacing control:
- Parentheses: `kw_left_p`, `kw_right_p`, `kw_left_p_sub`, `kw_right_p_sub`
- Commas: `kw_comma`, `kw_comma_ins`, `kw_comma_grpby`, `kw_comma_ordby`
- Major clauses: `kw_select`, `kw_from`, `kw_where`, `kw_groupby`, `kw_orderby`
- Joins: `kw_inner_join`, `kw_left_join`, `kw_right_join`

The spacing parameters are: `(before_newlines, before_indent, after_newlines, after_indent)`

## Testing SQL
Use these SQL statements to test your implementation:
```sql
-- Complex query with subqueries and functions
SELECT customer_id, first_name, last_name,
       (SELECT COUNT(*) FROM orders WHERE orders.customer_id = c.customer_id) as order_count,
       MAX(order_date) as latest,
       COALESCE(email, phone, 'No Contact') as contact
FROM customers c
WHERE country = 'USA'
  AND state IN (SELECT DISTINCT state FROM regions)
GROUP BY customer_id, first_name, last_name
ORDER BY last_name, first_name;
```

## Success Criteria
- All four features are working and independently toggleable
- Settings persist in the Windows registry
- Preview pane updates in real-time
- Code follows existing patterns and conventions
- Build completes without errors
- Features work as described in the plan

Start by reading the full implementation plan in `sql_advanced_formatting_plan.md`, then begin with Phase 1 (removing Preserve Comments).

Remember to use agents for complex searches and multi-file analysis!