/*
 *
 *  MainDlg.cpp 
 *
 */
#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "MainDlg.h"
#include "ExtOptsDlg.h"
#include "ChangeNameDlg.h"

static const	ATL::CString	c_strEmptyTimeInfo = _T("--:--");

extern			BYTE			hw_addr[6];
volatile		LONG			g_bMute = 0;

#define STR_EMPTY_TIME_INFO c_strEmptyTimeInfo
#define	REPLACE_CONTROL_VERT(__ctl,___offset) 	__ctl.GetClientRect(&rect);							\
											__ctl.MapWindowPoints(m_hWnd, &rect);				\
											__ctl.SetWindowPos(NULL, rect.left, rect.top+(___offset), 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_FRAMECHANGED); 
#define	REPLACE_CONTROL_HORZ(__ctl,___offset) 	__ctl.GetClientRect(&rect);							\
											__ctl.MapWindowPoints(m_hWnd, &rect);				\
											__ctl.SetWindowPos(NULL, rect.left+(___offset), rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_FRAMECHANGED); 
#define		TIMER_ID_PROGRESS_INFO		1001


/////////////////////////////////////////////////////////
// static helper functions

static BOOL CreateToolTip(HWND nWndTool, CToolTipCtrl& ctlToolTipCtrl, int nIDS)
{
	ATLASSERT(ctlToolTipCtrl.m_hWnd == NULL);

	ctlToolTipCtrl.Create(::GetParent(nWndTool));

	CToolInfo ti(TTF_SUBCLASS, nWndTool, 0, NULL, MAKEINTRESOURCE(nIDS));
	
	return ctlToolTipCtrl.AddTool(ti);
}

typedef DWORD _ARGB;

static void InitBitmapInfo(__out_bcount(cbInfo) BITMAPINFO *pbmi, ULONG cbInfo, LONG cx, LONG cy, WORD bpp)
{
	ZeroMemory(pbmi, cbInfo);

	pbmi->bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);
	pbmi->bmiHeader.biPlanes		= 1;
	pbmi->bmiHeader.biCompression	= BI_RGB;
	pbmi->bmiHeader.biWidth			= cx;
	pbmi->bmiHeader.biHeight		= cy;
	pbmi->bmiHeader.biBitCount		= bpp;
}

static HRESULT Create32BitHBITMAP(HDC hdc, const SIZE *psize, __deref_opt_out void **ppvBits, __out HBITMAP* phBmp)
{
	*phBmp = NULL;

	BITMAPINFO bmi;

	InitBitmapInfo(&bmi, sizeof(bmi), psize->cx, psize->cy, 32);

	HDC hdcUsed = hdc ? hdc : GetDC(NULL);

	if (hdcUsed)
	{
		*phBmp = CreateDIBSection(hdcUsed, &bmi, DIB_RGB_COLORS, ppvBits, NULL, 0);
		
		if (hdc != hdcUsed)
		{
			ReleaseDC(NULL, hdcUsed);
		}
	}
	return (NULL == *phBmp) ? E_OUTOFMEMORY : S_OK;
}

static HRESULT AddBitmapToMenuItem(HMENU hmenu, int iItem, BOOL fByPosition, HBITMAP hbmp)
{
	HRESULT hr = E_FAIL;

	MENUITEMINFO mii = { sizeof(mii) };
	mii.fMask = MIIM_BITMAP;
	mii.hbmpItem = hbmp;

	if (SetMenuItemInfo(hmenu, iItem, fByPosition, &mii))
	{
		hr = S_OK;
	}
	return hr;
}

static HRESULT MyLoadIconMetric(HINSTANCE hinst, PCWSTR pszName, int lims, __out HICON *phico)
{
	HRESULT	hr		= E_FAIL;
	HMODULE hMod	= LoadLibraryW(L"Comctl32.dll");

	if (hMod)
	{
		typedef HRESULT (WINAPI *_LoadIconMetric)(HINSTANCE, PCWSTR, int, __out HICON*);

		_LoadIconMetric pLoadIconMetric = (_LoadIconMetric)GetProcAddress(hMod, "LoadIconMetric");

		if (pLoadIconMetric)
		{
			hr = pLoadIconMetric(hinst, pszName, lims, phico);
		}
	}
	return hr;
}

static void AddIconToMenu(HMENU hMenu, int nPos, HINSTANCE hinst, PCWSTR pszName)
{
	IWICImagingFactory* pFactory = NULL;	
 
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
 
	if (SUCCEEDED(hr))
	{
		HICON hicon = NULL;

		if (SUCCEEDED(MyLoadIconMetric(hinst, pszName, 0 /*LIM_SMALL*/, &hicon)))
		{
			IWICBitmap *pBitmap = NULL;
 
			hr = pFactory->CreateBitmapFromHICON(hicon, &pBitmap);
 
			if (SUCCEEDED(hr))
			{
				IWICFormatConverter *pConverter = NULL;
				
				hr = pFactory->CreateFormatConverter(&pConverter);

				if (SUCCEEDED(hr))
				{
					hr = pConverter->Initialize(pBitmap, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.0f, WICBitmapPaletteTypeCustom);

					UINT cx;
					UINT cy;
 
					hr = pBitmap->GetSize(&cx, &cy);
 
					if (SUCCEEDED(hr))
					{
						HBITMAP		hbmp	= NULL;
						const SIZE	sizIcon = { (int)cx, -(int)cy };
 
						BYTE* pbBuffer = NULL;
 
						hr = Create32BitHBITMAP(NULL, &sizIcon, reinterpret_cast<void **>(&pbBuffer), &hbmp);
 
						if (SUCCEEDED(hr))
						{
							const UINT cbStride = cx * sizeof(_ARGB);
							const UINT cbBuffer = cy * cbStride;
 
							hr = pBitmap->CopyPixels(NULL, cbStride, cbBuffer, pbBuffer);

							if (SUCCEEDED(hr))
							{
								AddBitmapToMenuItem(hMenu, nPos, TRUE, hbmp);
							}
							else
							{
								DeleteObject(hbmp);
							}
						}
					}
					pConverter->Release();
				}
				pBitmap->Release();
			}
		}
		pFactory->Release();
	}
}

/////////////////////////////////////////////////////////////////////
// CMainDlg

void CMainDlg::ReadConfig()
{
	USES_CONVERSION;

	std::string _strApName;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "ApName", _strApName))
		m_strApName = A2CW_CP(_strApName.c_str(), CP_UTF8);

	std::string _strStartMinimized;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "StartMinimized", _strStartMinimized))
		m_bStartMinimized = _strStartMinimized != "no" ? TRUE : FALSE;

	std::string _strAutostart;
	std::string _strAutostartName(strConfigName);

	if (GetValueFromRegistry(HKEY_CURRENT_USER, _strAutostartName.c_str(), _strAutostart, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"))
		m_bAutostart = _strAutostart.empty() ? FALSE : TRUE;

	std::string _strTray;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "Tray", _strTray))
		m_bTray = _strTray != "no" ? TRUE : FALSE;

	std::string _strPassword;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "Password", _strPassword))
		m_strPassword = _strPassword.c_str();

	std::string _strLogToFile;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "LogToFile", _strLogToFile))
		m_bLogToFile = _strLogToFile != "no" ? TRUE : FALSE;

	std::string _strNoMetaInfo;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "NoMetaInfo", _strNoMetaInfo))
		m_bNoMetaInfo = _strNoMetaInfo != "no" ? TRUE : FALSE;	
	
	std::string _strNoMediaControl;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "NoMediaControl", _strNoMediaControl))
		m_bNoMediaControl = _strNoMediaControl != "no" ? TRUE : FALSE;	

	std::string _strSoundcardId;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "SoundcardId", _strSoundcardId))
	{
		m_varSoundcardId	= A2CW(_strSoundcardId.c_str());
		CHairTunes::SetSoundId(m_varSoundcardId);
	}
	std::string _strStartFillPos;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "StartFillPos", _strStartFillPos))
	{
		int n = atoi(_strStartFillPos.c_str());

		if (n > MAX_FILL)
			n = MAX_FILL;
		else if (n < MIN_FILL)
			n = MIN_FILL;

		CHairTunes::SetStartFill(n);
	}

	std::string _strInfoPanel;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "InfoPanel", _strInfoPanel))
		m_bInfoPanel = _strInfoPanel != "no" ? TRUE : FALSE;

	std::string _strPin;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "Pin", _strPin))
		m_bPin = _strPin != "no" ? TRUE : FALSE;

	std::string _strTimeMode;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "TimeMode", _strTimeMode))
		 m_strTimeMode = _strTimeMode;

	std::string _strTrayTrackInfo;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "TrayTrackInfo", _strTrayTrackInfo))
		m_bTrayTrackInfo = _strTrayTrackInfo != "no" ? TRUE : FALSE;

	std::string _strGlobalMMHook;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "GlobalMMHook", _strGlobalMMHook))
		m_bGlobalMMHook = _strGlobalMMHook != "no" ? true : false;
}

bool CMainDlg::WriteConfig()
{
	char buf[256];

	USES_CONVERSION;

	PutValueToRegistry(HKEY_CURRENT_USER, "InfoPanel", m_bInfoPanel ? "yes" : "no");
	PutValueToRegistry(HKEY_CURRENT_USER, "Pin", m_bPin ? "yes" : "no");
	PutValueToRegistry(HKEY_CURRENT_USER, "TimeMode", m_strTimeMode.c_str());
	PutValueToRegistry(HKEY_CURRENT_USER, "TrayTrackInfo", m_bTrayTrackInfo ? "yes" : "no");

	if (!PutValueToRegistry(HKEY_CURRENT_USER, "ApName", W2CA_CP(m_strApName, CP_UTF8)))
		return false;
	else
	{
		std::string strHwAddr = my_Base64::base64_encode(hw_addr, 6);

		ATLASSERT(!strHwAddr.empty());
		PutValueToRegistry(HKEY_CURRENT_USER, "HwAddr", strHwAddr.c_str());
	}
	if (!PutValueToRegistry(HKEY_CURRENT_USER, "StartMinimized", m_bStartMinimized ? "yes" : "no"))
		return false;
	if (!PutValueToRegistry(HKEY_CURRENT_USER, "Tray", m_bTray ? "yes" : "no"))
		return false;
	if (!PutValueToRegistry(HKEY_CURRENT_USER, "Password", T2CA(m_strPassword)))
		return false;
	if (!PutValueToRegistry(HKEY_CURRENT_USER, "LogToFile", m_bLogToFile ? "yes" : "no"))
		return false;
	if (!PutValueToRegistry(HKEY_CURRENT_USER, "NoMetaInfo", m_bNoMetaInfo ? "yes" : "no"))
		return false;
	if (!PutValueToRegistry(HKEY_CURRENT_USER, "NoMediaControl", m_bNoMediaControl ? "yes" : "no"))
		return false;

    ATLVERIFY(0 == _itoa_s(CHairTunes::GetStartFill(), buf, 10));
	
    if (!PutValueToRegistry(HKEY_CURRENT_USER, "StartFillPos", buf))
		return false;
	if (m_varSoundcardId.vt != VT_EMPTY)
	{
		PutValueToRegistry(HKEY_CURRENT_USER, L"SoundcardId", m_varSoundcardId);
	}
	else
	{
		RemoveValueFromRegistry(HKEY_CURRENT_USER, "SoundcardId");
	}
	CHairTunes::SetSoundId(m_varSoundcardId);

	char FilePath[MAX_PATH];
	ATLVERIFY(GetModuleFileNameA(NULL, FilePath, sizeof(FilePath)) > 0);

	if (m_bAutostart)
	{
		std::string _strAutostart = std::string("\"") + std::string(FilePath) + std::string("\"");

		std::string _strAutostartName(strConfigName);
	
		PutValueToRegistry(HKEY_CURRENT_USER, _strAutostartName.c_str(), _strAutostart.c_str(), "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	}
	else
	{
		std::string _strAutostartName(strConfigName);
		RemoveValueFromRegistry(HKEY_CURRENT_USER, _strAutostartName.c_str(), "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	}
	return true;
}

void CMainDlg::StopStartService(bool bStopOnly /* = false */)
{
	if (is_streaming())
	{
		::MessageBoxA(m_hWnd, "Current stream will be interrupted for reconfiguration, sorry!", strConfigName.c_str(), MB_ICONINFORMATION|MB_OK);
	}
	CWaitCursor wait;

	stop_serving();

	if (!bStopOnly)
	{
		Sleep(1000);
		start_serving();
	}
}

void CMainDlg::PutMMState(typeMMState nMmState)
{
	if (_m_nMMState != nMmState)
	{
		ATLASSERT(sizeof(unsigned int) == sizeof(_m_nMMState));
		InterlockedExchange((volatile unsigned int *)&_m_nMMState, (unsigned int)nMmState);
	
		BOOL bDummy = true;
		OnSetMultimediaState(0, 0, 0, bDummy);
	}
}

bool CMainDlg::SendDacpCommand(const char* strCmd, bool bRetryIfFailed /* = true */)
{
	ATLASSERT(strCmd);
	bool bResult = false;

	std::shared_ptr<CDacpService> dacpService;

	if (HasMultimediaControl(&dacpService))
	{
		CSocketBase client;

		_LOG("Trying to connect to DACP Host %s:%lu ... ", dacpService->m_strDacpHost.c_str(), (ULONG)ntohs(dacpService->m_nDacpPort));

		BOOL bConnected = client.Connect(dacpService->m_strDacpHost.c_str(), dacpService->m_nDacpPort);

		if (!bConnected)
		{
			dacpService->m_Event.Lock();
			dacpService->m_strDacpHost = dacpService->m_Event.m_strHost;
			dacpService->m_Event.Unlock();

			bConnected = client.Connect(dacpService->m_strDacpHost.c_str(), dacpService->m_nDacpPort);
		}
		if (bConnected)
		{
			_LOG("succeeded\n");

			CHttp request;

			request.Create();

			request.m_strMethod						= "GET";
			request.m_strUrl						= ic_string("/ctrl-int/1/") + ic_string(strCmd);
			request.m_mapHeader["Host"]				= m_strHostname + std::string(".local.");
			request.m_mapHeader["Active-Remote"]	= m_strActiveRemote.c_str();

			std::string strRequest = request.GetAsString(false);
						
			_LOG("Sending DACP request: %s\n", strCmd);

			client.Send(strRequest.c_str(), (int)strRequest.length());
						
			if (client.WaitForIncomingData(2000))
			{
				char buf[256];

				memset(buf, 0, sizeof(buf));
				client.recv(buf, sizeof(buf)-1);

				_LOG("Received DACP response: %s\n", buf);

				if (strstr(buf, "Forbidden"))
				{
					bResult = false;
					SetDacpID("", "");
					_LOG("Erased current DACP-ID, since we are not allowed to control this client\n");
				}
				else
					bResult = strstr(buf, "OK") ? true : false;
			}
			else
			{
				_LOG("Waiting for DACP response *timedout*\n");
			}
			client.Close();
		}
		else
		{
			_LOG("*failed*\n");

			if (bRetryIfFailed && dacpService->m_Event.IsValid())
			{
				_LOG("The DACP service port may have changed - try to resolve service again ... ");
							
				if (dacpService->m_Event.Resolve(this))
					bResult = SendDacpCommand(strCmd, false);
				else
					_LOG("Re-resolve failed!\n");
			}
		}
	}
	return bResult;
}

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
	if(m_hAccelerator)
	{
		if(::TranslateAccelerator(m_hWnd, m_hAccelerator, pMsg))
			return TRUE;
	}
	return CWindow::IsDialogMessage(pMsg);
}

BOOL CMainDlg::OnIdle()
{
	UIUpdateMenuBar();
	return FALSE;
}

void CMainDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	m_bVisible = bShow ? true : false;

	if (bShow)
		OnPin(0, 0, 0);
}

void CMainDlg::OnPin(UINT, int nID, HWND)
{
	if (nID != 0)
	{
		m_bPin = !m_bPin;

		PutValueToRegistry(HKEY_CURRENT_USER, "Pin", m_bPin ? "yes" : "no");
	}
	m_ctlPushPin.SetPinned(m_bPin ? TRUE : FALSE);
	SetWindowPos(m_bPin ? HWND_TOPMOST : HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	USES_CONVERSION;

	ATLVERIFY(CMyThread::Start());

	if (m_strHostname.empty())
	{
		m_strHostname = strConfigName.c_str();
	}

	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	m_hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(m_hIconSmall, FALSE);

	m_hAccelerator			= AtlLoadAccelerators(IDR_MAINFRAME);

	m_hHandCursor			= LoadCursor(NULL, IDC_HAND);
	m_strTimeInfo			= STR_EMPTY_TIME_INFO;
	m_strTimeInfoCurrent	= STR_EMPTY_TIME_INFO;

	m_bmWmc					= BitmapFromResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_WMC));
	m_bmProgressShadow		= BitmapFromResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_PROGRESS_SHADOW));
	m_bmAdShadow			= BitmapFromResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_AD_SHADOW));
	m_bmArtShadow			= BitmapFromResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_ART_SHADOW));

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);
	UIAddMenuBar(m_hWnd);

	m_threadShow.m_hWnd = m_hWnd;
	ATLVERIFY(m_threadShow.Start());

	if (m_bTray)
	{
		InitTray(A2CT(strConfigName.c_str()), m_hIconSmall, IDR_TRAY); 
	}
	m_ctlPassword.Attach(GetDlgItem(IDC_PASSWORD));
	m_ctlSet.Attach(GetDlgItem(IDC_CHANGENAME));
	m_ctlPanelFrame.Attach(GetDlgItem(IDC_STATIC_PANEL));
	m_ctlControlFrame.Attach(GetDlgItem(IDC_CONTROL_PANEL));
	m_ctlStatusFrame.Attach(GetDlgItem(IDC_STATUS_PANEL));
	m_ctlArtist.Attach(GetDlgItem(IDC_STATIC_ARTIST));
	m_ctlAlbum.Attach(GetDlgItem(IDC_STATIC_ALBUM));
	m_ctlTrack.Attach(GetDlgItem(IDC_STATIC_TRACK));

	m_ctlTimeInfo.Attach(GetDlgItem(IDC_STATIC_TIME_INFO));
	m_ctlPushPin.SubclassWindow(GetDlgItem(IDC_PUSH_PIN));
	m_ctlInfoBmp.SubclassWindow(GetDlgItem(IDC_INFO_BMP));
	m_ctlTimeBarBmp.SubclassWindow(GetDlgItem(IDC_TIME_BAR));

	m_ctlStatusInfo.Attach(GetDlgItem(IDC_STATIC_STATUS));

	ATLVERIFY(m_strReady.LoadString(ATL_IDS_IDLEMESSAGE));
	m_ctlStatusInfo.SetWindowText(m_strReady);

	CRect rect;
	
	m_ctlStatusFrame.GetClientRect(rect);
	m_ctlStatusFrame.MapWindowPoints(m_hWnd, rect);
	m_ctlStatusFrame.MoveWindow(rect.left, rect.top-1, rect.Width(), rect.Height()+1);

	m_ctlTimeBarBmp.GetClientRect(&rect);
	m_ctlTimeBarBmp.MapWindowPoints(m_hWnd, &rect);

	rect.left	-= 4;
	rect.top	-= 3;
	rect.right	+= 7;
	rect.bottom	+= 7;

	m_ctlTimeBarBmp.MoveWindow(rect, FALSE);

	m_ctlInfoBmp.SetWindowPos(NULL, 0, 0, 114, 114, SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);

	m_ctlInfoBmp.GetClientRect(&rect);
	m_ctlInfoBmp.MapWindowPoints(m_hWnd, &rect);

	rect.left	-= 3;
	rect.top	-= 3;
	rect.right	+= 8;
	rect.bottom	+= 9;

	m_ctlInfoBmp.MoveWindow(rect, FALSE);
	 
	GetDlgItem(IDC_INFOPANEL).EnableWindow(m_bNoMetaInfo ? FALSE : TRUE);

	m_ctlPlay.Set(IDB_PLAY, IDB_PLAY_PRESSED, IDB_PLAY_DISABLED);
	m_ctlPause.Set(IDB_PAUSE, IDB_PAUSE_PRESSED, IDB_PAUSE_DISABLED);
	m_ctlFFw.Set(IDB_FFW, IDB_FFW_PRESSED, IDB_FFW_DISABLED);
	m_ctlRew.Set(IDB_REW, IDB_REW_PRESSED, IDB_REW_DISABLED);
	m_ctlSkipNext.Set(IDB_SKIP_NEXT, IDB_SKIP_NEXT_PRESSED, IDB_SKIP_NEXT_DISABLED);
	m_ctlSkipPrev.Set(IDB_SKIP_PREV, IDB_SKIP_PREV_PRESSED, IDB_SKIP_PREV_DISABLED);
	m_ctlMute.Set(IDB_MUTE, IDB_MUTE_PRESSED, IDB_MUTE);
	m_ctlMute.Add(IDB_MUTE_PRESSED);

	DoDataExchange(FALSE);

	m_ctlSet.GetClientRect(&rect);
	m_ctlSet.MapWindowPoints(m_hWnd, &rect);
	m_ctlSet.SetWindowPos(NULL, rect.left, rect.top-1, rect.Width(), rect.Height()+2, SWP_NOZORDER | SWP_FRAMECHANGED);

	ATL::CString strMainFrame;

	if (!strMainFrame.LoadString(IDR_MAINFRAME) || strMainFrame != ATL::CString(A2CT(strConfigName.c_str())))
		SetWindowText(A2CT(strConfigName.c_str()));

	OnClickedInfoPanel(0, 0, NULL);

	SendMessage(WM_RAOP_CTX); 
	SendMessage(WM_SET_MMSTATE); 

	ATLVERIFY(CreateToolTip(m_ctlTrack, m_ctlToolTipTrackName, IDS_TRACK_LABEL));
	ATLVERIFY(CreateToolTip(m_ctlAlbum, m_ctlToolTipAlbumName, IDS_ALBUM_LABEL));
	ATLVERIFY(CreateToolTip(m_ctlArtist, m_ctlToolTipArtistName, IDS_ARTIST_LABEL));

	UISetCheck(ID_EDIT_TRAYTRACKINFO, m_bTrayTrackInfo);

	return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CMyThread::Stop();

	m_threadShow.Stop();
	
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	if (m_bmWmc)
	{
		delete m_bmWmc;
		m_bmWmc = NULL;
	}
	if (m_bmProgressShadow)
	{
		delete m_bmProgressShadow;
		m_bmProgressShadow = NULL;
	}
	if (m_bmAdShadow)
	{
		delete m_bmAdShadow;
		m_bmAdShadow = NULL;
	}
	if (m_bmArtShadow)
	{
		delete m_bmArtShadow;
		m_bmArtShadow = NULL;
	}
	if (m_pRaopContext)
		m_pRaopContext.reset();
	
	m_mtxDacpService.Lock();
	m_mapDacpService.clear();
	m_mtxDacpService.Unlock();

	return 0;
}

LRESULT CMainDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	WCHAR	FileName[MAX_PATH];
	CAboutDlg dlg;

	if (GetModuleFileNameW(NULL, FileName, sizeof(FileName)/sizeof(WCHAR)) > 0)
	{
		WCHAR strVersion[256];

		if (GetVersionInfo(FileName, strVersion, _countof(strVersion)))
			dlg.m_strVersion = strVersion;
	}
	dlg.DoModal();
	return 0;
}

LRESULT CMainDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add validation code 
	CloseDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
	CloseDialog(wID);
	return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
	DestroyWindow();
	::PostQuitMessage(nVal);
}

void CMainDlg::OnClickedSetApName(UINT, int, HWND)
{
	ChangeNameDlg dialog(m_strApName, m_strPassword);

	dialog.m_bTray				= m_bTray;
	dialog.m_bStartMinimized	= m_bStartMinimized;
	dialog.m_bAutostart			= m_bAutostart;

	if (dialog.DoModal() == IDOK)
	{
		bool bWriteConfig = false;

		if (m_strApName != dialog.getAirportName() || m_strPassword != dialog.getPassword())
		{
			m_strApName			= dialog.getAirportName();
			m_strPassword		= dialog.getPassword();

			bWriteConfig = true;
			StopStartService();
		}
		if (m_bStartMinimized != dialog.m_bStartMinimized)
		{
			m_bStartMinimized	= dialog.m_bStartMinimized;
			bWriteConfig		= true;
		}
		if (m_bAutostart != dialog.m_bAutostart)
		{
			m_bAutostart		= dialog.m_bAutostart;
			bWriteConfig		= true;
		}
		if (m_bTray	!= dialog.m_bTray)
		{
			bWriteConfig	= true;
			m_bTray			= dialog.m_bTray;

			if (m_bTray)
			{
				USES_CONVERSION;

				InitTray(A2CT(strConfigName.c_str()), m_hIconSmall, IDR_TRAY); 
			}
			else
				RemoveTray();
		}
		DoDataExchange(FALSE);

		if (bWriteConfig)	
			WriteConfig();
	}
}

void CMainDlg::OnSysCommand(UINT wParam, CPoint pt)
{
	SetMsgHandled(FALSE);

	if (m_bTray)
	{
		if (GET_SC_WPARAM(wParam) == SC_MINIMIZE)
		{
			ShowWindow(SW_HIDE);		
			SetMsgHandled(TRUE);

			OSVERSIONINFOEX Info;

			Info.dwOSVersionInfoSize = sizeof(Info);

#pragma warning( push )
#pragma warning( disable : 4996 )
	
			if (GetVersionEx((LPOSVERSIONINFO)&Info))
			{
				// no ack for Win 10
				if (Info.dwMajorVersion < 10)
				{			
					std::string strDummy;
			
					if (!GetValueFromRegistry(HKEY_CURRENT_USER, "TrayAck", strDummy))
					{
						ATL::CString strAck;
						USES_CONVERSION;

						ATLVERIFY(strAck.LoadString(IDS_TRAY_ACK));
				
						SetInfoText(A2CT(strConfigName.c_str()), strAck);
						PutValueToRegistry(HKEY_CURRENT_USER, "TrayAck", "ok");
					}
				}
			}
#pragma warning( pop )
		}
	}
}

void CMainDlg::OnShow(UINT, int, HWND)
{
	ShowWindow(SW_SHOWNORMAL);
	::SetForegroundWindow(m_hWnd);
}

void CMainDlg::OnClickedExtendedOptions(UINT, int, HWND)
{
	CExtOptsDlg dlg(m_bLogToFile, m_bNoMetaInfo, m_bNoMediaControl, CHairTunes::GetStartFill(), m_varSoundcardId);

	if (dlg.DoModal(m_hWnd) == IDOK)
	{
		USES_CONVERSION;


		const bool bRestartService =	(m_bLogToFile &&		!dlg.getLogToFile())
									||	(m_bNoMetaInfo			!= dlg.getNoMetaInfo())
									||	(m_bNoMediaControl		!= dlg.getNoMediaControls())
									||	(m_varSoundcardId		!= dlg.m_varSoundcardId)
									? true : false;

		if (m_bNoMetaInfo != dlg.getNoMetaInfo())
		{
			m_bNoMetaInfo	= dlg.getNoMetaInfo();
			m_bInfoPanel	= m_bNoMetaInfo ? FALSE : TRUE;

			DoDataExchange(FALSE);
			OnClickedInfoPanel(0, 0, NULL);

			GetDlgItem(IDC_INFOPANEL).EnableWindow(m_bInfoPanel);
		}

		m_bNoMediaControl		= dlg.getNoMediaControls();
		m_bLogToFile			= dlg.getLogToFile();
		m_bNoMetaInfo			= dlg.getNoMetaInfo();
		m_varSoundcardId		= dlg.m_varSoundcardId;

		CHairTunes::SetStartFill(dlg.getPos());

		WriteConfig();

		if (bRestartService)
		{
			StopStartService();
		}
	}
}

void CMainDlg::OnTimer(UINT_PTR nIDEvent)
{
	switch(nIDEvent)
	{
		case TIMER_ID_PROGRESS_INFO:
		{
			ATL::CString strTimeInfo		= STR_EMPTY_TIME_INFO;
			ATL::CString strTimeInfoCurrent	= STR_EMPTY_TIME_INFO;

			if (m_pRaopContext)
			{
				if (m_pRaopContext->TryLock())
				{
					if (m_pRaopContext->m_nTimeStamp != 0 && m_pRaopContext->m_nTimeTotal != 0)
					{
						time_t nCurPosInSecs = m_pRaopContext->m_nTimeCurrentPos 
											+ (::time(NULL) - m_pRaopContext->m_nTimeStamp)
											- 2;
						if (nCurPosInSecs >= 0)
						{
							int nNewCurPos = (int) ((nCurPosInSecs * 100) / m_pRaopContext->m_nTimeTotal);

							if (nNewCurPos > 100)
								nNewCurPos = 100;
	
							if (nNewCurPos != m_nCurPos)
							{
								m_nCurPos = nNewCurPos;
								m_ctlTimeBarBmp.Invalidate();
							}

							// allow max 2 secs overtime
							if (nCurPosInSecs - m_pRaopContext->m_nTimeTotal > 2)
								nCurPosInSecs = m_pRaopContext->m_nTimeTotal+2;

							if (m_pRaopContext->m_nDurHours == 0)
							{
								long nMinCurrent = (long)(nCurPosInSecs / 60);
								long nSecCurrent = (long)(nCurPosInSecs % 60);

								if (m_strTimeMode == std::string("time_total"))
								{
									strTimeInfoCurrent.Format(_T("%02d:%02d")	, nMinCurrent               , nSecCurrent);
									strTimeInfo.Format		 (_T("%02d:%02d")	, m_pRaopContext->m_nDurMins, m_pRaopContext->m_nDurSecs);
								}
								else
								{
									long nRemain = m_pRaopContext->m_nTimeTotal >= nCurPosInSecs 
																	? (long)(m_pRaopContext->m_nTimeTotal - nCurPosInSecs)
																	: (long)(nCurPosInSecs - m_pRaopContext->m_nTimeTotal);

									long nMinRemain = nRemain / 60;
									long nSecRemain = nRemain % 60;

									strTimeInfoCurrent.Format(_T("%02d:%02d"), nMinCurrent               , nSecCurrent);
									strTimeInfo.Format		 (_T("%s%02d:%02d")
																, nCurPosInSecs > m_pRaopContext->m_nTimeTotal ? _T("+") : _T("-")																			
																, nMinRemain				, nSecRemain);
								}
							}
							else
							{
								long	nHourCurrent  = (long)(nCurPosInSecs / 3600);
								time_t _nCurPosInSecs = nCurPosInSecs - (nHourCurrent*3600);

								long nMinCurrent = (long)(_nCurPosInSecs / 60);
								long nSecCurrent = (long)(_nCurPosInSecs % 60);

								if (m_strTimeMode == std::string("time_total"))
								{
									strTimeInfoCurrent.Format(_T("%02d:%02d:%02d")
																, nHourCurrent               , nMinCurrent               , nSecCurrent);
									strTimeInfo.Format		 (_T("%02d:%02d:%02d")
																, m_pRaopContext->m_nDurHours, m_pRaopContext->m_nDurMins, m_pRaopContext->m_nDurSecs);
								}
								else
								{
									long nRemain = m_pRaopContext->m_nTimeTotal >= nCurPosInSecs
																	? (long)(m_pRaopContext->m_nTimeTotal - nCurPosInSecs)
																	: (long)(nCurPosInSecs - m_pRaopContext->m_nTimeTotal);

									long nHourRemain = nRemain / 3600;
									nRemain -= (nHourRemain*3600);

									long nMinRemain = nRemain / 60;
									long nSecRemain = nRemain % 60;

									strTimeInfoCurrent.Format(_T("%02d:%02d:%02d")	
																, nHourCurrent               , nMinCurrent               , nSecCurrent);
									strTimeInfo.Format		 (_T("%s%02d:%02d:%02d")
																, nCurPosInSecs > m_pRaopContext->m_nTimeTotal ? _T("+") : _T("-")
																, nHourRemain				 , nMinRemain				 , nSecRemain);
								}
							}
						}
					}
					m_pRaopContext->Unlock();
				}
			}
			if (m_strTimeInfo != strTimeInfo)
			{
				m_strTimeInfo = strTimeInfo;
				DoDataExchange(FALSE, IDC_STATIC_TIME_INFO);
			}
			if (m_strTimeInfoCurrent != strTimeInfoCurrent)
			{
				m_strTimeInfoCurrent = strTimeInfoCurrent;
				DoDataExchange(FALSE, IDC_STATIC_TIME_INFO_CURRENT);

				if (m_strTimeInfoCurrent == STR_EMPTY_TIME_INFO)
				{
					if (0 != m_nCurPos)
					{
						m_nCurPos = 0;
						m_ctlTimeBarBmp.Invalidate();
					}
				}
			}
			return;
		}
		break;
	}
	SetMsgHandled(FALSE);
}

void  CMainDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	CWindow wnd = ::RealChildWindowFromPoint(m_hWnd, point);

	if (wnd.m_hWnd == m_ctlTimeInfo.m_hWnd)
	{
		if (m_pRaopContext && m_strTimeInfo != STR_EMPTY_TIME_INFO)
		{
			if (m_strTimeMode == std::string("time_total"))
				m_strTimeMode = std::string("time_remaining");
			else
				m_strTimeMode = std::string("time_total");

			OnTimer(TIMER_ID_PROGRESS_INFO);
			PutValueToRegistry(HKEY_CURRENT_USER, "TimeMode", m_strTimeMode.c_str());
		}
	}
	else
		SetMsgHandled(FALSE);
}

// Handler of WM_PAUSE
LRESULT CMainDlg::Pause(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// omit pause in case we're receiving this in order to a "FLUSH" while skipping
	if (wParam == 0 || (m_nMMState != skip_next && m_nMMState != skip_prev))
	{
		if (m_nMMState != pause)
		{
            CWindow::KillTimer(TIMER_ID_PROGRESS_INFO);
			m_nMMState = pause;
		}
	}
	return 0l;
}

// Handler of WM_RESUME
LRESULT CMainDlg::Resume(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// omit play in case we're receiving this in order to a "SET_PARAMETER/progress" while ffw or rew
	if (wParam == 0 || (m_nMMState != ffw && m_nMMState != rew))
	{
		if (m_nMMState != play)
		{
			m_nMMState = play;
            CWindow::SetTimer(TIMER_ID_PROGRESS_INFO, 1000);

			if (m_pRaopContext)
			{
				if (m_pRaopContext->m_pDecoder)
					m_pRaopContext->m_pDecoder->AllowAudioReconnect([this]() { if (m_nMMState == pause) { this->PostMessage(WM_RESUME); } });

				m_pRaopContext->Lock();
				ic_string strPeerIP = m_pRaopContext->m_strPeerIP;
				m_pRaopContext->Unlock();

				m_mtxQueryConnectedHost.Lock();
		
				m_listQueryConnectedHost.push_back(strPeerIP);
				SetEvent(m_hEvent);

				m_mtxQueryConnectedHost.Unlock();
			}
		}
	}
	return 0l;
}

void CMainDlg::OnEvent()
{
	ic_string strPeerIP;

	m_mtxQueryConnectedHost.Lock();
	
	if (!m_listQueryConnectedHost.empty())
	{
		strPeerIP = m_listQueryConnectedHost.front();
		m_listQueryConnectedHost.pop_front();
	}
	m_mtxQueryConnectedHost.Unlock();

	if (!strPeerIP.empty())
	{
		std::wstring strHostName;

		if (GetPeerName(strPeerIP.c_str(), strPeerIP.find('.') != ic_string::npos ? true : false, strHostName))
		{
			PostMessage(WM_SET_CONNECT_STATE, 0, (LPARAM) new ATL::CString(strHostName.c_str()));
		}
		else
		{
			USES_CONVERSION;

			// wait 2 secs
			if (WaitForSingleObject(m_hStopEvent, 2000) == WAIT_OBJECT_0)
				return;

			std::shared_ptr<CDacpService> dacpService;

			if (HasMultimediaControl(&dacpService) && !dacpService->m_strDacpHost.empty())
			{
				PostMessage(WM_SET_CONNECT_STATE, 0, (LPARAM) new ATL::CString(A2CW_CP(dacpService->m_strDacpHost.c_str(), CP_UTF8)));
			}
			else
			{
				PostMessage(WM_SET_CONNECT_STATE, 0, (LPARAM) new ATL::CString(A2CW(strPeerIP.c_str())));
			}
		}
	}
	else
	{
		ATLASSERT(FALSE);
	}
}

LRESULT CMainDlg::OnSetConnectedState(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	if (lParam != 0)
	{
		ATL::CString* pstrHostName = (ATL::CString*) lParam;

		if (m_pRaopContext)
		{
			ATL::CString strStatusText;

			ATLVERIFY(strStatusText.LoadString(IDS_STATUS_CONNECTED));

			int nLocalStuff = pstrHostName->Find(_T(".local."));

			if (nLocalStuff > 0)
				*pstrHostName = pstrHostName->Left(nLocalStuff);

			strStatusText += (*pstrHostName);

			strStatusText += _T("\"");
			m_ctlStatusInfo.SetWindowText(strStatusText);
		}
		else
		{
			m_ctlStatusInfo.SetWindowText(m_strReady);
		}
		delete pstrHostName;
	}
	else
	{
		if (m_pRaopContext)
		{
			ATL::CString strStatusText;

			ATLVERIFY(strStatusText.LoadString(IDS_STATUS_CONNECTED));
			int n = strStatusText.Find(_T(' '));

			if (n > 0)
			{
				strStatusText = strStatusText.Left(n);
				m_ctlStatusInfo.SetWindowText(strStatusText);
			}
			else
			{
				ATLASSERT(FALSE);
			}
		}
		else
		{
			m_ctlStatusInfo.SetWindowText(m_strReady);
		}
	}
	return 0l;
}

LRESULT CMainDlg::OnSetRaopContext(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	if (lParam)
	{
		if (m_pRaopContext)
			m_pRaopContext.reset();
		m_pRaopContext = *(std::shared_ptr<CRaopContext>*) lParam;
	}
	else
	{
		m_pRaopContext.reset();
	}
	PostMessage(WM_SET_CONNECT_STATE);

	// prepare info bmp
	m_ctlInfoBmp.Invalidate();

	// prepare time info
	m_strTimeInfo			= STR_EMPTY_TIME_INFO;
	m_strTimeInfoCurrent	= STR_EMPTY_TIME_INFO;
	m_nCurPos				= 0;

	m_ctlTimeBarBmp.Invalidate();
	DoDataExchange(FALSE, IDC_STATIC_TIME_INFO);
	DoDataExchange(FALSE, IDC_STATIC_TIME_INFO_CURRENT);

	// prepare song info
	BOOL bDummy = TRUE;
	OnSongInfo(0, 0, 0, bDummy);

	if (m_pRaopContext)
	{
		Resume();
	}
	else
	{
		Pause();
		PostMessage(WM_SET_MMSTATE);
	}
	return 0l;
}

LRESULT CMainDlg::OnSongInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	ATL::CString	str_daap_songalbum;
	ATL::CString	str_daap_songartist;	
	ATL::CString	str_daap_trackname;

	std::shared_ptr<CRaopContext> pRaopContext;

	if (lParam)
	{
		pRaopContext = *((std::shared_ptr<CRaopContext>*)lParam);
	}
	else
	{
		if (m_pRaopContext)
			pRaopContext = m_pRaopContext;
	}

	if (pRaopContext)
	{
		pRaopContext->Lock();

		str_daap_songalbum		= pRaopContext->m_str_daap_songalbum;
		str_daap_songartist		= pRaopContext->m_str_daap_songartist;
		str_daap_trackname		= pRaopContext->m_str_daap_trackname;
		
		pRaopContext->Unlock();
	}
	if (!lParam)
	{
		m_ctlArtist.SetWindowText(str_daap_songartist);
		m_ctlAlbum.SetWindowText(str_daap_songalbum);
		m_ctlTrack.SetWindowText(str_daap_trackname);

		m_ctlArtist.EnableWindow(!str_daap_songartist.IsEmpty());
		m_ctlAlbum.EnableWindow(!str_daap_songalbum.IsEmpty());
		m_ctlTrack.EnableWindow(!str_daap_trackname.IsEmpty());
	}
	if (	m_bTray && m_bTrayTrackInfo
		&&	(	!str_daap_songartist.IsEmpty()
			 || !str_daap_songalbum.IsEmpty()
			 || !str_daap_trackname.IsEmpty()
			 )
		)
	{
		if (!m_bVisible)
		{
			ATL::CString	strFmtNowPlaying;
			ATL::CString	strTrackLabel;
			ATL::CString	strAlbumLabel;
			ATL::CString	strArtistLabel;
			ATL::CString	strNowPlayingLabel;

			ATL::CString	strSep(_T(":\t"));
			ATL::CString	strNewLine(_T("\n"));

			ATLVERIFY(strTrackLabel.LoadString(IDS_TRACK_LABEL));
			ATLVERIFY(strAlbumLabel.LoadString(IDS_ALBUM_LABEL));
			ATLVERIFY(strArtistLabel.LoadString(IDS_ARTIST_LABEL));
			ATLVERIFY(strNowPlayingLabel.LoadString(IDS_NOW_PLAYING));

			if (!str_daap_songartist.IsEmpty())
			{
				if (!strFmtNowPlaying.IsEmpty())
					strFmtNowPlaying += strNewLine;

				strFmtNowPlaying += (strArtistLabel + strSep + str_daap_songartist);
			}
			if (!str_daap_trackname.IsEmpty())
			{
				if (!strFmtNowPlaying.IsEmpty())
					strFmtNowPlaying += strNewLine;

				strFmtNowPlaying += (strTrackLabel + strSep + str_daap_trackname);
			}
			if (!str_daap_songalbum.IsEmpty())
			{
				if (!strFmtNowPlaying.IsEmpty())
					strFmtNowPlaying += strNewLine;

				strFmtNowPlaying += (strAlbumLabel + strSep + str_daap_songalbum);
			}
			HICON hIcon = NULL;

			if (pRaopContext)
			{
				if (!pRaopContext->GetHICON(&hIcon))
				{
					hIcon = NULL;
				}
			}			
			if (!g_bMute || lParam)
				SetInfoText(strNowPlayingLabel, strFmtNowPlaying, hIcon);

			if (hIcon)
				::DestroyIcon(hIcon);
		}
	}
	return 0l;
}

void CMainDlg::OnInfoBmpPaint(HDC wParam)
{
	CRect rect;

	if (m_ctlInfoBmp.GetUpdateRect(&rect))
	{
		CPaintDC dcPaint(m_ctlInfoBmp.m_hWnd);

		if (!dcPaint.IsNull())
		{
			CBrush br;

			br.CreateSolidBrush(::GetSysColor(COLOR_3DFACE));

			dcPaint.FillRect(&rect, br);

			Graphics dcGraphics(dcPaint);

			CRect rectClient;

			m_ctlInfoBmp.GetClientRect(&rectClient);

			rect = rectClient;

			rect.left	+= 3;
			rect.top	+= 3;
			rect.right	-= 8;
			rect.bottom -= 9;

			Rect rClient(0, 0, rectClient.Width(), rectClient.Height());
			Rect r(rect.left, rect.top, rect.Width(), rect.Height());

			bool bDrawn = false;

			if (m_pRaopContext)
			{
				m_pRaopContext->Lock();

				if (m_pRaopContext->m_bmInfo)
				{
					dcGraphics.DrawImage(m_bmArtShadow, rClient);
					bDrawn = dcGraphics.DrawImage(m_pRaopContext->m_bmInfo, r) == Gdiplus::Ok ? true : false;
				}
				m_pRaopContext->Unlock();
			}
			if (!bDrawn)
			{
				CBrush			brShadow;
				brShadow.CreateSolidBrush(::GetSysColor(COLOR_BTNSHADOW));

				CBrushHandle oldBrush = dcPaint.SelectBrush(brShadow);
				
				dcPaint.RoundRect(rect, CPoint(16, 16));
				dcPaint.SelectBrush(oldBrush);

				dcGraphics.DrawImage(m_bmAdShadow, rClient);

				r.Inflate(-6, -6);
				dcGraphics.DrawImage(m_bmWmc, r);
			}
		}
	}
}

void CMainDlg::OnTimeBarBmpPaint(HDC wParam)
{
	CRect rect;

	if (m_ctlTimeBarBmp.GetUpdateRect(&rect))
	{
		CPaintDC dcPaint(m_ctlTimeBarBmp.m_hWnd);

		if (!dcPaint.IsNull())
		{
			Graphics		dcGraphics(dcPaint);
			CBrush			brFace;
			CBrush			brShadow;
			CBrush			brBar;
			CBrushHandle	oldBrush;
			CPoint			rnd(6, 6);
			CRect			rectClient;

			m_ctlTimeBarBmp.GetClientRect(&rectClient);

			Rect rClient(0, 0, rectClient.Width(), rectClient.Height());

			rect		= rectClient;
			rect.left	+= 4;
			rect.top	+= 3;
			rect.right	-= 7;
			rect.bottom -= 7;

			brFace.CreateSolidBrush(::GetSysColor(COLOR_3DFACE));
			brShadow.CreateSolidBrush(::GetSysColor(COLOR_BTNSHADOW));
			brBar.CreateSolidBrush(::GetActiveWindow() == m_hWnd ? ::GetSysColor(COLOR_GRADIENTACTIVECAPTION) : ::GetSysColor(COLOR_GRADIENTINACTIVECAPTION));
			
			CRgn rgn;

			rgn.CreateRoundRectRgn(rect.left, rect.top, rect.right+1, rect.bottom+1, rnd.x, rnd.y);

			dcPaint.SelectClipRgn(rgn, RGN_DIFF);
			dcPaint.FillRect(&rectClient, brFace);
			
			dcGraphics.DrawImage(m_bmProgressShadow, rClient);

			dcPaint.SelectClipRgn(rgn);

			CRect rectLeft(rect.TopLeft(), CSize((rect.Width() * m_nCurPos) / 100, rect.Height()));

			if (rectLeft.Width() > 2)
			{
				CRgn rgn1;

				rgn1.CreateRoundRectRgn(rectLeft.left, rectLeft.top, rectLeft.right+1, rectLeft.bottom+1, rnd.x, rnd.y);

				dcPaint.SelectClipRgn(rgn1, RGN_DIFF);

				oldBrush = dcPaint.SelectBrush(brShadow);
				dcPaint.RoundRect(rect, rnd);

				dcPaint.SelectClipRgn(rgn1);
				
				dcPaint.SelectBrush(brBar);
				dcPaint.RoundRect(rectLeft, rnd);
				
				dcPaint.SelectBrush(oldBrush);
			}
			else
			{
				oldBrush = dcPaint.SelectBrush(brShadow);
				dcPaint.RoundRect(rect, rnd);
				dcPaint.SelectBrush(oldBrush);
			}
		}
	}
}

void CMainDlg::OnActivate(UINT nState, BOOL bMinimized, CWindow wndOther)
{
	m_ctlTimeBarBmp.Invalidate();
	SetMsgHandled(FALSE);
}

BOOL CMainDlg::OnAppCommand(HWND, short cmd, WORD uDevice, int dwKeys)
{
	if (HasMultimediaControl())
	{
		switch(cmd)
		{
			case APPCOMMAND_MEDIA_NEXTTRACK:
			{
				if (m_ctlSkipNext.IsWindowEnabled())
				{
					OnSkipNext(0, 0, NULL);
					return TRUE;
				}
			}
			break;

			case APPCOMMAND_MEDIA_STOP:
			case APPCOMMAND_MEDIA_PAUSE:
			{
				if (m_ctlPause.IsWindowEnabled())
				{
					OnPause(0, 0, NULL);
					return TRUE;
				}
			}
			break;

			case APPCOMMAND_MEDIA_FAST_FORWARD:
			{
				if (m_ctlFFw.IsWindowEnabled())
				{
					OnForward(0, 0, NULL);
					return TRUE;
				}
			}
			break;

			case APPCOMMAND_MEDIA_PLAY:
			{
				if (m_ctlPlay.IsWindowEnabled())
				{
					OnPlay(0, 0, NULL);
					return TRUE;
				}
			}
			break;

			case APPCOMMAND_MEDIA_PLAY_PAUSE:
			{
				if (m_ctlPlay.IsWindowEnabled())
				{
					OnPlay(0, 0, NULL);
					return TRUE;
				}
				else if (m_ctlPause.IsWindowEnabled())
				{
					OnPause(0, 0, NULL);
					return TRUE;
				}
			}
			break;

			case APPCOMMAND_MEDIA_PREVIOUSTRACK:
			{
				if (m_ctlSkipPrev.IsWindowEnabled())
				{
					OnSkipPrev(0, 0, NULL);
					return TRUE;
				}
			}
			break;

			case APPCOMMAND_MEDIA_REWIND:
			{
				if (m_ctlRew.IsWindowEnabled())
				{
					OnRewind(0, 0, NULL);
					return TRUE;
				}
			}
			break;

			case APPCOMMAND_VOLUME_MUTE:
			{
				if (m_ctlMute.IsWindowEnabled())
				{
					OnMute(0, 0, NULL);
					return TRUE;
				}
			}
			break;		
		}
	}
	SetMsgHandled(FALSE);
	return FALSE;
}


LRESULT CMainDlg::OnPowerBroadcast(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	SetMsgHandled(FALSE);

	_LOG("OnPowerBroadcast: %d\n", wParam);

	// Prevent suspend or standby is we are streaming
	// Note that you only get to this message when system is timing out.  You
	// will not get it when a user forces a standby or suspend
	if ((wParam == PBT_APMQUERYSUSPEND || wParam == PBT_APMQUERYSTANDBY) &&
		is_streaming())
	{
		SetMsgHandled(TRUE);
		_LOG("OnPowerBroadcast: System wants to suspend and since we are streaming we deny the request\n");
		return BROADCAST_QUERY_DENY;
	}

	// If we did not get the query to know if we deny the standby/suspend
	// we have to close all clients because we will be suspended anyways
	// and all our connections will be closed by the OS.
	if ((wParam == PBT_APMSUSPEND || wParam == PBT_APMSTANDBY) &&
		is_streaming())
	{
		SetMsgHandled(TRUE);

		_LOG("OnPowerBroadcast: System is going to suspend and since we are streaming we will close the connections\n");

		stop_serving();
		Sleep(1000);
	}

	if (wParam == PBT_APMRESUMESUSPEND ||
		wParam == PBT_APMRESUMESTANDBY)
	{
		SetMsgHandled(TRUE);
		StopStartService();
	}
	return 0;
}

BOOL CMainDlg::OnSetCursor(CWindow wnd, UINT nHitTest, UINT message)
{
	SetMsgHandled(FALSE);
	return FALSE;
}

LRESULT CMainDlg::OnInfoBitmap(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_ctlInfoBmp.Invalidate();
	return 0l;
}

void CMainDlg::OnClickedInfoPanel(UINT, int, HWND)
{
	const int nOffsetValue = 134;

	DoDataExchange(TRUE, IDC_INFOPANEL);

	PutValueToRegistry(HKEY_CURRENT_USER, "InfoPanel", m_bInfoPanel ? "yes" : "no");

	CRect rect;
	GetWindowRect(&rect);

	if ((!m_bNoMetaInfo && m_bInfoPanel) && rect.Height() < 320)
	{
		SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height()+nOffsetValue, SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);

		m_ctlPanelFrame.GetWindowRect(&rect);
		m_ctlPanelFrame.SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height()+nOffsetValue, SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);

		m_ctlControlFrame.GetClientRect(&rect);
		m_ctlControlFrame.MapWindowPoints(m_hWnd, &rect);
		m_ctlControlFrame.SetWindowPos(NULL, rect.left, rect.top+nOffsetValue, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_FRAMECHANGED);

		REPLACE_CONTROL_VERT(m_ctlPlay,		nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlPause,	nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlFFw,		nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlRew,		nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlSkipNext,	nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlSkipPrev,	nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlMute	,	nOffsetValue)

		m_ctlInfoBmp.ShowWindow(SW_NORMAL);
		m_ctlArtist.ShowWindow(SW_NORMAL);
		m_ctlAlbum.ShowWindow(SW_NORMAL);
		m_ctlTrack.ShowWindow(SW_NORMAL);

		m_ctlInfoBmp.Invalidate();
	}
	else if ((m_bNoMetaInfo || !m_bInfoPanel) && rect.Height() > 280)
	{
		SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height()-nOffsetValue, SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);

		m_ctlPanelFrame.GetWindowRect(&rect);
		m_ctlPanelFrame.SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height()-nOffsetValue, SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);

		m_ctlControlFrame.GetClientRect(&rect);
		m_ctlControlFrame.MapWindowPoints(m_hWnd, &rect);
		m_ctlControlFrame.SetWindowPos(NULL, rect.left, rect.top-nOffsetValue, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_FRAMECHANGED);

		REPLACE_CONTROL_VERT(m_ctlPlay,		-nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlPause,	-nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlFFw,		-nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlRew,		-nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlSkipNext,	-nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlSkipPrev,	-nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlMute	,	-nOffsetValue)

		m_ctlInfoBmp.ShowWindow(SW_HIDE);
		m_ctlArtist.ShowWindow(SW_HIDE);
		m_ctlAlbum.ShowWindow(SW_HIDE);
		m_ctlTrack.ShowWindow(SW_HIDE);
	}
}

LRESULT CMainDlg::OnDACPRegistered(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	ATLASSERT(lParam);

	CRegisterServiceEvent* pEvent = (CRegisterServiceEvent*) lParam;

	_LOG("Service %sregistered: %s, %s\n", pEvent->m_bRegister ? "" : "de", pEvent->m_strService.c_str(), pEvent->m_strRegType.c_str());

	if (pEvent->m_bRegister)
	{
		pEvent->Resolve(this);
	}
	else
	{
		bool						bPostNewState = false;
		std::shared_ptr<CDacpService>	pDacpService;

		ULONGLONG DacpID = ToDacpID(pEvent->m_strService.c_str());

		m_mtxDacpService.Lock();

		if (DacpID == m_DACP_ID)
			bPostNewState = true;

		auto e = m_mapDacpService.find(DacpID);

		if (e != m_mapDacpService.end())
		{
			pDacpService = e->second;
			m_mapDacpService.erase(e);
		}
		m_mtxDacpService.Unlock();
		
		if (bPostNewState)
			PostMessage(WM_SET_MMSTATE);
	}
	delete pEvent;

	return 0l;
}

bool CMainDlg::OnServiceResolved(CRegisterServiceEvent* pServiceEvent, const char* strHost, USHORT nPort)
{
	ATLASSERT(pServiceEvent);

	_LOG("OnServiceResolved: %s  -  Host: %s : %lu\n", pServiceEvent->m_strService.c_str(), strHost, (ULONG)ntohs(nPort));

	bool bPostNewState = false;

	ULONGLONG DacpID = ToDacpID(pServiceEvent->m_strService.c_str());

	m_mtxDacpService.Lock();

	bool bNewItem = m_mapDacpService.find(DacpID) == m_mapDacpService.end() ? true : false;

	m_mapDacpService[DacpID] = std::make_shared<CDacpService>(strHost, nPort, pServiceEvent);
	ATLVERIFY(m_mapDacpService[DacpID]->m_Event.QueryHostAddress());

	if (bNewItem && DacpID == m_DACP_ID)
		bPostNewState = true;

	m_mtxDacpService.Unlock();

	if (bPostNewState)
		PostMessage(WM_SET_MMSTATE);
	
	return true;
}

LRESULT CMainDlg::OnSetMultimediaState(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	ATLTRACE(L"OnSetMultimediaState\n");

	if (m_ctlPlay.IsWindow())
	{
		if (HasMultimediaControl())
		{
			static const std::wstring strStates[] = { L"pause", L"play", L"ffw", L"rew", L"???", L"???" };

			ATLTRACE(L"OnSetMultimediaState: %s\n", strStates[_m_nMMState].c_str());
	
			switch(_m_nMMState)
			{
				case pause:
				{
					m_ctlPlay.EnableWindow(true);
					m_ctlPause.EnableWindow(false);
					m_ctlFFw.EnableWindow(false);
					m_ctlRew.EnableWindow(false);
					m_ctlSkipNext.EnableWindow(true);
					m_ctlSkipPrev.EnableWindow(true);
				}
				break;

				case play:
				{
					m_ctlPlay.EnableWindow(false);
					m_ctlPause.EnableWindow(true);
					m_ctlFFw.EnableWindow(true);
					m_ctlRew.EnableWindow(true);
					m_ctlSkipNext.EnableWindow(true);
					m_ctlSkipPrev.EnableWindow(true);
				}
				break;

				case ffw:
				{
					m_ctlPlay.EnableWindow(true);
					m_ctlPause.EnableWindow(true);
					m_ctlFFw.EnableWindow(false);
					m_ctlRew.EnableWindow(false);
					m_ctlSkipNext.EnableWindow(true);
					m_ctlSkipPrev.EnableWindow(true);
				}
				break;

				case rew:
				{
					m_ctlPlay.EnableWindow(true);
					m_ctlPause.EnableWindow(true);
					m_ctlFFw.EnableWindow(false);
					m_ctlRew.EnableWindow(false);
					m_ctlSkipNext.EnableWindow(true);
					m_ctlSkipPrev.EnableWindow(true);
				}
				break;
			}
		}
		else
		{
			m_ctlPlay.EnableWindow(false);
			m_ctlPause.EnableWindow(false);
			m_ctlFFw.EnableWindow(false);
			m_ctlRew.EnableWindow(false);
			m_ctlSkipNext.EnableWindow(false);
			m_ctlSkipPrev.EnableWindow(false);
		}
	}
	return 0l;
}

bool CMainDlg::OnPlay(UINT, int nID, HWND)
{
	bool bResult = false;

	switch(m_nMMState)
	{
		case play:
		break;

		case pause:
		{
			if (bResult = SendDacpCommand("play"))
				Resume();
		}
		break;

		case ffw:
		case rew:
		{
			if (bResult = SendDacpCommand("playresume"))
				Resume();
		}
		break;
	}
	return bResult;
}

bool CMainDlg::OnSkipPrev(UINT, int nID, HWND)
{
	bool bResult = false;

	switch(m_nMMState)
	{
		case play:
		{
			if (bResult = SendDacpCommand("previtem"))
			{
				if (m_pRaopContext && m_pRaopContext->m_nTimeTotal > 0)
				{
					m_nMMState = skip_prev;
				}
			}
		}
		break;

		case pause:
		{
			bResult = SendDacpCommand("previtem");
		}
		break;

		case ffw:
		case rew:
		{
			if (SendDacpCommand("playresume"))
				Resume();
			if (bResult = SendDacpCommand("previtem"))
				m_nMMState = skip_prev;
		}
		break;
	}
	return bResult;
}

bool CMainDlg::OnRewind(UINT, int nID, HWND)
{
	bool bResult = false;

	switch(m_nMMState)
	{
		case play:
		{
			if (bResult = SendDacpCommand("beginrew"))
			{
				if (m_pRaopContext && m_pRaopContext->m_nTimeTotal > 0)
				{
					m_nMMState = rew;
				}
			}
		}
		break;

		case pause:
		{
		}
		break;

		case ffw:
		case rew:
		{
			if (bResult = SendDacpCommand("playresume"))
				Resume();
		}
		break;
	}
	return bResult;
}

bool CMainDlg::OnPause(UINT, int /*nID*/, HWND)
{
	bool bResult = false;

	switch(m_nMMState)
	{
		case play:
		{
			if (bResult = SendDacpCommand("pause"))
				Pause();
		}
		break;

		case pause:
		{
		}
		break;

		case ffw:
		case rew:
		{
			if (SendDacpCommand("playresume"))
				Resume();

			if (bResult = SendDacpCommand("pause"))
				Pause();
		}
		break;
	}
	return bResult;
}

bool CMainDlg::OnForward(UINT, int nID, HWND)
{
	bool bResult = false;

	switch(m_nMMState)
	{
		case play:
		{
			if (bResult = SendDacpCommand("beginff"))
			{
				if (m_pRaopContext && m_pRaopContext->m_nTimeTotal > 0)
				{
					m_nMMState = ffw;
				}
			}
		}
		break;

		case pause:
		{
		}
		break;

		case ffw:
		case rew:
		{
			if (bResult = SendDacpCommand("playresume"))
				Resume();
		}
		break;
	}
	return bResult;
}

bool CMainDlg::OnSkipNext(UINT, int nID, HWND)
{
	bool bResult = false;

	switch(m_nMMState)
	{
		case play:
		{
			if (bResult = SendDacpCommand("nextitem"))
			{
				if (m_pRaopContext && m_pRaopContext->m_nTimeTotal > 0)
					m_nMMState = skip_next;
			}
		}
		break;

		case pause:
		{
			bResult = SendDacpCommand("nextitem");
		}
		break;

		case ffw:
		case rew:
		{
			if (SendDacpCommand("playresume"))
				Resume();
			if (bResult = SendDacpCommand("nextitem"))
				m_nMMState = skip_next;
		}
		break;
	}
	return bResult;
}

bool CMainDlg::OnMute(UINT, int nID, HWND)
{
	g_bMute ^= MUTE_FROM_TOGGLE_BUTTON;
	m_ctlMute.SetSelected(g_bMute & MUTE_FROM_TOGGLE_BUTTON);
	return true;
}

bool CMainDlg::OnMuteOnOff(UINT, int nID, HWND)
{
	if (IDC_MM_MUTE_ON == nID)
	{
		if ((g_bMute & MUTE_FROM_EXTERN) == 0)
		{
			g_bMute |= MUTE_FROM_EXTERN;
		}
	}
	else
	{
		ATLASSERT(nID == IDC_MM_MUTE_OFF);
		if (g_bMute & MUTE_FROM_EXTERN)
		{
			g_bMute &= ~MUTE_FROM_EXTERN;
		}
	}
	return true;
}

void CMainDlg::OnEditTrayTrackInfo(UINT, int, HWND)
{
	m_bTrayTrackInfo = !m_bTrayTrackInfo;
	UISetCheck(ID_EDIT_TRAYTRACKINFO, m_bTrayTrackInfo);

	PutValueToRegistry(HKEY_CURRENT_USER, "TrayTrackInfo", m_bTrayTrackInfo ? "yes" : "no");
}

void CMainDlg::OnRefresh(UINT uMsg /*= 0*/, int nID /*= 0*/, HWND hWnd /*= NULL*/)
{
	Invalidate();
	SetMsgHandled(FALSE);
}

void CMainDlg::OnOnlineUpdate(UINT uMsg /*= 0*/, int nID /*= 0*/, HWND hWnd /*= NULL*/)
{
}
