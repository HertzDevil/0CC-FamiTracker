/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.  To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/


#pragma once

#include "stdafx.h"		// // //

// Various custom controls

// Edit controls that can be enabled by double clicking
class CLockedEdit : public CEdit {
	DECLARE_DYNAMIC(CLockedEdit)
protected:
	DECLARE_MESSAGE_MAP()
public:
	CLockedEdit() = default;
	bool IsEditable() const;
	bool Update();
	int GetValue() const;
private:
	void OnLButtonDblClk(UINT nFlags, CPoint point);
	void OnSetFocus(CWnd* pOldWnd);
	void OnKillFocus(CWnd* pNewWnd);
private:
	bool m_bUpdate = false;
	int m_iValue = 0;
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};


// Edit controls that displays a banner when empty
class CBannerEdit : public CEdit {
	DECLARE_DYNAMIC(CBannerEdit)
protected:
	DECLARE_MESSAGE_MAP()
public:
	CBannerEdit(UINT nID) : CEdit() { m_strText.LoadStringW(nID); }
protected:
	CStringW m_strText;
protected:
	static const WCHAR BANNER_FONT[];
	static const COLORREF BANNER_COLOR;
public:
	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
};
