#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

typedef int (*fsqlf_init_fn)();
typedef int (*fsqlf_format_string_fn)(const char*, char**);
typedef void (*fsqlf_free_string_fn)(char*);
typedef void (*fsqlf_cleanup_fn)();
typedef void (*fsqlf_set_indent_type_fn)(int);
typedef void (*fsqlf_set_keyword_case_fn)(int);

int main() {
    HMODULE hDll = LoadLibrary("fsqlf.dll");
    if (!hDll) {
        printf("Failed to load fsqlf.dll\n");
        return 1;
    }

    fsqlf_init_fn fsqlf_init = (fsqlf_init_fn)GetProcAddress(hDll, "fsqlf_init");
    fsqlf_format_string_fn fsqlf_format_string = (fsqlf_format_string_fn)GetProcAddress(hDll, "fsqlf_format_string");
    fsqlf_free_string_fn fsqlf_free_string = (fsqlf_free_string_fn)GetProcAddress(hDll, "fsqlf_free_string");
    fsqlf_cleanup_fn fsqlf_cleanup = (fsqlf_cleanup_fn)GetProcAddress(hDll, "fsqlf_cleanup");
    fsqlf_set_indent_type_fn fsqlf_set_indent_type = (fsqlf_set_indent_type_fn)GetProcAddress(hDll, "fsqlf_set_indent_type");
    fsqlf_set_keyword_case_fn fsqlf_set_keyword_case = (fsqlf_set_keyword_case_fn)GetProcAddress(hDll, "fsqlf_set_keyword_case");

    if (!fsqlf_init || !fsqlf_format_string || !fsqlf_free_string || !fsqlf_cleanup || !fsqlf_set_indent_type) {
        printf("Failed to get function pointers\n");
        FreeLibrary(hDll);
        return 1;
    }

    fsqlf_init();

    const char* test_sql = "SELECT a, b, c FROM table1 WHERE x = 1 AND y = 2";

    printf("Testing fsqlf indentation support\n");
    printf("==================================\n\n");
    printf("Original SQL:\n%s\n\n", test_sql);

    // Test 1: 2 spaces
    printf("Test 1: 2 spaces indentation\n");
    printf("-----------------------------\n");
    fsqlf_set_indent_type(0);  // 0 = 2 spaces
    if (fsqlf_set_keyword_case) fsqlf_set_keyword_case(2);  // UPPER case
    char* result = NULL;
    if (fsqlf_format_string(test_sql, &result) == 0 && result) {
        printf("%s\n", result);
        // Check if indentation is 2 spaces by looking for "  AND"
        if (strstr(result, "  AND")) {
            printf("✓ 2-space indentation detected\n");
        } else {
            printf("✗ 2-space indentation not detected\n");
        }
        fsqlf_free_string(result);
    }
    printf("\n");

    // Test 2: 4 spaces
    printf("Test 2: 4 spaces indentation\n");
    printf("-----------------------------\n");
    fsqlf_cleanup();
    fsqlf_init();
    fsqlf_set_indent_type(1);  // 1 = 4 spaces
    if (fsqlf_set_keyword_case) fsqlf_set_keyword_case(2);  // UPPER case
    result = NULL;
    if (fsqlf_format_string(test_sql, &result) == 0 && result) {
        printf("%s\n", result);
        // Check if indentation is 4 spaces by looking for "    AND"
        if (strstr(result, "    AND")) {
            printf("✓ 4-space indentation detected\n");
        } else {
            printf("✗ 4-space indentation not detected\n");
        }
        fsqlf_free_string(result);
    }
    printf("\n");

    // Test 3: tabs
    printf("Test 3: Tab indentation\n");
    printf("------------------------\n");
    fsqlf_cleanup();
    fsqlf_init();
    fsqlf_set_indent_type(2);  // 2 = tabs
    if (fsqlf_set_keyword_case) fsqlf_set_keyword_case(2);  // UPPER case
    result = NULL;
    if (fsqlf_format_string(test_sql, &result) == 0 && result) {
        // Display tabs as [TAB] for visibility
        char* p = result;
        while (*p) {
            if (*p == '\t') {
                printf("[TAB]");
            } else {
                putchar(*p);
            }
            p++;
        }
        printf("\n");
        // Check if indentation uses tabs
        if (strstr(result, "\tAND")) {
            printf("✓ Tab indentation detected\n");
        } else {
            printf("✗ Tab indentation not detected\n");
        }
        fsqlf_free_string(result);
    }
    printf("\n");

    fsqlf_cleanup();
    FreeLibrary(hDll);

    printf("Test completed!\n");
    return 0;
}