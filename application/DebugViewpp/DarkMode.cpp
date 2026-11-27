// Dark mode support for the classic Win32/WTL controls used by the viewer. See DarkMode.h.

#include "atleverything.h"
#include "DarkMode.h"
#include "DebugViewppLib/ThemeManager.h"
#include <commctrl.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include <map>
#include <string>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

namespace fusion {
namespace debugviewpp {
namespace DarkMode {

namespace {

// ---------------------------------------------------------------------------------------------------
// Undocumented uxtheme entry points (the same ones used by Explorer, Notepad++ and others).
// ---------------------------------------------------------------------------------------------------
enum PreferredAppMode
{
    AppModeDefault = 0,
    AppModeAllowDark = 1,
    AppModeForceDark = 2,
    AppModeForceLight = 3
};

using fnSetPreferredAppMode = int(WINAPI*)(int);
using fnFlushMenuThemes = void(WINAPI*)();
using fnRefreshImmersiveColorPolicyState = void(WINAPI*)();

fnSetPreferredAppMode pSetPreferredAppMode = nullptr;
fnFlushMenuThemes pFlushMenuThemes = nullptr;
fnRefreshImmersiveColorPolicyState pRefreshImmersiveColorPolicyState = nullptr;

HHOOK g_hook = nullptr;
bool g_initialized = false;

// ---------------------------------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------------------------------
struct Palette
{
    COLORREF bar;             // menu bar / toolbar / rebar
    COLORREF barText;
    COLORREF hot;             // hovered toolbar button
    COLORREF pressed;         // pressed or checked toolbar button
    COLORREF border;
    COLORREF tabBg;           // tab strip
    COLORREF tabSel;          // selected tab (same colour as the content below it)
    COLORREF tabText;
    COLORREF tabInactiveText;
    COLORREF edit;            // edit / list boxes in dialogs
    COLORREF dialog;          // dialog background
    COLORREF header;          // list view header
    COLORREF headerText;
    COLORREF statusBg;
    COLORREF statusText;
    COLORREF disabledText;
};

Palette GetPalette()
{
    const auto& c = ThemeManager::Instance().GetColors();
    Palette p;
    p.bar = c.toolbarBackground;
    p.barText = c.toolbarText;
    p.hot = RGB(62, 62, 62);
    p.pressed = c.itemHighlight;
    p.border = RGB(85, 85, 85);
    p.tabBg = c.tabBackground;
    p.tabSel = c.background;
    p.tabText = c.tabText;
    p.tabInactiveText = RGB(160, 160, 160);
    p.edit = c.background;
    p.dialog = c.controlBackground;
    p.header = c.toolbarBackground;
    p.headerText = c.controlText;
    p.statusBg = c.statusBarBackground;
    p.statusText = c.statusBarText;
    p.disabledText = RGB(120, 120, 120);
    return p;
}

HBRUSH GetBrush(COLORREF color)
{
    static std::map<COLORREF, HBRUSH> brushes;
    auto it = brushes.find(color);
    if (it != brushes.end())
        return it->second;
    HBRUSH brush = ::CreateSolidBrush(color);
    brushes[color] = brush;
    return brush;
}

void FillRectColor(HDC dc, const RECT& rc, COLORREF color)
{
    ::FillRect(dc, &rc, GetBrush(color));
}

void DrawHLine(HDC dc, int x1, int x2, int y, COLORREF color)
{
    RECT rc = {x1, y, x2, y + 1};
    FillRectColor(dc, rc, color);
}

void DrawVLine(HDC dc, int x, int y1, int y2, COLORREF color)
{
    RECT rc = {x, y1, x + 1, y2};
    FillRectColor(dc, rc, color);
}

// ---------------------------------------------------------------------------------------------------
// Window class helpers
// ---------------------------------------------------------------------------------------------------
bool IsClass(HWND hwnd, const wchar_t* name)
{
    if (!hwnd)
        return false;
    wchar_t buf[80] = {};
    int n = ::GetClassNameW(hwnd, buf, 80);
    return n > 0 && _wcsicmp(buf, name) == 0;
}

bool IsCommandBar(HWND hwnd)
{
    return IsClass(hwnd, L"WTL_CommandBar") || IsClass(hwnd, L"WTL_CommandBar_Dark");
}

// Toolbars that live in the rebar (and the command bar) are drawn by this module.
bool IsRebarToolbar(HWND hwnd)
{
    if (IsCommandBar(hwnd))
        return true;
    return IsClass(hwnd, L"ToolbarWindow32") && IsClass(::GetParent(hwnd), L"ReBarWindow32");
}

bool IsShellView(HWND hwnd)
{
    return IsClass(hwnd, L"SHELLDLL_DefView") || IsClass(hwnd, L"DUIViewWndClassName") ||
           IsClass(hwnd, L"DirectUIHWND") || IsClass(hwnd, L"CtrlNotifySink");
}

// Controls hosted by the shell (file dialogs) are left alone: they cannot be themed consistently.
bool InsideShellView(HWND hwnd)
{
    HWND parent = ::GetParent(hwnd);
    for (int i = 0; parent && i < 6; ++i)
    {
        if (IsShellView(parent))
            return true;
        parent = ::GetParent(parent);
    }
    return false;
}

bool IsShellDialog(HWND dlg)
{
    return ::FindWindowExW(dlg, nullptr, L"DUIViewWndClassName", nullptr) ||
           ::FindWindowExW(dlg, nullptr, L"SHELLDLL_DefView", nullptr) ||
           ::FindWindowExW(dlg, nullptr, L"ComboBoxEx32", nullptr);
}

// CWindowImpl based superclasses get a generated "ATL:xxxx" class name, so detect list views by probing.
bool LooksLikeListView(HWND hwnd)
{
    wchar_t buf[16] = {};
    ::GetClassNameW(hwnd, buf, 16);
    if (wcsncmp(buf, L"ATL:", 4) != 0)
        return false;
    HWND header = reinterpret_cast<HWND>(::SendMessageW(hwnd, LVM_GETHEADER, 0, 0));
    return header && ::IsWindow(header) && ::GetParent(header) == hwnd;
}

HFONT GetControlFont(HWND hwnd)
{
    HFONT font = reinterpret_cast<HFONT>(::SendMessageW(hwnd, WM_GETFONT, 0, 0));
    return font ? font : reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
}

template <class F>
void PaintBuffered(HWND hwnd, WPARAM wParam, bool isPrint, F&& draw)
{
    PAINTSTRUCT ps = {};
    HDC dc = isPrint ? reinterpret_cast<HDC>(wParam) : ::BeginPaint(hwnd, &ps);
    RECT rc = {};
    ::GetClientRect(hwnd, &rc);
    if (rc.right > 0 && rc.bottom > 0)
    {
        HDC mem = ::CreateCompatibleDC(dc);
        HBITMAP bmp = ::CreateCompatibleBitmap(dc, rc.right, rc.bottom);
        HGDIOBJ oldBmp = ::SelectObject(mem, bmp);
        HGDIOBJ oldFont = ::SelectObject(mem, GetControlFont(hwnd));
        ::SetBkMode(mem, TRANSPARENT);
        draw(mem, rc);
        ::BitBlt(dc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
        ::SelectObject(mem, oldFont);
        ::SelectObject(mem, oldBmp);
        ::DeleteObject(bmp);
        ::DeleteDC(mem);
    }
    if (!isPrint)
        ::EndPaint(hwnd, &ps);
}

void SetDarkTitleBar(HWND hwnd, bool dark)
{
    BOOL value = dark ? TRUE : FALSE;
    ::DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &value, sizeof(value));
}

void SetControlTheme(HWND hwnd, bool dark, const wchar_t* darkTheme)
{
    ::SetWindowTheme(hwnd, dark ? darkTheme : nullptr, nullptr);
}

// ---------------------------------------------------------------------------------------------------
// Subclass procedures
// ---------------------------------------------------------------------------------------------------
const UINT_PTR kSubclassId = 1;

// List view header ---------------------------------------------------------------------------------
void PaintHeader(HWND hwnd, HDC dc, const RECT& rc)
{
    const Palette p = GetPalette();
    FillRectColor(dc, rc, p.header);
    ::SetTextColor(dc, p.headerText);

    int count = Header_GetItemCount(hwnd);
    for (int i = 0; i < count; ++i)
    {
        RECT item = {};
        if (!Header_GetItemRect(hwnd, i, &item))
            continue;

        wchar_t text[256] = {};
        HDITEMW hdi = {};
        hdi.mask = HDI_TEXT | HDI_FORMAT;
        hdi.pszText = text;
        hdi.cchTextMax = 256;
        Header_GetItem(hwnd, i, &hdi);

        RECT rcText = item;
        rcText.left += 6;
        rcText.right -= 6;
        UINT format = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX;
        if (hdi.fmt & HDF_RIGHT)
            format |= DT_RIGHT;
        else if (hdi.fmt & HDF_CENTER)
            format |= DT_CENTER;
        else
            format |= DT_LEFT;
        ::DrawTextW(dc, text, -1, &rcText, format);

        DrawVLine(dc, item.right - 1, item.top + 4, item.bottom - 4, p.border);
    }
    DrawHLine(dc, rc.left, rc.right, rc.bottom - 1, p.border);
}

LRESULT CALLBACK HeaderProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR)
{
    if (IsEnabled())
    {
        if (msg == WM_ERASEBKGND)
            return 1;
        if (msg == WM_PAINT || msg == WM_PRINTCLIENT)
        {
            PaintBuffered(hwnd, wParam, msg == WM_PRINTCLIENT, [hwnd](HDC dc, const RECT& rc) { PaintHeader(hwnd, dc, rc); });
            return 0;
        }
    }
    if (msg == WM_NCDESTROY)
        ::RemoveWindowSubclass(hwnd, HeaderProc, id);
    return ::DefSubclassProc(hwnd, msg, wParam, lParam);
}

// Tab control (SysTabControl32) -------------------------------------------------------------------
void PaintTabs(HWND hwnd, HDC dc, const RECT& rc)
{
    const Palette p = GetPalette();
    FillRectColor(dc, rc, p.tabSel);

    int count = TabCtrl_GetItemCount(hwnd);
    int selected = TabCtrl_GetCurSel(hwnd);

    LONG stripBottom = 0;
    for (int i = 0; i < count; ++i)
    {
        RECT item = {};
        TabCtrl_GetItemRect(hwnd, i, &item);
        stripBottom = (std::max)(stripBottom, item.bottom);
    }
    if (count == 0)
        return;

    stripBottom += 1;
    RECT strip = {rc.left, rc.top, rc.right, stripBottom};
    FillRectColor(dc, strip, p.tabBg);

    RECT selectedRect = {};
    for (int i = 0; i < count; ++i)
    {
        RECT item = {};
        TabCtrl_GetItemRect(hwnd, i, &item);
        const bool isSelected = (i == selected);

        RECT body = item;
        if (isSelected)
        {
            FillRectColor(dc, body, p.tabSel);
            selectedRect = item;
            DrawVLine(dc, item.left, item.top, item.bottom, p.border);
            DrawVLine(dc, item.right - 1, item.top, item.bottom, p.border);
            DrawHLine(dc, item.left, item.right, item.top, p.border);
        }
        else
        {
            DrawVLine(dc, item.right - 1, item.top + 3, item.bottom - 3, p.border);
        }

        wchar_t text[256] = {};
        TCITEMW tci = {};
        tci.mask = TCIF_TEXT;
        tci.pszText = text;
        tci.cchTextMax = 256;
        TabCtrl_GetItem(hwnd, i, &tci);

        RECT rcText = item;
        rcText.left += 4;
        rcText.right -= 4;
        ::SetTextColor(dc, isSelected ? p.tabText : p.tabInactiveText);
        ::DrawTextW(dc, text, -1, &rcText, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    // Line under the strip, interrupted by the selected tab.
    int y = stripBottom - 1;
    if (selectedRect.right > selectedRect.left)
    {
        DrawHLine(dc, rc.left, selectedRect.left + 1, y, p.border);
        DrawHLine(dc, selectedRect.right - 1, rc.right, y, p.border);
    }
    else
    {
        DrawHLine(dc, rc.left, rc.right, y, p.border);
    }
}

LRESULT CALLBACK TabProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR)
{
    if (IsEnabled())
    {
        if (msg == WM_ERASEBKGND)
            return 1;
        if (msg == WM_PAINT || msg == WM_PRINTCLIENT)
        {
            PaintBuffered(hwnd, wParam, msg == WM_PRINTCLIENT, [hwnd](HDC dc, const RECT& rc) { PaintTabs(hwnd, dc, rc); });
            return 0;
        }
    }
    if (msg == WM_NCDESTROY)
        ::RemoveWindowSubclass(hwnd, TabProc, id);
    return ::DefSubclassProc(hwnd, msg, wParam, lParam);
}

// Status bar --------------------------------------------------------------------------------------
void PaintStatusBar(HWND hwnd, HDC dc, const RECT& rc)
{
    const Palette p = GetPalette();
    FillRectColor(dc, rc, p.statusBg);
    DrawHLine(dc, rc.left, rc.right, rc.top, p.border);
    ::SetTextColor(dc, p.statusText);

    int parts = static_cast<int>(::SendMessageW(hwnd, SB_GETPARTS, 0, 0));
    for (int i = 0; i < parts; ++i)
    {
        RECT part = {};
        if (!::SendMessageW(hwnd, SB_GETRECT, i, reinterpret_cast<LPARAM>(&part)))
            continue;

        DWORD info = static_cast<DWORD>(::SendMessageW(hwnd, SB_GETTEXTLENGTHW, i, 0));
        int length = LOWORD(info);
        if ((HIWORD(info) & SBT_OWNERDRAW) == 0 && length > 0)
        {
            std::wstring text(static_cast<size_t>(length) + 1, L'\0');
            ::SendMessageW(hwnd, SB_GETTEXTW, i, reinterpret_cast<LPARAM>(&text[0]));
            RECT rcText = part;
            rcText.left += 4;
            rcText.right -= 3;
            ::DrawTextW(dc, text.c_str(), -1, &rcText, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX);
        }
        DrawVLine(dc, part.right, part.top + 3, part.bottom - 2, p.border);
    }
}

LRESULT CALLBACK StatusBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR)
{
    if (IsEnabled())
    {
        if (msg == WM_ERASEBKGND)
            return 1;
        if (msg == WM_PAINT || msg == WM_PRINTCLIENT)
        {
            PaintBuffered(hwnd, wParam, msg == WM_PRINTCLIENT, [hwnd](HDC dc, const RECT& rc) { PaintStatusBar(hwnd, dc, rc); });
            return 0;
        }
    }
    if (msg == WM_NCDESTROY)
        ::RemoveWindowSubclass(hwnd, StatusBarProc, id);
    return ::DefSubclassProc(hwnd, msg, wParam, lParam);
}

// Toolbars and the command bar: only the background is handled here, the buttons are drawn from the
// NM_CUSTOMDRAW notification caught in FrameProc.
LRESULT CALLBACK ToolbarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR)
{
    if (IsEnabled() && msg == WM_ERASEBKGND)
    {
        RECT rc = {};
        ::GetClientRect(hwnd, &rc);
        FillRectColor(reinterpret_cast<HDC>(wParam), rc, GetPalette().bar);
        return 1;
    }
    if (msg == WM_NCDESTROY)
        ::RemoveWindowSubclass(hwnd, ToolbarProc, id);
    return ::DefSubclassProc(hwnd, msg, wParam, lParam);
}

// Group boxes -------------------------------------------------------------------------------------
LRESULT CALLBACK GroupBoxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR)
{
    if (IsEnabled() && (msg == WM_PAINT || msg == WM_PRINTCLIENT))
    {
        const Palette p = GetPalette();
        PAINTSTRUCT ps = {};
        HDC dc = (msg == WM_PRINTCLIENT) ? reinterpret_cast<HDC>(wParam) : ::BeginPaint(hwnd, &ps);
        RECT rc = {};
        ::GetClientRect(hwnd, &rc);

        HGDIOBJ oldFont = ::SelectObject(dc, GetControlFont(hwnd));
        wchar_t text[256] = {};
        ::GetWindowTextW(hwnd, text, 256);
        SIZE size = {};
        ::GetTextExtentPoint32W(dc, L"Ag", 2, &size);

        RECT frame = rc;
        frame.top += size.cy / 2;
        FillRectColor(dc, rc, p.dialog);
        HBRUSH oldBrush = reinterpret_cast<HBRUSH>(::SelectObject(dc, ::GetStockObject(NULL_BRUSH)));
        HPEN pen = ::CreatePen(PS_SOLID, 1, p.border);
        HGDIOBJ oldPen = ::SelectObject(dc, pen);
        ::Rectangle(dc, frame.left, frame.top, frame.right, frame.bottom);
        ::SelectObject(dc, oldPen);
        ::SelectObject(dc, oldBrush);
        ::DeleteObject(pen);

        if (text[0])
        {
            SIZE textSize = {};
            ::GetTextExtentPoint32W(dc, text, static_cast<int>(wcslen(text)), &textSize);
            RECT rcText = {rc.left + 8, rc.top, rc.left + 8 + textSize.cx + 6, rc.top + size.cy};
            FillRectColor(dc, rcText, p.dialog);
            ::SetBkMode(dc, TRANSPARENT);
            ::SetTextColor(dc, (::GetWindowLongW(hwnd, GWL_STYLE) & WS_DISABLED) ? p.disabledText : p.barText);
            rcText.left += 3;
            ::DrawTextW(dc, text, -1, &rcText, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX);
        }
        ::SelectObject(dc, oldFont);
        if (msg != WM_PRINTCLIENT)
            ::EndPaint(hwnd, &ps);
        return 0;
    }
    if (msg == WM_NCDESTROY)
        ::RemoveWindowSubclass(hwnd, GroupBoxProc, id);
    return ::DefSubclassProc(hwnd, msg, wParam, lParam);
}

// Radio buttons -----------------------------------------------------------------------------------
// The dark visual style draws the label of radio buttons in a colour that is almost invisible on the dark
// dialog background, so they are painted here: ring (filled with a dot when checked) plus the label.
LRESULT CALLBACK RadioButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR)
{
    if (IsEnabled())
    {
        if (msg == WM_ERASEBKGND)
            return 1;
        if (msg == WM_PAINT || msg == WM_PRINTCLIENT)
        {
            PaintBuffered(hwnd, wParam, msg == WM_PRINTCLIENT, [hwnd](HDC dc, const RECT& rc) {
                const Palette p = GetPalette();
                const bool enabled = ::IsWindowEnabled(hwnd) != FALSE;
                const bool checked = ::SendMessageW(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
                const COLORREF accent = RGB(76, 194, 255);
                FillRectColor(dc, rc, p.dialog);

                const int dpi = ::GetDeviceCaps(dc, LOGPIXELSX);
                const int size = ::MulDiv(13, dpi, 96);
                const int top = rc.top + ((rc.bottom - rc.top) - size) / 2;
                RECT circle = {rc.left + 1, top, rc.left + 1 + size, top + size};

                HGDIOBJ oldBrush = ::SelectObject(dc, ::GetStockObject(NULL_BRUSH));
                HPEN pen = ::CreatePen(PS_SOLID, 1, !enabled ? p.disabledText : (checked ? accent : RGB(170, 170, 170)));
                HGDIOBJ oldPen = ::SelectObject(dc, pen);
                ::Ellipse(dc, circle.left, circle.top, circle.right, circle.bottom);
                ::SelectObject(dc, oldPen);
                ::DeleteObject(pen);
                if (checked)
                {
                    const int inset = (std::max)(3, size / 4);
                    HBRUSH dot = ::CreateSolidBrush(enabled ? accent : p.disabledText);
                    ::SelectObject(dc, dot);
                    HGDIOBJ nullPen = ::SelectObject(dc, ::GetStockObject(NULL_PEN));
                    ::Ellipse(dc, circle.left + inset, circle.top + inset, circle.right - inset + 1, circle.bottom - inset + 1);
                    ::SelectObject(dc, nullPen);
                    ::SelectObject(dc, oldBrush);
                    ::DeleteObject(dot);
                }
                else
                {
                    ::SelectObject(dc, oldBrush);
                }

                wchar_t text[256] = {};
                ::GetWindowTextW(hwnd, text, 256);
                RECT rcText = rc;
                rcText.left = circle.right + ::MulDiv(6, dpi, 96);
                ::SetTextColor(dc, enabled ? p.barText : p.disabledText);
                ::DrawTextW(dc, text, -1, &rcText, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_HIDEPREFIX);

                if (::GetFocus() == hwnd)
                {
                    RECT focus = rcText;
                    SIZE extent = {};
                    ::GetTextExtentPoint32W(dc, text, static_cast<int>(wcslen(text)), &extent);
                    focus.right = (std::min)(rcText.right, rcText.left + extent.cx + 2);
                    focus.top = rc.top + ((rc.bottom - rc.top) - extent.cy) / 2 - 1;
                    focus.bottom = focus.top + extent.cy + 2;
                    ::DrawFocusRect(dc, &focus);
                }
            });
            return 0;
        }
        if (msg == WM_SETFOCUS || msg == WM_KILLFOCUS || msg == BM_SETCHECK)
        {
            LRESULT result = ::DefSubclassProc(hwnd, msg, wParam, lParam);
            ::InvalidateRect(hwnd, nullptr, FALSE);
            return result;
        }
    }
    if (msg == WM_NCDESTROY)
        ::RemoveWindowSubclass(hwnd, RadioButtonProc, id);
    return ::DefSubclassProc(hwnd, msg, wParam, lParam);
}

// Dialogs -----------------------------------------------------------------------------------------
LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR)
{
    if (IsEnabled())
    {
        const Palette p = GetPalette();
        switch (msg)
        {
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        {
            HDC dc = reinterpret_cast<HDC>(wParam);
            ::SetTextColor(dc, p.barText);
            ::SetBkColor(dc, p.dialog);
            return reinterpret_cast<LRESULT>(GetBrush(p.dialog));
        }
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        {
            HDC dc = reinterpret_cast<HDC>(wParam);
            ::SetTextColor(dc, p.barText);
            ::SetBkColor(dc, p.edit);
            return reinterpret_cast<LRESULT>(GetBrush(p.edit));
        }
        case WM_ERASEBKGND:
        {
            RECT rc = {};
            ::GetClientRect(hwnd, &rc);
            FillRectColor(reinterpret_cast<HDC>(wParam), rc, p.dialog);
            return 1;
        }
        default:
            break;
        }
    }
    if (msg == WM_NCDESTROY)
        ::RemoveWindowSubclass(hwnd, DialogProc, id);
    return ::DefSubclassProc(hwnd, msg, wParam, lParam);
}

// Frame: draws the buttons of the command bar and of the toolbars in the rebar ---------------------
int ButtonTextLength(HWND bar, int command, wchar_t* buffer, int size)
{
    buffer[0] = L'\0';
    int length = static_cast<int>(::SendMessageW(bar, TB_GETBUTTONTEXTW, command, reinterpret_cast<LPARAM>(buffer)));
    if (length < 0 || length >= size)
        return 0;
    return length;
}

void DrawDropArrow(HDC dc, const RECT& rc, COLORREF color)
{
    int cx = (rc.left + rc.right) / 2;
    int cy = (rc.top + rc.bottom) / 2;
    POINT pts[3] = {{cx - 3, cy - 1}, {cx + 3, cy - 1}, {cx, cy + 2}};
    HBRUSH brush = GetBrush(color);
    HGDIOBJ oldBrush = ::SelectObject(dc, brush);
    HGDIOBJ oldPen = ::SelectObject(dc, ::GetStockObject(NULL_PEN));
    ::Polygon(dc, pts, 3);
    ::SelectObject(dc, oldPen);
    ::SelectObject(dc, oldBrush);
}

LRESULT DrawToolbarItem(NMTBCUSTOMDRAW* tb)
{
    const Palette p = GetPalette();
    HWND bar = tb->nmcd.hdr.hwndFrom;
    HDC dc = tb->nmcd.hdc;
    RECT rc = tb->nmcd.rc;
    const UINT state = tb->nmcd.uItemState;
    const int command = static_cast<int>(tb->nmcd.dwItemSpec);

    const bool disabled = (state & CDIS_DISABLED) != 0;
    const bool down = (state & (CDIS_SELECTED | CDIS_CHECKED)) != 0;
    const bool hot = (state & CDIS_HOT) != 0;

    // Separators carry no command. The command bar numbers its buttons from 0, so a narrow item is needed to tell
    // a separator from the first menu ("File").
    if (command == 0 && (rc.right - rc.left) <= 10)
    {
        DrawVLine(dc, (rc.left + rc.right) / 2, rc.top + 3, rc.bottom - 3, p.border);
        return CDRF_SKIPDEFAULT;
    }

    wchar_t text[128] = {};
    const int textLength = ButtonTextLength(bar, command, text, 128);

    COLORREF background = p.bar;
    if (!disabled)
    {
        if (down)
            background = IsCommandBar(bar) ? p.hot : p.pressed;
        else if (hot)
            background = p.hot;
    }
    FillRectColor(dc, rc, background);

    if (textLength > 0)
    {
        HGDIOBJ oldFont = ::SelectObject(dc, GetControlFont(bar));
        ::SetBkMode(dc, TRANSPARENT);
        ::SetTextColor(dc, disabled ? p.disabledText : p.barText);
        // Menu bar items carry a '&' mnemonic: keep the prefix handling, but do not underline it.
        ::DrawTextW(dc, text, -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_HIDEPREFIX);
        ::SelectObject(dc, oldFont);
        return CDRF_SKIPDEFAULT;
    }

    // Image button
    TBBUTTONINFOW info = {};
    info.cbSize = sizeof(info);
    info.dwMask = TBIF_IMAGE | TBIF_STYLE;
    if (::SendMessageW(bar, TB_GETBUTTONINFOW, command, reinterpret_cast<LPARAM>(&info)) < 0 || info.iImage < 0)
        return CDRF_SKIPDEFAULT;

    if (down && !disabled && !IsCommandBar(bar))
    {
        RECT frame = rc;
        ::FrameRect(dc, &frame, GetBrush(RGB(70, 120, 175)));
    }

    const bool dropDown = (info.fsStyle & (BTNS_DROPDOWN | BTNS_WHOLEDROPDOWN)) != 0;
    RECT rcImage = rc;
    if (dropDown)
    {
        RECT rcArrow = rc;
        rcArrow.left = rc.right - 12;
        DrawDropArrow(dc, rcArrow, disabled ? p.disabledText : p.barText);
        rcImage.right = rcArrow.left;
    }

    HIMAGELIST images = reinterpret_cast<HIMAGELIST>(::SendMessageW(bar, TB_GETIMAGELIST, 0, 0));
    bool useDisabledList = false;
    if (disabled)
    {
        HIMAGELIST disabledList = reinterpret_cast<HIMAGELIST>(::SendMessageW(bar, TB_GETDISABLEDIMAGELIST, 0, 0));
        if (disabledList)
        {
            images = disabledList;
            useDisabledList = true;
        }
    }
    else if (hot)
    {
        HIMAGELIST hotList = reinterpret_cast<HIMAGELIST>(::SendMessageW(bar, TB_GETHOTIMAGELIST, 0, 0));
        if (hotList)
            images = hotList;
    }
    if (images)
    {
        int cx = 0, cy = 0;
        ImageList_GetIconSize(images, &cx, &cy);
        int x = rcImage.left + ((rcImage.right - rcImage.left) - cx) / 2;
        int y = rcImage.top + ((rcImage.bottom - rcImage.top) - cy) / 2;
        UINT style = ILD_TRANSPARENT;
        if (disabled && !useDisabledList)
            style |= ILD_BLEND50;
        ImageList_DrawEx(images, info.iImage, dc, x, y, 0, 0, CLR_NONE, disabled ? p.bar : CLR_NONE, style);
    }
    return CDRF_SKIPDEFAULT;
}

LRESULT CALLBACK FrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR)
{
    if (msg == WM_NOTIFY && IsEnabled())
    {
        auto* header = reinterpret_cast<NMHDR*>(lParam);
        if (header && header->code == NM_CUSTOMDRAW && IsRebarToolbar(header->hwndFrom))
        {
            auto* tb = reinterpret_cast<NMTBCUSTOMDRAW*>(lParam);
            switch (tb->nmcd.dwDrawStage)
            {
            case CDDS_PREPAINT:
                return CDRF_NOTIFYITEMDRAW;
            case CDDS_ITEMPREPAINT:
                return DrawToolbarItem(tb);
            default:
                return CDRF_DODEFAULT;
            }
        }
    }
    if (msg == WM_NCDESTROY)
        ::RemoveWindowSubclass(hwnd, FrameProc, id);
    return ::DefSubclassProc(hwnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------------------------------------------
// Control specific treatment
// ---------------------------------------------------------------------------------------------------
void ApplyRebar(HWND hwnd, bool dark)
{
    const Palette p = GetPalette();
    // Visual styles ignore the colours below, so they are switched off while the rebar is dark.
    ::SetWindowTheme(hwnd, dark ? L"" : nullptr, dark ? L"" : nullptr);
    ::SendMessageW(hwnd, RB_SETBKCOLOR, 0, dark ? p.bar : CLR_DEFAULT);
    ::SendMessageW(hwnd, RB_SETTEXTCOLOR, 0, dark ? p.barText : CLR_DEFAULT);

    int bands = static_cast<int>(::SendMessageW(hwnd, RB_GETBANDCOUNT, 0, 0));
    for (int i = 0; i < bands; ++i)
    {
        REBARBANDINFOW rbbi = {};
        rbbi.cbSize = sizeof(rbbi);
        rbbi.fMask = RBBIM_COLORS;
        rbbi.clrFore = dark ? p.barText : ::GetSysColor(COLOR_BTNTEXT);
        rbbi.clrBack = dark ? p.bar : ::GetSysColor(COLOR_BTNFACE);
        ::SendMessageW(hwnd, RB_SETBANDINFOW, i, reinterpret_cast<LPARAM>(&rbbi));
    }
    ::InvalidateRect(hwnd, nullptr, TRUE);
}

void ApplyListView(HWND hwnd, bool dark)
{
    const auto& c = ThemeManager::Instance().GetColors();
    SetControlTheme(hwnd, dark, L"DarkMode_Explorer");
    ListView_SetBkColor(hwnd, dark ? c.background : ::GetSysColor(COLOR_WINDOW));
    ListView_SetTextBkColor(hwnd, dark ? c.background : ::GetSysColor(COLOR_WINDOW));
    ListView_SetTextColor(hwnd, dark ? c.text : ::GetSysColor(COLOR_WINDOWTEXT));

    HWND header = ListView_GetHeader(hwnd);
    if (header)
    {
        ::SetWindowSubclass(header, HeaderProc, kSubclassId, 0);
        ::InvalidateRect(header, nullptr, TRUE);
    }
}

void ApplyTreeView(HWND hwnd, bool dark)
{
    const auto& c = ThemeManager::Instance().GetColors();
    SetControlTheme(hwnd, dark, L"DarkMode_Explorer");
    TreeView_SetBkColor(hwnd, dark ? c.background : ::GetSysColor(COLOR_WINDOW));
    TreeView_SetTextColor(hwnd, dark ? c.text : ::GetSysColor(COLOR_WINDOWTEXT));
}

BOOL CALLBACK EnumChildApply(HWND hwnd, LPARAM)
{
    ApplyToControl(hwnd);
    return TRUE;
}

void ApplyToWindowTree(HWND root)
{
    ApplyToControl(root);
    ::EnumChildWindows(root, EnumChildApply, 0);
}

// ---------------------------------------------------------------------------------------------------
// Thread hook: themes dialogs and controls as they are created
// ---------------------------------------------------------------------------------------------------
void OnDialogInit(HWND dlg)
{
    if (IsShellDialog(dlg))
        return;
    ::SetWindowSubclass(dlg, DialogProc, kSubclassId, 0);
    ApplyToWindowTree(dlg);
    if ((::GetWindowLongPtrW(dlg, GWL_STYLE) & WS_CHILD) == 0)
        SetDarkTitleBar(dlg, IsEnabled());
}

LRESULT CALLBACK CallWndRetProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HC_ACTION)
    {
        auto* msg = reinterpret_cast<CWPRETSTRUCT*>(lParam);
        if (msg->message == WM_INITDIALOG)
            OnDialogInit(msg->hwnd);
        else if (msg->message == WM_CREATE && msg->lResult == 0)
            ApplyToControl(msg->hwnd);
    }
    return ::CallNextHookEx(g_hook, code, wParam, lParam);
}

BOOL CALLBACK EnumThreadWindowsApply(HWND hwnd, LPARAM lParam)
{
    HWND skip = reinterpret_cast<HWND>(lParam);
    if (hwnd == skip || !::IsWindowVisible(hwnd) || IsClass(hwnd, L"#32768") || IsShellDialog(hwnd))
        return TRUE;
    ApplyToWindowTree(hwnd);
    SetDarkTitleBar(hwnd, IsEnabled());
    ::RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
    return TRUE;
}

} // namespace

// -------------------------------------------------------------------------------------------------------
// Public interface
// -------------------------------------------------------------------------------------------------------
void Initialize()
{
    if (g_initialized)
        return;
    g_initialized = true;

    HMODULE uxtheme = ::GetModuleHandleW(L"uxtheme.dll");
    if (!uxtheme)
        uxtheme = ::LoadLibraryW(L"uxtheme.dll");
    if (uxtheme)
    {
        pSetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(::GetProcAddress(uxtheme, MAKEINTRESOURCEA(135)));
        pFlushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(::GetProcAddress(uxtheme, MAKEINTRESOURCEA(136)));
        pRefreshImmersiveColorPolicyState =
            reinterpret_cast<fnRefreshImmersiveColorPolicyState>(::GetProcAddress(uxtheme, MAKEINTRESOURCEA(104)));
    }

    g_hook = ::SetWindowsHookExW(WH_CALLWNDPROCRET, CallWndRetProc, nullptr, ::GetCurrentThreadId());
}

bool IsEnabled()
{
    return ThemeManager::Instance().IsDarkTheme();
}

MenuColors GetMenuColors()
{
    const auto& c = ThemeManager::Instance().GetColors();
    MenuColors m;
    m.background = c.menuBackground;
    m.text = c.menuText;
    m.disabledText = RGB(120, 120, 120);
    m.highlight = RGB(62, 62, 62);
    m.border = RGB(85, 85, 85);
    m.accent = c.itemHighlight;
    return m;
}

void InstallFrameHooks(HWND frame)
{
    if (frame)
        ::SetWindowSubclass(frame, FrameProc, kSubclassId, 0);
}

void ApplyToControl(HWND hwnd)
{
    if (!hwnd || !::IsWindow(hwnd) || InsideShellView(hwnd))
        return;

    wchar_t cls[80] = {};
    if (::GetClassNameW(hwnd, cls, 80) <= 0)
        return;
    const bool dark = IsEnabled();

    if (_wcsicmp(cls, L"SysListView32") == 0)
    {
        ApplyListView(hwnd, dark);
    }
    else if (_wcsicmp(cls, L"SysHeader32") == 0)
    {
        ::SetWindowSubclass(hwnd, HeaderProc, kSubclassId, 0);
    }
    else if (_wcsicmp(cls, L"SysTabControl32") == 0)
    {
        ::SetWindowSubclass(hwnd, TabProc, kSubclassId, 0);
        ::InvalidateRect(hwnd, nullptr, TRUE);
    }
    else if (_wcsicmp(cls, L"msctls_statusbar32") == 0)
    {
        ::SetWindowSubclass(hwnd, StatusBarProc, kSubclassId, 0);
        ::InvalidateRect(hwnd, nullptr, TRUE);
    }
    else if (_wcsicmp(cls, L"ReBarWindow32") == 0)
    {
        ApplyRebar(hwnd, dark);
    }
    else if (_wcsicmp(cls, L"ToolbarWindow32") == 0 || IsCommandBar(hwnd))
    {
        if (IsRebarToolbar(hwnd))
        {
            ::SetWindowSubclass(hwnd, ToolbarProc, kSubclassId, 0);
            ::InvalidateRect(hwnd, nullptr, TRUE);
        }
    }
    else if (_wcsicmp(cls, L"Button") == 0)
    {
        const LONG style = ::GetWindowLongW(hwnd, GWL_STYLE);
        const LONG type = style & BS_TYPEMASK;
        if (type == BS_GROUPBOX)
        {
            ::SetWindowSubclass(hwnd, GroupBoxProc, kSubclassId, 0);
            ::InvalidateRect(hwnd, nullptr, TRUE);
        }
        else if (type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON)
        {
            ::SetWindowSubclass(hwnd, RadioButtonProc, kSubclassId, 0);
            SetControlTheme(hwnd, dark, L"DarkMode_Explorer");
            ::InvalidateRect(hwnd, nullptr, TRUE);
        }
        else if (type != BS_OWNERDRAW)
        {
            SetControlTheme(hwnd, dark, L"DarkMode_Explorer");
        }
    }
    else if (_wcsicmp(cls, L"Edit") == 0)
    {
        const bool multiline = (::GetWindowLongW(hwnd, GWL_STYLE) & ES_MULTILINE) != 0;
        SetControlTheme(hwnd, dark, multiline ? L"DarkMode_Explorer" : L"DarkMode_CFD");
    }
    else if (_wcsicmp(cls, L"ComboBox") == 0)
    {
        SetControlTheme(hwnd, dark, L"DarkMode_CFD");
    }
    else if (_wcsicmp(cls, L"ListBox") == 0 || _wcsicmp(cls, L"ScrollBar") == 0 || _wcsicmp(cls, L"Scintilla") == 0)
    {
        SetControlTheme(hwnd, dark, L"DarkMode_Explorer");
    }
    else if (_wcsicmp(cls, L"SysTreeView32") == 0)
    {
        ApplyTreeView(hwnd, dark);
    }
    else if (LooksLikeListView(hwnd))
    {
        ApplyListView(hwnd, dark);
    }
}

void Refresh(HWND root)
{
    if (!g_initialized)
        Initialize();

    const bool dark = IsEnabled();
    if (pSetPreferredAppMode)
        pSetPreferredAppMode(dark ? AppModeForceDark : AppModeForceLight);
    if (pRefreshImmersiveColorPolicyState)
        pRefreshImmersiveColorPolicyState();
    if (pFlushMenuThemes)
        pFlushMenuThemes();

    if (root)
    {
        ApplyToWindowTree(root);
        SetDarkTitleBar(root, dark);
        ::RedrawWindow(root, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
    }
    // Floating windows and modeless dialogs of this thread.
    ::EnumThreadWindows(::GetCurrentThreadId(), EnumThreadWindowsApply, reinterpret_cast<LPARAM>(root));
}

} // namespace DarkMode
} // namespace debugviewpp
} // namespace fusion
