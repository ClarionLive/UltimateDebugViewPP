// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "PropertyItem.h"
#include "PropertyItemEditors.h"
#include "PropertyItemImpl.h"
#include "DebugViewppLib/Colors.h"
#include "Win32/Win32Lib.h"

namespace fusion {
namespace debugviewpp {

class ColorDialog : public CColorDialogImpl<ColorDialog>
{
public:
    ColorDialog(const wchar_t* title, COLORREF clrInit = 0, DWORD dwFlags = 0, HWND hWndParent = nullptr) :
        CColorDialogImpl<ColorDialog>(clrInit == Colors::Auto ? RGB(255, 255, 255) : clrInit, dwFlags, hWndParent),
        m_title(title),
        m_auto(clrInit == Colors::Auto),
        m_showAuto(false)
    {
    }

    BEGIN_MSG_MAP_EX(ColorDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_DESTROY(OnDestroy)
        CHAIN_MSG_MAP(CColorDialogImpl<ColorDialog>)
    END_MSG_MAP()

    BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
    {
        SetWindowText(m_title.c_str());
        if (m_showAuto)
        {
            RECT rect = {166, 160, 216, 176};
            m_btnAuto.Create(*this, &rect, L"Auto", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_TEXT | BS_LEFT, 0);
            m_btnAuto.SetFont(GetFont(), FALSE);
            m_btnAuto.SetCheck(m_auto ? BST_CHECKED : BST_UNCHECKED);
        }
        return TRUE;
    }

    void OnDestroy()
    {
        if (m_btnAuto.IsWindow() != 0)
        {
            m_auto = m_showAuto && m_btnAuto.GetCheck() == BST_CHECKED;
            m_btnAuto.DestroyWindow();
        }
        else
        {
            m_auto = false;
        }
    }

    void ShowAuto(bool show)
    {
        m_showAuto = show;
    }

    void SetCurrentColor(COLORREF color)
    {
        m_auto = color == Colors::Auto;
        CColorDialogImpl<ColorDialog>::SetCurrentColor(m_auto ? RGB(255, 255, 255) : color);
    }

    COLORREF GetColor() const
    {
        return m_auto ? Colors::Auto : CColorDialogImpl<ColorDialog>::GetColor();
    }

private:
    std::wstring m_title;
    CButton m_btnAuto;
    bool m_auto;
    bool m_showAuto;
};

class CPropertyColorItem : public CPropertyItem
{
public:
    CPropertyColorItem(const wchar_t* name, COLORREF color) :
        CPropertyItem(name, 0),
        m_dlg(name, color, CC_ANYCOLOR),
        m_currentColor(color),
        m_showAuto(false)
    {
    }

    COLORREF GetColor() const
    {
        return m_currentColor;
    }

    void SetColor(COLORREF color)
    {
        m_currentColor = color;
        // Don't call m_dlg.SetCurrentColor here - dialog doesn't exist yet
    }

    void ShowAuto(bool show)
    {
        m_showAuto = show;
        m_dlg.ShowAuto(show);
    }

    BOOL Activate(UINT action, LPARAM /*lParam*/) override
    {

        if (IsEnabled() == 0)
        {
            return FALSE;
        }

        switch (action)
        {
        case PACT_SPACE:
        case PACT_CLICK:
        case PACT_DBLCLICK:
        case PACT_ACTIVATE:
            {

                // Try to find a better parent window
                HWND parentHwnd = m_hWndOwner;
                if (!::IsWindow(parentHwnd) || !::IsWindowEnabled(parentHwnd))
                {
                    // Try the parent of the grid control
                    parentHwnd = ::GetParent(m_hWndOwner);
                    if (!::IsWindow(parentHwnd))
                    {
                        // Fall back to active window
                        parentHwnd = ::GetActiveWindow();
                        if (!::IsWindow(parentHwnd))
                        {
                            // Ultimate fallback to desktop
                            parentHwnd = ::GetDesktopWindow();
                        }
                    }
                }

                // Re-create the dialog with the current color
                m_dlg = ColorDialog(GetName(), m_currentColor, CC_ANYCOLOR);
                m_dlg.ShowAuto(m_showAuto);

                INT_PTR result = m_dlg.DoModal(parentHwnd);


                if (result == IDOK)
                {
                    // Get the selected color from the dialog
                    m_currentColor = m_dlg.GetColor();

                    // Let control owner know
                    NMPROPERTYITEM nmh = {{m_hWndOwner, static_cast<UINT_PTR>(::GetDlgCtrlID(m_hWndOwner)), PIN_ITEMCHANGED}, this};
                    ::SendMessage(::GetParent(m_hWndOwner), WM_NOTIFY, nmh.hdr.idFrom, reinterpret_cast<LPARAM>(&nmh));
                }
                else if (result == IDCANCEL)
                {
                }
                else
                {
                }
            }
            break;
        default: 
            break;
        }
        return TRUE;
    }

    void DrawValue(PROPERTYDRAWINFO& di) override
    {
        CDCHandle dc(di.hDC);
        RECT rect = di.rcItem;
        if (IsEnabled() == 0)
        {
            return dc.FillSolidRect(&rect, di.clrDisabledBack);
        }

        dc.FillSolidRect(&rect, di.clrBorder);
        ::InflateRect(&rect, -1, -1);
        dc.FillSolidRect(&rect, RGB(0, 0, 0));
        ::InflateRect(&rect, -1, -1);
        auto color = GetColor();
        if (color == Colors::Auto)
        {
            Win32::ScopedBkColor bg(dc, RGB(255, 255, 255));
            Win32::ScopedTextColor fg(dc, RGB(0, 0, 0));
            Win32::ScopedTextAlign ta(dc, TA_CENTER | TA_BOTTOM);
            dc.ExtTextOut((rect.left + rect.right) / 2, rect.bottom, ETO_OPAQUE, &rect, L"A", 1);
        }
        else
        {
            dc.FillSolidRect(&rect, color & 0xFFFFFF);
        }
    }

    BOOL GetValue(VARIANT* pValue) const override
    {
        CComVariant var(GetColor());
        return SUCCEEDED(var.Detach(pValue));
    }

    BOOL SetValue(const VARIANT& value) override
    {
        CComVariant var;
        if (FAILED(VariantChangeType(&var, &value, 0, VT_COLOR)))
        {
            return FALSE;
        }
        SetColor(var.intVal);
        return TRUE;
    }

private:
    ColorDialog m_dlg;
    COLORREF m_currentColor;
    bool m_showAuto;
};

inline CPropertyColorItem* PropCreateColorItem(const wchar_t* name, COLORREF color)
{
    return new CPropertyColorItem(name, color);
}

} // namespace debugviewpp
} // namespace fusion
