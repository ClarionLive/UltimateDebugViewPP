#include "HelpTabView.h"
#include "resource.h"
#include <shlwapi.h>
#include <string>
#include <cwctype>
#include <WebView2EnvironmentOptions.h>

using namespace Microsoft::WRL;

namespace fusion {
namespace debugviewpp {

// Base class for WebView2 callbacks
class CallbackBase : public IUnknown
{
public:
    CallbackBase() : m_refCount(1) {}
    virtual ~CallbackBase() = default;

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppvObject = static_cast<IUnknown*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return InterlockedIncrement(&m_refCount);
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG refCount = InterlockedDecrement(&m_refCount);
        if (refCount == 0)
            delete this;
        return refCount;
    }

protected:
    LONG m_refCount;
};

// Specializations for WebView2 callbacks
class EnvCompletedHandler : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
{
public:
    EnvCompletedHandler(std::function<HRESULT(HRESULT, ICoreWebView2Environment*)> callback)
        : m_callback(callback), m_refCount(1) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
    {
        if (riid == __uuidof(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler) ||
            riid == __uuidof(IUnknown))
        {
            *ppvObject = static_cast<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return InterlockedIncrement(&m_refCount);
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG refCount = InterlockedDecrement(&m_refCount);
        if (refCount == 0)
            delete this;
        return refCount;
    }

    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Environment* environment) override
    {
        return m_callback(result, environment);
    }

private:
    std::function<HRESULT(HRESULT, ICoreWebView2Environment*)> m_callback;
    LONG m_refCount;
};

class ControllerCompletedHandler : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
{
public:
    ControllerCompletedHandler(std::function<HRESULT(HRESULT, ICoreWebView2Controller*)> callback)
        : m_callback(callback), m_refCount(1) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
    {
        if (riid == __uuidof(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler) ||
            riid == __uuidof(IUnknown))
        {
            *ppvObject = static_cast<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return InterlockedIncrement(&m_refCount);
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG refCount = InterlockedDecrement(&m_refCount);
        if (refCount == 0)
            delete this;
        return refCount;
    }

    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Controller* controller) override
    {
        return m_callback(result, controller);
    }

private:
    std::function<HRESULT(HRESULT, ICoreWebView2Controller*)> m_callback;
    LONG m_refCount;
};

class NavigationCompletedHandler : public ICoreWebView2NavigationCompletedEventHandler
{
public:
    NavigationCompletedHandler(std::function<HRESULT(ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs*)> callback)
        : m_callback(callback), m_refCount(1) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
    {
        if (riid == __uuidof(ICoreWebView2NavigationCompletedEventHandler) ||
            riid == __uuidof(IUnknown))
        {
            *ppvObject = static_cast<ICoreWebView2NavigationCompletedEventHandler*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return InterlockedIncrement(&m_refCount);
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG refCount = InterlockedDecrement(&m_refCount);
        if (refCount == 0)
            delete this;
        return refCount;
    }

    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) override
    {
        return m_callback(sender, args);
    }

private:
    std::function<HRESULT(ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs*)> m_callback;
    LONG m_refCount;
};

class NavigationStartingHandler : public ICoreWebView2NavigationStartingEventHandler
{
public:
    NavigationStartingHandler(std::function<HRESULT(ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs*)> callback)
        : m_callback(callback), m_refCount(1) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
    {
        if (riid == __uuidof(ICoreWebView2NavigationStartingEventHandler) ||
            riid == __uuidof(IUnknown))
        {
            *ppvObject = static_cast<ICoreWebView2NavigationStartingEventHandler*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return InterlockedIncrement(&m_refCount);
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG refCount = InterlockedDecrement(&m_refCount);
        if (refCount == 0)
            delete this;
        return refCount;
    }

    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) override
    {
        return m_callback(sender, args);
    }

private:
    std::function<HRESULT(ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs*)> m_callback;
    LONG m_refCount;
};

CHelpTabView::CHelpTabView(const std::wstring& helpTitle)
    : m_helpTitle(helpTitle)
    , m_webViewReady(false)
    , m_isVisible(true)
    , m_currentZoomFactor(1.0)
{
}

CHelpTabView::~CHelpTabView()
{
    if (m_webView)
    {
        m_webView->remove_NavigationCompleted(m_navigationCompletedToken);
        m_webView->remove_NavigationStarting(m_navigationStartingToken);
    }
}

void CHelpTabView::Create(HWND parent, const RECT& rc)
{
    RECT rcCopy = rc;
    CWindowImpl<CHelpTabView>::Create(parent, &rcCopy, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        WS_EX_CONTROLPARENT);
}

LRESULT CHelpTabView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CreateNavigationToolbar();
    InitializeWebView();
    return 0;
}

LRESULT CHelpTabView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    ResizeWebView();
    return 0;
}

LRESULT CHelpTabView::OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    return 1; // Prevent flicker
}

LRESULT CHelpTabView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (m_webView)
    {
        m_webView->remove_NavigationCompleted(m_navigationCompletedToken);
        m_webView->remove_NavigationStarting(m_navigationStartingToken);
        m_webView.Reset();
    }
    if (m_webViewController)
    {
        m_webViewController->Close();
        m_webViewController.Reset();
    }
    m_webViewEnvironment.Reset();
    return 0;
}

void CHelpTabView::SetVisible(bool visible)
{
    m_isVisible = visible;
    ShowWindow(visible ? SW_SHOW : SW_HIDE);
    if (m_webViewController)
    {
        m_webViewController->put_IsVisible(visible ? TRUE : FALSE);
    }
}

void CHelpTabView::Resize(const RECT& rc)
{
    SetWindowPos(NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        SWP_NOZORDER | SWP_NOACTIVATE);
}

void CHelpTabView::RefreshTheme()
{
    // Update toolbar theme if needed
    if (m_toolbar.IsWindow())
    {
        m_toolbar.InvalidateRect(NULL);
    }
}

void CHelpTabView::CreateNavigationToolbar()
{
    // Create toolbar with text buttons (more reliable than icons)
    m_toolbar.Create(m_hWnd, NULL, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
        TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST | CCS_NODIVIDER | CCS_NORESIZE,
        WS_EX_CONTROLPARENT);

    m_toolbar.SetExtendedStyle(TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_HIDECLIPPEDBUTTONS | TBSTYLE_EX_MIXEDBUTTONS);
    m_toolbar.SetButtonStructSize(sizeof(TBBUTTON));

    // Skip image list for now - use text-only buttons for reliability

    // Add text-only toolbar buttons (I_IMAGENONE = -2 for no image)
    TBBUTTON buttons[] = {
        { I_IMAGENONE, ID_HELP_NAV_BACK, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"◀ Back" },
        { I_IMAGENONE, ID_HELP_NAV_FORWARD, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"Forward ▶" },
        { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0}, 0, 0 },
        { I_IMAGENONE, ID_HELP_NAV_REFRESH, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"↻ Refresh" },
        { I_IMAGENONE, ID_HELP_NAV_HOME, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"⌂ Home" },
        { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0}, 0, 0 },
        { I_IMAGENONE, ID_HELP_NAV_FIND, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"🔍 Find" },
        { I_IMAGENONE, ID_HELP_NAV_PRINT, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"🖨 Print" },
        { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0}, 0, 0 },
        { I_IMAGENONE, ID_HELP_NAV_ZOOMIN, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"➕ Zoom In" },
        { I_IMAGENONE, ID_HELP_NAV_ZOOMOUT, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"➖ Zoom Out" },
        { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0}, 0, 0 },
        { I_IMAGENONE, ID_HELP_NAV_COPYURL, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"📋 Copy URL" },
        { I_IMAGENONE, ID_HELP_NAV_EXTERNAL, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"🌐 Browser" },
    };

    m_toolbar.AddButtons(_countof(buttons), buttons);
    m_toolbar.AutoSize();

    // Make sure toolbar is visible and positioned correctly
    CRect rcClient;
    GetClientRect(&rcClient);
    m_toolbar.SetWindowPos(NULL, 0, 0, rcClient.Width(), TOOLBAR_HEIGHT, SWP_NOZORDER | SWP_SHOWWINDOW);
    m_toolbar.ShowWindow(SW_SHOW);
}

HRESULT CHelpTabView::InitializeWebView()
{
    // Create WebView2 environment
    HRESULT hr = CreateCoreWebView2Environment(
        new EnvCompletedHandler(
            [this](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT
            {
                return OnCreateEnvironmentCompleted(result, environment);
            }));

    return hr;
}

HRESULT CHelpTabView::OnCreateEnvironmentCompleted(HRESULT result, ICoreWebView2Environment* environment)
{
    if (FAILED(result))
        return result;

    m_webViewEnvironment = environment;

    // Create WebView controller
    RECT bounds;
    GetClientRect(&bounds);
    bounds.top += TOOLBAR_HEIGHT;

    return m_webViewEnvironment->CreateCoreWebView2Controller(
        m_hWnd,
        new ControllerCompletedHandler(
            [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT
            {
                return OnCreateWebViewControllerCompleted(result, controller);
            }));
}

HRESULT CHelpTabView::OnCreateWebViewControllerCompleted(HRESULT result, ICoreWebView2Controller* controller)
{
    if (FAILED(result))
        return result;

    m_webViewController = controller;
    m_webViewController->get_CoreWebView2(&m_webView);

    // Set up event handlers
    m_webView->add_NavigationCompleted(
        new NavigationCompletedHandler(
            [this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT
            {
                return OnNavigationCompleted(sender, args);
            }),
        &m_navigationCompletedToken);

    m_webView->add_NavigationStarting(
        new NavigationStartingHandler(
            [this](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT
            {
                return OnNavigationStarting(sender, args);
            }),
        &m_navigationStartingToken);

    // Configure settings
    ICoreWebView2Settings* settings;
    m_webView->get_Settings(&settings);
    settings->put_IsScriptEnabled(TRUE);
    settings->put_AreDefaultScriptDialogsEnabled(TRUE);
    settings->put_IsWebMessageEnabled(FALSE);
    settings->put_AreDevToolsEnabled(FALSE);

    m_webViewReady = true;
    ResizeWebView();

    // Navigate to home URL if set
    if (!m_homeURL.empty())
    {
        NavigateToURL(m_homeURL);
    }

    return S_OK;
}

HRESULT CHelpTabView::OnNavigationCompleted(ICoreWebView2* /*sender*/, ICoreWebView2NavigationCompletedEventArgs* /*args*/)
{
    UpdateNavigationButtons();
    return S_OK;
}

HRESULT CHelpTabView::OnNavigationStarting(ICoreWebView2* /*sender*/, ICoreWebView2NavigationStartingEventArgs* args)
{
    // Get the URI being navigated to
    LPWSTR uri = nullptr;
    args->get_Uri(&uri);
    if (uri)
    {
        m_currentURL = uri;
        CoTaskMemFree(uri);
    }

    return S_OK;
}

void CHelpTabView::UpdateNavigationButtons()
{
    if (!m_webView || !m_toolbar.IsWindow())
        return;

    BOOL canGoBack = FALSE;
    BOOL canGoForward = FALSE;

    m_webView->get_CanGoBack(&canGoBack);
    m_webView->get_CanGoForward(&canGoForward);

    m_toolbar.EnableButton(ID_HELP_NAV_BACK, canGoBack);
    m_toolbar.EnableButton(ID_HELP_NAV_FORWARD, canGoForward);
}

void CHelpTabView::ResizeWebView()
{
    if (!m_webViewController)
        return;

    CRect rcClient;
    GetClientRect(&rcClient);

    // Adjust for toolbar
    if (m_toolbar.IsWindow())
    {
        m_toolbar.SetWindowPos(NULL, 0, 0, rcClient.Width(), TOOLBAR_HEIGHT, SWP_NOZORDER);
        rcClient.top += TOOLBAR_HEIGHT;
    }

    RECT bounds = rcClient;
    m_webViewController->put_Bounds(bounds);
}

bool CHelpTabView::LoadHTMLFile(const std::wstring& filePath)
{
    if (!m_webViewReady)
    {
        // Store for later when WebView is ready
        m_homeURL = L"file:///" + filePath;
        return false;
    }

    // Convert file path to file:// URL
    std::wstring url = L"file:///" + filePath;

    // Replace backslashes with forward slashes
    for (auto& ch : url)
    {
        if (ch == L'\\')
            ch = L'/';
    }

    m_homeURL = url;
    return NavigateToURL(url);
}

bool CHelpTabView::NavigateToURL(const std::wstring& url)
{
    if (!m_webView)
        return false;

    HRESULT hr = m_webView->Navigate(url.c_str());
    return SUCCEEDED(hr);
}

void CHelpTabView::NavigateBack()
{
    if (m_webView)
    {
        m_webView->GoBack();
    }
}

void CHelpTabView::NavigateForward()
{
    if (m_webView)
    {
        m_webView->GoForward();
    }
}

void CHelpTabView::Refresh()
{
    if (m_webView)
    {
        m_webView->Reload();
    }
}

void CHelpTabView::NavigateHome()
{
    if (!m_homeURL.empty())
    {
        NavigateToURL(m_homeURL);
    }
}

LRESULT CHelpTabView::OnNavigateBack(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    NavigateBack();
    return 0;
}

LRESULT CHelpTabView::OnNavigateForward(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    NavigateForward();
    return 0;
}

LRESULT CHelpTabView::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    Refresh();
    return 0;
}

LRESULT CHelpTabView::OnNavigateHome(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    NavigateHome();
    return 0;
}

// New functionality implementations
void CHelpTabView::Find()
{
    if (m_webViewController && m_webView)
    {
        // Ensure the WebView has focus first
        m_webViewController->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);

        // Small delay to ensure focus is set
        ::Sleep(50);

        // Use SendInput to simulate Ctrl+F keypress
        INPUT inputs[4] = {};

        // Press Ctrl
        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_CONTROL;
        inputs[0].ki.dwFlags = 0;

        // Press F
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = 'F';
        inputs[1].ki.dwFlags = 0;

        // Release F
        inputs[2].type = INPUT_KEYBOARD;
        inputs[2].ki.wVk = 'F';
        inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

        // Release Ctrl
        inputs[3].type = INPUT_KEYBOARD;
        inputs[3].ki.wVk = VK_CONTROL;
        inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

        // Send the input
        ::SendInput(4, inputs, sizeof(INPUT));
    }
}

void CHelpTabView::Print()
{
    if (m_webView)
    {
        // Execute JavaScript to open the browser's print dialog
        m_webView->ExecuteScript(L"window.print()", nullptr);
    }
}

void CHelpTabView::ZoomIn()
{
    if (m_webViewController)
    {
        double currentZoom = GetZoomFactor();
        SetZoomFactor(currentZoom * 1.1); // Increase by 10%
    }
}

void CHelpTabView::ZoomOut()
{
    if (m_webViewController)
    {
        double currentZoom = GetZoomFactor();
        SetZoomFactor(currentZoom * 0.9); // Decrease by 10%
    }
}

void CHelpTabView::ZoomReset()
{
    if (m_webViewController)
    {
        SetZoomFactor(1.0); // Reset to 100%
    }
}

double CHelpTabView::GetZoomFactor()
{
    if (m_webViewController)
    {
        double zoomFactor = 1.0;
        m_webViewController->get_ZoomFactor(&zoomFactor);
        return zoomFactor;
    }
    return m_currentZoomFactor;
}

void CHelpTabView::SetZoomFactor(double factor)
{
    if (m_webViewController)
    {
        // Clamp zoom factor between 25% and 500%
        if (factor < 0.25) factor = 0.25;
        if (factor > 5.0) factor = 5.0;

        m_webViewController->put_ZoomFactor(factor);
        m_currentZoomFactor = factor;
    }
}

void CHelpTabView::CopyURL()
{
    if (m_webView && !m_currentURL.empty())
    {
        // Open clipboard
        if (::OpenClipboard(m_hWnd))
        {
            ::EmptyClipboard();

            // Allocate global memory for the URL
            size_t size = (m_currentURL.length() + 1) * sizeof(wchar_t);
            HGLOBAL hClipboardData = GlobalAlloc(GMEM_MOVEABLE, size);
            if (hClipboardData)
            {
                wchar_t* pchData = (wchar_t*)GlobalLock(hClipboardData);
                if (pchData)
                {
                    wcscpy_s(pchData, m_currentURL.length() + 1, m_currentURL.c_str());
                    GlobalUnlock(hClipboardData);
                    ::SetClipboardData(CF_UNICODETEXT, hClipboardData);
                }
            }
            ::CloseClipboard();
        }
    }
}

void CHelpTabView::OpenExternal()
{
    if (!m_currentURL.empty())
    {
        // Open the current URL in the default web browser
        ShellExecute(NULL, L"open", m_currentURL.c_str(), NULL, NULL, SW_SHOW);
    }
}

// Command handlers for new buttons
LRESULT CHelpTabView::OnFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    Find();
    return 0;
}

LRESULT CHelpTabView::OnPrint(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    Print();
    return 0;
}

LRESULT CHelpTabView::OnZoomIn(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ZoomIn();
    return 0;
}

LRESULT CHelpTabView::OnZoomOut(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ZoomOut();
    return 0;
}

LRESULT CHelpTabView::OnZoomReset(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ZoomReset();
    return 0;
}

LRESULT CHelpTabView::OnCopyURL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CopyURL();
    return 0;
}

LRESULT CHelpTabView::OnOpenExternal(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    OpenExternal();
    return 0;
}

// Keyboard shortcuts handler
LRESULT CHelpTabView::OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool altPressed = (GetKeyState(VK_MENU) & 0x8000) != 0;

    if (ctrlPressed)
    {
        switch (wParam)
        {
            case 'F':  // Ctrl+F - Find
                Find();
                return 0;
            case 'P':  // Ctrl+P - Print
                Print();
                return 0;
            case VK_OEM_PLUS:  // Ctrl+Plus - Zoom In
            case VK_ADD:       // Ctrl+NumPad Plus
                ZoomIn();
                return 0;
            case VK_OEM_MINUS: // Ctrl+Minus - Zoom Out
            case VK_SUBTRACT:  // Ctrl+NumPad Minus
                ZoomOut();
                return 0;
            case '0':  // Ctrl+0 - Reset Zoom
            case VK_NUMPAD0:
                ZoomReset();
                return 0;
        }
    }
    else if (altPressed)
    {
        switch (wParam)
        {
            case VK_LEFT:  // Alt+Left - Navigate Back
                NavigateBack();
                return 0;
            case VK_RIGHT: // Alt+Right - Navigate Forward
                NavigateForward();
                return 0;
        }
    }
    else
    {
        switch (wParam)
        {
            case VK_F5:  // F5 - Refresh
                Refresh();
                return 0;
        }
    }

    bHandled = FALSE;
    return 0;
}

} // namespace debugviewpp
} // namespace fusion