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

enum edit_t {
	EDIT_NONE,
	EDIT_LINE,
	EDIT_POINT,
	EDIT_LOOP,
	EDIT_RELEASE,
	EDIT_NOISE,		// // //
};

class CSequence;		// // //

// Graph editor base class
class CGraphEditor : public CWnd
{
public:
	explicit CGraphEditor(std::shared_ptr<CSequence> pSequence);		// // //
	DECLARE_DYNAMIC(CGraphEditor)

protected:
	virtual void Initialize();

	int GetItemWidth() const;
	int GetItemIndex(CPoint point) const;		// // //
	int GetItemGridIndex(CPoint point) const;		// // //
	virtual int GetItemTop() const = 0;		// // //
	virtual int GetItemBottom() const;		// // //
	virtual int GetItemHeight() const = 0;

	void ModifyItem(CPoint point, bool Redraw);		// // //
	void ItemModified(bool Redraw);		// // //

	virtual void ModifyLoopPoint(CPoint point, bool Redraw);
	virtual void ModifyReleasePoint(CPoint point, bool Redraw);
	virtual void ModifyReleased();
	void CursorChanged(int x);
	bool IsEditLine() const;

	virtual void DrawRange(CDC &DC, int Max, int Min);
	void DrawBackground(CDC &DC, int Lines, bool DrawMarks, int MarkOffset);		// // //
	void DrawLoopRelease(CDC &DC);		// // //
	void DrawLine(CDC &DC);

	template<COLORREF COL_BG1, COLORREF COL_BG2, COLORREF COL_EDGE1, COLORREF COL_EDGE2>
	void DrawRect(CDC &DC, int x, int y, int w, int h, bool flat);
	void DrawRect(CDC &DC, int x, int y, int w, int h);
	void DrawPlayRect(CDC &DC, int x, int y, int w, int h);
	void DrawCursorRect(CDC &DC, int x, int y, int w, int h);
	void DrawShadowRect(CDC &DC, int x, int y, int w, int h);

	void PaintBuffer(CDC &BackDC, CDC &FrontDC);

private:
	virtual int GetItemValue(CPoint point) const = 0;		// // //
	virtual int GetSequenceItemValue(int Index, int Value) const;		// // //

	void HighlightItem(CPoint point);		// // //

	void DrawTagPoint(CDC &DC, int index, LPCWSTR str, COLORREF col);		// // //

protected:
	static const int GRAPH_LEFT = 28;			// Left side marigin
	static const int GRAPH_BOTTOM = 5;			// Bottom marigin
	static const int ITEM_MAX_WIDTH = 40;
	static const int COLOR_LINES = 0x404040;

protected:
	CWnd *m_pParentWnd;
	const std::shared_ptr<CSequence> m_pSequence;		// // //
	CFont m_SmallFont;		// // //
	CRect m_GraphRect;
	CRect m_BottomRect;
	CRect m_ClientRect;
	CBitmap m_Bitmap;		// // //
	CDC m_BackDC;		// // //
	int m_iLastPlayPos;
	int m_iCurrentPlayPos;
	int m_iHighlightedItem;
	int m_iHighlightedValue;
	bool m_bButtonState;

	int m_iLastIndex = -1;		// // //
	int m_iLastValue = 0;		// // //

	CPoint m_ptLineStart, m_ptLineEnd;
	edit_t m_iEditing;

protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	BOOL CreateEx(DWORD dwExStyle, LPCWSTR lpszClassName, LPCWSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, LPVOID lpParam = NULL);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
};

// Bar graph editor
class CBarGraphEditor : public CGraphEditor
{
public:
	CBarGraphEditor(std::shared_ptr<CSequence> pSequence, int Levels) : CGraphEditor(std::move(pSequence)), m_iLevels(Levels) { }		// // //

	void SetMaxItems(int Levels);		// // //

private:
	int GetItemHeight() const override;
	int GetItemTop() const override;		// // //
	int GetItemValue(CPoint point) const override;		// // //

	int m_iLevels;

public:
	afx_msg void OnPaint() override;
};

// Arpeggio graph editor
class CArpeggioGraphEditor : public CGraphEditor
{
public:
	DECLARE_DYNAMIC(CArpeggioGraphEditor)
	explicit CArpeggioGraphEditor(std::shared_ptr<CSequence> pSequence) : CGraphEditor(std::move(pSequence)) { }		// // //

	void UpdateScrollBar();		// // //

private:
	void Initialize() override;
	void DrawRange(CDC &DC, int Max, int Min) override;
	int GetItemHeight() const override;
	int GetItemTop() const override;		// // //

private:
	int GetItemValue(CPoint point) const override;		// // //
	int GetSequenceItemValue(int Index, int Value) const override;		// // //
	SCROLLINFO MakeScrollInfo() const;		// // //

	static const int ITEMS = 20;
	int m_iScrollOffset = 0;
	int m_iScrollMax;
	CScrollBar m_cScrollBar;		// // //

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnPaint() override;
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
};

// Pitch graph editor
class CPitchGraphEditor : public CGraphEditor
{
public:
	explicit CPitchGraphEditor(std::shared_ptr<CSequence> pSequence) : CGraphEditor(std::move(pSequence)) { }		// // //

private:
	int GetItemHeight() const override;
	int GetItemTop() const override;		// // //
	int GetItemValue(CPoint point) const override;		// // //

	static const int ITEMS = 20;

public:
	afx_msg void OnPaint() override;
};

// Sunsoft noise editor
class CNoiseEditor : public CGraphEditor
{
public:
	DECLARE_DYNAMIC(CNoiseEditor)
	CNoiseEditor(std::shared_ptr<CSequence> pSequence, int Items) : CGraphEditor(std::move(pSequence)), m_iItems(Items) { }		// // //

private:
	void DrawRange(CDC &DC, int Max, int Min) override;		// // // 050B
	int GetItemHeight() const override;
	int GetItemTop() const override;		// // //
	int GetItemBottom() const override;		// // //
	int GetItemValue(CPoint point) const override;		// // //
	int GetSequenceItemValue(int Index, int Value) const override;		// // //

	void ModifyReleased() override;
	void ModifyNoise(CPoint point, bool Redraw);		// // //
	bool CheckNoiseFlags(CPoint point) const;		// // //

private:
	static const int BUTTON_HEIGHT = 9;		// // //
	static const int BUTTON_MARGIN = 26;		// // //

	int m_iItems;
	int m_iLastIndexX = -1;		// // //
	int m_iLastIndexY = -1;		// // //
	bool m_bAddNoiseFlags = false;		// // //

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint() override;
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};
