// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "atleverything.h"
#include "ModernToolbar.h"
#include "resource.h"
#include <atlmisc.h>

namespace fusion {
namespace debugviewpp {

// CModernToolbarButton implementation
LRESULT CModernToolbarButton::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CPaintDC dc(m_hWnd);
    CRect rcClient;
    GetClientRect(&rcClient);
    
    // Draw background
    COLORREF bgColor = RGB(255, 255, 255);
    if (m_isPressed)
        bgColor = RGB(200, 200, 200);
    else if (m_isHovered)
        bgColor = RGB(230, 230, 230);
    
    CBrush brush;
    brush.CreateSolidBrush(bgColor);
    dc.FillRect(&rcClient, brush);
    
    // Draw border on hover/press
    if (m_isHovered || m_isPressed)
    {
        CPen pen;
        pen.CreatePen(PS_SOLID, 1, RGB(150, 150, 150));
        CPenHandle oldPen = dc.SelectPen(pen);
        CBrushHandle oldBrush = dc.SelectBrush((HBRUSH)GetStockObject(NULL_BRUSH));
        dc.Rectangle(&rcClient);
        dc.SelectPen(oldPen);
        dc.SelectBrush(oldBrush);
    }
    
    // Draw icon
    if (m_hIcon)
    {
        int x = (rcClient.Width() - ICON_SIZE) / 2;
        int y = PADDING;
        dc.DrawIconEx(x, y, m_hIcon, ICON_SIZE, ICON_SIZE);
    }
    
    // Draw text
    if (!m_text.empty())
    {
        CRect rcText = rcClient;
        rcText.top = PADDING + ICON_SIZE + 2;
        rcText.bottom -= PADDING;
        
        dc.SetBkMode(TRANSPARENT);
        dc.SetTextColor(RGB(0, 0, 0));
        
        CFont font;
        font.CreatePointFont(85, L"Segoe UI");
        CFontHandle oldFont = dc.SelectFont(font);
        
        dc.DrawText(m_text.c_str(), -1, &rcText, 
                   DT_CENTER | DT_TOP | DT_WORDBREAK | DT_END_ELLIPSIS);
        
        dc.SelectFont(oldFont);
    }
    
    // Draw dropdown arrow if needed
    if (m_hasDropdown)
    {
        CRect rcArrow(rcClient.right - 12, rcClient.bottom - 12, 
                     rcClient.right - 4, rcClient.bottom - 4);
        
        // Draw small triangle
        POINT pts[3] = {
            {rcArrow.left, rcArrow.top},
            {rcArrow.right, rcArrow.top},
            {rcArrow.left + rcArrow.Width()/2, rcArrow.bottom}
        };
        
        CBrush arrowBrush;
        arrowBrush.CreateSolidBrush(RGB(100, 100, 100));
        CBrushHandle oldBrush = dc.SelectBrush(arrowBrush);
        dc.Polygon(pts, 3);
        dc.SelectBrush(oldBrush);
    }
    
    return 0;
}

LRESULT CModernToolbarButton::OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    return 1; // We handle background in OnPaint
}

LRESULT CModernToolbarButton::OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (!m_isHovered)
    {
        m_isHovered = true;
        
        // Track mouse leave
        TRACKMOUSEEVENT tme = {0};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        TrackMouseEvent(&tme);
        
        Invalidate();
    }
    return 0;
}

LRESULT CModernToolbarButton::OnMouseLeave(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    m_isHovered = false;
    m_isPressed = false;
    Invalidate();
    return 0;
}

LRESULT CModernToolbarButton::OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    m_isPressed = true;
    Invalidate();
    bHandled = FALSE; // Let button handle the click
    return 0;
}

LRESULT CModernToolbarButton::OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    m_isPressed = false;
    Invalidate();
    bHandled = FALSE; // Let button handle the click
    return 0;
}

// CModernToolbarGroup implementation
void CModernToolbarGroup::AddButton(UINT nID, const std::wstring& text, HICON hIcon, const std::wstring& tooltip)
{
    ButtonInfo info;
    info.nID = nID;
    info.text = text;
    info.hIcon = hIcon;
    info.tooltip = tooltip;
    info.isSeparator = false;
    m_buttons.push_back(info);
}

void CModernToolbarGroup::AddSeparator()
{
    ButtonInfo info;
    info.isSeparator = true;
    m_buttons.push_back(info);
}

LRESULT CModernToolbarGroup::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // Create button controls
    int x = GROUP_PADDING;
    int y = TITLE_HEIGHT;
    
    for (const auto& info : m_buttons)
    {
        if (info.isSeparator)
        {
            x += 8; // Separator width
            continue;
        }
        
        auto button = std::make_unique<CModernToolbarButton>();
        RECT rc = {x, y, x + CModernToolbarButton::BUTTON_WIDTH, 
                  y + CModernToolbarButton::BUTTON_HEIGHT};
        button->Create(m_hWnd, rc, nullptr, 
                      WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                      0, info.nID);
        button->SetIcon(info.hIcon);
        button->SetText(info.text);
        button->SetTooltip(info.tooltip);
        
        m_buttonControls.push_back(std::move(button));
        x += CModernToolbarButton::BUTTON_WIDTH + 4;
    }
    
    return 0;
}

LRESULT CModernToolbarGroup::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // Re-layout buttons if needed
    return 0;
}

LRESULT CModernToolbarGroup::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CPaintDC dc(m_hWnd);
    CRect rcClient;
    GetClientRect(&rcClient);
    
    // Draw background
    dc.FillRect(&rcClient, COLOR_WINDOW + 1);
    
    // Draw group title
    if (!m_title.empty())
    {
        CRect rcTitle(0, rcClient.Height() - 16, rcClient.Width(), rcClient.Height());
        
        dc.SetBkMode(TRANSPARENT);
        dc.SetTextColor(RGB(100, 100, 100));
        
        CFont font;
        font.CreatePointFont(75, L"Segoe UI");
        CFontHandle oldFont = dc.SelectFont(font);
        
        dc.DrawText(m_title.c_str(), -1, &rcTitle, DT_CENTER | DT_BOTTOM | DT_SINGLELINE);
        
        dc.SelectFont(oldFont);
    }
    
    // Draw separators
    int x = GROUP_PADDING;
    for (const auto& info : m_buttons)
    {
        if (info.isSeparator)
        {
            CPen pen;
            pen.CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
            CPenHandle oldPen = dc.SelectPen(pen);
            
            dc.MoveTo(x + 4, TITLE_HEIGHT + 8);
            dc.LineTo(x + 4, TITLE_HEIGHT + CModernToolbarButton::BUTTON_HEIGHT - 8);
            
            dc.SelectPen(oldPen);
            x += 8;
        }
        else
        {
            x += CModernToolbarButton::BUTTON_WIDTH + 4;
        }
    }
    
    return 0;
}

LRESULT CModernToolbarGroup::OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    return 1; // We handle background in OnPaint
}

// CModernToolbar implementation
void CModernToolbar::AddTab(const std::wstring& name)
{
    TabInfo tab;
    tab.name = name;
    m_tabs.push_back(std::move(tab));
    
    if (m_tabCtrl.m_hWnd)
    {
        m_tabCtrl.InsertItem(static_cast<int>(m_tabs.size() - 1), name.c_str());
    }
}

void CModernToolbar::SetActiveTab(int index)
{
    if (index >= 0 && index < static_cast<int>(m_tabs.size()))
    {
        m_activeTab = index;
        if (m_tabCtrl.m_hWnd)
        {
            m_tabCtrl.SetCurSel(index);
        }
        ShowTab(index);
    }
}

CModernToolbarGroup* CModernToolbar::AddGroup(const std::wstring& title)
{
    if (m_activeTab < 0 || m_activeTab >= static_cast<int>(m_tabs.size()))
        return nullptr;
    
    auto group = std::make_unique<CModernToolbarGroup>();
    group->SetTitle(title);
    
    CModernToolbarGroup* pGroup = group.get();
    m_tabs[m_activeTab].groups.push_back(std::move(group));
    
    if (m_hWnd)
    {
        // Create the group window
        CRect rcGroup;
        // Position will be set in LayoutGroups
        pGroup->Create(m_hWnd, rcGroup, nullptr, WS_CHILD | WS_VISIBLE);
        LayoutGroups();
    }
    
    return pGroup;
}

void CModernToolbar::AddQuickAccessButton(UINT nID, HICON hIcon, const std::wstring& tooltip)
{
    QuickAccessButton qab;
    qab.nID = nID;
    qab.hIcon = hIcon;
    qab.tooltip = tooltip;
    qab.button = std::make_unique<CButton>();
    
    if (m_hWnd)
    {
        int x = 4 + static_cast<int>(m_quickAccessButtons.size()) * (QAT_BUTTON_SIZE + 2);
        RECT rc = {x, 2, x + QAT_BUTTON_SIZE, 2 + QAT_BUTTON_SIZE};
        qab.button->Create(m_hWnd, rc, nullptr, 
                          WS_CHILD | WS_VISIBLE | BS_ICON | BS_FLAT,
                          0, nID);
        qab.button->SetIcon(hIcon);
        
        // TODO: Add tooltip
    }
    
    m_quickAccessButtons.push_back(std::move(qab));
}

LRESULT CModernToolbar::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // Create tab control
    CRect rcTab(0, QAT_BUTTON_SIZE + 4, 0, QAT_BUTTON_SIZE + 4 + TAB_HEIGHT);
    m_tabCtrl.Create(m_hWnd, rcTab, nullptr, 
                     WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_TABS);
    
    // Add existing tabs
    for (size_t i = 0; i < m_tabs.size(); ++i)
    {
        m_tabCtrl.InsertItem(static_cast<int>(i), m_tabs[i].name.c_str());
    }
    
    // Create groups for active tab
    if (m_activeTab >= 0 && m_activeTab < static_cast<int>(m_tabs.size()))
    {
        ShowTab(m_activeTab);
    }
    
    return 0;
}

LRESULT CModernToolbar::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    int cx = LOWORD(lParam);
    
    // Resize tab control
    if (m_tabCtrl.m_hWnd)
    {
        m_tabCtrl.SetWindowPos(nullptr, 0, QAT_BUTTON_SIZE + 4, 
                               cx, TAB_HEIGHT, SWP_NOZORDER);
    }
    
    // Re-layout groups
    LayoutGroups();
    
    return 0;
}

LRESULT CModernToolbar::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CPaintDC dc(m_hWnd);
    CRect rcClient;
    GetClientRect(&rcClient);
    
    // Draw background
    CBrush brush;
    brush.CreateSolidBrush(RGB(245, 245, 245));
    dc.FillRect(&rcClient, brush);
    
    // Draw QAT background
    CRect rcQAT(0, 0, rcClient.Width(), QAT_BUTTON_SIZE + 4);
    CBrush qatBrush;
    qatBrush.CreateSolidBrush(RGB(235, 235, 235));
    dc.FillRect(&rcQAT, qatBrush);
    
    // Draw separator line
    CPen pen;
    pen.CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    CPenHandle oldPen = dc.SelectPen(pen);
    dc.MoveTo(0, QAT_BUTTON_SIZE + 3);
    dc.LineTo(rcClient.Width(), QAT_BUTTON_SIZE + 3);
    dc.SelectPen(oldPen);
    
    return 0;
}

LRESULT CModernToolbar::OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    return 1; // We handle background in OnPaint
}

LRESULT CModernToolbar::OnButtonClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
    // Forward to parent
    ::SendMessage(GetParent(), WM_COMMAND, MAKEWPARAM(wID, BN_CLICKED), 0);
    bHandled = FALSE;
    return 0;
}

LRESULT CModernToolbar::OnTabSelChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
    int newTab = m_tabCtrl.GetCurSel();
    if (newTab != m_activeTab)
    {
        ShowTab(newTab);
        m_activeTab = newTab;
    }
    return 0;
}

void CModernToolbar::SetMinimized(bool minimized)
{
    m_isMinimized = minimized;
    // TODO: Implement minimize/restore
}

int CModernToolbar::GetHeight() const
{
    if (m_isMinimized)
        return MINIMIZED_HEIGHT;
    return TOOLBAR_HEIGHT + QAT_BUTTON_SIZE + TAB_HEIGHT + 8;
}

void CModernToolbar::LayoutGroups()
{
    if (m_activeTab < 0 || m_activeTab >= static_cast<int>(m_tabs.size()))
        return;
    
    CRect rcClient;
    GetClientRect(&rcClient);
    
    int x = 4;
    int y = QAT_BUTTON_SIZE + 4 + TAB_HEIGHT;
    int groupHeight = TOOLBAR_HEIGHT;
    
    for (auto& group : m_tabs[m_activeTab].groups)
    {
        if (group && group->m_hWnd)
        {
            // Calculate group width based on buttons
            int groupWidth = 120; // Default width
            
            group->SetWindowPos(nullptr, x, y, groupWidth, groupHeight, 
                               SWP_NOZORDER | SWP_SHOWWINDOW);
            x += groupWidth + 8;
        }
    }
}

void CModernToolbar::ShowTab(int index)
{
    // Hide all groups
    for (auto& tab : m_tabs)
    {
        for (auto& group : tab.groups)
        {
            if (group && group->m_hWnd)
                group->ShowWindow(SW_HIDE);
        }
    }
    
    // Show groups for selected tab
    if (index >= 0 && index < static_cast<int>(m_tabs.size()))
    {
        for (auto& group : m_tabs[index].groups)
        {
            if (group && group->m_hWnd)
                group->ShowWindow(SW_SHOW);
        }
    }
}

// CModernToolbarManager implementation
CModernToolbarManager::CModernToolbarManager(HWND hWndParent) : m_hWndParent(hWndParent)
{
}

CModernToolbarManager::~CModernToolbarManager()
{
    // Clean up icons
    for (HICON hIcon : m_icons)
    {
        if (hIcon)
            ::DestroyIcon(hIcon);
    }
}

bool CModernToolbarManager::Create()
{
    // Get parent window size to create toolbar with proper width
    CRect rcParent;
    ::GetClientRect(m_hWndParent, &rcParent);
    CRect rcToolbar(0, 0, rcParent.Width(), m_toolbar.GetHeight());
    HWND hWnd = m_toolbar.Create(m_hWndParent, rcToolbar, nullptr,
                                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    return hWnd != nullptr;
}

void CModernToolbarManager::SetupDefaultLayout()
{
    // Add tabs
    m_toolbar.AddTab(L"Home");
    m_toolbar.AddTab(L"Log");
    m_toolbar.AddTab(L"View");
    
    // Setup Home tab
    m_toolbar.SetActiveTab(0);
    
    // File group
    CModernToolbarGroup* pGroup = m_toolbar.AddGroup(L"File");
    if (pGroup)
    {
        pGroup->AddButton(ID_FILE_OPEN, L"Open", LoadIcon(ID_FILE_OPEN), L"Open log file (Ctrl+O)");
        pGroup->AddButton(ID_FILE_SAVE_LOG, L"Save Log", LoadIcon(ID_FILE_SAVE_LOG), L"Save all messages (Ctrl+Shift+S)");
        pGroup->AddButton(ID_FILE_SAVE_VIEW, L"Save View", LoadIcon(ID_FILE_SAVE_VIEW), L"Save current view");
    }
    
    // Edit group
    pGroup = m_toolbar.AddGroup(L"Edit");
    if (pGroup)
    {
        pGroup->AddButton(ID_VIEW_COPY, L"Copy", LoadIcon(ID_VIEW_COPY), L"Copy selection (Ctrl+C)");
        pGroup->AddButton(ID_LOG_CLEAR, L"Clear", LoadIcon(ID_LOG_CLEAR), L"Clear log (Ctrl+Shift+X)");
        pGroup->AddButton(ID_LOG_CROP, L"Crop", LoadIcon(ID_LOG_CROP), L"Crop to selection");
    }
    
    // View group
    pGroup = m_toolbar.AddGroup(L"View");
    if (pGroup)
    {
        pGroup->AddButton(ID_VIEW_SCROLL, L"Auto Scroll", LoadIcon(ID_VIEW_SCROLL), L"Auto scroll to new messages");
        pGroup->AddButton(ID_VIEW_TIME, L"Clock Time", LoadIcon(ID_VIEW_TIME), L"Toggle time display (Ctrl+T)");
        pGroup->AddButton(ID_OPTIONS_FONT, L"Font", LoadIcon(ID_OPTIONS_FONT), L"Change font");
    }
    
    // Search group
    pGroup = m_toolbar.AddGroup(L"Search");
    if (pGroup)
    {
        pGroup->AddButton(ID_VIEW_FIND, L"Find", LoadIcon(ID_VIEW_FIND), L"Find text (Ctrl+F)");
        pGroup->AddButton(ID_VIEW_FILTER, L"Filter", LoadIcon(ID_VIEW_FILTER), L"Edit filters (F5)");
    }
    
    // Setup Quick Access Toolbar
    m_toolbar.AddQuickAccessButton(ID_FILE_OPEN, LoadIcon(ID_FILE_OPEN, 16), L"Open");
    m_toolbar.AddQuickAccessButton(ID_FILE_SAVE_LOG, LoadIcon(ID_FILE_SAVE_LOG, 16), L"Save Log");
    m_toolbar.AddQuickAccessButton(ID_VIEW_COPY, LoadIcon(ID_VIEW_COPY, 16), L"Copy");
    m_toolbar.AddQuickAccessButton(ID_LOG_CLEAR, LoadIcon(ID_LOG_CLEAR, 16), L"Clear");
    m_toolbar.AddQuickAccessButton(ID_VIEW_FIND, LoadIcon(ID_VIEW_FIND, 16), L"Find");
}

HICON CModernToolbarManager::LoadIcon(UINT nID, int size)
{
    // For now, use existing 16x16 icons scaled up
    // In production, would load proper sized icons
    HICON hIcon = (HICON)::LoadImage(
        ::GetModuleHandle(nullptr),
        MAKEINTRESOURCE(nID),
        IMAGE_ICON,
        size, size,
        LR_DEFAULTCOLOR);
    
    if (hIcon)
        m_icons.push_back(hIcon);
    
    return hIcon;
}

void CModernToolbarManager::LoadSettings()
{
    // TODO: Load from registry
}

void CModernToolbarManager::SaveSettings()
{
    // TODO: Save to registry
}

} // namespace debugviewpp
} // namespace fusion