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
    fsqlf_kwmap_set_case(g_kwmap, FSQLF_KWCASE_UPPER);
    fsqlf_kwmap_set_major_clause_nl(g_kwmap, 1);
    
    // Try to read config file, but don't fail if not found
    fsqlf_kwmap_conffile_read_default(g_kwmap);
    
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
#endif