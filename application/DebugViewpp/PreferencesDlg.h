// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "resource.h"
#include "CobaltFusion/AtlWinExt.h"
#include "atleverything.h"
#include "DebugViewppLib/ThemeManager.h"
#include "DebugViewppLib/Filter.h"
#include "DebugViewppLib/SqlFormatter.h"
#include "PropertyColorItem.h"
#include "Grid.h"
#include "McpSettings.h"

namespace fusion {
namespace debugviewpp {

struct NewViewDefaults {
    bool showDetailPane = false;
    bool detailWordWrap = true;
    bool detailSyntaxHighlight = true;
    bool detailFormatContent = true;
    bool detailLineNumbers = true;
    int detailSplitterPos = 75;  // percentage
    Theme defaultTheme = Theme::System;
    LOGFONT detailFont = {};
    
    // Default prefix filters
    std::vector<Filter> systemPrefixFilters;
    std::vector<Filter> customPrefixFilters;

    // SQL Formatting Options
    SqlKeywordCase sqlKeywordCase = SqlKeywordCase::Upper;
    SqlIndentType sqlIndentType = SqlIndentType::Spaces;
    int sqlIndentSpaces = 4;             // Number of spaces when using Spaces
    SqlCommaAlignment sqlCommaAlignment = SqlCommaAlignment::Trailing;  // Comma alignment mode
    SqlFormatStyle sqlFormatStyle = SqlFormatStyle::Normal;  // Format style (Compact/Normal/Expanded)
    SqlParenthesesStyle sqlParenthesesStyle = SqlParenthesesStyle::SameLine;  // Parentheses handling for subqueries
    SqlFunctionStyle sqlFunctionStyle = SqlFunctionStyle::Compact;  // Function formatting style
    bool sqlAlignAliases = false;        // Vertically align column aliases
    bool sqlNewlineKeywords = true;      // Line breaks before major keywords

    NewViewDefaults();
};

class CPreferencesDlg : public CDialogImpl<CPreferencesDlg>,
                        public CDialogResize<CPreferencesDlg>
{
public:
    explicit CPreferencesDlg(const NewViewDefaults& defaults, const McpSettings& mcpSettings = McpSettings(), std::wstring mcpStatus = L"");

    NewViewDefaults GetDefaults() const;
    McpSettings GetMcpSettings() const { return m_mcpSettings; }

    enum { IDD = IDD_PREFERENCES };

    BEGIN_DLGRESIZE_MAP(CPreferencesDlg)
        DLGRESIZE_CONTROL(IDC_PREFERENCES_TAB, DLSZ_SIZE_X | DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDAPPLY, DLSZ_MOVE_X | DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

    BEGIN_MSG_MAP(CPreferencesDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        COMMAND_ID_HANDLER(IDOK, OnOk)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_ID_HANDLER(IDAPPLY, OnApply)
        NOTIFY_CODE_HANDLER(TCN_SELCHANGE, OnTabSelChange)
        CHAIN_MSG_MAP(CDialogResize<CPreferencesDlg>)
        DEFAULT_REFLECTION_HANDLER()
    END_MSG_MAP()
    
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnApply(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnTabSelChange(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    
private:
    // Tab page window for New View Defaults
    class CNewViewDefaultsPage : public CDialogImpl<CNewViewDefaultsPage>
    {
    public:
        enum { IDD = IDD_PREFERENCES_NEWVIEW };
        CNewViewDefaultsPage(CPreferencesDlg& parent);

        BEGIN_MSG_MAP(CNewViewDefaultsPage)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            MESSAGE_HANDLER(WM_CTLCOLORDLG, OnCtlColorDlg)
            MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
            COMMAND_ID_HANDLER_EX(IDC_DETAIL_FONT_BTN_NEW, OnFontButton)
            COMMAND_ID_HANDLER_EX(IDC_THEME_SYSTEM_NEW, OnThemeChange)
            COMMAND_ID_HANDLER_EX(IDC_THEME_LIGHT_NEW, OnThemeChange)
            COMMAND_ID_HANDLER_EX(IDC_THEME_DARK_NEW, OnThemeChange)
        END_MSG_MAP()
        
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnCtlColorDlg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        void LoadFromDefaults(const NewViewDefaults& defaults);
        void SaveToDefaults(NewViewDefaults& defaults);
        void OnFontButton(UINT uNotifyCode, int nID, CWindow wndCtl);
        void OnThemeChange(UINT uNotifyCode, int nID, CWindow wndCtl);
        void UpdateFontPreview();
        
    private:
        CPreferencesDlg& m_parent;
        LOGFONT m_currentFont;
    };
    
    // Tab page window for Prefix Defaults
    class CPrefixDefaultsPage : public CDialogImpl<CPrefixDefaultsPage>
    {
    public:
        enum { IDD = IDD_PREFERENCES_PREFIX };
        CPrefixDefaultsPage(CPreferencesDlg& parent);

        BEGIN_MSG_MAP(CPrefixDefaultsPage)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            MESSAGE_HANDLER(WM_CTLCOLORDLG, OnCtlColorDlg)
            MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
            COMMAND_ID_HANDLER_EX(IDC_RESET_PREFIX_DEFAULTS_NEW, OnResetDefaults)
            NOTIFY_CODE_HANDLER_EX(PIN_ADDITEM, OnAddItem)
            NOTIFY_CODE_HANDLER_EX(PIN_CLICK, OnClickItem)
            NOTIFY_CODE_HANDLER_EX(PIN_ITEMCHANGED, OnItemChanged)
            REFLECT_NOTIFICATIONS()
        END_MSG_MAP()
        
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnCtlColorDlg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        void LoadFromDefaults(const NewViewDefaults& defaults);
        void SaveToDefaults(NewViewDefaults& defaults);
        void OnResetDefaults(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnAddItem(NMHDR* pnmh);
        LRESULT OnClickItem(NMHDR* pnmh);
        LRESULT OnItemChanged(NMHDR* pnmh);
        
    private:
        void InitializeSystemFilters();
        void AddFilterToGrid(CPropertyGridCtrl& grid, const Filter& filter);
        Filter GetFilterFromGrid(CPropertyGridCtrl& grid, int item) const;
        void UpdateGridColors(CPropertyGridCtrl& grid, int item) const;
        
        CPreferencesDlg& m_parent;
        CPropertyGridCtrl m_systemGrid;
        CPropertyGridCtrl m_customGrid;
        std::vector<Filter> m_systemFilters;
        std::vector<Filter> m_customFilters;
    };

    // Tab page window for SQL Options
    class CSqlOptionsPage : public CDialogImpl<CSqlOptionsPage>
    {
    public:
        enum { IDD = IDD_PREFERENCES_SQL };
        CSqlOptionsPage(CPreferencesDlg& parent);

        BEGIN_MSG_MAP(CSqlOptionsPage)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            MESSAGE_HANDLER(WM_CTLCOLORDLG, OnCtlColorDlg)
            MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
            COMMAND_ID_HANDLER_EX(IDC_SQL_CASE_UPPER_NEW, OnFormatOptionChange)
            COMMAND_ID_HANDLER_EX(IDC_SQL_CASE_LOWER_NEW, OnFormatOptionChange)
            COMMAND_ID_HANDLER_EX(IDC_SQL_CASE_CAPITALIZE_NEW, OnFormatOptionChange)
            COMMAND_HANDLER_EX(IDC_SQL_INDENT_SPACES_EDIT_NEW, EN_CHANGE, OnFormatOptionChange)
            COMMAND_ID_HANDLER_EX(IDC_SQL_COMMA_INLINE_NEW, OnFormatOptionChange)
            COMMAND_ID_HANDLER_EX(IDC_SQL_COMMA_TRAILING_NEW, OnFormatOptionChange)
            COMMAND_ID_HANDLER_EX(IDC_SQL_COMMA_LEADING_NEW, OnFormatOptionChange)
            COMMAND_ID_HANDLER_EX(IDC_SQL_FORMAT_COMPACT_NEW, OnFormatOptionChange)
            COMMAND_ID_HANDLER_EX(IDC_SQL_FORMAT_NORMAL_NEW, OnFormatOptionChange)
            COMMAND_ID_HANDLER_EX(IDC_SQL_FORMAT_EXPANDED_NEW, OnFormatOptionChange)
            COMMAND_ID_HANDLER_EX(IDC_SQL_PAREN_SAMELINE_NEW, OnFormatOptionChange)
            COMMAND_ID_HANDLER_EX(IDC_SQL_PAREN_NEWLINE_NEW, OnFormatOptionChange)
            COMMAND_ID_HANDLER_EX(IDC_SQL_PAREN_INDENT_NEW, OnFormatOptionChange)
            COMMAND_ID_HANDLER_EX(IDC_SQL_NEWLINE_KEYWORDS_NEW, OnFormatOptionChange)
            NOTIFY_HANDLER_EX(IDC_SQL_INDENT_SPACES_SPIN, UDN_DELTAPOS, OnSpinChange)
        END_MSG_MAP()

        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnCtlColorDlg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        void LoadFromDefaults(const NewViewDefaults& defaults);
        void SaveToDefaults(NewViewDefaults& defaults);
        void OnFormatOptionChange(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnSpinChange(NMHDR* pnmh);
        void UpdatePreview();

    private:
        CPreferencesDlg& m_parent;
        CEdit m_previewEdit;
    };

    // Tab page for the MCP (AI assistant) server
    class CMcpPage : public CDialogImpl<CMcpPage>
    {
    public:
        enum { IDD = IDD_PREFERENCES_MCP };
        explicit CMcpPage(CPreferencesDlg& parent);

        BEGIN_MSG_MAP(CMcpPage)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            COMMAND_ID_HANDLER_EX(IDC_MCP_GENERATE_TOKEN, OnGenerateToken)
            COMMAND_ID_HANDLER_EX(IDC_MCP_COPY_CONFIG, OnCopyConfig)
            COMMAND_HANDLER_EX(IDC_MCP_PORT, EN_CHANGE, OnChange)
            COMMAND_HANDLER_EX(IDC_MCP_TOKEN, EN_CHANGE, OnChange)
            COMMAND_ID_HANDLER_EX(IDC_MCP_EXPORT_BROWSE, OnBrowseExport)
            COMMAND_ID_HANDLER_EX(IDC_MCP_EXPORT_DOCS, OnExportModeChange)
            COMMAND_ID_HANDLER_EX(IDC_MCP_EXPORT_CUSTOM, OnExportModeChange)
        END_MSG_MAP()

        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        void Load(const McpSettings& settings, const std::wstring& status);
        bool Save(McpSettings& settings);
        void OnGenerateToken(UINT uNotifyCode, int nID, CWindow wndCtl);
        void OnCopyConfig(UINT uNotifyCode, int nID, CWindow wndCtl);
        void OnChange(UINT uNotifyCode, int nID, CWindow wndCtl);
        void OnBrowseExport(UINT uNotifyCode, int nID, CWindow wndCtl);
        void OnExportModeChange(UINT uNotifyCode, int nID, CWindow wndCtl);
        void UpdateExportControls();
        void UpdateUrl();

    private:
        CPreferencesDlg& m_parent;
    };

    void CreateTabPages();
    void LoadControlsFromDefaults();
    void SaveControlsToDefaults();
    bool ApplyChanges();
    void InitializeDefaultFont();
    
    CTabCtrl m_tabCtrl;
    CNewViewDefaultsPage m_newViewDefaultsPage;
    CPrefixDefaultsPage m_prefixDefaultsPage;
    CSqlOptionsPage m_sqlOptionsPage;
    CMcpPage m_mcpPage;
    NewViewDefaults m_defaults;
    McpSettings m_mcpSettings;
    std::wstring m_mcpStatus;
    SIZE m_border;
};

} // namespace debugviewpp
} // namespace fusion