/*
 *
 *  TrayIcon.h
 *
 */

#pragma once

// The version of the structure supported by Shell v7
typedef struct _NOTIFYICONDATA_4 
{
	DWORD cbSize;
	HWND hWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	HICON hIcon;
	TCHAR szTip[128];
	DWORD dwState;
	DWORD dwStateMask;
	TCHAR szInfo[256];
	union 
	{
		UINT uTimeout;
		UINT uVersion;
	} DUMMYUNIONNAME;
	TCHAR szInfoTitle[64];
	DWORD dwInfoFlags;
	GUID guidItem;
	HICON hBalloonIcon;
} NOTIFYICONDATA_4;

#ifndef NIIF_LARGE_ICON
#define NIIF_LARGE_ICON 0x00000020
#endif

#define WM_TRAYICON		(WM_APP+0)

template <class T>
class CTrayIconImpl
{
public:
	CTrayIconImpl()
	{
		m_bInit			= false;
		memset(&m_nid, 0, sizeof(m_nid));
	}
	~CTrayIconImpl()
	{
		RemoveTray();
	}

	bool InitTray(LPCTSTR lpszToolTip, HICON hIcon, UINT nID)
	{
		const CMyMutex::AutoSync sync(m_mtx);

		if (m_bInit)
			return true;

		T* pT = static_cast<T*>(this);
	
		_tcsncpy_s(m_nid.szTip, _countof(m_nid.szTip), lpszToolTip, _TRUNCATE);

		m_nid.cbSize			= sizeof(NOTIFYICONDATA_4);
		m_nid.hWnd				= pT->m_hWnd;
		m_nid.uID				= nID;
		m_nid.hIcon				= hIcon;
		m_nid.uFlags			= NIF_MESSAGE | NIF_ICON | NIF_TIP;
		m_nid.uCallbackMessage	= WM_TRAYICON;
		m_bInit					= Shell_NotifyIcon(NIM_ADD, (PNOTIFYICONDATA)&m_nid) ? true : false;

		return m_bInit;
	}
	void UpdateTrayIcon(HICON hIcon)
	{
		const CMyMutex::AutoSync sync(m_mtx);

		if (m_bInit)
		{
			T* pT = static_cast<T*>(this);

			if (pT->m_bTray)
			{
				if (hIcon == NULL)
					hIcon = pT->m_hIconSmall;

				m_nid.uFlags	= NIF_ICON;
				m_nid.hIcon		= hIcon;

				Shell_NotifyIcon(NIM_MODIFY, (PNOTIFYICONDATA)&m_nid);
			}
		}
	}
	void SetTooltipText(LPCTSTR pszTooltipText)
	{
		const CMyMutex::AutoSync sync(m_mtx);

		if (m_bInit && pszTooltipText)
		{
			m_nid.uFlags = NIF_TIP;
			_tcsncpy_s(m_nid.szTip, _countof(m_nid.szTip), pszTooltipText, _TRUNCATE);

			Shell_NotifyIcon(NIM_MODIFY, (PNOTIFYICONDATA)&m_nid);
		}
	}
	void SetInfoText(LPCTSTR pszInfoTitle, LPCTSTR pszInfoText, HICON hIcon = NULL, bool avoidDuplicates = true)
	{
		const CMyMutex::AutoSync sync(m_mtx);

		if (m_bInit && pszInfoText && *pszInfoText && (!avoidDuplicates || m_strPreviousInfoText != std::wstring(pszInfoText)))
		{
			m_nid.uFlags		= NIF_INFO;
			m_nid.dwInfoFlags	= NIIF_USER | NIIF_NOSOUND | (hIcon ? NIIF_LARGE_ICON : 0);
			m_nid.uTimeout		= 15000;

			if (pszInfoTitle)
				_tcsncpy_s(m_nid.szInfoTitle, _countof(m_nid.szInfoTitle), pszInfoTitle, _TRUNCATE);
			_tcsncpy_s(m_nid.szInfo, _countof(m_nid.szInfo), pszInfoText, _TRUNCATE);

			m_nid.hBalloonIcon = hIcon;

			Shell_NotifyIcon(NIM_MODIFY, (PNOTIFYICONDATA)&m_nid);

			if (avoidDuplicates)
			{
				m_strPreviousInfoText = pszInfoText;
			}
		}
	}
	void RemoveTray()
	{
		const CMyMutex::AutoSync sync(m_mtx);

		if (m_bInit)
		{
			m_nid.uFlags = 0;
			Shell_NotifyIcon(NIM_DELETE, (PNOTIFYICONDATA)&m_nid);
			m_bInit = false;
		}
	}

	BEGIN_MSG_MAP(CTrayIconImpl<T>)
		MESSAGE_HANDLER(WM_TRAYICON, OnTrayIcon)
	END_MSG_MAP()

	LRESULT OnTrayIcon(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		if (wParam != m_nid.uID)
			return 0;

		T* pT = static_cast<T*>(this);

		if (LOWORD(lParam) == WM_RBUTTONUP)
		{
			CMenu Menu;

			if (!Menu.LoadMenu(m_nid.uID))
				return 0;

			CMenuHandle Popup(Menu.GetSubMenu(0));

			CPoint pos;
			GetCursorPos(&pos);

			SetForegroundWindow(pT->m_hWnd);

			Popup.SetMenuDefaultItem(0, TRUE);

			Popup.TrackPopupMenu(TPM_LEFTALIGN, pos.x, pos.y, pT->m_hWnd);
			pT->PostMessage(WM_NULL);
			Menu.DestroyMenu();
		}
		else if (LOWORD(lParam) == WM_LBUTTONDBLCLK)
		{
			::ShowWindow(pT->m_hWnd, SW_SHOWNORMAL);
			::SetForegroundWindow(pT->m_hWnd);
		}
		return 0;
	}

private:
	bool				m_bInit;
	NOTIFYICONDATA_4	m_nid;
	std::wstring		m_strPreviousInfoText;
	CMyMutex			m_mtx;
};
