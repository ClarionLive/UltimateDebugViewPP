// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "PreferencesDlg.h"
#include "CobaltFusion/Str.h"
#include "DebugViewppLib/SqlFormatter.h"
#include <string>
#include <exception>
#include <commctrl.h>
#include <bcrypt.h>
#include <algorithm>
#include <iterator>
#include <shobjidl.h>

namespace fusion {
namespace debugviewpp {

// NewViewDefaults constructor
NewViewDefaults::NewViewDefaults()
{
    // Initialize default font (Consolas 9pt)
    memset(&detailFont, 0, sizeof(LOGFONT));
    detailFont.lfHeight = -12; // 9pt at 96 DPI
    detailFont.lfWeight = FW_NORMAL;
    detailFont.lfCharSet = DEFAULT_CHARSET;
    detailFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    detailFont.lfClipPrecision = OUT_DEFAULT_PRECIS;
    detailFont.lfQuality = DEFAULT_QUALITY;
    detailFont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    wcscpy_s(detailFont.lfFaceName, L"Consolas");
    
    // Initialize default system prefix filters
    systemPrefixFilters.clear();
    systemPrefixFilters.emplace_back("[Events]", MatchType::Simple, FilterType::Highlight, RGB(200, 220, 255), RGB(0, 0, 0), true);
    systemPrefixFilters.emplace_back("[Open]", MatchType::Simple, FilterType::Highlight, RGB(200, 255, 200), RGB(0, 0, 0), true);
    systemPrefixFilters.emplace_back("[Close]", MatchType::Simple, FilterType::Highlight, RGB(255, 255, 200), RGB(0, 0, 0), true);
    systemPrefixFilters.emplace_back("[Trace]", MatchType::Simple, FilterType::Highlight, RGB(230, 230, 230), RGB(0, 0, 0), true);
    systemPrefixFilters.emplace_back("[Queue]", MatchType::Simple, FilterType::Highlight, RGB(200, 255, 255), RGB(0, 0, 0), true);
    systemPrefixFilters.emplace_back("[Group]", MatchType::Simple, FilterType::Highlight, RGB(230, 200, 255), RGB(0, 0, 0), true);
    systemPrefixFilters.emplace_back("[Error]", MatchType::Simple, FilterType::Highlight, RGB(255, 0, 0), RGB(255, 255, 255), true);
    systemPrefixFilters.emplace_back("[SQL]", MatchType::Simple, FilterType::Highlight, RGB(200, 230, 255), RGB(0, 0, 0), true);
}

// CPreferencesDlg implementation
CPreferencesDlg::CPreferencesDlg(const NewViewDefaults& defaults, const McpSettings& mcpSettings, std::wstring mcpStatus)
    : m_defaults(defaults)
    , m_newViewDefaultsPage(*this)
    , m_prefixDefaultsPage(*this)
    , m_sqlOptionsPage(*this)
    , m_mcpPage(*this)
    , m_mcpSettings(mcpSettings)
    , m_mcpStatus(std::move(mcpStatus))
{
    m_hWnd = NULL;  // Ensure dialog window handle is NULL
    m_border = { 0, 0 };
}

NewViewDefaults CPreferencesDlg::GetDefaults() const
{
    return m_defaults;
}

LRESULT CPreferencesDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

    // Center the dialog on the parent window
    CenterWindow(GetParent());

    // Set the icon
    HICON hIcon = static_cast<HICON>(LoadImage(ModuleHelper::GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
    if (hIcon)
    {
        SetIcon(hIcon, FALSE);
    }

    // Initialize dialog resize
    DlgResize_Init();

    // Get tab control - verify it exists
    m_tabCtrl.m_hWnd = NULL;  // Reset first
    m_tabCtrl = GetDlgItem(IDC_PREFERENCES_TAB);
    if (!m_tabCtrl.IsWindow())
    {
        MessageBox(L"Failed to get tab control", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }


    // Add tab pages
    TCITEM tci = {};
    tci.mask = TCIF_TEXT;

    tci.pszText = const_cast<LPWSTR>(L"New View Defaults");
    m_tabCtrl.InsertItem(0, &tci);

    tci.pszText = const_cast<LPWSTR>(L"Prefix Defaults");
    m_tabCtrl.InsertItem(1, &tci);

    tci.pszText = const_cast<LPWSTR>(L"SQL Options");
    m_tabCtrl.InsertItem(2, &tci);

    tci.pszText = const_cast<LPWSTR>(L"AI / MCP");
    m_tabCtrl.InsertItem(3, &tci);

    // Create tab pages
    CreateTabPages();

    // Load initial values
    LoadControlsFromDefaults();

    // Calculate borders for resize
    CRect tabRect, displayRect;
    m_tabCtrl.GetClientRect(&tabRect);
    displayRect = tabRect;
    m_tabCtrl.AdjustRect(FALSE, &displayRect); // FALSE to get display area from client area
    m_border.cx = displayRect.left - tabRect.left;
    m_border.cy = displayRect.top - tabRect.top;

    return TRUE;  // Let the system set the focus
}

LRESULT CPreferencesDlg::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);
    if (m_newViewDefaultsPage.IsWindow())
        m_newViewDefaultsPage.DestroyWindow();
    if (m_prefixDefaultsPage.IsWindow())
        m_prefixDefaultsPage.DestroyWindow();
    if (m_sqlOptionsPage.IsWindow())
        m_sqlOptionsPage.DestroyWindow();
    if (m_mcpPage.IsWindow())
        m_mcpPage.DestroyWindow();
    return 0;
}

LRESULT CPreferencesDlg::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);
    if (m_tabCtrl.IsWindow())
    {
        // Get the display area of the tab control
        CRect tabRect;
        m_tabCtrl.GetWindowRect(&tabRect);
        m_tabCtrl.AdjustRect(FALSE, &tabRect);
        m_tabCtrl.ScreenToClient(&tabRect);

        // Resize all tab pages to fit the display area
        if (m_newViewDefaultsPage.IsWindow())
        {
            m_newViewDefaultsPage.MoveWindow(&tabRect);
        }

        if (m_prefixDefaultsPage.IsWindow())
        {
            m_prefixDefaultsPage.MoveWindow(&tabRect);
        }

        if (m_sqlOptionsPage.IsWindow())
        {
            m_sqlOptionsPage.MoveWindow(&tabRect);
        }

        if (m_mcpPage.IsWindow())
        {
            m_mcpPage.MoveWindow(&tabRect);
        }
    }
    return 0;
}

LRESULT CPreferencesDlg::OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(wNotifyCode);
    UNREFERENCED_PARAMETER(wID);
    UNREFERENCED_PARAMETER(hWndCtl);
    UNREFERENCED_PARAMETER(bHandled);
    
    if (ApplyChanges())
    {
        EndDialog(IDOK);
    }
    return 0;
}

LRESULT CPreferencesDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(wNotifyCode);
    UNREFERENCED_PARAMETER(wID);
    UNREFERENCED_PARAMETER(hWndCtl);
    UNREFERENCED_PARAMETER(bHandled);
    
    EndDialog(IDCANCEL);
    return 0;
}

LRESULT CPreferencesDlg::OnApply(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(wNotifyCode);
    UNREFERENCED_PARAMETER(wID);
    UNREFERENCED_PARAMETER(hWndCtl);
    UNREFERENCED_PARAMETER(bHandled);
    
    ApplyChanges();
    return 0;
}

LRESULT CPreferencesDlg::OnTabSelChange(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(idCtrl);
    UNREFERENCED_PARAMETER(pnmh);
    UNREFERENCED_PARAMETER(bHandled);
    
    int selected = m_tabCtrl.GetCurSel();
    
    // Show/hide appropriate tab page
    if (m_newViewDefaultsPage.IsWindow())
    {
        m_newViewDefaultsPage.ShowWindow(selected == 0 ? SW_SHOW : SW_HIDE);
    }
    
    if (m_prefixDefaultsPage.IsWindow())
    {
        m_prefixDefaultsPage.ShowWindow(selected == 1 ? SW_SHOW : SW_HIDE);
    }

    if (m_sqlOptionsPage.IsWindow())
    {
        m_sqlOptionsPage.ShowWindow(selected == 2 ? SW_SHOW : SW_HIDE);
    }

    if (m_mcpPage.IsWindow())
    {
        m_mcpPage.ShowWindow(selected == 3 ? SW_SHOW : SW_HIDE);
    }

    return 0;
}

void CPreferencesDlg::CreateTabPages()
{
    // Ensure tab control is valid
    if (!m_tabCtrl.IsWindow())
        return;

    // Get the display area of the tab control
    CRect tabRect;
    m_tabCtrl.GetWindowRect(&tabRect);
    m_tabCtrl.AdjustRect(FALSE, &tabRect);
    m_tabCtrl.ScreenToClient(&tabRect);

    // Create dialog pages as children of the tab control (not the dialog)
    m_newViewDefaultsPage.Create(m_tabCtrl, tabRect);
    if (!m_newViewDefaultsPage.IsWindow())
    {
        MessageBox(L"Failed to create New View Defaults page", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    m_newViewDefaultsPage.MoveWindow(&tabRect);
    m_newViewDefaultsPage.ShowWindow(SW_SHOW);

    // Create the Prefix Defaults page
    m_prefixDefaultsPage.Create(m_tabCtrl, tabRect);
    if (!m_prefixDefaultsPage.IsWindow())
    {
        MessageBox(L"Failed to create Prefix Defaults page", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    m_prefixDefaultsPage.MoveWindow(&tabRect);
    m_prefixDefaultsPage.ShowWindow(SW_HIDE);

    // Create the SQL Options page
    m_sqlOptionsPage.Create(m_tabCtrl, tabRect);
    if (!m_sqlOptionsPage.IsWindow())
    {
        MessageBox(L"Failed to create SQL Options page", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    m_sqlOptionsPage.MoveWindow(&tabRect);
    m_sqlOptionsPage.ShowWindow(SW_HIDE);

    // Create the AI / MCP page
    m_mcpPage.Create(m_tabCtrl, tabRect);
    if (m_mcpPage.IsWindow())
    {
        m_mcpPage.MoveWindow(&tabRect);
        m_mcpPage.ShowWindow(SW_HIDE);
        m_mcpPage.Load(m_mcpSettings, m_mcpStatus);
    }
}

void CPreferencesDlg::LoadControlsFromDefaults()
{
    if (m_newViewDefaultsPage.IsWindow())
    {
        m_newViewDefaultsPage.LoadFromDefaults(m_defaults);
    }
    if (m_prefixDefaultsPage.IsWindow())
    {
        m_prefixDefaultsPage.LoadFromDefaults(m_defaults);
    }
    if (m_sqlOptionsPage.IsWindow())
    {
        m_sqlOptionsPage.LoadFromDefaults(m_defaults);
    }
}

void CPreferencesDlg::SaveControlsToDefaults()
{
    if (m_newViewDefaultsPage.IsWindow())
    {
        m_newViewDefaultsPage.SaveToDefaults(m_defaults);
    }
    if (m_prefixDefaultsPage.IsWindow())
    {
        m_prefixDefaultsPage.SaveToDefaults(m_defaults);
    }
    if (m_sqlOptionsPage.IsWindow())
    {
        m_sqlOptionsPage.SaveToDefaults(m_defaults);
    }
}

bool CPreferencesDlg::ApplyChanges()
{
    if (m_mcpPage.IsWindow() && !m_mcpPage.Save(m_mcpSettings))
    {
        m_tabCtrl.SetCurSel(3);
        BOOL handled = TRUE;
        OnTabSelChange(0, nullptr, handled);
        return false;
    }
    SaveControlsToDefaults();
    return true;
}

// CNewViewDefaultsPage implementation
CPreferencesDlg::CNewViewDefaultsPage::CNewViewDefaultsPage(CPreferencesDlg& parent)
    : m_parent(parent)
{
    m_hWnd = NULL;  // Ensure window handle is NULL before Create
}


LRESULT CPreferencesDlg::CNewViewDefaultsPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);
    return TRUE;
}

void CPreferencesDlg::CNewViewDefaultsPage::LoadFromDefaults(const NewViewDefaults& defaults)
{
    m_currentFont = defaults.detailFont;

    ::SendMessage(GetDlgItem(IDC_SHOW_DETAIL_PANE_NEW), BM_SETCHECK, defaults.showDetailPane ? BST_CHECKED : BST_UNCHECKED, 0);
    ::SetWindowText(GetDlgItem(IDC_SPLITTER_POS), std::to_wstring(defaults.detailSplitterPos).c_str());
    ::SendMessage(GetDlgItem(IDC_DETAIL_WORDWRAP_NEW), BM_SETCHECK, defaults.detailWordWrap ? BST_CHECKED : BST_UNCHECKED, 0);
    ::SendMessage(GetDlgItem(IDC_DETAIL_SYNTAX_HIGHLIGHT), BM_SETCHECK, defaults.detailSyntaxHighlight ? BST_CHECKED : BST_UNCHECKED, 0);
    ::SendMessage(GetDlgItem(IDC_DETAIL_FORMAT_CONTENT), BM_SETCHECK, defaults.detailFormatContent ? BST_CHECKED : BST_UNCHECKED, 0);
    ::SendMessage(GetDlgItem(IDC_DETAIL_LINE_NUMBERS), BM_SETCHECK, defaults.detailLineNumbers ? BST_CHECKED : BST_UNCHECKED, 0);

    // Set theme radio buttons
    ::SendMessage(GetDlgItem(IDC_THEME_SYSTEM_NEW), BM_SETCHECK, defaults.defaultTheme == Theme::System ? BST_CHECKED : BST_UNCHECKED, 0);
    ::SendMessage(GetDlgItem(IDC_THEME_LIGHT_NEW), BM_SETCHECK, defaults.defaultTheme == Theme::Light ? BST_CHECKED : BST_UNCHECKED, 0);
    ::SendMessage(GetDlgItem(IDC_THEME_DARK_NEW), BM_SETCHECK, defaults.defaultTheme == Theme::Dark ? BST_CHECKED : BST_UNCHECKED, 0);

    UpdateFontPreview();
}

void CPreferencesDlg::CNewViewDefaultsPage::SaveToDefaults(NewViewDefaults& defaults)
{
    defaults.showDetailPane = (::SendMessage(GetDlgItem(IDC_SHOW_DETAIL_PANE_NEW), BM_GETCHECK, 0, 0) == BST_CHECKED);

    // Get splitter position from edit control
    wchar_t buffer[32];
    ::GetWindowText(GetDlgItem(IDC_SPLITTER_POS), buffer, 32);
    defaults.detailSplitterPos = _wtoi(buffer);

    defaults.detailWordWrap = (::SendMessage(GetDlgItem(IDC_DETAIL_WORDWRAP_NEW), BM_GETCHECK, 0, 0) == BST_CHECKED);
    defaults.detailSyntaxHighlight = (::SendMessage(GetDlgItem(IDC_DETAIL_SYNTAX_HIGHLIGHT), BM_GETCHECK, 0, 0) == BST_CHECKED);
    defaults.detailFormatContent = (::SendMessage(GetDlgItem(IDC_DETAIL_FORMAT_CONTENT), BM_GETCHECK, 0, 0) == BST_CHECKED);
    defaults.detailLineNumbers = (::SendMessage(GetDlgItem(IDC_DETAIL_LINE_NUMBERS), BM_GETCHECK, 0, 0) == BST_CHECKED);
    defaults.detailFont = m_currentFont;

    if (::SendMessage(GetDlgItem(IDC_THEME_SYSTEM_NEW), BM_GETCHECK, 0, 0) == BST_CHECKED)
        defaults.defaultTheme = Theme::System;
    else if (::SendMessage(GetDlgItem(IDC_THEME_LIGHT_NEW), BM_GETCHECK, 0, 0) == BST_CHECKED)
        defaults.defaultTheme = Theme::Light;
    else if (::SendMessage(GetDlgItem(IDC_THEME_DARK_NEW), BM_GETCHECK, 0, 0) == BST_CHECKED)
        defaults.defaultTheme = Theme::Dark;
}

void CPreferencesDlg::CNewViewDefaultsPage::OnFontButton(UINT uNotifyCode, int nID, CWindow wndCtl)
{
    UNREFERENCED_PARAMETER(uNotifyCode);
    UNREFERENCED_PARAMETER(nID);
    UNREFERENCED_PARAMETER(wndCtl);
    
    CHOOSEFONT cf = {};
    cf.lStructSize = sizeof(CHOOSEFONT);
    cf.hwndOwner = m_hWnd;
    cf.lpLogFont = &m_currentFont;
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_FIXEDPITCHONLY;
    
    if (ChooseFont(&cf))
    {
        UpdateFontPreview();
    }
}

void CPreferencesDlg::CNewViewDefaultsPage::OnThemeChange(UINT uNotifyCode, int nID, CWindow wndCtl)
{
    UNREFERENCED_PARAMETER(uNotifyCode);
    UNREFERENCED_PARAMETER(nID);
    UNREFERENCED_PARAMETER(wndCtl);
    
    // Theme radio button change - nothing special needed
}

void CPreferencesDlg::CNewViewDefaultsPage::UpdateFontPreview()
{
    HWND previewCtrl = GetDlgItem(IDC_DETAIL_FONT_PREVIEW_NEW);
    if (previewCtrl)
    {
        // Calculate point size from logical height
        HDC hdc = GetDC();
        int pointSize = -MulDiv(m_currentFont.lfHeight, 72, GetDeviceCaps(hdc, LOGPIXELSY));
        ReleaseDC(hdc);

        CString previewText;
        previewText.Format(L"Font Preview: %s %dpt", m_currentFont.lfFaceName, pointSize);
        ::SetWindowText(previewCtrl, previewText);
    }
}

LRESULT CPreferencesDlg::CNewViewDefaultsPage::OnCtlColorDlg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    bHandled = FALSE;
    return 0;
}

LRESULT CPreferencesDlg::CNewViewDefaultsPage::OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    bHandled = FALSE;
    return 0;
}

// CPrefixDefaultsPage implementation
CPreferencesDlg::CPrefixDefaultsPage::CPrefixDefaultsPage(CPreferencesDlg& parent)
    : m_parent(parent)
{
    m_hWnd = NULL;  // Ensure window handle is NULL before Create
    // Ensure the grid window handles are initialized to NULL
    // This prevents IsWindow() from checking garbage values
    m_systemGrid.m_hWnd = NULL;
    m_customGrid.m_hWnd = NULL;
}

LRESULT CPreferencesDlg::CPrefixDefaultsPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

    // Get grid controls from dialog resource
    m_systemGrid.SubclassWindow(GetDlgItem(IDC_SYSTEM_PREFIX_GRID_NEW));
    m_customGrid.SubclassWindow(GetDlgItem(IDC_CUSTOM_PREFIX_GRID_NEW));

    // Initialize system grid columns if not already done
    if (m_systemGrid.IsWindow() && m_systemGrid.GetColumnCount() == 0)
    {
        m_systemGrid.InsertColumn(0, L"Enable", LVCFMT_LEFT, 50, 0, -1, 0);
        m_systemGrid.InsertColumn(1, L"Prefix", LVCFMT_LEFT, 120, 0, -1, 1);
        m_systemGrid.InsertColumn(2, L"Type", LVCFMT_LEFT, 80, 0, -1, 2);
        m_systemGrid.InsertColumn(3, L"Background", LVCFMT_LEFT, 80, 0, -1, 3);
        m_systemGrid.InsertColumn(4, L"Foreground", LVCFMT_LEFT, 80, 0, -1, 4);
    }

    // Initialize custom grid columns if not already done
    if (m_customGrid.IsWindow() && m_customGrid.GetColumnCount() == 0)
    {
        m_customGrid.InsertColumn(0, L"Enable", LVCFMT_LEFT, 50, 0, -1, 0);
        m_customGrid.InsertColumn(1, L"Prefix", LVCFMT_LEFT, 120, 0, -1, 1);
        m_customGrid.InsertColumn(2, L"Type", LVCFMT_LEFT, 80, 0, -1, 2);
        m_customGrid.InsertColumn(3, L"Background", LVCFMT_LEFT, 80, 0, -1, 3);
        m_customGrid.InsertColumn(4, L"Foreground", LVCFMT_LEFT, 80, 0, -1, 4);
        m_customGrid.InsertColumn(5, L"Remove", LVCFMT_LEFT, 60, 0, -1, 5);

        // Set extended grid styles for custom grid to allow adding items
        m_customGrid.SetExtendedGridStyle(PGS_EX_SINGLECLICKEDIT | PGS_EX_ADDITEMATEND);
    }

    // Don't call InitializeSystemFilters here - it should already be done by LoadFromDefaults
    // If LoadFromDefaults wasn't called yet (shouldn't happen), initialize now
    if (m_systemFilters.empty() && m_parent.GetDefaults().systemPrefixFilters.empty())
    {
        InitializeSystemFilters();
    }

    // Populate grids with the loaded filters
    for (const auto& filter : m_systemFilters)
    {
        AddFilterToGrid(m_systemGrid, filter);
    }

    for (const auto& filter : m_customFilters)
    {
        AddFilterToGrid(m_customGrid, filter);
    }

    return TRUE;
}

void CPreferencesDlg::CPrefixDefaultsPage::InitializeSystemFilters()
{
    // Initialize system filters with defaults
    m_systemFilters.clear();
    m_systemFilters.emplace_back("[Events]", MatchType::Simple, FilterType::Highlight, RGB(200, 220, 255), RGB(0, 0, 0), true);
    m_systemFilters.emplace_back("[Open]", MatchType::Simple, FilterType::Highlight, RGB(200, 255, 200), RGB(0, 0, 0), true);
    m_systemFilters.emplace_back("[Close]", MatchType::Simple, FilterType::Highlight, RGB(255, 255, 200), RGB(0, 0, 0), true);
    m_systemFilters.emplace_back("[Trace]", MatchType::Simple, FilterType::Highlight, RGB(230, 230, 230), RGB(0, 0, 0), true);
    m_systemFilters.emplace_back("[Queue]", MatchType::Simple, FilterType::Highlight, RGB(200, 255, 255), RGB(0, 0, 0), true);
    m_systemFilters.emplace_back("[Group]", MatchType::Simple, FilterType::Highlight, RGB(230, 200, 255), RGB(0, 0, 0), true);
    m_systemFilters.emplace_back("[Error]", MatchType::Simple, FilterType::Highlight, RGB(255, 0, 0), RGB(255, 255, 255), true);
    m_systemFilters.emplace_back("[SQL]", MatchType::Simple, FilterType::Highlight, RGB(200, 230, 255), RGB(0, 0, 0), true);
}

void CPreferencesDlg::CPrefixDefaultsPage::LoadFromDefaults(const NewViewDefaults& defaults)
{
    // Load system filters from defaults or use built-in defaults
    if (!defaults.systemPrefixFilters.empty())
    {
        m_systemFilters = defaults.systemPrefixFilters;
    }
    else
    {
        InitializeSystemFilters();
    }

    // Load custom filters from defaults
    m_customFilters = defaults.customPrefixFilters;

    // Only populate grids if they have been created
    if (m_systemGrid.IsWindow())
    {
        m_systemGrid.DeleteAllItems();
        for (const auto& filter : m_systemFilters)
        {
            AddFilterToGrid(m_systemGrid, filter);
        }
    }

    if (m_customGrid.IsWindow())
    {
        m_customGrid.DeleteAllItems();
        for (const auto& filter : m_customFilters)
        {
            AddFilterToGrid(m_customGrid, filter);
        }
    }
}

void CPreferencesDlg::CPrefixDefaultsPage::SaveToDefaults(NewViewDefaults& defaults)
{
    // Only save from grids if they exist
    if (m_systemGrid.IsWindow())
    {
        // Update system filters from grid
        defaults.systemPrefixFilters.clear();
        int systemCount = m_systemGrid.GetItemCount();
        for (int i = 0; i < systemCount; ++i)
        {
            defaults.systemPrefixFilters.push_back(GetFilterFromGrid(m_systemGrid, i));
        }
    }
    else
    {
        // Use the cached filters if grids don't exist yet
        defaults.systemPrefixFilters = m_systemFilters;
    }

    if (m_customGrid.IsWindow())
    {
        // Update custom filters from grid
        defaults.customPrefixFilters.clear();
        int customCount = m_customGrid.GetItemCount();
        for (int i = 0; i < customCount; ++i)
        {
            defaults.customPrefixFilters.push_back(GetFilterFromGrid(m_customGrid, i));
        }
    }
    else
    {
        // Use the cached filters if grids don't exist yet
        defaults.customPrefixFilters = m_customFilters;
    }
}

void CPreferencesDlg::CPrefixDefaultsPage::AddFilterToGrid(CPropertyGridCtrl& grid, const Filter& filter)
{
    int item = grid.GetItemCount();
    
    // Create properties
    auto pEnable = PropCreateCheckButton(L"", filter.enable);
    HPROPERTY pPrefix = nullptr;
    auto pType = PropCreateReadOnlyItem(L"", L"Highlight"); // Fixed type for prefix filters
    auto pBgColor = PropCreateColorItem(L"Background Color", filter.bgColor);
    auto pFgColor = PropCreateColorItem(L"Foreground Color", filter.fgColor);
    
    // For system grid, make prefix read-only
    if (&grid == &m_systemGrid)
    {
        pPrefix = PropCreateReadOnlyItem(L"", WStr(filter.text).str().c_str());
    }
    else
    {
        pPrefix = PropCreateSimple(L"", WStr(filter.text).str().c_str());
    }
    
    grid.InsertItem(item, pEnable);
    grid.SetSubItem(item, 1, pPrefix);
    grid.SetSubItem(item, 2, pType);
    grid.SetSubItem(item, 3, pBgColor);
    grid.SetSubItem(item, 4, pFgColor);
    
    // Add remove button for custom grid
    if (&grid == &m_customGrid)
    {
        grid.SetSubItem(item, 5, PropCreateReadOnlyItem(L"", L"x"));
    }
    
    UpdateGridColors(grid, item);
}

Filter CPreferencesDlg::CPrefixDefaultsPage::GetFilterFromGrid(CPropertyGridCtrl& grid, int item) const
{
    // Get enable state
    CComVariant enableVal;
    if (auto* enableProp = dynamic_cast<CPropertyCheckButtonItem*>(grid.GetProperty(item, 0)))
    {
        enableProp->GetValue(&enableVal);
    }
    bool enable = enableVal.boolVal != VARIANT_FALSE;
    
    // Get prefix text
    std::wstring prefix;
    if (auto* prefixProp = grid.GetProperty(item, 1))
    {
        CComVariant prefixVal;
        prefixProp->GetValue(&prefixVal);
        if (prefixVal.vt == VT_BSTR && prefixVal.bstrVal != nullptr)
        {
            prefix = prefixVal.bstrVal;
        }
    }
    
    // Get background color
    COLORREF bgColor = RGB(255, 255, 255);
    if (auto* bgProp = dynamic_cast<CPropertyColorItem*>(grid.GetProperty(item, 3)))
    {
        bgColor = bgProp->GetColor();
    }
    
    // Get foreground color
    COLORREF fgColor = RGB(0, 0, 0);
    if (auto* fgProp = dynamic_cast<CPropertyColorItem*>(grid.GetProperty(item, 4)))
    {
        fgColor = fgProp->GetColor();
    }
    
    return Filter(Str(prefix), MatchType::Simple, FilterType::Highlight, bgColor, fgColor, enable);
}

void CPreferencesDlg::CPrefixDefaultsPage::UpdateGridColors(CPropertyGridCtrl& grid, int item) const
{
    // Update the prefix text color based on the selected colors
    if (auto* prefixProp = grid.GetProperty(item, 1))
    {
        if (auto* bgProp = dynamic_cast<CPropertyColorItem*>(grid.GetProperty(item, 3)))
        {
            if (auto* fgProp = dynamic_cast<CPropertyColorItem*>(grid.GetProperty(item, 4)))
            {
                auto bgColor = bgProp->GetColor();
                auto fgColor = fgProp->GetColor();
                
                // Try to set colors on both edit and read-only item types
                if (auto* editItem = dynamic_cast<CPropertyEditItem*>(prefixProp))
                {
                    editItem->SetBkColor(bgColor);
                    editItem->SetTextColor(fgColor);
                }
                else if (auto* readOnlyItem = dynamic_cast<CPropertyReadOnlyItem*>(prefixProp))
                {
                    readOnlyItem->SetBkColor(bgColor);
                    readOnlyItem->SetTextColor(fgColor);
                }
            }
        }
    }
}

LRESULT CPreferencesDlg::CPrefixDefaultsPage::OnAddItem(NMHDR* pnmh)
{
    auto& nmhdr = *reinterpret_cast<NMPROPERTYITEM*>(pnmh);
    
    // Only allow adding items to custom grid
    if (nmhdr.hdr.hwndFrom == m_customGrid.m_hWnd)
    {
        Filter filter("", MatchType::Simple, FilterType::Highlight, RGB(255, 255, 255), RGB(0, 0, 0), true);
        AddFilterToGrid(m_customGrid, filter);
        m_customFilters.push_back(filter);
        m_customGrid.SendMessage(WM_KEYDOWN, VK_F2, 0);
    }
    
    return 0;
}

LRESULT CPreferencesDlg::CPrefixDefaultsPage::OnClickItem(NMHDR* pnmh)
{
    auto& nmhdr = *reinterpret_cast<NMPROPERTYITEM*>(pnmh);
    
    int iItem;
    int iSubItem;
    
    // Check which grid the click is in
    CPropertyGridCtrl* activeGrid = nullptr;
    if (nmhdr.hdr.hwndFrom == m_systemGrid.m_hWnd)
    {
        activeGrid = &m_systemGrid;
    }
    else if (nmhdr.hdr.hwndFrom == m_customGrid.m_hWnd)
    {
        activeGrid = &m_customGrid;
    }
    
    if (activeGrid && activeGrid->FindProperty(nmhdr.prop, iItem, iSubItem) != 0)
    {
        // Handle remove button click for custom grid
        if (activeGrid == &m_customGrid && iSubItem == 5) // Remove column
        {
            activeGrid->DeleteItem(iItem);
            if (iItem < static_cast<int>(m_customFilters.size()))
            {
                m_customFilters.erase(m_customFilters.begin() + iItem);
            }
            return TRUE;
        }
    }
    
    // Let other columns handle their own click events
    return FALSE;
}

LRESULT CPreferencesDlg::CPrefixDefaultsPage::OnItemChanged(NMHDR* pnmh)
{
    auto& nmhdr = *reinterpret_cast<NMPROPERTYITEM*>(pnmh);
    
    int iItem;
    int iSubItem;
    
    // Check which grid the change is in
    CPropertyGridCtrl* activeGrid = nullptr;
    if (nmhdr.hdr.hwndFrom == m_systemGrid.m_hWnd)
    {
        activeGrid = &m_systemGrid;
    }
    else if (nmhdr.hdr.hwndFrom == m_customGrid.m_hWnd)
    {
        activeGrid = &m_customGrid;
    }
    
    if (activeGrid && activeGrid->FindProperty(nmhdr.prop, iItem, iSubItem) != 0)
    {
        // Update colors when background or foreground changes
        if (iSubItem == 3 || iSubItem == 4) // Background or foreground column
        {
            UpdateGridColors(*activeGrid, iItem);
        }
    }
    
    return 0;
}

void CPreferencesDlg::CPrefixDefaultsPage::OnResetDefaults(UINT uNotifyCode, int nID, CWindow wndCtl)
{
    UNREFERENCED_PARAMETER(uNotifyCode);
    UNREFERENCED_PARAMETER(nID);
    UNREFERENCED_PARAMETER(wndCtl);

    // Reset system filters to defaults
    InitializeSystemFilters();

    // Clear custom filters
    m_customFilters.clear();

    // Refresh system grid
    if (m_systemGrid.IsWindow())
    {
        m_systemGrid.DeleteAllItems();
        for (const auto& filter : m_systemFilters)
        {
            AddFilterToGrid(m_systemGrid, filter);
        }
    }

    // Refresh custom grid
    if (m_customGrid.IsWindow())
    {
        m_customGrid.DeleteAllItems();
    }
}

LRESULT CPreferencesDlg::CPrefixDefaultsPage::OnCtlColorDlg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    bHandled = FALSE;
    return 0;
}

LRESULT CPreferencesDlg::CPrefixDefaultsPage::OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    bHandled = FALSE;
    return 0;
}

// CSqlOptionsPage implementation
CPreferencesDlg::CSqlOptionsPage::CSqlOptionsPage(CPreferencesDlg& parent)
    : m_parent(parent)
{
    m_hWnd = NULL;  // Ensure window handle is NULL before Create
}

LRESULT CPreferencesDlg::CSqlOptionsPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

    // Get preview edit control from dialog resource
    m_previewEdit = GetDlgItem(IDC_SQL_PREVIEW_NEW);

    // Set a monospace font for the preview if the control exists
    if (m_previewEdit.IsWindow())
    {
        CLogFont lf;
        lf.lfHeight = -12;
        wcscpy_s(lf.lfFaceName, L"Consolas");
        CFont font;
        font.CreateFontIndirect(&lf);
        m_previewEdit.SetFont(font.Detach());
    }

    // Initialize the spin control
    CUpDownCtrl spinCtrl(GetDlgItem(IDC_SQL_INDENT_SPACES_SPIN));
    if (spinCtrl.IsWindow())
    {
        // Manually set the buddy window
        CEdit editCtrl(GetDlgItem(IDC_SQL_INDENT_SPACES_EDIT_NEW));
        if (editCtrl.IsWindow())
        {
            spinCtrl.SetBuddy(editCtrl);
        }
        spinCtrl.SetRange(1, 20);
        spinCtrl.SetPos(4); // Default to 4 spaces
    }

    return TRUE;
}

void CPreferencesDlg::CSqlOptionsPage::LoadFromDefaults(const NewViewDefaults& defaults)
{
    // Set keyword case radio button
    switch (defaults.sqlKeywordCase)
    {
    case SqlKeywordCase::Upper:
        CheckDlgButton(IDC_SQL_CASE_UPPER_NEW, BST_CHECKED);
        break;
    case SqlKeywordCase::Lower:
        CheckDlgButton(IDC_SQL_CASE_LOWER_NEW, BST_CHECKED);
        break;
    case SqlKeywordCase::Capitalize:
        CheckDlgButton(IDC_SQL_CASE_CAPITALIZE_NEW, BST_CHECKED);
        break;
    }

    // Set indentation spaces value (always using spaces now, no tabs)
    int spaces = (defaults.sqlIndentType == SqlIndentType::Tab) ? 4 : defaults.sqlIndentSpaces;
    SetDlgItemInt(IDC_SQL_INDENT_SPACES_EDIT_NEW, spaces, FALSE);

    // Update spin control
    CUpDownCtrl spinCtrl(GetDlgItem(IDC_SQL_INDENT_SPACES_SPIN));
    if (spinCtrl.IsWindow())
    {
        spinCtrl.SetPos(spaces);
    }

    // Set comma alignment radio button
    switch (defaults.sqlCommaAlignment)
    {
    case SqlCommaAlignment::Inline:
        CheckDlgButton(IDC_SQL_COMMA_INLINE_NEW, BST_CHECKED);
        break;
    case SqlCommaAlignment::Trailing:
        CheckDlgButton(IDC_SQL_COMMA_TRAILING_NEW, BST_CHECKED);
        break;
    case SqlCommaAlignment::Leading:
        CheckDlgButton(IDC_SQL_COMMA_LEADING_NEW, BST_CHECKED);
        break;
    }

    // Set Format Style radio buttons
    switch (defaults.sqlFormatStyle) {
    case SqlFormatStyle::Compact:
        CheckDlgButton(IDC_SQL_FORMAT_COMPACT_NEW, BST_CHECKED);
        break;
    case SqlFormatStyle::Normal:
        CheckDlgButton(IDC_SQL_FORMAT_NORMAL_NEW, BST_CHECKED);
        break;
    case SqlFormatStyle::Expanded:
        CheckDlgButton(IDC_SQL_FORMAT_EXPANDED_NEW, BST_CHECKED);
        break;
    }

    // Set Parentheses Style radio buttons
    switch (defaults.sqlParenthesesStyle) {
    case SqlParenthesesStyle::SameLine:
        CheckDlgButton(IDC_SQL_PAREN_SAMELINE_NEW, BST_CHECKED);
        break;
    case SqlParenthesesStyle::NewLine:
        CheckDlgButton(IDC_SQL_PAREN_NEWLINE_NEW, BST_CHECKED);
        break;
    case SqlParenthesesStyle::IndentContent:
        CheckDlgButton(IDC_SQL_PAREN_INDENT_NEW, BST_CHECKED);
        break;
    }

    // Set checkboxes
    CheckDlgButton(IDC_SQL_NEWLINE_KEYWORDS_NEW, defaults.sqlNewlineKeywords ? BST_CHECKED : BST_UNCHECKED);

    UpdatePreview();
}

void CPreferencesDlg::CSqlOptionsPage::SaveToDefaults(NewViewDefaults& defaults)
{
    // Get keyword case
    if (IsDlgButtonChecked(IDC_SQL_CASE_UPPER_NEW))
        defaults.sqlKeywordCase = SqlKeywordCase::Upper;
    else if (IsDlgButtonChecked(IDC_SQL_CASE_LOWER_NEW))
        defaults.sqlKeywordCase = SqlKeywordCase::Lower;
    else if (IsDlgButtonChecked(IDC_SQL_CASE_CAPITALIZE_NEW))
        defaults.sqlKeywordCase = SqlKeywordCase::Capitalize;

    // Get indentation (always using spaces now)
    defaults.sqlIndentType = SqlIndentType::Spaces;
    defaults.sqlIndentSpaces = GetDlgItemInt(IDC_SQL_INDENT_SPACES_EDIT_NEW, NULL, FALSE);
    // Ensure it's within valid range
    if (defaults.sqlIndentSpaces < 1) defaults.sqlIndentSpaces = 1;
    if (defaults.sqlIndentSpaces > 20) defaults.sqlIndentSpaces = 20;

    // Get comma alignment
    if (IsDlgButtonChecked(IDC_SQL_COMMA_INLINE_NEW))
        defaults.sqlCommaAlignment = SqlCommaAlignment::Inline;
    else if (IsDlgButtonChecked(IDC_SQL_COMMA_TRAILING_NEW))
        defaults.sqlCommaAlignment = SqlCommaAlignment::Trailing;
    else if (IsDlgButtonChecked(IDC_SQL_COMMA_LEADING_NEW))
        defaults.sqlCommaAlignment = SqlCommaAlignment::Leading;

    // Get format style
    if (IsDlgButtonChecked(IDC_SQL_FORMAT_COMPACT_NEW))
        defaults.sqlFormatStyle = SqlFormatStyle::Compact;
    else if (IsDlgButtonChecked(IDC_SQL_FORMAT_NORMAL_NEW))
        defaults.sqlFormatStyle = SqlFormatStyle::Normal;
    else if (IsDlgButtonChecked(IDC_SQL_FORMAT_EXPANDED_NEW))
        defaults.sqlFormatStyle = SqlFormatStyle::Expanded;

    // Get parentheses style
    if (IsDlgButtonChecked(IDC_SQL_PAREN_SAMELINE_NEW))
        defaults.sqlParenthesesStyle = SqlParenthesesStyle::SameLine;
    else if (IsDlgButtonChecked(IDC_SQL_PAREN_NEWLINE_NEW))
        defaults.sqlParenthesesStyle = SqlParenthesesStyle::NewLine;
    else if (IsDlgButtonChecked(IDC_SQL_PAREN_INDENT_NEW))
        defaults.sqlParenthesesStyle = SqlParenthesesStyle::IndentContent;

    // Get checkboxes
    defaults.sqlNewlineKeywords = IsDlgButtonChecked(IDC_SQL_NEWLINE_KEYWORDS_NEW) == BST_CHECKED;
}

void CPreferencesDlg::CSqlOptionsPage::OnFormatOptionChange(UINT uNotifyCode, int nID, CWindow wndCtl)
{
    UNREFERENCED_PARAMETER(uNotifyCode);
    UNREFERENCED_PARAMETER(nID);
    UNREFERENCED_PARAMETER(wndCtl);

    UpdatePreview();
}

LRESULT CPreferencesDlg::CSqlOptionsPage::OnSpinChange(NMHDR* pnmh)
{
    UNREFERENCED_PARAMETER(pnmh);
    UpdatePreview();
    return 0;
}

void CPreferencesDlg::CSqlOptionsPage::UpdatePreview()
{
    // Sample SQL query for preview
    std::wstring sampleSql = L"SELECT customer_id, first_name, last_name, email, phone_number ";
    sampleSql += L"FROM customers WHERE country = 'USA' AND state = 'CA' ";
    sampleSql += L"ORDER BY last_name, first_name -- Sort by name";

    // Get current settings to build SqlFormatOptions
    SqlFormatOptions options;

    // Get keyword case
    if (IsDlgButtonChecked(IDC_SQL_CASE_UPPER_NEW))
        options.keywordCase = SqlKeywordCase::Upper;
    else if (IsDlgButtonChecked(IDC_SQL_CASE_LOWER_NEW))
        options.keywordCase = SqlKeywordCase::Lower;
    else if (IsDlgButtonChecked(IDC_SQL_CASE_CAPITALIZE_NEW))
        options.keywordCase = SqlKeywordCase::Capitalize;

    // Get indentation (always using spaces now)
    options.indentType = SqlIndentType::Spaces;
    options.indentSpaces = GetDlgItemInt(IDC_SQL_INDENT_SPACES_EDIT_NEW, NULL, FALSE);
    if (options.indentSpaces < 1) options.indentSpaces = 1;
    if (options.indentSpaces > 20) options.indentSpaces = 20;

    // Get comma alignment
    if (IsDlgButtonChecked(IDC_SQL_COMMA_INLINE_NEW))
        options.commaAlignment = SqlCommaAlignment::Inline;
    else if (IsDlgButtonChecked(IDC_SQL_COMMA_TRAILING_NEW))
        options.commaAlignment = SqlCommaAlignment::Trailing;
    else if (IsDlgButtonChecked(IDC_SQL_COMMA_LEADING_NEW))
        options.commaAlignment = SqlCommaAlignment::Leading;

    // Get format style
    if (IsDlgButtonChecked(IDC_SQL_FORMAT_COMPACT_NEW))
        options.formatStyle = SqlFormatStyle::Compact;
    else if (IsDlgButtonChecked(IDC_SQL_FORMAT_NORMAL_NEW))
        options.formatStyle = SqlFormatStyle::Normal;
    else if (IsDlgButtonChecked(IDC_SQL_FORMAT_EXPANDED_NEW))
        options.formatStyle = SqlFormatStyle::Expanded;

    // Get parentheses style
    if (IsDlgButtonChecked(IDC_SQL_PAREN_SAMELINE_NEW))
        options.parenthesesStyle = SqlParenthesesStyle::SameLine;
    else if (IsDlgButtonChecked(IDC_SQL_PAREN_NEWLINE_NEW))
        options.parenthesesStyle = SqlParenthesesStyle::NewLine;
    else if (IsDlgButtonChecked(IDC_SQL_PAREN_INDENT_NEW))
        options.parenthesesStyle = SqlParenthesesStyle::IndentContent;

    // Get other formatting options
    options.newlineBeforeKeywords = IsDlgButtonChecked(IDC_SQL_NEWLINE_KEYWORDS_NEW) == BST_CHECKED;

    // Use FormatSQL to format the sample SQL with the fsqlf.dll
    std::wstring formatted;
    try
    {
        // Log the attempt to format
        FILE* log = fopen("C:\\Dev\\UltimateDebugViewPP\\sql_preview_debug.log", "w");
        if (log)
        {
            fwprintf(log, L"Attempting to format SQL:\n%s\n", sampleSql.c_str());
            fwprintf(log, L"Options: case=%d, indent=%d spaces=%d, comma_align=%d, newline=%d\n",
                (int)options.keywordCase, (int)options.indentType, options.indentSpaces,
                (int)options.commaAlignment, options.newlineBeforeKeywords ? 1 : 0);
            fclose(log);
        }

        formatted = FormatSQL(sampleSql, options);

        // Convert Unix line endings (\n) to Windows line endings (\r\n) for the edit control
        std::wstring windowsFormatted;
        windowsFormatted.reserve(formatted.size() + formatted.size() / 10); // Reserve extra space for \r characters
        for (size_t i = 0; i < formatted.size(); ++i)
        {
            if (formatted[i] == L'\n' && (i == 0 || formatted[i - 1] != L'\r'))
            {
                windowsFormatted += L"\r\n";
            }
            else
            {
                windowsFormatted += formatted[i];
            }
        }
        formatted = windowsFormatted;

        // Log the result
        log = fopen("C:\\Dev\\UltimateDebugViewPP\\sql_preview_debug.log", "a");
        if (log)
        {
            fwprintf(log, L"\nFormatted result:\n%s\n", formatted.c_str());
            fclose(log);
        }
    }
    catch (const std::exception& e)
    {
        // Log the error
        FILE* log = fopen("C:\\Dev\\UltimateDebugViewPP\\sql_preview_debug.log", "a");
        if (log)
        {
            fprintf(log, "\nException caught: %s\n", e.what());
            fclose(log);
        }
        // If formatting fails, show the original SQL
        formatted = sampleSql;
    }
    catch (...)
    {
        // Log the error
        FILE* log = fopen("C:\\Dev\\UltimateDebugViewPP\\sql_preview_debug.log", "a");
        if (log)
        {
            fprintf(log, "\nUnknown exception caught\n");
            fclose(log);
        }
        // If formatting fails, show the original SQL
        formatted = sampleSql;
    }

    // Set preview text
    m_previewEdit.SetWindowText(formatted.c_str());
}

LRESULT CPreferencesDlg::CSqlOptionsPage::OnCtlColorDlg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    bHandled = FALSE;
    return 0;
}

LRESULT CPreferencesDlg::CSqlOptionsPage::OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    bHandled = FALSE;
    return 0;
}

// CMcpPage implementation
CPreferencesDlg::CMcpPage::CMcpPage(CPreferencesDlg& parent)
    : m_parent(parent)
{
    m_hWnd = NULL;
}

LRESULT CPreferencesDlg::CMcpPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);
    CEdit(GetDlgItem(IDC_MCP_PORT)).SetLimitText(5);
    CEdit(GetDlgItem(IDC_MCP_TOKEN)).SetLimitText(128);
    return TRUE;
}

void CPreferencesDlg::CMcpPage::Load(const McpSettings& settings, const std::wstring& status)
{
    CheckDlgButton(IDC_MCP_ENABLE, settings.enabled ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemInt(IDC_MCP_PORT, static_cast<UINT>(settings.port), FALSE);
    SetDlgItemText(IDC_MCP_TOKEN, settings.token.c_str());
    CheckDlgButton(IDC_MCP_MASK_SECRETS, settings.maskSecrets ? BST_CHECKED : BST_UNCHECKED);
    CheckRadioButton(IDC_MCP_EXPORT_DOCS, IDC_MCP_EXPORT_CUSTOM, settings.exportMode == McpExportMode::Folder ? IDC_MCP_EXPORT_CUSTOM : IDC_MCP_EXPORT_DOCS);
    SetDlgItemText(IDC_MCP_EXPORT_FOLDER, settings.exportFolder.c_str());
    SetDlgItemText(IDC_MCP_EXPORT_SUBFOLDER, settings.exportSubfolder.c_str());
    CheckDlgButton(IDC_MCP_EXPORT_PERAPP, settings.exportPerApp ? BST_CHECKED : BST_UNCHECKED);
    UpdateExportControls();
    std::wstring text = L"Status: " + (status.empty() ? std::wstring(L"not running") : status) + L"   (changes take effect when you press OK)";
    SetDlgItemText(IDC_MCP_STATUS, text.c_str());
    UpdateUrl();
}

void CPreferencesDlg::CMcpPage::UpdateExportControls()
{
    bool custom = IsDlgButtonChecked(IDC_MCP_EXPORT_CUSTOM) == BST_CHECKED;
    GetDlgItem(IDC_MCP_EXPORT_FOLDER).EnableWindow(custom);
    GetDlgItem(IDC_MCP_EXPORT_BROWSE).EnableWindow(custom);
}

void CPreferencesDlg::CMcpPage::OnExportModeChange(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    UpdateExportControls();
}

void CPreferencesDlg::CMcpPage::OnBrowseExport(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CComPtr<IFileOpenDialog> dialog;
    if (FAILED(dialog.CoCreateInstance(CLSID_FileOpenDialog)))
        return;
    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    dialog->SetTitle(L"Folder for logs the AI saves");
    if (FAILED(dialog->Show(m_hWnd)))
        return;
    CComPtr<IShellItem> item;
    PWSTR path = nullptr;
    if (SUCCEEDED(dialog->GetResult(&item)) && SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path != nullptr)
    {
        SetDlgItemText(IDC_MCP_EXPORT_FOLDER, path);
        ::CoTaskMemFree(path);
    }
}

bool CPreferencesDlg::CMcpPage::Save(McpSettings& settings)
{
    BOOL ok = FALSE;
    UINT port = GetDlgItemInt(IDC_MCP_PORT, &ok, FALSE);
    if (!ok || port < 1 || port > 65535)
    {
        MessageBox(L"The MCP port must be a number from 1 to 65535.", L"Preferences", MB_OK | MB_ICONWARNING);
        GetDlgItem(IDC_MCP_PORT).SetFocus();
        return false;
    }

    wchar_t buffer[160] = {};
    GetDlgItemText(IDC_MCP_TOKEN, buffer, 159);
    std::wstring token(buffer);
    token.erase(0, token.find_first_not_of(L" \t"));
    token.erase(token.find_last_not_of(L" \t") + 1);
    bool printable = std::all_of(token.begin(), token.end(), [](wchar_t c) { return c > L' ' && c < 127 && c != L'"' && c != L'\\'; });
    if (!printable)
    {
        MessageBox(L"The access token may only contain letters, digits and plain punctuation (no spaces, quotes or backslashes).", L"Preferences", MB_OK | MB_ICONWARNING);
        GetDlgItem(IDC_MCP_TOKEN).SetFocus();
        return false;
    }

    bool enabled = IsDlgButtonChecked(IDC_MCP_ENABLE) == BST_CHECKED;
    if (token.empty())
    {
        // A token is always required; create one so enabling the server is one click.
        token = GenerateMcpToken();
        SetDlgItemText(IDC_MCP_TOKEN, token.c_str());
    }
    else if (token.size() < McpMinTokenLength)
    {
        MessageBox(L"The access token must be at least 16 characters. Press Generate for a random one.", L"Preferences", MB_OK | MB_ICONWARNING);
        GetDlgItem(IDC_MCP_TOKEN).SetFocus();
        return false;
    }

    auto exportMode = IsDlgButtonChecked(IDC_MCP_EXPORT_CUSTOM) == BST_CHECKED ? McpExportMode::Folder : McpExportMode::Documents;
    wchar_t folderBuffer[MAX_PATH * 2] = {};
    GetDlgItemText(IDC_MCP_EXPORT_FOLDER, folderBuffer, static_cast<int>(std::size(folderBuffer)) - 1);
    std::wstring exportFolder(folderBuffer);
    wchar_t subBuffer[160] = {};
    GetDlgItemText(IDC_MCP_EXPORT_SUBFOLDER, subBuffer, 159);
    std::wstring exportSubfolder(subBuffer);
    for (auto* text : {&exportFolder, &exportSubfolder})
    {
        text->erase(0, text->find_first_not_of(L" \t"));
        text->erase(text->find_last_not_of(L" \t") + 1);
    }
    if (exportMode == McpExportMode::Folder)
    {
        DWORD attributes = ::GetFileAttributesW(exportFolder.c_str());
        if (exportFolder.empty() || attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            MessageBox(L"Choose an existing folder for the logs the AI saves (or pick another option).", L"Preferences", MB_OK | MB_ICONWARNING);
            GetDlgItem(IDC_MCP_EXPORT_FOLDER).SetFocus();
            return false;
        }
    }
    if (!exportSubfolder.empty() && !IsPlainMcpName(exportSubfolder))
    {
        MessageBox(L"The sub-folder must be a single plain folder name such as DebugLogs (letters, digits, space, . _ -).", L"Preferences", MB_OK | MB_ICONWARNING);
        GetDlgItem(IDC_MCP_EXPORT_SUBFOLDER).SetFocus();
        return false;
    }

    settings.enabled = enabled;
    settings.port = static_cast<int>(port);
    settings.token = token;
    settings.maskSecrets = IsDlgButtonChecked(IDC_MCP_MASK_SECRETS) == BST_CHECKED;
    settings.exportMode = exportMode;
    settings.exportFolder = exportFolder;
    settings.exportSubfolder = exportSubfolder;
    settings.exportPerApp = IsDlgButtonChecked(IDC_MCP_EXPORT_PERAPP) == BST_CHECKED;
    return true;
}

void CPreferencesDlg::CMcpPage::OnGenerateToken(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    auto token = GenerateMcpToken();
    if (token.empty())
    {
        MessageBox(L"Could not generate a random token.", L"Preferences", MB_OK | MB_ICONERROR);
        return;
    }
    SetDlgItemText(IDC_MCP_TOKEN, token.c_str());
}

void CPreferencesDlg::CMcpPage::OnChange(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    UpdateUrl();
}

void CPreferencesDlg::CMcpPage::UpdateUrl()
{
    BOOL ok = FALSE;
    UINT port = GetDlgItemInt(IDC_MCP_PORT, &ok, FALSE);
    std::wstring text = L"Endpoint: http://127.0.0.1:" + (ok ? std::to_wstring(port) : std::wstring(L"?")) + L"/mcp";
    SetDlgItemText(IDC_MCP_URL, text.c_str());
}

void CPreferencesDlg::CMcpPage::OnCopyConfig(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    McpSettings settings;
    if (!Save(settings))
        return;

    std::wstring json = L"{\r\n  \"mcpServers\": {\r\n    \"ultimate-debugview\": {\r\n      \"type\": \"http\",\r\n      \"url\": \"http://127.0.0.1:" +
                        std::to_wstring(settings.port) + L"/mcp\"";
    if (!settings.token.empty())
    {
        json += L",\r\n      \"headers\": { \"Authorization\": \"Bearer " + settings.token + L"\" }";
    }
    json += L"\r\n    }\r\n  }\r\n}\r\n";

    if (!OpenClipboard())
        return;
    EmptyClipboard();
    size_t bytes = (json.size() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (hMem != nullptr)
    {
        memcpy(::GlobalLock(hMem), json.c_str(), bytes);
        ::GlobalUnlock(hMem);
        if (::SetClipboardData(CF_UNICODETEXT, hMem) == nullptr)
            ::GlobalFree(hMem);
    }
    CloseClipboard();
    SetDlgItemText(IDC_MCP_STATUS, L"Copied. Paste it into .mcp.json in your project (or merge the \"ultimate-debugview\" entry).");
}

} // namespace debugviewpp
} // namespace fusion