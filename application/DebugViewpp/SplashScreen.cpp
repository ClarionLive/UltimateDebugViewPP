// (C) Copyright ClarionLive 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "SplashScreen.h"

#include <atomic>
#include <mutex>
#include <thread>

#include <commctrl.h>

#include "resource.h"

namespace fusion {
namespace debugviewpp {
namespace SplashScreen {

namespace {

constexpr UINT WM_SPLASH_UPDATE = WM_APP + 1;
constexpr int Width = 460;
constexpr int Height = 170;
constexpr COLORREF Background = RGB(255, 255, 255);
constexpr COLORREF Accent = RGB(0, 99, 177);
constexpr COLORREF TextColor = RGB(40, 40, 40);
constexpr COLORREF SubtleText = RGB(110, 110, 110);

struct State
{
    std::thread thread;
    std::atomic<HWND> hwnd{nullptr};
    HWND progress = nullptr;
    std::mutex mutex;
    std::wstring status = L"Starting...";
    std::wstring version;
    std::atomic<int> percent{0};
    HFONT titleFont = nullptr;
    HFONT textFont = nullptr;
    HICON icon = nullptr;
};

State g_state;
std::mutex g_lifetime;  // serialises Start and Close

void Paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC dc = ::BeginPaint(hwnd, &ps);
    RECT rc;
    ::GetClientRect(hwnd, &rc);

    HBRUSH background = ::CreateSolidBrush(Background);
    ::FillRect(dc, &rc, background);
    ::DeleteObject(background);

    RECT band = {0, 0, rc.right, 6};
    HBRUSH accent = ::CreateSolidBrush(Accent);
    ::FillRect(dc, &band, accent);
    ::DeleteObject(accent);

    if (g_state.icon != nullptr)
        ::DrawIconEx(dc, 24, 28, g_state.icon, 48, 48, 0, nullptr, DI_NORMAL);

    ::SetBkMode(dc, TRANSPARENT);
    auto old = ::SelectObject(dc, g_state.titleFont);
    ::SetTextColor(dc, TextColor);
    RECT title = {88, 26, rc.right - 20, 60};
    ::DrawTextW(dc, L"Ultimate DebugView++", -1, &title, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    ::SelectObject(dc, g_state.textFont);
    ::SetTextColor(dc, SubtleText);
    RECT version = {88, 60, rc.right - 20, 80};
    std::wstring versionText = L"Version " + g_state.version;
    ::DrawTextW(dc, versionText.c_str(), -1, &version, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    std::wstring status;
    {
        std::lock_guard<std::mutex> lock(g_state.mutex);
        status = g_state.status;
    }
    ::SetTextColor(dc, TextColor);
    RECT statusRect = {24, 100, rc.right - 24, 120};
    ::DrawTextW(dc, status.c_str(), -1, &statusRect, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

    ::SelectObject(dc, old);
    ::EndPaint(hwnd, &ps);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
        Paint(hwnd);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_SPLASH_UPDATE:
    {
        if (g_state.progress != nullptr)
            ::SendMessageW(g_state.progress, PBM_SETPOS, static_cast<WPARAM>(g_state.percent.load()), 0);
        RECT statusRect = {24, 100, Width - 24, 120};
        ::InvalidateRect(hwnd, &statusRect, FALSE);
        return 0;
    }
    case WM_NCHITTEST:
        return HTCAPTION;  // drag it anywhere
    case WM_CLOSE:
        ::DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    default:
        return ::DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void ThreadMain(HINSTANCE instance, HANDLE ready)
{
    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_PROGRESS_CLASS};
    ::InitCommonControlsEx(&icc);

    WNDCLASSEXW wc = {sizeof(wc)};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = instance;
    wc.hCursor = ::LoadCursorW(nullptr, IDC_APPSTARTING);
    wc.lpszClassName = L"UltimateDebugViewSplash";
    ::RegisterClassExW(&wc);

    g_state.icon = static_cast<HICON>(::LoadImageW(instance, MAKEINTRESOURCEW(IDR_MAINFRAME), IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR));
    NONCLIENTMETRICSW metrics = {sizeof(metrics)};
    ::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
    LOGFONTW title = metrics.lfMessageFont;
    title.lfHeight = -22;
    title.lfWeight = FW_SEMIBOLD;
    g_state.titleFont = ::CreateFontIndirectW(&title);
    LOGFONTW text = metrics.lfMessageFont;
    text.lfHeight = -13;
    g_state.textFont = ::CreateFontIndirectW(&text);

    // Centre on the monitor with the mouse: that is where the user just launched it from.
    POINT cursor = {};
    ::GetCursorPos(&cursor);
    MONITORINFO monitor = {sizeof(monitor)};
    ::GetMonitorInfoW(::MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY), &monitor);
    const RECT& work = monitor.rcWork;
    int x = work.left + (work.right - work.left - Width) / 2;
    int y = work.top + (work.bottom - work.top - Height) / 2;

    HWND hwnd = ::CreateWindowExW(WS_EX_TOOLWINDOW, wc.lpszClassName, L"Ultimate DebugView++", WS_POPUP | WS_BORDER, x, y, Width, Height,
        nullptr, nullptr, instance, nullptr);
    if (hwnd != nullptr)
    {
        g_state.progress = ::CreateWindowExW(0, PROGRESS_CLASSW, nullptr, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, 24, 128, Width - 50, 16, hwnd,
            nullptr, instance, nullptr);
        ::SendMessageW(g_state.progress, PBM_SETRANGE32, 0, 100);
        ::SendMessageW(g_state.progress, PBM_SETPOS, static_cast<WPARAM>(g_state.percent.load()), 0);
        ::ShowWindow(hwnd, SW_SHOWNORMAL);
        ::UpdateWindow(hwnd);
        g_state.hwnd = hwnd;
    }
    ::SetEvent(ready);

    MSG msg;
    while (hwnd != nullptr && ::GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }

    g_state.hwnd = nullptr;
    g_state.progress = nullptr;
    if (g_state.titleFont != nullptr)
        ::DeleteObject(g_state.titleFont);
    if (g_state.textFont != nullptr)
        ::DeleteObject(g_state.textFont);
    if (g_state.icon != nullptr)
        ::DestroyIcon(g_state.icon);
    g_state.titleFont = g_state.textFont = nullptr;
    g_state.icon = nullptr;
}

} // namespace

void Start(HINSTANCE instance, const std::wstring& version)
{
    std::lock_guard<std::mutex> lock(g_lifetime);
    if (g_state.thread.joinable())
        return;
    g_state.version = version;
    HANDLE ready = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (ready == nullptr)
        return;
    g_state.thread = std::thread(ThreadMain, instance, ready);
    ::WaitForSingleObject(ready, 5000);  // the splash is on screen before the slow part starts
    ::CloseHandle(ready);
}

void Progress(int percent, const std::wstring& text)
{
    {
        std::lock_guard<std::mutex> lock(g_state.mutex);
        g_state.status = text;
    }
    g_state.percent = percent < 0 ? 0 : (percent > 100 ? 100 : percent);
    HWND hwnd = g_state.hwnd;
    if (hwnd != nullptr)
        ::PostMessageW(hwnd, WM_SPLASH_UPDATE, 0, 0);
}

void Close()
{
    std::lock_guard<std::mutex> lock(g_lifetime);
    if (!g_state.thread.joinable())
        return;
    HWND hwnd = g_state.hwnd;
    if (hwnd != nullptr)
        ::PostMessageW(hwnd, WM_CLOSE, 0, 0);
    else
        ::PostThreadMessageW(::GetThreadId(g_state.thread.native_handle()), WM_QUIT, 0, 0);
    g_state.thread.join();
}

} // namespace SplashScreen
} // namespace debugviewpp
} // namespace fusion
