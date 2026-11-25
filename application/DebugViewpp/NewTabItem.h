// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <stdexcept>
#include "atleverything.h"

#pragma warning(push, 3)
#pragma warning(disable : 4838)

#include "CustomTabCtrl.h"
#include "DotNetTabCtrl.h"
#include "TabbedFrame.h"
#include "CLogViewTabItem2.h"
#pragma warning(pop)

namespace fusion {
namespace debugviewpp {

// Special tab item that displays "+" and cannot be closed or dragged
// Used to allow creating new tabs by clicking on the "+" tab
class CNewTabItem : public CTabViewTabItem
{
public:
    CNewTabItem() : CTabViewTabItem()
    {
        SetText(L"+");
        SetToolTip(L"Click to create a new tab");  // Fixed: Use SetToolTip
        SetCanClose(false);  // The "+" tab should never be closeable
    }
    
    ~CNewTabItem() override = default;
    
    // Override to return "+" as the title
    std::wstring GetTitle() const { return L"+"; }
    
    // Create method does nothing for the "+" tab
    void Create(HWND /*parent*/) 
    { 
        // No actual view content needed for the "+" tab
        // Set a dummy HWND to avoid null pointer issues
        SetTabView(nullptr);
    }
    
    // SetMainFrame does nothing for the "+" tab
    void SetMainFrame(CMainFrame* /*pMainFrame*/) 
    { 
        // No main frame operations needed for the "+" tab
    }
    
    // GetLogViewParent returns null since there's no actual view
    HWND GetLogViewParent() { return nullptr; }
    
    // Safe no-op implementations for methods that might be called on all tabs
    void RefreshTheme() { /* No theme to refresh */ }
    void SetDetailFont(HFONT /*font*/) { /* No detail font to set */ }
    void ShowDetailPane(bool /*show*/) { /* No detail pane */ }
    void SetSplitterPos(int /*pos*/) { /* No splitter */ }
    void SetDetailWordWrap(bool /*wrap*/) { /* No detail settings */ }
    void SetDetailSyntaxHighlight(bool /*highlight*/) { /* No detail settings */ }
    void SetDetailFormatContent(bool /*format*/) { /* No detail settings */ }
    void SetDetailLineNumbers(bool /*show*/) { /* No detail settings */ }
    void UpdateFloatingWindowTrackers() { /* No floating windows */ }
    void UpdateFloatingWindows() { /* No floating windows */ }

private:
    // No member variables needed - this is a minimal implementation
};

} // namespace debugviewpp
} // namespace fusion