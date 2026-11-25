# fsqlf.dll Native Indentation Support Implementation Plan

## Overview
Modify fsqlf.dll to natively support configurable indentation (2 spaces, 4 spaces, tabs) as shown in the UltimateDebugViewPP preferences dialog, without requiring post-processing.

## Current State
- fsqlf uses hardcoded 4-space indentation in `lib_fsqlf/formatter/print_keywords.c` (line 141)
- Keyword case configuration is working correctly
- Keyword spacing configuration is working correctly
- Indentation style preferences (2 spaces, 4 spaces, tabs) are NOT being applied

## Implementation Tasks

### 1. Modify print_keywords.c for Configurable Indentation
**File:** `C:\Dev\UltimateDebugViewPP\fsqlf_temp\lib_fsqlf\formatter\print_keywords.c`

**Changes needed:**
- Line 141: Replace `const char *indent = "    ";` with a global configurable variable
- Add global variables:
  ```c
  static char* g_indent_string = "    ";  // Default to 4 spaces
  static int g_indent_length = 4;
  ```
- Add setter function:
  ```c
  void fsqlf_set_indent_string_internal(const char* indent_str) {
      // Update g_indent_string and g_indent_length
  }
  ```
- Update `struct_spacing_to_str()` function to use `g_indent_string` and `g_indent_length`

### 2. Add DLL Export Functions
**File:** `C:\Dev\UltimateDebugViewPP\fsqlf_temp\dll_wrapper.c`

**Add new exports:**
```c
// Set custom indent string (e.g., "  " for 2 spaces, "\t" for tab)
EXPORT void fsqlf_set_indent_string(const char* indent_str);

// Convenience function using indent type enum
// 0 = 2 spaces, 1 = 4 spaces, 2 = tabs
EXPORT void fsqlf_set_indent_type(int indent_type);
```

### 3. Update SqlFormatter.cpp Integration
**File:** `C:\Dev\UltimateDebugViewPP\application\DebugViewppLib\SqlFormatter.cpp`

**Changes needed:**
- Add function pointer typedef for new indent configuration function
- Load and call `fsqlf_set_indent_type` based on `m_prefs.indentStyle`:
  - `IndentStyle::TwoSpaces` → call with 0
  - `IndentStyle::FourSpaces` → call with 1
  - `IndentStyle::Tabs` → call with 2

## File Structure
```
C:\Dev\UltimateDebugViewPP\
├── fsqlf_temp\
│   ├── dll_wrapper.c                    # Add export functions here
│   └── lib_fsqlf\
│       └── formatter\
│           └── print_keywords.c         # Modify indent handling here
├── fsqlf_dll\
│   └── CMakeLists.txt                  # Build configuration (no changes needed)
└── application\
    └── DebugViewppLib\
        └── SqlFormatter.cpp             # Update to use new functions
```

## Build Process
The project uses CMake with Visual Studio 2022:
```bash
"C:\Program Files\CMake\bin\cmake.exe" -B build -G "Visual Studio 17 2022" .
"C:\Program Files\CMake\bin\cmake.exe" --build build --config Debug --target fsqlf
```

## Testing
After implementation, test with SQL that demonstrates indentation:
```sql
SELECT a, b, c
FROM table1
WHERE x = 1
  AND y = 2
```

Should produce different indentation based on preference:
- 2 spaces: `  AND y = 2`
- 4 spaces: `    AND y = 2`
- Tabs: `	AND y = 2`

## Important Notes
1. The fsqlf library already has all necessary infrastructure for configuration
2. The DLL is built as part of the UltimateDebugViewPP CMake project
3. All keyword configuration uses internal names (e.g., "kw_select" not "SELECT")
4. The solution must be native to fsqlf.dll for reusability across projects