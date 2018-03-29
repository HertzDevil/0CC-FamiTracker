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
#include <vector>		// // //

class CSequence;		// // //
class CGraphEditorComponent;		// // //

// Graph editor base class
class CGraphEditor : public CWnd
{
public:
	explicit CGraphEditor(std::shared_ptr<CSequence> pSequence);		// // //
	virtual ~CGraphEditor() noexcept;
	DECLARE_DYNAMIC(CGraphEditor)

	std::shared_ptr<CSequence> GetSequence();		// // //
	std::shared_ptr<const CSequence> GetSequence() const;		// // //

	int GetItemCount() const;		// // //
	int GetCurrentPlayPos() const;		// // //

	void ItemModified();		// // //
	void OnHoverSequenceItem(int idx, int val);		// // //

	template <typename T, typename... Args>
	T &MakeGraphComponent(Args&&... args) {		// // //
		auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
		auto &x = *ptr;
		components_.push_back(std::move(ptr));
		return x;
	}

protected:
	virtual void Initialize();

public:		// // //
	static const int GRAPH_LEFT = 28;			// Left side marigin

protected:
	CWnd *m_pParentWnd = nullptr;
	const std::shared_ptr<CSequence> m_pSequence;		// // //
	CRect m_ClientRect;
	CBitmap m_Bitmap;		// // //
	CDC m_BackDC;		// // //
	int m_iLastPlayPos;
	int m_iCurrentPlayPos;

private:
	std::vector<std::unique_ptr<CGraphEditorComponent>> components_;		// // //
	CGraphEditorComponent *focused_ = nullptr;		// // //

protected:
	DECLARE_MESSAGE_MAP()
public:
	BOOL PreTranslateMessage(MSG* pMsg) override;		// // //
	afx_msg void OnPaint();		// // //
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	BOOL CreateEx(DWORD dwExStyle, LPCWSTR lpszClassName, LPCWSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, LPVOID lpParam = NULL);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
};

// Arpeggio graph editor
class CArpeggioGraphEditor : public CGraphEditor
{
public:
	DECLARE_DYNAMIC(CArpeggioGraphEditor)
	explicit CArpeggioGraphEditor(std::shared_ptr<CSequence> pSequence) : CGraphEditor(std::move(pSequence)) { }		// // //

	int GetGraphScrollOffset() const;		// // //

private:
	void Initialize() override;

private:
	SCROLLINFO MakeScrollInfo() const;		// // //

	static constexpr int ITEMS = 21;		// // //

	int m_iScrollOffset = 0;
	int m_iScrollMax;
	CScrollBar m_cScrollBar;		// // //

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
};
