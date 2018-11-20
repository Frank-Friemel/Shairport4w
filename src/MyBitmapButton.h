/*
 *
 *  MyBitmapButton.h
 *
 */

#pragma once

class CMyBitmapButton : public CBitmapButtonImpl<CMyBitmapButton>
{
public:
	DECLARE_WND_SUPERCLASS(_T("WTL_BitmapButton"), GetWndClassName())

	CMyBitmapButton(DWORD dwExtendedStyle = BMPBTN_AUTOSIZE, HIMAGELIST hImageList = NULL) : 
		CBitmapButtonImpl<CMyBitmapButton>(dwExtendedStyle, hImageList)
	{
		m_bmNormal		= NULL;
		m_bmDisabled	= NULL;
		m_bmPressed		= NULL;
	}
	~CMyBitmapButton()
	{
	}
	static bool Resize(Bitmap& bmToResize, const CSize& rectDest, HBITMAP* bmResult, COLORREF dwBkCol, bool bFillBk = false);

	BEGIN_MSG_MAP(CMyBitmapButton)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MSG_WM_DESTROY(OnDestroy)
		CHAIN_MSG_MAP(__super) 
	END_MSG_MAP()

	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		if (wParam)
		{
			CDCHandle dcPaint((HDC)wParam);

			CRect rect;
			GetClientRect(&rect);

			CBrush br;

			br.CreateSolidBrush(::GetSysColor(COLOR_3DFACE));

			dcPaint.FillRect(&rect, br);
		}
		return 1;
	}
	void Set(int nNormalID, int PressedID, int nDisabledID, HINSTANCE hInstance = _Module.GetResourceInstance());
	void Set(CSize szDest, int nNormalID, int PressedID, int nDisabledID, HINSTANCE hInstance = _Module.GetResourceInstance());

	virtual void OnDestroy()
	{
		if (m_bmNormal)
		{
			delete m_bmNormal;
			m_bmNormal = NULL;
		}
		if (m_bmDisabled)
		{
			delete m_bmDisabled;
			m_bmDisabled = NULL;
		}
		if (m_bmPressed)
		{
			delete m_bmPressed;
			m_bmPressed = NULL;
		}
		SetMsgHandled(FALSE);
	}
public:
	Bitmap*								m_bmNormal;
	Bitmap*								m_bmDisabled;
	Bitmap*								m_bmPressed;
};

/////////////////////////////////////////////////////////////////////////////
// CMyBitmapButtonEx

class CMyBitmapButtonEx : public CMyBitmapButton
{
public:
	CMyBitmapButtonEx()
	{
		m_bmSelected	= NULL;
		m_bSelected		= false;
	}
	void Add(int nSelID, HINSTANCE hInstance = _Module.GetResourceInstance())
	{
		m_bmSelected	= BitmapFromResource(hInstance, MAKEINTRESOURCE(nSelID));
		
		if (m_bmSelected == NULL)
			return;

		HBITMAP		hbm = NULL;
		Color		bck_col(::GetSysColor(COLOR_3DFACE));

		m_bmSelected->GetHBITMAP(bck_col, &hbm);
		m_nSel = m_ImageList.Add(hbm);
	}
	void SetSelected(bool bSelected)
	{
		if (m_bSelected != bSelected)
		{
			m_bSelected = bSelected;
			m_nImage[_nImageNormal] = bSelected ? m_nSel : 0;
			Invalidate();
		}
	}
protected:
	virtual void OnDestroy()
	{
		__super::OnDestroy();

		if (m_bmSelected)
		{
			delete m_bmSelected;
			m_bmSelected = NULL;
		}
	}
public:
	Bitmap*								m_bmSelected;
	int									m_nSel;
	bool								m_bSelected;
};
