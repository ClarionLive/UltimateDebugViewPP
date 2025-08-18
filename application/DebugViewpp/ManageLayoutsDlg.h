// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "CobaltFusion/AtlWinExt.h"
#include "resource.h"
#include "atleverything.h"
#include <vector>
#include <string>

namespace fusion {
namespace debugviewpp {

class CLogView;

class CManageLayoutsDlg : public CDialogImpl<CManageLayoutsDlg>,
                          public ExceptionHandler<CManageLayoutsDlg, std::exception>
{
public:
    explicit CManageLayoutsDlg(CLogView& logView);

    enum
    {
        IDD = IDD_MANAGE_LAYOUTS
    };

    std::wstring GetSelectedLayout() const;
    bool ShouldLoadLayout() const;

private:
    DECLARE_MSG_MAP()

    void OnException();
    void OnException(const std::exception& ex);
    BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
    void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnLoad(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnRename(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnDelete(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnListSelChange(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnListDblClick(UINT uNotifyCode, int nID, CWindow wndCtl);

    void PopulateList();
    void UpdateButtonStates();
    std::wstring GetSelectedLayoutName() const;

    CLogView& m_logView;
    CListBox m_listBox;
    std::vector<std::wstring> m_layouts;
    std::wstring m_selectedLayout;
    bool m_loadLayout;
};

} // namespace debugviewpp
} // namespace fusion