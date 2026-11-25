// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memory>
#include <vector>

#include "atleverything.h"
#include <atlsplit.h>

namespace WTL {
using ATL::CString;
}

#include <atlcom.h>

#pragma warning(push, 3)
#pragma warning(disable : 4838)

#include "CustomTabCtrl.h"
#include "DotNetTabCtrl.h"
// #include "DotNetTabCtrlWithCloseButtons.h"  // Removed - reverting to original tab control
#include "TabbedFrame.h"
#pragma warning(pop)

#include "CobaltFusion/AtlWinExt.h"
#include "CobaltFusion/ExecutorClient.h"
#include "DebugViewppLib/DBWinBuffer.h"
#include "DebugViewppLib/DBWinReader.h"
#include "DebugViewppLib/KernelReader.h"
#include "DebugViewppLib/LineBuffer.h"
#include "DebugViewppLib/LogSources.h"
#include "DebugViewppLib/FileWriter.h"
#include "CLogViewTabItem2.h"
#include "ClarionProcedureView.h"
#include "FloatingMainTabWindow.h"
#include "FindDlg.h"
#include "RunDlg.h"
#include "LogView.h"
#include "PreferencesDlg.h"

namespace fusion {
namespace debugviewpp {

struct SelectionInfo;
class DbgviewReader;

using SelectedTabItem = CLogViewTabItem2;

void RemoveDriver();

class CMainFrame : public CTabbedFrameImpl<CMainFrame, CDotNetTabCtrl<SelectedTabItem>>,
                   public CUpdateUI<CMainFrame>,
                   public ExceptionHandler<CMainFrame, std::exception>,
                   public CMessageFilter,
                   public CIdleHandler
{
public:
    using TabbedFrame = CTabbedFrameImpl<CMainFrame, CDotNetTabCtrl<SelectedTabItem>>;

    CMainFrame();
    ~CMainFrame() override;

    DECLARE_FRAME_WND_CLASS(nullptr, IDR_MAINFRAME)

    BEGIN_UPDATE_UI_MAP(CMainFrame)
        UPDATE_ELEMENT(ID_LOG_PAUSE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_LOG_GLOBAL, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_LOG_KERNEL, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_LOG_KERNEL_VERBOSE, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_LOG_KERNEL_PASSTHROUGH, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_SCROLL, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_VIEW_SCROLL_STOP, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_VIEW_TIME, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_VIEW_PROCESSCOLORS, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_DARKMODE, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_DETAIL_PANE, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_BOOKMARK, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_VIEW_COLUMN_LINE, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_COLUMN_DATE, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_COLUMN_TIME, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_COLUMN_PID, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_COLUMN_PROCESS, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_OPTIONS_LINKVIEWS, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_OPTIONS_AUTONEWLINE, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_OPTIONS_PROCESS_PREFIX, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_OPTIONS_ALWAYSONTOP, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_OPTIONS_HIDE, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_OPTIONS_EXPANDED_TOOLBAR, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_DEFAULT_PANE, UPDUI_STATUSBAR)
        UPDATE_ELEMENT(ID_SELECTION_PANE, UPDUI_STATUSBAR)
        UPDATE_ELEMENT(ID_VIEW_PANE, UPDUI_STATUSBAR)
        UPDATE_ELEMENT(ID_LOGFILE_PANE, UPDUI_STATUSBAR)
        UPDATE_ELEMENT(ID_MEMORY_PANE, UPDUI_STATUSBAR)
    END_UPDATE_UI_MAP()

    void SetLogging();
    void LoadConfiguration(const std::wstring& fileName);
    void SaveConfiguration(const std::wstring& fileName);
    void Load(const std::wstring& fileName, bool keeptailing);
    void Load(HANDLE hFile);
    void Load(std::istream& file, const std::string& name, FILETIME fileTime);
    void SetSelectTabByName(const std::wstring& tabName);
    void CapturePipe(HANDLE hPipe);
    void FindNext(const std::wstring& text);
    void FindPrevious(const std::wstring& text);
    void ClearFind();
    void OnDropped(std::wstring uri);
    void UpdateDetailView(CLogView* pCallingView, int lineIndex);
    void UpdateDetailView(CLogView* pCallingView, const std::vector<int>& lineIndices);
    void FindTextInView(const std::wstring& text);
    void HighlightTextInView(const std::wstring& text);  // Just highlight, don't search
    void ShowFilterDialogWithText(const std::wstring& text);

    // Return the command which should be used to show the window, it is
    // restored from the registry when creating it.
    int GetShowCommand() const { return m_showCmd; }
    void SaveSettings();
    bool ImportDebugViewPPSettings();
    void LoadNewViewDefaults();
    void SaveNewViewDefaults();

    // Procedure and process view creation (public for Timeline access)
    void CreateProcedureView(const std::string& procedureName, const std::string& processName = "");
    void JumpToDebugRow(int lineIndex, const std::string& viewName = "");

    // Public getter for splitter position
    int GetProcedureStatsSplitterPos() const { return m_procedureStatsSplitterPos; }

private:
    enum
    {
        WM_FIRST = WM_APP,
        WM_SYSTEMTRAYICON,
    };

    DECLARE_MSG_MAP()

    BOOL PreTranslateMessage(MSG* pMsg) override;
    BOOL OnIdle() override;

    void OnException();
    void OnException(const std::exception& ex);
    LRESULT OnCreate(const CREATESTRUCT* pCreate);
    void OnClose();
    LRESULT OnQueryEndSession(WPARAM wParam, LPARAM lParam);
    LRESULT OnEndSession(WPARAM wParam, LPARAM lParam);
    bool OnUpdate();
    bool OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    void OnTimer(UINT_PTR nIDEvent);
    BOOL OnSetCursor(CWindow wnd, UINT nHitTest, UINT message);
    void ProcessLines(const Lines& lines);

    int LogFontSizeFromPointSize(int fontSize);
    int LogFontSizeToPointSize(int logFontSize);

    std::wstring GetSelectionInfoText(const std::wstring& label, const SelectionInfo& selection) const;
    SelectionInfo GetLogFileRange() const;
    void UpdateUI();
    void UpdateStatusBar();
    bool LoadSettings();

    bool IsPaused() const;
    void Pause();
    void Resume();
    void UpdateTitle();
    bool GetAlwaysOnTop() const;
    void SetAlwaysOnTop(bool value);
    void SetProcedureStatsSplitterPos(int pct);

    void AddFilterView();
    void AddFilterView(const std::wstring& name, const LogFilter& filter = LogFilter());
    void AddFilterView(std::shared_ptr<CLogView> logview);
    void AddMessage(const Message& message);
    
    // Helper method to add the "+" tab as the last tab
    void AddPlusTab();

    // Auto-view creation for processes
    int FindViewWithProcessFilter(const std::wstring& processName);
    void CreateProcessView(const std::wstring& processName);
    void SwitchToView(int index);

    void SetModifiedMark(int tabindex, bool modified);
    void ClearLog();
    void SaveLogFile(const std::wstring& fileName);
    void SaveViewFile(const std::wstring& fileName);
    void SaveViewSelection(const std::wstring& fileName);

    void OnContextMenu(HWND /*hWnd*/, CPoint pt);
    void OnSize(UINT nType, CSize size);
    void OnMove(CPoint pt);
    LRESULT OnSysCommand(UINT nCommand, CPoint /*unused*/);
    LRESULT OnSystemTrayIcon(UINT /*unused*/, WPARAM wParam, LPARAM lParam);
    LRESULT OnScRestore(UINT /*unused*/, INT /*unused*/, HWND /*unused*/);
    LRESULT OnScClose(UINT /*unused*/, INT /*unused*/, HWND /*unused*/);
    LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
    LRESULT OnCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnBeginTabDrag(NMHDR* pnmh);
    LRESULT OnChangeTab(NMHDR* pnmh);
    LRESULT OnCloseTab(NMHDR* pnmh);
    LRESULT OnDeleteTab(NMHDR* pnmh);
    void OnFileNewTab(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnFileOpen(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnFileRun(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnFileSaveLog(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnFileExit(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnFileSaveView(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnFileSaveViewSelection(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnFileLoadConfiguration(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnFileSaveConfiguration(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnFilePreferences(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnProcedureSplitterMoved(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnLinkViews(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnAutoNewline(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnProcessPrefix(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnHide(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnAlwaysOnTop(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnExpandedToolbar(UINT uNotifyCode, int nID, CWindow wndCtl);
    void ApplyExpandedToolbarState();
    void OnImportSettings(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnLogClear(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnLogCrop(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnLogPause(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnLogGlobal(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnLogKernel(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnLogKernelVerbose(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnLogKernelPassThrough(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnLogHistory(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnLogDebugviewAgent(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewFind(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewFont(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewFilter(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewDarkMode(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewDetailPane(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewProcedureTracker(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnTabPopOut(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnTabPopIn(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnMainTabPopOut(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnMainTabPopIn(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewClose(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewDuplicate(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnSources(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnAppAbout(UINT uNotifyCode, int nID, CWindow wndCtl);
    LRESULT OnUserCreateProcedureView(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnUserJumpToDebugRow(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    int GetViewCount() const;
    int GetActualViewCount() const; // Returns view count excluding the "+" tab
    CLogView& GetView(int i);
    CLogView& GetView();
    void SetLogFont();
    void UpdateProcedureViewTracker();

public:
    // Get SQL formatting options from preferences
    fusion::debugviewpp::SqlFormatOptions GetSqlFormatOptions() const;
    void SetDetailFont(const LOGFONT& lf);
    HFONT GetDetailFont() const;
    
private:
    void SetTitle(const std::wstring& title = L"");
    void HandleDroppedFile(const std::wstring& file);
    void Run(const std::wstring& pathName = L"");
    void UpdateLogSources(const std::vector<SourceInfo>& sources);
    void AddLogSource(const SourceInfo& info);
    void CloseView(int i);
    void UpdateViewTabCloseStates();

    void WriteDriverFromResource();
    void RemoveDriver();

    CCommandBarCtrl m_cmdBar;
    CMultiPaneStatusBarCtrl m_statusBar;

    LogFile m_logFile;
    std::unique_ptr<FileWriter> m_logWriter;
    int m_filterNr = 1;
    CFindDlg m_findDlg;
    Win32::HFont m_hFont;
    Win32::HFont m_hDetailFont;
    bool m_linkViews = false;
    bool m_hide = false;
    bool m_tryGlobal = false;
    bool m_tryKernel = false;
    CRunDlg m_runDlg;
    std::wstring m_logFileName;
    std::wstring m_txtFileName;
    std::wstring m_configFileName;
    size_t m_initialPrivateBytes;
    NOTIFYICONDATA m_notifyIconData;
    LOGFONT m_logfont;
    LOGFONT m_detailLogfont;
    std::wstring m_applicationName;
    DBWinReader* m_pLocalReader = nullptr;
    DBWinReader* m_pGlobalReader = nullptr;
    KernelReader* m_pKernelReader = nullptr;
    bool m_verboseKernelMessage = false;
    bool m_passthroughMode = false;
    DbgviewReader* m_pDbgviewReader = nullptr;
    std::vector<SourceInfo> m_sourceInfos;
    std::unique_ptr<GuiExecutorClient> m_GuiExecutorClient;
    LogSources m_logSources;
    Win32::JobObject m_jobs;
    Win32::Handle m_httpMonitorHandle;
    std::deque<Lines> m_incomingMessages;
    int m_showCmd = SW_SHOWDEFAULT;
    bool m_expandedToolbar = false;
    int m_procedureStatsSplitterPos = 60;
    std::string m_driverLocation = GetDebugviewDriverLocation();
    
    // New view defaults
    NewViewDefaults m_newViewDefaults;
    
    // Procedure tracking window
    CClarionProcedureView m_wndProcedureView;
    bool m_bProcedurePaneVisible = false;
    CRect m_procedureWindowRect;
    bool m_bInitializing = true;  // Flag to prevent AddFilterView during startup
    bool m_bProgrammaticTabChange = false;  // Flag to prevent dialog during programmatic tab changes
    
    // Floating main tab tracking
    struct FloatingMainTabInfo
    {
        std::unique_ptr<CFloatingMainTabWindow> window;
        CLogViewTabItem2* tabItem;
        int originalIndex;
        std::wstring tabName;
    };
    std::vector<FloatingMainTabInfo> m_floatingMainTabs;
    
    // Helper methods for floating main tabs
    void PopOutMainTab(int index);
    void ReattachMainTab(CFloatingMainTabWindow* floatingWindow);
    bool IsMainTabPoppedOut(int index) const;
    LRESULT OnFloatingMainWindowClosing(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

} // namespace debugviewpp
} // namespace fusion
