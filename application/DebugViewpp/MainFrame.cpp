// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <filesystem>
#include <optional>
#include <iostream>
#include <format>
#include <cstdio>
#include <functional>
#include <iomanip>
#include <sstream>

#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "CobaltFusion/AtlWinExt.h"
#include "CobaltFusion/scope_guard.h"
#include "CobaltFusion/stringbuilder.h"
#include "CobaltFusion/dbgstream.h"
#include "CobaltFusion/hstream.h"
#include "CobaltFusion/Math.h"
#include "CobaltFusion/GuiExecutor.h"
#include "CobaltFusion/fusionassert.h"
#include "Win32/Registry.h"
#include "DebugViewppLib/ProcessReader.h"
#include "DebugViewppLib/DbgviewReader.h"
#include "DebugViewppLib/SocketReader.h"
#include "DebugViewppLib/FileReader.h"
#include "DebugViewppLib/FileIO.h"
#include "DebugViewppLib/LogFilter.h"
#include "DebugViewppLib/Conversions.h"
#include "DebugViewppLib/ThemeManager.h"
#include "DebugViewppLib/Colors.h"
#include "DebugViewppLib/ClarionQueueManager.h"

#include "resource.h"
#include "RunDlg.h"
#include "HistoryDlg.h"
#include "FilterDlg.h"
#include "SourcesDlg.h"
#include "AboutDlg.h"
#include "FileOptionDlg.h"
#include "LogView.h"
#include "MainFrame.h"
#include "CLogViewTabItem2.h"
#include "HelpTabView.h"
#include "ProcedureStatsSplitter.h"
#include "NewTabItem.h"
#include "McpServer.h"
#include "McpTools.h"
#include "version.h"

namespace fusion {
namespace debugviewpp {

// Registry paths
const wchar_t* RegistryPath = L"Software\\ClarionLive\\UltimateDebugView++";
const wchar_t* OldRegistryPath = L"Software\\Cobalt Fusion\\DebugView++";
const wchar_t* OldRegistryPath2 = L"Software\\CobaltFusion\\DebugView++";  // Alternative path without space

using namespace std::chrono_literals;

std::wstring GetPersonalPath()
{
    std::wstring path;
    wchar_t szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_PERSONAL, nullptr, 0, szPath)))
    {
        path = szPath;
    }
    return path;
}

std::wstring FormatUnits(int n, const std::wstring& unit)
{
    if (n == 0)
    {
        return L"";
    }
    if (n == 1)
    {
        return wstringbuilder() << n << " " << unit;
    }
    return wstringbuilder() << n << " " << unit << "s";
}

std::wstring FormatDuration(double seconds)
{
    int minutes = FloorTo<int>(seconds / 60);
    seconds -= 60.0 * minutes;

    int hours = minutes / 60;
    minutes -= 60 * hours;

    int days = hours / 24;
    hours -= 24 * days;

    if (days > 0)
    {
        return wstringbuilder() << FormatUnits(days, L"day") << L" " << FormatUnits(hours, L"hour");
    }

    if (hours > 0)
    {
        return wstringbuilder() << FormatUnits(hours, L"hour") << L" " << FormatUnits(minutes, L"minute");
    }

    if (minutes > 0)
    {
        return wstringbuilder() << FormatUnits(minutes, L"minute") << L" " << FormatUnits(FloorTo<int>(seconds), L"second");
    }

    static const wchar_t* units[] = {L"s", L"ms", L"µs", L"ns", nullptr};
    const wchar_t** unit = units;
    while (*unit != nullptr && seconds > 0 && seconds < 1)
    {
        seconds *= 1e3;
        ++unit;
    }

    return wstringbuilder() << std::fixed << std::setprecision(3) << seconds << L" " << *unit;
}

BEGIN_MSG_MAP2(CMainFrame)
    MSG_WM_CREATE(OnCreate)
    MSG_WM_CLOSE(OnClose)
    MSG_WM_SIZE(OnSize)
    MSG_WM_MOVE(OnMove)
    MSG_WM_QUERYENDSESSION(OnQueryEndSession)
    MSG_WM_ENDSESSION(OnEndSession)
    MSG_WM_MOUSEWHEEL(OnMouseWheel)
    MSG_WM_TIMER(OnTimer)
    MSG_WM_SETCURSOR(OnSetCursor)
    MSG_WM_CONTEXTMENU(OnContextMenu)
    MSG_WM_SYSCOMMAND(OnSysCommand)
    MESSAGE_HANDLER_EX(WM_SYSTEMTRAYICON, OnSystemTrayIcon)
    MESSAGE_HANDLER_EX(WM_MCP_CALL, OnMcpCall)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, OnCustomDraw)
    COMMAND_ID_HANDLER_EX(SC_RESTORE, OnScRestore)
    COMMAND_ID_HANDLER_EX(SC_CLOSE, OnScClose)
    COMMAND_ID_HANDLER_EX(ID_FILE_NEWVIEW, OnFileNewTab)
    COMMAND_ID_HANDLER_EX(ID_FILE_OPEN, OnFileOpen)
    COMMAND_ID_HANDLER_EX(ID_FILE_RUN, OnFileRun)
    COMMAND_ID_HANDLER_EX(ID_FILE_SAVE_LOG, OnFileSaveLog)
    COMMAND_ID_HANDLER_EX(ID_APP_EXIT, OnFileExit)
    COMMAND_ID_HANDLER_EX(ID_FILE_SAVE_VIEW, OnFileSaveView)
    COMMAND_ID_HANDLER_EX(ID_FILE_SAVE_VIEW_SELECTION, OnFileSaveViewSelection)
    COMMAND_ID_HANDLER_EX(ID_FILE_LOAD_CONFIGURATION, OnFileLoadConfiguration)
    COMMAND_ID_HANDLER_EX(ID_FILE_SAVE_CONFIGURATION, OnFileSaveConfiguration)
    COMMAND_ID_HANDLER_EX(ID_FILE_PREFERENCES, OnFilePreferences)
    COMMAND_ID_HANDLER_EX(ID_LOG_CLEAR, OnLogClear)
    COMMAND_ID_HANDLER_EX(ID_LOG_CROP, OnLogCrop)
    COMMAND_ID_HANDLER_EX(ID_LOG_PAUSE, OnLogPause)
    COMMAND_ID_HANDLER_EX(ID_LOG_GLOBAL, OnLogGlobal)
    COMMAND_ID_HANDLER_EX(ID_LOG_KERNEL, OnLogKernel)
    COMMAND_ID_HANDLER_EX(ID_LOG_KERNEL_VERBOSE, OnLogKernelVerbose)
    COMMAND_ID_HANDLER_EX(ID_LOG_KERNEL_PASSTHROUGH, OnLogKernelPassThrough)
    COMMAND_ID_HANDLER_EX(ID_LOG_HISTORY, OnLogHistory)
    COMMAND_ID_HANDLER_EX(ID_LOG_DEBUGVIEW_AGENT, OnLogDebugviewAgent)
    COMMAND_ID_HANDLER_EX(ID_VIEW_FIND, OnViewFind)
    COMMAND_ID_HANDLER_EX(ID_VIEW_FILTER, OnViewFilter)
    COMMAND_ID_HANDLER_EX(ID_VIEW_DETAIL_PANE, OnViewDetailPane)
    COMMAND_ID_HANDLER_EX(ID_VIEW_PROCEDURE_TRACKER, OnViewProcedureTracker)
    COMMAND_ID_HANDLER_EX(ID_TAB_POPOUT, OnTabPopOut)
    COMMAND_ID_HANDLER_EX(ID_TAB_POPIN, OnTabPopIn)
    COMMAND_ID_HANDLER_EX(ID_MAIN_TAB_POPOUT, OnMainTabPopOut)
    COMMAND_ID_HANDLER_EX(ID_MAIN_TAB_POPIN, OnMainTabPopIn)
    MESSAGE_HANDLER(WM_FLOATING_MAIN_WINDOW_CLOSING, OnFloatingMainWindowClosing)
    MESSAGE_HANDLER(WM_USER_CREATE_PROCEDURE_VIEW, OnUserCreateProcedureView)
    MESSAGE_HANDLER(WM_USER_JUMP_TO_DEBUG_ROW, OnUserJumpToDebugRow)
    COMMAND_ID_HANDLER_EX(ID_VIEW_CLOSE, OnViewClose)
    COMMAND_ID_HANDLER_EX(ID_VIEW_DUPLICATE, OnViewDuplicate)
    COMMAND_ID_HANDLER_EX(ID_VIEW_DARKMODE, OnViewDarkMode)
    COMMAND_ID_HANDLER_EX(ID_LOG_SOURCES, OnSources)
    COMMAND_ID_HANDLER_EX(ID_OPTIONS_LINKVIEWS, OnLinkViews)
    COMMAND_ID_HANDLER_EX(ID_OPTIONS_AUTONEWLINE, OnAutoNewline)
    COMMAND_ID_HANDLER_EX(ID_OPTIONS_PROCESS_PREFIX, OnProcessPrefix)
    COMMAND_ID_HANDLER_EX(ID_OPTIONS_FONT, OnViewFont)
    COMMAND_ID_HANDLER_EX(ID_OPTIONS_ALWAYSONTOP, OnAlwaysOnTop)
    COMMAND_ID_HANDLER_EX(ID_OPTIONS_HIDE, OnHide)
    COMMAND_ID_HANDLER_EX(ID_OPTIONS_EXPANDED_TOOLBAR, OnExpandedToolbar)
    COMMAND_ID_HANDLER_EX(ID_OPTIONS_IMPORT_SETTINGS, OnImportSettings)
    COMMAND_ID_HANDLER_EX(ID_APP_ABOUT, OnAppAbout)
    NOTIFY_CODE_HANDLER_EX(CTCN_BEGINITEMDRAG, OnBeginTabDrag)
    NOTIFY_CODE_HANDLER_EX(CTCN_SELCHANGE, OnChangeTab)
    NOTIFY_CODE_HANDLER_EX(CTCN_CLOSE, OnCloseTab)
    NOTIFY_CODE_HANDLER_EX(CTCN_DELETEITEM, OnDeleteTab)

    if (uMsg == EM_REPLACESEL)
    {
        m_logSources.AddMessage(static_cast<DWORD>(wParam), "SendMessage", Str(reinterpret_cast<wchar_t*>(lParam)));
    }

    // forward WM_COMMANDs directly to the active LogView, otherwise TabbedFrame forwards WM_COMMANDs to the Splitter
    // which has FORWARD_NOTIFICATIONS() and sends it back the the CMainFrame causing an infinite loop
    if (uMsg == WM_COMMAND)
    {
        if (GetViewCount() > 0)
        {
            GetView().SendMessage(uMsg, wParam, lParam);
        }
        return TRUE;
    }
    COMMAND_ID_HANDLER_EX(30012, OnProcedureSplitterMoved)  // ID_PROCEDURE_SPLITTER_MOVED
    CHAIN_MSG_MAP(TabbedFrame)
    CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
    REFLECT_NOTIFICATIONS()
END_MSG_MAP()

LOGFONT& GetDefaultLogFont()
{
    static LOGFONT lf;
    static int initialized = GetObjectW(AtlGetDefaultGuiFont(), sizeof(lf), &lf);
    return lf;
}

CMainFrame::CMainFrame() :
    m_findDlg(*this),
    m_tryGlobal(IsWindowsVistaOrGreater() && HasGlobalDBWinReaderRights()),
    m_logFileName(L"DebugView++.dblog"),
    m_txtFileName(L"Messages.dblog"),
    m_configFileName(L"DebugView++.dbconf"),
    m_initialPrivateBytes(ProcessInfo::GetPrivateBytes()),
    m_logfont(GetDefaultLogFont()),
    m_GuiExecutorClient(std::make_unique<GuiExecutorClient>()),
    m_logSources(*m_GuiExecutorClient)
{
    m_notifyIconData.cbSize = 0;
}

CMainFrame::~CMainFrame()
{
    StopMcpServer();
    RemoveDriver();
}

void CMainFrame::StartMcpServer()
{
    StopMcpServer();
    m_mcpError.clear();
    if (!m_mcpSettings.enabled)
    {
        return;
    }

    // The server never runs without a token; create one the first time it is needed.
    if (m_mcpSettings.token.size() < McpMinTokenLength)
    {
        m_mcpSettings.token = GenerateMcpToken();
        CRegKey reg;
        if (reg.Create(HKEY_CURRENT_USER, RegistryPath) == ERROR_SUCCESS)
        {
            reg.SetStringValue(L"McpToken", m_mcpSettings.token.c_str());
        }
    }

    if (!m_mcpTools)
    {
        m_mcpTools = std::make_unique<McpToolRegistry>();
        RegisterDebugViewTools(*m_mcpTools, *this);
    }
    if (!m_mcpServer)
    {
        m_mcpServer = std::make_unique<McpServer>(*m_mcpTools);
    }

    McpServerConfig config;
    config.port = static_cast<unsigned short>(m_mcpSettings.port);
    config.token = WideToUtf8(m_mcpSettings.token);
    config.serverVersion = VERSION_STR;
    HWND hwnd = m_hWnd;
    config.postToUi = [hwnd](std::function<void()> fn) {
        auto pFn = new std::function<void()>(std::move(fn));
        if (::PostMessage(hwnd, WM_MCP_CALL, 0, reinterpret_cast<LPARAM>(pFn)) == FALSE)
        {
            delete pFn;
            return false;
        }
        return true;
    };

    std::string error;
    if (!m_mcpServer->Start(config, error))
    {
        m_mcpError = Utf8ToWide(error);
    }
}

void CMainFrame::StopMcpServer()
{
    if (m_mcpServer)
    {
        m_mcpServer->Stop();
    }
}

LRESULT CMainFrame::OnMcpCall(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
{
    std::unique_ptr<std::function<void()>> pFn(reinterpret_cast<std::function<void()>*>(lParam));
    if (!pFn || !*pFn)
    {
        return 0;
    }

    // A modal dialog (Filter, Preferences, a message box) disables the frame and may hold pointers
    // into the log and views. Posted calls still arrive through its message loop, so hold them until
    // the frame is enabled again. A call still held after 15 s is cancelled by the MCP server and
    // then does nothing when it finally runs.
    if (!IsWindowEnabled() || !m_deferredMcpCalls.empty() || m_runningDeferredMcpCalls)
    {
        m_deferredMcpCalls.push_back(std::move(*pFn));
        ::SetTimer(m_hWnd, McpDeferTimerId, 100, nullptr);
        return 0;
    }

    (*pFn)();  // exceptions are caught inside and handed back to the waiting MCP request
    return 0;
}

void CMainFrame::RunDeferredMcpCalls()
{
    if (m_runningDeferredMcpCalls)
    {
        return;  // re-entered from a nested message loop; the outer loop continues
    }
    m_runningDeferredMcpCalls = true;
    // Take calls one at a time from the front and re-check before each: a call may open a dialog
    // (and pump messages), and calls posted meanwhile queue behind the ones already waiting.
    while (!m_deferredMcpCalls.empty() && IsWindowEnabled())
    {
        auto call = std::move(m_deferredMcpCalls.front());
        m_deferredMcpCalls.erase(m_deferredMcpCalls.begin());
        call();
    }
    m_runningDeferredMcpCalls = false;
    if (m_deferredMcpCalls.empty())
    {
        ::KillTimer(m_hWnd, McpDeferTimerId);
    }
}

void CMainFrame::ApplyMcpSettings(const McpSettings& settings)
{
    m_mcpSettings = settings;
    m_mcpSettings.port = std::clamp(m_mcpSettings.port, 1, 65535);
    StartMcpServer();
    SaveSettings();
    UpdateStatusBar();
}

std::wstring CMainFrame::GetMcpStatusText() const
{
    if (!m_mcpSettings.enabled)
    {
        return L"";
    }
    if (m_mcpServer && m_mcpServer->IsRunning())
    {
        return wstringbuilder() << L"MCP :" << m_mcpServer->Port();
    }
    return wstringbuilder() << L"MCP off: " << (m_mcpError.empty() ? L"not started" : m_mcpError);
}

void CMainFrame::SetLogging()
{
    m_logWriter = std::make_unique<FileWriter>(L"debugview.dblog", m_logFile);
}

void CMainFrame::OnException()
{
    errormessage("No additional information available", "Exception occurred");
}

void CMainFrame::OnException(const std::exception& ex)
{
    errormessage(ex.what(), "Exception occurred");
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
    // Intercept left button down on the tab control to handle "+" tab clicks specially
    if (pMsg->message == WM_LBUTTONDOWN && pMsg->hwnd == GetTabCtrl().m_hWnd)
    {
        POINT pt = { static_cast<LONG>(LOWORD(pMsg->lParam)), static_cast<LONG>(HIWORD(pMsg->lParam)) };
        CTCHITTESTINFO hti = { 0 };
        hti.pt = pt;
        int tabIndex = GetTabCtrl().HitTest(&hti);
        
        // Check if clicking on the "+" tab (last tab)
        if (tabIndex >= 0 && tabIndex >= GetActualViewCount())
        {
            // Handle the "+" tab click directly
            AddFilterView();
            
            // Reset cursor immediately
            ::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
            
            // Prevent further processing of this message
            return TRUE;
        }
    }
    
    return TabbedFrame::PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle()
{
    UpdateUI();
    UIUpdateToolBar();
    UIUpdateStatusBar();
    return FALSE;
}

LRESULT CMainFrame::OnCreate(const CREATESTRUCT* /*pCreate*/)
{
    m_notifyIconData.cbSize = 0;

    HWND hWndCmdBar = m_cmdBar.Create(*this, rcDefault, nullptr, ATL_SIMPLE_CMDBAR_PANE_STYLE);
    m_cmdBar.AttachMenu(GetMenu());
    m_cmdBar.LoadImages(IDR_MAINFRAME);
    SetMenu(nullptr); // disable second menu bar

    // Load the expanded toolbar setting early so we know which toolbar to create
    {
        CRegKey reg;
        if (reg.Open(HKEY_CURRENT_USER, RegistryPath, KEY_READ) == ERROR_SUCCESS)
        {
            DWORD expandedValue = 0;
            if (reg.QueryDWORDValue(L"ExpandedToolbar", expandedValue) == ERROR_SUCCESS)
            {
                m_expandedToolbar = (expandedValue != 0);
            }
        }
    }

    CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
    CReBarCtrl rebar(m_hWndToolBar);

    AddSimpleReBarBand(hWndCmdBar);

    // Don't create the standard toolbar here if we're going to immediately replace it
    // with the expanded toolbar. This will be handled in ApplyExpandedToolbarState()
    if (!m_expandedToolbar)
    {
        HWND hWndToolBar =
            CreateSimpleToolBarCtrl(rebar, IDR_MAINFRAME, 0, ATL_SIMPLE_TOOLBAR_PANE_STYLE); // DrMemory: LEAK 1696 direct bytes
        AddSimpleReBarBand(hWndToolBar, nullptr, 1);
        UIAddToolBar(hWndToolBar);
    }

    m_findDlg.Create(rebar);
    AddSimpleReBarBand(m_findDlg, L"Find: ", 0, 10000);
    SizeSimpleReBarBands();

    rebar.LockBands(true);
    rebar.SetNotifyWnd(*this);

    m_hWndStatusBar = m_statusBar.Create(*this);
    int paneIds[] = {ID_DEFAULT_PANE, ID_SELECTION_PANE, ID_VIEW_PANE, ID_LOGFILE_PANE, ID_MEMORY_PANE};
    m_statusBar.SetPanes(paneIds, 5, false);
    UIAddStatusBar(m_hWndStatusBar);

    // Create tab window (standard way)
    CreateTabWindow(*this, rcDefault, CTCS_DRAGREARRANGE | CTCS_CLOSEBUTTON);
    // Don't add a default view here - LoadSettings will handle it
    // Don't hide tab control - we want tabs visible even with single view for the "+" tab
    
    // Create procedure view as a child window that we'll dock to the side
    m_wndProcedureView.Create(*this, CWindow::rcDefault, _T("Procedure Tracker"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER,
        WS_EX_CLIENTEDGE);
    
    // Initially hide the procedure window
    m_wndProcedureView.ShowWindow(SW_HIDE);

    SetLogFont();
    LoadSettings();
    
    // Load and apply theme
    auto& themeManager = ThemeManager::Instance();
    themeManager.LoadFromRegistry();
    Colors::UpdateThemeColors();
    themeManager.ApplyToWindow(m_hWnd);
    UISetCheck(ID_VIEW_DARKMODE, themeManager.IsDarkTheme());
    
    // Apply dark mode to toolbar and status bar if needed
    if (themeManager.IsDarkTheme())
    {
        // Apply dark theme to status bar
        if (m_hWndStatusBar)
        {
            themeManager.ApplyToWindow(m_hWndStatusBar);
            ::SendMessage(m_hWndStatusBar, SB_SETBKCOLOR, 0, RGB(45, 45, 45));
            ::InvalidateRect(m_hWndStatusBar, nullptr, TRUE);
        }
        
        // Note: We're NOT applying dark theme to the rebar/toolbar because
        // the "Find:" label text becomes invisible in dark mode due to Windows limitations.
        // The rebar will remain in light mode for better usability.
        
        // Find dialog no longer needs custom theme handling
    }
    
    // Apply expanded toolbar state if it was previously enabled
    if (m_expandedToolbar)
    {
        ApplyExpandedToolbarState();
    }
    
    // Apply word wrap state and detail font to the detail view if it exists
    // Views are created in LoadSettings, so apply settings to all of them
    for (int i = 0; i < GetActualViewCount(); ++i)
    {
        auto pTabItem = GetTabCtrl().GetItem(i);
        if (pTabItem)
        {
            pTabItem->SetMainFrame(this);  // This will re-apply the word wrap state
            if (m_hDetailFont)
            {
                pTabItem->SetDetailFont(m_hDetailFont.get());
            }
        }
    }

    // Note: AddPlusTab is already called in LoadSettings, no need to call it again here

    // register object for message filtering and idle updates
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != nullptr);
    pLoop->AddMessageFilter(this);
    pLoop->AddIdleHandler(this);

    m_logSources.SubscribeToUpdate([this] { return OnUpdate(); });

    // Resume can throw if a second debugview is running
    // so do not rely on any commands executed afterwards
    Resume();

    // Initialization is complete, allow AddFilterView to be called from "+" tab clicks
    m_bInitializing = false;

    StartMcpServer();

    return 0;
}

void CMainFrame::OnClose()
{
    StopMcpServer();
    SaveSettings();
    DestroyWindow();

    if (m_notifyIconData.cbSize != 0u)
    {
        Shell_NotifyIcon(NIM_DELETE, &m_notifyIconData);
        m_notifyIconData.cbSize = 0;
    }
}

void CMainFrame::OnSize(UINT nType, CSize /*size*/)
{
    // Let the framework handle the base sizing
    SetMsgHandled(FALSE);
    
    // If the procedure tracker is visible, reposition it
    if (m_bProcedurePaneVisible && m_wndProcedureView.IsWindow() && m_wndProcedureView.IsWindowVisible())
    {
        // Get the window area in screen coordinates
        CRect windowRect;
        GetWindowRect(&windowRect);
        
        // Calculate position for procedure window (25% width on the right)
        int procedureWidth = windowRect.Width() / 4;  // 25% width
        CRect procedureRect;
        procedureRect.left = windowRect.right - procedureWidth;
        procedureRect.top = windowRect.top;
        procedureRect.right = windowRect.right;
        procedureRect.bottom = windowRect.bottom;
        
        // Account for window frame
        AdjustWindowRectEx(&procedureRect, WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX, FALSE, WS_EX_TOOLWINDOW);
        
        // Reposition the procedure window
        m_wndProcedureView.SetWindowPos(NULL, procedureRect.left, procedureRect.top,
                                        procedureRect.Width(), procedureRect.Height(),
                                        SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void CMainFrame::OnMove(CPoint /*pt*/)
{
    // If the procedure tracker is visible, move it with the main window
    if (m_bProcedurePaneVisible && m_wndProcedureView.IsWindow() && m_wndProcedureView.IsWindowVisible())
    {
        // Get the window area in screen coordinates
        CRect windowRect;
        GetWindowRect(&windowRect);
        
        // Calculate position for procedure window (25% width on the right)
        int procedureWidth = windowRect.Width() / 4;  // 25% width
        CRect procedureRect;
        procedureRect.left = windowRect.right - procedureWidth;
        procedureRect.top = windowRect.top;
        procedureRect.right = windowRect.right;
        procedureRect.bottom = windowRect.bottom;
        
        // Account for window frame
        AdjustWindowRectEx(&procedureRect, WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX, FALSE, WS_EX_TOOLWINDOW);
        
        // Move the procedure window
        m_wndProcedureView.SetWindowPos(NULL, procedureRect.left, procedureRect.top,
                                        0, 0,
                                        SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
    }
}

LRESULT CMainFrame::OnQueryEndSession(WPARAM /*unused*/, LPARAM /*unused*/)
{
    // MSDN:
    // The WM_QUERYENDSESSION message is sent when the user chooses to end the session or when an application calls one of the system
    // shutdown functions When an application returns TRUE for this message, it receives the WM_ENDSESSION message. Each application should
    // return TRUE or FALSE immediately upon receiving this message, and defer any cleanup operations until it receives the WM_ENDSESSION
    // message.
    return TRUE;
}

LRESULT CMainFrame::OnEndSession(WPARAM /*unused*/, LPARAM /*unused*/)
{
    OnClose();
    return TRUE;
}

void CMainFrame::UpdateUI()
{
    UpdateStatusBar();

    UISetCheck(ID_VIEW_TIME, GetView().GetClockTime());
    UISetCheck(ID_VIEW_PROCESSCOLORS, GetView().GetViewProcessColors());
    UISetCheck(ID_VIEW_DARKMODE, ThemeManager::Instance().IsDarkTheme());
    
    // Check if detail pane is visible for current tab
    bool detailPaneVisible = false;
    int tabIndex = std::max(0, GetTabCtrl().GetCurSel());
    if (tabIndex < GetTabCtrl().GetItemCount())
    {
        auto pTabItem = GetTabCtrl().GetItem(tabIndex);
        if (pTabItem)
        {
            detailPaneVisible = pTabItem->IsDetailPaneVisible();
        }
    }
    UISetCheck(ID_VIEW_DETAIL_PANE, detailPaneVisible);
    // Procedure tracking is now integrated into detail pane tabs, so show same state as detail pane
    UISetCheck(ID_VIEW_PROCEDURE_TRACKER, detailPaneVisible);
    
    UISetCheck(ID_VIEW_SCROLL, GetView().GetAutoScroll());
    UISetCheck(ID_VIEW_SCROLL_STOP, GetView().GetAutoScrollStop());
    UISetCheck(ID_VIEW_BOOKMARK, GetView().GetBookmark());

    for (int id = ID_VIEW_COLUMN_FIRST; id <= ID_VIEW_COLUMN_LAST; ++id)
    {
        UISetCheck(id, GetView().IsColumnViewed(id));
    }

    UISetCheck(ID_OPTIONS_LINKVIEWS, m_linkViews);
    UIEnable(ID_OPTIONS_LINKVIEWS, static_cast<BOOL>(GetTabCtrl().GetItemCount() > 1));
    UISetCheck(ID_OPTIONS_AUTONEWLINE, m_logSources.GetAutoNewLine());
    UISetCheck(ID_OPTIONS_PROCESS_PREFIX, m_logSources.GetProcessPrefix());
    UISetCheck(ID_OPTIONS_ALWAYSONTOP, GetAlwaysOnTop());
    UISetCheck(ID_OPTIONS_HIDE, m_hide);
    UISetCheck(ID_LOG_PAUSE, !m_pLocalReader);
    UIEnable(ID_LOG_GLOBAL, !!m_pLocalReader);
    UISetCheck(ID_LOG_GLOBAL, m_tryGlobal);
    UISetCheck(ID_LOG_KERNEL, m_tryKernel);
    UIEnable(ID_LOG_KERNEL_VERBOSE, !!m_pKernelReader);
    UIEnable(ID_LOG_KERNEL_PASSTHROUGH, !!m_pKernelReader);
    UISetCheck(ID_LOG_KERNEL_VERBOSE, m_verboseKernelMessage);
    UISetCheck(ID_LOG_KERNEL_PASSTHROUGH, m_passthroughMode);
}

std::wstring FormatDateTime(const SYSTEMTIME& systemTime)
{
    int size = GetTimeFormat(LOCALE_USER_DEFAULT, 0, &systemTime, nullptr, nullptr, 0);
    size += GetDateFormat(LOCALE_USER_DEFAULT, 0, &systemTime, nullptr, nullptr, 0);
    std::vector<wchar_t> buf(size);

    int offset = GetDateFormat(LOCALE_USER_DEFAULT, 0, &systemTime, nullptr, buf.data(), size);
    buf[offset - 1] = ' ';
    GetTimeFormat(LOCALE_USER_DEFAULT, 0, &systemTime, nullptr, buf.data() + offset, size);
    return std::wstring(buf.data(), size - 1);
}

std::wstring FormatDateTime(const FILETIME& fileTime)
{
    return FormatDateTime(Win32::FileTimeToSystemTime(Win32::FileTimeToLocalFileTime(fileTime)));
}

std::wstring FormatBytes(size_t size)
{
    static const wchar_t* units[] = {L"bytes", L"kB", L"MB", L"GB", L"TB", L"PB", L"EB", nullptr};
    const wchar_t** unit = units;
    const int kb = 1024;
    while (size / kb > 0 && unit[1] != nullptr)
    {
        size = size / kb;
        ++unit;
    }

    return wstringbuilder() << size << L" " << *unit;
}

std::wstring CMainFrame::GetSelectionInfoText(const std::wstring& label, const SelectionInfo& selection) const
{
    if (selection.count == 0)
    {
        return std::wstring();
    }

    if (selection.count == 1)
    {
        return label + L": " + FormatDateTime(m_logFile[selection.beginLine].systemTime);
    }

    double dt = m_logFile[selection.endLine].time - m_logFile[selection.beginLine].time;
    return wstringbuilder() << label << L": " << FormatDuration(dt) << L" (" << selection.count << " lines)";
}

SelectionInfo CMainFrame::GetLogFileRange() const
{
    if (m_logFile.Empty())
    {
        return SelectionInfo();
    }

    return SelectionInfo(0, m_logFile.Count() - 1, m_logFile.Count());
}

void CMainFrame::UpdateStatusBar()
{
    auto isearch = GetView().GetHighlightText();
    std::wstring search = wstringbuilder() << L"Searching: \"" << isearch << L"\"";
    std::wstring state = m_pLocalReader != nullptr ? L"Ready" : L"Paused";
    auto mcpStatus = GetMcpStatusText();
    if (!mcpStatus.empty())
    {
        state += L"  |  " + mcpStatus;
    }
    UISetText(ID_DEFAULT_PANE, isearch.empty() ? state.c_str() : search.c_str());
    UISetText(ID_SELECTION_PANE, GetSelectionInfoText(L"Selected", GetView().GetSelectedRange()).c_str());
    UISetText(ID_VIEW_PANE, GetSelectionInfoText(L"View", GetView().GetViewRange()).c_str());
    UISetText(ID_LOGFILE_PANE, GetSelectionInfoText(L"Log", GetLogFileRange()).c_str());

    auto currentUsage = ProcessInfo::GetPrivateBytes();
    auto memoryUsage = currentUsage > m_initialPrivateBytes ? currentUsage - m_initialPrivateBytes : 0;
    UISetText(ID_MEMORY_PANE, FormatBytes(memoryUsage).c_str());
}

void CMainFrame::ProcessLines(const Lines& lines)
{
    if (lines.empty())
    {
        return;
    }

    // design decision: filtering is done on the UI thread, see CLogView::Add
    // changing this would introduces extra thread and thus complexity. Do that only if it solves a problem.

    int views = GetActualViewCount();
    for (int i = 0; i < views; ++i)
    {
        GetView(i).BeginUpdate();
    }

    if (m_logSources.GetProcessPrefix())
    {
        for (auto& line : lines)
        {
            AddMessage(Message(line.time, line.systemTime, line.pid, line.processName, "[" + std::to_string(line.pid) + "] " + line.message));
        }
    }
    else
    {
        for (auto& line : lines)
        {
            AddMessage(Message(line.time, line.systemTime, line.pid, line.processName, line.message));
        }
    }

    for (int i = 0; i < views; ++i)
    {
        if (GetView(i).EndUpdate() && GetTabCtrl().GetCurSel() != i)
        {
            SetModifiedMark(i, true);
            GetTabCtrl().UpdateLayout();
            GetTabCtrl().Invalidate();
        }
    }
    
    // Update floating procedure windows in real-time
    if (GetViewCount() > 0)
    {
        int currentTab = GetTabCtrl().GetCurSel();
        if (currentTab >= 0 && currentTab < GetViewCount())
        {
            auto pTabItem = dynamic_cast<CLogViewTabItem2*>(GetTabCtrl().GetItem(currentTab));
            if (pTabItem)
            {
                pTabItem->UpdateFloatingWindows();
            }
        }
    }
}

bool CMainFrame::OnUpdate()
{
    Lines bucket;
    auto lines = m_logSources.GetLines();
    int count = 0;
    for (auto&& line : lines)
    {
        if (count++ < 5000)
        {
            bucket.emplace_back(std::move(line));
        }
        else
        {
            count = 0;
            m_incomingMessages.emplace_back(std::move(bucket));
            bucket = Lines();
        }
    }
    if (!bucket.empty())
    {
        m_incomingMessages.emplace_back(std::move(bucket));
    }

    if (m_incomingMessages.empty())
    {
        return false;
    }


    auto linesbucket = std::move(m_incomingMessages.front());
    m_incomingMessages.pop_front();
    ProcessLines(linesbucket);
    if (!m_incomingMessages.empty())
    {
        m_GuiExecutorClient->CallAfter(20ms, [this] { OnUpdate(); });
    }

    return true;
}

bool CMainFrame::OnMouseWheel(UINT nFlags, short zDelta, CPoint /*pt*/)
{
    if ((nFlags & MK_CONTROL) == 0)
    {
        return false;
    }

    int size = static_cast<int>(LogFontSizeToPointSize(m_logfont.lfHeight) * std::pow(1.15, zDelta / WHEEL_DELTA) + 0.5);
    size = std::max(size, 4);
    size = std::min(size, 24);
    m_logfont.lfHeight = LogFontSizeFromPointSize(size);
    SetLogFont();
    return true;
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1001)
    {
        // Reset cursor after dialog closes
        ::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
        ::KillTimer(m_hWnd, 1001);
    }
    else if (nIDEvent == McpDeferTimerId)
    {
        RunDeferredMcpCalls();
    }
}

BOOL CMainFrame::OnSetCursor(CWindow wnd, UINT nHitTest, UINT /*message*/)
{
    // Always reset cursor to arrow if it's showing "no drop" cursor
    // This can happen when tab drag state gets stuck
    HCURSOR currentCursor = ::GetCursor();
    static HCURSOR hArrowCursor = ::LoadCursor(nullptr, IDC_ARROW);
    
    // Get system no-drop cursor (usually from ole32.dll)
    static HCURSOR hNoCursor = nullptr;
    static HCURSOR hSystemNoCursor = ::LoadCursor(nullptr, IDC_NO);
    if (!hNoCursor)
    {
        HMODULE hOle32 = ::GetModuleHandle(L"ole32.dll");
        if (hOle32)
        {
            hNoCursor = ::LoadCursor(hOle32, MAKEINTRESOURCE(1));  // No drop cursor from ole32
        }
    }
    
    // Check if current cursor is any "no" cursor variant
    if (currentCursor == hSystemNoCursor || 
        (hNoCursor && currentCursor == hNoCursor))
    {
        // Force reset to arrow cursor
        ::SetCursor(hArrowCursor);
        
        // Also force tab control to reset its state
        if (wnd.m_hWnd == GetTabCtrl().m_hWnd || nHitTest == HTCLIENT)
        {
            GetTabCtrl().SendMessage(WM_LBUTTONUP, 0, 0);
            ::ReleaseCapture();
        }
        
        return TRUE;  // We handled it
    }
    
    // Allow normal cursor processing
    SetMsgHandled(win32::False);
    return FALSE;
}

void CMainFrame::OnContextMenu(HWND hWnd, CPoint pt)
{
    if (hWnd != m_TabCtrl)
    {
        SetMsgHandled(win32::False);
        return;
    }

    CTCHITTESTINFO hit;
    hit.pt = pt;
    m_TabCtrl.ScreenToClient(&hit.pt);
    int item = m_TabCtrl.HitTest(&hit);

    CMenu menuContext;
    menuContext.LoadMenu(IDR_TAB_CONTEXTMENU);
    CMenuHandle menuPopup(menuContext.GetSubMenu(0));

    // Remove Pop Out Tab and Pop In Tab items - these are only for detail tabs
    menuPopup.DeleteMenu(ID_MAIN_TAB_POPOUT, MF_BYCOMMAND);
    menuPopup.DeleteMenu(ID_MAIN_TAB_POPIN, MF_BYCOMMAND);
    
    // Also remove the separator that was between them and the other items
    // Find and remove empty separators
    for (int i = menuPopup.GetMenuItemCount() - 1; i >= 0; i--)
    {
        UINT state = menuPopup.GetMenuState(i, MF_BYPOSITION);
        if (state == 0xFFFFFFFF) continue; // Invalid item
        
        // Check if it's a separator
        if (state & MF_SEPARATOR)
        {
            // Check if there's another separator right after it (double separator)
            if (i > 0)
            {
                UINT prevState = menuPopup.GetMenuState(i - 1, MF_BYPOSITION);
                if (prevState != 0xFFFFFFFF && (prevState & MF_SEPARATOR))
                {
                    menuPopup.DeleteMenu(i, MF_BYPOSITION);
                }
            }
        }
    }

    if (item < 0)
    {
        menuPopup.EnableMenuItem(ID_VIEW_FILTER, MF_BYCOMMAND | MF_GRAYED);
        menuPopup.EnableMenuItem(ID_VIEW_CLEAR, MF_BYCOMMAND | MF_GRAYED);
        menuPopup.EnableMenuItem(ID_VIEW_CLOSE, MF_BYCOMMAND | MF_GRAYED);
    }

    menuPopup.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, *this);
}

void CMainFrame::HandleDroppedFile(const std::wstring& file)
{
    SetTitle(file);
    using boost::algorithm::iequals;
    auto ext = std::filesystem::path(file).extension().wstring();
    if (iequals(ext, L".exe"))
    {
        Run(file);
    }
    else if (iequals(ext, L".cmd") || iequals(ext, L".bat"))
    {
        m_logSources.AddProcessReader(L"cmd.exe", wstringbuilder() << L"/Q /C \"" << file << "\"");
    }
    else
    {
        if (IsBinaryFileType(IdentifyFile(file)))
        {
            m_logSources.AddBinaryFileReader(file);
        }
        else
        {
            m_logSources.AddAnyFileReader(file, true);
        }
    }
}

void CMainFrame::OnDropped(const std::wstring uri)
{
    if (std::filesystem::is_regular_file(uri))
    {
        HandleDroppedFile(uri);
    }
    else
    {
        std::wstring httpmonitor = wstringbuilder() << Win32::GetExecutionPath() << "\\debugview++plugins\\HttpMonitor.exe";
        if (std::filesystem::exists(httpmonitor))
        {
            if (m_httpMonitorHandle)
            {
                ::TerminateProcess(m_httpMonitorHandle.get(), 0);
            }

            Win32::Process process(httpmonitor, uri);
            m_httpMonitorHandle = Win32::DuplicateHandle(process.GetProcessHandle());
            m_jobs.AddProcessByHandle(m_httpMonitorHandle.get());
        }
        else
        {
            m_logSources.AddMessage(stringbuilder() << httpmonitor << " missing, dropped url ignored\n");
        }
    }
}

LRESULT CMainFrame::OnSysCommand(UINT nCommand, CPoint /*unused*/)
{
    switch (nCommand)
    {
    case SC_MINIMIZE:
        if (!m_hide)
        {
            break;
        }

        if (m_notifyIconData.cbSize == 0u)
        {
            m_notifyIconData.cbSize = sizeof(m_notifyIconData);
            m_notifyIconData.hWnd = *this;
            m_notifyIconData.uID = 1;
            m_notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            m_notifyIconData.uCallbackMessage = WM_SYSTEMTRAYICON;
            m_notifyIconData.hIcon =
                AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
            CString sWindowText;
            GetWindowText(sWindowText);
            _tcscpy_s(m_notifyIconData.szTip, sWindowText);
            if (!Shell_NotifyIcon(NIM_ADD, &m_notifyIconData))
            {
                break;
            }
        }
        ShowWindow(SW_HIDE);
        return 0;
    default:
        break;
    }

    SetMsgHandled(win32::False);
    return 0;
}

LRESULT CMainFrame::OnSystemTrayIcon(UINT /*unused*/, WPARAM /*unused*/, LPARAM lParam)
{
    switch (lParam)
    {
    case WM_LBUTTONDBLCLK: SendMessage(WM_COMMAND, SC_RESTORE); break;
    case WM_RBUTTONUP: {
        SetForegroundWindow(m_hWnd);
        CMenuHandle menu = GetSystemMenu(0);
        menu.EnableMenuItem(SC_RESTORE, MF_BYCOMMAND | MF_ENABLED);
        menu.EnableMenuItem(SC_MOVE, MF_BYCOMMAND | MF_GRAYED);
        menu.EnableMenuItem(SC_SIZE, MF_BYCOMMAND | MF_GRAYED);
        menu.EnableMenuItem(SC_MINIMIZE, MF_BYCOMMAND | MF_GRAYED);
        menu.EnableMenuItem(SC_MAXIMIZE, MF_BYCOMMAND | MF_GRAYED);
        menu.EnableMenuItem(SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
        POINT position = Win32::GetCursorPos();
        menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN, position.x, position.y, m_hWnd);
        break;
    }
    default:
        break;
    }
    return 0;
}

LRESULT CMainFrame::OnScRestore(UINT /*unused*/, INT /*unused*/, HWND /*unused*/)
{
    if (m_notifyIconData.cbSize != 0u)
    {
        Shell_NotifyIcon(NIM_DELETE, &m_notifyIconData);
        m_notifyIconData.cbSize = 0;
    }
    ShowWindow(SW_SHOW);
    BringWindowToTop();
    return 0;
}

LRESULT CMainFrame::OnScClose(UINT /*unused*/, INT /*unused*/, HWND /*unused*/)
{
    PostMessage(WM_COMMAND, ID_APP_EXIT);
    return 0;
}

int CMainFrame::LogFontSizeFromPointSize(int fontSize)
{
    CDC dc(GetDC());
    return -MulDiv(fontSize, GetDeviceCaps(dc, LOGPIXELSY), 72);
}

int CMainFrame::LogFontSizeToPointSize(int logFontSize)
{
    CDC dc(GetDC());
    return -MulDiv(logFontSize, 72, GetDeviceCaps(dc, LOGPIXELSY));
}

bool CMainFrame::LoadSettings()
{
    auto mutex = Win32::CreateMutex(nullptr, false, L"Local\\DebugView++");
    Win32::MutexLock lock(mutex.get());

    // Check if this is the first run of Ultimate DebugView++
    CRegKey testReg;
    bool isFirstRun = false;
    if (testReg.Open(HKEY_CURRENT_USER, RegistryPath, KEY_READ) != ERROR_SUCCESS)
    {
        isFirstRun = true;
        testReg.Close();
        
        // Check if original DebugView++ settings exist
        CRegKey oldReg;
        bool foundOldSettings = false;
        
        if (oldReg.Open(HKEY_CURRENT_USER, OldRegistryPath, KEY_READ) == ERROR_SUCCESS ||
            oldReg.Open(HKEY_CURRENT_USER, OldRegistryPath2, KEY_READ) == ERROR_SUCCESS)
        {
            foundOldSettings = true;
            oldReg.Close();
        }
        
        if (foundOldSettings)
        {
            int result = MessageBox(
                L"This appears to be the first run of Ultimate DebugView++.\n\n"
                L"Would you like to import your settings from DebugView++?\n"
                L"This includes your views, filters, column layouts, and preferences.",
                L"Import Settings",
                MB_YESNO | MB_ICONQUESTION);
            
            if (result == IDYES)
            {
                ImportDebugViewPPSettings();
            }
        }
    }

    DWORD x = 0;
    DWORD y = 0;
    DWORD cx = 0;
    DWORD cy = 0;
    CRegKey reg;
    reg.Create(HKEY_CURRENT_USER, RegistryPath);
    if (reg.QueryDWORDValue(L"X", x) == ERROR_SUCCESS &&
        reg.QueryDWORDValue(L"Y", y) == ERROR_SUCCESS &&
        reg.QueryDWORDValue(L"Width", cx) == ERROR_SUCCESS &&
        reg.QueryDWORDValue(L"Height", cy) == ERROR_SUCCESS)
    {
        WINDOWPLACEMENT placement = {0};
        placement.length = sizeof(placement);

        placement.rcNormalPosition.left = x;
        placement.rcNormalPosition.top = y;
        placement.rcNormalPosition.right = x + cx;
        placement.rcNormalPosition.bottom = y + cy;

        if (reg.QueryDWORDValue(L"MaxX", x) == ERROR_SUCCESS &&
            reg.QueryDWORDValue(L"MaxY", y) == ERROR_SUCCESS)
        {
            placement.ptMaxPosition.x = x;
            placement.ptMaxPosition.y = y;
        }

        // This is tricky: we should _not_ use the show command corresponding
        // to the actual initial window state in WINDOWPLACEMENT because doing
        // this loses the normal geometry, i.e. if we pass SW_SHOWMAXIMIZED to
        // SetWindowPlacement(), the window appears maximized initially, but
        // restoring it doesn't restore its previous rectangle.
        //
        // Instead, we always use SW_SHOWNORMAL here, but use the appropriate
        // show command for the ShowWindow() call in Main(), as this ensures
        // both that the window starts initially maximized, if it was closed in
        // this state, and that it can be restored to its last normal position.
        DWORD on = FALSE;
        if (reg.QueryDWORDValue(L"Maximized", on) == ERROR_SUCCESS && on)
            m_showCmd = SW_SHOWMAXIMIZED;
        else if (reg.QueryDWORDValue(L"Minimized", on) == ERROR_SUCCESS && on)
            m_showCmd = SW_SHOWMINIMIZED;

        placement.showCmd = SW_SHOWNORMAL;

        // Ignore errors, the worst that can happen is that the window doesn't
        // appear at the correct position, but this is not the end of the world.
        ::SetWindowPlacement(*this, &placement);
    }

    m_linkViews = Win32::RegGetDWORDValue(reg, L"LinkViews", 0) != 0;
    m_logSources.SetAutoNewLine(Win32::RegGetDWORDValue(reg, L"AutoNewLine", 1) != 0);
    SetAlwaysOnTop(Win32::RegGetDWORDValue(reg, L"AlwaysOnTop", 0) != 0);

    m_applicationName = Win32::RegGetStringValue(reg, L"ApplicationName", L"Ultimate Debugview ++");
    SetTitle();

    m_hide = Win32::RegGetDWORDValue(reg, L"Hide", 0) != 0;
    m_expandedToolbar = Win32::RegGetDWORDValue(reg, L"ExpandedToolbar", 0) != 0;
    SetProcedureStatsSplitterPos(static_cast<int>(Win32::RegGetDWORDValue(reg, L"ProcedureStatsSplitterPos", m_procedureStatsSplitterPos)));

    auto fontName = Win32::RegGetStringValue(reg, L"FontName", L"").substr(0, LF_FACESIZE - 1);
    int fontSize = Win32::RegGetDWORDValue(reg, L"FontSize", 8);
    if (!fontName.empty())
    {
        LOGFONT lf = {0};
        m_logfont = lf;
        std::copy(fontName.begin(), fontName.end(), m_logfont.lfFaceName);
        m_logfont.lfHeight = LogFontSizeFromPointSize(fontSize);
        SetLogFont();
    }
    
    // Load detail pane font
    auto detailFontName = Win32::RegGetStringValue(reg, L"DetailFontName", L"Consolas").substr(0, LF_FACESIZE - 1);
    int detailFontSize = Win32::RegGetDWORDValue(reg, L"DetailFontSize", 9);
    {
        LOGFONT lf = {0};
        m_detailLogfont = lf;
        std::copy(detailFontName.begin(), detailFontName.end(), m_detailLogfont.lfFaceName);
        m_detailLogfont.lfHeight = LogFontSizeFromPointSize(detailFontSize);
        Win32::HFont hFont(CreateFontIndirect(&m_detailLogfont));
        if (hFont)
        {
            m_hDetailFont = std::move(hFont);
        }
    }

    CRegKey regViews;
    if (regViews.Open(reg, L"Views") == ERROR_SUCCESS)
    {
        // Get the saved count of views
        DWORD viewCount = Win32::RegGetDWORDValue(regViews, L"Count", 0);

        // If no count is saved (old format), count the views manually for backward compatibility
        if (viewCount == 0)
        {
            for (DWORD i = 0;; ++i)
            {
                CRegKey testView;
                if (testView.Open(regViews, WStr(wstringbuilder() << L"View" << i)) != ERROR_SUCCESS)
                {
                    viewCount = i;
                    break;
                }
                testView.Close();
            }
        }

        // Clear any existing views (none should exist since we don't create default in OnCreate)
        // But be defensive in case there are any
        while (GetActualViewCount() > 0)
        {
            // Remove views starting from the last one
            CloseView(GetActualViewCount() - 1);
        }
        
        // Load views in the exact order they were saved
        for (DWORD i = 0; i < viewCount; ++i)
        {
            CRegKey regView;
            if (regView.Open(regViews, WStr(wstringbuilder() << L"View" << i)) != ERROR_SUCCESS)
            {
                // If we can't find this view, stop loading
                // This handles the case where registry is corrupted
                break;
            }

            auto name = Win32::RegGetStringValue(regView);
            if (name.empty())
            {
                name = L"View";  // Default name if not found
            }

            // Always create new views since we cleared all existing ones
            AddFilterView(name, LogFilter());
            // Then load the actual filters and settings
            GetView(static_cast<int>(i)).LoadSettings(regView);
        }

        // If no views were loaded, create a default one
        if (GetActualViewCount() == 0)
        {
            AddFilterView(L"View", LogFilter());
        }
        
        // Load splitter positions and detail pane settings for all views
        for (int i = 0; i < GetActualViewCount(); ++i)
        {
            auto pTabItem = GetTabCtrl().GetItem(i);
            if (pTabItem)
            {
                CRegKey regView;
                regView.Open(regViews, WStr(wstringbuilder() << L"View" << i));
                
                if (regView)
                {
                    // Load detail pane visibility (default to false for new views)
                    bool detailPaneVisible = Win32::RegGetDWORDValue(regView, L"DetailPaneVisible", 0) != 0;
                    pTabItem->ShowDetailPane(detailPaneVisible);
                    
                    int splitterPos = Win32::RegGetDWORDValue(regView, L"DetailSplitterPos", 75);
                    // Ensure the value is within valid range (0-100)
                    if (splitterPos < 0) splitterPos = 0;
                    if (splitterPos > 100) splitterPos = 100;
                    pTabItem->SetSplitterPos(splitterPos);
                    
                    // Load per-view detail pane settings (all default to on for new views)
                    bool wordWrap = Win32::RegGetDWORDValue(regView, L"DetailWordWrap", 1) != 0;
                    bool syntaxHighlight = Win32::RegGetDWORDValue(regView, L"DetailSyntaxHighlight", 1) != 0;
                    bool formatContent = Win32::RegGetDWORDValue(regView, L"DetailFormatContent", 1) != 0;
                    bool lineNumbers = Win32::RegGetDWORDValue(regView, L"DetailLineNumbers", 1) != 0;
                    
                    pTabItem->SetDetailWordWrap(wordWrap);
                    pTabItem->SetDetailSyntaxHighlight(syntaxHighlight);
                    pTabItem->SetDetailFormatContent(formatContent);
                    pTabItem->SetDetailLineNumbers(lineNumbers);
                }
            }
        }
        
        // Add the "+" tab at the end if we have views
        if (GetActualViewCount() > 0)
        {
            AddPlusTab();
        }
        
        GetTabCtrl().SetCurSel(Win32::RegGetDWORDValue(regViews, L"Current", 0));
        GetTabCtrl().UpdateLayout();
        GetTabCtrl().Invalidate();
    }
    else
    {
        // First run - no registry settings exist
        // Create a default view since none were loaded
        AddFilterView(L"View", LogFilter());

        // Ensure the initial view has detail pane hidden
        auto pTabItem = GetTabCtrl().GetItem(0);
        if (pTabItem)
        {
            pTabItem->ShowDetailPane(false);  // Explicitly hide detail pane on first run
        }

        // Add the "+" tab for first run
        AddPlusTab();
    }

    CRegKey regColors;
    if (regColors.Open(reg, L"Colors") == ERROR_SUCCESS)
    {
        auto colors = ColorDialog::GetCustomColors();
        for (int i = 0; i < 16; ++i)
        {
            colors[i] = Win32::RegGetDWORDValue(regColors, WStr(wstringbuilder() << L"Color" << i));
        }
    }
    
    // Load procedure pane settings
    m_bProcedurePaneVisible = Win32::RegGetDWORDValue(reg, L"ProcedurePaneVisible", 0) != 0;
    // These are stored but not currently used since we auto-position the window
    m_procedureWindowRect.left = 0;
    m_procedureWindowRect.top = 0;
    m_procedureWindowRect.right = 400;
    m_procedureWindowRect.bottom = 600;
    
    // We'll apply visibility after the window is fully created
    // NOTE: Procedure tracking is now integrated into detail pane tabs
    // UISetCheck(ID_VIEW_PROCEDURE_TRACKER, m_bProcedurePaneVisible);

    // Load new view defaults
    LoadNewViewDefaults();

    // MCP server settings (server is started at the end of OnCreate)
    m_mcpSettings.enabled = Win32::RegGetDWORDValue(reg, L"McpEnabled", 0) != 0;
    m_mcpSettings.port = std::clamp(static_cast<int>(Win32::RegGetDWORDValue(reg, L"McpPort", 7777)), 1, 65535);
    m_mcpSettings.token = Win32::RegGetStringValue(reg, L"McpToken", L"");
    m_mcpSettings.maskSecrets = Win32::RegGetDWORDValue(reg, L"McpMaskSecrets", 1) != 0;

    return true;
}

void CMainFrame::SaveSettings()
{
    auto mutex = Win32::CreateMutex(nullptr, false, L"Local\\DebugView++");
    Win32::MutexLock lock(mutex.get());

    auto placement = Win32::GetWindowPlacement(*this);

    CRegKey reg;
    reg.Create(HKEY_CURRENT_USER, RegistryPath);
    reg.SetDWORDValue(L"X", placement.rcNormalPosition.left);
    reg.SetDWORDValue(L"Y", placement.rcNormalPosition.top);
    reg.SetDWORDValue(L"Width", placement.rcNormalPosition.right - placement.rcNormalPosition.left);
    reg.SetDWORDValue(L"Height", placement.rcNormalPosition.bottom - placement.rcNormalPosition.top);

    reg.SetDWORDValue(L"Maximized", placement.showCmd == SW_SHOWMAXIMIZED);
    reg.SetDWORDValue(L"Minimized", placement.showCmd == SW_SHOWMINIMIZED);

    reg.SetDWORDValue(L"MaxX", placement.ptMaxPosition.x);
    reg.SetDWORDValue(L"MaxY", placement.ptMaxPosition.y);

    reg.SetDWORDValue(L"LinkViews", static_cast<DWORD>(m_linkViews));
    reg.SetDWORDValue(L"AutoNewLine", static_cast<DWORD>(m_logSources.GetAutoNewLine()));
    reg.SetDWORDValue(L"AlwaysOnTop", static_cast<DWORD>(GetAlwaysOnTop()));
    reg.SetDWORDValue(L"Hide", static_cast<DWORD>(m_hide));
    reg.SetDWORDValue(L"ExpandedToolbar", static_cast<DWORD>(m_expandedToolbar));
    reg.SetDWORDValue(L"McpEnabled", static_cast<DWORD>(m_mcpSettings.enabled));
    reg.SetDWORDValue(L"McpPort", static_cast<DWORD>(m_mcpSettings.port));
    reg.SetStringValue(L"McpToken", m_mcpSettings.token.c_str());
    reg.SetDWORDValue(L"McpMaskSecrets", static_cast<DWORD>(m_mcpSettings.maskSecrets));

    reg.SetStringValue(L"FontName", m_logfont.lfFaceName);
    reg.SetDWORDValue(L"FontSize", LogFontSizeToPointSize(m_logfont.lfHeight));
    
    // Save detail pane font
    reg.SetStringValue(L"DetailFontName", m_detailLogfont.lfFaceName);
    reg.SetDWORDValue(L"DetailFontSize", LogFontSizeToPointSize(m_detailLogfont.lfHeight));

    reg.RecurseDeleteKey(L"Views");

    CRegKey regViews;
    regViews.Create(reg, L"Views");
    regViews.SetDWORDValue(L"Current", GetTabCtrl().GetCurSel());

    int views = GetActualViewCount();
    regViews.SetDWORDValue(L"Count", views);
    
    // Save each view with its current tab position
    // The key is the position, and we save all view data including name and filters
    for (int i = 0; i < views; ++i)
    {
        CRegKey regView;
        regView.Create(regViews, WStr(wstringbuilder() << L"View" << i));
        regView.SetStringValue(L"", GetView(i).GetName().c_str());
        GetView(i).SaveSettings(regView);
        
        // Update and save the splitter position for this view
        auto pTabItem = GetTabCtrl().GetItem(i);
        if (pTabItem)
        {
            // Save detail pane visibility
            regView.SetDWORDValue(L"DetailPaneVisible", static_cast<DWORD>(pTabItem->IsDetailPaneVisible()));
            
            pTabItem->UpdateSplitterPos();  // Get current position before saving
            regView.SetDWORDValue(L"DetailSplitterPos", static_cast<DWORD>(pTabItem->GetSplitterPos()));
            
            // Save per-view detail pane settings
            regView.SetDWORDValue(L"DetailWordWrap", static_cast<DWORD>(pTabItem->GetDetailWordWrap()));
            regView.SetDWORDValue(L"DetailSyntaxHighlight", static_cast<DWORD>(pTabItem->GetDetailSyntaxHighlight()));
            regView.SetDWORDValue(L"DetailFormatContent", static_cast<DWORD>(pTabItem->GetDetailFormatContent()));
            regView.SetDWORDValue(L"DetailLineNumbers", static_cast<DWORD>(pTabItem->GetDetailLineNumbers()));
        }
    }

    int procedureStatsPos = m_procedureStatsSplitterPos;
    bool procedureSplitFound = false;
    for (int i = 0; i < views; ++i)
    {
        auto pTabItem = GetTabCtrl().GetItem(i);
        if (auto* logTab = dynamic_cast<CLogViewTabItem2*>(pTabItem))
        {
            auto openProceduresTab = logTab->GetDetailTabContainer().GetOpenProceduresTab();
            if (openProceduresTab)
            {
                procedureStatsPos = openProceduresTab->GetSplitterPosPct();
                procedureSplitFound = true;
                break;
            }
        }
    }
    if (!procedureSplitFound && m_wndProcedureView.IsWindow())
    {
        procedureStatsPos = m_wndProcedureView.GetProcedureStatsSplitterPos();
    }
    SetProcedureStatsSplitterPos(procedureStatsPos);
    reg.SetDWORDValue(L"ProcedureStatsSplitterPos", static_cast<DWORD>(m_procedureStatsSplitterPos));

    // Save procedure pane settings
    reg.SetDWORDValue(L"ProcedurePaneVisible", static_cast<DWORD>(m_bProcedurePaneVisible));
    // Note: We're not saving position since the procedure tracker is now docked
    
    CRegKey regColors;
    regColors.Create(reg, L"Colors");
    auto colors = ColorDialog::GetCustomColors();
    for (int i = 0; i < 16; ++i)
    {
        regColors.SetDWORDValue(WStr(wstringbuilder() << L"Color" << i), colors[i]);
    }
    
    // Save new view defaults
    SaveNewViewDefaults();
}

bool CMainFrame::ImportDebugViewPPSettings()
{
    // Check if Ultimate settings already exist
    CRegKey testReg;
    if (testReg.Open(HKEY_CURRENT_USER, RegistryPath, KEY_READ) == ERROR_SUCCESS)
    {
        // Check if we have any views saved
        CRegKey viewsReg;
        if (viewsReg.Open(testReg, L"Views", KEY_READ) == ERROR_SUCCESS)
        {
            // Settings already exist, ask user if they want to overwrite
            int result = MessageBox(
                L"Ultimate DebugView++ settings already exist.\n\n"
                L"Do you want to overwrite them with settings from DebugView++?",
                L"Import Settings",
                MB_YESNO | MB_ICONQUESTION);
            
            if (result != IDYES)
                return false;
        }
    }
    
    // Try to find original DebugView++ settings
    CRegKey oldReg;
    bool foundOldSettings = false;
    const wchar_t* sourcePath = nullptr;
    
    // Try first path (with space)
    if (oldReg.Open(HKEY_CURRENT_USER, OldRegistryPath, KEY_READ) == ERROR_SUCCESS)
    {
        foundOldSettings = true;
        sourcePath = OldRegistryPath;
        oldReg.Close();
    }
    // Try second path (without space)
    else if (oldReg.Open(HKEY_CURRENT_USER, OldRegistryPath2, KEY_READ) == ERROR_SUCCESS)
    {
        foundOldSettings = true;
        sourcePath = OldRegistryPath2;
        oldReg.Close();
    }
    
    if (!foundOldSettings)
    {
        MessageBox(
            L"No DebugView++ settings found to import.",
            L"Import Settings",
            MB_OK | MB_ICONINFORMATION);
        return false;
    }
    
    // Helper function to copy registry values recursively
    std::function<void(HKEY, HKEY)> CopyRegistryKey = [&](HKEY hKeySrc, HKEY hKeyDst)
    {
        // Copy all values
        DWORD index = 0;
        wchar_t valueName[256];
        DWORD valueNameLen;
        DWORD type;
        BYTE data[4096];
        DWORD dataLen;
        
        while (true)
        {
            valueNameLen = 256;
            dataLen = 4096;
            
            LONG result = RegEnumValue(hKeySrc, index++, valueName, &valueNameLen,
                                      nullptr, &type, data, &dataLen);
            if (result != ERROR_SUCCESS)
                break;
                
            RegSetValueEx(hKeyDst, valueName, 0, type, data, dataLen);
        }
        
        // Copy all subkeys
        index = 0;
        wchar_t keyName[256];
        DWORD keyNameLen;
        
        while (true)
        {
            keyNameLen = 256;
            LONG result = RegEnumKeyEx(hKeySrc, index++, keyName, &keyNameLen,
                                      nullptr, nullptr, nullptr, nullptr);
            if (result != ERROR_SUCCESS)
                break;
                
            CRegKey srcSubKey, dstSubKey;
            if (srcSubKey.Open(hKeySrc, keyName, KEY_READ) == ERROR_SUCCESS)
            {
                dstSubKey.Create(hKeyDst, keyName);
                CopyRegistryKey(srcSubKey.m_hKey, dstSubKey.m_hKey);
            }
        }
    };
    
    // Import the settings
    CRegKey srcReg, dstReg;
    if (srcReg.Open(HKEY_CURRENT_USER, sourcePath, KEY_READ) == ERROR_SUCCESS)
    {
        dstReg.Create(HKEY_CURRENT_USER, RegistryPath);
        CopyRegistryKey(srcReg.m_hKey, dstReg.m_hKey);
        srcReg.Close();
        dstReg.Close();
    }
    
    // Also import column layouts if they exist
    std::wstring srcColumnPath = std::wstring(sourcePath) + L"\\ColumnLayouts";
    std::wstring dstColumnPath = std::wstring(RegistryPath) + L"\\ColumnLayouts";
    
    CRegKey srcColumns, dstColumns;
    if (srcColumns.Open(HKEY_CURRENT_USER, srcColumnPath.c_str(), KEY_READ) == ERROR_SUCCESS)
    {
        dstColumns.Create(HKEY_CURRENT_USER, dstColumnPath.c_str());
        CopyRegistryKey(srcColumns.m_hKey, dstColumns.m_hKey);
    }
    
    MessageBox(
        L"Settings imported successfully from DebugView++.\n\n"
        L"Please restart Ultimate DebugView++ to apply the imported settings.",
        L"Import Complete",
        MB_OK | MB_ICONINFORMATION);
    
    // Add the "+" tab as the last tab after importing settings
    AddPlusTab();
    
    return true;
}

void CMainFrame::LoadNewViewDefaults()
{
    CRegKey reg;
    if (reg.Open(HKEY_CURRENT_USER, RegistryPath, KEY_READ) != ERROR_SUCCESS)
        return;
    
    CRegKey regDefaults;
    if (regDefaults.Open(reg, L"NewViewDefaults", KEY_READ) != ERROR_SUCCESS)
        return;
    
    DWORD dwValue;
    
    // Load boolean settings
    if (regDefaults.QueryDWORDValue(L"ShowDetailPane", dwValue) == ERROR_SUCCESS)
        m_newViewDefaults.showDetailPane = (dwValue != 0);
        
    if (regDefaults.QueryDWORDValue(L"DetailWordWrap", dwValue) == ERROR_SUCCESS)
        m_newViewDefaults.detailWordWrap = (dwValue != 0);
        
    if (regDefaults.QueryDWORDValue(L"DetailSyntaxHighlight", dwValue) == ERROR_SUCCESS)
        m_newViewDefaults.detailSyntaxHighlight = (dwValue != 0);
        
    if (regDefaults.QueryDWORDValue(L"DetailFormatContent", dwValue) == ERROR_SUCCESS)
        m_newViewDefaults.detailFormatContent = (dwValue != 0);
        
    if (regDefaults.QueryDWORDValue(L"DetailLineNumbers", dwValue) == ERROR_SUCCESS)
        m_newViewDefaults.detailLineNumbers = (dwValue != 0);
    
    // Load splitter position
    if (regDefaults.QueryDWORDValue(L"DetailSplitterPos", dwValue) == ERROR_SUCCESS)
    {
        // Ensure the value is within valid range (0-100)
        int splitterPos = static_cast<int>(dwValue);
        if (splitterPos < 0) splitterPos = 0;
        if (splitterPos > 100) splitterPos = 100;
        m_newViewDefaults.detailSplitterPos = splitterPos;
    }
    
    // Load theme
    if (regDefaults.QueryDWORDValue(L"DefaultTheme", dwValue) == ERROR_SUCCESS)
    {
        switch (dwValue)
        {
        case 0:
            m_newViewDefaults.defaultTheme = Theme::System;
            break;
        case 1:
            m_newViewDefaults.defaultTheme = Theme::Light;
            break;
        case 2:
            m_newViewDefaults.defaultTheme = Theme::Dark;
            break;
        default:
            m_newViewDefaults.defaultTheme = Theme::System;
            break;
        }
    }
    
    // Load font settings
    wchar_t fontName[LF_FACESIZE] = L"";
    ULONG fontNameSize = LF_FACESIZE;
    if (regDefaults.QueryStringValue(L"DetailFontName", fontName, &fontNameSize) == ERROR_SUCCESS)
    {
        wcscpy_s(m_newViewDefaults.detailFont.lfFaceName, fontName);
    }
    
    if (regDefaults.QueryDWORDValue(L"DetailFontSize", dwValue) == ERROR_SUCCESS)
    {
        m_newViewDefaults.detailFont.lfHeight = LogFontSizeFromPointSize(static_cast<int>(dwValue));
    }

    // Load SQL formatting options
    if (regDefaults.QueryDWORDValue(L"SqlKeywordCase", dwValue) == ERROR_SUCCESS)
    {
        switch (dwValue)
        {
        case 0:
            m_newViewDefaults.sqlKeywordCase = SqlKeywordCase::Upper;
            break;
        case 1:
            m_newViewDefaults.sqlKeywordCase = SqlKeywordCase::Lower;
            break;
        case 2:
            m_newViewDefaults.sqlKeywordCase = SqlKeywordCase::Capitalize;
            break;
        default:
            m_newViewDefaults.sqlKeywordCase = SqlKeywordCase::Upper;
            break;
        }
    }

    if (regDefaults.QueryDWORDValue(L"SqlIndentType", dwValue) == ERROR_SUCCESS)
    {
        m_newViewDefaults.sqlIndentType = (dwValue == 1) ? SqlIndentType::Tab : SqlIndentType::Spaces;
    }

    if (regDefaults.QueryDWORDValue(L"SqlIndentSpaces", dwValue) == ERROR_SUCCESS)
    {
        m_newViewDefaults.sqlIndentSpaces = static_cast<int>(dwValue);
        if (m_newViewDefaults.sqlIndentSpaces < 1) m_newViewDefaults.sqlIndentSpaces = 1;
        if (m_newViewDefaults.sqlIndentSpaces > 20) m_newViewDefaults.sqlIndentSpaces = 20;
    }

    // Load comma alignment (0=Inline, 1=Trailing, 2=Leading)
    DWORD sqlCommaAlignment = 1; // Default to Trailing
    if (regDefaults.QueryDWORDValue(L"SqlCommaAlignment", dwValue) == ERROR_SUCCESS)
    {
        switch (dwValue)
        {
        case 0:
            m_newViewDefaults.sqlCommaAlignment = SqlCommaAlignment::Inline;
            break;
        case 1:
            m_newViewDefaults.sqlCommaAlignment = SqlCommaAlignment::Trailing;
            break;
        case 2:
            m_newViewDefaults.sqlCommaAlignment = SqlCommaAlignment::Leading;
            break;
        default:
            m_newViewDefaults.sqlCommaAlignment = SqlCommaAlignment::Trailing;
            break;
        }
    }

    if (regDefaults.QueryDWORDValue(L"SqlNewlineKeywords", dwValue) == ERROR_SUCCESS)
        m_newViewDefaults.sqlNewlineKeywords = (dwValue != 0);

    // Load format style
    if (regDefaults.QueryDWORDValue(L"SqlFormatStyle", dwValue) == ERROR_SUCCESS)
    {
        switch (dwValue)
        {
        case 0:
            m_newViewDefaults.sqlFormatStyle = SqlFormatStyle::Compact;
            break;
        case 1:
            m_newViewDefaults.sqlFormatStyle = SqlFormatStyle::Normal;
            break;
        case 2:
            m_newViewDefaults.sqlFormatStyle = SqlFormatStyle::Expanded;
            break;
        default:
            m_newViewDefaults.sqlFormatStyle = SqlFormatStyle::Normal;
            break;
        }
    }

    // Load parentheses style
    if (regDefaults.QueryDWORDValue(L"SqlParenthesesStyle", dwValue) == ERROR_SUCCESS)
    {
        switch (dwValue)
        {
        case 0:
            m_newViewDefaults.sqlParenthesesStyle = SqlParenthesesStyle::SameLine;
            break;
        case 1:
            m_newViewDefaults.sqlParenthesesStyle = SqlParenthesesStyle::NewLine;
            break;
        case 2:
            m_newViewDefaults.sqlParenthesesStyle = SqlParenthesesStyle::IndentContent;
            break;
        default:
            m_newViewDefaults.sqlParenthesesStyle = SqlParenthesesStyle::SameLine;
            break;
        }
    }

    // Load prefix filters
    CRegKey regPrefixFilters;
    if (regPrefixFilters.Open(regDefaults, L"PrefixFilters", KEY_READ) == ERROR_SUCCESS)
    {
        // Load system prefix filters (only colors and enable state, text is hardcoded)
        CRegKey regSystemPrefixes;
        if (regSystemPrefixes.Open(regPrefixFilters, L"SystemPrefixes", KEY_READ) == ERROR_SUCCESS)
        {
            // Load settings for each system prefix filter
            const wchar_t* systemPrefixes[] = { L"Events", L"Open", L"Close", L"Trace", L"Queue", L"Group", L"Error", L"SQL" };
            const std::string systemPrefixTexts[] = { "[Events]", "[Open]", "[Close]", "[Trace]", "[Queue]", "[Group]", "[Error]", "[SQL]" };

            for (size_t i = 0; i < 8 && i < m_newViewDefaults.systemPrefixFilters.size(); ++i)
            {
                CRegKey regPrefix;
                if (regPrefix.Open(regSystemPrefixes, systemPrefixes[i], KEY_READ) == ERROR_SUCCESS)
                {
                    // Load colors and enable state from registry, keep hardcoded text
                    if (regPrefix.QueryDWORDValue(L"BgColor", dwValue) == ERROR_SUCCESS)
                    {
                        m_newViewDefaults.systemPrefixFilters[i].bgColor = static_cast<COLORREF>(dwValue);
                    }
                    
                    if (regPrefix.QueryDWORDValue(L"FgColor", dwValue) == ERROR_SUCCESS)
                    {
                        m_newViewDefaults.systemPrefixFilters[i].fgColor = static_cast<COLORREF>(dwValue);
                    }
                    
                    if (regPrefix.QueryDWORDValue(L"Enable", dwValue) == ERROR_SUCCESS)
                    {
                        m_newViewDefaults.systemPrefixFilters[i].enable = (dwValue != 0);
                    }
                }
            }
        }
        
        // Load custom prefix filters
        CRegKey regCustomPrefixes;
        if (regCustomPrefixes.Open(regPrefixFilters, L"CustomPrefixes", KEY_READ) == ERROR_SUCCESS)
        {
            m_newViewDefaults.customPrefixFilters.clear();
            
            for (int i = 0;; ++i)
            {
                CRegKey regPrefix;
                std::wstring prefixKey = std::to_wstring(i);
                if (regPrefix.Open(regCustomPrefixes, prefixKey.c_str(), KEY_READ) != ERROR_SUCCESS)
                {
                    break;
                }
                
                // Load all properties for custom prefix filter
                wchar_t text[256] = L"";
                ULONG textSize = 256;
                if (regPrefix.QueryStringValue(L"Text", text, &textSize) != ERROR_SUCCESS)
                {
                    continue; // Skip if no text
                }
                
                DWORD type = FilterType::Highlight;
                regPrefix.QueryDWORDValue(L"Type", type);
                
                DWORD bgColor = RGB(255, 255, 255);
                regPrefix.QueryDWORDValue(L"BgColor", bgColor);
                
                DWORD fgColor = RGB(0, 0, 0);
                regPrefix.QueryDWORDValue(L"FgColor", fgColor);
                
                DWORD enable = 1;
                regPrefix.QueryDWORDValue(L"Enable", enable);
                
                // Create and add the filter
                std::string filterText = Str(std::wstring(text));
                FilterType::type filterType = IntToFilterType(static_cast<int>(type));
                m_newViewDefaults.customPrefixFilters.emplace_back(
                    filterText, 
                    MatchType::Simple, 
                    filterType,
                    static_cast<COLORREF>(bgColor), 
                    static_cast<COLORREF>(fgColor), 
                    enable != 0
                );
            }
        }
    }
}

void CMainFrame::SaveNewViewDefaults()
{
    CRegKey reg;
    reg.Create(HKEY_CURRENT_USER, RegistryPath);
    
    CRegKey regDefaults;
    regDefaults.Create(reg, L"NewViewDefaults");
    
    // Save boolean settings
    regDefaults.SetDWORDValue(L"ShowDetailPane", m_newViewDefaults.showDetailPane ? 1 : 0);
    regDefaults.SetDWORDValue(L"DetailWordWrap", m_newViewDefaults.detailWordWrap ? 1 : 0);
    regDefaults.SetDWORDValue(L"DetailSyntaxHighlight", m_newViewDefaults.detailSyntaxHighlight ? 1 : 0);
    regDefaults.SetDWORDValue(L"DetailFormatContent", m_newViewDefaults.detailFormatContent ? 1 : 0);
    regDefaults.SetDWORDValue(L"DetailLineNumbers", m_newViewDefaults.detailLineNumbers ? 1 : 0);
    
    // Save splitter position
    regDefaults.SetDWORDValue(L"DetailSplitterPos", static_cast<DWORD>(m_newViewDefaults.detailSplitterPos));
    
    // Save theme
    DWORD themeValue = 0;
    switch (m_newViewDefaults.defaultTheme)
    {
    case Theme::System:
        themeValue = 0;
        break;
    case Theme::Light:
        themeValue = 1;
        break;
    case Theme::Dark:
        themeValue = 2;
        break;
    }
    regDefaults.SetDWORDValue(L"DefaultTheme", themeValue);
    
    // Save font settings
    regDefaults.SetStringValue(L"DetailFontName", m_newViewDefaults.detailFont.lfFaceName);
    regDefaults.SetDWORDValue(L"DetailFontSize", LogFontSizeToPointSize(m_newViewDefaults.detailFont.lfHeight));
    
    // Save prefix filters
    CRegKey regPrefixFilters;
    regPrefixFilters.Create(regDefaults, L"PrefixFilters");
    
    // Save system prefix filters (only colors and enable state, text is hardcoded)
    CRegKey regSystemPrefixes;
    regSystemPrefixes.Create(regPrefixFilters, L"SystemPrefixes");
    
    const wchar_t* systemPrefixes[] = { L"Events", L"Open", L"Close", L"Trace", L"Queue", L"Group", L"Error", L"SQL" };
    for (size_t i = 0; i < 8 && i < m_newViewDefaults.systemPrefixFilters.size(); ++i)
    {
        CRegKey regPrefix;
        regPrefix.Create(regSystemPrefixes, systemPrefixes[i]);
        
        regPrefix.SetDWORDValue(L"BgColor", m_newViewDefaults.systemPrefixFilters[i].bgColor);
        regPrefix.SetDWORDValue(L"FgColor", m_newViewDefaults.systemPrefixFilters[i].fgColor);
        regPrefix.SetDWORDValue(L"Enable", m_newViewDefaults.systemPrefixFilters[i].enable ? 1 : 0);
    }
    
    // Save custom prefix filters
    CRegKey regCustomPrefixes;
    regCustomPrefixes.Create(regPrefixFilters, L"CustomPrefixes");
    
    for (size_t i = 0; i < m_newViewDefaults.customPrefixFilters.size(); ++i)
    {
        CRegKey regPrefix;
        std::wstring prefixKey = std::to_wstring(i);
        regPrefix.Create(regCustomPrefixes, prefixKey.c_str());
        
        const auto& filter = m_newViewDefaults.customPrefixFilters[i];
        regPrefix.SetStringValue(L"Text", WStr(filter.text));
        regPrefix.SetDWORDValue(L"Type", FilterTypeToInt(filter.filterType));
        regPrefix.SetDWORDValue(L"BgColor", filter.bgColor);
        regPrefix.SetDWORDValue(L"FgColor", filter.fgColor);
        regPrefix.SetDWORDValue(L"Enable", filter.enable ? 1 : 0);
    }

    // Save SQL formatting options
    DWORD sqlKeywordCase = 0;
    switch (m_newViewDefaults.sqlKeywordCase)
    {
    case fusion::debugviewpp::SqlKeywordCase::Upper:
        sqlKeywordCase = 0;
        break;
    case fusion::debugviewpp::SqlKeywordCase::Lower:
        sqlKeywordCase = 1;
        break;
    case fusion::debugviewpp::SqlKeywordCase::Capitalize:
        sqlKeywordCase = 2;
        break;
    }
    regDefaults.SetDWORDValue(L"SqlKeywordCase", sqlKeywordCase);

    DWORD sqlIndentStyle = 0;
    regDefaults.SetDWORDValue(L"SqlIndentType", m_newViewDefaults.sqlIndentType == SqlIndentType::Tab ? 1 : 0);
    regDefaults.SetDWORDValue(L"SqlIndentSpaces", m_newViewDefaults.sqlIndentSpaces);

    // Save comma alignment (0=Inline, 1=Trailing, 2=Leading)
    DWORD sqlCommaAlignment = 1; // Default to Trailing
    switch (m_newViewDefaults.sqlCommaAlignment)
    {
    case SqlCommaAlignment::Inline:
        sqlCommaAlignment = 0;
        break;
    case SqlCommaAlignment::Trailing:
        sqlCommaAlignment = 1;
        break;
    case SqlCommaAlignment::Leading:
        sqlCommaAlignment = 2;
        break;
    }
    regDefaults.SetDWORDValue(L"SqlCommaAlignment", sqlCommaAlignment);
    regDefaults.SetDWORDValue(L"SqlNewlineKeywords", m_newViewDefaults.sqlNewlineKeywords ? 1 : 0);

    // Save format style
    DWORD sqlFormatStyle = 1;  // Default to Normal
    switch (m_newViewDefaults.sqlFormatStyle)
    {
    case SqlFormatStyle::Compact:
        sqlFormatStyle = 0;
        break;
    case SqlFormatStyle::Normal:
        sqlFormatStyle = 1;
        break;
    case SqlFormatStyle::Expanded:
        sqlFormatStyle = 2;
        break;
    }
    regDefaults.SetDWORDValue(L"SqlFormatStyle", sqlFormatStyle);

    // Save parentheses style
    DWORD sqlParenthesesStyle = 0;  // Default to SameLine
    switch (m_newViewDefaults.sqlParenthesesStyle)
    {
    case SqlParenthesesStyle::SameLine:
        sqlParenthesesStyle = 0;
        break;
    case SqlParenthesesStyle::NewLine:
        sqlParenthesesStyle = 1;
        break;
    case SqlParenthesesStyle::IndentContent:
        sqlParenthesesStyle = 2;
        break;
    }
    regDefaults.SetDWORDValue(L"SqlParenthesesStyle", sqlParenthesesStyle);
}

void CMainFrame::FindNext(const std::wstring& text)
{
    if (!GetView().FindNext(text))
    {
        MessageBeep(MB_ICONASTERISK);
    }
}

void CMainFrame::FindPrevious(const std::wstring& text)
{
    if (!GetView().FindPrevious(text))
    {
        MessageBeep(MB_ICONASTERISK);
    }
}

// Helper function to format event details with aligned columns (DEPRECATED - kept for compatibility)
std::wstring FormatEventDetailsWithColumns(const Message& message, int lineIndex)
{
    // Now just returns the message text
    return WStr(message.text).str();
}

// Helper function to get just the message text for detail content
std::wstring FormatMessageOnly(const Message& message)
{
    return WStr(message.text).str();
}

// Helper function to format metadata for status line
std::wstring FormatMetadataForStatus(const Message& message, int lineIndex)
{
    std::wstringstream status;

    // Basic metadata
    status << L"Line: " << std::to_wstring(lineIndex + 1)
           << L"  |  Time: " << WStr(GetTimeText(message.systemTime)).str()
           << L"  |  Process: [" << std::to_wstring(message.processId) << L"] "
           << WStr(message.processName).str();

    // Add Clarion-specific fields if present
    if (message.isClarionFormat)
    {
        if (!message.clarionThread.empty())
        {
            status << L"  |  Thread: " << WStr(message.clarionThread).str();
        }

        if (!message.clarionProcedure.empty())
        {
            status << L"  |  Procedure: " << WStr(message.clarionProcedure).str();

            // Add procedure event info
            if (message.procedureEvent == Message::ProcedureEvent::Open)
            {
                status << L" [OPEN]";
            }
            else if (message.procedureEvent == Message::ProcedureEvent::Close)
            {
                status << L" [CLOSE]";
                if (message.procedureDuration > 0.0)
                {
                    status << L" (" << std::fixed << std::setprecision(3)
                           << message.procedureDuration << L"s)";
                }
            }

            // Add call depth
            if (message.callDepth > 0)
            {
                status << L" (Depth: " << message.callDepth << L")";
            }
        }

        if (!message.clarionModule.empty())
        {
            status << L"  |  Module: " << WStr(message.clarionModule).str();
        }

        if (!message.clarionApp.empty())
        {
            status << L"  |  App: " << WStr(message.clarionApp).str();
        }

        if (!message.clarionPrefix.empty())
        {
            status << L"  |  Prefix: " << WStr(message.clarionPrefix).str();
        }
    }

    return status.str();
}

void CMainFrame::UpdateDetailView(CLogView* pCallingView, int lineIndex)
{
    if (GetViewCount() == 0 || !pCallingView)
        return;

    // Get the current tab item
    auto tabIndex = std::max(0, GetTabCtrl().GetCurSel());
    auto pTabItem = GetTabCtrl().GetItem(tabIndex);

    // Only update if the calling view is the current tab's view
    if (pTabItem && &pTabItem->GetView() == pCallingView && lineIndex >= 0 && lineIndex < m_logFile.Count())
    {
        // Get the full message text
        const auto& message = m_logFile[lineIndex];

        // Build status text with all metadata
        std::wstring statusText = FormatMetadataForStatus(message, lineIndex);

        // Get just the message text for content
        std::wstring contentText = FormatMessageOnly(message);

        // Get the unparsed message for Clarion messages
        std::wstring unparsedText;
        if (message.isClarionFormat)
        {
            unparsedText = WStr(m_logFile.GetRawMessage(lineIndex)).str();
        }

        // Update the detail view with separated status, content, and unparsed text
        pTabItem->UpdateDetailView(statusText, contentText, unparsedText);

        // Check if this is a Queue message and update Queue tab if it is
        if (message.isQueueMessage && !message.queueId.empty())
        {
            auto queueManager = m_logFile.GetQueueManager();
            if (queueManager)
            {
                auto queueData = queueManager->GetQueue(message.queueId);
                if (queueData)
                {
                    pTabItem->UpdateQueueView(queueData);
                }
            }
        }
    }
}

void CMainFrame::UpdateDetailView(CLogView* pCallingView, const std::vector<int>& lineIndices)
{
    if (GetViewCount() == 0 || !pCallingView)
        return;

    // Get the current tab item
    auto tabIndex = std::max(0, GetTabCtrl().GetCurSel());
    auto pTabItem = GetTabCtrl().GetItem(tabIndex);

    // Only update if the calling view is the current tab's view
    if (!pTabItem || &pTabItem->GetView() != pCallingView)
        return;

    if (lineIndices.empty())
    {
        // Clear the detail view when nothing is selected
        pTabItem->UpdateDetailView(L"", L"", L"");
        return;
    }

    std::wstring statusText;
    std::wstring contentText;
    std::wstring unparsedText;

    // If only one line selected, show detailed metadata in status line
    if (lineIndices.size() == 1)
    {
        int lineIndex = lineIndices[0];
        if (lineIndex >= 0 && lineIndex < m_logFile.Count())
        {
            const auto& message = m_logFile[lineIndex];

            // Build status text with all metadata
            statusText = FormatMetadataForStatus(message, lineIndex);

            // Get just the message text for content
            contentText = FormatMessageOnly(message);

            // Get unparsed text for Clarion messages
            if (message.isClarionFormat)
            {
                unparsedText = WStr(m_logFile.GetRawMessage(lineIndex)).str();
            }

            // Update the detail view first
            pTabItem->UpdateDetailView(statusText, contentText, unparsedText);

            // Check if this is a Queue message and update Queue tab if it is
            if (message.isQueueMessage && !message.queueId.empty())
            {
                auto queueManager = m_logFile.GetQueueManager();
                if (queueManager)
                {
                    auto queueData = queueManager->GetQueue(message.queueId);
                    if (queueData)
                    {
                        pTabItem->UpdateQueueView(queueData);
                    }
                }
            }
            return;  // Single selection handled, don't update again below
        }
    }
    else
    {
        // Multiple lines selected - status shows count
        statusText = L"Selected: " + std::to_wstring(lineIndices.size()) + L" lines";

        // Combine all message texts with CR/LF
        for (int lineIndex : lineIndices)
        {
            if (lineIndex >= 0 && lineIndex < m_logFile.Count())
            {
                const auto& message = m_logFile[lineIndex];
                if (!contentText.empty())
                    contentText += L"\r\n";
                contentText += WStr(message.text).str();
            }
        }
    }

    // Update the detail view with separated status and content
    // unparsedText is only set for single-line selections with Clarion format
    pTabItem->UpdateDetailView(statusText, contentText, unparsedText);
}

void CMainFrame::FindTextInView(const std::wstring& text)
{
    if (GetViewCount() == 0 || text.empty())
        return;
        
    // Set the search text in the find dialog
    m_findDlg.SetDlgItemText(IDC_TEXT, text.c_str());
    
    // Trigger find next
    GetView().SetHighlightText(text);
    if (!GetView().FindNext(text))
    {
        MessageBeep(MB_ICONASTERISK);
    }
}

void CMainFrame::HighlightTextInView(const std::wstring& text)
{
    if (GetViewCount() == 0 || text.empty())
        return;
        
    // Set the search text in the find dialog
    m_findDlg.SetDlgItemText(IDC_TEXT, text.c_str());
    
    // Just highlight the text without searching for next occurrence
    GetView().SetHighlightText(text);
}

void CMainFrame::ClearFind()
{
    if (GetViewCount() == 0)
        return;
    
    // Clear the highlighting in the current view
    GetView().SetHighlightText(L"");
    
    // Clear the highlighting in the detail pane
    int tabIndex = std::max(0, GetTabCtrl().GetCurSel());
    if (tabIndex < GetTabCtrl().GetItemCount())
    {
        auto pTabItem = GetTabCtrl().GetItem(tabIndex);
        if (pTabItem)
        {
            pTabItem->ClearDetailHighlight();
        }
    }
    
    // Update the status bar
    UpdateStatusBar();
}

void CMainFrame::ShowFilterDialogWithText(const std::wstring& text)
{
    if (GetViewCount() == 0 || text.empty())
        return;
        
    // Get current filters from the active view
    auto filters = GetView().GetFilters();
    
    // Create a new include filter with the provided text
    Filter newFilter;
    newFilter.text = Str(text).str();
    newFilter.matchType = MatchType::Simple;
    newFilter.filterType = FilterType::Include;
    newFilter.bgColor = RGB(255, 255, 255);
    newFilter.fgColor = RGB(0, 0, 0);
    newFilter.enable = true;
    newFilter.matched = false;
    
    // Add the new filter to the message filters
    filters.messageFilters.push_back(newFilter);
    
    // Open the filter dialog with the modified filters
    CFilterDlg dlg(GetView().GetName(), filters, &m_logFile, {}, false, GetView().GetUniqueProcedures());
    if (dlg.DoModal() == IDOK)
    {
        int tabIdx = GetTabCtrl().GetCurSel();
        auto pTabItem = GetTabCtrl().GetItem(tabIdx);
        if (pTabItem)
        {
            pTabItem->SetText(dlg.GetName().c_str());
        }
        GetTabCtrl().UpdateLayout();
        GetTabCtrl().Invalidate();
        GetView().SetName(dlg.GetName());
        GetView().SetFilters(dlg.GetFilters());
        SaveSettings();
    }
}

void CMainFrame::AddFilterView()
{
    ++m_filterNr;
    
    // Combine system and custom default prefix filters for new views
    std::vector<Filter> defaultPrefixFilters;
    defaultPrefixFilters.reserve(m_newViewDefaults.systemPrefixFilters.size() + m_newViewDefaults.customPrefixFilters.size());
    defaultPrefixFilters.insert(defaultPrefixFilters.end(), m_newViewDefaults.systemPrefixFilters.begin(), m_newViewDefaults.systemPrefixFilters.end());
    defaultPrefixFilters.insert(defaultPrefixFilters.end(), m_newViewDefaults.customPrefixFilters.begin(), m_newViewDefaults.customPrefixFilters.end());
    
    // Cancel any drag operation in the tab control before showing dialog
    // This prevents the "no drop" cursor from persisting
    GetTabCtrl().SendMessage(WM_LBUTTONUP, 0, 0);
    ::ReleaseCapture();
    
    // Ensure we have a stable state before showing dialog
    MSG msg;
    while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    
    // Reset cursor before showing dialog
    ::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
    
    CFilterDlg dlg(wstringbuilder() << L"View " << m_filterNr, LogFilter(), &m_logFile, defaultPrefixFilters, true, GetView().GetUniqueProcedures());
    INT_PTR result = dlg.DoModal();
    
    // Force tab control to stop any drag operation after dialog
    GetTabCtrl().SendMessage(WM_LBUTTONUP, 0, 0);
    ::ReleaseCapture();
    
    // Enhanced cursor reset and focus management after dialog closes
    ::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
    
    // Force cursor refresh by capturing and releasing mouse
    ::SetCapture(m_hWnd);
    ::ReleaseCapture();
    
    // Send WM_CAPTURECHANGED to ensure no window thinks it has capture
    ::SendMessage(GetTabCtrl().m_hWnd, WM_CAPTURECHANGED, 0, 0);
    
    // Ensure any pending messages are processed
    while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    
    // Force main window to foreground and active
    ::SetForegroundWindow(m_hWnd);
    ::SetActiveWindow(m_hWnd);
    
    // Post a WM_SETCURSOR message to trigger cursor update
    POINT pt;
    ::GetCursorPos(&pt);
    ::ScreenToClient(m_hWnd, &pt);
    ::PostMessage(m_hWnd, WM_SETCURSOR, (WPARAM)m_hWnd, MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
    
    if (result != IDOK)
    {
        // Multiple cursor reset attempts for cancel case
        ::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
        
        // Force cursor update with mouse move simulation
        POINT mousePt;
        ::GetCursorPos(&mousePt);
        ::SetCursorPos(mousePt.x, mousePt.y);
        
        // Force the tab control to update its state
        GetTabCtrl().Invalidate();
        GetTabCtrl().UpdateWindow();
        
        // Set focus back to the current view after cancel
        if (GetActualViewCount() > 0)
        {
            int curTab = GetTabCtrl().GetCurSel();
            if (curTab >= 0 && curTab < GetActualViewCount())
            {
                GetView().SetFocus();
                
                // Send a mouse move to reset any hover states
                POINT curPos;
                ::GetCursorPos(&curPos);
                ::ScreenToClient(m_hWnd, &curPos);
                ::SendMessage(m_hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(curPos.x, curPos.y));
            }
        }
        return;
    }

    AddFilterView(dlg.GetName(), dlg.GetFilters());
    SaveSettings();
    
    // Enhanced cursor and focus reset for success case
    ::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
    
    // Force cursor update with mouse move simulation  
    POINT cursorPos;
    ::GetCursorPos(&cursorPos);
    ::SetCursorPos(cursorPos.x, cursorPos.y);
    
    // Set focus to the newly created view
    if (GetActualViewCount() > 0)
    {
        GetView().SetFocus();
    }
    
    // Use a timer to reset the cursor after all events are processed
    // This ensures the cursor reset happens after Windows has finished processing
    ::SetTimer(m_hWnd, 1001, 50, nullptr);  // 50ms delay
}

void CMainFrame::AddFilterView(const std::wstring& name, const LogFilter& filter)
{
    AddFilterView(std::make_shared<CLogView>(name, *this, m_logFile, filter));
}

void CMainFrame::AddFilterView(std::shared_ptr<CLogView> logview)
{
    auto pTabItem = std::make_unique<SelectedTabItem>();
    pTabItem->Create(*this);
    pTabItem->SetMainFrame(this);
    
    // Apply detail font from preferences if available, otherwise use current font
    if (m_newViewDefaults.detailFont.lfHeight != 0)
    {
        // Create font from preferences
        HFONT hPrefsFont = ::CreateFontIndirect(&m_newViewDefaults.detailFont);
        if (hPrefsFont)
        {
            pTabItem->SetDetailFont(hPrefsFont);
            ::DeleteObject(hPrefsFont);  // SetDetailFont makes its own copy
        }
    }
    else if (m_hDetailFont)
    {
        // Fall back to current detail font if no preferences set
        pTabItem->SetDetailFont(m_hDetailFont.get());
    }

    logview->Create(pTabItem->GetLogViewParent(), rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    logview->SetFont(m_hFont.get());

    pTabItem->SetText(logview->GetName().c_str());
    pTabItem->SetView(logview);

    int newIndex = GetActualViewCount(); // Insert before the "+" tab (which is always last)
    
    // The first tab (View) should not be closeable
    if (newIndex == 0)
    {
        pTabItem->SetCanClose(false);
    }

    // Apply new view defaults
    auto pRawTabItem = pTabItem.get();  // Get raw pointer before releasing ownership
    
    // Apply detail pane defaults
    if (m_newViewDefaults.showDetailPane)
    {
        pRawTabItem->ShowDetailPane(true);
        pRawTabItem->SetSplitterPos(m_newViewDefaults.detailSplitterPos);
    }
    
    // Apply detail pane display options
    pRawTabItem->SetDetailWordWrap(m_newViewDefaults.detailWordWrap);
    pRawTabItem->SetDetailSyntaxHighlight(m_newViewDefaults.detailSyntaxHighlight);
    pRawTabItem->SetDetailFormatContent(m_newViewDefaults.detailFormatContent);
    pRawTabItem->SetDetailLineNumbers(m_newViewDefaults.detailLineNumbers);
    
    // Apply theme if not system default
    if (m_newViewDefaults.defaultTheme != Theme::System)
    {
        ThemeManager::Instance().SetTheme(m_newViewDefaults.defaultTheme);
        Colors::UpdateThemeColors();
    }

    // notice: InsertItem takes ownership of the raw pointer, DeleteItem calls delete
    GetTabCtrl().InsertItem(newIndex, pTabItem.release());
    
    // Set flag to prevent dialog from showing when we programmatically change tabs
    m_bProgrammaticTabChange = true;
    GetTabCtrl().SetCurSel(newIndex);
    m_bProgrammaticTabChange = false;
    
    ShowTabControl();
    UpdateViewTabCloseStates();
}

void CMainFrame::AddPlusTab()
{
    // Ensure tab control is valid before proceeding
    if (!GetTabCtrl().IsWindow())
    {
        return;
    }

    // Set flag to prevent any dialog from showing while manipulating the "+" tab
    m_bProgrammaticTabChange = true;

    // Remove any existing "+" tab first
    for (int i = GetTabCtrl().GetItemCount() - 1; i >= 0; --i)
    {
        auto pTabItem = GetTabCtrl().GetItem(i);
        if (pTabItem && pTabItem->GetText() == L"+")
        {
            GetTabCtrl().DeleteItem(i);
            break;
        }
    }
    
    // Add the "+" tab as text-only (no actual tab item object)
    // This avoids type compatibility issues with CLogViewTabItem2
    // We handle clicks on this tab specially in OnChangeTab
    GetTabCtrl().InsertItem(GetTabCtrl().GetItemCount(), L"+", -1, L"Click to create a new tab", false);
    
    // Clear the flag
    m_bProgrammaticTabChange = false;

    UpdateViewTabCloseStates();
}

LRESULT CMainFrame::OnBeginTabDrag(NMHDR* pnmh)
{
    auto& nmhdr = *reinterpret_cast<NMCTCITEM*>(pnmh);

    // Prevent dragging the "+" tab (always the last tab)
    return static_cast<LRESULT>(nmhdr.iItem >= GetActualViewCount());
}

LRESULT CMainFrame::OnChangeTab(NMHDR* pnmh)
{
    auto& nmhdr = *reinterpret_cast<NMCTC2ITEMS*>(pnmh);

    // Check if the user is trying to select the "+" tab (always the last tab)
    int actualViewCount = GetActualViewCount();
    
    // Validate that the indices are within valid bounds
    // Invalid indices can occur during tab control internal operations
    int totalTabs = GetTabCtrl().GetItemCount();
    if (nmhdr.iItem2 < 0 || nmhdr.iItem2 >= totalTabs)
    {
        // Invalid index - ignore this event
        SetMsgHandled(win32::False);
        return 0;
    }
    
    if (nmhdr.iItem2 >= actualViewCount)
    {
        // User clicked on the "+" tab - prevent the selection change
        SetMsgHandled(win32::True);  // Mark as handled to prevent further processing
        
        // Keep the previous tab selected (if valid)
        if (nmhdr.iItem1 >= 0 && nmhdr.iItem1 < actualViewCount)
        {
            // Force the tab control to stay on the previous selection
            // Use PostMessage to set selection after current message processing completes
            ::PostMessage(GetTabCtrl().m_hWnd, TCM_SETCURSEL, nmhdr.iItem1, 0);
        }
        
        // Only create a new filter view if we're not initializing and not in programmatic change
        if (!m_bInitializing && !m_bProgrammaticTabChange)
        {
            // User actually clicked the "+" tab - show the dialog
            // Call directly instead of PostMessage to avoid timing issues
            AddFilterView();
        }
        
        // Reset cursor immediately
        ::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
        
        return 1;  // Return non-zero to indicate we handled it and cancel the selection
    }
    
    // Normal tab selection - let it proceed
    SetMsgHandled(win32::False);

    if (nmhdr.iItem2 >= 0 && nmhdr.iItem2 < actualViewCount)
    {
        SetModifiedMark(nmhdr.iItem2, false);
        
        // Restore the detail pane content for the newly activated tab
        auto pTabItem = GetTabCtrl().GetItem(nmhdr.iItem2);
        if (pTabItem)
        {
            pTabItem->RestoreDetailContent();
        }
        
        // Update the procedure view to use the active view's tracker
        UpdateProcedureViewTracker();
        
        // Update UI to reflect the detail pane state of the new tab
        UpdateUI();
    }

    if (!m_linkViews || nmhdr.iItem1 == nmhdr.iItem2 || nmhdr.iItem1 < 0 || nmhdr.iItem1 >= actualViewCount || nmhdr.iItem2 < 0 ||
        nmhdr.iItem2 >= actualViewCount)
    {
        return 0;
    }

    int line = GetView(nmhdr.iItem1).GetFocusLine();
    GetView(nmhdr.iItem2).SetFocusLine(line);

    return 0;
}

void CMainFrame::SetModifiedMark(int tabindex, bool modified)
{
    auto name = GetView(tabindex).GetName();
    if (modified)
    {
        name += L"*";
    }

    //    GetTabCtrl().GetItem(nmhdr.iItem2)->SetHighlighted(modified)
    auto pTabItem = GetTabCtrl().GetItem(tabindex);
    if (pTabItem)
    {
        pTabItem->SetText(name.c_str());
    }
}

LRESULT CMainFrame::OnCloseTab(NMHDR* pnmh)
{
    auto& nmhdr = *reinterpret_cast<NMCTCITEM*>(pnmh);
    
    // Prevent closing the "+" tab (always the last tab)
    if (nmhdr.iItem >= GetActualViewCount())
    {
        // This is the "+" tab - don't allow closing it
        return 0;
    }

    // Keep at least one view alive
    if (GetActualViewCount() <= 1)
    {
        return 0;
    }

    CloseView(nmhdr.iItem);
    return 0;
}

LRESULT CMainFrame::OnDeleteTab(NMHDR* pnmh)
{
    // This handler is called when a tab is being deleted
    // At this point, the tab item might already be in an invalid state
    // Return FALSE to allow the default processing to continue
    return FALSE;
}

void CMainFrame::OnFileNewTab(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    AddFilterView();
    
    // Note: "+" tab is already present, no need to re-add it
}

void CMainFrame::SaveLogFile(const std::wstring& filename)
{
    UISetText(0, WStr(wstringbuilder() << "Saving " << filename));
    Win32::ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));

    std::ofstream fs;
    OpenLogFile(fs, filename);
    int count = m_logFile.Count();
    for (int i = 0; i < count; ++i)
    {
        auto msg = m_logFile[i];
        WriteLogFileMessage(fs, msg.time, msg.systemTime, msg.processId, msg.processName, msg.text);
    }
    fs.close();
    if (!fs)
    {
        Win32::ThrowLastError(filename);
    }

    m_logFileName = filename;
    UpdateStatusBar();
}

void CMainFrame::SaveViewFile(const std::wstring& filename)
{
    UISetText(0, WStr(wstringbuilder() << "Saving view to " << filename));
    Win32::ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));
    GetView().Save(filename);
    m_txtFileName = filename;
    UpdateStatusBar();
}

void CMainFrame::SaveViewSelection(const std::wstring& filename)
{
    UISetText(0, WStr(wstringbuilder() << "Saving selection to " << filename));
    Win32::ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));
    GetView().SaveSelection(filename);
    m_txtFileName = filename;
    UpdateStatusBar();
}
struct View
{
    int index;
    std::string name;
    bool clockTime;
    bool processColors;
    LogFilter filters;
    boost::optional<boost::property_tree::ptree> columnsPt;
};

struct SourceInfoHelper
{
    SourceInfoHelper(int index, const std::wstring& description, SourceType::type sourceType) :
        index(index),
        sourceInfo(description, sourceType)
    {
    }

    int index;
    SourceInfo sourceInfo;
};

void CMainFrame::LoadConfiguration(const std::wstring& fileName)
{
    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(Str(fileName), pt, boost::property_tree::xml_parser::trim_whitespace);

    auto autoNewline = pt.get<bool>("DebugViewPP.AutoNewline");
    auto linkViews = pt.get<bool>("DebugViewPP.LinkViews");

    auto viewsPt = pt.get_child("DebugViewPP.Views");
    std::vector<View> views;
    for (auto& item : viewsPt)
    {
        if (item.first == "View")
        {
            View view;
            auto& viewPt = item.second;
            view.index = viewPt.get<int>("Index");
            view.name = viewPt.get<std::string>("Name");
            view.clockTime = viewPt.get<bool>("ClockTime");
            view.processColors = viewPt.get<bool>("ProcessColors");
            view.filters.messageFilters = MakeFilters(viewPt.get_child("MessageFilters"));
            view.filters.processFilters = MakeFilters(viewPt.get_child("ProcessFilters"));
            
            // Load prefix filters if they exist (for backward compatibility)
            auto prefixFiltersNode = viewPt.get_child_optional("PrefixFilters");
            if (prefixFiltersNode)
            {
                view.filters.prefixFilters = MakeFilters(*prefixFiltersNode);
            }
            view.columnsPt = viewPt.get_child_optional("Columns");
            views.push_back(view);
        }
    }
    std::sort(views.begin(), views.end(), [](const View& v1, const View& v2) { return v1.index < v2.index; });

    m_logSources.SetAutoNewLine(autoNewline);
    m_linkViews = linkViews;

    for (int i = 0; i < static_cast<int>(views.size()); ++i)
    {
        if (i >= GetActualViewCount())
        {
            AddFilterView(WStr(views[i].name), views[i].filters);
        }
        else
        {
            GetView(i).SetFilters(views[i].filters);
        }

        auto& logView = GetView(i);
        logView.SetClockTime(views[i].clockTime);
        logView.SetViewProcessColors(views[i].processColors);
        if (views[i].columnsPt)
        {
            logView.ReadColumns(*(views[i].columnsPt));
        }
    }

    int i = GetActualViewCount();
    auto size = static_cast<int>(views.size());
    while (i > size)
    {
        --i;
        CloseView(i);
    }

    std::vector<SourceInfo> sourceInfos;
    auto sourcesPt = pt.get_child_optional("DebugViewPP.Sources");
    if (sourcesPt)
    {
        std::vector<SourceInfoHelper> sources;
        for (const auto& item : *sourcesPt)
        {
            if (item.first == "Source")
            {
                auto& sourcePt = item.second;
                int index = sourcePt.get<int>("Index");
                SourceType::type type = StringToSourceType(sourcePt.get<std::string>("SourceType"));
                std::wstring description = WStr(sourcePt.get<std::string>("Description"));
                SourceInfoHelper helper(index, description, type);
                helper.sourceInfo.address = WStr(sourcePt.get<std::string>("Address")).str();
                helper.sourceInfo.port = sourcePt.get<int>("Port");
                helper.sourceInfo.enabled = sourcePt.get<bool>("Enabled");
                sources.push_back(helper);
            }
        }

        std::sort(sources.begin(), sources.end(), [](const SourceInfoHelper& si1, const SourceInfoHelper& si2) { return si1.index < si2.index; });

        for (const auto& helper : sources)
        {
            sourceInfos.push_back(helper.sourceInfo);
        }
    }
    UpdateLogSources(sourceInfos);
    m_sourceInfos = sourceInfos;
    
    // Add the "+" tab as the last tab after loading configuration
    AddPlusTab();
}

void CMainFrame::SaveConfiguration(const std::wstring& fileName)
{
#if BOOST_VERSION < 105600
    boost::property_tree::xml_writer_settings<char> settings('\t', 1);
#else
    boost::property_tree::xml_writer_settings<std::string> settings('\t', 1);
#endif

    boost::property_tree::ptree mainPt;
    mainPt.put("AutoNewline", m_logSources.GetAutoNewLine());
    mainPt.put("LinkViews", m_linkViews);

    int views = GetActualViewCount();
    for (int i = 0; i < views; ++i)
    {
        auto& logView = GetView(i);
        auto filters = logView.GetFilters();
        boost::property_tree::ptree viewPt;
        viewPt.put("Index", i);
        viewPt.put("Name", Str(logView.GetName()).str());
        viewPt.put("ClockTime", logView.GetClockTime());
        viewPt.put("ProcessColors", logView.GetViewProcessColors());
        viewPt.put_child("MessageFilters", MakePTree(filters.messageFilters));
        viewPt.put_child("ProcessFilters", MakePTree(filters.processFilters));
        viewPt.put_child("PrefixFilters", MakePTree(filters.prefixFilters));
        viewPt.put_child("Columns", MakePTree(logView.GetColumns()));
        mainPt.add_child("Views.View", viewPt);
    }

    for (int i = 0; i < static_cast<int>(m_sourceInfos.size()); ++i)
    {
        const auto& sourceInfo = m_sourceInfos[i];
        boost::property_tree::ptree sourcePt;
        sourcePt.put("Index", i);
        sourcePt.put("Enabled", sourceInfo.enabled);
        sourcePt.put("Description", Str(sourceInfo.description).str());
        sourcePt.put("SourceType", SourceTypeToString(sourceInfo.type));
        sourcePt.put("Address", Str(sourceInfo.address).str());
        sourcePt.put("Port", sourceInfo.port);
        mainPt.add_child("Sources.Source", sourcePt);
    }

    boost::property_tree::ptree pt;
    pt.add_child("DebugViewPP", mainPt);

    boost::property_tree::write_xml(Str(fileName), pt, std::locale(), settings);
}

void CMainFrame::OnFileOpen(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CFileOptionDlg dlg(1, L"Keep file open", L".dblog", m_logFileName.c_str(), OFN_FILEMUSTEXIST,
        L"DebugView++ Log Files (*.dblog)\0*.dblog\0"
        L"DebugView Log Files (*.log)\0*.log\0"
        L"All Files (*.*)\0*.*\0\0",
        nullptr);
    dlg.m_ofn.nFilterIndex = 0;
    dlg.m_ofn.lpstrTitle = L"Load Log File";
    if (dlg.DoModal() == IDOK)
    {
        Load(std::wstring(dlg.m_szFileName), dlg.Option());
    }
}

void CMainFrame::Run(const std::wstring& pathName)
{
    if (!pathName.empty())
    {
        m_runDlg.SetPathName(pathName);
    }

    if (m_runDlg.DoModal() == IDOK)
    {
        m_logSources.AddProcessReader(m_runDlg.GetPathName(), m_runDlg.GetArguments());
    }
}

void CMainFrame::OnFileRun(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    Run();
}

void CMainFrame::Load(const std::wstring& filename, bool keeptailing)
{
    SetTitle(filename);
    ClearLog();
    m_logSources.AddAnyFileReader(WStr(std::filesystem::path(filename).filename().string()), keeptailing);
}

void CMainFrame::SetTitle(const std::wstring& title)
{
    std::wstring windowText = title.empty() ? m_applicationName : L"[" + title + L"] - " + m_applicationName;
    SetWindowText(windowText.c_str());
}

void CMainFrame::UpdateProcedureViewTracker()
{
    // Update the procedure tabs in the detail pane instead of the separate window
    if (GetViewCount() > 0)
    {
        int currentTab = GetTabCtrl().GetCurSel();
        if (currentTab >= 0 && currentTab < GetViewCount())
        {
            // Get the active tab item
            auto pTabItem = dynamic_cast<CLogViewTabItem2*>(GetTabCtrl().GetItem(currentTab));
            if (pTabItem)
            {
                CLogView& activeView = GetView();
                
                // Update the procedure tabs in this tab item
                pTabItem->UpdateProcedureTabs();
                
                // Also update any floating windows to use the new tracker
                pTabItem->UpdateFloatingWindowTrackers();
            }
        }
    }
    
    // Legacy: Also update the separate procedure view window if it exists (for backward compatibility)
    // TODO: Remove this once we're confident the integration works
    if (m_wndProcedureView.IsWindow())
    {
        if (GetViewCount() > 0)
        {
            CLogView& activeView = GetView();
            auto& tracker = activeView.GetProcedureTracker();
            m_wndProcedureView.SetTracker(&tracker);
        }
        else
        {
            m_wndProcedureView.SetTracker(nullptr);
        }
    }
}

void CMainFrame::Load(HANDLE hFile)
{
    hstream file(hFile);
    FILETIME ft = Win32::GetSystemTimeAsFileTime();
    Load(file, "", ft);
}

void CMainFrame::Load(std::istream& file, const std::string& name, FILETIME fileTime)
{
    Win32::ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));

    ClearLog();

    Line line(0.0);
    line.processName = name;
    line.systemTime = fileTime;
    while (ReadLogFileMessage(file, line))
    {
        AddMessage(Message(line.time, line.systemTime, line.pid, line.processName, line.message));
    }
}

void CMainFrame::SetSelectTabByName(const std::wstring& tabName)
{
    const int views = GetActualViewCount();
    for (int i = 0; i < views; ++i)
    {
        if (GetView(i).GetName() == tabName)
        {
            GetTabCtrl().SetCurSel(i);
            break;
        }
    }
}

void CMainFrame::CapturePipe(HANDLE hPipe)
{
    m_logSources.AddPipeReader(Win32::GetParentProcessId(), hPipe);
}

void CMainFrame::OnFileExit(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    PostMessage(WM_CLOSE);
}

void CMainFrame::OnFileSaveLog(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CFileDialog dlg(0, L".dblog", m_logFileName.c_str(), OFN_OVERWRITEPROMPT,
        L"DebugView++ Log Files (*.dblog)\0*.dblog\0"
        L"All Files (*.*)\0*.*\0\0",
        nullptr);
    dlg.m_ofn.nFilterIndex = 0;
    dlg.m_ofn.lpstrTitle = L"Save all messages in memory buffer";
    if (dlg.DoModal() == IDOK)
    {
        SaveLogFile(dlg.m_szFileName);
    }
}

void CMainFrame::OnFileSaveView(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CFileDialog dlg(0, L".dblog", m_txtFileName.c_str(), OFN_OVERWRITEPROMPT,
        L"DebugView++ Log Files (*.dblog)\0*.dblog\0"
        L"All Files (*.*)\0*.*\0\0");
    dlg.m_ofn.nFilterIndex = 0;
    dlg.m_ofn.lpstrTitle = L"Save the messages in the current view";
    if (dlg.DoModal() == IDOK)
    {
        SaveViewFile(dlg.m_szFileName);
    }
}

void CMainFrame::OnFileSaveViewSelection(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CFileDialog dlg(0, L".dblog", m_txtFileName.c_str(), OFN_OVERWRITEPROMPT,
        L"DebugView++ Log Files (*.dblog)\0*.dblog\0"
        L"All Files (*.*)\0*.*\0\0");
    dlg.m_ofn.nFilterIndex = 0;
    dlg.m_ofn.lpstrTitle = L"Save the messages selected in the current view";
    if (dlg.DoModal() == IDOK)
    {
        SaveViewSelection(dlg.m_szFileName);
    }
}

void CMainFrame::OnFileLoadConfiguration(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CFileDialog dlg(1, L".dbconf", m_configFileName.c_str(), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
        L"DebugView++ Configuration Files (*.dbconf)\0*.dbconf\0\0");
    dlg.m_ofn.nFilterIndex = 0;
    dlg.m_ofn.lpstrTitle = L"Load View Configuration";
    if (dlg.DoModal() == IDOK)
    {
        LoadConfiguration(dlg.m_szFileName);
    }
}

void CMainFrame::OnFileSaveConfiguration(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CFileDialog dlg(
        0, L".dbconf", m_configFileName.c_str(), OFN_OVERWRITEPROMPT, L"DebugView++ Configuration Files (*.dbconf)\0*.dbconf\0\0");
    dlg.m_ofn.nFilterIndex = 0;
    dlg.m_ofn.lpstrTitle = L"Save View Configuration";
    if (dlg.DoModal() == IDOK)
    {
        SaveConfiguration(dlg.m_szFileName);
    }
}

void CMainFrame::OnFilePreferences(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    FILE* log = fopen("C:\\Dev\\UltimateDebugViewPP\\prefs_debug.log", "a");
    if (log) {
        fprintf(log, "\n=== OnFilePreferences called ===\n");
        fprintf(log, "Current m_newViewDefaults.sqlKeywordCase: %d\n", (int)m_newViewDefaults.sqlKeywordCase);
        fclose(log);
    }

    CPreferencesDlg dlg(m_newViewDefaults, m_mcpSettings, GetMcpStatusText());
    if (dlg.DoModal(m_hWnd) == IDOK)
    {
        m_newViewDefaults = dlg.GetDefaults();
        if (dlg.GetMcpSettings() != m_mcpSettings)
        {
            ApplyMcpSettings(dlg.GetMcpSettings());
        }
        if (log = fopen("C:\\Dev\\UltimateDebugViewPP\\prefs_debug.log", "a")) {
            fprintf(log, "After OK, m_newViewDefaults.sqlKeywordCase: %d\n", (int)m_newViewDefaults.sqlKeywordCase);
            fclose(log);
        }
        SaveNewViewDefaults();

        // Apply SQL formatting preferences to existing views by refreshing detail views
        // This ensures that if a detail view is showing SQL, it will be reformatted with new preferences
        int views = GetActualViewCount();
        for (int i = 0; i < views; ++i)
        {
            auto* pTabItem = static_cast<CLogViewTabItem2*>(GetTabCtrl().GetItem(i));
            if (pTabItem)
            {
                // Refresh the detail view to apply new SQL formatting options
                pTabItem->RefreshDetailView();
            }
        }
    }
}

void CMainFrame::ClearLog()
{
    // Keep MCP cursors monotonic across clears.
    m_seqBase += m_logFile.Count();
    ++m_clearGeneration;
    m_logFile.Clear();
    m_logSources.ResetTimer();

    // Clear all view trackers (each view has its own tracker now)
    int views = GetActualViewCount();
    for (int i = 0; i < views; ++i)
    {
        GetView(i).Clear();  // This will also clear the view's procedure tracker
        SetModifiedMark(i, false);

        // Clear the detail tabs (Details, Procedure Flow, Procedure Stats) for this view
        auto* pTabItem = static_cast<CLogViewTabItem2*>(GetTabCtrl().GetItem(i));
        if (pTabItem)
        {
            // Clear the Details tab
            pTabItem->UpdateDetailView(L"", L"", L"");

            // Update procedure tabs - they will show empty since tracker was cleared
            pTabItem->UpdateProcedureTabs();
        }

        GetTabCtrl().UpdateLayout();
        GetTabCtrl().Invalidate();
    }

    // Update the procedure view to reflect the cleared state
    if (m_wndProcedureView.IsWindow())
    {
        m_wndProcedureView.UpdateProcedureTree();
    }

    // Also update the procedure tabs in the detail pane
    UpdateProcedureViewTracker();

    UpdateStatusBar();
}

void CMainFrame::OnLogClear(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    ClearLog();
}

void CMainFrame::OnLogCrop(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    auto selection = GetView().GetSelectedRange();
    if (selection.count < 2)
    {
        return;
    }

    LogFile temp;
    temp.Append(m_logFile, selection.beginLine, selection.endLine);
    // Cropping renumbers the log; keep MCP cursors monotonic like ClearLog does.
    m_seqBase += m_logFile.Count();
    ++m_clearGeneration;
    std::swap(temp, m_logFile);

    m_logSources.ResetTimer();
    int views = GetActualViewCount();
    for (int i = 0; i < views; ++i)
    {
        auto& view = GetView(i);
        view.Clear();
        view.ResetToLine(0);
        SetModifiedMark(i, false);
        GetTabCtrl().UpdateLayout();
        GetTabCtrl().Invalidate();
    }
    UpdateStatusBar();
}

void CMainFrame::OnLinkViews(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_linkViews = !m_linkViews;
}

void CMainFrame::OnAutoNewline(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_logSources.SetAutoNewLine(!m_logSources.GetAutoNewLine());
}

void CMainFrame::OnProcessPrefix(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_logSources.SetProcessPrefix(!m_logSources.GetProcessPrefix());
}

void CMainFrame::OnHide(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_hide = !m_hide;
}

void CMainFrame::ApplyExpandedToolbarState()
{
    // Get the rebar control
    CReBarCtrl rebar(m_hWndToolBar);
    
    // Find and save the Find dialog handle if it exists
    HWND hWndFindDialog = NULL;
    int findDialogBandIndex = -1;
    
    // Remove ALL toolbar bands except the command bar (band 0)
    // We'll iterate through bands and remove any ToolbarWindow32 that isn't band 0
    // Must iterate backwards to avoid index shifting issues
    int bandCount = rebar.GetBandCount();
    for (int i = bandCount - 1; i > 0; i--)  // Start from last band, stop before band 0
    {
        REBARBANDINFO rbbi = {0};
        rbbi.cbSize = sizeof(REBARBANDINFO);
        rbbi.fMask = RBBIM_CHILD;
        
        if (rebar.GetBandInfo(i, &rbbi) && rbbi.hwndChild != NULL)
        {
            wchar_t className[256] = {0};
            ::GetClassName(rbbi.hwndChild, className, 256);
            
            // Check if this is the Find dialog
            if (rbbi.hwndChild == m_findDlg.m_hWnd)
            {
                hWndFindDialog = rbbi.hwndChild;
                findDialogBandIndex = i;
                // Don't delete the Find dialog, just remember it
            }
            // Remove if it's a toolbar (ToolbarWindow32 class)
            else if (wcscmp(className, L"ToolbarWindow32") == 0)
            {
                ::DestroyWindow(rbbi.hwndChild);
                rebar.DeleteBand(i);
            }
        }
    }
    
    // Now create the appropriate toolbar
    HWND hWndNewToolBar = NULL;
    
    if (m_expandedToolbar)
    {
        // Create expanded toolbar with text labels
        hWndNewToolBar = CreateSimpleToolBarCtrl(rebar, IDR_MAINFRAME, FALSE, 
            ATL_SIMPLE_TOOLBAR_PANE_STYLE | TBSTYLE_LIST);
        
        // Set extended style for mixed buttons (icon + text)
        CToolBarCtrl toolbar(hWndNewToolBar);
        toolbar.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS);
        
        // Get all buttons and set text for each
        int nButtonCount = toolbar.GetButtonCount();
        for (int i = 0; i < nButtonCount; i++)
        {
            TBBUTTON tb = {0};
            toolbar.GetButton(i, &tb);
            
            if (tb.fsStyle & BTNS_SEP)
                continue;  // Skip separators
            
            TBBUTTONINFO tbbi = {0};
            tbbi.cbSize = sizeof(TBBUTTONINFO);
            tbbi.dwMask = TBIF_STYLE | TBIF_TEXT;
            tbbi.fsStyle = tb.fsStyle | BTNS_SHOWTEXT | BTNS_AUTOSIZE;
            
            // Set text based on command ID
            switch (tb.idCommand)
            {
            case ID_FILE_OPEN:
                tbbi.pszText = const_cast<LPWSTR>(L"Open");
                break;
            case ID_FILE_SAVE_LOG:
                tbbi.pszText = const_cast<LPWSTR>(L"Save Log");
                break;
            case ID_LOG_CLEAR:
                tbbi.pszText = const_cast<LPWSTR>(L"Clear");
                break;
            case ID_LOG_PAUSE:
                tbbi.pszText = const_cast<LPWSTR>(L"Pause");
                break;
            case ID_LOG_GLOBAL:
                tbbi.pszText = const_cast<LPWSTR>(L"Global");
                break;
            case ID_VIEW_FILTER:
                tbbi.pszText = const_cast<LPWSTR>(L"Filter");
                break;
            case ID_VIEW_FIND:
                tbbi.pszText = const_cast<LPWSTR>(L"Find");
                break;
            case ID_VIEW_COPY:
                tbbi.pszText = const_cast<LPWSTR>(L"Copy");
                break;
            case ID_VIEW_SCROLL:
                tbbi.pszText = const_cast<LPWSTR>(L"Auto Scroll");
                break;
            case ID_VIEW_SCROLL_STOP:
                tbbi.pszText = const_cast<LPWSTR>(L"Stop");
                break;
            case ID_VIEW_TIME:
                tbbi.pszText = const_cast<LPWSTR>(L"Clock Time");
                break;
            case ID_OPTIONS_LINKVIEWS:
                tbbi.pszText = const_cast<LPWSTR>(L"Link Views");
                break;
            case ID_VIEW_BOOKMARK:
                tbbi.pszText = const_cast<LPWSTR>(L"Bookmark");
                break;
            case ID_VIEW_PREVIOUS_BOOKMARK:
                tbbi.pszText = const_cast<LPWSTR>(L"Previous");
                break;
            case ID_VIEW_NEXT_BOOKMARK:
                tbbi.pszText = const_cast<LPWSTR>(L"Next");
                break;
            case ID_VIEW_CLEAR_BOOKMARKS:
                tbbi.pszText = const_cast<LPWSTR>(L"Clear");
                break;
            case ID_OPTIONS_FONT:
                tbbi.pszText = const_cast<LPWSTR>(L"Font");
                break;
            default:
                continue;  // Skip unknown buttons
            }
            
            toolbar.SetButtonInfo(tb.idCommand, &tbbi);
        }
        
        // Insert the toolbar band at position 1 (after command bar)
        REBARBANDINFO rbbi = {0};
        rbbi.cbSize = sizeof(REBARBANDINFO);
        rbbi.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
        rbbi.fStyle = RBBS_BREAK | RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS;  // RBBS_BREAK forces new line
        rbbi.hwndChild = hWndNewToolBar;
        
        // Get the ideal size of the toolbar
        SIZE size = {0};
        CToolBarCtrl tb(hWndNewToolBar);
        tb.GetMaxSize(&size);
        rbbi.cxMinChild = 0;
        rbbi.cyMinChild = size.cy;
        rbbi.cx = size.cx;
        
        rebar.InsertBand(1, &rbbi);
        
        // Move Find bar to a new line if it exists
        int newBandCount = rebar.GetBandCount();
        if (newBandCount > 2)  // We have cmd bar + toolbar + find dialog
        {
            REBARBANDINFO rbbi2 = {0};
            rbbi2.cbSize = sizeof(REBARBANDINFO);
            rbbi2.fMask = RBBIM_STYLE;
            rbbi2.fStyle = RBBS_BREAK | RBBS_CHILDEDGE;  // RBBS_BREAK forces new line
            rebar.SetBandInfo(2, &rbbi2);  // Band 2 is the Find bar
        }
    }
    else
    {
        // Create standard compact toolbar
        hWndNewToolBar = CreateSimpleToolBarCtrl(rebar, IDR_MAINFRAME, FALSE, 
            ATL_SIMPLE_TOOLBAR_PANE_STYLE);
        
        // Insert the toolbar band at position 1 (after command bar)
        REBARBANDINFO rbbi = {0};
        rbbi.cbSize = sizeof(REBARBANDINFO);
        rbbi.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
        rbbi.fStyle = RBBS_BREAK | RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS;  // RBBS_BREAK forces new line
        rbbi.hwndChild = hWndNewToolBar;
        
        // Get the ideal size of the toolbar
        SIZE size = {0};
        CToolBarCtrl tb(hWndNewToolBar);
        tb.GetMaxSize(&size);
        rbbi.cxMinChild = 0;
        rbbi.cyMinChild = size.cy;
        rbbi.cx = size.cx;
        
        rebar.InsertBand(1, &rbbi);
        
        // Put Find bar back on same line if it exists
        int newBandCount = rebar.GetBandCount();
        if (newBandCount > 2)  // We have cmd bar + toolbar + find dialog
        {
            REBARBANDINFO rbbi2 = {0};
            rbbi2.cbSize = sizeof(REBARBANDINFO);
            rbbi2.fMask = RBBIM_STYLE;
            rbbi2.fStyle = RBBS_CHILDEDGE;  // Remove RBBS_BREAK to put on same line
            rebar.SetBandInfo(2, &rbbi2);
        }
    }
    
    // Update UI with the actual toolbar handle we just created
    if (hWndNewToolBar != NULL)
    {
        UIAddToolBar(hWndNewToolBar);
    }
    UpdateLayout();
    
    // Update menu checkmark
    UISetCheck(ID_OPTIONS_EXPANDED_TOOLBAR, m_expandedToolbar);
}

void CMainFrame::OnExpandedToolbar(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    // Toggle expanded toolbar state
    m_expandedToolbar = !m_expandedToolbar;
    
    // Apply the new state
    ApplyExpandedToolbarState();
}

void CMainFrame::OnImportSettings(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    ImportDebugViewPPSettings();
}

bool CMainFrame::GetAlwaysOnTop() const
{
    return (GetWindowLong(GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
}

void CMainFrame::SetAlwaysOnTop(bool value)
{
    SetWindowPos(value ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void CMainFrame::SetProcedureStatsSplitterPos(int pct)
{
    m_procedureStatsSplitterPos = ClampProcedureStatsSplitPct(pct);
    if (m_wndProcedureView.IsWindow())
    {
        m_wndProcedureView.SetProcedureStatsSplitterPos(m_procedureStatsSplitterPos);
    }
}

void CMainFrame::OnProcedureSplitterMoved(UINT /*uNotifyCode*/, int /*nID*/, CWindow wndCtl)
{
    // The splitter position is sent in the window handle as LPARAM
    int newPos = (int)(LPARAM)wndCtl.m_hWnd;
    SetProcedureStatsSplitterPos(newPos);

    // Save the position to registry immediately
    CRegKey reg;
    if (reg.Open(HKEY_CURRENT_USER, L"Software\\DebugView++\\Settings") == ERROR_SUCCESS)
    {
        reg.SetDWORDValue(L"ProcedureStatsSplitterPos", static_cast<DWORD>(m_procedureStatsSplitterPos));
    }
}

void CMainFrame::OnAlwaysOnTop(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    SetAlwaysOnTop(!GetAlwaysOnTop());
}

bool CMainFrame::IsPaused() const
{
    return m_pLocalReader == nullptr;
}

void CMainFrame::Pause()
{
    SetTitle(L"Paused");
    if (m_pLocalReader != nullptr)
    {
        m_logSources.Remove(m_pLocalReader);
        m_pLocalReader = nullptr;
    }
    if (m_pGlobalReader != nullptr)
    {
        m_logSources.Remove(m_pGlobalReader);
        m_pGlobalReader = nullptr;
    }
    if (m_pKernelReader != nullptr)
    {
        m_logSources.Remove(m_pKernelReader);
        m_pKernelReader = nullptr;
    }
    m_logSources.AddMessage("<paused>");
}

void CMainFrame::UpdateTitle()
{
    SetTitle();
}

void CMainFrame::Resume()
{
    SetTitle();

    if (m_pLocalReader == nullptr)
    {
        try
        {
            m_pLocalReader = m_logSources.AddDBWinReader(false);
        }
        catch (std::exception&)
        {
            MessageBox(L"Unable to capture Win32 Messages.\n"
                       L"\n"
                       L"Another DebugView++ (or similar application) might be running.\n",
                m_applicationName.c_str(), MB_ICONERROR | MB_OK);
            return;
        }
    }

    if (m_tryGlobal && m_pGlobalReader == nullptr)
    {
        try
        {
            m_pGlobalReader = m_logSources.AddDBWinReader(true);
        }
        catch (std::exception&)
        {
            MessageBox(L"Unable to capture Global Win32 Messages.\n"
                       L"\n"
                       L"Make sure you have appropriate permissions.\n"
                       L"\n"
                       L"You may need to start this application by right-clicking it and selecting\n"
                       L"'Run As Administator' even if you have administrator rights.",
                m_applicationName.c_str(), MB_ICONERROR | MB_OK);
            m_tryGlobal = false;
        }
    }

    if (m_tryKernel && m_pKernelReader == nullptr)
    {
        try
        {
            m_pKernelReader = m_logSources.AddKernelReader();
        }
        catch (std::exception& e)
        {
            const auto message = std::format("Unable to capture Kernel Messages.\n"
                                             "({})\n\n"
                                             "Make sure you have appropriate permissions.\n"
                                             "\n"
                                             "You may need to start this application by right-clicking it and selecting\n"
                                             "'Run As Administator' even if you have administrator rights.",
                e.what());
            MessageBox(WStr(message), m_applicationName.c_str(), MB_ICONERROR | MB_OK);
            m_tryKernel = false;
        }
    }
    UpdateTitle();
}

void CMainFrame::OnLogPause(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    if (IsPaused())
    {
        Resume();
    }
    else
    {
        Pause();
    }
}

void CMainFrame::OnLogGlobal(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_tryGlobal = (m_pGlobalReader == nullptr);

    if (m_tryGlobal)
    {
        Resume();
    }
    else
    {
        m_logSources.Remove(m_pGlobalReader);
        m_pGlobalReader = nullptr;
    }
    UpdateTitle();
}

void CMainFrame::WriteDriverFromResource()
{
    if (std::filesystem::exists(m_driverLocation))
    {
        return;
    }
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_DBGV_DRIVER), RT_RCDATA);
    if (hRes)
    {
        HGLOBAL hLoadedRes = LoadResource(NULL, hRes);
        if (hLoadedRes)
        {
            DWORD dwSize = SizeofResource(NULL, hRes);
            void* pLockedRes = LockResource(hLoadedRes);
            if (pLockedRes)
            {
                std::ofstream outFile(m_driverLocation.c_str(), std::ios::binary);
                outFile.write(static_cast<const char*>(pLockedRes), dwSize);
                outFile.close();
            }
        }
    }
}

void CMainFrame::RemoveDriver()
{
    // can fail silently
    std::remove(m_driverLocation.c_str());
}

void CMainFrame::OnLogKernel(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_tryKernel = (m_pKernelReader == nullptr);

    if (m_tryKernel)
    {
        WriteDriverFromResource();
        Resume();
    }
    else
    {
        m_logSources.Remove(m_pKernelReader);
        m_pKernelReader = nullptr;
        RemoveDriver();
    }
    UpdateTitle();
}

void CMainFrame::OnLogKernelVerbose(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_verboseKernelMessage = !m_verboseKernelMessage;
    if (m_pKernelReader)
    {
        m_pKernelReader->SetVerbose(m_verboseKernelMessage);
    }
}

void CMainFrame::OnLogKernelPassThrough(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_passthroughMode = !m_passthroughMode;
    if (m_pKernelReader)
    {
        m_pKernelReader->SetPassThrough(m_passthroughMode);
    }
}

void CMainFrame::OnLogHistory(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CHistoryDlg dlg(m_logFile.GetHistorySize(), m_logFile.GetHistorySize() == 0);
    if (dlg.DoModal() == IDOK)
    {
        m_logFile.SetHistorySize(dlg.GetHistorySize());
    }
}

std::wstring GetExecutionPath()
{
    auto path = std::filesystem::absolute(Win32::GetModuleFilename());
    return path.remove_filename().c_str();
}

void CMainFrame::OnLogDebugviewAgent(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    if (m_pDbgviewReader == nullptr)
    {
        std::string dbgview = stringbuilder() << Win32::GetExecutionPath() << "\\dbgview.exe";
        if (std::filesystem::exists(dbgview.c_str()))
        {
            std::string cmd = stringbuilder() << "start \"\" " << dbgview << " /a";
            system(cmd.c_str());
        }
        else
        {
            m_logSources.AddMessage("dbgview.exe not found");
        }
        m_pDbgviewReader = m_logSources.AddDbgviewReader("127.0.0.1");
    }
    else
    {
        m_logSources.Remove(m_pDbgviewReader);
        m_pDbgviewReader = nullptr;
    }
}

void CMainFrame::OnViewFilter(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    // Get all default prefix filters (system + custom)
    std::vector<Filter> defaultPrefixFilters;
    defaultPrefixFilters.insert(defaultPrefixFilters.end(), 
        m_newViewDefaults.systemPrefixFilters.begin(), 
        m_newViewDefaults.systemPrefixFilters.end());
    defaultPrefixFilters.insert(defaultPrefixFilters.end(), 
        m_newViewDefaults.customPrefixFilters.begin(), 
        m_newViewDefaults.customPrefixFilters.end());
    
    CFilterDlg dlg(GetView().GetName(), GetView().GetFilters(), &m_logFile, defaultPrefixFilters, false, GetView().GetUniqueProcedures());
    if (dlg.DoModal() != IDOK)
    {
        return;
    }

    int tabIdx = GetTabCtrl().GetCurSel();
    auto pTabItem = GetTabCtrl().GetItem(tabIdx);
    if (pTabItem)
    {
        pTabItem->SetText(dlg.GetName().c_str());
    }
    GetTabCtrl().UpdateLayout();
    GetTabCtrl().Invalidate();
    GetView().SetName(dlg.GetName());
    GetView().SetFilters(dlg.GetFilters());
    SaveSettings();
}

void CMainFrame::OnViewDetailPane(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    // Get the current tab
    int tabIndex = std::max(0, GetTabCtrl().GetCurSel());
    if (tabIndex < GetTabCtrl().GetItemCount())
    {
        auto pTabItem = GetTabCtrl().GetItem(tabIndex);
        if (pTabItem)
        {
            // Toggle detail pane visibility
            bool isVisible = pTabItem->IsDetailPaneVisible();
            pTabItem->ShowDetailPane(!isVisible);
            
            // Update UI check state
            UpdateUI();
        }
    }
}

void CMainFrame::OnViewProcedureTracker(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    // NOTE: Procedure tracking is now integrated into the detail pane tabs.
    // This functionality has been moved to the "Procedure Flow" and "Procedure Stats" tabs
    // in the detail pane. Users can access it by enabling the detail pane and switching to those tabs.
    
    // For now, we'll redirect this to show the detail pane instead
    // Toggle detail pane visibility for the active tab
    if (GetViewCount() > 0)
    {
        int currentTab = GetTabCtrl().GetCurSel();
        if (currentTab >= 0 && currentTab < GetViewCount())
        {
            auto pTabItem = dynamic_cast<CLogViewTabItem2*>(GetTabCtrl().GetItem(currentTab));
            if (pTabItem)
            {
                bool isDetailVisible = pTabItem->IsDetailPaneVisible();
                pTabItem->ShowDetailPane(!isDetailVisible);
                
                if (!isDetailVisible) // If we're showing the detail pane
                {
                    // Switch to the Procedure Flow tab (tab index 1)
                    auto& tabContainer = pTabItem->GetDetailTabContainer();
                    tabContainer.SetActiveTab(1); // Procedure Flow tab
                }
                
                UpdateUI();
            }
        }
    }
    
    /* LEGACY CODE - Separate procedure window functionality (now disabled)
    // Toggle procedure pane visibility
    m_bProcedurePaneVisible = !m_bProcedurePaneVisible;
    
    if (m_bProcedurePaneVisible)
    {
        // Create the procedure window if it doesn't exist
        if (!m_wndProcedureView.IsWindow())
        {
            // Create as a popup window with frame and caption
            DWORD dwStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX | WS_VISIBLE;
            DWORD dwExStyle = WS_EX_TOOLWINDOW;
            m_wndProcedureView.Create(m_hWnd, CWindow::rcDefault, _T("Procedure Tracker"), 
                                      dwStyle, dwExStyle);
            m_wndProcedureView.SetParent(m_hWnd);  // Set parent but keep as popup
        }
        
        // Position and show the window
        m_wndProcedureView.ShowWindow(SW_SHOW);
        
        // Update the tracker for the current active view
        UpdateProcedureViewTracker();
    }
    else
    {
        // Hide the procedure window
        if (m_wndProcedureView.IsWindow())
        {
            m_wndProcedureView.ShowWindow(SW_HIDE);
        }
    }
    
    // Update UI check state
    UISetCheck(ID_VIEW_PROCEDURE_TRACKER, m_bProcedurePaneVisible);
    */
}

void CMainFrame::OnTabPopOut(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    // Pop out the active tab in the detail pane for the current view
    if (GetViewCount() > 0)
    {
        int currentTab = GetTabCtrl().GetCurSel();
        if (currentTab >= 0 && currentTab < GetViewCount())
        {
            auto pTabItem = dynamic_cast<CLogViewTabItem2*>(GetTabCtrl().GetItem(currentTab));
            if (pTabItem && pTabItem->IsDetailPaneVisible())
            {
                auto& tabContainer = pTabItem->GetDetailTabContainer();
                int activeDetailTab = tabContainer.GetActiveTab();
                
                // Don't allow popping out the Details tab (index 0)
                if (activeDetailTab > 0)
                {
                    tabContainer.PopOutTab(activeDetailTab);
                }
            }
        }
    }
}

void CMainFrame::OnTabPopIn(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    // Send pop-in command to all floating windows for the current view
    if (GetViewCount() > 0)
    {
        int currentTab = GetTabCtrl().GetCurSel();
        if (currentTab >= 0 && currentTab < GetViewCount())
        {
            auto pTabItem = dynamic_cast<CLogViewTabItem2*>(GetTabCtrl().GetItem(currentTab));
            if (pTabItem)
            {
                auto& tabContainer = pTabItem->GetDetailTabContainer();
                // The container will handle popping in all floating windows
                ::SendMessage(tabContainer.m_hWnd, WM_COMMAND, ID_TAB_POPIN, 0);
            }
        }
    }
}

void CMainFrame::OnMainTabPopOut(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    // Pop out the current main tab
    int currentTab = GetTabCtrl().GetCurSel();
    if (currentTab >= 0 && currentTab < GetViewCount())
    {
        PopOutMainTab(currentTab);
    }
}

void CMainFrame::OnMainTabPopIn(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    // Pop in all floating main tabs
    while (!m_floatingMainTabs.empty())
    {
        // Close each floating window which will trigger reattachment
        auto& info = m_floatingMainTabs.back();
        if (info.window)
        {
            info.window->PostMessage(WM_CLOSE);
        }
        m_floatingMainTabs.pop_back();
    }
}

void CMainFrame::PopOutMainTab(int index)
{
    if (index < 0 || index >= GetActualViewCount())
        return;
    
    // Don't pop out if already popped out
    if (IsMainTabPoppedOut(index))
        return;
    
    // Don't pop out if this is the last view
    if (GetActualViewCount() <= 1)
        return;
    
    // Get the tab item
    auto pTabItem = dynamic_cast<CLogViewTabItem2*>(GetTabCtrl().GetItem(index));
    if (!pTabItem)
        return;
    
    // Create floating window
    auto floatingWindow = std::make_unique<CFloatingMainTabWindow>(this, pTabItem, index);
    
    // Create and show the floating window
    RECT rcFloat = {100, 100, 900, 700}; // Default size
    floatingWindow->Create(nullptr, rcFloat, nullptr, 
                          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          WS_EX_TOOLWINDOW);
    
    floatingWindow->ShowWindow(SW_SHOW);
    floatingWindow->RefreshTheme();
    
    // Store floating window info
    FloatingMainTabInfo info;
    info.window = std::move(floatingWindow);
    info.tabItem = pTabItem;
    info.originalIndex = index;
    info.tabName = pTabItem->GetTitle();
    m_floatingMainTabs.push_back(std::move(info));
    
    // Remove the tab from the main window (but don't delete the object)
    // Note: We set the second parameter to false to prevent deletion
    GetTabCtrl().DeleteItem(index, false);
    
    // If we have other tabs, select one
    if (GetViewCount() > 0)
    {
        // Select the previous tab if possible, otherwise the first
        int newIndex = (index > 0) ? index - 1 : 0;
        GetTabCtrl().SetCurSel(newIndex);
    }
}

void CMainFrame::ReattachMainTab(CFloatingMainTabWindow* floatingWindow)
{
    if (!floatingWindow)
        return;
    
    // Find and remove from floating tabs
    auto it = std::find_if(m_floatingMainTabs.begin(), m_floatingMainTabs.end(),
        [floatingWindow](const FloatingMainTabInfo& info) {
            return info.window.get() == floatingWindow;
        });
    
    if (it != m_floatingMainTabs.end())
    {
        int originalIndex = it->originalIndex;
        CLogViewTabItem2* tabItem = it->tabItem;
        
        if (tabItem && originalIndex >= 0 && originalIndex < GetTabCtrl().GetItemCount())
        {
            // Reparent the tab content back to main window
            HWND hTabContent = tabItem->GetLogViewParent();
            if (hTabContent && ::IsWindow(hTabContent))
            {
                // First, reparent the content back to its original parent
                // Note: The tab item needs to be re-created/re-added to the tab control
                tabItem->Create(*this);
                
                // Re-insert the tab at its original position (or at the end if position is invalid)
                int insertIndex = originalIndex;
                if (insertIndex < 0 || insertIndex > GetTabCtrl().GetItemCount())
                {
                    insertIndex = GetTabCtrl().GetItemCount();
                }
                
                // Re-insert the tab item (the tab control takes ownership again)
                GetTabCtrl().InsertItem(insertIndex, tabItem);
                
                // Make it the active tab
                GetTabCtrl().SetCurSel(insertIndex);
                
                // Force resize to ensure proper layout
                RECT rcClient;
                GetClientRect(&rcClient);
                ::SetWindowPos(hTabContent, nullptr, 0, 0,
                              rcClient.right - rcClient.left,
                              rcClient.bottom - rcClient.top,
                              SWP_NOZORDER | SWP_SHOWWINDOW);
                
                // Refresh the view
                tabItem->GetView().Invalidate();
                tabItem->GetView().UpdateWindow();
            }
        }
        
        // Destroy the floating window
        if (it->window)
        {
            it->window->DestroyWindow();
        }
        
        // Remove from tracking
        m_floatingMainTabs.erase(it);
    }
}

bool CMainFrame::IsMainTabPoppedOut(int index) const
{
    return std::any_of(m_floatingMainTabs.begin(), m_floatingMainTabs.end(),
        [index](const FloatingMainTabInfo& info) {
            return info.originalIndex == index;
        });
}

LRESULT CMainFrame::OnFloatingMainWindowClosing(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // Reattach the floating tab
    CFloatingMainTabWindow* floatingWindow = reinterpret_cast<CFloatingMainTabWindow*>(wParam);
    ReattachMainTab(floatingWindow);
    return 0;
}

void CMainFrame::OnViewDarkMode(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    auto& themeManager = ThemeManager::Instance();
    
    // Toggle between light and dark (if system theme, switch to explicit dark)
    Theme newTheme = themeManager.IsDarkTheme() ? Theme::Light : Theme::Dark;
    themeManager.SetTheme(newTheme);
    
    // Update global colors
    Colors::UpdateThemeColors();
    
    // Apply theme to main window
    themeManager.ApplyToWindow(m_hWnd);
    
    // Note: We're NOT applying dark theme to the rebar/toolbar because
    // the "Find:" label text becomes invisible in dark mode due to Windows limitations.
    // Just refresh the rebar to ensure it's properly displayed
    if (m_hWndToolBar)
    {
        ::InvalidateRect(m_hWndToolBar, nullptr, TRUE);
        ::UpdateWindow(m_hWndToolBar);
    }
    
    // Update status bar colors
    if (m_hWndStatusBar)
    {
        if (themeManager.IsDarkTheme())
        {
            // Apply dark theme to status bar
            themeManager.ApplyToWindow(m_hWndStatusBar);
            
            // Set dark background color for status bar
            ::SendMessage(m_hWndStatusBar, SB_SETBKCOLOR, 0, RGB(45, 45, 45));
        }
        else
        {
            // Reset to default colors
            ::SendMessage(m_hWndStatusBar, SB_SETBKCOLOR, 0, CLR_DEFAULT);
        }
        
        ::InvalidateRect(m_hWndStatusBar, nullptr, TRUE);
    }
    
    // Refresh all views and detail panes (only actual views, not the "+" tab)
    for (int i = 0; i < GetActualViewCount(); ++i)
    {
        auto pItem = GetTabCtrl().GetItem(i);
        if (pItem)
        {
            pItem->GetView().InvalidateRect(nullptr, TRUE);
            pItem->RefreshTheme();  // Refresh detail pane theme
        }
    }
    
    // Find dialog no longer needs custom theme handling
    
    // Update menu checkmark
    UISetCheck(ID_VIEW_DARKMODE, themeManager.IsDarkTheme());
    
    // Force UI update
    UIUpdateMenuBar();
    
    // Force main window to repaint
    InvalidateRect(nullptr, TRUE);
    UpdateWindow();
}

void CMainFrame::OnViewClose(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CloseView(GetTabCtrl().GetCurSel());
}

void CMainFrame::OnViewDuplicate(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    auto name = GetView().GetName() + L" (copy)";
    CFilterDlg dlg(name, GetView().GetFilters(), &m_logFile, {}, false, GetView().GetUniqueProcedures());
    if (dlg.DoModal() != IDOK)
    {
        return;
    }

    AddFilterView(dlg.GetName(), dlg.GetFilters());

    // auto &view = GetView();
    // auto newLogView = std::make_shared<CLogView>(view);        // no copy constructor available
    // newLogView->SetName(view.GetName() + L" (copy)");
    // AddFilterView(newLogView);
    
    // Note: "+" tab is already present, no need to re-add it

    SaveSettings();
}

void CMainFrame::OnSources(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CSourcesDlg dlg(m_sourceInfos);
    if (dlg.DoModal() != IDOK)
    {
        return;
    }

    auto sourceInfos = dlg.GetSourceInfos();
    UpdateLogSources(sourceInfos);

    m_sourceInfos = sourceInfos;
}

void CMainFrame::UpdateLogSources(const std::vector<SourceInfo>& sources)
{
    m_logSources.RemoveSources([this](LogSource* logsource) {
        if (logsource == m_pLocalReader)
        {
            return false;
        }
        if (logsource == m_pGlobalReader)
        {
            return false;
        }
        return true;
    });

    for (auto& sourceInfo : sources)
    {
        if (sourceInfo.enabled)
        {
            AddLogSource(sourceInfo);
        }
    }
}

void CMainFrame::AddLogSource(const SourceInfo& info)
{
    switch (info.type)
    {
    case SourceType::DebugViewAgent: m_logSources.AddDbgviewReader(Str(info.address)); break;
    case SourceType::Udp: m_logSources.AddUDPReader(info.port); break;
    case SourceType::Tcp: throw std::exception("SourceType::Tcp not implememted");
    default:
        // do nothing
        throw std::exception("SourceType not implememted");
    }
}

void CMainFrame::CloseView(int i)
{
    int views = GetActualViewCount();
    if (i >= 0 && i < views)
    {
        // Set flag to prevent AddFilterView from being called during programmatic tab change
        m_bProgrammaticTabChange = true;

        // Get the tab item and call Cleanup before deletion
        auto pTabItem = GetTabCtrl().GetItem(i);
        if (pTabItem)
        {
            // Call Cleanup to properly destroy windows before the tab is deleted
            pTabItem->Cleanup();
        }

        // Now delete the tab item - windows have been cleaned up
        // Using false to prevent OnDeleteTab from being called
        // Check if the tab control is still valid before deletion
        if (::IsWindow(GetTabCtrl().m_hWnd))
        {
            GetTabCtrl().DeleteItem(i, false);
        }

        // Select the appropriate tab after deletion
        if (views > 1)  // If there are other views besides the one we're closing
        {
            GetTabCtrl().SetCurSel(i == views - 1 ? i - 1 : i);
        }

        // Clear the flag
        m_bProgrammaticTabChange = false;

        UpdateViewTabCloseStates();
    }
}

void CMainFrame::UpdateViewTabCloseStates()
{
    int viewCount = GetActualViewCount();
    bool canClose = viewCount > 1;

    for (int i = 0; i < viewCount; ++i)
    {
        if (auto* pTabItem = dynamic_cast<CLogViewTabItem2*>(GetTabCtrl().GetItem(i)))
        {
            pTabItem->SetCanClose(canClose);
        }
    }
}

void CMainFrame::OnViewFind(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_findDlg.SetFocus();
}

void CMainFrame::OnViewFont(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CFontDialog dlg(&m_logfont, CF_SCREENFONTS);
    if (dlg.DoModal(*this) == IDOK)
    {
        m_logfont = dlg.m_lf;
        SetLogFont();
    }
}

void CMainFrame::SetLogFont()
{
    Win32::HFont hFont(CreateFontIndirect(&m_logfont));
    if (!hFont)
    {
        return;
    }

    int views = GetActualViewCount();
    for (int i = 0; i < views; ++i)
    {
        GetView(i).SetFont(hFont.get());
    }
    m_hFont = std::move(hFont);
}

fusion::debugviewpp::SqlFormatOptions CMainFrame::GetSqlFormatOptions() const
{
    FILE* log = fopen("C:\\Dev\\UltimateDebugViewPP\\prefs_debug.log", "a");
    if (log) {
        fprintf(log, "GetSqlFormatOptions: m_newViewDefaults.sqlKeywordCase = %d\n", (int)m_newViewDefaults.sqlKeywordCase);
        fclose(log);
    }

    fusion::debugviewpp::SqlFormatOptions options;
    options.keywordCase = m_newViewDefaults.sqlKeywordCase;
    options.indentType = m_newViewDefaults.sqlIndentType;
    options.indentSpaces = m_newViewDefaults.sqlIndentSpaces;
    options.commaAlignment = m_newViewDefaults.sqlCommaAlignment;
    options.formatStyle = m_newViewDefaults.sqlFormatStyle;
    options.parenthesesStyle = m_newViewDefaults.sqlParenthesesStyle;
    options.newlineBeforeKeywords = m_newViewDefaults.sqlNewlineKeywords;

    if (log = fopen("C:\\Dev\\UltimateDebugViewPP\\prefs_debug.log", "a")) {
        fprintf(log, "GetSqlFormatOptions: options.keywordCase = %d\n", (int)options.keywordCase);
        fprintf(log, "GetSqlFormatOptions: options.indentType = %d, spaces = %d\n", (int)options.indentType, options.indentSpaces);
        fprintf(log, "GetSqlFormatOptions: options.commaAlignment = %d\n", (int)options.commaAlignment);
        fprintf(log, "GetSqlFormatOptions: options.newlineBeforeKeywords = %d\n", options.newlineBeforeKeywords ? 1 : 0);
        fclose(log);
    }

    return options;
}

void CMainFrame::SetDetailFont(const LOGFONT& lf)
{
    m_detailLogfont = lf;
    Win32::HFont hFont(CreateFontIndirect(&m_detailLogfont));
    if (!hFont)
    {
        return;
    }
    
    m_hDetailFont = std::move(hFont);
    
    // Apply to all existing detail panes (only actual views, not the "+" tab)
    for (int i = 0; i < GetActualViewCount(); ++i)
    {
        auto pItem = GetTabCtrl().GetItem(i);
        if (pItem)
        {
            pItem->SetDetailFont(m_hDetailFont.get());
        }
    }
}

HFONT CMainFrame::GetDetailFont() const
{
    return m_hDetailFont.get();
}

void CMainFrame::OnAppAbout(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CAboutDlg dlg;
    dlg.DoModal();
}

int CMainFrame::GetViewCount() const
{
    return m_TabCtrl.GetItemCount();
}

int CMainFrame::GetActualViewCount() const
{
    int totalTabs = m_TabCtrl.GetItemCount();

    // Check if the last tab is the "+" tab
    // During initialization, the "+" tab might not exist yet
    if (totalTabs > 0)
    {
        auto pLastTab = m_TabCtrl.GetItem(totalTabs - 1);
        if (pLastTab && pLastTab->GetText() == L"+")
        {
            // The last tab is the "+" tab, so subtract it
            return totalTabs - 1;
        }
    }

    // No "+" tab, so all tabs are actual views
    return totalTabs;
}

CLogView& CMainFrame::GetView(int i)
{
    assert(i >= 0 && i < GetActualViewCount());
    auto pTabItem = GetTabCtrl().GetItem(i);
    if (!pTabItem)
    {
        // This should not happen for valid indices, but handle it gracefully
        throw std::runtime_error("Invalid tab index or tab item is null");
    }
    return pTabItem->GetView();
}

CLogView& CMainFrame::GetView()
{
    int curSel = GetTabCtrl().GetCurSel();
    int actualViewCount = GetActualViewCount();
    
    // Ensure we never try to access the "+" tab as a view
    if (curSel >= actualViewCount)
    {
        curSel = actualViewCount > 0 ? actualViewCount - 1 : 0;
    }
    
    return GetView(std::max(0, curSel));
}

bool IsClearBufferMessage(const std::string& message)
{
    return message.find("DBGVIEWCLEAR") == 0;
}

void CMainFrame::AddMessage(const Message& message)
{
    // Check for special "NewDebugView" message
    if (message.text == "@@NEWDEBUGVIEW@@")
    {
        std::wstring processName = WStr(message.processName).str();
        if (!processName.empty())
        {
            int existingViewIndex = FindViewWithProcessFilter(processName);
            if (existingViewIndex >= 0)
            {
                // View exists - switch to it and clear it
                SwitchToView(existingViewIndex);
                GetView(existingViewIndex).Clear();
            }
            else
            {
                // Create new view for this process
                CreateProcessView(processName);
            }
        }
        return; // Don't add this special message to the log
    }

    if (IsClearBufferMessage(message.text))
    {
        ClearLog();
        return;
    }

    int beginIndex = m_logFile.BeginIndex();
    int index = m_logFile.EndIndex();
    m_logFile.Add(message);
    
    // After adding to logFile, retrieve it back to get the parsed version
    Message parsedMsg = m_logFile[index];
    
    // Add message to all views - each view maintains its own procedure tracker
    int views = GetActualViewCount();
    for (int i = 0; i < views; ++i)
    {
        GetView(i).Add(beginIndex, index, message);
    }
    
    // If the procedure window is visible and this is a Clarion procedure event,
    // refresh the procedure view to show the latest data from the ACTIVE view only
    if (m_wndProcedureView.IsWindow() && m_bProcedurePaneVisible && 
        parsedMsg.isClarionFormat && parsedMsg.procedureEvent != Message::ProcedureEvent::None)
    {
        // Only refresh if we have views
        if (GetViewCount() > 0)
        {
            int activeTab = GetTabCtrl().GetCurSel();
            if (activeTab >= 0 && activeTab < GetViewCount())
            {
                // Refresh the separate procedure view window (legacy)
                if (m_wndProcedureView.IsWindow())
                {
                    m_wndProcedureView.UpdateProcedureTree();
                }
                
                // Also refresh the procedure tabs in the detail pane
                auto pTabItem = dynamic_cast<CLogViewTabItem2*>(GetTabCtrl().GetItem(activeTab));
                if (pTabItem)
                {
                    // Get the procedure tabs and update their displays using convenience methods
                    auto& tabContainer = pTabItem->GetDetailTabContainer();
                    auto procedureFlowTab = tabContainer.GetProcedureFlowTab();
                    auto openProceduresTab = tabContainer.GetOpenProceduresTab();
                    
                    if (procedureFlowTab)
                        procedureFlowTab->UpdateDisplay();
                    if (openProceduresTab)
                        openProceduresTab->UpdateDisplay();
                }
            }
        }
    }
}

int CMainFrame::FindViewWithProcessFilter(const std::wstring& processName)
{
    int viewCount = GetActualViewCount();
    for (int i = 0; i < viewCount; ++i)
    {
        auto filters = GetView(i).GetFilters();
        // Check if this view has a single process include filter matching our process
        if (filters.processFilters.size() == 1)
        {
            const auto& filter = filters.processFilters[0];
            if (filter.filterType == FilterType::Include && 
                filter.matchType == MatchType::Simple &&
                filter.enable &&
                WStr(filter.text).str() == processName)
            {
                return i;
            }
        }
    }
    return -1; // Not found
}

void CMainFrame::CreateProcessView(const std::wstring& processName)
{
    // Create a filter for this specific process
    LogFilter filter;
    // Use the parameterized constructor to properly initialize the regex
    Filter processFilter(
        Str(processName).str(),     // text
        MatchType::Simple,           // matchType
        FilterType::Include,         // filterType
        Colors::Auto,                // bgColor
        Colors::Text,                // fgColor
        true                         // enable
    );
    filter.processFilters.push_back(processFilter);
    
    // Apply default prefix filters from preferences
    filter.prefixFilters.reserve(m_newViewDefaults.systemPrefixFilters.size() + m_newViewDefaults.customPrefixFilters.size());
    filter.prefixFilters.insert(filter.prefixFilters.end(), m_newViewDefaults.systemPrefixFilters.begin(), m_newViewDefaults.systemPrefixFilters.end());
    filter.prefixFilters.insert(filter.prefixFilters.end(), m_newViewDefaults.customPrefixFilters.begin(), m_newViewDefaults.customPrefixFilters.end());
    
    // Create the view with the process name as the tab name
    AddFilterView(processName, filter);
    
    // Note: "+" tab is already present, no need to re-add it
    
    // Switch to the newly created view (last actual view)
    int newViewIndex = GetActualViewCount() - 1;
    SwitchToView(newViewIndex);
}

void CMainFrame::CreateProcedureView(const std::string& procedureName, const std::string& processName)
{
    // Create a filter for this specific procedure
    LogFilter filter;

    // Add procedure filter
    Filter procedureFilter(
        procedureName,               // text
        MatchType::Simple,           // matchType
        FilterType::Include,         // filterType
        Colors::Auto,                // bgColor
        Colors::Text,                // fgColor
        true                         // enable
    );
    filter.procedureFilters.push_back(procedureFilter);

    // Add process filter if specified
    if (!processName.empty())
    {
        Filter processFilter(
            processName,                 // text
            MatchType::Simple,           // matchType
            FilterType::Include,         // filterType
            Colors::Auto,                // bgColor
            Colors::Text,                // fgColor
            true                         // enable
        );
        filter.processFilters.push_back(processFilter);
    }

    // Apply default prefix filters from preferences
    filter.prefixFilters.reserve(m_newViewDefaults.systemPrefixFilters.size() + m_newViewDefaults.customPrefixFilters.size());
    filter.prefixFilters.insert(filter.prefixFilters.end(), m_newViewDefaults.systemPrefixFilters.begin(), m_newViewDefaults.systemPrefixFilters.end());
    filter.prefixFilters.insert(filter.prefixFilters.end(), m_newViewDefaults.customPrefixFilters.begin(), m_newViewDefaults.customPrefixFilters.end());

    // Create view with descriptive name
    std::wstring viewName = L"Proc: " + WStr(procedureName).str();
    AddFilterView(viewName, filter);

    // Switch to the newly created view
    int newViewIndex = GetActualViewCount() - 1;
    SwitchToView(newViewIndex);
}

void CMainFrame::JumpToDebugRow(int lineIndex, const std::string& viewName)
{
    // If no valid line index is provided, show a message
    if (lineIndex < 0)
    {
        ::MessageBox(m_hWnd,
                     _T("Jump to Debug Row requires source tracking to be implemented.\n\n")
                     _T("This feature will be available in a future update and will allow ")
                     _T("jumping directly to the source log line where the procedure event originated."),
                     _T("Feature Not Yet Available"),
                     MB_OK | MB_ICONINFORMATION);
        return;
    }

    // Find the correct view by name if provided
    if (GetActualViewCount() > 0)
    {
        SwitchToView(0);  // Switch to first view for now

        // Get the current view and jump to the line
        CLogView& view = GetView();
        if (lineIndex >= 0 && lineIndex < view.GetItemCount())
        {
            view.SetFocusLine(lineIndex);
            view.SetItemState(lineIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            view.EnsureVisible(lineIndex, FALSE);
        }
    }
}

void CMainFrame::SwitchToView(int index)
{
    if (index >= 0 && index < GetActualViewCount())
    {
        // Set flag to prevent dialog from showing during programmatic view switch
        m_bProgrammaticTabChange = true;
        GetTabCtrl().SetCurSel(index);
        m_bProgrammaticTabChange = false;

        // Send a selection change notification to update the UI
        NMHDR nmhdr = {0};
        nmhdr.hwndFrom = GetTabCtrl().m_hWnd;
        nmhdr.idFrom = GetTabCtrl().GetDlgCtrlID();
        nmhdr.code = TCN_SELCHANGE;
        SendMessage(WM_NOTIFY, nmhdr.idFrom, reinterpret_cast<LPARAM>(&nmhdr));
    }
}

LRESULT CMainFrame::OnUserCreateProcedureView(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    // wParam contains a pointer to the procedure name string
    // lParam contains a pointer to the process name string (optional, can be null)
    const char* procedureName = reinterpret_cast<const char*>(wParam);
    const char* processName = reinterpret_cast<const char*>(lParam);

    if (procedureName)
    {
        std::string procName(procedureName);
        std::string procProcess(processName ? processName : "");
        CreateProcedureView(procName, procProcess);
    }

    return 0;
}

LRESULT CMainFrame::OnUserJumpToDebugRow(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    // wParam contains the line index
    // lParam contains a pointer to the view name string (optional)
    int lineIndex = static_cast<int>(wParam);
    const char* viewName = reinterpret_cast<const char*>(lParam);

    std::string viewNameStr(viewName ? viewName : "");
    JumpToDebugRow(lineIndex, viewNameStr);

    return 0;
}

LRESULT CMainFrame::OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    auto& themeManager = ThemeManager::Instance();
    if (themeManager.IsDarkTheme())
    {
        // Paint the background dark for dark mode
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(&rc);
        HBRUSH hBrush = ::CreateSolidBrush(RGB(45, 45, 45));
        ::FillRect(hdc, &rc, hBrush);
        ::DeleteObject(hBrush);
        return 1;  // We handled it
    }
    
    bHandled = FALSE;
    return 0;
}

LRESULT CMainFrame::OnCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
    // We're not applying dark mode to the toolbar/rebar anymore
    // due to the "Find:" label visibility issue
    bHandled = FALSE;
    return 0;
}

} // namespace debugviewpp
} // namespace fusion



