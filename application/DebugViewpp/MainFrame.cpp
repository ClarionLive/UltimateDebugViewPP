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

#include "resource.h"
#include "RunDlg.h"
#include "HistoryDlg.h"
#include "FilterDlg.h"
#include "SourcesDlg.h"
#include "AboutDlg.h"
#include "FileOptionDlg.h"
#include "LogView.h"
#include "MainFrame.h"

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
    MSG_WM_QUERYENDSESSION(OnQueryEndSession)
    MSG_WM_ENDSESSION(OnEndSession)
    MSG_WM_MOUSEWHEEL(OnMouseWheel)
    MSG_WM_CONTEXTMENU(OnContextMenu)
    MSG_WM_SYSCOMMAND(OnSysCommand)
    MESSAGE_HANDLER_EX(WM_SYSTEMTRAYICON, OnSystemTrayIcon)
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
    RemoveDriver();
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

    CreateTabWindow(*this, rcDefault, CTCS_DRAGREARRANGE | CTCS_CLOSEBUTTON);
    AddFilterView(L"View");
    HideTabControl();

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
    
    // Apply word wrap state and detail font to the detail view that was already created
    // The first tab was created before LoadSettings, so we need to apply the setting now
    if (GetViewCount() > 0)
    {
        auto pTabItem = GetTabCtrl().GetItem(0);
        if (pTabItem)
        {
            pTabItem->SetMainFrame(this);  // This will re-apply the word wrap state
            if (m_hDetailFont)
            {
                pTabItem->SetDetailFont(m_hDetailFont.get());
            }
        }
    }

    // register object for message filtering and idle updates
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != nullptr);
    pLoop->AddMessageFilter(this);
    pLoop->AddIdleHandler(this);

    m_logSources.SubscribeToUpdate([this] { return OnUpdate(); });

    // Resume can throw if a second debugview is running
    // so do not rely on any commands executed afterwards
    Resume();
    return 0;
}

void CMainFrame::OnClose()
{
    SaveSettings();
    DestroyWindow();

    if (m_notifyIconData.cbSize != 0u)
    {
        Shell_NotifyIcon(NIM_DELETE, &m_notifyIconData);
        m_notifyIconData.cbSize = 0;
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
    UISetText(ID_DEFAULT_PANE, isearch.empty() ? (m_pLocalReader != nullptr ? L"Ready" : L"Paused") : search.c_str());
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

    int views = GetViewCount();
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
        for (size_t i = 0;; ++i)
        {
            CRegKey regView;
            if (regView.Open(regViews, WStr(wstringbuilder() << L"View" << i)) != ERROR_SUCCESS)
            {
                break;
            }

            auto name = Win32::RegGetStringValue(regView);
            if (i == 0)
            {
                GetTabCtrl().GetItem(0)->SetText(name.c_str());
            }
            else
            {
                AddFilterView(name);
            }
            GetView().LoadSettings(regView);
            
            // Load the splitter position for this view
            auto pTabItem = GetTabCtrl().GetItem(static_cast<int>(i));
            if (pTabItem)
            {
                // Load detail pane visibility (default to false for new views)
                bool detailPaneVisible = Win32::RegGetDWORDValue(regView, L"DetailPaneVisible", 0) != 0;
                pTabItem->ShowDetailPane(detailPaneVisible);
                
                int splitterPos = Win32::RegGetDWORDValue(regView, L"DetailSplitterPos", 75);
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
        GetTabCtrl().SetCurSel(Win32::RegGetDWORDValue(regViews, L"Current", 0));
        GetTabCtrl().UpdateLayout();
        GetTabCtrl().Invalidate();
    }
    else
    {
        // First run - no registry settings exist
        // Ensure the initial view created in OnCreate has detail pane hidden
        auto pTabItem = GetTabCtrl().GetItem(0);
        if (pTabItem)
        {
            pTabItem->ShowDetailPane(false);  // Explicitly hide detail pane on first run
        }
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

    reg.SetStringValue(L"FontName", m_logfont.lfFaceName);
    reg.SetDWORDValue(L"FontSize", LogFontSizeToPointSize(m_logfont.lfHeight));
    
    // Save detail pane font
    reg.SetStringValue(L"DetailFontName", m_detailLogfont.lfFaceName);
    reg.SetDWORDValue(L"DetailFontSize", LogFontSizeToPointSize(m_detailLogfont.lfHeight));

    reg.RecurseDeleteKey(L"Views");

    CRegKey regViews;
    regViews.Create(reg, L"Views");
    regViews.SetDWORDValue(L"Current", GetTabCtrl().GetCurSel());

    int views = GetViewCount();
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

    CRegKey regColors;
    regColors.Create(reg, L"Colors");
    auto colors = ColorDialog::GetCustomColors();
    for (int i = 0; i < 16; ++i)
    {
        regColors.SetDWORDValue(WStr(wstringbuilder() << L"Color" << i), colors[i]);
    }
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
    
    return true;
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
        
        // Build status text with metadata
        std::wstring statusText = L"Line: " + std::to_wstring(lineIndex + 1) + 
                                 L"  |  Time: " + WStr(GetTimeText(message.systemTime)).str() + 
                                 L"  |  Process: [" + std::to_wstring(message.processId) + L"] " + 
                                 WStr(message.processName).str();
        
        // Content is just the message text
        std::wstring contentText = WStr(message.text).str();
        
        // Update the detail view with separated status and content
        pTabItem->UpdateDetailView(statusText, contentText);
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
        pTabItem->UpdateDetailView(L"", L"");
        return;
    }
    
    std::wstring statusText;
    std::wstring contentText;
    
    // If only one line selected, show detailed metadata
    if (lineIndices.size() == 1)
    {
        int lineIndex = lineIndices[0];
        if (lineIndex >= 0 && lineIndex < m_logFile.Count())
        {
            const auto& message = m_logFile[lineIndex];
            
            // Build status text with metadata
            statusText = L"Line: " + std::to_wstring(lineIndex + 1) + 
                        L"  |  Time: " + WStr(GetTimeText(message.systemTime)).str() + 
                        L"  |  Process: [" + std::to_wstring(message.processId) + L"] " + 
                        WStr(message.processName).str();
            
            // Content is just the message text
            contentText = WStr(message.text).str();
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
    pTabItem->UpdateDetailView(statusText, contentText);
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
    CFilterDlg dlg(GetView().GetName(), filters, &m_logFile);
    if (dlg.DoModal() == IDOK)
    {
        int tabIdx = GetTabCtrl().GetCurSel();
        GetTabCtrl().GetItem(tabIdx)->SetText(dlg.GetName().c_str());
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
    CFilterDlg dlg(wstringbuilder() << L"View " << m_filterNr, LogFilter(), &m_logFile);
    if (dlg.DoModal() != IDOK)
    {
        return;
    }

    AddFilterView(dlg.GetName(), dlg.GetFilters());
    SaveSettings();
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
    
    // Apply detail font if available
    if (m_hDetailFont)
    {
        pTabItem->SetDetailFont(m_hDetailFont.get());
    }

    logview->Create(pTabItem->GetLogViewParent(), rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    logview->SetFont(m_hFont.get());

    pTabItem->SetText(logview->GetName().c_str());
    pTabItem->SetView(logview);

    int newIndex = GetTabCtrl().GetItemCount();
    
    // The first tab (View) should not be closeable
    if (newIndex == 0)
    {
        pTabItem->SetCanClose(false);
    }

    // notice: InsertItem takes ownership of the raw pointer, DeleteItem calls delete
    GetTabCtrl().InsertItem(newIndex, pTabItem.release());
    GetTabCtrl().SetCurSel(newIndex);
    ShowTabControl();
}

LRESULT CMainFrame::OnBeginTabDrag(NMHDR* pnmh)
{
    auto& nmhdr = *reinterpret_cast<NMCTCITEM*>(pnmh);

    return static_cast<LRESULT>(nmhdr.iItem >= GetViewCount());
}

LRESULT CMainFrame::OnChangeTab(NMHDR* pnmh)
{
    SetMsgHandled(win32::False);

    auto& nmhdr = *reinterpret_cast<NMCTC2ITEMS*>(pnmh);

    if (nmhdr.iItem2 >= 0 && nmhdr.iItem2 < GetViewCount())
    {
        SetModifiedMark(nmhdr.iItem2, false);
        
        // Restore the detail pane content for the newly activated tab
        auto pTabItem = GetTabCtrl().GetItem(nmhdr.iItem2);
        if (pTabItem)
        {
            pTabItem->RestoreDetailContent();
        }
        
        // Update UI to reflect the detail pane state of the new tab
        UpdateUI();
    }

    if (!m_linkViews || nmhdr.iItem1 == nmhdr.iItem2 || nmhdr.iItem1 < 0 || nmhdr.iItem1 >= GetViewCount() || nmhdr.iItem2 < 0 ||
        nmhdr.iItem2 >= GetViewCount())
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
    GetTabCtrl().GetItem(tabindex)->SetText(name.c_str());
}

LRESULT CMainFrame::OnCloseTab(NMHDR* pnmh)
{
    auto& nmhdr = *reinterpret_cast<NMCTCITEM*>(pnmh);
    CloseView(nmhdr.iItem);
    return 0;
}

LRESULT CMainFrame::OnDeleteTab(NMHDR* pnmh)
{
    auto& nmhdr = *reinterpret_cast<NMCTCITEM*>(pnmh);

    if (nmhdr.iItem >= 0 && nmhdr.iItem < GetViewCount())
    {
        GetView(nmhdr.iItem).DestroyWindow();
    }

    return FALSE;
}

void CMainFrame::OnFileNewTab(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    AddFilterView();
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
            view.columnsPt = viewPt.get_child_optional("Columns");
            views.push_back(view);
        }
    }
    std::sort(views.begin(), views.end(), [](const View& v1, const View& v2) { return v1.index < v2.index; });

    m_logSources.SetAutoNewLine(autoNewline);
    m_linkViews = linkViews;

    for (int i = 0; i < static_cast<int>(views.size()); ++i)
    {
        if (i >= GetViewCount())
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

    int i = GetViewCount();
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

    int views = GetViewCount();
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
    const int views = GetViewCount();
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

void CMainFrame::ClearLog()
{
    m_logFile.Clear();
    m_logSources.ResetTimer();
    int views = GetViewCount();
    for (int i = 0; i < views; ++i)
    {
        GetView(i).Clear();
        SetModifiedMark(i, false);
        GetTabCtrl().UpdateLayout();
        GetTabCtrl().Invalidate();
    }
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
    std::swap(temp, m_logFile);

    m_logSources.ResetTimer();
    int views = GetViewCount();
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
    CFilterDlg dlg(GetView().GetName(), GetView().GetFilters(), &m_logFile);
    if (dlg.DoModal() != IDOK)
    {
        return;
    }

    int tabIdx = GetTabCtrl().GetCurSel();
    GetTabCtrl().GetItem(tabIdx)->SetText(dlg.GetName().c_str());
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
    
    // Refresh all views and detail panes
    for (int i = 0; i < GetTabCtrl().GetItemCount(); ++i)
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
    CFilterDlg dlg(name, GetView().GetFilters(), &m_logFile);
    if (dlg.DoModal() != IDOK)
    {
        return;
    }

    AddFilterView(dlg.GetName(), dlg.GetFilters());

    // auto &view = GetView();
    // auto newLogView = std::make_shared<CLogView>(view);        // no copy constructor available
    // newLogView->SetName(view.GetName() + L" (copy)");
    // AddFilterView(newLogView);

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
    int views = GetViewCount();
    if (i >= 0 && i < views)
    {
        // DeleteItem actually calls delete on the contained item
        GetTabCtrl().DeleteItem(i, false);
        GetTabCtrl().SetCurSel(i == views - 1 ? i - 1 : i);
        if (GetViewCount() == 1)
        {
            HideTabControl();
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

    int views = GetViewCount();
    for (int i = 0; i < views; ++i)
    {
        GetView(i).SetFont(hFont.get());
    }
    m_hFont = std::move(hFont);
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
    
    // Apply to all existing detail panes
    for (int i = 0; i < GetTabCtrl().GetItemCount(); ++i)
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

CLogView& CMainFrame::GetView(int i)
{
    assert(i >= 0 && i < GetViewCount());
    return GetTabCtrl().GetItem(i)->GetView();
}

CLogView& CMainFrame::GetView()
{
    return GetView(std::max(0, GetTabCtrl().GetCurSel()));
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
    int views = GetViewCount();
    for (int i = 0; i < views; ++i)
    {
        GetView(i).Add(beginIndex, index, message);
    }
}

int CMainFrame::FindViewWithProcessFilter(const std::wstring& processName)
{
    int viewCount = GetViewCount();
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
    Filter processFilter;
    processFilter.text = Str(processName).str();
    processFilter.matchType = MatchType::Simple;
    processFilter.filterType = FilterType::Include;
    processFilter.bgColor = Colors::Auto;
    processFilter.fgColor = Colors::Text;
    processFilter.enable = true;
    filter.processFilters.push_back(processFilter);
    
    // Create the view with the process name as the tab name
    AddFilterView(processName, filter);
    
    // Switch to the newly created view
    int newViewIndex = GetViewCount() - 1;
    SwitchToView(newViewIndex);
    
    // Populate the new view with existing messages from this process
    auto& newView = GetView(newViewIndex);
    int messageCount = m_logFile.Count();
    for (int i = 0; i < messageCount; ++i)
    {
        auto msg = m_logFile[i];
        // Add the message to the new view (it will be filtered by the process filter)
        newView.Add(i, i, msg);
    }
}

void CMainFrame::SwitchToView(int index)
{
    if (index >= 0 && index < GetViewCount())
    {
        GetTabCtrl().SetCurSel(index);
        // Send a selection change notification to update the UI
        NMHDR nmhdr = {0};
        nmhdr.hwndFrom = GetTabCtrl().m_hWnd;
        nmhdr.idFrom = GetTabCtrl().GetDlgCtrlID();
        nmhdr.code = TCN_SELCHANGE;
        SendMessage(WM_NOTIFY, nmhdr.idFrom, reinterpret_cast<LPARAM>(&nmhdr));
    }
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
