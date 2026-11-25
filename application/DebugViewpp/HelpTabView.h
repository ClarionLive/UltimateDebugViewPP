#pragma once

#include "atleverything.h"
#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlctrls.h>
#include <atlmisc.h>
#include <memory>
#include <string>
#include <functional>
#include <wrl.h>
#include <wrl/client.h>
#include "WebView2.h"
#include "CLogViewTabItem2.h"

namespace fusion {
namespace debugviewpp {

class CHelpTabView : public CWindowImpl<CHelpTabView>, public IDetailTabView
{
public:
    DECLARE_WND_CLASS(_T("HelpTabView"))

    CHelpTabView(const std::wstring& helpTitle = L"Help");
    virtual ~CHelpTabView();

    BEGIN_MSG_MAP(CHelpTabView)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        COMMAND_ID_HANDLER(ID_HELP_NAV_BACK, OnNavigateBack)
        COMMAND_ID_HANDLER(ID_HELP_NAV_FORWARD, OnNavigateForward)
        COMMAND_ID_HANDLER(ID_HELP_NAV_REFRESH, OnRefresh)
        COMMAND_ID_HANDLER(ID_HELP_NAV_HOME, OnNavigateHome)
        COMMAND_ID_HANDLER(ID_HELP_NAV_FIND, OnFind)
        COMMAND_ID_HANDLER(ID_HELP_NAV_PRINT, OnPrint)
        COMMAND_ID_HANDLER(ID_HELP_NAV_ZOOMIN, OnZoomIn)
        COMMAND_ID_HANDLER(ID_HELP_NAV_ZOOMOUT, OnZoomOut)
        COMMAND_ID_HANDLER(ID_HELP_NAV_ZOOMRESET, OnZoomReset)
        COMMAND_ID_HANDLER(ID_HELP_NAV_COPYURL, OnCopyURL)
        COMMAND_ID_HANDLER(ID_HELP_NAV_EXTERNAL, OnOpenExternal)
    END_MSG_MAP()

    // IDetailTabView interface implementation
    virtual void Create(HWND parent, const RECT& rc) override;
    virtual HWND GetHWND() override { return m_hWnd; }
    virtual void SetVisible(bool visible) override;
    virtual void Resize(const RECT& rc) override;
    virtual void RefreshTheme() override;
    virtual std::wstring GetTabName() const override { return m_helpTitle; }

    // Help-specific methods
    bool LoadHTMLFile(const std::wstring& filePath);
    bool NavigateToURL(const std::wstring& url);
    void NavigateBack();
    void NavigateForward();
    void Refresh();
    void NavigateHome();
    void SetHomeURL(const std::wstring& url) { m_homeURL = url; }

    // New functionality
    void Find();
    void Print();
    void ZoomIn();
    void ZoomOut();
    void ZoomReset();
    void CopyURL();
    void OpenExternal();
    double GetZoomFactor();
    void SetZoomFactor(double factor);

protected:
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNavigateBack(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnNavigateForward(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnRefresh(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnNavigateHome(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFind(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPrint(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnZoomIn(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnZoomOut(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnZoomReset(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCopyURL(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnOpenExternal(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:
    // WebView2 components
    Microsoft::WRL::ComPtr<ICoreWebView2Environment> m_webViewEnvironment;
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_webViewController;
    Microsoft::WRL::ComPtr<ICoreWebView2> m_webView;

    // Navigation toolbar
    CToolBarCtrl m_toolbar;
    CImageList m_imageList;

    // State
    std::wstring m_helpTitle;
    std::wstring m_homeURL;
    std::wstring m_currentURL;
    bool m_webViewReady;
    bool m_isVisible;
    double m_currentZoomFactor;

    // Helper methods
    HRESULT InitializeWebView();
    void ResizeWebView();
    void CreateNavigationToolbar();
    void UpdateNavigationButtons();

    // WebView2 callbacks
    HRESULT OnCreateEnvironmentCompleted(HRESULT result, ICoreWebView2Environment* environment);
    HRESULT OnCreateWebViewControllerCompleted(HRESULT result, ICoreWebView2Controller* controller);
    HRESULT OnNavigationCompleted(ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args);
    HRESULT OnNavigationStarting(ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args);

    // Event tokens
    EventRegistrationToken m_navigationCompletedToken;
    EventRegistrationToken m_navigationStartingToken;

    static const int TOOLBAR_HEIGHT = 28;
    static const int TOOLBAR_BUTTON_SIZE = 24;
};

} // namespace debugviewpp
} // namespace fusion