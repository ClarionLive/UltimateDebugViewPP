#pragma once

// fsqlf DLL interface
// Simple wrapper around the fsqlf SQL formatter library

#ifdef _WIN32
    #ifdef FSQLF_DLL_EXPORTS
        #define FSQLF_API __declspec(dllexport)
    #else
        #define FSQLF_API __declspec(dllimport)
    #endif
#else
    #define FSQLF_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the formatter (call once)
FSQLF_API int fsqlf_init();

// Format SQL string
// Input: sql_in - input SQL string
// Output: sql_out - buffer to receive formatted SQL (must be freed by caller using fsqlf_free_string)
// Returns: 0 on success, -1 on error
FSQLF_API int fsqlf_format_string(const char* sql_in, char** sql_out);

// Free memory allocated by fsqlf_format_string
FSQLF_API void fsqlf_free_string(char* str);

// Wide string version for Windows
#ifdef _WIN32
FSQLF_API int fsqlf_format_wstring(const wchar_t* sql_in, wchar_t** sql_out);
FSQLF_API void fsqlf_free_wstring(wchar_t* str);
#endif

// Cleanup (call when done)
FSQLF_API void fsqlf_cleanup();

#ifdef __cplusplus
}
#endif