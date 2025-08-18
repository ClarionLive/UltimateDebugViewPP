// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "atleverything.h"
#include "resource.h"
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atlframe.h>
#include <vector>
#include <memory>

namespace fusion {
namespace debugviewpp {

// Simple modern toolbar that extends the standard toolbar with text labels
class CModernSimpleToolbar : public CWindowImpl<CModernSimpleToolbar, CToolBarCtrl>
{
public:
    DECLARE_WND_SUPERCLASS(NULL, CToolBarCtrl::GetWndClassName())
    
    BEGIN_MSG_MAP(CModernSimpleToolbar)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
    END_MSG_MAP()
    
    struct ButtonInfo
    {
        UINT nID;
        std::wstring text;
        std::wstring tooltip;
        int imageIndex;
    };
    
    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        // Set extended styles for modern look
        SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS);
        
        // Set button style to show text
        ModifyStyle(0, TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS);
        
        // Set button size for larger, modern look
        SetButtonSize(CSize(80, 48));
        
        return 0;
    }
    
    void AddModernButton(UINT nID, LPCTSTR text, int imageIndex = -1)
    {
        TBBUTTON button = {0};
        button.iBitmap = imageIndex;
        button.idCommand = nID;
        button.fsState = TBSTATE_ENABLED;
        button.fsStyle = BTNS_BUTTON | BTNS_SHOWTEXT;
        button.iString = AddStrings(text);
        
        AddButtons(1, &button);
    }
    
    void AddModernDropdownButton(UINT nID, LPCTSTR text, int imageIndex = -1)
    {
        TBBUTTON button = {0};
        button.iBitmap = imageIndex;
        button.idCommand = nID;
        button.fsState = TBSTATE_ENABLED;
        button.fsStyle = BTNS_DROPDOWN | BTNS_SHOWTEXT | BTNS_WHOLEDROPDOWN;
        button.iString = AddStrings(text);
        
        AddButtons(1, &button);
    }
    
    void AddSeparator()
    {
        TBBUTTON button = {0};
        button.fsStyle = BTNS_SEP;
        AddButtons(1, &button);
    }
};

// Helper to create a modern-style toolbar in the existing framework
class CModernToolbarHelper
{
public:
    static HWND CreateModernToolbar(HWND hWndParent, UINT nID = ATL_IDW_TOOLBAR)
    {
        CModernSimpleToolbar toolbar;
        CRect rcDefault(0, 0, 0, 0);
        HWND hWndToolBar = toolbar.Create(hWndParent, rcDefault, nullptr,
            ATL_SIMPLE_TOOLBAR_PANE_STYLE | TBSTYLE_LIST | TBSTYLE_FLAT, 0, nID);
        
        if (hWndToolBar)
        {
            toolbar.SetButtonStructSize();
            
            // Load existing toolbar bitmap but make it larger
            toolbar.LoadImages(IDR_MAINFRAME);
            
            // Set larger image size for modern look
            toolbar.SetBitmapSize(CSize(24, 24));
            toolbar.SetButtonSize(CSize(72, 40));
            
            // Add buttons with text labels
            toolbar.AddModernButton(ID_FILE_OPEN, L"Open\nCtrl+O", 0);
            toolbar.AddModernButton(ID_FILE_SAVE_LOG, L"Save Log\nCtrl+Shift+S", 1);
            toolbar.AddSeparator();
            toolbar.AddModernButton(ID_VIEW_COPY, L"Copy\nCtrl+C", 3);
            toolbar.AddModernButton(ID_LOG_CLEAR, L"Clear\nCtrl+Shift+X", 4);
            toolbar.AddSeparator();
            toolbar.AddModernButton(ID_VIEW_SCROLL, L"Auto Scroll", 6);
            toolbar.AddModernButton(ID_VIEW_TIME, L"Clock Time\nCtrl+T", 7);
            toolbar.AddModernButton(ID_OPTIONS_FONT, L"Font", 8);
            toolbar.AddModernButton(ID_VIEW_FILTER, L"Filter\nF5", 9);
            toolbar.AddSeparator();
            toolbar.AddModernButton(ID_VIEW_BOOKMARK, L"Bookmark\nCtrl+F2", 12);
            toolbar.AddSeparator();
            toolbar.AddModernButton(ID_LOG_PAUSE, L"Pause", 17);
            
            toolbar.Detach();
        }
        
        return hWndToolBar;
    }
};

} // namespace debugviewpp
} // namespace fusion