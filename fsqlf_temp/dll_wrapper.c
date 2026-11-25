// Simple DLL wrapper for fsqlf
// Provides a simple string-to-string formatting API

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/lib_fsqlf.h"
#include "lib_fsqlf/formatter/print_keywords.h"

static fsqlf_kwmap_t g_kwmap = NULL;
static int g_initialized = 0;

// Initialize the formatter (call once)
EXPORT int fsqlf_init() {
    if (g_initialized) {
        return 0;
    }
    
    // Initialize keyword map with defaults
    fsqlf_kwmap_init(&g_kwmap);

    // Set some reasonable defaults
    // Default to ORIGINAL case (no change) instead of UPPER
    fsqlf_kwmap_set_case(g_kwmap, FSQLF_KWCASE_ORIGINAL);
    // fsqlf_kwmap_set_major_clause_nl(g_kwmap, 1);

    // Try to read config file, but don't fail if not found
    // Commented out - config file was overriding programmatic settings
    // fsqlf_kwmap_conffile_read_default(g_kwmap);

    FILE* log = fopen("C:\\Dev\\UltimateDebugViewPP\\fsqlf_dll_debug.log", "a");
    if (log) {
        fprintf(log, "fsqlf_init: Initialized\n");
        fclose(log);
    }

    g_initialized = 1;
    return 0;
}

// Format SQL string
// Input: sql_in - input SQL string
// Output: sql_out - buffer to receive formatted SQL (must be freed by caller)
// Returns: 0 on success, -1 on error
EXPORT int fsqlf_format_string(const char* sql_in, char** sql_out) {
    if (!g_initialized) {
        if (fsqlf_init() != 0) {
            return -1;
        }
    }
    
    if (!sql_in || !sql_out) {
        return -1;
    }
    
    // Format the SQL
    fsqlf_format_bytes(g_kwmap, sql_in, strlen(sql_in), sql_out);
    
    return 0;
}

// Free memory allocated by fsqlf_format_string
EXPORT void fsqlf_free_string(char* str) {
    if (str) {
        free(str);
    }
}

// Cleanup (call when done)
EXPORT void fsqlf_cleanup() {
    if (g_initialized && g_kwmap) {
        fsqlf_kwmap_destroy(g_kwmap);
        g_kwmap = NULL;
        g_initialized = 0;
    }
}

// Wide string wrapper for Windows
#ifdef _WIN32
#include <windows.h>

EXPORT int fsqlf_format_wstring(const wchar_t* sql_in, wchar_t** sql_out) {
    if (!sql_in || !sql_out) {
        return -1;
    }
    
    // Convert wide string to UTF-8
    int in_size = WideCharToMultiByte(CP_UTF8, 0, sql_in, -1, NULL, 0, NULL, NULL);
    if (in_size <= 0) {
        return -1;
    }
    
    char* utf8_in = (char*)malloc(in_size);
    if (!utf8_in) {
        return -1;
    }
    
    WideCharToMultiByte(CP_UTF8, 0, sql_in, -1, utf8_in, in_size, NULL, NULL);
    
    // Format the SQL
    FILE* log = fopen("C:\\Dev\\UltimateDebugViewPP\\fsqlf_dll_debug.log", "a");
    if (log) {
        fprintf(log, "fsqlf_format_wstring: About to format SQL\n");
        fclose(log);
    }
    char* utf8_out = NULL;
    int result = fsqlf_format_string(utf8_in, &utf8_out);
    free(utf8_in);
    
    if (result != 0 || !utf8_out) {
        return -1;
    }
    
    // Convert result back to wide string
    int out_size = MultiByteToWideChar(CP_UTF8, 0, utf8_out, -1, NULL, 0);
    if (out_size <= 0) {
        fsqlf_free_string(utf8_out);
        return -1;
    }
    
    *sql_out = (wchar_t*)malloc(out_size * sizeof(wchar_t));
    if (!*sql_out) {
        fsqlf_free_string(utf8_out);
        return -1;
    }
    
    MultiByteToWideChar(CP_UTF8, 0, utf8_out, -1, *sql_out, out_size);
    fsqlf_free_string(utf8_out);
    
    return 0;
}

EXPORT void fsqlf_free_wstring(wchar_t* str) {
    if (str) {
        free(str);
    }
}

// Export function to set keyword case (Upper/Lower/Capitalize)
EXPORT void fsqlf_set_keyword_case(int case_option) {
    FILE* log = fopen("C:\\Dev\\UltimateDebugViewPP\\fsqlf_dll_debug.log", "a");
    if (log) {
        fprintf(log, "fsqlf_set_keyword_case called with option: %d\n", case_option);
        fprintf(log, "g_initialized: %d, g_kwmap: %p\n", g_initialized, g_kwmap);
        fclose(log);
    }
    if (g_initialized && g_kwmap) {
        fsqlf_kwmap_set_case(g_kwmap, (enum fsqlf_kwcase)case_option);
        if (log = fopen("C:\\Dev\\UltimateDebugViewPP\\fsqlf_dll_debug.log", "a")) {
            fprintf(log, "Successfully set keyword case to: %d\n", case_option);
            fclose(log);
        }
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
    FILE* log = fopen("C:\\Dev\\UltimateDebugViewPP\\fsqlf_dll_debug.log", "a");
    if (log) {
        fprintf(log, "fsqlf_set_keyword_spacing: keyword=%s, before_nl=%d, before_indent=%d, after_nl=%d, after_indent=%d\n",
                keyword, before_newlines, before_indent, after_newlines, after_indent);
        fprintf(log, "g_initialized: %d, g_kwmap: %p\n", g_initialized, g_kwmap);
        fclose(log);
    }

    if (g_initialized && g_kwmap) {
        struct fsqlf_kw_conf* kw = fsqlf_kw_get(g_kwmap, keyword);
        if (kw) {
            kw->before.new_line = before_newlines;
            kw->before.indent = before_indent;
            kw->after.new_line = after_newlines;
            kw->after.indent = after_indent;

            // Handle comma spacing specially - if it's a comma and no indent/newline, add single space
            if (strstr(keyword, "comma") != NULL && after_newlines == 0 && after_indent == 0) {
                kw->after.space = 1;  // Single space after comma
            }

            if (log = fopen("C:\\Dev\\UltimateDebugViewPP\\fsqlf_dll_debug.log", "a")) {
                fprintf(log, "Successfully set spacing for keyword '%s': before(nl=%d,ind=%d) after(nl=%d,ind=%d)\n",
                        keyword, before_newlines, before_indent, after_newlines, after_indent);
                fclose(log);
            }
        } else {
            if (log = fopen("C:\\Dev\\UltimateDebugViewPP\\fsqlf_dll_debug.log", "a")) {
                fprintf(log, "WARNING: Keyword '%s' not found in keyword map!\n", keyword);
                fclose(log);
            }
        }
    } else {
        if (log = fopen("C:\\Dev\\UltimateDebugViewPP\\fsqlf_dll_debug.log", "a")) {
            fprintf(log, "ERROR: Cannot set spacing - not initialized or kwmap is NULL\n");
            fclose(log);
        }
    }
}

// Export function to set keyword spelling variation
EXPORT void fsqlf_set_keyword_spelling(int spelling_option) {
    if (g_initialized && g_kwmap) {
        fsqlf_kwmap_set_spelling(g_kwmap, (enum fsqlf_kwspelling)spelling_option);
    }
}

// Debug function to list all available keywords
EXPORT void fsqlf_debug_list_keywords() {
    FILE* log = fopen("C:\\Dev\\UltimateDebugViewPP\\fsqlf_dll_debug.log", "a");
    if (!log) return;

    fprintf(log, "\n=== Available keywords in kwmap ===\n");

    // List common keywords we know exist from kwmap_defaults.def
    const char* known_keywords[] = {
        "kw_select", "kw_from", "kw_where", "kw_and", "kw_or",
        "kw_comma", "kw_inner_join", "kw_left_join", "kw_right_join",
        "kw_groupby", "kw_orderby", "kw_having", "kw_on",
        "kw_union", "kw_union_all", "kw_set", "kw_update",
        "kw_insertinto", "kw_deletefrom", "kw_comma_ins", "kw_comma_set",
        "kw_comma_grpby", "kw_comma_ordby", "kw_comma_join"
    };

    for (int i = 0; i < sizeof(known_keywords)/sizeof(known_keywords[0]); i++) {
        struct fsqlf_kw_conf* kw = fsqlf_kw_get(g_kwmap, known_keywords[i]);
        if (kw) {
            fprintf(log, "  Found: %s\n", known_keywords[i]);
        } else {
            fprintf(log, "  NOT FOUND: %s\n", known_keywords[i]);
        }
    }

    fprintf(log, "=================================\n");
    fclose(log);
}

// Set custom indent string (e.g., "  " for 2 spaces, "\t" for tab)
EXPORT void fsqlf_set_indent_string(const char* indent_str) {
    fsqlf_set_indent_string_internal(indent_str);
}

// Convenience function using indent type enum
// 0 = 2 spaces, 1 = 4 spaces, 2 = tabs
EXPORT void fsqlf_set_indent_type(int indent_type) {
    switch(indent_type) {
        case 0:
            fsqlf_set_indent_string("  ");  // 2 spaces
            break;
        case 1:
            fsqlf_set_indent_string("    ");  // 4 spaces
            break;
        case 2:
            fsqlf_set_indent_string("\t");  // tab
            break;
        default:
            fsqlf_set_indent_string("    ");  // default to 4 spaces
            break;
    }
}
#endif