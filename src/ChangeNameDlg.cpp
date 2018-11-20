/*
 *
 *  ChangeNameDlg.cpp 
 *
 */

#include "stdafx.h"
#include "resource.h"

#include "changenamedlg.h"

ChangeNameDlg::ChangeNameDlg(ATL::CString airportName, ATL::CString password) :
	m_Name(airportName), m_Password(password), m_VerifyPassword(password)
{
}

ATL::CString ChangeNameDlg::getAirportName()
{
	return m_Name;
}

ATL::CString ChangeNameDlg::getPassword()
{
	return m_Password;
}

LRESULT ChangeNameDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow(GetParent());

	// set icons
	HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	m_hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(m_hIconSmall, FALSE);

	DoDataExchange(FALSE);

	return TRUE;
}

LRESULT ChangeNameDlg::OnOkay(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	DoDataExchange(TRUE);

	if (m_Password != m_VerifyPassword)
	{
		MessageBox(_T("Password fields do not contain matching strings."), _T("Error"), MB_ICONERROR | MB_OK);
		return 0;
	}

	this->EndDialog(TRUE);
	return 0;
}

LRESULT ChangeNameDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	this->EndDialog(FALSE);
	return 0;
}
