#pragma once

#include <atlbase.h>
#include <atlwin.h>
#include <string>
#include <Scintilla.h>
#include <SciLexer.h>

namespace fusion {
namespace debugviewpp {

class CScintillaWrapper : public CWindow
{
public:
    
    CScintillaWrapper();
    ~CScintillaWrapper();
    
    // Window creation
    HWND Create(HWND hWndParent, const RECT& rc = CWindow::rcDefault, 
                LPCTSTR szWindowName = nullptr, 
                DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
                DWORD dwExStyle = 0);
    
    // Basic text operations
    void SetText(const std::string& text);
    void SetText(const std::wstring& text);
    std::string GetText() const;
    void ClearAll();
    
    // Line numbers
    void ShowLineNumbers(bool show);
    void SetLineNumberWidth(int width);
    
    // Styling and lexers
    void SetLexer(int lexer);
    void SetLexerLanguage(const char* language);
    void StyleSetFont(int style, const char* fontName);
    void StyleSetSize(int style, int sizePoints);
    void StyleSetFore(int style, COLORREF fore);
    void StyleSetBack(int style, COLORREF back);
    void StyleSetBold(int style, bool bold);
    void StyleSetItalic(int style, bool italic);
    void StyleClearAll();
    
    // SQL lexer setup
    void ConfigureSQLLexer(bool darkTheme);
    // JSON lexer setup  
    void ConfigureJSONLexer(bool darkTheme);
    // XML lexer setup
    void ConfigureXMLLexer(bool darkTheme);
    // Plain text setup
    void ConfigurePlainText(bool darkTheme);
    
    // Editor features
    void SetReadOnly(bool readOnly);
    void SetWordWrap(bool wordWrap);
    void SetTabWidth(int width);
    void SetIndentationGuides(bool show);
    
    // Font
    void SetFont(HFONT hFont);
    
    // Scrolling
    void ScrollToLine(int line);
    int GetFirstVisibleLine() const;
    void SetFirstVisibleLine(int line);
    
    // Selection
    void SelectAll();
    void SetSelection(int start, int end);
    std::string GetSelectedText() const;
    
    // Search
    int FindText(int flags, const char* text, int start = 0, int end = -1);
    
    // Theme
    void ApplyTheme(bool darkTheme);
    
    // Text highlighting
    void HighlightAllOccurrences(const std::string& text);
    void ClearHighlights();
    
    // Make SendEditor public for advanced operations
    LRESULT SendEditor(UINT msg, WPARAM wParam = 0, LPARAM lParam = 0) const;
    
private:
    // Helper to apply line number colors based on theme
    void ApplyLineNumberColors(bool darkTheme);
    
    // Helper to convert wide string to UTF-8
    std::string WideToUTF8(const std::wstring& wide) const;
    std::wstring UTF8ToWide(const std::string& utf8) const;
    
    SciFnDirect m_pSciMsg;
    sptr_t m_pSciWndData;
    bool m_initialized;
};

} // namespace debugviewpp
} // namespace fusion