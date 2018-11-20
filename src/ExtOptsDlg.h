/*
 *
 *  ExtOptsDlg.h 
 *
 */

#pragma once

#include "resource.h"

class CExtOptsDlg :	public CDialogImpl<CExtOptsDlg>, public CWinDataExchange<CExtOptsDlg>
{
public:
	CExtOptsDlg(BOOL bLogToFile, BOOL bNoMetaInfo, BOOL bNoMediaControls, int nPos, const CComVariant& varSoundcardId);
	~CExtOptsDlg(void);

	enum { IDD = IDD_EXT_OPTS };

	// Accessors
	BOOL			getLogToFile();
	BOOL			getNoMetaInfo();
	BOOL			getNoMediaControls();
	int				getPos();

protected:
	BEGIN_DDX_MAP(CMainDlg)
		DDX_CHECK(IDC_LOG_TO_FILE, m_bLogToFile)
		DDX_CHECK(IDC_NO_META_INFO, m_bNoMetaInfo)
		DDX_CHECK(IDC_NO_NEDIA_CONTROLS, m_bNoMediaControls)
	END_DDX_MAP()

	BEGIN_MSG_MAP_EX(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		MSG_WM_HSCROLL(OnHScroll)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	void OnHScroll(int, short, HWND);

private:
	BOOL								m_bLogToFile;
	int									m_nPos;
	BOOL								m_bNoMetaInfo;
	BOOL								m_bNoMediaControls;
	CComboBox							m_ctlSoundcard;
	HICON								m_hIconSmall;
	CTrackBarCtrl						m_ctlBuffering;
	CStatic								m_ctlBufferingLabel;
	std::map<int, CComVariant>		    m_mapSoundcardID;
public:	
	CComVariant							m_varSoundcardId;
};

