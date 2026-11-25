// Simple SQL formatter for DebugView++
// Formats SQL statements with proper indentation and line breaks

#include "DebugViewppLib/SqlFormatter.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cwctype>
#include <sstream>
#include <fstream>
#include <Windows.h>

namespace fusion {
namespace debugviewpp {

// Enums from fsqlf
enum fsqlf_kwcase
{
    FSQLF_KWCASE_ORIGINAL = 0,
    FSQLF_KWCASE_LOWER = 1,
    FSQLF_KWCASE_UPPER = 2,
    FSQLF_KWCASE_INITCAP = 3
};

// Forward declarations
struct fsqlf_kw_conf;
typedef struct fsqlf_kw_conf* fsqlf_kwmap_t;

// Function pointers for fsqlf DLL
typedef int (*fsqlf_init_fn)();
typedef int (*fsqlf_format_wstring_fn)(const wchar_t*, wchar_t**);
typedef void (*fsqlf_free_wstring_fn)(wchar_t*);
typedef void (*fsqlf_cleanup_fn)();

// New configuration function pointers
typedef void (*fsqlf_set_keyword_case_fn)(int);
typedef void (*fsqlf_set_major_clause_newlines_fn)(int);
typedef void (*fsqlf_set_keyword_spacing_fn)(const char*, int, int, int, int);
typedef void (*fsqlf_set_keyword_spelling_fn)(int);
typedef void (*fsqlf_set_indent_type_fn)(int);  // For fixed indent types
typedef void (*fsqlf_set_indent_string_fn)(const char*);  // For custom indent strings

class SqlFormatter {
private:
    static HMODULE s_hDll;
    static fsqlf_init_fn s_fsqlf_init;
    static fsqlf_format_wstring_fn s_fsqlf_format_wstring;
    static fsqlf_free_wstring_fn s_fsqlf_free_wstring;
    static fsqlf_cleanup_fn s_fsqlf_cleanup;

    // New configuration functions
    static fsqlf_set_keyword_case_fn s_fsqlf_set_keyword_case;
    static fsqlf_set_major_clause_newlines_fn s_fsqlf_set_major_clause_newlines;
    static fsqlf_set_keyword_spacing_fn s_fsqlf_set_keyword_spacing;
    static fsqlf_set_keyword_spelling_fn s_fsqlf_set_keyword_spelling;
    static fsqlf_set_indent_type_fn s_fsqlf_set_indent_type;  // For fixed indent types
    static fsqlf_set_indent_string_fn s_fsqlf_set_indent_string;  // For custom indent strings

    static bool s_initialized;
    
    static void LogDebug(const std::wstring& message) {
        std::wofstream log(L"C:\\Dev\\DebugViewPP\\sql_formatter_debug.log", std::ios::app);
        if (log.is_open()) {
            SYSTEMTIME st;
            GetLocalTime(&st);
            log << L"[" << st.wHour << L":" << st.wMinute << L":" << st.wSecond << L"." << st.wMilliseconds << L"] " 
                << message << std::endl;
            log.close();
        }
    }
    
    static bool InitializeDll() {
        if (s_initialized) {
            LogDebug(L"InitializeDll: Already initialized");
            return true;
        }
        
        LogDebug(L"InitializeDll: Starting initialization");
        
        // Try to load fsqlf.dll from the same directory as the executable
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        wchar_t* lastSlash = wcsrchr(path, L'\\');
        if (lastSlash) {
            wcscpy(lastSlash + 1, L"fsqlf.dll");
        }
        
        LogDebug(L"Trying to load DLL from: " + std::wstring(path));
        s_hDll = LoadLibraryW(path);
        if (!s_hDll) {
            DWORD error = GetLastError();
            LogDebug(L"Failed to load from exe directory, error: " + std::to_wstring(error));
            
            // Try loading from current directory
            LogDebug(L"Trying to load from current directory");
            s_hDll = LoadLibraryW(L"fsqlf.dll");
            
            if (!s_hDll) {
                error = GetLastError();
                LogDebug(L"Failed to load from current directory, error: " + std::to_wstring(error));
            }
        }
        
        if (!s_hDll) {
            LogDebug(L"DLL loading failed completely");
            return false;
        }
        
        LogDebug(L"DLL loaded successfully");
        
        // Get function pointers
        s_fsqlf_init = (fsqlf_init_fn)GetProcAddress(s_hDll, "fsqlf_init");
        s_fsqlf_format_wstring = (fsqlf_format_wstring_fn)GetProcAddress(s_hDll, "fsqlf_format_wstring");
        s_fsqlf_free_wstring = (fsqlf_free_wstring_fn)GetProcAddress(s_hDll, "fsqlf_free_wstring");
        s_fsqlf_cleanup = (fsqlf_cleanup_fn)GetProcAddress(s_hDll, "fsqlf_cleanup");

        // Load configuration functions
        s_fsqlf_set_keyword_case = (fsqlf_set_keyword_case_fn)GetProcAddress(s_hDll, "fsqlf_set_keyword_case");
        s_fsqlf_set_major_clause_newlines = (fsqlf_set_major_clause_newlines_fn)GetProcAddress(s_hDll, "fsqlf_set_major_clause_newlines");
        s_fsqlf_set_keyword_spacing = (fsqlf_set_keyword_spacing_fn)GetProcAddress(s_hDll, "fsqlf_set_keyword_spacing");
        s_fsqlf_set_keyword_spelling = (fsqlf_set_keyword_spelling_fn)GetProcAddress(s_hDll, "fsqlf_set_keyword_spelling");
        s_fsqlf_set_indent_type = (fsqlf_set_indent_type_fn)GetProcAddress(s_hDll, "fsqlf_set_indent_type");  // Load indent type function
        s_fsqlf_set_indent_string = (fsqlf_set_indent_string_fn)GetProcAddress(s_hDll, "fsqlf_set_indent_string");  // Load custom indent function

        LogDebug(L"Function pointers: init=" + std::to_wstring((size_t)s_fsqlf_init) +
                 L", format=" + std::to_wstring((size_t)s_fsqlf_format_wstring) +
                 L", free=" + std::to_wstring((size_t)s_fsqlf_free_wstring) +
                 L", cleanup=" + std::to_wstring((size_t)s_fsqlf_cleanup));
        LogDebug(L"Config functions: set_case=" + std::to_wstring((size_t)s_fsqlf_set_keyword_case) +
                 L", set_newlines=" + std::to_wstring((size_t)s_fsqlf_set_major_clause_newlines) +
                 L", set_spacing=" + std::to_wstring((size_t)s_fsqlf_set_keyword_spacing) +
                 L", set_indent_type=" + std::to_wstring((size_t)s_fsqlf_set_indent_type));

        if (!s_fsqlf_init || !s_fsqlf_format_wstring || !s_fsqlf_free_wstring || !s_fsqlf_cleanup) {
            LogDebug(L"Failed to get all function pointers");
            FreeLibrary(s_hDll);
            s_hDll = NULL;
            return false;
        }

        // Initialize fsqlf
        LogDebug(L"Calling fsqlf_init");
        int initResult = s_fsqlf_init();
        LogDebug(L"fsqlf_init returned: " + std::to_wstring(initResult));

        if (initResult != 0) {
            LogDebug(L"fsqlf_init failed");
            FreeLibrary(s_hDll);
            s_hDll = NULL;
            return false;
        }


        s_initialized = true;
        LogDebug(L"InitializeDll: Success!");
        return true;
    }
    
public:
    static std::wstring Format(const std::wstring& sql, const SqlFormatOptions& options) {
        // Simple test write
        FILE* f = fopen("C:\\Dev\\UltimateDebugViewPP\\sql_formatter_called.txt", "a");
        if (f) {
            fprintf(f, "Format called with options: case=%d, indentType=%d, spaces=%d, comma_align=%d, newlines=%d\n",
                    (int)options.keywordCase, (int)options.indentType, options.indentSpaces,
                    (int)options.commaAlignment, options.newlineBeforeKeywords ? 1 : 0);
            fclose(f);
        }

        LogDebug(L"Format called with SQL length: " + std::to_wstring(sql.length()));

        // Try to use fsqlf DLL if available
        if (InitializeDll()) {
            LogDebug(L"Using fsqlf DLL for formatting");

            // Apply user formatting options EVERY time before formatting
            if (s_fsqlf_set_keyword_case) {
                int caseOption = 2; // Default to UPPER
                LogDebug(L"options.keywordCase enum value: " + std::to_wstring((int)options.keywordCase));
                switch (options.keywordCase) {
                    case SqlKeywordCase::Lower:
                        caseOption = 1; // FSQLF_CASE_LOWER
                        LogDebug(L"Setting keyword case to LOWER");
                        break;
                    case SqlKeywordCase::Upper:
                        caseOption = 2; // FSQLF_CASE_UPPER
                        LogDebug(L"Setting keyword case to UPPER");
                        break;
                    case SqlKeywordCase::Capitalize:
                        caseOption = 3; // FSQLF_CASE_INITCAP
                        LogDebug(L"Setting keyword case to CAPITALIZE");
                        break;
                    default:
                        LogDebug(L"Unknown keyword case value, using UPPER");
                        break;
                }

                // Log to file for debugging
                FILE* log = fopen("C:\\Dev\\UltimateDebugViewPP\\sql_formatter_called.txt", "a");
                if (log) {
                    fprintf(log, "Setting keyword case to %d (1=lower, 2=upper, 3=capitalize)\n", caseOption);
                    fclose(log);
                }

                LogDebug(L"Calling s_fsqlf_set_keyword_case with option: " + std::to_wstring(caseOption));
                s_fsqlf_set_keyword_case(caseOption);
            } else {
                LogDebug(L"s_fsqlf_set_keyword_case function pointer is NULL!");
            }

            if (s_fsqlf_set_major_clause_newlines) {
                // Set newlines based on user preference
                int newlineCount = options.newlineBeforeKeywords ? 1 : 0;
                LogDebug(L"Setting newline count to: " + std::to_wstring(newlineCount));
                s_fsqlf_set_major_clause_newlines(newlineCount);
            } else {
                LogDebug(L"WARNING: s_fsqlf_set_major_clause_newlines is NULL!");
            }

            // Configure native indentation first
            int indentSize = 1; // Always use 1 indent level
            if (options.indentType == SqlIndentType::Tab) {
                LogDebug(L"Setting native indent to Tab");
                if (s_fsqlf_set_indent_string) {
                    s_fsqlf_set_indent_string("\t");
                } else if (s_fsqlf_set_indent_type) {
                    s_fsqlf_set_indent_type(2);  // Fallback to old API: 2 = tabs
                }
            } else {
                // Spaces - use the custom number
                LogDebug(L"Setting native indent to " + std::to_wstring(options.indentSpaces) + L" spaces");
                if (s_fsqlf_set_indent_string) {
                    // Create a string with the specified number of spaces
                    std::string spaces(options.indentSpaces, ' ');
                    s_fsqlf_set_indent_string(spaces.c_str());
                } else if (s_fsqlf_set_indent_type) {
                    // Fallback to old API with fixed values
                    if (options.indentSpaces <= 2)
                        s_fsqlf_set_indent_type(0);  // 2 spaces
                    else
                        s_fsqlf_set_indent_type(1);  // 4 spaces
                }
            }

            if (!s_fsqlf_set_indent_string && !s_fsqlf_set_indent_type) {
                LogDebug(L"WARNING: Neither indent function is available - using default indentation");
            }

            // Configure keyword spacing based on newline preference
            if (s_fsqlf_set_keyword_spacing) {
                // Define all major SQL keywords that need spacing configuration
                const char* majorKeywords[] = {
                    "kw_select",
                    "kw_from",
                    "kw_where",
                    "kw_inner_join",
                    "kw_left_join",
                    "kw_right_join",
                    "kw_full_join",
                    "kw_cross_join",
                    "kw_groupby",
                    "kw_orderby",
                    "kw_having",
                    "kw_union",
                    "kw_union_all",
                    "kw_set",
                    "kw_update",
                    "kw_insertinto",
                    "kw_deletefrom"
                };

                // Configure spacing for major keywords based on format style and newline preference
                int keywordNewline = 0;

                // Determine newline count based on format style
                switch (options.formatStyle) {
                case SqlFormatStyle::Compact:
                    keywordNewline = 0;  // No newlines for compact
                    break;
                case SqlFormatStyle::Normal:
                    keywordNewline = options.newlineBeforeKeywords ? 1 : 0;  // Respect user preference
                    break;
                case SqlFormatStyle::Expanded:
                    keywordNewline = 2;  // Extra newlines for expanded
                    break;
                }

                for (int i = 0; i < sizeof(majorKeywords)/sizeof(majorKeywords[0]); i++) {
                    LogDebug(L"Setting spacing for " + std::wstring(majorKeywords[i], majorKeywords[i] + strlen(majorKeywords[i])) +
                             L" with newline=" + std::to_wstring(keywordNewline));
                    s_fsqlf_set_keyword_spacing(majorKeywords[i], keywordNewline, 0, 0, 0);
                }

                // Configure comma alignment (overridden by Compact format style)
                if (options.formatStyle == SqlFormatStyle::Compact) {
                    // Compact format: force inline commas regardless of alignment setting
                    LogDebug(L"Compact format: forcing inline commas");
                    s_fsqlf_set_keyword_spacing("kw_comma", 0, 0, 0, 0);
                    s_fsqlf_set_keyword_spacing("kw_comma_ins", 0, 0, 0, 0);
                    s_fsqlf_set_keyword_spacing("kw_comma_set", 0, 0, 0, 0);
                    s_fsqlf_set_keyword_spacing("kw_comma_grpby", 0, 0, 0, 0);
                    s_fsqlf_set_keyword_spacing("kw_comma_ordby", 0, 0, 0, 0);
                } else {
                    // Normal or Expanded format: use comma alignment setting
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
                }

                // Special handling for AND, OR, ON - only when newlines are enabled
                if (options.newlineBeforeKeywords) {
                    LogDebug(L"Configuring special keyword spacing with indentation");
                    // ON gets indentation but no newline
                    s_fsqlf_set_keyword_spacing("kw_on", 0, indentSize, 0, 0);
                    // AND/OR get newline and indentation
                    s_fsqlf_set_keyword_spacing("kw_and", 1, indentSize, 0, 0);
                    s_fsqlf_set_keyword_spacing("kw_or", 1, indentSize, 0, 0);
                } else {
                    LogDebug(L"Configuring AND/OR/ON for inline formatting");
                    // No newlines or special indentation for AND/OR/ON
                    s_fsqlf_set_keyword_spacing("kw_on", 0, 0, 0, 0);
                    s_fsqlf_set_keyword_spacing("kw_and", 0, 0, 0, 0);
                    s_fsqlf_set_keyword_spacing("kw_or", 0, 0, 0, 0);
                }

                // Configure parentheses for subqueries
                switch (options.parenthesesStyle) {
                case SqlParenthesesStyle::SameLine:
                    LogDebug(L"Setting parentheses style to same line");
                    // No newlines around parentheses
                    s_fsqlf_set_keyword_spacing("kw_left_p_sub", 0, 0, 0, 0);
                    s_fsqlf_set_keyword_spacing("kw_right_p_sub", 0, 0, 0, 0);
                    break;
                case SqlParenthesesStyle::NewLine:
                    LogDebug(L"Setting parentheses style to new line");
                    // Parentheses on separate lines
                    s_fsqlf_set_keyword_spacing("kw_left_p_sub", 1, 0, 0, 0);
                    s_fsqlf_set_keyword_spacing("kw_right_p_sub", 0, 0, 1, 0);
                    break;
                case SqlParenthesesStyle::IndentContent:
                    LogDebug(L"Setting parentheses style to indent content");
                    // Additional indentation inside parentheses
                    s_fsqlf_set_keyword_spacing("kw_left_p_sub", 1, 1, 0, 0);
                    s_fsqlf_set_keyword_spacing("kw_right_p_sub", 0, -1, 1, 0);
                    break;
                }
            } else {
                LogDebug(L"WARNING: s_fsqlf_set_keyword_spacing is NULL!");
            }

            wchar_t* formatted = nullptr;
            int formatResult = s_fsqlf_format_wstring(sql.c_str(), &formatted);
            LogDebug(L"fsqlf_format_wstring returned: " + std::to_wstring(formatResult));

            if (formatResult == 0 && formatted) {
                std::wstring result(formatted);
                LogDebug(L"Formatted SQL length: " + std::to_wstring(result.length()));
                LogDebug(L"fsqlf output (first 200 chars): " + result.substr(0, 200));
                s_fsqlf_free_wstring(formatted);

                // No post-processing needed - fsqlf now handles indentation natively
                LogDebug(L"Native indentation applied, returning fsqlf result directly");
                return result;
            } else {
                LogDebug(L"fsqlf_format_wstring failed or returned null");
            }
        } else {
            LogDebug(L"DLL not available, using simple formatter");
        }
        
        // Fall back to simple formatter if DLL not available or fails
        LogDebug(L"Falling back to simple formatter");
        return FormatSimple(sql, options);
    }
    
    static std::wstring FormatSimple(const std::wstring& sql, const SqlFormatOptions& options) {
        if (sql.empty()) return sql;
        
        // First, extract and remove all comments
        std::vector<std::pair<size_t, std::wstring>> comments;
        std::wstring sqlWithoutComments;
        size_t pos = 0;
        bool inString = false;
        wchar_t stringChar = 0;
        
        while (pos < sql.length()) {
            // Handle strings to avoid treating -- inside strings as comments
            if (!inString && (sql[pos] == L'\'' || sql[pos] == L'"')) {
                inString = true;
                stringChar = sql[pos];
                sqlWithoutComments += sql[pos++];
                continue;
            }
            
            if (inString && sql[pos] == stringChar) {
                sqlWithoutComments += sql[pos++];
                // Check for escaped quote
                if (pos < sql.length() && sql[pos] == stringChar) {
                    sqlWithoutComments += sql[pos++];
                } else {
                    inString = false;
                }
                continue;
            }
            
            if (inString) {
                sqlWithoutComments += sql[pos++];
                continue;
            }
            
            // Handle comments
            if (!inString && pos + 1 < sql.length() && sql[pos] == L'-' && sql[pos + 1] == L'-') {
                // Found a comment, extract everything from here to end of line or end of string
                std::wstring comment;
                comment += sql[pos++]; // First dash
                comment += sql[pos++]; // Second dash
                
                // Get rest of comment until newline or end
                while (pos < sql.length() && sql[pos] != L'\n') {
                    comment += sql[pos++];
                }
                
                // Store comment for later
                comments.push_back({sqlWithoutComments.length(), comment});
                
                // If we hit a newline, add it to the SQL (but not the comment)
                if (pos < sql.length() && sql[pos] == L'\n') {
                    sqlWithoutComments += sql[pos++];
                }
                continue;
            }
            
            // Regular character
            sqlWithoutComments += sql[pos++];
        }
        
        // Now format the SQL without comments
        std::wstring formatted = FormatSQLWithoutComments(sqlWithoutComments, options);
        
        // Comments are not preserved in current implementation
        
        return formatted;
    }
    

private:
    static std::wstring FormatSQLWithoutComments(const std::wstring& sql, const SqlFormatOptions& options) {
        if (sql.empty()) return sql;
        
        // List of SQL keywords that should start a new line
        // Order matters - longer keywords first to avoid partial matches
        std::vector<std::wstring> newlineKeywords = {
            L"LEFT OUTER JOIN", L"RIGHT OUTER JOIN", L"FULL OUTER JOIN",
            L"LEFT JOIN", L"RIGHT JOIN", L"INNER JOIN", L"CROSS JOIN",
            L"GROUP BY", L"ORDER BY", L"UNION ALL",
            L"INSERT INTO", L"DELETE FROM",
            L"SELECT", L"FROM", L"WHERE", L"HAVING",
            L"JOIN", L"ON", L"UNION",
            L"VALUES", L"UPDATE", L"SET",
            L"CREATE", L"ALTER", L"DROP", L"WITH", L"LIMIT", L"OFFSET"
        };
        
        // Keywords that increase indentation
        std::vector<std::wstring> indentKeywords = {
            L"SELECT"
        };
        
        // Keywords that decrease indentation
        std::vector<std::wstring> unindentKeywords = {
            L"FROM", L"WHERE", L"GROUP BY", L"HAVING", L"ORDER BY", L"UNION", L"LIMIT", L"OFFSET"
        };
        
        std::wstring result;
        std::wstring upperSql = ToUpper(sql);
        size_t pos = 0;
        int indentLevel = 0;
        bool inString = false;
        wchar_t stringChar = 0;
        
        // Skip leading whitespace
        while (pos < sql.length() && ::iswspace(sql[pos])) {
            pos++;
        }
        
        while (pos < sql.length()) {
            // Handle strings
            if (!inString && (sql[pos] == L'\'' || sql[pos] == L'"')) {
                inString = true;
                stringChar = sql[pos];
                result += sql[pos++];
                continue;
            }
            
            if (inString && sql[pos] == stringChar) {
                result += sql[pos++];
                // Check for escaped quote
                if (pos < sql.length() && sql[pos] == stringChar) {
                    result += sql[pos++];
                } else {
                    inString = false;
                }
                continue;
            }
            
            if (inString) {
                result += sql[pos++];
                continue;
            }
            
            // Check for keywords
            bool keywordFound = false;
            for (const auto& keyword : newlineKeywords) {
                if (pos + keyword.length() <= upperSql.length()) {
                    std::wstring substr = upperSql.substr(pos, keyword.length());

                    // Check if this matches the keyword and is followed by space or punctuation
                    if (substr == keyword) {
                        bool validEnd = (pos + keyword.length() >= upperSql.length()) ||
                                       !::iswalnum(upperSql[pos + keyword.length()]);

                        if (validEnd) {
                            // Check if we need to unindent
                            for (const auto& unindentKw : unindentKeywords) {
                                if (keyword == unindentKw && indentLevel > 0) {
                                    indentLevel--;
                                    break;
                                }
                            }

                            // Add newline and indentation if needed
                            if (options.newlineBeforeKeywords) {
                                if (!result.empty() && result.back() != L'\n') {
                                    result += L"\n";
                                }

                                // Add indentation based on style
                                for (int i = 0; i < indentLevel; i++) {
                                    if (options.indentType == SqlIndentType::Tab) {
                                        result += L"\t";
                                    } else {
                                        // Add the specified number of spaces
                                        result += std::wstring(options.indentSpaces, L' ');
                                    }
                                }
                            } else if (!result.empty()) {
                                result += L" ";
                            }

                            // Add the keyword with correct case
                            std::wstring keywordToAdd = sql.substr(pos, keyword.length());
                            switch (options.keywordCase) {
                                case SqlKeywordCase::Upper:
                                    keywordToAdd = keyword; // Already uppercase
                                    break;
                                case SqlKeywordCase::Lower:
                                    std::transform(keywordToAdd.begin(), keywordToAdd.end(), keywordToAdd.begin(), ::towlower);
                                    break;
                                case SqlKeywordCase::Capitalize:
                                    if (!keywordToAdd.empty()) {
                                        keywordToAdd[0] = ::towupper(keywordToAdd[0]);
                                        std::transform(keywordToAdd.begin() + 1, keywordToAdd.end(), keywordToAdd.begin() + 1, ::towlower);
                                    }
                                    break;
                            }
                            result += keywordToAdd;
                            pos += keyword.length();
                            
                            // Check if we need to indent
                            for (const auto& indentKw : indentKeywords) {
                                if (keyword == indentKw) {
                                    indentLevel++;
                                    break;
                                }
                            }
                            
                            // Add space after keyword if needed
                            if (pos < sql.length() && !::iswspace(sql[pos])) {
                                result += L" ";
                            }
                            
                            keywordFound = true;
                            break;
                        }
                    }
                }
            }
            
            if (!keywordFound) {
                // Handle commas in SELECT lists
                if (sql[pos] == L',') {
                    result += L",";
                    pos++;

                    // Skip whitespace after comma
                    while (pos < sql.length() && ::iswspace(sql[pos])) {
                        pos++;
                    }

                    // Add newline and indentation for next item if comma alignment is not inline
                    if (options.commaAlignment != SqlCommaAlignment::Inline && pos < sql.length()) {
                        result += L"\n";
                        for (int i = 0; i < indentLevel; i++) {
                            if (options.indentType == SqlIndentType::Tab) {
                                result += L"\t";
                            } else {
                                // Add the specified number of spaces
                                result += std::wstring(options.indentSpaces, L' ');
                            }
                        }
                    } else if (pos < sql.length()) {
                        result += L" ";
                    }
                }
                // Handle parentheses
                else if (sql[pos] == L'(') {
                    result += sql[pos++];
                    indentLevel++;
                }
                else if (sql[pos] == L')') {
                    if (indentLevel > 0) indentLevel--;
                    result += sql[pos++];
                }
                // Handle multiple spaces
                else if (::iswspace(sql[pos])) {
                    // Collapse multiple spaces to one
                    result += L' ';
                    pos++;
                    while (pos < sql.length() && ::iswspace(sql[pos])) {
                        pos++;
                    }
                }
                else {
                    result += sql[pos++];
                }
            }
        }
        
        return result;
    }
    
private:
    static std::wstring ToUpper(const std::wstring& str) {
        std::wstring result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::towupper);
        return result;
    }
};

// Static member definitions
HMODULE SqlFormatter::s_hDll = NULL;
fsqlf_init_fn SqlFormatter::s_fsqlf_init = nullptr;
fsqlf_format_wstring_fn SqlFormatter::s_fsqlf_format_wstring = nullptr;
fsqlf_free_wstring_fn SqlFormatter::s_fsqlf_free_wstring = nullptr;
fsqlf_cleanup_fn SqlFormatter::s_fsqlf_cleanup = nullptr;

fsqlf_set_keyword_case_fn SqlFormatter::s_fsqlf_set_keyword_case = nullptr;
fsqlf_set_major_clause_newlines_fn SqlFormatter::s_fsqlf_set_major_clause_newlines = nullptr;
fsqlf_set_keyword_spacing_fn SqlFormatter::s_fsqlf_set_keyword_spacing = nullptr;
fsqlf_set_keyword_spelling_fn SqlFormatter::s_fsqlf_set_keyword_spelling = nullptr;
fsqlf_set_indent_type_fn SqlFormatter::s_fsqlf_set_indent_type = nullptr;
fsqlf_set_indent_string_fn SqlFormatter::s_fsqlf_set_indent_string = nullptr;

bool SqlFormatter::s_initialized = false;

// Export the format functions
std::wstring FormatSQL(const std::wstring& sql) {
    // Use default options
    SqlFormatOptions defaultOptions;
    return SqlFormatter::Format(sql, defaultOptions);
}

std::wstring FormatSQL(const std::wstring& sql, const SqlFormatOptions& options) {
    return SqlFormatter::Format(sql, options);
}

} // namespace debugviewpp
} // namespace fusion