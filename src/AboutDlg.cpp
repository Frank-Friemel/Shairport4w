/*
 *
 *  AboutDlg.cpp 
 *
 */

#include "stdafx.h"
#include "resource.h"
#include "MyBitmapButton.h"

#include "aboutdlg.h"


typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

static bool _IsWow64()
{
	LPFN_ISWOW64PROCESS fnIsWow64Process;

	BOOL bIsWow64 = FALSE;

	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(::GetModuleHandleA("kernel32"),"IsWow64Process");

	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
		{
			bIsWow64 = FALSE;
		}
	}
	return bIsWow64 ? true : false;
}

static BOOL BrowseForDirectory(HWND hWnd, LPCTSTR szTitle, LPTSTR szPath, UINT uiFlag /*=0*/)
{
	BROWSEINFO bi;

	memset(&bi, 0, sizeof(bi));

	bi.hwndOwner		= hWnd;
	bi.pidlRoot			= NULL;
	bi.pszDisplayName	= szPath;
	bi.lpszTitle		= szTitle;
	bi.ulFlags			= BIF_RETURNONLYFSDIRS | uiFlag;
	bi.lpfn				= NULL;
	bi.lParam			= NULL;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

	if (pidl)
		return SHGetPathFromIDList(pidl, szPath);
	return FALSE;
}


LRESULT CAboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
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

LRESULT CAboutDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}
