// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "CLogViewTabItem2.h"
#include "LogView.h"
#include "MainFrame.h"
#include "resource.h"
#include <atldlgs.h>  // For CFontDialog
#include <atlconv.h>  // For CW2A, CA2W
#include <Scintilla.h>  // For SCNotification and SCN_DOUBLECLICK
#include <vector>

namespace fusion {
namespace debugviewpp {

void CLogViewTabItem::SetView(std::shared_ptr<CLogView> pView)
{
    m_pView = pView;
    SetTabView(*pView);
}

CLogView& CLogViewTabItem::GetView()
{
    return *m_pView;
}




void DisablePaneHeader(CMyPaneContainer& panecontainer)
{
    panecontainer.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON, 0);
    panecontainer.m_cxyHeader = 0;
}

void CLogViewTabItem2::Create(HWND parent)
{
    m_split.Create(parent, CWindow::rcDefault, nullptr, 
                  WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    SetTabView(m_split);

    m_top.Create(m_split, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    m_bottom.Create(m_split, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

    DisablePaneHeader(m_top);
    DisablePaneHeader(m_bottom);
    m_split.SetSplitterPanes(m_top, m_bottom, true);
    
    // Set splitter extended style to reduce flicker
    m_split.SetSplitterExtendedStyle(SPLIT_PROPORTIONAL | SPLIT_BOTTOMALIGNED);

    // Create the detail view wrapper window
    m_detailView.Create(m_bottom, CWindow::rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN);
    
    // Set the detail view as the client of the bottom pane
    m_bottom.SetClient(m_detailView);
    
    // Force the window to process its WM_CREATE message immediately
    // This ensures the Scintilla control is created
    m_detailView.UpdateWindow();
    
    // Post a message to set the initial text after all initialization is complete
    m_detailView.PostMessage(WM_USER + 100);  // Custom message to set initial text
    
    // Force layout update
    m_split.UpdateWindow();
    
    // Set initial splitter state based on m_detailPaneVisible
    if (m_detailPaneVisible)
    {
        m_split.SetSplitterPosPct(m_splitterPos);
    }
    else
    {
        // Hide the detail pane on first run
        m_split.SetSinglePaneMode(SPLIT_PANE_TOP);
    }
}

CLogViewTabItem2::~CLogViewTabItem2()
{
    m_split.DestroyWindow();
}


void CLogViewTabItem2::SetView(std::shared_ptr<CLogView> pView)
{
    m_pView = pView;
    m_top.SetClient(*m_pView);
}

CLogView& CLogViewTabItem2::GetView()
{
    return *m_pView;
}

void CLogViewTabItem2::ShowDetailPane(bool show)
{
    if (m_detailPaneVisible == show)
        return;
        
    m_detailPaneVisible = show;
    
    if (show)
    {
        // Show the bottom pane
        m_split.SetSinglePaneMode(SPLIT_PANE_NONE);
        m_split.SetSplitterPosPct(m_splitterPos);
        
        // Force proper layout update
        m_split.UpdateSplitterLayout();
        
        // Ensure the detail view's theme is applied when it becomes visible
        m_detailView.RefreshTheme();
    }
    else
    {
        // Remember current position before hiding
        m_splitterPos = m_split.GetSplitterPosPct();
        // Hide the bottom pane
        m_split.SetSinglePaneMode(SPLIT_PANE_TOP);
    }
}

void CLogViewTabItem2::UpdateDetailView(const std::wstring& text)
{
    m_detailView.SetText(text);
    // Store for later restoration
    m_lastDetailStatus = L"";
    m_lastDetailContent = text;
}

void CLogViewTabItem2::UpdateDetailView(const std::wstring& status, const std::wstring& content)
{
    m_detailView.SetContent(status, content);
    // Store for later restoration
    m_lastDetailStatus = status;
    m_lastDetailContent = content;
}

void CLogViewTabItem2::StoreDetailContent(const std::wstring& status, const std::wstring& content)
{
    m_lastDetailStatus = status;
    m_lastDetailContent = content;
}

void CLogViewTabItem2::RestoreDetailContent()
{
    if (!m_lastDetailContent.empty() || !m_lastDetailStatus.empty())
    {
        m_detailView.SetContent(m_lastDetailStatus, m_lastDetailContent);
    }
}

void CLogViewTabItem2::SetSplitterPos(int pos)
{
    m_splitterPos = pos;
    if (m_detailPaneVisible && m_split.m_hWnd)
    {
        m_split.SetSplitterPosPct(m_splitterPos);
    }
}

void CLogViewTabItem2::UpdateSplitterPos()
{
    if (m_detailPaneVisible && m_split.m_hWnd)
    {
        int currentPos = m_split.GetSplitterPosPct();
        if (currentPos != m_splitterPos)
        {
            m_splitterPos = currentPos;
        }
    }
}

// Button handler implementations for CDetailView
LRESULT CDetailView::OnFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (!m_pMainFrame)
        return 0;
        
    // Get selected text from Scintilla control
    std::string selectedText = m_edit.GetSelectedText();
    
    if (!selectedText.empty())
    {
        // Convert to wide string
        int size = MultiByteToWideChar(CP_UTF8, 0, selectedText.c_str(), -1, nullptr, 0);
        std::wstring wideText(size - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, selectedText.c_str(), -1, &wideText[0], size);
        
        // Trigger find in main view
        m_pMainFrame->FindTextInView(wideText);
    }
    return 0;
}

LRESULT CDetailView::OnFilter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (!m_pMainFrame)
        return 0;
        
    // Get selected text from Scintilla control
    std::string selectedText = m_edit.GetSelectedText();
    
    if (!selectedText.empty())
    {
        // Convert to wide string
        int size = MultiByteToWideChar(CP_UTF8, 0, selectedText.c_str(), -1, nullptr, 0);
        std::wstring wideText(size - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, selectedText.c_str(), -1, &wideText[0], size);
        
        // Open filter dialog with text as include filter
        m_pMainFrame->ShowFilterDialogWithText(wideText);
    }
    return 0;
}

void CDetailView::InitWordWrapState()
{
    // State is now initialized via SetWordWrap() from CLogViewTabItem2
    m_btnWordWrap.SetToggled(m_wordWrap);
    
    // Update Scintilla word wrap setting
    if (m_edit.m_hWnd)
    {
        m_edit.SetWordWrap(m_wordWrap);
    }
}

void CDetailView::InitSyntaxHighlightState()
{
    // State is now initialized via SetSyntaxHighlight() from CLogViewTabItem2
    m_btnSyntax.SetToggled(m_syntaxHighlight);
}

void CDetailView::InitFormatState()
{
    // State is now initialized via SetFormatContent() from CLogViewTabItem2
    m_btnFormat.SetToggled(m_formatContent);
}

void CDetailView::InitLineNumbersState()
{
    // State is now initialized via SetLineNumbers() from CLogViewTabItem2
    m_btnLineNumbers.SetToggled(m_showLineNumbers);
    
    // Apply the line numbers setting to the Scintilla control
    if (m_edit.m_hWnd)
    {
        m_edit.ShowLineNumbers(m_showLineNumbers);
    }
}

LRESULT CDetailView::OnWordWrap(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_wordWrap = !m_wordWrap;
    m_btnWordWrap.SetToggled(m_wordWrap);
    
    // State is now stored locally per-view
    
    // Update Scintilla word wrap setting
    if (m_edit.m_hWnd)
    {
        m_edit.SetWordWrap(m_wordWrap);
    }
    
    return 0;
}

LRESULT CDetailView::OnSyntaxHighlight(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_syntaxHighlight = !m_syntaxHighlight;
    m_btnSyntax.SetToggled(m_syntaxHighlight);
    
    // State is now stored locally per-view
    
    // Re-apply formatting with the stored original text
    // Don't get the current text from the edit control as it may already be formatted
    if (!m_originalText.empty())
    {
        SetText(m_originalText);
    }
    
    return 0;
}

LRESULT CDetailView::OnFormat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_formatContent = !m_formatContent;
    m_btnFormat.SetToggled(m_formatContent);
    
    // State is now stored locally per-view
    
    // Re-apply formatting with the stored original text
    // Don't get the current text from the edit control as it may already be formatted
    if (!m_originalText.empty())
    {
        SetText(m_originalText);
    }
    
    return 0;
}

LRESULT CDetailView::OnLineNumbers(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_showLineNumbers = !m_showLineNumbers;
    m_btnLineNumbers.SetToggled(m_showLineNumbers);
    
    // State is now stored locally per-view
    
    // Update Scintilla line numbers
    if (m_edit.m_hWnd)
    {
        m_edit.ShowLineNumbers(m_showLineNumbers);
    }
    
    return 0;
}

LRESULT CDetailView::OnCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    std::string text = m_edit.GetText();
    
    if (!text.empty() && OpenClipboard())
    {
        EmptyClipboard();
        
        // Convert to wide string
        int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        std::wstring wideText(size - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wideText[0], size);
        
        size_t len = (wideText.length() + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
        if (hMem)
        {
            LPVOID pMem = GlobalLock(hMem);
            if (pMem)
            {
                memcpy(pMem, wideText.c_str(), len);
                GlobalUnlock(hMem);
                SetClipboardData(CF_UNICODETEXT, hMem);
            }
        }
        CloseClipboard();
    }
    return 0;
}

LRESULT CDetailView::OnFont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // Get current font or default
    LOGFONT lf = GetDetailFont();
    
    // Show font dialog
    CFontDialog dlg(&lf, CF_SCREENFONTS);
    if (dlg.DoModal(m_hWnd) == IDOK)
    {
        // Create and set the new font
        CFont newFont;
        newFont.CreateFontIndirect(&dlg.m_lf);
        SetDetailFont(newFont.Detach());
        
        // Notify MainFrame to save the font setting
        if (m_pMainFrame)
        {
            m_pMainFrame->SetDetailFont(dlg.m_lf);
            m_pMainFrame->SaveSettings();
        }
    }
    return 0;
}

LRESULT CDetailView::OnNotify(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
    LPNMHDR pnmh = (LPNMHDR)lParam;
    
    // Check if this is a notification from the Scintilla control
    if (pnmh && pnmh->hwndFrom == m_edit.m_hWnd)
    {
        SCNotification* pscn = (SCNotification*)lParam;
        
        // Handle double-click notification
        if (pscn->nmhdr.code == SCN_DOUBLECLICK)
        {
            // Scintilla automatically selects the word on double-click
            // We just need to get the selected text
            std::string selectedText = m_edit.GetSelectedText();
            
            if (!selectedText.empty())
            {
                // Highlight all occurrences in the detail pane
                m_edit.HighlightAllOccurrences(selectedText);
                
                // Clear the selection so only the highlight indicator is visible
                m_edit.SendEditor(SCI_SETSEL, -1, -1);  // -1, -1 clears selection
                
                // Also highlight in the main grid
                if (m_pMainFrame)
                {
                    // Convert to wide string and pass to main frame
                    CA2W wideText(selectedText.c_str());
                    std::wstring selectedWord(wideText);
                    m_highlightText = selectedWord;  // Store for later use
                    
                    // Use HighlightTextInView to just highlight without moving to next occurrence
                    m_pMainFrame->HighlightTextInView(selectedWord);
                }
            }
            
            bHandled = TRUE;
            return 0;
        }
    }
    
    bHandled = FALSE;
    return 0;
}

void CDetailView::SetText(const std::wstring& text)
{
    // Store the original text
    m_originalText = text;
    
    // Clear any existing highlights when new text is set
    if (m_edit.m_hWnd)
    {
        m_edit.ClearHighlights();
    }
    m_highlightText.clear();
    
    if (m_edit.m_hWnd)
    {
        // Ensure highlighter is initialized
        if (!m_highlighter)
        {
            m_highlighter = std::make_unique<SyntaxHighlighter>();
        }
        
        // Determine if we should format the text
        std::wstring displayText = text;
        std::wstring formatName = L"Plain Text";
        ContentFormat detectedFormat = ContentFormat::PlainText;
        
        if (m_highlighter)
        {
            // Detect format ONCE on the original text
            detectedFormat = m_highlighter->DetectFormat(text);
            
            // Format the text if formatting is enabled
            if (m_formatContent)
            {
                switch (detectedFormat)
                {
                    case ContentFormat::JSON:
                        displayText = m_highlighter->FormatJSON(text);
                        break;
                    case ContentFormat::XML:
                        displayText = m_highlighter->FormatXML(text);
                        break;
                    case ContentFormat::SQL:
                        displayText = m_highlighter->FormatSQL(text);
                        break;
                    default:
                        // No formatting for other types
                        break;
                }
            }
        }
        
        // Set the text in Scintilla
        m_edit.SetText(displayText);
        
        // Apply theme immediately after setting text to ensure dark mode is applied
        bool isDarkTheme = fusion::debugviewpp::ThemeManager::Instance().IsDarkTheme();
        m_edit.ApplyTheme(isDarkTheme);
        m_edit.ConfigurePlainText(isDarkTheme);
        
        // Force the Scintilla control to repaint with the new theme
        m_edit.SendEditor(SCI_COLOURISE, 0, -1);
        m_edit.InvalidateRect(nullptr, TRUE);
        m_edit.UpdateWindow();
        ::InvalidateRect(m_edit.m_hWnd, nullptr, TRUE);
        ::UpdateWindow(m_edit.m_hWnd);
        
        // Configure syntax highlighting if enabled
        
        if (m_syntaxHighlight && m_highlighter)
        {
            // Apply syntax highlighting based on the ORIGINALLY detected format (not re-detected on formatted text)
            switch (detectedFormat)
            {
                case ContentFormat::SQL:
                    formatName = L"SQL";
                    m_edit.ConfigureSQLLexer(isDarkTheme);
                    break;
                case ContentFormat::JSON:
                    formatName = L"JSON";
                    m_edit.ConfigureJSONLexer(isDarkTheme);
                    break;
                case ContentFormat::XML:
                    formatName = L"XML";
                    m_edit.ConfigureXMLLexer(isDarkTheme);
                    break;
                default:
                    formatName = L"Plain Text";
                    m_edit.ConfigurePlainText(isDarkTheme);
                    break;
            }
            
            // Update status line to show detected format when syntax highlighting is on
            if (m_statusLine.m_hWnd)
            {
                std::wstring currentStatus;
                int len = m_statusLine.GetWindowTextLength();
                if (len > 0)
                {
                    currentStatus.resize(len + 1);
                    m_statusLine.GetWindowText(&currentStatus[0], len + 1);
                    currentStatus.resize(len);
                }
                
                // Check if we already have format info in the status
                size_t formatPos = currentStatus.find(L"[Format:");
                if (formatPos != std::wstring::npos)
                {
                    // Replace existing format info
                    currentStatus = currentStatus.substr(0, formatPos) + L"[Format: " + formatName + L"]";
                }
                else
                {
                    // Append format info
                    if (!currentStatus.empty())
                    {
                        currentStatus += L" [Format: " + formatName + L"]";
                    }
                    else
                    {
                        currentStatus = L"Format: " + formatName;
                    }
                }
                m_statusLine.SetWindowText(currentStatus.c_str());
            }
        }
        else
        {
            // Plain text without syntax highlighting
            m_edit.ConfigurePlainText(isDarkTheme);
            
            // Still show format detection even when highlighting is off
            if (m_statusLine.m_hWnd && detectedFormat != ContentFormat::PlainText)
            {
                switch (detectedFormat)
                {
                    case ContentFormat::SQL:
                        formatName = L"SQL (detected)";
                        break;
                    case ContentFormat::JSON:
                        formatName = L"JSON (detected)";
                        break;
                    case ContentFormat::XML:
                        formatName = L"XML (detected)";
                        break;
                    default:
                        break;
                }
                
                std::wstring currentStatus;
                int len = m_statusLine.GetWindowTextLength();
                if (len > 0)
                {
                    currentStatus.resize(len + 1);
                    m_statusLine.GetWindowText(&currentStatus[0], len + 1);
                    currentStatus.resize(len);
                }
                
                // Check if we already have format info in the status
                size_t formatPos = currentStatus.find(L"[Format:");
                size_t detectedPos = currentStatus.find(L"(detected)");
                if (formatPos == std::wstring::npos && detectedPos == std::wstring::npos)
                {
                    // Append format info if not plain text
                    if (detectedFormat != ContentFormat::PlainText)
                    {
                        if (!currentStatus.empty())
                        {
                            currentStatus += L" [" + formatName + L"]";
                        }
                        else
                        {
                            currentStatus = formatName;
                        }
                        m_statusLine.SetWindowText(currentStatus.c_str());
                    }
                }
            }
        }
    }
}

// These methods are no longer needed - Scintilla handles line numbers and syntax highlighting internally
void CDetailView::ApplySyntaxHighlighting(const std::wstring& text)
{
    // This method is now empty - syntax highlighting is handled in SetText() using Scintilla lexers
}

void CDetailView::SetHighlightText(const std::wstring& text)
{
    m_highlightText = text;
    
    if (m_edit.m_hWnd)
    {
        if (text.empty())
        {
            // Clear highlights
            m_edit.ClearHighlights();
        }
        else
        {
            // Convert to UTF-8 and highlight all occurrences
            CW2A utf8Text(text.c_str());
            m_edit.HighlightAllOccurrences(std::string(utf8Text));
        }
    }
}


} // namespace debugviewpp
} // namespace fusion
