// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <windows.h>
#include <richedit.h>

namespace fusion {
namespace debugviewpp {

enum class ContentFormat
{
    Unknown,
    PlainText,
    JSON,
    XML,
    SQL,
    StackTrace,
    LogLevel
};

struct TextRange
{
    int start;
    int end;
    COLORREF color;
    bool bold = false;
    bool italic = false;
};

struct HighlightColors
{
    // Light theme colors
    struct Light 
    {
        static constexpr COLORREF keyword = RGB(0, 0, 255);      // Blue
        static constexpr COLORREF string = RGB(0, 128, 0);       // Green
        static constexpr COLORREF number = RGB(255, 128, 0);     // Orange
        static constexpr COLORREF boolean = RGB(128, 0, 128);    // Purple
        static constexpr COLORREF comment = RGB(128, 128, 128);  // Gray
        static constexpr COLORREF tag = RGB(0, 0, 255);          // Blue
        static constexpr COLORREF attribute = RGB(255, 0, 0);    // Red
        static constexpr COLORREF method = RGB(0, 0, 160);       // Dark Blue
        static constexpr COLORREF error = RGB(255, 0, 0);        // Red
        static constexpr COLORREF warning = RGB(255, 128, 0);    // Orange
        static constexpr COLORREF info = RGB(0, 0, 255);         // Blue
    };
    
    // Dark theme colors
    struct Dark
    {
        static constexpr COLORREF keyword = RGB(86, 156, 214);   // Light Blue
        static constexpr COLORREF string = RGB(214, 157, 133);   // Light Brown
        static constexpr COLORREF number = RGB(181, 206, 168);   // Light Green
        static constexpr COLORREF boolean = RGB(184, 139, 184);  // Light Purple
        static constexpr COLORREF comment = RGB(106, 153, 85);   // Green-gray
        static constexpr COLORREF tag = RGB(86, 156, 214);       // Light Blue
        static constexpr COLORREF attribute = RGB(255, 128, 64); // Orange (more distinct)
        static constexpr COLORREF method = RGB(220, 220, 170);   // Light Yellow
        static constexpr COLORREF error = RGB(255, 128, 128);    // Light Red
        static constexpr COLORREF warning = RGB(255, 200, 128);  // Light Orange
        static constexpr COLORREF info = RGB(128, 200, 255);     // Light Blue
    };
};

class SyntaxHighlighter
{
public:
    SyntaxHighlighter();
    ~SyntaxHighlighter();
    
    // Detect the format of the given text
    ContentFormat DetectFormat(const std::wstring& text);
    
    // Get highlighting ranges for the text based on detected or specified format
    std::vector<TextRange> GetHighlightRanges(const std::wstring& text, ContentFormat format = ContentFormat::Unknown);
    
    // Apply highlighting to a RichEdit control
    void ApplyHighlighting(HWND hRichEdit, const std::wstring& text, bool isDarkTheme);
    
    // RTF generation for syntax highlighting
    std::string GenerateRTFForXML(const std::wstring& text, bool isDarkTheme, bool showLineNumbers = false);
    std::string GenerateRTFForJSON(const std::wstring& text, bool isDarkTheme, bool showLineNumbers = false);
    std::string GenerateRTFForSQL(const std::wstring& text, bool isDarkTheme, bool showLineNumbers = false);
    std::string GenerateRTFForLogLevel(const std::wstring& text, bool isDarkTheme, bool showLineNumbers = false);
    std::string GenerateRTFForStackTrace(const std::wstring& text, bool isDarkTheme, bool showLineNumbers = false);
    std::string GenerateRTFForPlainText(const std::wstring& text, bool isDarkTheme, bool showLineNumbers = false);
    
    // Content formatting (beautification)
    std::wstring FormatJSON(const std::wstring& text);
    std::wstring FormatXML(const std::wstring& text);
    std::wstring FormatSQL(const std::wstring& text);
    
    // Enable/disable highlighting
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }
    
private:
    // Format detection helpers
    bool IsJSON(const std::wstring& text);
    bool IsXML(const std::wstring& text);
    bool IsSQL(const std::wstring& text);
    bool IsStackTrace(const std::wstring& text);
    bool HasLogLevel(const std::wstring& text);
    
    // Parsing methods for each format
    std::vector<TextRange> ParseJSON(const std::wstring& text, bool isDarkTheme);
    std::vector<TextRange> ParseXML(const std::wstring& text, bool isDarkTheme);
    std::vector<TextRange> ParseSQL(const std::wstring& text, bool isDarkTheme);
    std::vector<TextRange> ParseStackTrace(const std::wstring& text, bool isDarkTheme);
    std::vector<TextRange> ParseLogLevel(const std::wstring& text, bool isDarkTheme);
    
    // Helper methods
    COLORREF GetColor(ContentFormat format, const std::wstring& tokenType, bool isDarkTheme);
    void ApplyRangeFormat(HWND hRichEdit, const TextRange& range);
    
    // Line number helpers
    std::string GenerateRTFWithLineNumbers(const std::wstring& text, const std::string& contentRtf, bool isDarkTheme);
    std::string WrapWithLineNumbers(const std::string& rtfContent, const std::wstring& text, bool isDarkTheme);
    int CountLines(const std::wstring& text);
    
    bool m_enabled = true;
    ContentFormat m_lastDetectedFormat = ContentFormat::Unknown;
};

} // namespace debugviewpp
} // namespace fusion