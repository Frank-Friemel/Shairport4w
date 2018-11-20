#include "StdAfx.h"
#include "PushPinButton.h"
#include "resource.h"

CPushPinButton::CPushPinButton()
{
	m_bPinned			= FALSE;
	m_MaxRect			= CRect(0, 0, 0, 0);
	m_pPinnedBitmap		= NULL;
	m_pUnPinnedBitmap	= NULL;
}

void CPushPinButton::ReloadBitmaps()
{
	LoadBitmaps();

	SizeToContent();

	GetParent().InvalidateRect(m_MaxRect);
	Invalidate();
}

void CPushPinButton::LoadBitmaps()
{
	if (m_pPinnedBitmap)
	{
		delete m_pPinnedBitmap;
		m_pPinnedBitmap = NULL;
	}
	if (m_pUnPinnedBitmap)
	{
		delete m_pUnPinnedBitmap;
		m_pUnPinnedBitmap = NULL;
	}
	m_pPinnedBitmap		= BitmapFromResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_PUSH_PIN_PRESSED));
	m_pUnPinnedBitmap	= BitmapFromResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_PUSH_PIN_RELEASED)); 
}

void CPushPinButton::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	ATLASSERT(lpDrawItemStruct);
  
	Bitmap* pBitmap;
	CRect rect(lpDrawItemStruct->rcItem);
	
	if (m_bPinned)
	{
		CDCHandle dc(lpDrawItemStruct->hDC);

		dc.Draw3dRect(&rect, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DHILIGHT));

		pBitmap = m_pPinnedBitmap;
	}
	else
		pBitmap = m_pUnPinnedBitmap;
  
	Graphics dcGraphics(lpDrawItemStruct->hDC);

	rect.DeflateRect(3, 3);

	Rect r(rect.left, rect.top, rect.Width(), rect.Height());

	dcGraphics.DrawImage(pBitmap, r);
}

void CPushPinButton::SetPinned(BOOL bPinned)
{
	m_bPinned = bPinned;
	Invalidate();
}

BOOL CPushPinButton::SubclassWindow(HWND hWnd)
{
	if (__super::SubclassWindow(hWnd))
	{
		ATLASSERT(GetWindowLong(GWL_STYLE) & BS_OWNERDRAW);

		LoadBitmaps();

		SizeToContent();

		return TRUE;
	}
	return FALSE;
}

void CPushPinButton::SizeToContent()
{
	if (m_pPinnedBitmap == NULL)
		return;

	m_MaxRect = CRect(0, 0, m_pPinnedBitmap->GetWidth(), m_pPinnedBitmap->GetHeight());
	
	m_MaxRect.InflateRect(3, 3);

	ClientToScreen(&m_MaxRect);

	CPoint p1(m_MaxRect.left, m_MaxRect.top);
	CPoint p2(m_MaxRect.right, m_MaxRect.bottom);
	
	HWND hParent = ::GetParent(m_hWnd);
	
	::ScreenToClient(hParent, &p1);
	::ScreenToClient(hParent, &p2);
	
	m_MaxRect = CRect(p1, p2);

	ATLVERIFY(SetWindowPos(NULL, -1, -1, m_MaxRect.Width(), m_MaxRect.Height(), 
						SWP_NOMOVE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOACTIVATE));
}
