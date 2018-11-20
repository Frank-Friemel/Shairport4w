/*
 *
 *  AboutDlg.h 
 *
 */

#pragma once

class CAboutDlg : public CDialogImpl<CAboutDlg>, public CWinDataExchange<CAboutDlg>
{
public:
	enum { IDD = IDD_ABOUTBOX };

protected:
	BEGIN_DDX_MAP(CAboutDlg)
		DDX_TEXT(IDC_STATIC_VERSION, m_strVersion)
	END_DDX_MAP()

	BEGIN_MSG_MAP_EX(CAboutDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	ATL::CString		m_strVersion;

private:
	HICON				m_hIconSmall;
};
