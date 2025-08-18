// Simple SQL formatter for DebugView++
// Formats SQL statements with proper indentation and line breaks

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

// Function pointers for fsqlf DLL
typedef int (*fsqlf_init_fn)();
typedef int (*fsqlf_format_wstring_fn)(const wchar_t*, wchar_t**);
typedef void (*fsqlf_free_wstring_fn)(wchar_t*);
typedef void (*fsqlf_cleanup_fn)();

class SqlFormatter {
private:
    static HMODULE s_hDll;
    static fsqlf_init_fn s_fsqlf_init;
    static fsqlf_format_wstring_fn s_fsqlf_format_wstring;
    static fsqlf_free_wstring_fn s_fsqlf_free_wstring;
    static fsqlf_cleanup_fn s_fsqlf_cleanup;
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
        
        LogDebug(L"Function pointers: init=" + std::to_wstring((size_t)s_fsqlf_init) +
                 L", format=" + std::to_wstring((size_t)s_fsqlf_format_wstring) +
                 L", free=" + std::to_wstring((size_t)s_fsqlf_free_wstring) +
                 L", cleanup=" + std::to_wstring((size_t)s_fsqlf_cleanup));
        
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
    static std::wstring Format(const std::wstring& sql) {
        // Simple test write
        FILE* f = fopen("C:\\Dev\\DebugViewPP\\sql_formatter_called.txt", "a");
        if (f) {
            fprintf(f, "Format called with length: %zu\n", sql.length());
            fclose(f);
        }
        
        LogDebug(L"Format called with SQL length: " + std::to_wstring(sql.length()));
        
        // Try to use fsqlf DLL if available
        if (InitializeDll()) {
            LogDebug(L"Using fsqlf DLL for formatting");
            wchar_t* formatted = nullptr;
            int formatResult = s_fsqlf_format_wstring(sql.c_str(), &formatted);
            LogDebug(L"fsqlf_format_wstring returned: " + std::to_wstring(formatResult));
            
            if (formatResult == 0 && formatted) {
                std::wstring result(formatted);
                LogDebug(L"Formatted SQL length: " + std::to_wstring(result.length()));
                s_fsqlf_free_wstring(formatted);
                return result;
            } else {
                LogDebug(L"fsqlf_format_wstring failed or returned null");
            }
        } else {
            LogDebug(L"DLL not available, using simple formatter");
        }
        
        // Fall back to simple formatter if DLL not available or fails
        LogDebug(L"Falling back to simple formatter");
        return FormatSimple(sql);
    }
    
    static std::wstring FormatSimple(const std::wstring& sql) {
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
        std::wstring formatted = FormatSQLWithoutComments(sqlWithoutComments);
        
        // Add comments back at the end if any
        if (!comments.empty()) {
            for (const auto& comment : comments) {
                formatted += L"\n" + comment.second;
            }
        }
        
        return formatted;
    }
    
private:
    static std::wstring FormatSQLWithoutComments(const std::wstring& sql) {
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
                            
                            // Add newline and indentation
                            if (!result.empty() && result.back() != L'\n') {
                                result += L"\n";
                            }
                            
                            // Add indentation
                            for (int i = 0; i < indentLevel; i++) {
                                result += L"    ";
                            }
                            
                            // Add the keyword (preserve original case)
                            result += sql.substr(pos, keyword.length());
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
                    
                    // Add newline and indentation for next item
                    if (pos < sql.length()) {
                        result += L"\n";
                        for (int i = 0; i < indentLevel; i++) {
                            result += L"    ";
                        }
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
bool SqlFormatter::s_initialized = false;

// Export the format function
std::wstring FormatSQL(const std::wstring& sql) {
    return SqlFormatter::Format(sql);
}

} // namespace debugviewpp
} // namespace fusion