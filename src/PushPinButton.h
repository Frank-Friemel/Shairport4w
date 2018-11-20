/*
 *
 *  PushPinButton.h
 *
 */

#pragma once

class CPushPinButton : public CWindowImpl<CButton>
{
public:
	CPushPinButton();
   
	void SetPinned(BOOL bPinned);
	BOOL IsPinned() { return m_bPinned; };
   
	void ReloadBitmaps(); 
   
	BOOL SubclassWindow(HWND hWnd);

protected:
	BEGIN_MSG_MAP_EX(CPushPinButton)
		MSG_OCM_DRAWITEM(OnDrawItem)
	END_MSG_MAP()
   
	void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
   
	void SizeToContent();
	void LoadBitmaps();
   
	BOOL    m_bPinned;
	Bitmap*	m_pPinnedBitmap;
	Bitmap*	m_pUnPinnedBitmap;
	CRect   m_MaxRect;
};
