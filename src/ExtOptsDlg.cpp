/*
 *
 *  ExtOptsDlg.cpp 
 *
 */

#include "StdAfx.h"
#include "ExtOptsDlg.h"
#include "AudioPlayer.h"

CExtOptsDlg::CExtOptsDlg(BOOL bLogToFile, BOOL bNoMetaInfo, BOOL bNoMediaControls, int nPos, const CComVariant& varSoundcardId) :
	m_bLogToFile(bLogToFile), m_bNoMetaInfo(bNoMetaInfo), m_bNoMediaControls(bNoMediaControls), m_nPos(nPos), m_varSoundcardId(varSoundcardId) 
{
}

CExtOptsDlg::~CExtOptsDlg(void)
{
}

LRESULT CExtOptsDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow();

	// set icons
	HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	m_hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(m_hIconSmall, FALSE);

	DoDataExchange(FALSE);

	m_ctlSoundcard.Attach(GetDlgItem(IDC_SOUNDCARD));

	ATL::CString strDefault;

	if (!strDefault.LoadString(IDS_SYSTEM_DEFAULT))
		strDefault = _T("system-default");

	m_ctlSoundcard.AddString(strDefault);

    std::map< UINT, std::pair<std::wstring, CComVariant> > mapDevices;

    CAudioPlayer::GetDeviceList(mapDevices);

    int n = 1;

    for (auto i = mapDevices.begin(); i != mapDevices.end(); ++i)
    {
        m_mapSoundcardID[n++] = i->second.second;
        m_ctlSoundcard.AddString(i->second.first.c_str());
    }

	switch (m_varSoundcardId.vt)
	{
		case VT_NULL:
		case VT_EMPTY:
		{
			m_ctlSoundcard.SetCurSel(0);
		}
		break;

		case VT_BSTR:
		{
			bool bByID = false;

			// by ID
			for (auto i = m_mapSoundcardID.begin(); i != m_mapSoundcardID.end(); ++i)
			{
				if (i->second == m_varSoundcardId)
				{
					m_ctlSoundcard.SetCurSel(i->first);
					bByID = true;
					break;
				}
			}
			if (!bByID)
			{
				// should be a number
				if (wcslen(m_varSoundcardId.bstrVal))
				{
					m_ctlSoundcard.SetCurSel(_wtoi(m_varSoundcardId.bstrVal));
				}
				else
				{
					// empty std::string -> default sound!
					m_ctlSoundcard.SetCurSel(0);
				}
			}
		}
		break;

		default:
		{
			m_ctlSoundcard.SetCurSel(m_varSoundcardId.lVal);
		}
		break;
	}

	m_ctlBuffering.Attach(GetDlgItem(IDC_BUFFERING));
	m_ctlBufferingLabel.Attach(GetDlgItem(IDC_STATIC_BUFFERING));

	m_ctlBuffering.SetRange(MIN_FILL, MAX_FILL, TRUE);
	m_ctlBuffering.SetPos(m_nPos);

	OnHScroll(0, 0, m_ctlBuffering.m_hWnd);

	return TRUE;
}

BOOL CExtOptsDlg::getLogToFile()
{
	return m_bLogToFile;
}

BOOL CExtOptsDlg::getNoMetaInfo()
{
	return m_bNoMetaInfo;
}

BOOL CExtOptsDlg::getNoMediaControls()
{
	return m_bNoMediaControls;
}

int CExtOptsDlg::getPos()
{
	return m_nPos;
}

LRESULT CExtOptsDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	auto nSoundcardId = m_ctlSoundcard.GetCurSel();

	if (nSoundcardId == CB_ERR || nSoundcardId == 0)
	{
		m_varSoundcardId.Clear();
	}
	else
	{
		m_varSoundcardId = m_mapSoundcardID[nSoundcardId];
		ATLASSERT(m_varSoundcardId.vt != VT_EMPTY);
	}
	m_nPos = m_ctlBuffering.GetPos();

	DoDataExchange(TRUE);
	EndDialog(IDOK);
	return 0;
}

LRESULT CExtOptsDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(IDCANCEL);
	return 0;
}

void CExtOptsDlg::OnHScroll(int, short, HWND)
{
	WCHAR buf[256];

	m_nPos = m_ctlBuffering.GetPos();

	ATL::CString strBuffering;

	strBuffering.LoadString(IDS_BUFFERING);

	swprintf_s(buf, 256, L"%s (%ld Frames)", (PCWSTR)strBuffering, m_nPos);
	m_ctlBufferingLabel.SetWindowTextW(buf);
}
