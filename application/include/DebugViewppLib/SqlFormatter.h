#pragma once

#include <string>

namespace fusion {
namespace debugviewpp {

// SQL Formatting Options
enum class SqlKeywordCase {
    Upper,      // SELECT, FROM, WHERE
    Lower,      // select, from, where
    Capitalize  // Select, From, Where
};

enum class SqlIndentType {
    Spaces,
    Tab
};

enum class SqlCommaAlignment {
    Inline,     // No line breaks, commas inline
    Trailing,   // Commas at end of line (default)
    Leading     // Commas at beginning of line
};

enum class SqlFormatStyle {
    Compact,    // Minimize newlines for dense output
    Normal,     // Default formatting
    Expanded    // Extra newlines between major clauses
};

enum class SqlParenthesesStyle {
    SameLine,       // No newlines around parentheses
    NewLine,        // Parentheses on separate lines
    IndentContent   // Additional indentation inside parentheses
};

enum class SqlFunctionStyle {
    Compact,        // COUNT(*) - no spaces
    Expanded,       // COUNT( * ) - with spaces
    MultiLine       // Multi-line for multiple arguments
};

struct SqlFormatOptions {
    SqlKeywordCase keywordCase = SqlKeywordCase::Upper;
    SqlIndentType indentType = SqlIndentType::Spaces;
    int indentSpaces = 4;  // Number of spaces when indentType is Spaces
    SqlCommaAlignment commaAlignment = SqlCommaAlignment::Trailing;
    SqlFormatStyle formatStyle = SqlFormatStyle::Normal;
    SqlParenthesesStyle parenthesesStyle = SqlParenthesesStyle::SameLine;
    SqlFunctionStyle functionStyle = SqlFunctionStyle::Compact;
    bool alignAliases = false;  // Vertically align column aliases
    bool newlineBeforeKeywords = true;
};

// Format SQL statement with default options
std::wstring FormatSQL(const std::wstring& sql);

// Format SQL statement with custom options
std::wstring FormatSQL(const std::wstring& sql, const SqlFormatOptions& options);

} // namespace debugviewpp
} // namespace fusion