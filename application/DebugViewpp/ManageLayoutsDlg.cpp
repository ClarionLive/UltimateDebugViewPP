// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "ManageLayoutsDlg.h"
#include "LogView.h"
#include "RenameProcessDlg.h"
#include "Win32/Win32Lib.h"
#include <atlbase.h>

namespace fusion {
namespace debugviewpp {

BEGIN_MSG_MAP2(CManageLayoutsDlg)
    MSG_WM_INITDIALOG(OnInitDialog)
    COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
    COMMAND_ID_HANDLER_EX(IDC_LOAD_LAYOUT, OnLoad)
    COMMAND_ID_HANDLER_EX(IDC_RENAME_LAYOUT, OnRename)
    COMMAND_ID_HANDLER_EX(IDC_DELETE_LAYOUT, OnDelete)
    COMMAND_HANDLER_EX(IDC_LAYOUTS_LIST, LBN_SELCHANGE, OnListSelChange)
    COMMAND_HANDLER_EX(IDC_LAYOUTS_LIST, LBN_DBLCLK, OnListDblClick)
END_MSG_MAP()

CManageLayoutsDlg::CManageLayoutsDlg(CLogView& logView) :
    m_logView(logView),
    m_loadLayout(false)
{
}

void CManageLayoutsDlg::OnException()
{
    MessageBox(L"Unknown exception occurred", L"Error", MB_OK | MB_ICONERROR);
}

void CManageLayoutsDlg::OnException(const std::exception& ex)
{
    std::wstring msg = L"Error: ";
    msg += Win32::MultiByteToWideChar(ex.what());
    MessageBox(msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
}

BOOL CManageLayoutsDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    CenterWindow();
    
    m_listBox = GetDlgItem(IDC_LAYOUTS_LIST);
    
    PopulateList();
    UpdateButtonStates();
    
    return TRUE;
}

void CManageLayoutsDlg::PopulateList()
{
    m_listBox.ResetContent();
    m_layouts = m_logView.GetSavedLayouts();
    
    for (const auto& layout : m_layouts)
    {
        m_listBox.AddString(layout.c_str());
    }
    
    if (m_listBox.GetCount() > 0)
    {
        m_listBox.SetCurSel(0);
    }
}

void CManageLayoutsDlg::UpdateButtonStates()
{
    bool hasSelection = (m_listBox.GetCurSel() != LB_ERR);
    
    ::EnableWindow(GetDlgItem(IDC_LOAD_LAYOUT), hasSelection);
    ::EnableWindow(GetDlgItem(IDC_RENAME_LAYOUT), hasSelection);
    ::EnableWindow(GetDlgItem(IDC_DELETE_LAYOUT), hasSelection);
}

std::wstring CManageLayoutsDlg::GetSelectedLayoutName() const
{
    int sel = m_listBox.GetCurSel();
    if (sel == LB_ERR)
        return L"";
    
    int len = m_listBox.GetTextLen(sel);
    if (len == LB_ERR)
        return L"";
    
    CString text;
    text.GetBuffer(len + 1);
    m_listBox.GetText(sel, text.GetBuffer());
    text.ReleaseBuffer();
    return text.GetString();
}

void CManageLayoutsDlg::OnCancel(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    EndDialog(IDCANCEL);
}

void CManageLayoutsDlg::OnLoad(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_selectedLayout = GetSelectedLayoutName();
    if (!m_selectedLayout.empty())
    {
        m_loadLayout = true;
        EndDialog(IDOK);
    }
}

void CManageLayoutsDlg::OnRename(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    std::wstring oldName = GetSelectedLayoutName();
    if (oldName.empty())
        return;
    
    // Use the rename dialog to get a new name
    CRenameProcessDlg dlg(oldName);
    if (dlg.DoModal() == IDOK)
    {
        std::wstring newName = dlg.GetName();
        
        if (newName.empty() || newName == oldName)
            return;
        
        // Check if new name already exists
        if (m_logView.LayoutExists(newName))
        {
            MessageBox(L"A layout with that name already exists.", L"Error", MB_OK | MB_ICONWARNING);
            return;
        }
        
        // Load the layout data
        m_logView.LoadColumnLayout(oldName);
        
        // Save with new name
        m_logView.SaveColumnLayout(newName);
        
        // Delete old layout
        m_logView.DeleteColumnLayout(oldName);
        
        // Refresh the list
        PopulateList();
        
        // Select the renamed item
        int index = m_listBox.FindStringExact(-1, newName.c_str());
        if (index != LB_ERR)
        {
            m_listBox.SetCurSel(index);
        }
    }
}

void CManageLayoutsDlg::OnDelete(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    std::wstring layoutName = GetSelectedLayoutName();
    if (layoutName.empty())
        return;
    
    std::wstring msg = L"Delete layout '" + layoutName + L"'?";
    if (MessageBox(msg.c_str(), L"Confirm Delete", MB_YESNO | MB_ICONQUESTION) == IDYES)
    {
        m_logView.DeleteColumnLayout(layoutName);
        
        // Refresh the list
        PopulateList();
        UpdateButtonStates();
    }
}

void CManageLayoutsDlg::OnListSelChange(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    UpdateButtonStates();
}

void CManageLayoutsDlg::OnListDblClick(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    // Double-click loads the layout
    OnLoad(0, 0, CWindow());
}

std::wstring CManageLayoutsDlg::GetSelectedLayout() const
{
    return m_selectedLayout;
}

bool CManageLayoutsDlg::ShouldLoadLayout() const
{
    return m_loadLayout;
}

} // namespace debugviewpp
} // namespace fusion