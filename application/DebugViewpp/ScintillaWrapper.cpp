#include "ScintillaWrapper.h"
#include <atlconv.h>
#include <vector>
#include "DebugViewppLib/ThemeManager.h"
#include <SciLexer.h>
#include <ILexer.h>
#include <Lexilla.h>

namespace fusion {
namespace debugviewpp {

// Static initialization for Scintilla
static bool g_scintillaInitialized = false;

static void EnsureScintillaInitialized()
{
    if (!g_scintillaInitialized)
    {
        Scintilla_RegisterClasses(GetModuleHandle(nullptr));
        g_scintillaInitialized = true;
    }
}

CScintillaWrapper::CScintillaWrapper()
    : m_pSciMsg(nullptr)
    , m_pSciWndData(0)
    , m_initialized(false)
{
    EnsureScintillaInitialized();
}

CScintillaWrapper::~CScintillaWrapper()
{
    if (m_hWnd && ::IsWindow(m_hWnd))
    {
        DestroyWindow();
    }
}

HWND CScintillaWrapper::Create(HWND hWndParent, const RECT& rc, 
                                LPCTSTR szWindowName, 
                                DWORD dwStyle,
                                DWORD dwExStyle)
{
    // Create the actual Scintilla window
    HWND hWnd = ::CreateWindowEx(
        dwExStyle,
        L"Scintilla",  // The registered Scintilla class name
        szWindowName ? szWindowName : L"",
        dwStyle,
        rc.left, rc.top,
        rc.right - rc.left, rc.bottom - rc.top,
        hWndParent,
        NULL,
        GetModuleHandle(NULL),
        NULL);
    
    if (hWnd)
    {
        // Attach to the CWindow wrapper
        Attach(hWnd);
        
        // Get direct access function pointer
        m_pSciMsg = (SciFnDirect)SendMessage(SCI_GETDIRECTFUNCTION, 0, 0);
        m_pSciWndData = SendMessage(SCI_GETDIRECTPOINTER, 0, 0);
        m_initialized = true;
        
        // Set default settings
        SendEditor(SCI_SETCODEPAGE, SC_CP_UTF8);
        SendEditor(SCI_SETTABWIDTH, 4);
        SendEditor(SCI_SETINDENTATIONGUIDES, SC_IV_LOOKBOTH);
        
        // Set default margins
        SendEditor(SCI_SETMARGINWIDTHN, 1, 0); // Hide margin 1 by default
        SendEditor(SCI_SETMARGINWIDTHN, 2, 0); // Hide margin 2 by default
    }
    
    return hWnd;
}

LRESULT CScintillaWrapper::SendEditor(UINT msg, WPARAM wParam, LPARAM lParam) const
{
    if (m_initialized && m_pSciMsg)
    {
        return m_pSciMsg(m_pSciWndData, msg, wParam, lParam);
    }
    else if (m_hWnd)
    {
        return ::SendMessage(m_hWnd, msg, wParam, lParam);
    }
    return 0;
}

void CScintillaWrapper::SetText(const std::string& text)
{
    SendEditor(SCI_SETTEXT, 0, (LPARAM)text.c_str());
}

void CScintillaWrapper::SetText(const std::wstring& text)
{
    SetText(WideToUTF8(text));
}

std::string CScintillaWrapper::GetText() const
{
    int len = SendEditor(SCI_GETLENGTH) + 1;
    std::vector<char> buffer(len);
    SendEditor(SCI_GETTEXT, len, (LPARAM)buffer.data());
    return std::string(buffer.data());
}

void CScintillaWrapper::ClearAll()
{
    SendEditor(SCI_CLEARALL);
}

void CScintillaWrapper::ShowLineNumbers(bool show)
{
    if (show)
    {
        // Calculate width for line numbers
        int lineCount = SendEditor(SCI_GETLINECOUNT);
        int maxLineDigits = 1;
        while (lineCount >= 10)
        {
            lineCount /= 10;
            maxLineDigits++;
        }
        
        // Set margin 0 for line numbers
        SendEditor(SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
        
        // Calculate pixel width (roughly 8 pixels per digit plus some padding)
        int pixelWidth = (maxLineDigits + 2) * 8;
        SendEditor(SCI_SETMARGINWIDTHN, 0, pixelWidth);
        
        // Apply theme-appropriate colors to line numbers
        bool isDarkTheme = ThemeManager::Instance().IsDarkTheme();
        ApplyLineNumberColors(isDarkTheme);
    }
    else
    {
        SendEditor(SCI_SETMARGINWIDTHN, 0, 0);
    }
}

void CScintillaWrapper::SetLineNumberWidth(int width)
{
    SendEditor(SCI_SETMARGINWIDTHN, 0, width);
}

void CScintillaWrapper::SetLexer(int lexer)
{
    // For SCLEX_NULL, just clear the lexer
    if (lexer == SCLEX_NULL)
    {
        SendEditor(SCI_SETILEXER, 0, 0);
        return;
    }
    
    // Map SCLEX constants to lexer names for Lexilla
    const char* lexerName = nullptr;
    
    switch (lexer)
    {
        case SCLEX_SQL:
            lexerName = "mssql";  // Use MSSQL lexer for better SQL support
            break;
        case SCLEX_MSSQL:
            lexerName = "mssql";
            break;
        case SCLEX_JSON:
            lexerName = "json";
            break;
        case SCLEX_XML:
            lexerName = "xml";
            break;
        default:
            // Unknown lexer, clear it
            SendEditor(SCI_SETILEXER, 0, 0);
            return;
    }
    
    // Create the lexer from Lexilla
    ILexer5* pLexer = CreateLexer(lexerName);
    
    if (pLexer)
    {
        // Set the lexer in Scintilla
        SendEditor(SCI_SETILEXER, 0, (LPARAM)pLexer);
        
        // Note: Scintilla takes ownership of the lexer and will delete it
        // when a new lexer is set or the control is destroyed
    }
    else
    {
        // If CreateLexer failed, clear the lexer
        // This shouldn't happen if Lexilla is properly linked
        SendEditor(SCI_SETILEXER, 0, 0);
    }
}

void CScintillaWrapper::SetLexerLanguage(const char* language)
{
    // This would require Lexilla integration
    // For now we'll use direct style configuration
}

void CScintillaWrapper::StyleSetFont(int style, const char* fontName)
{
    SendEditor(SCI_STYLESETFONT, style, (LPARAM)fontName);
}

void CScintillaWrapper::StyleSetSize(int style, int sizePoints)
{
    SendEditor(SCI_STYLESETSIZE, style, sizePoints);
}

void CScintillaWrapper::StyleSetFore(int style, COLORREF fore)
{
    SendEditor(SCI_STYLESETFORE, style, fore);
}

void CScintillaWrapper::StyleSetBack(int style, COLORREF back)
{
    SendEditor(SCI_STYLESETBACK, style, back);
}

void CScintillaWrapper::StyleSetBold(int style, bool bold)
{
    SendEditor(SCI_STYLESETBOLD, style, bold);
}

void CScintillaWrapper::StyleSetItalic(int style, bool italic)
{
    SendEditor(SCI_STYLESETITALIC, style, italic);
}

void CScintillaWrapper::StyleClearAll()
{
    SendEditor(SCI_STYLECLEARALL);
}

void CScintillaWrapper::ConfigureSQLLexer(bool darkTheme)
{
    // Use MSSQL lexer instead of generic SQL for better handling of identifiers
    SetLexer(SCLEX_MSSQL);
    
    // Configure MSSQL lexer properties
    SendEditor(SCI_SETPROPERTY, (WPARAM)"fold", (LPARAM)"1");
    
    // Set default style first
    if (darkTheme)
    {
        StyleSetBack(STYLE_DEFAULT, RGB(30, 30, 30));
        StyleSetFore(STYLE_DEFAULT, RGB(220, 220, 220));
    }
    else
    {
        StyleSetBack(STYLE_DEFAULT, RGB(255, 255, 255));
        StyleSetFore(STYLE_DEFAULT, RGB(0, 0, 0));
    }
    
    StyleClearAll(); // Apply default to all styles
    
    // Re-apply line number colors after StyleClearAll
    ApplyLineNumberColors(darkTheme);
    
    // MSSQL specific styles
    if (darkTheme)
    {
        // Dark theme colors
        StyleSetFore(SCE_MSSQL_DEFAULT, RGB(220, 220, 220));           // Default text
        StyleSetFore(SCE_MSSQL_COMMENT, RGB(87, 166, 74));             // Green comments
        StyleSetFore(SCE_MSSQL_LINE_COMMENT, RGB(87, 166, 74));        // Green line comments
        StyleSetFore(SCE_MSSQL_STRING, RGB(214, 157, 133));            // Orange strings
        StyleSetFore(SCE_MSSQL_NUMBER, RGB(181, 206, 168));            // Light green numbers
        StyleSetFore(SCE_MSSQL_OPERATOR, RGB(180, 180, 180));          // Gray operators
        StyleSetFore(SCE_MSSQL_IDENTIFIER, RGB(220, 220, 220));        // Default identifiers
        StyleSetFore(SCE_MSSQL_VARIABLE, RGB(156, 220, 254));          // Light blue variables
        StyleSetFore(SCE_MSSQL_COLUMN_NAME, RGB(220, 220, 220));       // Column names
        StyleSetFore(SCE_MSSQL_STATEMENT, RGB(86, 156, 214));          // Blue keywords
        StyleSetBold(SCE_MSSQL_STATEMENT, true);
        StyleSetFore(SCE_MSSQL_DATATYPE, RGB(78, 201, 176));           // Cyan for types
        StyleSetFore(SCE_MSSQL_FUNCTION, RGB(255, 160, 122));          // Orange functions
        StyleSetFore(SCE_MSSQL_STORED_PROCEDURE, RGB(255, 160, 122));  // Orange stored procs
    }
    else
    {
        // Light theme colors
        StyleSetFore(SCE_MSSQL_DEFAULT, RGB(0, 0, 0));                 // Default text
        StyleSetFore(SCE_MSSQL_COMMENT, RGB(0, 128, 0));               // Green comments
        StyleSetFore(SCE_MSSQL_LINE_COMMENT, RGB(0, 128, 0));          // Green line comments
        StyleSetFore(SCE_MSSQL_STRING, RGB(163, 21, 21));              // Red strings
        StyleSetFore(SCE_MSSQL_NUMBER, RGB(9, 136, 90));               // Green numbers
        StyleSetFore(SCE_MSSQL_OPERATOR, RGB(0, 0, 0));                // Black operators
        StyleSetFore(SCE_MSSQL_IDENTIFIER, RGB(0, 0, 0));              // Black identifiers
        StyleSetFore(SCE_MSSQL_VARIABLE, RGB(0, 16, 128));             // Dark blue variables
        StyleSetFore(SCE_MSSQL_COLUMN_NAME, RGB(0, 0, 0));             // Column names
        StyleSetFore(SCE_MSSQL_STATEMENT, RGB(0, 0, 255));             // Blue keywords
        StyleSetBold(SCE_MSSQL_STATEMENT, true);
        StyleSetFore(SCE_MSSQL_DATATYPE, RGB(43, 145, 175));           // Cyan for types
        StyleSetFore(SCE_MSSQL_FUNCTION, RGB(255, 0, 255));            // Magenta functions
        StyleSetFore(SCE_MSSQL_STORED_PROCEDURE, RGB(255, 0, 255));    // Magenta stored procs
    }
    
    // Set SQL keywords
    const char* sqlKeywords = "select from where and or not in exists between like as "
                              "inner left right full outer join on using "
                              "insert into values update set delete "
                              "create table alter drop index view trigger procedure function "
                              "begin end if then else case when "
                              "group by having order asc desc limit offset "
                              "distinct all any some union intersect except "
                              "primary key foreign references constraint unique check default "
                              "null is not true false";
    
    SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)sqlKeywords);
    
    // Force re-colorization after setting everything up
    SendEditor(SCI_COLOURISE, 0, -1);
    
    // SQL data types
    const char* sqlTypes = "int integer bigint smallint tinyint float real double decimal numeric "
                          "char varchar text nchar nvarchar ntext "
                          "date time datetime timestamp year "
                          "bit bool boolean blob clob "
                          "binary varbinary image";
    
    SendEditor(SCI_SETKEYWORDS, 1, (LPARAM)sqlTypes);
}

void CScintillaWrapper::ConfigureJSONLexer(bool darkTheme)
{
    SetLexer(SCLEX_JSON);
    
    // Set default style
    if (darkTheme)
    {
        StyleSetBack(STYLE_DEFAULT, RGB(30, 30, 30));
        StyleSetFore(STYLE_DEFAULT, RGB(220, 220, 220));
    }
    else
    {
        StyleSetBack(STYLE_DEFAULT, RGB(255, 255, 255));
        StyleSetFore(STYLE_DEFAULT, RGB(0, 0, 0));
    }
    
    StyleClearAll();
    
    // Re-apply line number colors after StyleClearAll
    ApplyLineNumberColors(darkTheme);
    
    // JSON specific styles
    if (darkTheme)
    {
        StyleSetFore(SCE_JSON_STRING, RGB(214, 157, 133));      // Orange strings
        StyleSetFore(SCE_JSON_STRINGEOL, RGB(214, 157, 133));
        StyleSetFore(SCE_JSON_PROPERTYNAME, RGB(156, 220, 254)); // Light blue property names
        StyleSetFore(SCE_JSON_NUMBER, RGB(181, 206, 168));       // Light green numbers
        StyleSetFore(SCE_JSON_KEYWORD, RGB(86, 156, 214));       // Blue for true/false/null
        StyleSetBold(SCE_JSON_KEYWORD, true);
        StyleSetFore(SCE_JSON_ERROR, RGB(252, 86, 83));          // Red for errors
        StyleSetBack(SCE_JSON_ERROR, RGB(60, 0, 0));
    }
    else
    {
        StyleSetFore(SCE_JSON_STRING, RGB(163, 21, 21));         // Red strings
        StyleSetFore(SCE_JSON_STRINGEOL, RGB(163, 21, 21));
        StyleSetFore(SCE_JSON_PROPERTYNAME, RGB(0, 16, 128));    // Dark blue property names
        StyleSetFore(SCE_JSON_NUMBER, RGB(9, 136, 90));          // Green numbers
        StyleSetFore(SCE_JSON_KEYWORD, RGB(0, 0, 255));          // Blue for true/false/null
        StyleSetBold(SCE_JSON_KEYWORD, true);
        StyleSetFore(SCE_JSON_ERROR, RGB(255, 0, 0));            // Red for errors
        StyleSetBack(SCE_JSON_ERROR, RGB(255, 200, 200));
    }
}

void CScintillaWrapper::ConfigureXMLLexer(bool darkTheme)
{
    SetLexer(SCLEX_XML);
    
    // Set default style
    if (darkTheme)
    {
        StyleSetBack(STYLE_DEFAULT, RGB(30, 30, 30));
        StyleSetFore(STYLE_DEFAULT, RGB(220, 220, 220));
    }
    else
    {
        StyleSetBack(STYLE_DEFAULT, RGB(255, 255, 255));
        StyleSetFore(STYLE_DEFAULT, RGB(0, 0, 0));
    }
    
    StyleClearAll();
    
    // Re-apply line number colors after StyleClearAll
    ApplyLineNumberColors(darkTheme);
    
    // XML specific styles
    if (darkTheme)
    {
        StyleSetFore(SCE_H_TAG, RGB(86, 156, 214));              // Blue tags
        StyleSetFore(SCE_H_TAGUNKNOWN, RGB(86, 156, 214));
        StyleSetFore(SCE_H_ATTRIBUTE, RGB(156, 220, 254));       // Light blue attributes
        StyleSetFore(SCE_H_ATTRIBUTEUNKNOWN, RGB(156, 220, 254));
        StyleSetFore(SCE_H_VALUE, RGB(214, 157, 133));           // Orange values
        StyleSetFore(SCE_H_DOUBLESTRING, RGB(214, 157, 133));
        StyleSetFore(SCE_H_SINGLESTRING, RGB(214, 157, 133));
        StyleSetFore(SCE_H_COMMENT, RGB(87, 166, 74));           // Green comments
        StyleSetFore(SCE_H_CDATA, RGB(78, 201, 176));            // Cyan CDATA
    }
    else
    {
        StyleSetFore(SCE_H_TAG, RGB(0, 0, 128));                 // Dark blue tags
        StyleSetFore(SCE_H_TAGUNKNOWN, RGB(0, 0, 128));
        StyleSetFore(SCE_H_ATTRIBUTE, RGB(255, 0, 0));           // Red attributes
        StyleSetFore(SCE_H_ATTRIBUTEUNKNOWN, RGB(255, 0, 0));
        StyleSetFore(SCE_H_VALUE, RGB(163, 21, 21));             // Dark red values
        StyleSetFore(SCE_H_DOUBLESTRING, RGB(163, 21, 21));
        StyleSetFore(SCE_H_SINGLESTRING, RGB(163, 21, 21));
        StyleSetFore(SCE_H_COMMENT, RGB(0, 128, 0));             // Green comments
        StyleSetFore(SCE_H_CDATA, RGB(0, 0, 255));               // Blue CDATA
    }
}

void CScintillaWrapper::ConfigurePlainText(bool darkTheme)
{
    SetLexer(SCLEX_NULL); // No lexer for plain text
    
    // Set style 32 (STYLE_DEFAULT) and 33 (margin background)
    if (darkTheme)
    {
        SendEditor(SCI_STYLESETBACK, 32, RGB(30, 30, 30));
        SendEditor(SCI_STYLESETFORE, 32, RGB(220, 220, 220));
        SendEditor(SCI_STYLESETBACK, 33, RGB(30, 30, 30));
    }
    else
    {
        SendEditor(SCI_STYLESETBACK, 32, RGB(255, 255, 255));
        SendEditor(SCI_STYLESETFORE, 32, RGB(0, 0, 0));
        SendEditor(SCI_STYLESETBACK, 33, RGB(255, 255, 255));
    }
    
    StyleClearAll();
    
    // Enable EOL filled to extend background to right edge
    SendEditor(SCI_STYLESETEOLFILLED, 32, 1);
    
    // Set whitespace/empty area background
    SendEditor(SCI_SETWHITESPACEBACK, 1, darkTheme ? RGB(30, 30, 30) : RGB(255, 255, 255));
    SendEditor(SCI_SETWHITESPACEFORE, 1, darkTheme ? RGB(220, 220, 220) : RGB(0, 0, 0));
    
    // Re-apply line number colors after StyleClearAll
    ApplyLineNumberColors(darkTheme);
    
    // Force the control to repaint
    SendEditor(SCI_COLOURISE, 0, -1);
    SendEditor(SCI_SETVIEWWS, SCWS_INVISIBLE);  // Hide whitespace markers but apply background
}

void CScintillaWrapper::SetReadOnly(bool readOnly)
{
    SendEditor(SCI_SETREADONLY, readOnly);
}

void CScintillaWrapper::SetWordWrap(bool wordWrap)
{
    SendEditor(SCI_SETWRAPMODE, wordWrap ? SC_WRAP_WORD : SC_WRAP_NONE);
}

void CScintillaWrapper::SetTabWidth(int width)
{
    SendEditor(SCI_SETTABWIDTH, width);
}

void CScintillaWrapper::SetIndentationGuides(bool show)
{
    SendEditor(SCI_SETINDENTATIONGUIDES, show ? SC_IV_LOOKBOTH : SC_IV_NONE);
}

void CScintillaWrapper::SetFont(HFONT hFont)
{
    if (hFont)
    {
        LOGFONT lf;
        ::GetObject(hFont, sizeof(LOGFONT), &lf);
        
        // Convert to UTF-8
        std::string fontName = WideToUTF8(lf.lfFaceName);
        
        // Calculate point size from height
        HDC hdc = ::GetDC(m_hWnd);
        int pointSize = -MulDiv(lf.lfHeight, 72, GetDeviceCaps(hdc, LOGPIXELSY));
        ::ReleaseDC(m_hWnd, hdc);
        
        // Apply to all styles
        for (int i = 0; i <= STYLE_MAX; i++)
        {
            StyleSetFont(i, fontName.c_str());
            StyleSetSize(i, pointSize);
        }
    }
}

void CScintillaWrapper::ScrollToLine(int line)
{
    SendEditor(SCI_GOTOLINE, line);
}

int CScintillaWrapper::GetFirstVisibleLine() const
{
    return SendEditor(SCI_GETFIRSTVISIBLELINE);
}

void CScintillaWrapper::SetFirstVisibleLine(int line)
{
    SendEditor(SCI_SETFIRSTVISIBLELINE, line);
}

void CScintillaWrapper::SelectAll()
{
    SendEditor(SCI_SELECTALL);
}

void CScintillaWrapper::SetSelection(int start, int end)
{
    SendEditor(SCI_SETSEL, start, end);
}

std::string CScintillaWrapper::GetSelectedText() const
{
    int len = SendEditor(SCI_GETSELTEXT, 0, 0);
    if (len > 0)
    {
        std::vector<char> buffer(len);
        SendEditor(SCI_GETSELTEXT, 0, (LPARAM)buffer.data());
        return std::string(buffer.data());
    }
    return "";
}

int CScintillaWrapper::FindText(int flags, const char* text, int start, int end)
{
    Sci_TextToFind ttf;
    ttf.chrg.cpMin = start;
    ttf.chrg.cpMax = (end == -1) ? SendEditor(SCI_GETLENGTH) : end;
    ttf.lpstrText = text;
    
    return SendEditor(SCI_FINDTEXT, flags, (LPARAM)&ttf);
}

void CScintillaWrapper::ApplyLineNumberColors(bool darkTheme)
{
    if (darkTheme)
    {
        // Dark theme: light gray numbers on slightly lighter background
        SendEditor(SCI_STYLESETBACK, STYLE_LINENUMBER, RGB(40, 40, 40));
        SendEditor(SCI_STYLESETFORE, STYLE_LINENUMBER, RGB(200, 200, 200));
    }
    else
    {
        // Light theme: dark gray numbers on light gray background
        SendEditor(SCI_STYLESETBACK, STYLE_LINENUMBER, RGB(240, 240, 240));
        SendEditor(SCI_STYLESETFORE, STYLE_LINENUMBER, RGB(40, 40, 40));
    }
}

void CScintillaWrapper::ApplyTheme(bool darkTheme)
{
    // First set the default style colors (STYLE_DEFAULT = 32)
    COLORREF backColor = darkTheme ? RGB(30, 30, 30) : RGB(255, 255, 255);
    COLORREF foreColor = darkTheme ? RGB(220, 220, 220) : RGB(0, 0, 0);
    
    // Set STYLE_DEFAULT (32)
    SendEditor(SCI_STYLESETBACK, 32, backColor);
    SendEditor(SCI_STYLESETFORE, 32, foreColor);
    
    // Apply default style to all styles FIRST (this copies STYLE_DEFAULT to all other styles)
    SendEditor(SCI_STYLECLEARALL);
    
    // Now explicitly set all styles from 0-39 to ensure complete coverage
    // This is necessary because sometimes STYLECLEARALL doesn't propagate properly
    for (int i = 0; i <= 39; i++)
    {
        SendEditor(SCI_STYLESETBACK, i, backColor);
        SendEditor(SCI_STYLESETFORE, i, foreColor);
    }
    
    // Enable EOL filled - ensures background extends to right edge of window
    SendEditor(SCI_STYLESETEOLFILLED, 32, 1);
    
    // Set caret and selection colors
    if (darkTheme)
    {
        SendEditor(SCI_SETCARETFORE, RGB(255, 255, 255));
        SendEditor(SCI_SETSELBACK, 1, RGB(51, 94, 168));
        SendEditor(SCI_SETSELFORE, 1, RGB(255, 255, 255));
        // Set whitespace background
        SendEditor(SCI_SETWHITESPACEBACK, 1, RGB(30, 30, 30));
        SendEditor(SCI_SETWHITESPACEFORE, 1, RGB(220, 220, 220));
    }
    else
    {
        SendEditor(SCI_SETCARETFORE, RGB(0, 0, 0));
        SendEditor(SCI_SETSELBACK, 1, RGB(173, 214, 255));
        SendEditor(SCI_SETSELFORE, 1, RGB(0, 0, 0));
        // Set whitespace background
        SendEditor(SCI_SETWHITESPACEBACK, 1, RGB(255, 255, 255));
        SendEditor(SCI_SETWHITESPACEFORE, 1, RGB(0, 0, 0));
    }
    
    // Apply line number colors
    ApplyLineNumberColors(darkTheme);
    
    // Force refresh of the control
    SendEditor(SCI_COLOURISE, 0, -1);
    SendEditor(SCI_SETVIEWWS, SCWS_INVISIBLE);  // Hide whitespace markers but apply background
    
    // Force a complete redraw
    InvalidateRect(nullptr, TRUE);
    UpdateWindow();
}

std::string CScintillaWrapper::WideToUTF8(const std::wstring& wide) const
{
    if (wide.empty())
        return "";
    
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::vector<char> buffer(size);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, buffer.data(), size, nullptr, nullptr);
    return std::string(buffer.data());
}

std::wstring CScintillaWrapper::UTF8ToWide(const std::string& utf8) const
{
    if (utf8.empty())
        return L"";
    
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::vector<wchar_t> buffer(size);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, buffer.data(), size);
    return std::wstring(buffer.data());
}

void CScintillaWrapper::HighlightAllOccurrences(const std::string& text)
{
    // Clear any existing highlights first
    ClearHighlights();
    
    if (text.empty())
        return;
    
    // Define indicator 0 for highlighting (using a box with yellow background)
    const int HIGHLIGHT_INDICATOR = 0;
    SendEditor(SCI_INDICSETSTYLE, HIGHLIGHT_INDICATOR, INDIC_ROUNDBOX);
    SendEditor(SCI_INDICSETFORE, HIGHLIGHT_INDICATOR, RGB(255, 255, 55)); // Yellow highlight color from Colors::Highlight
    SendEditor(SCI_INDICSETALPHA, HIGHLIGHT_INDICATOR, 100); // Semi-transparent
    SendEditor(SCI_INDICSETOUTLINEALPHA, HIGHLIGHT_INDICATOR, 255); // Solid outline
    
    // Get document length
    int docLength = (int)SendEditor(SCI_GETLENGTH, 0, 0);
    
    // Find all occurrences and highlight them
    Sci_TextToFind ttf;
    ttf.chrg.cpMin = 0;
    ttf.chrg.cpMax = docLength;
    ttf.lpstrText = text.c_str();
    
    int pos = 0;
    while (pos < docLength)
    {
        // Search for the text (case-insensitive whole word)
        int foundPos = (int)SendEditor(SCI_FINDTEXT, SCFIND_WHOLEWORD, (LPARAM)&ttf);
        if (foundPos == -1)
            break;
        
        // Set the indicator for this occurrence
        SendEditor(SCI_SETINDICATORCURRENT, HIGHLIGHT_INDICATOR, 0);
        SendEditor(SCI_INDICATORFILLRANGE, foundPos, (int)text.length());
        
        // Move search start position forward
        ttf.chrg.cpMin = foundPos + (int)text.length();
    }
}

void CScintillaWrapper::ClearHighlights()
{
    // Clear indicator 0 from the entire document
    const int HIGHLIGHT_INDICATOR = 0;
    int docLength = (int)SendEditor(SCI_GETLENGTH, 0, 0);
    SendEditor(SCI_SETINDICATORCURRENT, HIGHLIGHT_INDICATOR, 0);
    SendEditor(SCI_INDICATORCLEARRANGE, 0, docLength);
}

} // namespace debugviewpp
} // namespace fusion