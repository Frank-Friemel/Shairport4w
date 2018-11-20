/*
 *
 *  MainDlg.h 
 *
 */

#pragma once

#include "PushPinButton.h"
#include "MyBitmapButton.h"
#include "TrayIcon.h"
#include "DacpService.h"

#include "MyAppMessages.h"

class CMainDlg : public CDialogImpl<CMainDlg>, public CUpdateUI<CMainDlg>,
		public CMessageFilter, public CIdleHandler, public CWinDataExchange<CMainDlg>
		, public CTrayIconImpl<CMainDlg>, protected CServiceResolveCallback
		, protected CMyThread
{
	class CShowThread : public CMyThread
	{
	public:
		CShowThread() : CMyThread(false, EVENT_NAME_SHOW_WINDOW)
		{
			m_hWnd = NULL;
		}
		virtual void OnEvent()
		{		
			ATLASSERT(m_hWnd);
			::ShowWindow(m_hWnd, SW_SHOWNORMAL);
			::SetForegroundWindow(m_hWnd);
		}
		HWND	m_hWnd;
	};
	CShowThread	m_threadShow;

public:
	typedef enum
	{
		pause,
		play,
		ffw,
		rew,
		skip_next,
		skip_prev
	} typeMMState;

	CMainDlg() : m_ctlInfoBmp(this, 1), m_ctlTimeBarBmp(this, 2), m_bLogToFile(bLogToFile)
	{
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);

		char hostname[256];

		memset(hostname, 0, sizeof(hostname));

		if (gethostname(hostname, sizeof(hostname)-1) == 0 && strlen(hostname) > 0)
		{
			USES_CONVERSION;

			m_strHostname	= hostname;
			m_strApName		= A2CW(hostname); 
		}
		else
		{
			WCHAR wbuf[512];
			DWORD n = (sizeof(wbuf) / sizeof(WCHAR))-1;

			if (::GetComputerNameW(wbuf, &n))
			{
				if (n > 2)
				{
					wbuf[n] = L'\0';
					m_strApName = wbuf;
				}
			}
		}
		if (m_strApName.IsEmpty())
			m_strApName.LoadStringW(IDR_MAINFRAME);

		m_bStartMinimized					= FALSE;
		m_bAutostart						= FALSE;
		m_bTray								= TRUE;
		m_strPassword						= L"";
		m_bNoMetaInfo						= FALSE;
		m_bNoMediaControl					= FALSE;
		m_bInfoPanel						= TRUE;
		m_pRaopContext.reset();
		m_bmWmc								= NULL;
		m_bmProgressShadow					= NULL;
		m_bmAdShadow						= NULL;
		m_bmArtShadow						= NULL;
		m_bPin								= TRUE;
		m_strTimeMode						= "time_remaining";
		m_nCurPos							= 0;
		m_nMMState							= pause;
		m_bVisible							= false;
		m_bTrayTrackInfo					= TRUE;
		_m_nMMState							= pause;
		m_hAccelerator						= NULL;
		m_varSoundcardId					= CHairTunes::GetSoundId();
		m_bGlobalMMHook						= true;
	}
	enum { IDD = IDD_MAINDLG };


	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();

	inline bool HasMultimediaControl(std::shared_ptr<CDacpService>* ppResult = NULL)
	{
		bool bResult = false;

		m_mtxDacpService.Lock();
		auto i = m_mapDacpService.find(m_DACP_ID);
 
		if (i != m_mapDacpService.end())
		{
			bResult = true;

			if (ppResult)
				*ppResult = i->second;
		}
		m_mtxDacpService.Unlock();

		return bResult;
	}
	inline void SetDacpID(ic_string strDacpID, ic_string strActiveRemote)
	{
		bool bPostNewState = false;

		m_mtxDacpService.Lock();

		m_DACP_ID			= ToDacpID(strDacpID.c_str());
		m_strActiveRemote	= strActiveRemote;

		if (strDacpID.empty() || m_mapDacpService.find(m_DACP_ID) != m_mapDacpService.end())
			bPostNewState = true;

		m_mtxDacpService.Unlock();

		if (bPostNewState)
			PostMessage(WM_SET_MMSTATE);
	}
	inline bool IsCurrentContext(std::shared_ptr<CRaopContext>& pRaopContext)
	{
		return m_pRaopContext.get() == pRaopContext.get();
	}
	inline typeMMState GetMMState()
	{
		return _m_nMMState;
	}

	BEGIN_UPDATE_UI_MAP(CMainDlg)
		UPDATE_ELEMENT(ID_EDIT_TRAYTRACKINFO, UPDUI_MENUPOPUP)
	END_UPDATE_UI_MAP()

	BEGIN_DDX_MAP(CMainDlg)
		DDX_TEXT(IDC_APNAME, m_strApName)
		DDX_TEXT(IDC_PASSWORD, m_strPassword)
		DDX_CHECK(IDC_INFOPANEL, m_bInfoPanel)
		DDX_TEXT(IDC_STATIC_TIME_INFO, m_strTimeInfo)
		DDX_TEXT(IDC_STATIC_TIME_INFO_CURRENT, m_strTimeInfoCurrent)
		DDX_CONTROL(IDC_MM_PLAY, m_ctlPlay)
		DDX_CONTROL(IDC_MM_SKIP_PREV, m_ctlSkipPrev)
		DDX_CONTROL(IDC_MM_REW, m_ctlRew)
		DDX_CONTROL(IDC_MM_PAUSE, m_ctlPause)
		DDX_CONTROL(IDC_MM_FFW, m_ctlFFw)
		DDX_CONTROL(IDC_MM_SKIP_NEXT, m_ctlSkipNext)
		DDX_CONTROL(IDC_MM_MUTE, m_ctlMute)
	END_DDX_MAP()

public:
	HICON															m_hIconSmall;
	ATL::CString													m_strApName;
	ATL::CString													m_strPassword;
	CComVariant														m_varSoundcardId;
	std::string														m_strHostname;
	BOOL															m_bStartMinimized;
	bool															m_bGlobalMMHook;
	BOOL															m_bAutostart;
	BOOL															m_bTray;
	BOOL&															m_bLogToFile;
	BOOL															m_bNoMetaInfo;
	BOOL															m_bNoMediaControl;

	BOOL															m_bInfoPanel;
	BOOL															m_bPin;
	HCURSOR															m_hHandCursor;
	Bitmap*															m_bmWmc;
	Bitmap*															m_bmProgressShadow;
	Bitmap*															m_bmAdShadow;
	Bitmap*															m_bmArtShadow;
	ATL::CString													m_strTimeInfo;
	ATL::CString													m_strTimeInfoCurrent;
	std::string														m_strTimeMode;
	int																m_nCurPos;
	bool															m_bVisible;
	BOOL															m_bTrayTrackInfo;

protected:
	HACCEL															m_hAccelerator;
	std::shared_ptr<CRaopContext>								    m_pRaopContext;
	CMyMutex														m_mtxDacpService;
	ic_string														m_strActiveRemote;
	ULONGLONG														m_DACP_ID;
	std::map< ULONGLONG, std::shared_ptr<CDacpService> >			m_mapDacpService;
	typeMMState														_m_nMMState;
	__declspec(property(get=GetMMState,put=PutMMState))	typeMMState	m_nMMState;

	CMyMutex														m_mtxQueryConnectedHost;
	std::list<ic_string>									        m_listQueryConnectedHost;

	CBitmap															m_bmUpdate;

	CComVariant														m_strSetupVersion;
	ATL::CString													m_strReady;

	void PutMMState(typeMMState nMmState);

public:
	CButton															m_ctlUpdate;
	CButton															m_ctlSet;
	CEdit															m_ctlPassword;
	CStatic															m_ctlPanelFrame;
	CStatic															m_ctlControlFrame;
	CStatic															m_ctlStatusFrame;
	CStatic															m_ctlTimeInfo;
	CStatic															m_ctlStatusInfo;
	CContainedWindowT<CStatic>										m_ctlInfoBmp;
	CContainedWindowT<CStatic>										m_ctlTimeBarBmp;
	CEdit															m_ctlArtist;
	CEdit															m_ctlAlbum;
	CEdit															m_ctlTrack;
	CToolTipCtrl													m_ctlToolTipTrackName;
	CToolTipCtrl													m_ctlToolTipAlbumName;
	CToolTipCtrl													m_ctlToolTipArtistName;

	CPushPinButton													m_ctlPushPin;

	CMyBitmapButton													m_ctlPlay;
	CMyBitmapButton													m_ctlPause;
	CMyBitmapButton													m_ctlFFw;
	CMyBitmapButton													m_ctlRew;
	CMyBitmapButton													m_ctlSkipNext;
	CMyBitmapButton													m_ctlSkipPrev;
	CMyBitmapButtonEx												m_ctlMute;

	void ReadConfig();
	bool WriteConfig();

	BEGIN_MSG_MAP_EX(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER_EX(IDC_CHANGENAME, OnClickedSetApName)
		MSG_WM_SYSCOMMAND(OnSysCommand)
		COMMAND_ID_HANDLER_EX(IDC_SHOW, OnShow)
		COMMAND_ID_HANDLER_EX(IDC_EXTENDED_OPTIONS, OnClickedExtendedOptions)
		COMMAND_ID_HANDLER_EX(IDC_INFOPANEL, OnClickedInfoPanel)
		COMMAND_ID_HANDLER_EX(ID_EDIT_TRAYTRACKINFO, OnEditTrayTrackInfo)
		MESSAGE_HANDLER(WM_RAOP_CTX, OnSetRaopContext)
		MESSAGE_HANDLER(WM_INFO_BMP, OnInfoBitmap)
		MESSAGE_HANDLER(WM_PAUSE, Pause)
		MESSAGE_HANDLER(WM_RESUME, Resume)
		MESSAGE_HANDLER(WM_SONG_INFO, OnSongInfo)
		MESSAGE_HANDLER(WM_DACP_REG, OnDACPRegistered)
		MESSAGE_HANDLER(WM_SET_MMSTATE, OnSetMultimediaState)
		MESSAGE_HANDLER(WM_SET_CONNECT_STATE, OnSetConnectedState)
		MSG_WM_TIMER(OnTimer)
		COMMAND_ID_HANDLER_EX(IDC_PUSH_PIN, OnPin)
		MSG_WM_SHOWWINDOW(OnShowWindow)
		MSG_WM_LBUTTONDOWN(OnLButtonDown)
		MSG_WM_ACTIVATE(OnActivate)
		MSG_WM_SETCURSOR(OnSetCursor)
		COMMAND_ID_HANDLER_EX(IDC_MM_PLAY,		OnPlay)
		COMMAND_ID_HANDLER_EX(IDC_MM_SKIP_PREV, OnSkipPrev)
		COMMAND_ID_HANDLER_EX(IDC_MM_REW,		OnRewind)
		COMMAND_ID_HANDLER_EX(IDC_MM_PAUSE,		OnPause)
		COMMAND_ID_HANDLER_EX(IDC_MM_FFW,		OnForward)
		COMMAND_ID_HANDLER_EX(IDC_MM_SKIP_NEXT, OnSkipNext)
		COMMAND_ID_HANDLER_EX(IDC_MM_MUTE,		OnMute)
		COMMAND_ID_HANDLER_EX(IDC_MM_MUTE_ON,	OnMuteOnOff)
		COMMAND_ID_HANDLER_EX(IDC_MM_MUTE_OFF,	OnMuteOnOff)
		COMMAND_ID_HANDLER_EX(ID_REFRESH,		OnRefresh)
		COMMAND_ID_HANDLER_EX(ID_MENU_ENTRY_ID_ONLINE_UPDATE,		OnOnlineUpdate)
		MSG_WM_APPCOMMAND(OnAppCommand)
		MESSAGE_HANDLER(WM_POWERBROADCAST, OnPowerBroadcast)

		MSG_WM_NCHITTEST(OnNcHitTest)
		CHAIN_MSG_MAP(CUpdateUI<CMainDlg>)
		CHAIN_MSG_MAP(CTrayIconImpl<CMainDlg>) 
		REFLECT_NOTIFICATIONS()
	ALT_MSG_MAP(1)
		MSG_WM_PAINT(OnInfoBmpPaint)
	ALT_MSG_MAP(2)
		MSG_WM_PAINT(OnTimeBarBmpPaint)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnNcHitTest(CPoint point)
	{
		SetMsgHandled(FALSE);
		return FALSE;
	}
	void OnClickedSetApName(UINT, int, HWND);
	void OnSysCommand(UINT wParam, CPoint pt);
	void OnShow(UINT, int, HWND);
	void OnClickedExtendedOptions(UINT, int, HWND);
	LRESULT OnSetRaopContext(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	void OnClickedInfoPanel(UINT, int, HWND);
	void OnEditTrayTrackInfo(UINT, int, HWND);
	LRESULT OnInfoBitmap(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	void OnInfoBmpPaint(HDC wParam);
	void OnTimeBarBmpPaint(HDC wParam);
	BOOL OnSetCursor(CWindow wnd, UINT nHitTest, UINT message);
	void OnTimer(UINT_PTR nIDEvent);
	LRESULT Pause(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT Resume(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSongInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDACPRegistered(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);

	LRESULT OnSetMultimediaState(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnSetConnectedState(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	void OnPin(UINT, int nID, HWND);
	void OnShowWindow(BOOL bShow, UINT nStatus);
	void OnLButtonDown(UINT nFlags, CPoint point);
	void OnActivate(UINT nState, BOOL bMinimized, CWindow wndOther);
	LRESULT OnPowerBroadcast(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	BOOL OnAppCommand(HWND, short cmd, WORD uDevice, int dwKeys);

	bool OnPlay(UINT uMsg = 0, int nID = 0, HWND hWnd = NULL);
	bool OnSkipPrev(UINT uMsg = 0, int nID = 0, HWND hWnd = NULL);
	bool OnRewind(UINT uMsg = 0, int nID = 0, HWND hWnd = NULL);
	bool OnPause(UINT uMsg = 0, int nID = 0, HWND hWnd = NULL);
	bool OnForward(UINT uMsg = 0, int nID = 0, HWND hWnd = NULL);
	bool OnSkipNext(UINT uMsg = 0, int nID = 0, HWND hWnd = NULL);
	bool OnMute(UINT uMsg = 0, int nID = 0, HWND hWnd = NULL);
	bool OnMuteOnOff(UINT uMsg = 0, int nID = 0, HWND hWnd = NULL);

	void OnRefresh(UINT uMsg = 0, int nID = 0, HWND hWnd = NULL);

	void OnOnlineUpdate(UINT uMsg = 0, int nID = 0, HWND hWnd = NULL);

	void CloseDialog(int nVal);

protected:
	void StopStartService(bool bStopOnly = false);
	bool SendDacpCommand(const char* strCmd, bool bRetryIfFailed = true);

	inline void Pause()
	{
		BOOL bDummy;
		Pause(0, 0, 0, bDummy);
	}
	inline void Resume()
	{
		BOOL bDummy;
		Resume(0, 0, 0, bDummy);
	}
	virtual bool OnServiceResolved(CRegisterServiceEvent* pServiceEvent, const char* strHost, USHORT nPort);

	virtual void OnEvent();
};
