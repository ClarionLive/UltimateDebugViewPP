// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "DebugViewppLib/SyntaxHighlighter.h"
#include "DebugViewppLib/SqlFormatter.h"
#include <regex>
#include <algorithm>
#include <cctype>
#include <cwctype>
#include <fstream>
#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlctrls.h>

namespace fusion {
namespace debugviewpp {

SyntaxHighlighter::SyntaxHighlighter()
{
}

SyntaxHighlighter::~SyntaxHighlighter()
{
}

ContentFormat SyntaxHighlighter::DetectFormat(const std::wstring& text)
{
    // Check Error messages first - they have specific format
    if (IsErrorMessage(text))
        return ContentFormat::ErrorMessage;

    // Check Clarion Event messages first - they have a very specific prefix
    if (IsClarionEvent(text))
        return ContentFormat::ClarionEvent;

    // Check Clarion Group messages - they have pipe-separated field format
    if (IsClarionGroup(text))
        return ContentFormat::ClarionGroup;

    // Check for [SQL] prefix first for explicit SQL content
    if (text.find(L"[SQL]") == 0 || text.find(L"[sql]") == 0)
        return ContentFormat::SQL;

    // Check SQL FIRST - it's more specific than JSON detection which just looks for brackets
    // SQL might have log prefixes like DEBUG:
    if (IsSQL(text))
        return ContentFormat::SQL;

    // Check XML before JSON since XML has more specific patterns
    // and JSON detection can be triggered by angle brackets in XML
    if (IsXML(text))
        return ContentFormat::XML;

    // Check JSON after XML
    if (IsJSON(text))
        return ContentFormat::JSON;

    // Check for stack traces before log levels
    // (stack traces may start with ERROR: but have specific patterns)
    if (IsStackTrace(text))
        return ContentFormat::StackTrace;

    // Check log levels last (most generic)
    if (HasLogLevel(text))
        return ContentFormat::LogLevel;

    return ContentFormat::PlainText;
}

std::vector<TextRange> SyntaxHighlighter::GetHighlightRanges(const std::wstring& text, ContentFormat format)
{
    if (!m_enabled)
        return {};
    
    // Auto-detect format if not specified
    if (format == ContentFormat::Unknown)
    {
        format = DetectFormat(text);
    }
    
    // For now, return empty ranges until we implement the parsers
    // This will be implemented in the next phase
    return {};
}

void SyntaxHighlighter::ApplyHighlighting(HWND hRichEdit, const std::wstring& text, bool isDarkTheme)
{
    if (!m_enabled || !hRichEdit)
        return;
    
    ContentFormat format = DetectFormat(text);
    if (format == ContentFormat::PlainText)
        return;
    
    std::vector<TextRange> ranges;
    
    switch (format)
    {
    case ContentFormat::JSON:
        ranges = ParseJSON(text, isDarkTheme);
        break;
    case ContentFormat::XML:
        ranges = ParseXML(text, isDarkTheme);
        break;
    case ContentFormat::SQL:
        ranges = ParseSQL(text, isDarkTheme);
        break;
    case ContentFormat::StackTrace:
        ranges = ParseStackTrace(text, isDarkTheme);
        break;
    case ContentFormat::LogLevel:
        ranges = ParseLogLevel(text, isDarkTheme);
        break;
    default:
        return;
    }
    
    // Sort ranges by start position to ensure consistent application
    std::sort(ranges.begin(), ranges.end(), [](const TextRange& a, const TextRange& b) {
        return a.start < b.start;
    });
    
    // Disable redraw for the entire operation
    ::SendMessage(hRichEdit, WM_SETREDRAW, FALSE, 0);
    
    // Apply the ranges to the RichEdit control
    for (const auto& range : ranges)
    {
        ApplyRangeFormat(hRichEdit, range);
    }
    
    // Re-enable redraw and force update
    ::SendMessage(hRichEdit, WM_SETREDRAW, TRUE, 0);
    ::InvalidateRect(hRichEdit, nullptr, FALSE);
}

bool SyntaxHighlighter::IsJSON(const std::wstring& text)
{
    // Check if text contains JSON, even with a prefix
    size_t jsonStart = text.find(L'{');
    size_t jsonArrayStart = text.find(L'[');
    
    // No JSON markers found
    if (jsonStart == std::wstring::npos && jsonArrayStart == std::wstring::npos)
        return false;
    
    // If we only have square brackets at the beginning, check if it's a log level like [ERROR]
    if (jsonArrayStart == 0 && jsonStart == std::wstring::npos)
    {
        // Check if it's a log level pattern like [ERROR], [INFO], etc.
        std::wregex logLevelPattern(L"^\\s*\\[(ERROR|WARN|WARNING|INFO|DEBUG|TRACE|FATAL|CRITICAL)\\]", 
                                    std::regex_constants::icase);
        if (std::regex_search(text, logLevelPattern))
            return false;  // It's a log level, not JSON
    }
    
    // Find the actual start position
    size_t startPos = jsonStart;
    if (jsonArrayStart != std::wstring::npos && 
        (jsonStart == std::wstring::npos || jsonArrayStart < jsonStart))
    {
        startPos = jsonArrayStart;
    }
    
    // Check if we have a matching closing brace/bracket
    wchar_t openChar = text[startPos];
    wchar_t expectedClose = (openChar == L'{') ? L'}' : L']';
    
    // Simple check: find the last occurrence of the closing character
    size_t lastClose = text.rfind(expectedClose);
    if (lastClose == std::wstring::npos || lastClose <= startPos)
        return false;
    
    // Look for JSON-like patterns: "key": value
    std::wregex jsonPattern(L"\"[^\"]+\"\\s*:");
    if (std::regex_search(text, jsonPattern))
        return true;
    
    // For array detection, check if content looks like JSON array
    if (openChar == L'[')
    {
        // Extract content between brackets
        std::wstring content = text.substr(startPos + 1, lastClose - startPos - 1);
        
        // Check for JSON array patterns: numbers, strings, objects, arrays
        // Must have at least one of: quoted strings, numbers, true/false/null, nested objects/arrays
        std::wregex jsonArrayPattern(L"(\"[^\"]*\"|\\d+(\\.\\d+)?|true|false|null|\\{|\\[)");
        if (!std::regex_search(content, jsonArrayPattern))
            return false;
    }
    
    // Check for common JSON keywords with word boundaries
    std::wregex jsonKeywordPattern(L"\\b(true|false|null)\\b");
    if (std::regex_search(text, jsonKeywordPattern))
        return true;
    
    // Only return true if we have strong JSON indicators
    return false;
}

bool SyntaxHighlighter::IsXML(const std::wstring& text)
{
    // Check for XML declaration or common XML patterns
    if (text.find(L"<?xml") != std::wstring::npos)
        return true;
    
    // Look for XML-like tags
    std::wregex xmlPattern(L"<[^>]+>.*</[^>]+>");
    if (std::regex_search(text, xmlPattern))
        return true;
    
    // Check for self-closing tags
    std::wregex selfClosingPattern(L"<[^>]+/>");
    if (std::regex_search(text, selfClosingPattern))
        return true;
    
    return false;
}

bool SyntaxHighlighter::IsClarionGroup(const std::wstring& text)
{
    // Check if the text has the Clarion Group format:
    // "Group Name",""|"FIELD1","Value1"|"FIELD2","Value2"|...
    // Must have at least one pipe character and quoted values

    size_t startPos = text.find_first_not_of(L" \t\r\n");
    if (startPos == std::wstring::npos)
        return false;

    // Must contain pipes for field separation
    if (text.find(L'|') == std::wstring::npos)
        return false;

    // Must have quotes (Group messages have quoted field names and values)
    if (text.find(L'"') == std::wstring::npos)
        return false;

    // Check for pattern: "...",".." or "..",""|"
    // Look for quote-comma-quote pattern which is typical in Group messages
    size_t quoteCommaQuote = text.find(L"\",\"");
    if (quoteCommaQuote != std::wstring::npos)
    {
        // Also check for pipe separator
        size_t pipePos = text.find(L"|\"");
        if (pipePos != std::wstring::npos)
            return true;
    }

    return false;
}

bool SyntaxHighlighter::IsSQL(const std::wstring& text)
{
    // Convert to uppercase for case-insensitive comparison
    std::wstring upper = text;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::towupper);
    
    // Must have at least one primary SQL command keyword
    const std::vector<std::wstring> primaryKeywords = {
        L"SELECT ", L"INSERT INTO", L"UPDATE ", L"DELETE FROM",
        L"CREATE TABLE", L"DROP TABLE", L"ALTER TABLE",
        L"EXEC ", L"EXECUTE ", L"BEGIN TRANSACTION"
    };
    
    bool hasPrimary = false;
    for (const auto& keyword : primaryKeywords)
    {
        if (upper.find(keyword) != std::wstring::npos)
        {
            hasPrimary = true;
            break;
        }
    }
    
    if (!hasPrimary)
        return false;
    
    // Also check for secondary SQL indicators
    const std::vector<std::wstring> secondaryKeywords = {
        L" FROM ", L" WHERE ", L" JOIN ", L" ORDER BY ",
        L" GROUP BY ", L" HAVING ", L" VALUES ", L" SET "
    };
    
    int matchCount = 0;
    for (const auto& keyword : secondaryKeywords)
    {
        if (upper.find(keyword) != std::wstring::npos)
            matchCount++;
    }
    
    // Need at least one secondary keyword to confirm it's SQL
    return matchCount > 0;
}

bool SyntaxHighlighter::IsStackTrace(const std::wstring& text)
{
    // Convert to uppercase for case-insensitive comparison
    std::wstring upper = text;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::towupper);
    
    // Check for explicit stack trace indicators
    if (upper.find(L"STACK TRACE") != std::wstring::npos)
        return true;
    
    // Look for common stack trace patterns
    std::wregex stackPatterns[] = {
        std::wregex(L"\\s+at\\s+[\\w\\.]+"),                    // .NET/Java style: "   at System.Something"
        std::wregex(L"Exception.*:"),                           // Exception messages
        std::wregex(L"\\w+Exception"),                          // Exception class names
        std::wregex(L"\\([^)]+\\.\\w+:\\d+\\)"),               // File:line patterns
        std::wregex(L"#\\d+\\s+0x[0-9a-fA-F]+\\s+in\\s+")      // GDB style
    };
    
    for (const auto& pattern : stackPatterns)
    {
        if (std::regex_search(text, pattern))
            return true;
    }
    
    return false;
}

bool SyntaxHighlighter::HasLogLevel(const std::wstring& text)
{
    // Check for common log level indicators at the beginning of lines
    // Support both "ERROR:" style and "[ERROR]" style
    std::wregex logLevelPattern1(L"^\\s*(ERROR|WARN|WARNING|INFO|DEBUG|TRACE|FATAL|CRITICAL)\\s*[:\\-\\|]", 
                                 std::regex_constants::icase);
    std::wregex logLevelPattern2(L"^\\s*\\[(ERROR|WARN|WARNING|INFO|DEBUG|TRACE|FATAL|CRITICAL)\\]", 
                                 std::regex_constants::icase);
    
    return std::regex_search(text, logLevelPattern1) || std::regex_search(text, logLevelPattern2);
}

bool SyntaxHighlighter::IsErrorMessage(const std::wstring& text)
{
    // Check if the text has the Error message format:
    // Message: <msg>,Errorcode: <code>,Error: <desc>,ErrorFile: <file>,FileError: <err>,FileErrorCode: <code>
    return text.find(L"Message:") != std::wstring::npos &&
           text.find(L"Errorcode:") != std::wstring::npos &&
           text.find(L"Error:") != std::wstring::npos;
}

bool SyntaxHighlighter::IsClarionEvent(const std::wstring& text)
{
    // Check if the text starts with Event* pattern (Clarion event message)
    // These messages come from lines with [Events] prefix but the prefix isn't in the message
    size_t startPos = text.find_first_not_of(L" \t\r\n");
    if (startPos == std::wstring::npos)
        return false;
    
    // Check if it starts with "Event*" followed by a number
    if (text.compare(startPos, 6, L"Event*") == 0)
    {
        // Make sure there's a number after Event*
        if (startPos + 6 < text.length() && std::iswdigit(text[startPos + 6]))
            return true;
    }
    
    return false;
}

std::string SyntaxHighlighter::GenerateRTFForJSON(const std::wstring& text, bool isDarkTheme, bool showLineNumbers)
{
    // Line numbers are now handled separately in the UI, not in RTF generation
    
    // RTF header with color table
    std::string rtf = "{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0\\fmodern\\fcharset0 Consolas;}}";
    
    // Color table for JSON syntax highlighting
    if (isDarkTheme)
    {
        rtf += "{\\colortbl;";
        rtf += "\\red86\\green156\\blue214;";     // cf1 - Keys (light blue)
        rtf += "\\red206\\green145\\blue120;";    // cf2 - String values (light brown)
        rtf += "\\red181\\green206\\blue168;";    // cf3 - Numbers (light green)
        rtf += "\\red184\\green139\\blue184;";    // cf4 - Boolean/null (light purple)
        rtf += "\\red220\\green220\\blue220;";    // cf5 - Normal text/punctuation (light gray)
        rtf += "}";
    }
    else
    {
        rtf += "{\\colortbl;";
        rtf += "\\red0\\green0\\blue255;";        // cf1 - Keys (blue)
        rtf += "\\red0\\green128\\blue0;";        // cf2 - String values (green)
        rtf += "\\red255\\green128\\blue0;";      // cf3 - Numbers (orange)
        rtf += "\\red128\\green0\\blue128;";      // cf4 - Boolean/null (purple)
        rtf += "\\red0\\green0\\blue0;";          // cf5 - Normal text/punctuation (black)
        rtf += "}";
    }
    
    // Set default font and color
    rtf += "\\f0\\fs18\\cf5 ";
    
    // State machine for JSON parsing
    enum class State {
        Normal,
        InString,
        InStringEscape,
        InNumber,
        AfterColon
    };
    
    State state = State::Normal;
    std::wstring buffer;
    bool lastWasColon = false;
    
    for (size_t i = 0; i < text.length(); ++i)
    {
        wchar_t ch = text[i];
        
        switch (state)
        {
            case State::Normal:
                if (ch == L'"')
                {
                    // Start of string - could be key or value
                    state = State::InString;
                    buffer.clear();
                    
                    // Check if this is after a colon (making it a value) or not (making it a key)
                    // Look back for recent colon
                    bool isValue = lastWasColon;
                    
                    if (isValue)
                    {
                        rtf += "{\\cf2 \""; // String value color
                    }
                    else
                    {
                        rtf += "{\\cf1 \""; // Key color
                    }
                }
                else if ((ch >= L'0' && ch <= L'9') || ch == L'-')
                {
                    // Start of number
                    state = State::InNumber;
                    rtf += "{\\cf3 ";
                    rtf += static_cast<char>(ch);
                }
                else if (ch == L't' || ch == L'f' || ch == L'n')
                {
                    // Possible boolean or null
                    std::wstring ahead = text.substr(i);
                    if (ahead.substr(0, 4) == L"true")
                    {
                        rtf += "{\\cf4 true}";
                        i += 3;
                    }
                    else if (ahead.substr(0, 5) == L"false")
                    {
                        rtf += "{\\cf4 false}";
                        i += 4;
                    }
                    else if (ahead.substr(0, 4) == L"null")
                    {
                        rtf += "{\\cf4 null}";
                        i += 3;
                    }
                    else
                    {
                        rtf += static_cast<char>(ch);
                    }
                }
                else if (ch == L':')
                {
                    rtf += static_cast<char>(ch);
                    lastWasColon = true;
                }
                else if (ch == L',')
                {
                    rtf += static_cast<char>(ch);
                    lastWasColon = false;
                }
                else if (ch == L'{' || ch == L'}')
                {
                    rtf += "\\";
                    rtf += static_cast<char>(ch);
                    lastWasColon = false;
                }
                else if (ch == L'\n')
                {
                    rtf += "\\par ";
                }
                else if (ch == L'\r')
                {
                    // Skip
                }
                else if (ch == L'\t')
                {
                    rtf += "\\tab ";
                }
                else if (!::iswspace(ch))
                {
                    lastWasColon = false;
                    rtf += static_cast<char>(ch);
                }
                else
                {
                    rtf += static_cast<char>(ch);
                }
                break;
                
            case State::InString:
                if (ch == L'\\')
                {
                    state = State::InStringEscape;
                    rtf += "\\\\";
                }
                else if (ch == L'"')
                {
                    // End of string
                    rtf += "\"}";
                    state = State::Normal;
                }
                else if (ch == L'\n')
                {
                    rtf += "\\par ";
                }
                else if (ch == L'\r')
                {
                    // Skip
                }
                else if (ch == L'\t')
                {
                    rtf += "\\tab ";
                }
                else if (ch < 128)
                {
                    if (ch == L'{' || ch == L'}')
                    {
                        rtf += "\\";
                    }
                    rtf += static_cast<char>(ch);
                }
                else
                {
                    rtf += "\\u" + std::to_string(static_cast<int>(ch)) + "?";
                }
                break;
                
            case State::InStringEscape:
                rtf += static_cast<char>(ch);
                state = State::InString;
                break;
                
            case State::InNumber:
                if ((ch >= L'0' && ch <= L'9') || ch == L'.' || ch == L'e' || ch == L'E' || ch == L'+' || ch == L'-')
                {
                    rtf += static_cast<char>(ch);
                }
                else
                {
                    // End of number
                    rtf += "}";
                    state = State::Normal;
                    i--; // Re-process this character
                }
                break;
        }
    }
    
    // Close any open formatting
    if (state == State::InString)
    {
        rtf += "}";
    }
    else if (state == State::InNumber)
    {
        rtf += "}";
    }
    
    rtf += "}"; // Close RTF document
    return rtf;
}

std::vector<TextRange> SyntaxHighlighter::ParseJSON(const std::wstring& text, bool isDarkTheme)
{
    std::vector<TextRange> ranges;
    
    // Simple JSON parser for basic highlighting
    // This is a placeholder - a full implementation would use a proper JSON parser
    
    // Find all quoted strings (both keys and values)
    std::wregex stringPattern(L"\"([^\"\\\\]|\\\\.)*\"");
    std::wsregex_iterator it(text.begin(), text.end(), stringPattern);
    std::wsregex_iterator end;
    
    while (it != end)
    {
        TextRange range;
        range.start = static_cast<int>(it->position());
        range.end = range.start + static_cast<int>(it->length());
        
        // Check if this is a key (followed by colon) or a value
        size_t colonPos = text.find(L':', range.end);
        bool isKey = false;
        if (colonPos != std::wstring::npos)
        {
            // Check if there's only whitespace between the string and the colon
            std::wstring between = text.substr(range.end, colonPos - range.end);
            if (between.find_first_not_of(L" \t\r\n") == std::wstring::npos)
            {
                isKey = true;
            }
        }
        
        if (isKey)
        {
            range.color = isDarkTheme ? HighlightColors::Dark::tag : HighlightColors::Light::tag;
        }
        else
        {
            range.color = isDarkTheme ? HighlightColors::Dark::string : HighlightColors::Light::string;
        }
        
        ranges.push_back(range);
        ++it;
    }
    
    // Find numbers
    std::wregex numberPattern(L"\\b-?\\d+(\\.\\d+)?([eE][+-]?\\d+)?\\b");
    it = std::wsregex_iterator(text.begin(), text.end(), numberPattern);
    
    while (it != end)
    {
        TextRange range;
        range.start = static_cast<int>(it->position());
        range.end = range.start + static_cast<int>(it->length());
        range.color = isDarkTheme ? HighlightColors::Dark::number : HighlightColors::Light::number;
        
        // Make sure this isn't inside a string
        bool insideString = false;
        for (const auto& stringRange : ranges)
        {
            if (range.start >= stringRange.start && range.end <= stringRange.end)
            {
                insideString = true;
                break;
            }
        }
        
        if (!insideString)
            ranges.push_back(range);
        
        ++it;
    }
    
    // Find boolean and null values
    std::wregex boolPattern(L"\\b(true|false|null)\\b");
    it = std::wsregex_iterator(text.begin(), text.end(), boolPattern);
    
    while (it != end)
    {
        TextRange range;
        range.start = static_cast<int>(it->position());
        range.end = range.start + static_cast<int>(it->length());
        range.color = isDarkTheme ? HighlightColors::Dark::boolean : HighlightColors::Light::boolean;
        
        // Make sure this isn't inside a string
        bool insideString = false;
        for (const auto& stringRange : ranges)
        {
            if (range.start >= stringRange.start && range.end <= stringRange.end)
            {
                insideString = true;
                break;
            }
        }
        
        if (!insideString)
            ranges.push_back(range);
        
        ++it;
    }
    
    return ranges;
}

// Helper function to convert wide string to RTF-escaped narrow string
static std::string WideToRTF(const std::wstring& text)
{
    std::string result;
    for (wchar_t ch : text)
    {
        if (ch < 128)
        {
            // ASCII characters
            switch (ch)
            {
                case '\\': result += "\\\\"; break;
                case '{': result += "\\{"; break;
                case '}': result += "\\}"; break;
                case '\n': result += "\\par "; break;
                case '\r': break; // Skip carriage returns
                case '\t': result += "\\tab "; break;
                default: result += static_cast<char>(ch); break;
            }
        }
        else
        {
            // Unicode characters
            result += "\\u" + std::to_string(static_cast<int>(ch)) + "?";
        }
    }
    return result;
}

std::string SyntaxHighlighter::GenerateRTFForXML(const std::wstring& text, bool isDarkTheme, bool showLineNumbers)
{
    // Line numbers are now handled separately in the UI, not in RTF generation
    // RTF header with color table
    std::string rtf = "{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0\\fmodern\\fcharset0 Consolas;}}";
    
    // Color table for syntax highlighting
    if (isDarkTheme)
    {
        rtf += "{\\colortbl;";
        rtf += "\\red86\\green156\\blue214;";     // cf1 - Tag/element (light blue)
        rtf += "\\red255\\green160\\blue122;";    // cf2 - Attribute (orange)
        rtf += "\\red206\\green145\\blue120;";    // cf3 - String (light brown)
        rtf += "\\red106\\green153\\blue85;";     // cf4 - Comment (green-gray)
        rtf += "\\red220\\green220\\blue220;";    // cf5 - Normal text (light gray)
        rtf += "}";
    }
    else
    {
        rtf += "{\\colortbl;";
        rtf += "\\red0\\green0\\blue255;";        // cf1 - Tag/element (blue)
        rtf += "\\red255\\green0\\blue0;";        // cf2 - Attribute (red)
        rtf += "\\red0\\green128\\blue0;";        // cf3 - String (green)
        rtf += "\\red128\\green128\\blue128;";    // cf4 - Comment (gray)
        rtf += "\\red0\\green0\\blue0;";          // cf5 - Normal text (black)
        rtf += "}";
    }
    
    // Set default font and color
    rtf += "\\f0\\fs18\\cf5 ";
    
    // State machine for XML parsing
    enum class State {
        Normal,
        TagStart,
        TagName,
        InTag,
        AttributeName,
        AttributeEquals,
        AttributeValueDQ,
        AttributeValueSQ,
        Comment,
        CDATA,
        XMLDecl
    };
    
    State state = State::Normal;
    std::wstring buffer;
    
    for (size_t i = 0; i < text.length(); ++i)
    {
        wchar_t ch = text[i];
        wchar_t nextCh = (i + 1 < text.length()) ? text[i + 1] : L'\0';
        
        switch (state)
        {
            case State::Normal:
                if (ch == L'<')
                {
                    // Check for special sequences
                    if (i + 3 < text.length() && text.substr(i, 4) == L"<!--")
                    {
                        state = State::Comment;
                        rtf += "{\\cf4\\i "; // Comment color, italic
                        rtf += WideToRTF(L"<!--");
                        i += 3;
                    }
                    else if (i + 8 < text.length() && text.substr(i, 9) == L"<![CDATA[")
                    {
                        state = State::CDATA;
                        rtf += "{\\cf3 "; // String color
                        rtf += WideToRTF(L"<![CDATA[");
                        i += 8;
                    }
                    else if (nextCh == L'?')
                    {
                        state = State::XMLDecl;
                        rtf += "{\\cf4\\i "; // Comment color, italic
                        rtf += WideToRTF(L"<?");
                        i++;
                    }
                    else
                    {
                        state = State::TagStart;
                        rtf += "{\\cf1 <"; // Tag color
                    }
                }
                else
                {
                    rtf += WideToRTF(std::wstring(1, ch));
                }
                break;
                
            case State::TagStart:
                if (ch == L'/' || ::iswalpha(ch) || ch == L'_')
                {
                    state = State::TagName;
                    buffer = ch == L'/' ? L"/" : L"";
                    if (ch != L'/') buffer += ch;
                }
                else if (ch == L'>')
                {
                    rtf += ">}";
                    state = State::Normal;
                }
                break;
                
            case State::TagName:
                if (::iswalnum(ch) || ch == L':' || ch == L'-' || ch == L'.' || ch == L'_')
                {
                    buffer += ch;
                }
                else
                {
                    rtf += "{\\b " + WideToRTF(buffer) + "}"; // Bold tag name
                    buffer.clear();
                    
                    if (ch == L'>')
                    {
                        rtf += ">}";
                        state = State::Normal;
                    }
                    else if (ch == L'/' && nextCh == L'>')
                    {
                        rtf += "/>}";
                        i++;
                        state = State::Normal;
                    }
                    else
                    {
                        rtf += WideToRTF(std::wstring(1, ch));
                        state = State::InTag;
                    }
                }
                break;
                
            case State::InTag:
                if (::iswalpha(ch) || ch == L'_')
                {
                    state = State::AttributeName;
                    buffer = ch;
                }
                else if (ch == L'>')
                {
                    rtf += ">}";
                    state = State::Normal;
                }
                else if (ch == L'/' && nextCh == L'>')
                {
                    rtf += "/>}";
                    i++;
                    state = State::Normal;
                }
                else
                {
                    rtf += WideToRTF(std::wstring(1, ch));
                }
                break;
                
            case State::AttributeName:
                if (::iswalnum(ch) || ch == L':' || ch == L'-' || ch == L'.' || ch == L'_')
                {
                    buffer += ch;
                }
                else if (ch == L'=')
                {
                    rtf += "{\\cf2 " + WideToRTF(buffer) + "}="; // Attribute color
                    buffer.clear();
                    state = State::AttributeEquals;
                }
                else
                {
                    rtf += "{\\cf2 " + WideToRTF(buffer) + "}"; // Attribute color
                    buffer.clear();
                    
                    if (ch == L'>')
                    {
                        rtf += ">}";
                        state = State::Normal;
                    }
                    else
                    {
                        rtf += WideToRTF(std::wstring(1, ch));
                        state = State::InTag;
                    }
                }
                break;
                
            case State::AttributeEquals:
                if (ch == L'"')
                {
                    state = State::AttributeValueDQ;
                    rtf += "{\\cf3 \""; // String color
                    buffer.clear();
                }
                else if (ch == L'\'')
                {
                    state = State::AttributeValueSQ;
                    rtf += "{\\cf3 '"; // String color
                    buffer.clear();
                }
                else if (!::iswspace(ch))
                {
                    rtf += WideToRTF(std::wstring(1, ch));
                    state = State::InTag;
                }
                else
                {
                    rtf += WideToRTF(std::wstring(1, ch));
                }
                break;
                
            case State::AttributeValueDQ:
                if (ch == L'"')
                {
                    rtf += WideToRTF(buffer) + "\"}";
                    buffer.clear();
                    state = State::InTag;
                }
                else
                {
                    buffer += ch;
                }
                break;
                
            case State::AttributeValueSQ:
                if (ch == L'\'')
                {
                    rtf += WideToRTF(buffer) + "'}";
                    buffer.clear();
                    state = State::InTag;
                }
                else
                {
                    buffer += ch;
                }
                break;
                
            case State::Comment:
                if (ch == L'-' && i + 2 < text.length() && text.substr(i, 3) == L"-->")
                {
                    rtf += "-->}";
                    i += 2;
                    state = State::Normal;
                }
                else
                {
                    rtf += WideToRTF(std::wstring(1, ch));
                }
                break;
                
            case State::CDATA:
                if (ch == L']' && i + 2 < text.length() && text.substr(i, 3) == L"]]>")
                {
                    rtf += "]]>}";
                    i += 2;
                    state = State::Normal;
                }
                else
                {
                    rtf += WideToRTF(std::wstring(1, ch));
                }
                break;
                
            case State::XMLDecl:
                if (ch == L'?' && nextCh == L'>')
                {
                    rtf += "?>}";
                    i++;
                    state = State::Normal;
                }
                else
                {
                    rtf += WideToRTF(std::wstring(1, ch));
                }
                break;
        }
    }
    
    // Close any open groups
    if (state != State::Normal)
    {
        rtf += "}";
    }
    
    rtf += "}"; // Close RTF document
    return rtf;
}

std::vector<TextRange> SyntaxHighlighter::ParseXML(const std::wstring& text, bool isDarkTheme)
{
    // This function is no longer used for XML - RTF generation is used instead
    std::vector<TextRange> ranges;
    
    enum class State {
        Normal,
        TagStart,           // Just saw <
        TagName,            // Reading tag name
        InTag,              // Inside tag, looking for attributes
        AttributeName,      // Reading attribute name
        AttributeEquals,    // Just saw =
        AttributeValueStart,// About to read attribute value
        AttributeValueDQ,   // Inside double-quoted attribute value
        AttributeValueSQ,   // Inside single-quoted attribute value
        Comment,            // Inside <!-- comment -->
        CommentEnd1,        // Saw - in comment
        CommentEnd2,        // Saw -- in comment
        CDATA,              // Inside <![CDATA[...]]>
        CDATAEnd1,          // Saw ] in CDATA
        CDATAEnd2,          // Saw ]] in CDATA
        XMLDecl,            // Inside <?xml ... ?>
        XMLDeclEnd          // Saw ? in XML declaration
    };
    
    State state = State::Normal;
    int rangeStart = 0;
    int tagStart = 0;
    int attrNameStart = 0;
    int attrValueStart = 0;
    wchar_t quoteChar = 0;
    
    // Helper to add a range
    auto addRange = [&](int start, int end, COLORREF color, bool bold = false, bool italic = false) {
        if (start < end && start >= 0 && end <= static_cast<int>(text.length())) {
            TextRange range;
            range.start = start;
            range.end = end;
            range.color = color;
            range.bold = bold;
            range.italic = italic;
            ranges.push_back(range);
        }
    };
    
    for (int i = 0; i < static_cast<int>(text.length()); ++i) {
        wchar_t ch = text[i];
        wchar_t nextCh = (i + 1 < static_cast<int>(text.length())) ? text[i + 1] : L'\0';
        
        switch (state) {
            case State::Normal:
                if (ch == L'<') {
                    // Check for special sequences
                    if (i + 3 < static_cast<int>(text.length()) && 
                        text.substr(i, 4) == L"<!--") {
                        state = State::Comment;
                        rangeStart = i;
                    }
                    else if (i + 8 < static_cast<int>(text.length()) && 
                             text.substr(i, 9) == L"<![CDATA[") {
                        state = State::CDATA;
                        rangeStart = i;
                    }
                    else if (nextCh == L'?') {
                        state = State::XMLDecl;
                        rangeStart = i;
                    }
                    else {
                        state = State::TagStart;
                        tagStart = i;
                        // Add opening bracket
                        addRange(i, i + 1, isDarkTheme ? HighlightColors::Dark::tag : HighlightColors::Light::tag);
                    }
                }
                break;
                
            case State::TagStart:
                if (ch == L'/' || ::iswalpha(ch) || ch == L'_') {
                    state = State::TagName;
                    rangeStart = (ch == L'/') ? i + 1 : i;
                }
                else if (ch == L'>') {
                    // Empty tag <>
                    addRange(i, i + 1, isDarkTheme ? HighlightColors::Dark::tag : HighlightColors::Light::tag);
                    state = State::Normal;
                }
                else {
                    state = State::Normal; // Invalid tag
                }
                break;
                
            case State::TagName:
                if (::iswalnum(ch) || ch == L':' || ch == L'-' || ch == L'.' || ch == L'_') {
                    // Continue reading tag name
                }
                else if (::iswspace(ch)) {
                    // End of tag name
                    addRange(rangeStart, i, isDarkTheme ? HighlightColors::Dark::tag : HighlightColors::Light::tag, true);
                    state = State::InTag;
                }
                else if (ch == L'>') {
                    // End of tag
                    addRange(rangeStart, i, isDarkTheme ? HighlightColors::Dark::tag : HighlightColors::Light::tag, true);
                    addRange(i, i + 1, isDarkTheme ? HighlightColors::Dark::tag : HighlightColors::Light::tag);
                    state = State::Normal;
                }
                else if (ch == L'/' && nextCh == L'>') {
                    // Self-closing tag
                    addRange(rangeStart, i, isDarkTheme ? HighlightColors::Dark::tag : HighlightColors::Light::tag, true);
                    addRange(i, i + 2, isDarkTheme ? HighlightColors::Dark::tag : HighlightColors::Light::tag);
                    i++; // Skip the >
                    state = State::Normal;
                }
                break;
                
            case State::InTag:
                if (::iswalpha(ch) || ch == L'_') {
                    state = State::AttributeName;
                    attrNameStart = i;
                }
                else if (ch == L'>') {
                    addRange(i, i + 1, isDarkTheme ? HighlightColors::Dark::tag : HighlightColors::Light::tag);
                    state = State::Normal;
                }
                else if (ch == L'/' && nextCh == L'>') {
                    addRange(i, i + 2, isDarkTheme ? HighlightColors::Dark::tag : HighlightColors::Light::tag);
                    i++; // Skip the >
                    state = State::Normal;
                }
                // Skip whitespace
                break;
                
            case State::AttributeName:
                if (::iswalnum(ch) || ch == L':' || ch == L'-' || ch == L'.' || ch == L'_') {
                    // Continue reading attribute name
                }
                else if (ch == L'=') {
                    addRange(attrNameStart, i, isDarkTheme ? HighlightColors::Dark::attribute : HighlightColors::Light::attribute);
                    state = State::AttributeEquals;
                }
                else if (::iswspace(ch)) {
                    addRange(attrNameStart, i, isDarkTheme ? HighlightColors::Dark::attribute : HighlightColors::Light::attribute);
                    state = State::InTag; // Attribute without value
                }
                else if (ch == L'>') {
                    addRange(attrNameStart, i, isDarkTheme ? HighlightColors::Dark::attribute : HighlightColors::Light::attribute);
                    addRange(i, i + 1, isDarkTheme ? HighlightColors::Dark::tag : HighlightColors::Light::tag);
                    state = State::Normal;
                }
                break;
                
            case State::AttributeEquals:
                if (ch == L'"') {
                    state = State::AttributeValueDQ;
                    attrValueStart = i;
                }
                else if (ch == L'\'') {
                    state = State::AttributeValueSQ;
                    attrValueStart = i;
                }
                else if (!::iswspace(ch)) {
                    // Unquoted attribute value (not standard but handle it)
                    state = State::InTag;
                }
                break;
                
            case State::AttributeValueDQ:
                if (ch == L'"') {
                    addRange(attrValueStart, i + 1, isDarkTheme ? HighlightColors::Dark::string : HighlightColors::Light::string);
                    state = State::InTag;
                }
                break;
                
            case State::AttributeValueSQ:
                if (ch == L'\'') {
                    addRange(attrValueStart, i + 1, isDarkTheme ? HighlightColors::Dark::string : HighlightColors::Light::string);
                    state = State::InTag;
                }
                break;
                
            case State::Comment:
                if (ch == L'-' && nextCh == L'-') {
                    state = State::CommentEnd1;
                }
                break;
                
            case State::CommentEnd1:
                if (ch == L'-') {
                    state = State::CommentEnd2;
                }
                else {
                    state = State::Comment;
                }
                break;
                
            case State::CommentEnd2:
                if (ch == L'>') {
                    addRange(rangeStart, i + 1, isDarkTheme ? HighlightColors::Dark::comment : HighlightColors::Light::comment, false, true);
                    state = State::Normal;
                }
                else if (ch != L'-') {
                    state = State::Comment;
                }
                break;
                
            case State::CDATA:
                if (ch == L']' && nextCh == L']') {
                    state = State::CDATAEnd1;
                }
                break;
                
            case State::CDATAEnd1:
                if (ch == L']') {
                    state = State::CDATAEnd2;
                }
                else {
                    state = State::CDATA;
                }
                break;
                
            case State::CDATAEnd2:
                if (ch == L'>') {
                    addRange(rangeStart, i + 1, isDarkTheme ? HighlightColors::Dark::string : HighlightColors::Light::string);
                    state = State::Normal;
                }
                else if (ch != L']') {
                    state = State::CDATA;
                }
                break;
                
            case State::XMLDecl:
                if (ch == L'?' && nextCh == L'>') {
                    state = State::XMLDeclEnd;
                }
                break;
                
            case State::XMLDeclEnd:
                if (ch == L'>') {
                    addRange(rangeStart, i + 1, isDarkTheme ? HighlightColors::Dark::comment : HighlightColors::Light::comment, false, true);
                    state = State::Normal;
                }
                break;
        }
    }
    
    // Handle any unclosed elements at end of text
    switch (state) {
        case State::TagName:
            addRange(rangeStart, static_cast<int>(text.length()), isDarkTheme ? HighlightColors::Dark::tag : HighlightColors::Light::tag, true);
            break;
        case State::AttributeName:
            addRange(attrNameStart, static_cast<int>(text.length()), isDarkTheme ? HighlightColors::Dark::attribute : HighlightColors::Light::attribute);
            break;
        case State::AttributeValueDQ:
        case State::AttributeValueSQ:
            addRange(attrValueStart, static_cast<int>(text.length()), isDarkTheme ? HighlightColors::Dark::string : HighlightColors::Light::string);
            break;
        case State::Comment:
        case State::CommentEnd1:
        case State::CommentEnd2:
            addRange(rangeStart, static_cast<int>(text.length()), isDarkTheme ? HighlightColors::Dark::comment : HighlightColors::Light::comment, false, true);
            break;
        case State::CDATA:
        case State::CDATAEnd1:
        case State::CDATAEnd2:
            addRange(rangeStart, static_cast<int>(text.length()), isDarkTheme ? HighlightColors::Dark::string : HighlightColors::Light::string);
            break;
        case State::XMLDecl:
        case State::XMLDeclEnd:
            addRange(rangeStart, static_cast<int>(text.length()), isDarkTheme ? HighlightColors::Dark::comment : HighlightColors::Light::comment, false, true);
            break;
        default:
            break;
    }
    
    return ranges;
}

std::string SyntaxHighlighter::GenerateRTFForSQL(const std::wstring& text, bool isDarkTheme, bool showLineNumbers)
{
    // Line numbers are now handled separately in the UI, not in RTF generation
    // Debug logging
    {
        std::wofstream log(L"C:\\Dev\\DebugViewPP\\syntax_highlighter_debug.log", std::ios::app);
        if (log.is_open()) {
            log << L"GenerateRTFForSQL called with text length: " << text.length() << std::endl;
            log << L"First 100 chars: " << text.substr(0, 100) << std::endl;
            log.close();
        }
    }
    
    // Format the SQL text with proper indentation and line breaks
    // Call the global FormatSQL from SqlFormatter.cpp, not the member function
    const std::wstring formattedText = fusion::debugviewpp::FormatSQL(text);
    
    // RTF header with color table
    std::string rtf = "{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0\\fmodern\\fcharset0 Consolas;}}";
    
    // Color table for SQL syntax highlighting
    if (isDarkTheme)
    {
        rtf += "{\\colortbl;";
        rtf += "\\red86\\green156\\blue214;";     // cf1 - Keywords (light blue)
        rtf += "\\red206\\green145\\blue120;";    // cf2 - Strings (light brown)
        rtf += "\\red181\\green206\\blue168;";    // cf3 - Numbers (light green)
        rtf += "\\red255\\green160\\blue122;";    // cf4 - Functions (orange)
        rtf += "\\red106\\green153\\blue85;";     // cf5 - Comments (green-gray)
        rtf += "\\red220\\green220\\blue220;";    // cf6 - Normal text (light gray)
        rtf += "}";
    }
    else
    {
        rtf += "{\\colortbl;";
        rtf += "\\red0\\green0\\blue255;";        // cf1 - Keywords (blue)
        rtf += "\\red255\\green0\\blue0;";        // cf2 - Strings (red)
        rtf += "\\red0\\green128\\blue0;";        // cf3 - Numbers (green)
        rtf += "\\red255\\green128\\blue0;";      // cf4 - Functions (orange)
        rtf += "\\red128\\green128\\blue128;";    // cf5 - Comments (gray)
        rtf += "\\red0\\green0\\blue0;";          // cf6 - Normal text (black)
        rtf += "}";
    }
    
    // Set default font and color
    rtf += "\\f0\\fs18\\cf6 ";
    
    // SQL keywords (case-insensitive)
    const std::vector<std::wstring> keywords = {
        L"SELECT", L"FROM", L"WHERE", L"JOIN", L"LEFT", L"RIGHT", L"INNER", L"OUTER", L"FULL",
        L"ON", L"AS", L"INSERT", L"INTO", L"VALUES", L"UPDATE", L"SET", L"DELETE",
        L"CREATE", L"DROP", L"ALTER", L"TABLE", L"INDEX", L"VIEW", L"DATABASE",
        L"AND", L"OR", L"NOT", L"IN", L"EXISTS", L"BETWEEN", L"LIKE", L"IS", L"NULL",
        L"ORDER", L"BY", L"GROUP", L"HAVING", L"LIMIT", L"OFFSET", L"UNION", L"ALL",
        L"DISTINCT", L"CASE", L"WHEN", L"THEN", L"ELSE", L"END", L"BEGIN", L"COMMIT",
        L"ROLLBACK", L"TRANSACTION", L"DECLARE", L"EXEC", L"EXECUTE"
    };
    
    // SQL functions
    const std::vector<std::wstring> functions = {
        L"COUNT", L"SUM", L"AVG", L"MIN", L"MAX", L"ROUND", L"FLOOR", L"CEIL",
        L"CONCAT", L"SUBSTRING", L"LENGTH", L"UPPER", L"LOWER", L"TRIM", L"REPLACE",
        L"NOW", L"GETDATE", L"DATEADD", L"DATEDIFF", L"CAST", L"CONVERT", L"COALESCE"
    };
    
    // Convert keywords and functions to uppercase for comparison
    std::wstring upperText = formattedText;
    std::transform(upperText.begin(), upperText.end(), upperText.begin(), ::towupper);
    
    enum class State {
        Normal,
        InString,
        InComment,
        InNumber
    };
    
    State state = State::Normal;
    
    for (size_t i = 0; i < formattedText.length(); ++i)
    {
        wchar_t ch = formattedText[i];
        
        switch (state)
        {
            case State::Normal:
                if (ch == L'\'')
                {
                    // Start of string
                    state = State::InString;
                    rtf += "{\\cf2 '";
                }
                else if (ch == L'-' && i + 1 < formattedText.length() && formattedText[i + 1] == L'-')
                {
                    // SQL comment
                    state = State::InComment;
                    rtf += "{\\cf5\\i --";
                    i++;
                }
                else if ((ch >= L'0' && ch <= L'9'))
                {
                    // Number
                    state = State::InNumber;
                    rtf += "{\\cf3 ";
                    rtf += static_cast<char>(ch);
                }
                else if (::iswalpha(ch) || ch == L'_')
                {
                    // Possible keyword or function
                    size_t wordEnd = i;
                    while (wordEnd < formattedText.length() && (::iswalnum(formattedText[wordEnd]) || formattedText[wordEnd] == L'_'))
                    {
                        wordEnd++;
                    }
                    
                    std::wstring word = upperText.substr(i, wordEnd - i);
                    std::wstring originalWord = formattedText.substr(i, wordEnd - i);
                    
                    bool isKeyword = false;
                    bool isFunction = false;
                    
                    // Check if it's a keyword
                    for (const auto& kw : keywords)
                    {
                        if (word == kw)
                        {
                            isKeyword = true;
                            break;
                        }
                    }
                    
                    // Check if it's a function
                    if (!isKeyword)
                    {
                        for (const auto& fn : functions)
                        {
                            if (word == fn)
                            {
                                isFunction = true;
                                break;
                            }
                        }
                    }
                    
                    if (isKeyword)
                    {
                        rtf += "{\\cf1\\b ";
                        for (wchar_t wc : originalWord)
                        {
                            rtf += static_cast<char>(wc);
                        }
                        rtf += "}";
                    }
                    else if (isFunction)
                    {
                        rtf += "{\\cf4 ";
                        for (wchar_t wc : originalWord)
                        {
                            rtf += static_cast<char>(wc);
                        }
                        rtf += "}";
                    }
                    else
                    {
                        for (wchar_t wc : originalWord)
                        {
                            if (wc == L'{' || wc == L'}' || wc == L'\\')
                                rtf += "\\";
                            rtf += static_cast<char>(wc);
                        }
                    }
                    
                    i = wordEnd - 1;
                }
                else if (ch == L'\n')
                {
                    rtf += "\\par ";
                }
                else if (ch == L'\r')
                {
                    // Skip
                }
                else if (ch == L'\t')
                {
                    rtf += "\\tab ";
                }
                else if (ch == L'{' || ch == L'}' || ch == L'\\')
                {
                    rtf += "\\";
                    rtf += static_cast<char>(ch);
                }
                else
                {
                    rtf += static_cast<char>(ch);
                }
                break;
                
            case State::InString:
                if (ch == L'\'')
                {
                    // Check for escaped quote
                    if (i + 1 < formattedText.length() && formattedText[i + 1] == L'\'')
                    {
                        rtf += "''";
                        i++;
                    }
                    else
                    {
                        // End of string
                        rtf += "'}";
                        state = State::Normal;
                    }
                }
                else if (ch == L'{' || ch == L'}' || ch == L'\\')
                {
                    rtf += "\\";
                    rtf += static_cast<char>(ch);
                }
                else
                {
                    rtf += static_cast<char>(ch);
                }
                break;
                
            case State::InComment:
                if (ch == L'\n')
                {
                    rtf += "}\\par ";
                    state = State::Normal;
                }
                else if (ch == L'{' || ch == L'}' || ch == L'\\')
                {
                    rtf += "\\";
                    rtf += static_cast<char>(ch);
                }
                else
                {
                    rtf += static_cast<char>(ch);
                }
                break;
                
            case State::InNumber:
                if ((ch >= L'0' && ch <= L'9') || ch == L'.')
                {
                    rtf += static_cast<char>(ch);
                }
                else
                {
                    rtf += "}";
                    state = State::Normal;
                    i--; // Re-process this character
                }
                break;
        }
    }
    
    // Close any open formatting
    if (state == State::InString || state == State::InComment || state == State::InNumber)
    {
        rtf += "}";
    }
    
    rtf += "}"; // Close RTF document
    return rtf;
}

std::vector<TextRange> SyntaxHighlighter::ParseSQL(const std::wstring& text, bool isDarkTheme)
{
    // Placeholder for SQL parsing (not used anymore)
    return {};
}

std::string SyntaxHighlighter::GenerateRTFForLogLevel(const std::wstring& text, bool isDarkTheme, bool showLineNumbers)
{
    // Line numbers are now handled separately in the UI, not in RTF generation
    // RTF header with color table
    std::string rtf = "{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0\\fmodern\\fcharset0 Consolas;}}";
    
    // Color table for log level highlighting
    if (isDarkTheme)
    {
        rtf += "{\\colortbl;";
        rtf += "\\red255\\green128\\blue128;";    // cf1 - Error (light red)
        rtf += "\\red255\\green200\\blue128;";    // cf2 - Warning (light orange)
        rtf += "\\red128\\green200\\blue255;";    // cf3 - Info (light blue)
        rtf += "\\red128\\green255\\blue128;";    // cf4 - Debug (light green)
        rtf += "\\red180\\green180\\blue180;";    // cf5 - Trace (light gray)
        rtf += "\\red220\\green220\\blue220;";    // cf6 - Normal text (light gray)
        rtf += "}";
    }
    else
    {
        rtf += "{\\colortbl;";
        rtf += "\\red255\\green0\\blue0;";        // cf1 - Error (red)
        rtf += "\\red255\\green128\\blue0;";      // cf2 - Warning (orange)
        rtf += "\\red0\\green0\\blue255;";        // cf3 - Info (blue)
        rtf += "\\red0\\green128\\blue0;";        // cf4 - Debug (green)
        rtf += "\\red128\\green128\\blue128;";    // cf5 - Trace (gray)
        rtf += "\\red0\\green0\\blue0;";          // cf6 - Normal text (black)
        rtf += "}";
    }
    
    // Set default font and color
    rtf += "\\f0\\fs18\\cf6 ";
    
    // Process the text
    for (size_t i = 0; i < text.length(); ++i)
    {
        // Check for bracketed log levels like [ERROR]
        if (text[i] == L'[' && i + 2 < text.length())
        {
            size_t closeBracket = text.find(L']', i + 1);
            if (closeBracket != std::wstring::npos && closeBracket - i <= 10)
            {
                std::wstring bracketContent = text.substr(i + 1, closeBracket - i - 1);
                std::wstring upperBracket = bracketContent;
                std::transform(upperBracket.begin(), upperBracket.end(), upperBracket.begin(), ::towupper);
                
                int colorIndex = 0;
                if (upperBracket == L"ERROR" || upperBracket == L"FATAL" || upperBracket == L"CRITICAL")
                    colorIndex = 1;
                else if (upperBracket == L"WARN" || upperBracket == L"WARNING")
                    colorIndex = 2;
                else if (upperBracket == L"INFO")
                    colorIndex = 3;
                else if (upperBracket == L"DEBUG")
                    colorIndex = 4;
                else if (upperBracket == L"TRACE")
                    colorIndex = 5;
                
                if (colorIndex > 0)
                {
                    // Output the opening bracket
                    rtf += "[";
                    // Output the colored level text
                    rtf += "{\\cf" + std::to_string(colorIndex) + "\\b ";
                    for (size_t j = i + 1; j < closeBracket; ++j)
                    {
                        wchar_t ch = text[j];
                        if (ch < 128)
                            rtf += static_cast<char>(ch);
                        else
                            rtf += "\\u" + std::to_string(static_cast<int>(ch)) + "?";
                    }
                    rtf += "}";
                    // Output the closing bracket
                    rtf += "]";
                    i = closeBracket;
                    continue;
                }
            }
        }
        
        // Look for log level keywords at word boundaries (without brackets)
        if (i == 0 || !::iswalnum(text[i-1]))
        {
            std::wstring remaining = text.substr(i);
            std::wstring upperRemaining = remaining;
            std::transform(upperRemaining.begin(), upperRemaining.end(), upperRemaining.begin(), ::towupper);
            
            bool found = false;
            std::wstring level;
            int colorIndex = 0;
            
            // Check for error levels
            if (upperRemaining.find(L"ERROR") == 0 || upperRemaining.find(L"FATAL") == 0 || 
                upperRemaining.find(L"CRITICAL") == 0)
            {
                if (upperRemaining.find(L"ERROR") == 0) level = L"ERROR";
                else if (upperRemaining.find(L"FATAL") == 0) level = L"FATAL";
                else level = L"CRITICAL";
                colorIndex = 1;
                found = true;
            }
            // Check for warning levels
            else if (upperRemaining.find(L"WARN") == 0 || upperRemaining.find(L"WARNING") == 0)
            {
                level = (upperRemaining.find(L"WARNING") == 0) ? L"WARNING" : L"WARN";
                colorIndex = 2;
                found = true;
            }
            // Check for info level
            else if (upperRemaining.find(L"INFO") == 0)
            {
                level = L"INFO";
                colorIndex = 3;
                found = true;
            }
            // Check for debug level
            else if (upperRemaining.find(L"DEBUG") == 0)
            {
                level = L"DEBUG";
                colorIndex = 4;
                found = true;
            }
            // Check for trace level
            else if (upperRemaining.find(L"TRACE") == 0)
            {
                level = L"TRACE";
                colorIndex = 5;
                found = true;
            }
            
            if (found)
            {
                // Make sure it's a complete word (not part of another word)
                size_t levelLen = level.length();
                if (i + levelLen >= text.length() || !::iswalnum(text[i + levelLen]))
                {
                    rtf += "{\\cf" + std::to_string(colorIndex) + "\\b ";
                    for (size_t j = 0; j < levelLen; ++j)
                    {
                        rtf += static_cast<char>(text[i + j]);
                    }
                    rtf += "}";
                    i += levelLen - 1;
                    continue;
                }
            }
        }
        
        // Normal character processing
        wchar_t ch = text[i];
        if (ch == L'\n')
        {
            rtf += "\\par ";
        }
        else if (ch == L'\r')
        {
            // Skip
        }
        else if (ch == L'\t')
        {
            rtf += "\\tab ";
        }
        else if (ch == L'{' || ch == L'}' || ch == L'\\')
        {
            rtf += "\\";
            rtf += static_cast<char>(ch);
        }
        else if (ch < 128)
        {
            rtf += static_cast<char>(ch);
        }
        else
        {
            rtf += "\\u" + std::to_string(static_cast<int>(ch)) + "?";
        }
    }
    
    rtf += "}"; // Close RTF document
    return rtf;
}

std::string SyntaxHighlighter::GenerateRTFForStackTrace(const std::wstring& text, bool isDarkTheme, bool showLineNumbers)
{
    // Line numbers are now handled separately in the UI, not in RTF generation
    // RTF header with color table
    std::string rtf = "{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0\\fmodern\\fcharset0 Consolas;}}";
    
    // Color table for stack trace highlighting
    if (isDarkTheme)
    {
        rtf += "{\\colortbl;";
        rtf += "\\red255\\green128\\blue128;";    // cf1 - Exception names (light red)
        rtf += "\\red86\\green156\\blue214;";     // cf2 - File paths (light blue)
        rtf += "\\red181\\green206\\blue168;";    // cf3 - Line numbers (light green)
        rtf += "\\red220\\green220\\blue170;";    // cf4 - Method names (light yellow)
        rtf += "\\red220\\green220\\blue220;";    // cf5 - Normal text (light gray)
        rtf += "}";
    }
    else
    {
        rtf += "{\\colortbl;";
        rtf += "\\red255\\green0\\blue0;";        // cf1 - Exception names (red)
        rtf += "\\red0\\green0\\blue255;";        // cf2 - File paths (blue)
        rtf += "\\red0\\green128\\blue0;";        // cf3 - Line numbers (green)
        rtf += "\\red128\\green0\\blue128;";      // cf4 - Method names (purple)
        rtf += "\\red0\\green0\\blue0;";          // cf5 - Normal text (black)
        rtf += "}";
    }
    
    // Set default font and color
    rtf += "\\f0\\fs18\\cf5 ";
    
    // Simple stack trace highlighting
    // Look for patterns like:
    // - "at MethodName" 
    // - "Exception" in class names
    // - File paths with extensions
    // - Line numbers after colons
    
    for (size_t i = 0; i < text.length(); ++i)
    {
        wchar_t ch = text[i];
        
        // Look for "at " pattern (method names in stack traces)
        if (i + 3 < text.length() && text.substr(i, 3) == L"at ")
        {
            rtf += "at ";
            i += 2;
            
            // Highlight the method name that follows
            size_t methodEnd = i + 1;
            while (methodEnd < text.length() && text[methodEnd] != L'(' && text[methodEnd] != L'\n')
            {
                methodEnd++;
            }
            
            if (methodEnd > i + 1)
            {
                rtf += "{\\cf4 ";
                for (size_t j = i + 1; j < methodEnd; ++j)
                {
                    wchar_t mc = text[j];
                    if (mc == L'{' || mc == L'}' || mc == L'\\')
                        rtf += "\\";
                    rtf += static_cast<char>(mc);
                }
                rtf += "}";
                i = methodEnd - 1;
            }
        }
        // Look for "Exception" in words
        else if (i == 0 || !::iswalnum(text[i-1]))
        {
            size_t wordEnd = i;
            while (wordEnd < text.length() && (::iswalnum(text[wordEnd]) || text[wordEnd] == L'.'))
            {
                wordEnd++;
            }
            
            std::wstring word = text.substr(i, wordEnd - i);
            
            if (word.find(L"Exception") != std::wstring::npos || 
                word.find(L"Error") != std::wstring::npos)
            {
                rtf += "{\\cf1\\b ";
                for (wchar_t wc : word)
                {
                    if (wc == L'{' || wc == L'}' || wc == L'\\')
                        rtf += "\\";
                    rtf += static_cast<char>(wc);
                }
                rtf += "}";
                i = wordEnd - 1;
            }
            // Look for file paths (contains . and / or \)
            else if (word.find(L'.') != std::wstring::npos && 
                     (word.find(L'/') != std::wstring::npos || word.find(L'\\') != std::wstring::npos))
            {
                rtf += "{\\cf2 ";
                for (wchar_t wc : word)
                {
                    if (wc == L'{' || wc == L'}' || wc == L'\\')
                        rtf += "\\";
                    rtf += static_cast<char>(wc);
                }
                rtf += "}";
                i = wordEnd - 1;
            }
            // Look for line numbers (: followed by digits)
            else if (ch == L':' && i + 1 < text.length() && ::iswdigit(text[i + 1]))
            {
                rtf += ":";
                i++;
                size_t numEnd = i;
                while (numEnd < text.length() && ::iswdigit(text[numEnd]))
                {
                    numEnd++;
                }
                
                rtf += "{\\cf3 ";
                for (size_t j = i; j < numEnd; ++j)
                {
                    rtf += static_cast<char>(text[j]);
                }
                rtf += "}";
                i = numEnd - 1;
            }
            else
            {
                // Normal character
                if (ch == L'\n')
                    rtf += "\\par ";
                else if (ch == L'\r')
                    ; // Skip
                else if (ch == L'\t')
                    rtf += "\\tab ";
                else if (ch == L'{' || ch == L'}' || ch == L'\\')
                {
                    rtf += "\\";
                    rtf += static_cast<char>(ch);
                }
                else if (ch < 128)
                    rtf += static_cast<char>(ch);
                else
                    rtf += "\\u" + std::to_string(static_cast<int>(ch)) + "?";
            }
        }
        else
        {
            // Normal character
            if (ch == L'\n')
                rtf += "\\par ";
            else if (ch == L'\r')
                ; // Skip
            else if (ch == L'\t')
                rtf += "\\tab ";
            else if (ch == L'{' || ch == L'}' || ch == L'\\')
            {
                rtf += "\\";
                rtf += static_cast<char>(ch);
            }
            else if (ch < 128)
                rtf += static_cast<char>(ch);
            else
                rtf += "\\u" + std::to_string(static_cast<int>(ch)) + "?";
        }
    }
    
    rtf += "}"; // Close RTF document
    return rtf;
}

std::vector<TextRange> SyntaxHighlighter::ParseStackTrace(const std::wstring& text, bool isDarkTheme)
{
    // Placeholder for stack trace parsing (not used anymore)
    return {};
}

std::vector<TextRange> SyntaxHighlighter::ParseLogLevel(const std::wstring& text, bool isDarkTheme)
{
    std::vector<TextRange> ranges;
    
    // Find log levels and color them appropriately
    std::wregex levelPattern(L"\\b(ERROR|FATAL|CRITICAL)\\b", std::regex_constants::icase);
    std::wsregex_iterator it(text.begin(), text.end(), levelPattern);
    std::wsregex_iterator end;
    
    while (it != end)
    {
        TextRange range;
        range.start = static_cast<int>(it->position());
        range.end = range.start + static_cast<int>(it->length());
        range.color = isDarkTheme ? HighlightColors::Dark::error : HighlightColors::Light::error;
        range.bold = true;
        ranges.push_back(range);
        ++it;
    }
    
    // Warning levels
    levelPattern = std::wregex(L"\\b(WARN|WARNING)\\b", std::regex_constants::icase);
    it = std::wsregex_iterator(text.begin(), text.end(), levelPattern);
    
    while (it != end)
    {
        TextRange range;
        range.start = static_cast<int>(it->position());
        range.end = range.start + static_cast<int>(it->length());
        range.color = isDarkTheme ? HighlightColors::Dark::warning : HighlightColors::Light::warning;
        range.bold = true;
        ranges.push_back(range);
        ++it;
    }
    
    // Info levels
    levelPattern = std::wregex(L"\\b(INFO|DEBUG|TRACE)\\b", std::regex_constants::icase);
    it = std::wsregex_iterator(text.begin(), text.end(), levelPattern);
    
    while (it != end)
    {
        TextRange range;
        range.start = static_cast<int>(it->position());
        range.end = range.start + static_cast<int>(it->length());
        range.color = isDarkTheme ? HighlightColors::Dark::info : HighlightColors::Light::info;
        ranges.push_back(range);
        ++it;
    }
    
    return ranges;
}

void SyntaxHighlighter::ApplyRangeFormat(HWND hRichEdit, const TextRange& range)
{
    // Send messages to the RichEdit control to apply formatting
    ::SendMessage(hRichEdit, EM_SETSEL, range.start, range.end);
    
    CHARFORMAT2 cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = range.color;
    cf.dwEffects = 0;
    
    if (range.bold)
    {
        cf.dwMask |= CFM_BOLD;
        cf.dwEffects |= CFE_BOLD;
    }
    
    if (range.italic)
    {
        cf.dwMask |= CFM_ITALIC;
        cf.dwEffects |= CFE_ITALIC;
    }
    
    ::SendMessage(hRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

std::wstring SyntaxHighlighter::FormatJSON(const std::wstring& text)
{
    // Check if the text contains JSON embedded in a log message
    size_t jsonStart = text.find(L'{');
    size_t jsonArrayStart = text.find(L'[');
    
    // If there's no JSON structure, return original
    if (jsonStart == std::wstring::npos && jsonArrayStart == std::wstring::npos)
        return text;
    
    // Find the actual start of JSON (could be { or [)
    size_t actualStart = jsonStart;
    if (jsonArrayStart != std::wstring::npos && 
        (jsonStart == std::wstring::npos || jsonArrayStart < jsonStart))
    {
        actualStart = jsonArrayStart;
    }
    
    // Extract prefix (like "INFO: API Response: ")
    std::wstring prefix = text.substr(0, actualStart);
    std::wstring jsonContent = text.substr(actualStart);
    
    // Format the JSON part
    std::wstring result = prefix;  // Keep the prefix
    int indentLevel = 0;
    const std::wstring indent = L"  ";  // 2 spaces per level
    bool inString = false;
    bool escapeNext = false;
    
    for (size_t i = 0; i < jsonContent.length(); ++i)
    {
        wchar_t ch = jsonContent[i];
        wchar_t nextCh = (i + 1 < jsonContent.length()) ? jsonContent[i + 1] : L'\0';
        
        // Handle escape sequences
        if (escapeNext)
        {
            result += ch;
            escapeNext = false;
            continue;
        }
        
        if (ch == L'\\')
        {
            result += ch;
            escapeNext = true;
            continue;
        }
        
        // Handle strings
        if (ch == L'"' && !escapeNext)
        {
            inString = !inString;
            result += ch;
            continue;
        }
        
        if (inString)
        {
            result += ch;
            continue;
        }
        
        // Skip whitespace outside strings
        if (::iswspace(ch))
            continue;
        
        // Handle structural characters
        switch (ch)
        {
            case L'{':
            case L'[':
                result += ch;
                if (nextCh != L'}' && nextCh != L']')
                {
                    indentLevel++;
                    result += L'\n';
                    for (int j = 0; j < indentLevel; ++j)
                        result += indent;
                }
                break;
                
            case L'}':
            case L']':
                if (result.length() > 0 && result[result.length() - 1] != L'[' && result[result.length() - 1] != L'{')
                {
                    indentLevel--;
                    result += L'\n';
                    for (int j = 0; j < indentLevel; ++j)
                        result += indent;
                }
                result += ch;
                break;
                
            case L',':
                result += ch;
                result += L'\n';
                for (int j = 0; j < indentLevel; ++j)
                    result += indent;
                break;
                
            case L':':
                result += ch;
                result += L' ';
                break;
                
            default:
                result += ch;
                break;
        }
    }
    
    return result;
}

std::wstring SyntaxHighlighter::FormatXML(const std::wstring& text)
{
    // Check if the text contains XML embedded in a log message
    size_t xmlStart = text.find(L'<');
    
    // If there's no XML structure, return original
    if (xmlStart == std::wstring::npos)
        return text;
    
    // Check if it looks like an XML tag (not just a less-than comparison)
    bool isXML = false;
    if (xmlStart + 1 < text.length())
    {
        wchar_t nextChar = text[xmlStart + 1];
        // Check for valid XML tag start: <letter, </, <?, <!
        isXML = ::iswalpha(nextChar) || nextChar == L'/' || nextChar == L'?' || nextChar == L'!';
    }
    
    if (!isXML)
        return text;
    
    // Extract prefix and XML content
    std::wstring prefix = text.substr(0, xmlStart);
    std::wstring xmlContent = text.substr(xmlStart);
    
    // Format the XML part
    std::wstring result = prefix;  // Keep the prefix
    int indentLevel = 0;
    const std::wstring indent = L"  ";  // 2 spaces per level
    bool inTag = false;
    bool inClosingTag = false;
    bool inComment = false;
    bool inCDATA = false;
    size_t lastNewline = 0;
    
    for (size_t i = 0; i < xmlContent.length(); ++i)
    {
        wchar_t ch = xmlContent[i];
        
        // Handle comments
        if (!inComment && !inCDATA && i + 3 < xmlContent.length() && 
            xmlContent.substr(i, 4) == L"<!--")
        {
            // Add newline if not at start of line
            if (i > lastNewline && result.length() > 0 && result.back() != L'\n')
            {
                result += L'\n';
                for (int j = 0; j < indentLevel; ++j)
                    result += indent;
            }
            
            inComment = true;
            result += L"<!--";
            i += 3;
            continue;
        }
        
        if (inComment)
        {
            result += ch;
            if (i >= 2 && xmlContent.substr(i - 2, 3) == L"-->")
            {
                inComment = false;
                result += L'\n';
                lastNewline = result.length();
            }
            continue;
        }
        
        // Handle CDATA
        if (!inCDATA && i + 8 < xmlContent.length() && 
            xmlContent.substr(i, 9) == L"<![CDATA[")
        {
            if (result.length() > 0 && result.back() != L'\n')
            {
                result += L'\n';
                for (int j = 0; j < indentLevel; ++j)
                    result += indent;
            }
            
            inCDATA = true;
            result += L"<![CDATA[";
            i += 8;
            continue;
        }
        
        if (inCDATA)
        {
            result += ch;
            if (i >= 2 && xmlContent.substr(i - 2, 3) == L"]]>")
            {
                inCDATA = false;
            }
            continue;
        }
        
        // Handle tags
        if (ch == L'<' && !inTag)
        {
            // Check if it's a closing tag
            if (i + 1 < xmlContent.length() && xmlContent[i + 1] == L'/')
            {
                inClosingTag = true;
                indentLevel = std::max(0, indentLevel - 1);
            }
            
            // Add newline and indent before tag
            if (result.length() > 0 && result.back() != L'\n')
            {
                result += L'\n';
            }
            for (int j = 0; j < indentLevel; ++j)
                result += indent;
            
            inTag = true;
            result += ch;
        }
        else if (ch == L'>' && inTag)
        {
            result += ch;
            inTag = false;
            
            // Check if it's a self-closing tag
            bool selfClosing = (result.length() >= 2 && result[result.length() - 2] == L'/');
            
            if (!selfClosing && !inClosingTag)
            {
                indentLevel++;
            }
            
            inClosingTag = false;
            
            // Don't add newline after declaration or processing instruction
            if (!(result.length() >= 2 && result[result.length() - 2] == L'?'))
            {
                lastNewline = result.length() + 1;
            }
        }
        else if (!::iswspace(ch) || inTag)
        {
            result += ch;
        }
    }
    
    return result;
}

std::wstring SyntaxHighlighter::FormatSQL(const std::wstring& text)
{
    // Use the professional SQL formatter with fsqlf.dll

    // Call the global FormatSQL function from SqlFormatter.cpp which uses fsqlf.dll
    std::wstring formatted = fusion::debugviewpp::FormatSQL(text);


    return formatted;
}

std::wstring SyntaxHighlighter::FormatSQL(const std::wstring& text, const SqlFormatOptions& options)
{
    // Use the professional SQL formatter with fsqlf.dll and custom options

    // Call the global FormatSQL function from SqlFormatter.cpp which uses fsqlf.dll
    std::wstring formatted = fusion::debugviewpp::FormatSQL(text, options);


    return formatted;
}

std::wstring SyntaxHighlighter::FormatClarionEvent(const std::wstring& text)
{
    // The text should start with Event* (no [Events] prefix in the message itself)
    size_t startPos = text.find_first_not_of(L" \t\r\n");
    if (startPos == std::wstring::npos)
        return text;
    
    // Check if it starts with Event*
    if (text.compare(startPos, 6, L"Event*") != 0)
        return text;
    
    // Use the text from the start position
    std::wstring eventData = text.substr(startPos);
    
    // Build the formatted output
    std::wstring result;
    
    // Parse the Event field specially (it contains decimal, hex, and description)
    size_t eventFieldEnd = eventData.find(L',');
    if (eventFieldEnd != std::wstring::npos)
    {
        std::wstring eventField = eventData.substr(0, eventFieldEnd);
        
        // Parse Event*258  102h Selecting format
        size_t starPos = eventField.find(L'*');
        if (starPos != std::wstring::npos && starPos + 1 < eventField.length())
        {
            // Extract the value part after the asterisk
            std::wstring eventValue = eventField.substr(starPos + 1);
            
            // Find the decimal value (first number)
            size_t decimalEnd = 0;
            while (decimalEnd < eventValue.length() && std::iswdigit(eventValue[decimalEnd]))
                decimalEnd++;
            
            std::wstring decimal = eventValue.substr(0, decimalEnd);
            
            // Skip whitespace
            size_t hexStart = decimalEnd;
            while (hexStart < eventValue.length() && std::iswspace(eventValue[hexStart]))
                hexStart++;
            
            // Find the hex value and description
            std::wstring hexAndDesc = eventValue.substr(hexStart);
            size_t hexEnd = 0;
            while (hexEnd < hexAndDesc.length() && 
                   (std::iswalnum(hexAndDesc[hexEnd]) || hexAndDesc[hexEnd] == L'h'))
                hexEnd++;
            
            std::wstring hex = hexAndDesc.substr(0, hexEnd);
            
            // Skip whitespace to get description
            size_t descStart = hexEnd;
            while (descStart < hexAndDesc.length() && std::iswspace(hexAndDesc[descStart]))
                descStart++;
            
            std::wstring description = hexAndDesc.substr(descStart);
            
            // Format the Event line with padding
            std::wstring eventLabel = L"Event:";
            while (eventLabel.length() < 15)
                eventLabel += L" ";

            result = eventLabel + decimal + L" (Decimal) / " + hex + L" (Hex)";
            if (!description.empty())
                result += L" - " + description;
            result += L"\r\n";
        }
        
        // Parse remaining fields
        size_t fieldStart = eventFieldEnd + 1;
        while (fieldStart < eventData.length())
        {
            // Skip leading whitespace
            while (fieldStart < eventData.length() && std::iswspace(eventData[fieldStart]))
                fieldStart++;
            
            // Find the next comma or end of string
            size_t fieldEnd = eventData.find(L',', fieldStart);
            if (fieldEnd == std::wstring::npos)
                fieldEnd = eventData.length();
            
            // Extract the field
            std::wstring field = eventData.substr(fieldStart, fieldEnd - fieldStart);
            
            // Remove trailing whitespace
            while (!field.empty() && std::iswspace(field.back()))
                field.pop_back();
            
            if (!field.empty())
            {
                // Parse Field*Value format
                size_t starPos = field.find(L'*');
                if (starPos != std::wstring::npos)
                {
                    std::wstring fieldName = field.substr(0, starPos);
                    std::wstring fieldValue = (starPos + 1 < field.length()) ? 
                                            field.substr(starPos + 1) : L"";
                    
                    // Handle empty values
                    if (fieldValue.empty() || fieldValue == L" ")
                        fieldValue = L"n/a";

                    // Format with padding for alignment (use 15 chars for label column)
                    std::wstring paddedName = fieldName + L":";
                    while (paddedName.length() < 15)
                        paddedName += L" ";

                    result += paddedName + fieldValue + L"\r\n";
                }
                else if (field.find(L'=') != std::wstring::npos || field.find(L' ') != std::wstring::npos)
                {
                    // Handle fields with = or space (like "Field*?RELTREE = 1")
                    result += field + L"\r\n";
                }
            }
            
            fieldStart = fieldEnd + 1;
        }
    }
    else
    {
        // No comma found, return original text
        return text;
    }
    
    // Remove trailing newline if present
    while (!result.empty() && (result.back() == L'\r' || result.back() == L'\n'))
        result.pop_back();
    
    return result;
}

std::wstring SyntaxHighlighter::FormatErrorMessage(const std::wstring& text)
{
    // Parse and format Error messages
    // Format: Message: <msg>,Errorcode: <code>,Error: <desc>,ErrorFile: <file>,FileError: <err>,FileErrorCode: <code>

    std::wstring result;
    size_t pos = 0;

    // Parse each key-value pair
    while (pos < text.length())
    {
        // Find the next key-value pair
        size_t colonPos = text.find(L':', pos);
        if (colonPos == std::wstring::npos) break;

        // Find the start of the key (skip any spaces/commas)
        size_t keyStart = pos;
        while (keyStart < colonPos && (text[keyStart] == L' ' || text[keyStart] == L','))
        {
            keyStart++;
        }

        std::wstring key = text.substr(keyStart, colonPos - keyStart);

        // Find the value (up to the next comma that precedes a key)
        size_t valueStart = colonPos + 1;
        while (valueStart < text.length() && text[valueStart] == L' ')
        {
            valueStart++;
        }

        // Find the end of the value
        size_t valueEnd = text.length();
        size_t nextKeyPos = valueStart;
        while (nextKeyPos < text.length())
        {
            size_t nextComma = text.find(L',', nextKeyPos);
            if (nextComma == std::wstring::npos) break;

            // Check if this comma is followed by a known key
            size_t afterComma = nextComma + 1;
            while (afterComma < text.length() && text[afterComma] == L' ')
            {
                afterComma++;
            }

            // Check for known keys
            if (afterComma < text.length())
            {
                if (text.substr(afterComma, 9) == L"Errorcode" ||
                    text.substr(afterComma, 5) == L"Error" ||
                    text.substr(afterComma, 9) == L"ErrorFile" ||
                    text.substr(afterComma, 9) == L"FileError" ||
                    text.substr(afterComma, 13) == L"FileErrorCode" ||
                    text.substr(afterComma, 7) == L"Message")
                {
                    valueEnd = nextComma;
                    break;
                }
            }
            nextKeyPos = nextComma + 1;
        }

        std::wstring value = text.substr(valueStart, valueEnd - valueStart);

        // Trim trailing spaces from value
        size_t lastNonSpace = value.find_last_not_of(L" ");
        if (lastNonSpace != std::wstring::npos)
        {
            value = value.substr(0, lastNonSpace + 1);
        }

        // Format as two-column layout with padding for alignment
        // Calculate padding for alignment (assume 20 chars for label column)
        size_t padLength = 20;
        if (key.length() < padLength)
        {
            key += std::wstring(padLength - key.length(), L' ');
        }

        result += key + L": " + value + L"\n";

        pos = valueEnd;
    }

    // Remove trailing newline if present
    if (!result.empty() && result.back() == L'\n')
    {
        result.pop_back();
    }

    return result;
}

std::wstring SyntaxHighlighter::FormatClarionGroup(const std::wstring& text)
{
    // Parse and format Clarion Group messages
    // Format: "Group Name",""|"FIELD1","Value1"|"FIELD2","Value2"|...

    std::wstring result;
    size_t pos = 0;

    // Skip leading whitespace
    while (pos < text.length() && std::iswspace(text[pos]))
        pos++;

    // Parse the group name (first quoted value)
    if (pos < text.length() && text[pos] == L'"')
    {
        pos++; // Skip opening quote
        size_t endQuote = text.find(L'"', pos);
        if (endQuote != std::wstring::npos)
        {
            std::wstring groupName = text.substr(pos, endQuote - pos);

            // Format group name with padding
            std::wstring label = L"Group:";
            while (label.length() < 15)
                label += L" ";
            result = label + groupName + L"\r\n";

            pos = endQuote + 1;

            // Skip comma and empty value after group name
            if (pos < text.length() && text[pos] == L',')
            {
                pos++;
                // Skip the empty value part ,""|
                size_t pipePos = text.find(L'|', pos);
                if (pipePos != std::wstring::npos)
                {
                    pos = pipePos + 1;

                    // Now parse field-value pairs
                    while (pos < text.length())
                    {
                        // Skip whitespace
                        while (pos < text.length() && std::iswspace(text[pos]))
                            pos++;

                        // Check for field name (starts with quote)
                        if (pos < text.length() && text[pos] == L'"')
                        {
                            pos++; // Skip opening quote
                            size_t fieldEndQuote = text.find(L'"', pos);
                            if (fieldEndQuote != std::wstring::npos)
                            {
                                std::wstring fieldName = text.substr(pos, fieldEndQuote - pos);
                                pos = fieldEndQuote + 1;

                                // Skip comma and get value
                                if (pos < text.length() && text[pos] == L',')
                                {
                                    pos++;

                                    // Get the value (also quoted)
                                    if (pos < text.length() && text[pos] == L'"')
                                    {
                                        pos++; // Skip opening quote
                                        size_t valueEndQuote = text.find(L'"', pos);
                                        if (valueEndQuote != std::wstring::npos)
                                        {
                                            std::wstring fieldValue = text.substr(pos, valueEndQuote - pos);
                                            pos = valueEndQuote + 1;

                                            // Format field-value pair with padding
                                            std::wstring paddedName = fieldName + L":";
                                            while (paddedName.length() < 15)
                                                paddedName += L" ";

                                            result += paddedName + fieldValue + L"\r\n";

                                            // Skip pipe separator if present
                                            if (pos < text.length() && text[pos] == L'|')
                                                pos++;
                                        }
                                        else
                                            break;
                                    }
                                    else
                                        break;
                                }
                                else
                                    break;
                            }
                            else
                                break;
                        }
                        else
                            break;
                    }
                }
            }
        }
    }

    // If parsing failed, return original text
    if (result.empty())
        return text;

    // Remove trailing newline
    while (!result.empty() && (result.back() == L'\r' || result.back() == L'\n'))
        result.pop_back();

    return result;
}

int SyntaxHighlighter::CountLines(const std::wstring& text)
{
    if (text.empty())
        return 0;
    
    int count = 1;
    for (wchar_t ch : text)
    {
        if (ch == L'\n')
            count++;
    }
    return count;
}

std::string SyntaxHighlighter::GenerateRTFWithLineNumbers(const std::wstring& text, const std::string& contentRtf, bool isDarkTheme)
{
    int lineCount = CountLines(text);
    int numWidth = std::max(3, static_cast<int>(std::to_string(lineCount).length()));
    
    // Create new RTF with line numbers
    std::string rtf = "{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0\\fmodern\\fcharset0 Consolas;}}";
    
    // Extract color table from original RTF and add line number colors
    size_t colorTableStart = contentRtf.find("{\\colortbl");
    size_t colorTableEnd = contentRtf.find("}", colorTableStart);
    
    if (colorTableStart != std::string::npos && colorTableEnd != std::string::npos)
    {
        // Extract existing color table
        std::string colorTable = contentRtf.substr(colorTableStart, colorTableEnd - colorTableStart);
        
        // Add line number colors to the end (before the closing brace)
        if (isDarkTheme)
        {
            colorTable += "\\red128\\green128\\blue128;";  // Line number color (gray)
            colorTable += "\\red60\\green60\\blue60;";     // Separator
        }
        else
        {
            colorTable += "\\red100\\green100\\blue100;";  // Line number color (gray)
            colorTable += "\\red200\\green200\\blue200;";  // Separator
        }
        colorTable += "}";
        
        rtf += colorTable;
    }
    else
    {
        // Create default color table
        if (isDarkTheme)
        {
            rtf += "{\\colortbl;";
            rtf += "\\red220\\green220\\blue220;";  // cf1 - Main text
            rtf += "\\red128\\green128\\blue128;";  // cf2 - Line numbers
            rtf += "\\red60\\green60\\blue60;";     // cf3 - Separator
            rtf += "}";
        }
        else
        {
            rtf += "{\\colortbl;";
            rtf += "\\red0\\green0\\blue0;";        // cf1 - Main text
            rtf += "\\red100\\green100\\blue100;";  // cf2 - Line numbers
            rtf += "\\red200\\green200\\blue200;";  // cf3 - Separator
            rtf += "}";
        }
    }
    
    rtf += "\\f0\\fs18 ";
    
    // Extract content from original RTF (skip header and color table)
    size_t contentStart = contentRtf.find("\\f0\\fs18");
    if (contentStart == std::string::npos)
    {
        contentStart = contentRtf.find("\\f0");
    }
    if (contentStart != std::string::npos)
    {
        contentStart = contentRtf.find(" ", contentStart) + 1;
    }
    else
    {
        contentStart = colorTableEnd + 1;
    }
    
    size_t contentEnd = contentRtf.rfind('}');
    std::string content = contentRtf.substr(contentStart, contentEnd - contentStart);
    
    // Count color references in original to determine offset for line number colors
    int colorCount = 6; // Default number of colors in most syntax highlighters
    size_t cfPos = 0;
    while ((cfPos = contentRtf.find("\\cf", cfPos)) != std::string::npos)
    {
        int cfNum = 0;
        if (sscanf(contentRtf.c_str() + cfPos + 3, "%d", &cfNum) == 1)
        {
            colorCount = std::max(colorCount, cfNum);
        }
        cfPos += 3;
    }
    
    // Add line numbers inline with the content
    int lineNum = 1;
    bool atLineStart = true;
    
    // Process the content and add line numbers
    for (size_t i = 0; i < content.length(); ++i)
    {
        if (atLineStart)
        {
            // Add line number
            rtf += "{\\cf" + std::to_string(colorCount + 1) + " ";
            std::string lineNumStr = std::to_string(lineNum++);
            while (lineNumStr.length() < numWidth)
            {
                lineNumStr = " " + lineNumStr;
            }
            rtf += lineNumStr;
            rtf += "}";
            rtf += "{\\cf" + std::to_string(colorCount + 2) + " | }";
            atLineStart = false;
        }
        
        // Check for RTF line break
        if (content.substr(i, 5) == "\\par ")
        {
            rtf += content.substr(i, 5);
            i += 4;
            atLineStart = true;
        }
        else
        {
            rtf += content[i];
        }
    }
    
    rtf += "}";
    return rtf;
}

std::string SyntaxHighlighter::WrapWithLineNumbers(const std::string& rtfContent, const std::wstring& text, bool isDarkTheme)
{
    int lineCount = CountLines(text);
    
    // Calculate width needed for line numbers (at least 3 digits)
    int numWidth = std::max(3, static_cast<int>(std::to_string(lineCount).length()));
    
    // Create RTF table with two columns: line numbers and content
    std::string rtf = "{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0\\fmodern\\fcharset0 Consolas;}}";
    
    // Color table for line numbers
    if (isDarkTheme)
    {
        rtf += "{\\colortbl;";
        rtf += "\\red128\\green128\\blue128;";  // cf1 - Line number color (gray)
        rtf += "\\red30\\green30\\blue30;";     // cf2 - Line number background
        rtf += "\\red60\\green60\\blue60;";     // cf3 - Separator line
        rtf += "}";
    }
    else
    {
        rtf += "{\\colortbl;";
        rtf += "\\red100\\green100\\blue100;";  // cf1 - Line number color (gray)
        rtf += "\\red240\\green240\\blue240;";  // cf2 - Line number background
        rtf += "\\red200\\green200\\blue200;";  // cf3 - Separator line
        rtf += "}";
    }
    
    rtf += "\\f0\\fs18 ";
    
    // Split the original text into lines to generate line numbers
    std::vector<std::wstring> lines;
    std::wstring currentLine;
    for (wchar_t ch : text)
    {
        if (ch == L'\n')
        {
            lines.push_back(currentLine);
            currentLine.clear();
        }
        else if (ch != L'\r')
        {
            currentLine += ch;
        }
    }
    if (!currentLine.empty() || (!lines.empty() && text.back() == L'\n'))
    {
        lines.push_back(currentLine);
    }
    
    // Extract the content part from the original RTF (skip header and color table)
    std::string contentRtf = rtfContent;
    size_t contentStart = contentRtf.find("\\f0\\fs");
    if (contentStart != std::string::npos)
    {
        contentStart = contentRtf.find(' ', contentStart) + 1;
    }
    else
    {
        contentStart = contentRtf.find("}\\f0") + 4;
        if (contentStart == std::string::npos + 4)
        {
            // Fallback: skip to after color table
            contentStart = contentRtf.rfind('}', contentRtf.find("{\\colortbl")) + 1;
        }
    }
    
    size_t contentEnd = contentRtf.rfind('}');
    std::string content = contentRtf.substr(contentStart, contentEnd - contentStart);
    
    // Build RTF with line numbers
    rtf += "{\\pard\\intbl ";
    
    // Create a table row for each line
    for (int i = 1; i <= lineCount; ++i)
    {
        // Line number cell
        rtf += "{\\cf1\\cb2 ";
        std::string lineNum = std::to_string(i);
        while (lineNum.length() < numWidth)
        {
            lineNum = " " + lineNum;
        }
        rtf += lineNum + " ";
        rtf += "}";
        
        // Separator
        rtf += "{\\cf3 | }";
        
        // Content for this line (we need to extract it from the RTF)
        // For now, we'll inject the full content and rely on RTF line breaks
        if (i == 1)
        {
            rtf += content;
        }
        
        if (i < lineCount)
        {
            rtf += "\\line ";
        }
    }
    
    rtf += "\\par}";
    rtf += "}";
    
    return rtf;
}

std::string SyntaxHighlighter::GenerateRTFForPlainText(const std::wstring& text, bool isDarkTheme, bool showLineNumbers)
{
    // RTF header with color table
    std::string rtf = "{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0\\fmodern\\fcharset0 Consolas;}}";
    
    // Color table
    if (isDarkTheme)
    {
        rtf += "{\\colortbl;\\red220\\green220\\blue220;}"; // Light gray text for dark mode
    }
    else
    {
        rtf += "{\\colortbl;\\red0\\green0\\blue0;}"; // Black text for light mode  
    }
    
    rtf += "\\f0\\fs18\\cf1 ";
    
    // If line numbers are requested, use a different approach
    if (showLineNumbers)
    {
        int lineCount = CountLines(text);
        int numWidth = std::max(3, static_cast<int>(std::to_string(lineCount).length()));
        
        // Add line number colors to color table
        std::string numberedRtf = "{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0\\fmodern\\fcharset0 Consolas;}}";
        
        if (isDarkTheme)
        {
            numberedRtf += "{\\colortbl;";
            numberedRtf += "\\red220\\green220\\blue220;";  // cf1 - Main text
            numberedRtf += "\\red128\\green128\\blue128;";  // cf2 - Line numbers
            numberedRtf += "\\red60\\green60\\blue60;";     // cf3 - Separator
            numberedRtf += "}";
        }
        else
        {
            numberedRtf += "{\\colortbl;";
            numberedRtf += "\\red0\\green0\\blue0;";        // cf1 - Main text
            numberedRtf += "\\red100\\green100\\blue100;";  // cf2 - Line numbers
            numberedRtf += "\\red200\\green200\\blue200;";  // cf3 - Separator
            numberedRtf += "}";
        }
        
        numberedRtf += "\\f0\\fs18 ";
        
        // Process text line by line
        int lineNum = 1;
        for (size_t i = 0; i < text.length(); ++i)
        {
            // Add line number at start of each line
            if (i == 0 || (i > 0 && text[i-1] == L'\n'))
            {
                numberedRtf += "{\\cf2 ";
                std::string lineNumStr = std::to_string(lineNum++);
                while (lineNumStr.length() < numWidth)
                {
                    lineNumStr = " " + lineNumStr;
                }
                numberedRtf += lineNumStr;
                numberedRtf += "}";
                numberedRtf += "{\\cf3  | }";
                numberedRtf += "\\cf1 ";
            }
            
            wchar_t ch = text[i];
            if (ch < 128)
            {
                switch (ch)
                {
                    case L'\\': numberedRtf += "\\\\"; break;
                    case L'{': numberedRtf += "\\{"; break;
                    case L'}': numberedRtf += "\\}"; break;
                    case L'\n': numberedRtf += "\\par "; break;
                    case L'\r': break;
                    case L'\t': numberedRtf += "\\tab "; break;
                    default: numberedRtf += static_cast<char>(ch); break;
                }
            }
            else
            {
                numberedRtf += "\\u" + std::to_string(static_cast<int>(ch)) + "?";
            }
        }
        
        numberedRtf += "}";
        return numberedRtf;
    }
    else
    {
        // Convert text to RTF-escaped ASCII without line numbers
        for (wchar_t ch : text)
        {
            if (ch < 128)
            {
                switch (ch)
                {
                    case L'\\': rtf += "\\\\"; break;
                    case L'{': rtf += "\\{"; break;
                    case L'}': rtf += "\\}"; break;
                    case L'\n': rtf += "\\par "; break;
                    case L'\r': break;
                    case L'\t': rtf += "\\tab "; break;
                    default: rtf += static_cast<char>(ch); break;
                }
            }
            else
            {
                rtf += "\\u" + std::to_string(static_cast<int>(ch)) + "?";
            }
        }
        
        rtf += "}";
        return rtf;
    }
}

} // namespace debugviewpp
} // namespace fusion
