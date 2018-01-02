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
#include <memory>		// // //

class CMainFrame;

// The instrument list
class CInstrumentListCtrl : public CListCtrl {
	DECLARE_DYNAMIC(CInstrumentListCtrl)
protected:
	DECLARE_MESSAGE_MAP()
public:
	CInstrumentListCtrl(CMainFrame *pMainFrame);

	int GetInstrumentIndex(int Selection) const;
	int FindInstrument(int Index) const;
	void SelectInstrument(int Index);
	void SelectNextItem();
	void SelectPreviousItem();
	void InsertInstrument(int Index);
	void RemoveInstrument(int Index);
	void SetInstrumentName(int Index, LPCTSTR pName);		// // //

private:
	CMainFrame *m_pMainFrame;
	std::unique_ptr<CImageList> m_pDragImage;		// // //
	UINT m_nDragIndex;
	UINT m_nDropIndex;
	bool m_bDragging;

public:
	afx_msg void OnContextMenu(CWnd*, CPoint);
	afx_msg void OnAddInstrument();
	afx_msg void OnLvnBeginlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnKeydown(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};
