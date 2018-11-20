/*
 *
 *  ChangeNameDlg.h 
 *
 */

#pragma once


class ChangeNameDlg : public CDialogImpl<ChangeNameDlg>, public CWinDataExchange<ChangeNameDlg>
{
public:
	enum { IDD = IDD_CHANGEUSER };

	ChangeNameDlg(ATL::CString airportName, ATL::CString password);

	// Accessors
	ATL::CString getAirportName();
	ATL::CString getPassword();

protected:
	BEGIN_DDX_MAP(ChangeNameDlg)
		DDX_TEXT(IDC_NAME, m_Name)
		DDX_TEXT(IDC_PASS, m_Password)
		DDX_TEXT(IDC_VERIFYPASS, m_VerifyPassword)
		DDX_CHECK(IDC_START_MINIMIZED, m_bStartMinimized)
		DDX_CHECK(IDC_AUTOSTART, m_bAutostart)
		DDX_CHECK(IDC_TRAY, m_bTray)
	END_DDX_MAP()

	BEGIN_MSG_MAP(ChangeNameDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOkay)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOkay(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	HICON				m_hIconSmall;
	
	ATL::CString		m_Name;
	ATL::CString		m_Password;
	ATL::CString		m_VerifyPassword;
public:
	BOOL				m_bStartMinimized;
	BOOL				m_bAutostart;
	BOOL				m_bTray;

};
