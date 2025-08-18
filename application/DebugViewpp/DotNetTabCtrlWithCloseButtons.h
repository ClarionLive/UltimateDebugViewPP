#pragma once

#include "../Libraries/TabbingFramework/DotNetTabCtrl.h"
#include <vector>

// Custom tab control that shows close buttons on individual tabs using child button windows
template <typename TItem = CCustomTabItem>
class CDotNetTabCtrlWithCloseButtons : public CDotNetTabCtrl<TItem>
{
protected:
    typedef CDotNetTabCtrlWithCloseButtons<TItem> thisClass;
    typedef CDotNetTabCtrl<TItem> baseClass;

    // Structure to hold close button info for each tab
    struct TabCloseButton
    {
        HWND hWndButton;
        int nTabIndex;
        
        TabCloseButton() : hWndButton(NULL), nTabIndex(-1) {}
    };
    
    std::vector<TabCloseButton> m_tabCloseButtons;
    
public:
    CDotNetTabCtrlWithCloseButtons()
    {
    }

    BEGIN_MSG_MAP(thisClass)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        REFLECTED_NOTIFY_CODE_HANDLER_EX(TCN_SELCHANGE, OnTabSelChange)
        CHAIN_MSG_MAP(baseClass)
    END_MSG_MAP()
    
    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        bHandled = FALSE;
        UpdateCloseButtons();
        return 0;
    }
    
    LRESULT OnCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        // Check if it's from one of our close buttons
        HWND hWndButton = (HWND)lParam;
        if (hWndButton != NULL)
        {
            for (size_t i = 0; i < m_tabCloseButtons.size(); ++i)
            {
                if (m_tabCloseButtons[i].hWndButton == hWndButton)
                {
                    // Send close notification to parent
                    NMCTC2ITEMS nmh = {{0}};
                    nmh.hdr.hwndFrom = this->m_hWnd;
                    nmh.hdr.idFrom = this->GetDlgCtrlID();
                    nmh.hdr.code = CTCN_CLOSE;
                    nmh.iItem1 = m_tabCloseButtons[i].nTabIndex;
                    nmh.iItem2 = -1;
                    
                    ::SendMessage(this->GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM)&nmh);
                    return 0;
                }
            }
        }
        bHandled = FALSE;
        return 0;
    }
    
    LRESULT OnTabSelChange(LPNMHDR /*pnmh*/)
    {
        UpdateCloseButtons();
        return 0;
    }
    
    
    // Override InsertItem to update buttons when tabs are added
    int InsertItem(int nItem, TItem* pItem)
    {
        int result = baseClass::InsertItem(nItem, pItem);
        // Force a full layout recalculation to account for close button space
        this->UpdateLayout();
        this->Invalidate();
        UpdateCloseButtons();
        return result;
    }
    
    // Override DeleteItem to clean up buttons when tabs are removed
    BOOL DeleteItem(size_t nItem, bool bUpdateSelection = true, bool bNotify = true)
    {
        // Destroy the button for this tab if it exists
        if (nItem < m_tabCloseButtons.size())
        {
            if (m_tabCloseButtons[nItem].hWndButton != NULL)
            {
                ::DestroyWindow(m_tabCloseButtons[nItem].hWndButton);
            }
        }
        
        BOOL result = baseClass::DeleteItem(nItem, bUpdateSelection, bNotify);
        UpdateCloseButtons();
        return result;
    }
    
    // Override UpdateLayout to update button positions after base layout
    void UpdateLayout(void)
    {
        // Call base implementation first
        baseClass::UpdateLayout();
        
        // Update button positions after base layout
        UpdateCloseButtons();
    }
    
    // Override to prevent text from drawing under close button
    void DrawItem_ImageAndText(DWORD dwStyle, LPNMCTCCUSTOMDRAW lpNMCustomDraw, int nIconVerticalCenter, RECT& rcTab, RECT& rcText)
    {
        // Get the item to check if it can be closed
        size_t nItem = lpNMCustomDraw->nmcd.dwItemSpec;
        TItem* pItem = baseClass::GetItem(nItem);
        
        // If this tab has a close button, reduce the text drawing area
        if (pItem && pItem->CanClose())
        {
            // Make room for the close button - need to reduce both rcTab and rcText
            rcTab.right -= 24;   // Reduce the tab drawing area
            rcText.right -= 24;  // Also reduce the text area
        }
        
        // Call base implementation with adjusted rectangles
        baseClass::DrawItem_ImageAndText(dwStyle, lpNMCustomDraw, nIconVerticalCenter, rcTab, rcText);
    }
    
    void UpdateCloseButtons()
    {
        // Remove existing buttons
        for (auto& btn : m_tabCloseButtons)
        {
            if (btn.hWndButton != NULL)
            {
                ::DestroyWindow(btn.hWndButton);
                btn.hWndButton = NULL;
            }
        }
        m_tabCloseButtons.clear();
        
        // Don't create buttons if we don't have a window yet
        if (!this->IsWindow())
            return;
        
        // Create a button for each closeable tab
        size_t nCount = this->m_Items.GetCount();
        m_tabCloseButtons.resize(nCount);
        
        for (size_t i = 0; i < nCount; ++i)
        {
            TItem* pItem = this->m_Items[i];
            if (pItem && pItem->CanClose())
            {
                RECT rcItem = pItem->GetRect();
                
                // Skip if tab is not visible
                if (rcItem.left == 0 && rcItem.right == 0)
                    continue;
                
                // Adjust for scroll offset
                ::OffsetRect(&rcItem, this->m_iScrollOffset, 0);
                
                // Calculate close button position (on the right side of the tab)
                // Position button inside the tab with proper padding
                RECT rcClose;
                rcClose.right = rcItem.right - 8;  // Padding from right edge
                rcClose.left = rcClose.right - 16; // 16 pixel wide button
                rcClose.top = rcItem.top + ((rcItem.bottom - rcItem.top - 16) / 2);
                rcClose.bottom = rcClose.top + 16;
                
                // Create a small button with "×" text
                HWND hWndButton = ::CreateWindow(
                    L"BUTTON",
                    L"×",
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
                    rcClose.left, rcClose.top,
                    rcClose.right - rcClose.left,
                    rcClose.bottom - rcClose.top,
                    this->m_hWnd,
                    (HMENU)(INT_PTR)(10000 + i), // Button ID
                    ::GetModuleHandle(NULL),
                    NULL);
                
                if (hWndButton != NULL)
                {
                    // Set a small font for the button
                    HFONT hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
                    ::SendMessage(hWndButton, WM_SETFONT, (WPARAM)hFont, TRUE);
                    
                    m_tabCloseButtons[i].hWndButton = hWndButton;
                    m_tabCloseButtons[i].nTabIndex = (int)i;
                }
            }
        }
    }
};