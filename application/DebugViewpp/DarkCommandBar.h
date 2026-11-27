// WTL command bar (menu bar with owner drawn popup menus) that follows the dark theme.
//
// CCommandBarCtrl draws its popup menu items with GetSysColor(), so they stay light in dark mode.
// This class draws them with the dark palette when the dark theme is active and falls back to the
// stock WTL drawing otherwise.

#pragma once

#include "atleverything.h"
#include "DarkMode.h"

namespace fusion {
namespace debugviewpp {

class CDarkCommandBarCtrl : public CCommandBarCtrlImpl<CDarkCommandBarCtrl>
{
public:
    DECLARE_WND_SUPERCLASS(_T("WTL_CommandBar_Dark"), GetWndClassName())

    void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
    {
        if (DarkMode::IsEnabled())
            DrawItemDark(lpDrawItemStruct);
        else
            CCommandBarCtrlImpl<CDarkCommandBarCtrl>::DrawItem(lpDrawItemStruct);
    }

private:
    static void Fill(HDC dc, const RECT& rc, COLORREF color)
    {
        HBRUSH brush = ::CreateSolidBrush(color);
        ::FillRect(dc, &rc, brush);
        ::DeleteObject(brush);
    }

    // Draws a check mark (or a bullet for radio items) with the Marlett symbol font.
    void DrawMark(HDC dc, const RECT& rc, bool radio, COLORREF color)
    {
        LOGFONT lf = {};
        lf.lfHeight = -(rc.bottom - rc.top) * 2 / 3;
        lf.lfCharSet = SYMBOL_CHARSET;
        lstrcpy(lf.lfFaceName, _T("Marlett"));
        HFONT font = ::CreateFontIndirect(&lf);
        HGDIOBJ oldFont = ::SelectObject(dc, font);
        ::SetTextColor(dc, color);
        RECT rcMark = rc;
        ::DrawText(dc, radio ? _T("h") : _T("a"), 1, &rcMark, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
        ::SelectObject(dc, oldFont);
        ::DeleteObject(font);
    }

    void DrawItemDark(LPDRAWITEMSTRUCT lpDrawItemStruct)
    {
        auto* pmd = reinterpret_cast<_MenuItemData*>(lpDrawItemStruct->itemData);
        CDCHandle dc = lpDrawItemStruct->hDC;
        const RECT& rcItem = lpDrawItemStruct->rcItem;
        const DarkMode::MenuColors colors = DarkMode::GetMenuColors();

        const bool disabled = (lpDrawItemStruct->itemState & ODS_GRAYED) != 0;
        bool selected = (lpDrawItemStruct->itemState & ODS_SELECTED) != 0;
        const bool checked = (lpDrawItemStruct->itemState & ODS_CHECKED) != 0;
        if (LOWORD(lpDrawItemStruct->itemID) == static_cast<WORD>(-1))
            selected = false;

        // Background of the whole item.
        Fill(dc, rcItem, (selected && !(pmd->fType & MFT_SEPARATOR)) ? colors.highlight : colors.background);

        if (pmd->fType & MFT_SEPARATOR)
        {
            RECT line = rcItem;
            line.left += m_szButton.cx + s_kcxGap;
            line.top += (line.bottom - line.top) / 2;
            line.bottom = line.top + 1;
            Fill(dc, line, colors.border);
            return;
        }

        RECT rcButn = {rcItem.left, rcItem.top, rcItem.left + m_szButton.cx, rcItem.top + m_szButton.cy};
        ::OffsetRect(&rcButn, 0, ((rcItem.bottom - rcItem.top) - (rcButn.bottom - rcButn.top)) / 2);

        // Frame around the image / check mark of checked items.
        if (checked)
        {
            RECT rcCheck = rcButn;
            ::InflateRect(&rcCheck, -1, -1);
            HBRUSH brush = ::CreateSolidBrush(colors.accent);
            ::FrameRect(dc, &rcCheck, brush);
            ::DeleteObject(brush);
        }

        const int iButton = pmd->iButton;
        if (iButton >= 0)
        {
            SIZE sz = {rcButn.right - rcButn.left - m_szBitmap.cx, rcButn.bottom - rcButn.top - m_szBitmap.cy};
            POINT point = {rcButn.left + sz.cx / 2, rcButn.top + sz.cy / 2};
            if (!disabled)
            {
                ::ImageList_Draw(m_hImageList, iButton, dc, point.x, point.y, ILD_TRANSPARENT);
            }
            else
            {
                ::ImageList_DrawEx(m_hImageList, iButton, dc, point.x, point.y, 0, 0, CLR_NONE, colors.background,
                    ILD_TRANSPARENT | ILD_BLEND50);
            }
        }
        else if (checked)
        {
            CMenuItemInfo info;
            info.fMask = MIIM_CHECKMARKS | MIIM_TYPE;
            ::GetMenuItemInfo(reinterpret_cast<HMENU>(lpDrawItemStruct->hwndItem), lpDrawItemStruct->itemID, MF_BYCOMMAND, &info);
            const bool radio = (info.fType & MFT_RADIOCHECK) != 0;
            DrawMark(dc, rcButn, radio, disabled ? colors.disabledText : colors.text);
        }

        // Text
        RECT rcText = rcItem;
        rcText.left += m_szButton.cx + s_kcxGap + s_kcxTextMargin;
        rcText.right -= m_szButton.cx;
        dc.SetBkMode(TRANSPARENT);
        DrawMenuText(dc, rcText, pmd->lpstrText, disabled ? colors.disabledText : colors.text);
    }
};

} // namespace debugviewpp
} // namespace fusion
