# SQL Formatting Options Fix for UltimateDebugView++

## Problem Summary
The SQL formatting preferences in UltimateDebugView++ are not being applied when fsqlf.dll is available. Users can set preferences (e.g., lowercase keywords) in the Preferences dialog, but the SQL is always displayed with uppercase keywords.

## Root Cause
The fsqlf.dll wrapper (`fsqlf_temp\dll_wrapper.c`) hardcodes formatting options:
- Line 28: `fsqlf_kwmap_set_case(g_kwmap, FSQLF_KWCASE_UPPER);` - forces uppercase keywords
- Line 29: `fsqlf_kwmap_set_major_clause_nl(g_kwmap, 1);` - hardcoded newline settings
- No configuration functions are exported from the DLL to change these settings dynamically

## Solution: Export Configuration Functions from fsqlf.dll

### Files to Modify

1. **C:\Dev\UltimateDebugViewPP\fsqlf_temp\dll_wrapper.c**
2. **C:\Dev\UltimateDebugViewPP\application\include\DebugViewppLib\fsqlf_dll.h**
3. **C:\Dev\UltimateDebugViewPP\application\DebugViewppLib\SqlFormatter.cpp**

### Implementation Steps

#### Step 1: Add Configuration Functions to dll_wrapper.c

Add these functions after the existing exports (around line 130):

```c
// Export function to set keyword case (Upper/Lower/Capitalize)
EXPORT void fsqlf_set_keyword_case(int case_option) {
    if (g_initialized && g_kwmap) {
        fsqlf_kwmap_set_case(g_kwmap, (enum fsqlf_kwcase)case_option);
    }
}

// Export function to set indent/spacing for major clauses
EXPORT void fsqlf_set_major_clause_newlines(int nl_count) {
    if (g_initialized && g_kwmap) {
        fsqlf_kwmap_set_major_clause_nl(g_kwmap, nl_count);
    }
}

// Export function to configure individual keyword spacing
EXPORT void fsqlf_set_keyword_spacing(const char* keyword,
                                      int before_newlines, int before_indent,
                                      int after_newlines, int after_indent) {
    if (g_initialized && g_kwmap) {
        struct fsqlf_kw_conf* kw = fsqlf_kw_get(g_kwmap, keyword);
        if (kw) {
            kw->before.new_line = before_newlines;
            kw->before.indent = before_indent;
            kw->after.new_line = after_newlines;
            kw->after.indent = after_indent;
        }
    }
}

// Export function to set keyword spelling variation
EXPORT void fsqlf_set_keyword_spelling(int spelling_option) {
    if (g_initialized && g_kwmap) {
        fsqlf_kwmap_set_spelling(g_kwmap, (enum fsqlf_kwspelling)spelling_option);
    }
}
```

Also, remove or comment out the hardcoded settings in `fsqlf_init()` (lines 28-29):
```c
// Remove these hardcoded settings:
// fsqlf_kwmap_set_case(g_kwmap, FSQLF_KWCASE_UPPER);
// fsqlf_kwmap_set_major_clause_nl(g_kwmap, 1);
```

#### Step 2: Update fsqlf_dll.h

Add these declarations before the final `#ifdef __cplusplus` block:

```c
// Configuration functions
FSQLF_API void fsqlf_set_keyword_case(int case_option);
FSQLF_API void fsqlf_set_major_clause_newlines(int nl_count);
FSQLF_API void fsqlf_set_keyword_spacing(const char* keyword,
                                         int before_newlines, int before_indent,
                                         int after_newlines, int after_indent);
FSQLF_API void fsqlf_set_keyword_spelling(int spelling_option);

// Case option constants (matching fsqlf_kwcase enum)
#define FSQLF_CASE_ORIGINAL  0
#define FSQLF_CASE_LOWER     1
#define FSQLF_CASE_UPPER     2
#define FSQLF_CASE_INITCAP   3

// Spelling option constants (matching fsqlf_kwspelling enum)
#define FSQLF_SPELLING_ORIGINAL  0
#define FSQLF_SPELLING_DEFAULT   1
```

#### Step 3: Update SqlFormatter.cpp

In the SqlFormatter.cpp file:

1. Add function pointer typedefs (around line 30):
```cpp
typedef void (*fsqlf_set_keyword_case_fn)(int);
typedef void (*fsqlf_set_major_clause_newlines_fn)(int);
typedef void (*fsqlf_set_keyword_spacing_fn)(const char*, int, int, int, int);
typedef void (*fsqlf_set_keyword_spelling_fn)(int);
```

2. Add static function pointers (around line 40):
```cpp
static fsqlf_set_keyword_case_fn s_fsqlf_set_keyword_case = nullptr;
static fsqlf_set_major_clause_newlines_fn s_fsqlf_set_major_clause_newlines = nullptr;
static fsqlf_set_keyword_spacing_fn s_fsqlf_set_keyword_spacing = nullptr;
static fsqlf_set_keyword_spelling_fn s_fsqlf_set_keyword_spelling = nullptr;
```

3. In `InitializeDll()`, load the new functions (after loading existing functions):
```cpp
// Load configuration functions
s_fsqlf_set_keyword_case = (fsqlf_set_keyword_case_fn)GetProcAddress(s_hDll, "fsqlf_set_keyword_case");
s_fsqlf_set_major_clause_newlines = (fsqlf_set_major_clause_newlines_fn)GetProcAddress(s_hDll, "fsqlf_set_major_clause_newlines");
s_fsqlf_set_keyword_spacing = (fsqlf_set_keyword_spacing_fn)GetProcAddress(s_hDll, "fsqlf_set_keyword_spacing");
s_fsqlf_set_keyword_spelling = (fsqlf_set_keyword_spelling_fn)GetProcAddress(s_hDll, "fsqlf_set_keyword_spelling");
```

4. In the `Format()` function, before calling `s_fsqlf_format_wstring`, apply user settings:
```cpp
// Apply user formatting options
if (s_fsqlf_set_keyword_case) {
    int caseOption = 2; // Default to UPPER
    switch (options.keywordCase) {
        case SqlKeywordCase::Lower:
            caseOption = 1; // FSQLF_CASE_LOWER
            break;
        case SqlKeywordCase::Upper:
            caseOption = 2; // FSQLF_CASE_UPPER
            break;
        case SqlKeywordCase::Capitalize:
            caseOption = 3; // FSQLF_CASE_INITCAP
            break;
    }
    s_fsqlf_set_keyword_case(caseOption);
}

if (s_fsqlf_set_major_clause_newlines) {
    // Set newlines based on user preference
    int newlineCount = options.newlineBeforeKeywords ? 1 : 0;
    s_fsqlf_set_major_clause_newlines(newlineCount);
}

// Configure indent style if function is available
if (s_fsqlf_set_keyword_spacing) {
    int indentSize = 4; // Default
    switch (options.indentStyle) {
        case SqlIndentStyle::TwoSpaces:
            indentSize = 2;
            break;
        case SqlIndentStyle::FourSpaces:
            indentSize = 4;
            break;
        case SqlIndentStyle::Tab:
            indentSize = 1; // Tab counts as 1 indent unit in fsqlf
            break;
    }
    // Configure SELECT list items
    if (options.alignCommas) {
        s_fsqlf_set_keyword_spacing("comma", 0, 0, 1, indentSize);
    }
}
```

5. Remove the `ApplyFormattingOptions` workaround and its call since fsqlf will now format correctly.

#### Step 4: Rebuild fsqlf.dll

1. Navigate to `C:\Dev\UltimateDebugViewPP\fsqlf_dll`
2. Run: `cmake -B build -G "Visual Studio 17 2022" .`
3. Run: `cmake --build build --config Release`
4. Copy the new fsqlf.dll to both Debug and Release directories

#### Step 5: Build and Test UltimateDebugView++

1. Run: `cmake --build build --config Debug --target UltimateDebugviewpp`
2. Test with SQL that has keywords
3. Verify that preferences are now applied correctly

## Expected Results

After implementing these changes:
- SQL keyword case will respect user preferences (Upper/Lower/Capitalize)
- Indentation style will be configurable
- Newline placement will follow user settings
- Comments will be preserved
- Comma alignment will work as configured

## Testing

Test with SQL like:
```sql
SELECT id, name FROM users WHERE active = 1 ORDER BY created_at DESC
```

With lowercase preference, it should display as:
```sql
select id, name from users where active = 1 order by created_at desc
```