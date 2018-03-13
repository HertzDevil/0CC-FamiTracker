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

#include "GraphEditor.h"
#include "Sequence.h"		// // //
#include "Graphics.h"
#include "FamiTrackerEnv.h"		// // //
#include "APU/Types.h"		// // //
#include "SoundGen.h"
#include "SequenceEditorMessage.h"		// // //
#include "DPI.h"		// // //
#include "PatternNote.h"		// // //
#include "Color.h"		// // //
#include "str_conv/str_conv.hpp"		// // //
#include "NoteName.h"		// // //

// CGraphEditor

// The graphical sequence editor

IMPLEMENT_DYNAMIC(CGraphEditor, CWnd)

BEGIN_MESSAGE_MAP(CGraphEditor, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_TIMER()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()


CGraphEditor::CGraphEditor(std::shared_ptr<CSequence> pSequence) :		// // //
	m_iHighlightedItem(-1),
	m_iHighlightedValue(0),
	m_bButtonState(false),
	m_pSequence(std::move(pSequence)),
	m_iLastPlayPos(0)
{
	m_ptLineStart = m_ptLineEnd = CPoint(0, 0);
}

BOOL CGraphEditor::OnEraseBkgnd(CDC* DC)
{
	return FALSE;
}

BOOL CGraphEditor::CreateEx(DWORD dwExStyle, LPCWSTR lpszClassName, LPCWSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, LPVOID lpParam)
{
	if (CWnd::CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, lpParam) == -1)
		return -1;

	LOGFONTW LogFont = { };		// // //

	const LPCWSTR SMALL_FONT_FACE = L"Verdana";		// // //

	wcscpy_s(LogFont.lfFaceName, SMALL_FONT_FACE);
	LogFont.lfHeight = -10;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
	m_SmallFont.CreateFontIndirect(&LogFont);		// // //

	// Calculate the draw areas
	GetClientRect(m_GraphRect);
	GetClientRect(m_BottomRect);
	GetClientRect(m_ClientRect);

	m_GraphRect.left += DPI::SX(GRAPH_LEFT);		// // //
	m_GraphRect.top += DPI::SY(GRAPH_BOTTOM);
	m_GraphRect.bottom -= DPI::SY(16);
	m_BottomRect.left += GRAPH_LEFT;
	m_BottomRect.top = m_GraphRect.bottom + 1;

	m_pParentWnd = pParentWnd;

	RedrawWindow();

	CDC *pDC = GetDC();

	m_BackDC.CreateCompatibleDC(pDC);
	m_Bitmap.CreateCompatibleBitmap(pDC, m_ClientRect.Width(), m_ClientRect.Height());
	m_BackDC.SelectObject(&m_Bitmap);

	ReleaseDC(pDC);

	Initialize();

	CWnd::SetTimer(0, 30, NULL);

	return 0;
}

void CGraphEditor::Initialize()
{
	// Allow extra initialization
}

void CGraphEditor::OnTimer(UINT nIDEvent)
{
	if (m_pSequence) {
		int Pos = Env.GetSoundGenerator()->GetSequencePlayPos(m_pSequence);
		//int Pos = m_pSequence->GetPlayPos();
		if (Pos != m_iLastPlayPos) {
			m_iCurrentPlayPos = Pos;
			RedrawWindow();
		}
		m_iLastPlayPos = Pos;
	}
}

void CGraphEditor::OnPaint()
{
	CPaintDC dc(this);
}

void CGraphEditor::PaintBuffer(CDC &BackDC, CDC &FrontDC)
{
	if (this == GetFocus()) {
		CRect focusRect = m_ClientRect;
		BackDC.SetBkColor(0);
		BackDC.DrawFocusRect(focusRect);
	}

	FrontDC.BitBlt(0, 0, m_ClientRect.Width(), m_ClientRect.Height(), &BackDC, 0, 0, SRCCOPY);
}

void CGraphEditor::DrawBackground(CDC &DC, int Lines, bool DrawMarks, int MarkOffset)
{
	const COLORREF COL_BACKGROUND	= 0x000000;
	const COLORREF COL_BOTTOM		= 0x404040;
	const COLORREF COL_HORZ_BAR		= 0x202020;

	const int Top = GetItemTop();		// // //
	const int Bottom = GetItemBottom();

	// Fill background
	DC.FillSolidRect(m_ClientRect, COL_BACKGROUND);
	DC.FillSolidRect(m_GraphRect.left, Top, 1, m_GraphRect.bottom, COL_BOTTOM);

	// Fill bottom area
	DC.FillSolidRect(m_BottomRect, COL_BOTTOM);

	if (m_pSequence) {
		int ItemWidth = GetItemWidth();

		// Draw horizontal bars
		int count = m_pSequence->GetItemCount();
		for (int i = 1; i < count; i += 2) {
			int x = m_GraphRect.left + i * ItemWidth + 1;
			int y = Top + 1;
			int w = ItemWidth;
			int h = Bottom - Top;		// // //
			DC.FillSolidRect(x, y, w, h, COL_HORZ_BAR);
		}
	}

	int marker = MarkOffset;

	if (Lines > 0) {
		int StepHeight = (Bottom - Top) / Lines;

		// Draw vertical bars
		for (int i = 0; i <= Lines; ++i) {
			int x = m_GraphRect.left + 1;
			int y = Top + StepHeight * i;
			int w = m_GraphRect.Width() - 2;
			int h = 1;

			if (DrawMarks) {
				if ((++marker + 1) % 12 == 0) {
					DC.FillSolidRect(x, y, w, StepHeight, COL_HORZ_BAR);
				}
			}

			DC.FillSolidRect(x, y, w, h, COLOR_LINES);
		}
	}
}

void CGraphEditor::DrawRange(CDC &DC, int Max, int Min)
{
	CFont *pOldFont = DC.SelectObject(&m_SmallFont);		// // //
	const int Top = GetItemTop();		// // //
	const int Bottom = GetItemBottom();

	DC.FillSolidRect(m_ClientRect.left, Top, m_GraphRect.left, Bottom, 0x00);

	DC.SetBkMode(OPAQUE);
	DC.SetTextColor(0xFFFFFF);
	DC.SetBkColor(DC.GetPixel(0, 0));	// Ugly but works

	CRect textRect(2, 0, GRAPH_LEFT - 5, 10);
	CRect topRect = textRect, bottomRect = textRect;

	topRect.MoveToY(Top - 3);
	DC.DrawTextW(FormattedW(L"%02i", Max), topRect, DT_RIGHT);

	bottomRect.MoveToY(Bottom - 11);
	DC.DrawTextW(FormattedW(L"%02i", Min), bottomRect, DT_RIGHT);

	DC.SelectObject(pOldFont);
}

void CGraphEditor::DrawLoopPoint(CDC &DC, int StepWidth)
{
	// Draw loop point
	int LoopPoint = m_pSequence->GetLoopPoint();

	CFont *pOldFont = DC.SelectObject(&m_SmallFont);

	if (LoopPoint > -1) {
		int x = StepWidth * LoopPoint + GRAPH_LEFT + 1;

		GradientBar(DC, x + 1, m_BottomRect.top, m_BottomRect.right - x, m_BottomRect.Height(), 0x808000, 0x202000);		// // //
		DC.FillSolidRect(x, m_BottomRect.top, 1, m_BottomRect.bottom, 0xF0F000);

		DC.SetTextColor(0xFFFFFF);
		DC.SetBkMode(TRANSPARENT);
		DC.TextOutW(x + 4, m_BottomRect.top, L"Loop");
	}

	DC.SelectObject(pOldFont);
}

void CGraphEditor::DrawReleasePoint(CDC &DC, int StepWidth)
{
	// Draw loop point
	int ReleasePoint = m_pSequence->GetReleasePoint();

	CFont *pOldFont = DC.SelectObject(&m_SmallFont);

	if (ReleasePoint > -1) {
		int x = StepWidth * ReleasePoint + GRAPH_LEFT + 1;

		GradientBar(DC, x + 1, m_BottomRect.top, m_BottomRect.right - x, m_BottomRect.Height(), 0x800080, 0x200020);		// // //
		DC.FillSolidRect(x, m_BottomRect.top, 1, m_BottomRect.bottom, 0xF000F0);

		DC.SetTextColor(0xFFFFFF);
		DC.SetBkMode(TRANSPARENT);
		DC.TextOutW(x + 4, m_BottomRect.top, L"Release");
	}

	DC.SelectObject(pOldFont);
}

void CGraphEditor::DrawLoopRelease(CDC &DC, int StepWidth)		// // //
{
	int LoopPoint = m_pSequence->GetLoopPoint();
	int ReleasePoint = m_pSequence->GetReleasePoint();

	if (ReleasePoint > LoopPoint) {
		DrawLoopPoint(DC, StepWidth);
		DrawReleasePoint(DC, StepWidth);
	}
	else if (ReleasePoint < LoopPoint) {
		DrawReleasePoint(DC, StepWidth);
		DrawLoopPoint(DC, StepWidth);
	}
	else if (LoopPoint > -1) { // LoopPoint == ReleasePoint
		CFont *pOldFont = DC.SelectObject(&m_SmallFont);

		int x = StepWidth * LoopPoint + GRAPH_LEFT + 1;

		GradientBar(DC, x + 1, m_BottomRect.top, m_BottomRect.right - x, m_BottomRect.Height(), 0x008080, 0x002020);		// // //
		DC.FillSolidRect(x, m_BottomRect.top, 1, m_BottomRect.bottom, 0x00F0F0);

		DC.SetTextColor(0xFFFFFF);
		DC.SetBkMode(TRANSPARENT);
		DC.TextOutW(x + 4, m_BottomRect.top, L"Loop, Release");

		DC.SelectObject(pOldFont);
	}
}

void CGraphEditor::DrawLine(CDC &DC)
{
	if (m_ptLineStart.x != 0 && m_ptLineStart.y != 0) {
		CPen Pen;
		Pen.CreatePen(1, 3, 0xFFFFFF);
		CPen *pOldPen = DC.SelectObject(&Pen);
		DC.MoveTo(m_ptLineStart);
		DC.LineTo(m_ptLineEnd);
		DC.SelectObject(pOldPen);
	}
}

template<COLORREF COL_BG1, COLORREF COL_BG2, COLORREF COL_EDGE1, COLORREF COL_EDGE2>
void CGraphEditor::DrawRect(CDC &DC, int x, int y, int w, int h, bool flat)
{
	if (h == 0)
		h = 1;

	if (flat) {
		GradientRect(DC, x + 1, y + 1, w - 2, h - 2, COL_BG1, COL_BG2);		// // //
		DC.Draw3dRect(x, y, w, h, COL_EDGE2, COL_EDGE2);
	}
	else {
		GradientRect(DC, x + 1, y + 1, w - 2, h - 2, COL_BG1, COL_BG2);
		DC.Draw3dRect(x, y, w, h, COL_EDGE1, COL_EDGE2);
	}
}

void CGraphEditor::DrawRect(CDC &DC, int x, int y, int w, int h)
{
	DrawRect<GREY(240), GREY(208), WHITE, GREY(160)>(DC, x, y, w, h, false);
}

void CGraphEditor::DrawPlayRect(CDC &DC, int x, int y, int w, int h)
{
	DrawRect<MakeRGB(160, 240, 160), MakeRGB(134, 220, 134), MakeRGB(198, 242, 198), MakeRGB(106, 223, 106)>(DC, x, y, w, h, false);
}

void CGraphEditor::DrawCursorRect(CDC &DC, int x, int y, int w, int h)
{
	DrawRect<MakeRGB(192, 224, 255), MakeRGB(153, 198, 229), MakeRGB(215, 235, 253), MakeRGB(120, 182, 226)>(DC, x, y, w, h, m_bButtonState);
}

void CGraphEditor::DrawShadowRect(CDC &DC, int x, int y, int w, int h)
{
	if (h == 0)
		h = 1;

	GradientBar(DC, x, y, w, h, 0x484848, 0x444444);		// // //
}

int CGraphEditor::GetItemWidth() const
{
	if (!m_pSequence || m_pSequence->GetItemCount() == 0)
		return 0;

	int Width = m_GraphRect.Width();
	Width = (Width / m_pSequence->GetItemCount()) * m_pSequence->GetItemCount();

	Width = Width / m_pSequence->GetItemCount();

	if (Width > ITEM_MAX_WIDTH)
		Width = ITEM_MAX_WIDTH;

	return Width;
}

int CGraphEditor::GetItemBottom() const		// // //
{
	return m_GraphRect.bottom;
}

void CGraphEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_bButtonState = true;

	SetCapture();
	SetFocus();

	if (!m_pSequence || !m_pSequence->GetItemCount())
		return;

	if (point.y < m_GraphRect.bottom) {
		m_iEditing = EDIT_POINT;
		ModifyItem(point, true);
		if (point.x > GRAPH_LEFT)
			CursorChanged(point.x - GRAPH_LEFT);
	}
	else if (point.y > m_GraphRect.bottom) {
		m_iEditing = EDIT_LOOP;
		ModifyLoopPoint(point, true);
	}
}

void CGraphEditor::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bButtonState = false;

	ModifyReleased();
	ReleaseCapture();
	Invalidate();
}

void CGraphEditor::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_pSequence || !m_pSequence->GetItemCount())
		return;

	if (nFlags & MK_LBUTTON) {
		switch (m_iEditing) {
			case EDIT_POINT:
				ModifyItem(point, true);
				break;
			case EDIT_LOOP:
				ModifyLoopPoint(point, true);
				break;
		}
	}
	else if (nFlags & MK_RBUTTON) {
		switch (m_iEditing) {
			case EDIT_LINE:	{
				int PosX = point.x;
				int PosY = point.y;

				if (m_iEditing == EDIT_LINE) {
					m_ptLineEnd = CPoint(PosX, PosY);

					int StartX = (m_ptLineStart.x < m_ptLineEnd.x ? m_ptLineStart.x : m_ptLineEnd.x);
					int StartY = (m_ptLineStart.x < m_ptLineEnd.x ? m_ptLineStart.y : m_ptLineEnd.y);
					int EndX = (m_ptLineStart.x > m_ptLineEnd.x ? m_ptLineStart.x : m_ptLineEnd.x);
					int EndY = (m_ptLineStart.x > m_ptLineEnd.x ? m_ptLineStart.y : m_ptLineEnd.y);

					float DeltaY = float(EndY - StartY) / float((EndX - StartX) + 1);
					float fY = float(StartY);

					for (int x = StartX; x < EndX; ++x) {
						ModifyItem(CPoint(x, (int)fY), false);
						fY += DeltaY;
					}

					RedrawWindow();
					m_pParentWnd->PostMessageW(WM_SEQUENCE_CHANGED, 1);
				}
				}
				break;
			case EDIT_RELEASE:
				ModifyReleasePoint(point, true);
				break;
		}
	}
	else {
		// No button
		HighlightItem(point);
	}

	// Notify parent
	if (m_pSequence->GetItemCount() > 0 && point.x > GRAPH_LEFT) {
		CursorChanged(point.x - GRAPH_LEFT);
	}
	else
		m_pParentWnd->PostMessageW(WM_CURSOR_CHANGE, 0, 0);
}

void CGraphEditor::OnRButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();

	if (point.y < m_GraphRect.bottom) {
		m_ptLineStart = m_ptLineEnd = point;
		m_iEditing = EDIT_LINE;
	}
	else if (point.y > m_GraphRect.bottom) {
		m_iEditing = EDIT_RELEASE;
		ModifyReleasePoint(point, true);
	}

	CWnd::OnRButtonDown(nFlags, point);
}

void CGraphEditor::OnRButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();

	m_ptLineStart = CPoint(0, 0);
	m_iEditing = EDIT_NONE;
	RedrawWindow();
	CWnd::OnRButtonUp(nFlags, point);
}

void CGraphEditor::ModifyReleased()
{
}

void CGraphEditor::ModifyItem(CPoint point, bool Redraw)
{
	if (Redraw) {
		RedrawWindow(NULL);
		m_pParentWnd->PostMessageW(WM_SEQUENCE_CHANGED, 1);
	}
}

void CGraphEditor::ModifyLoopPoint(CPoint point, bool Redraw)
{
	if (!m_pSequence || !m_pSequence->GetItemCount())
		return;

	int ItemWidth = GetItemWidth();
	int LoopPoint = (point.x - GRAPH_LEFT + (ItemWidth / 2)) / ItemWidth;

	// Disable loop point by dragging it to far right
	if (LoopPoint >= (int)m_pSequence->GetItemCount())
		LoopPoint = -1;

	m_pSequence->SetLoopPoint(LoopPoint);

	if (Redraw) {
		RedrawWindow(NULL);
		m_pParentWnd->PostMessageW(WM_SEQUENCE_CHANGED, 1);
	}
}

void CGraphEditor::ModifyReleasePoint(CPoint point, bool Redraw)
{
	if (!m_pSequence || !m_pSequence->GetItemCount())
		return;

	int ItemWidth = GetItemWidth();
	int ReleasePoint = (point.x - GRAPH_LEFT + (ItemWidth / 2)) / ItemWidth;

	// Disable loop point by dragging it to far right
	if (ReleasePoint >= (int)m_pSequence->GetItemCount())
		ReleasePoint = -1;

	m_pSequence->SetReleasePoint(ReleasePoint);

	if (Redraw) {
		RedrawWindow(NULL);
		m_pParentWnd->PostMessageW(WM_SEQUENCE_CHANGED, 1);
	}
}

void CGraphEditor::CursorChanged(int x)
{
	if (m_pSequence->GetItemCount() == 0)
		return;

	int Width = GetItemWidth();

	if (Width == 0)
		return;

	int Pos = x / Width;

	if (Pos >= (signed)m_pSequence->GetItemCount())
		return;

	int Value = m_pSequence->GetItem(Pos);
	m_pParentWnd->PostMessageW(WM_CURSOR_CHANGE, Pos, Value);
}

bool CGraphEditor::IsEditLine() const
{
	return m_iEditing == EDIT_LINE;
}

void CGraphEditor::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);
	RedrawWindow();
}

void CGraphEditor::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);
	RedrawWindow();
}

// Bar graph editor (volume and duty setting)

void CBarGraphEditor::OnPaint()
{
	CPaintDC dc(this);

	DrawBackground(m_BackDC, m_iLevels, false, 0);
	DrawRange(m_BackDC, m_iLevels, 0);

	// Return now if no sequence is selected
	if (!m_pSequence) {
		PaintBuffer(m_BackDC, dc);
		return;
	}

	// Draw items
	int Count = m_pSequence->GetItemCount();

	if (!Count) {
		PaintBuffer(m_BackDC, dc);
		return;
	}

	int StepWidth = GetItemWidth();
	int StepHeight = GetItemHeight();		// // //
	const int Top = GetItemTop();		// // //

	if (m_iHighlightedValue > 0 && m_iHighlightedItem >= 0 && m_iHighlightedItem < Count) {
		int x = m_GraphRect.left + m_iHighlightedItem * StepWidth + 1;
		int y = Top + StepHeight * (m_iLevels - m_iHighlightedValue);
		int w = StepWidth;
		int h = StepHeight * m_iHighlightedValue;
		DrawShadowRect(m_BackDC, x, y, w, h);
	}

	// Draw items
	for (int i = 0; i < Count; ++i) {
		int x = m_GraphRect.left + i * StepWidth + 1;
		int y = Top + StepHeight * (m_iLevels - m_pSequence->GetItem(i));
		int w = StepWidth;
		int h = StepHeight * m_pSequence->GetItem(i);

		if (m_iCurrentPlayPos == i)
			DrawPlayRect(m_BackDC, x, y, w, h);
		else if ((m_iHighlightedItem == i) && (m_pSequence->GetItem(i) >= m_iHighlightedValue) && !IsEditLine())
			DrawCursorRect(m_BackDC, x, y, w, h);
		else
			DrawRect(m_BackDC, x, y, w, h);
	}

	DrawLoopRelease(m_BackDC, StepWidth);		// // //
	DrawLine(m_BackDC);

	PaintBuffer(m_BackDC, dc);
}

void CBarGraphEditor::HighlightItem(CPoint point)
{
	int ItemWidth = GetItemWidth();
	int ItemHeight = GetItemHeight();
	int ItemIndex = (point.x - GRAPH_LEFT) / ItemWidth;
	int ItemValue = m_iLevels - (((point.y - GetItemTop()) + (ItemHeight / 2)) / ItemHeight);		// // //
	int LastItem = m_iHighlightedItem;
	int LastValue = m_iHighlightedValue;

	m_iHighlightedItem = ItemIndex;
	m_iHighlightedValue = ItemValue;

	if (m_GraphRect.PtInRect(point) == 0 || unsigned(ItemIndex) >= m_pSequence->GetItemCount()) {
		m_iHighlightedItem = -1;
		m_iHighlightedValue = 0;
	}

	if (LastItem != m_iHighlightedItem || LastValue != m_iHighlightedValue)
		RedrawWindow(NULL);
}

void CBarGraphEditor::ModifyItem(CPoint point, bool Redraw)
{
	int ItemWidth = GetItemWidth();
	int ItemHeight = GetItemHeight();
	int ItemIndex = (point.x - GRAPH_LEFT) / ItemWidth;
	int ItemValue = m_iLevels - (((point.y - GetItemTop()) + (ItemHeight / 2)) / ItemHeight);

	if (ItemValue < 0)
		ItemValue = 0;
	if (ItemValue > m_iLevels)
		ItemValue = m_iLevels;

	if (ItemIndex < 0 || ItemIndex >= (int)m_pSequence->GetItemCount())
		return;

	m_iHighlightedItem = ItemIndex;
	m_iHighlightedValue = ItemValue;

	m_pSequence->SetItem(ItemIndex, ItemValue);

	CGraphEditor::ModifyItem(point, Redraw);
}

int CBarGraphEditor::GetItemHeight() const
{
	return (GetItemBottom() - GetItemTop()) / m_iLevels;		// // //
}

int CBarGraphEditor::GetItemTop() const		// // //
{
	return m_GraphRect.top + m_GraphRect.Height() % m_iLevels;
}

void CBarGraphEditor::SetMaxItems(int Levels)		// // //
{
	m_iLevels = Levels;
	RedrawWindow();
}

// Arpeggio graph editor

IMPLEMENT_DYNAMIC(CArpeggioGraphEditor, CGraphEditor)

BEGIN_MESSAGE_MAP(CArpeggioGraphEditor, CGraphEditor)
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

CArpeggioGraphEditor::CArpeggioGraphEditor(std::shared_ptr<CSequence> pSequence) :		// // //
	CGraphEditor(pSequence),
	m_iScrollOffset(0)
{
}

void CArpeggioGraphEditor::Initialize()
{
	// Setup scrollbar
	const int SCROLLBAR_WIDTH = ::GetSystemMetrics(SM_CXHSCROLL);		// // //
	SCROLLINFO info;

	m_GraphRect.right -= SCROLLBAR_WIDTH;
	m_cScrollBar.Create(SBS_VERT | SBS_LEFTALIGN | WS_CHILD | WS_VISIBLE, CRect(m_GraphRect.right, m_GraphRect.top, m_GraphRect.right + SCROLLBAR_WIDTH, m_GraphRect.bottom), this, 0);

	info.cbSize = sizeof(SCROLLINFO);
	info.fMask = SIF_ALL;

	switch (m_pSequence->GetSetting()) {		// // //
	case SETTING_ARP_FIXED:
		info.nMax = 84;
		info.nMin = 0;
		info.nPage = 1;
		info.nPos = 84;
		m_iScrollMax = 84; break;
	case SETTING_ARP_SCHEME:
		info.nMax = 72;
		info.nMin = 0;
		info.nPage = 1;
		info.nPos = ARPSCHEME_MAX;
		m_iScrollMax = ARPSCHEME_MAX; break;
	default:
		info.nMax = 192;
		info.nMin = 0;
		info.nPage = 10;
		info.nPos = 96;
		m_iScrollMax = 96;
	}

	if (m_pSequence != NULL && m_pSequence->GetItemCount() > 0) {
		m_iScrollOffset = m_pSequence->GetItem(0);
		if (m_pSequence->GetSetting() == SETTING_ARP_SCHEME) {		// // //
			m_iScrollOffset &= 0x3F;
			if (m_iScrollOffset > ARPSCHEME_MAX)
				m_iScrollOffset -= 0x40;
		}

		info.nPos = m_iScrollMax - m_iScrollOffset;		// // //
		if (info.nPos < info.nMin)
			info.nPos = info.nMin;
		if (info.nPos > info.nMax)
			info.nPos = info.nMax;

	}
	else
		m_iScrollOffset = 0;

	m_cScrollBar.SetScrollInfo(&info);
	m_cScrollBar.ShowScrollBar(TRUE);
	m_cScrollBar.EnableScrollBar(ESB_ENABLE_BOTH);

	m_ClientRect.right -= SCROLLBAR_WIDTH;

	CGraphEditor::Initialize();
}

void CArpeggioGraphEditor::ChangeSetting()
{
	// Reset the scrollbar
	SCROLLINFO info;

	switch (m_pSequence->GetSetting()) {		// // //
	case SETTING_ARP_FIXED:
		info.nMax = 84;
		info.nMin = 0;
		info.nPage = 1;
		info.nPos = 84;
		m_iScrollMax = 84; break;
	case SETTING_ARP_SCHEME:
		info.nMax = 72;
		info.nMin = 0;
		info.nPage = 1;
		info.nPos = ARPSCHEME_MAX;
		m_iScrollMax = ARPSCHEME_MAX; break;
	default:
		info.nMax = 192;
		info.nMin = 0;
		info.nPage = 10;
		info.nPos = 96;
		m_iScrollMax = 96;
	}

	m_iScrollOffset = 0;
	m_cScrollBar.SetScrollInfo(&info);
}

void CArpeggioGraphEditor::DrawRange(CDC &DC, int Max, int Min)
{
	const char NOTES_A[] = {'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B'};
	const char NOTES_B[] = {'-', '#', '-', '#', '-', '-', '#', '-', '#', '-', '#', '-'};
	const char NOTES_C[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

	if (m_pSequence->GetSetting() != SETTING_ARP_FIXED) {
		// Absolute, relative
		CGraphEditor::DrawRange(DC, Max, Min);
	}
	else {
		// Fixed
		DC.FillSolidRect(m_ClientRect.left, m_GraphRect.top, m_GraphRect.left, m_GraphRect.bottom, 0x00);

		CFont *pOldFont = DC.SelectObject(&m_SmallFont);

		DC.SetTextColor(0xFFFFFF);
		DC.SetBkColor(0);

		// Top
		int NoteValue = m_iScrollOffset + 20;
		DC.TextOutW(2, m_GraphRect.top - 3, conv::to_wide(GetNoteString(GET_NOTE(NoteValue), GET_OCTAVE(NoteValue))).data());		// // //

		// Bottom
		NoteValue = m_iScrollOffset;
		DC.TextOutW(2, m_GraphRect.bottom - 13, conv::to_wide(GetNoteString(GET_NOTE(NoteValue), GET_OCTAVE(NoteValue))).data());

		DC.SelectObject(pOldFont);
	}
}

void CArpeggioGraphEditor::OnPaint()
{
	CPaintDC dc(this);

	DrawBackground(m_BackDC, ITEMS, true,		// // //
		m_pSequence->GetSetting() == SETTING_ARP_FIXED ? 2 - m_iScrollOffset : -m_iScrollOffset);
	DrawRange(m_BackDC, m_iScrollOffset + 10, m_iScrollOffset - 10);

	// Return now if no sequence is selected
	if (!m_pSequence) {
		PaintBuffer(m_BackDC, dc);
		return;
	}

	// Draw items
	int Count = m_pSequence->GetItemCount();

	if (!Count) {
		PaintBuffer(m_BackDC, dc);
		return;
	}

	int StepWidth = GetItemWidth();
	int StepHeight = GetItemHeight();		// // //
	const int Top = GetItemTop();		// // //

	// One last line
	m_BackDC.FillSolidRect(m_GraphRect.left + 1, Top + ITEMS * StepHeight, m_GraphRect.Width() - 2, 1, COLOR_LINES);

	if (m_iHighlightedItem >= 0 && m_iHighlightedItem < Count) {
		int item;			// // //
		if (m_pSequence->GetSetting() == SETTING_ARP_SCHEME) {
			int value = (m_iHighlightedValue + 0x100) % 0x40;
			if (value > ARPSCHEME_MAX) value -= 0x40;
			item = (ITEMS / 2) - value + m_iScrollOffset;
		}
		else
			item = (ITEMS / 2) - m_iHighlightedValue + m_iScrollOffset;
		if (m_pSequence->GetSetting() == SETTING_ARP_FIXED)
			item += (ITEMS / 2);
		if (item >= 0 && item <= 20) {
			int x = m_GraphRect.left + m_iHighlightedItem * StepWidth + 1;
			int y = Top + StepHeight * item + 1;
			int w = StepWidth;
			int h = StepHeight - 1;
			DrawShadowRect(m_BackDC, x, y, w, h);
		}
	}

	// Draw items
	CFont *pOldFont = m_BackDC.SelectObject(&m_SmallFont);		// // //
	m_BackDC.SetTextAlign(TA_CENTER);
	m_BackDC.SetTextColor(0xFFFFFF);
	m_BackDC.SetBkMode(TRANSPARENT);
	for (int i = 0; i < Count; ++i) {
		int item;			// // //
		if (m_pSequence->GetSetting() == SETTING_ARP_SCHEME) {
			int value = (m_pSequence->GetItem(i) + 0x100) % 0x40;
			if (value > ARPSCHEME_MAX) value -= 0x40;
			item = (ITEMS / 2) - value + m_iScrollOffset;
		}
		else
			item = (ITEMS / 2) - m_pSequence->GetItem(i) + m_iScrollOffset;
		if (m_pSequence->GetSetting() == SETTING_ARP_FIXED)
			item += (ITEMS / 2);
		if (item >= 0 && item <= ITEMS) {
			int x = m_GraphRect.left + i * StepWidth + 1;
			int y = Top + StepHeight * item;
			int w = StepWidth;
			int h = StepHeight;

			if (m_iCurrentPlayPos == i)
				DrawPlayRect(m_BackDC, x, y, w, h);
			else if (m_iHighlightedItem == i && (m_pSequence->GetItem(i) == m_iHighlightedValue) && !IsEditLine())
				DrawCursorRect(m_BackDC, x, y, w, h);
			else
				DrawRect(m_BackDC, x, y, w, h);

			if (m_pSequence->GetSetting() == SETTING_ARP_SCHEME) {
				const LPCWSTR HEAD[] = {L"", L"x", L"y", L"-y"};
				m_BackDC.TextOutW(x + w / 2, y - 2 * h, HEAD[(m_pSequence->GetItem(i) & 0xFF) >> 6]);
			}
		}
	}
	m_BackDC.SetTextAlign(TA_LEFT);
	m_BackDC.SelectObject(pOldFont);

	DrawLoopRelease(m_BackDC, StepWidth);		// // //
	DrawLine(m_BackDC);

	PaintBuffer(m_BackDC, dc);
}

void CArpeggioGraphEditor::ModifyItem(CPoint point, bool Redraw)
{
	int ItemWidth = GetItemWidth();
	int ItemIndex = (point.x - GRAPH_LEFT) / ItemWidth;
	int ItemValue = GetItemValue(point.y);

	if (ItemIndex < 0 || ItemIndex >= (int)m_pSequence->GetItemCount())
		return;

	m_iHighlightedItem = ItemIndex;
	m_iHighlightedValue = ItemValue;

	if (m_pSequence->GetSetting() == SETTING_ARP_SCHEME) {		// // //
		auto value = static_cast<arp_scheme_mode_t>((uint8_t)(m_pSequence->GetItem(ItemIndex) & 0xC0));
		if (::GetKeyState(VK_NUMPAD0) & 0x80)
			value = arp_scheme_mode_t::none;
		else if (::GetKeyState(VK_NUMPAD1) & 0x80)
			value = arp_scheme_mode_t::X;
		else if (::GetKeyState(VK_NUMPAD2) & 0x80)
			value = arp_scheme_mode_t::Y;
		else if (::GetKeyState(VK_NUMPAD3) & 0x80)
			value = arp_scheme_mode_t::NegY;
		ItemValue = (ItemValue & 0x3F) | value_cast(value);
	}
	m_pSequence->SetItem(ItemIndex, ItemValue);

	CGraphEditor::ModifyItem(point, Redraw);
}

void CArpeggioGraphEditor::HighlightItem(CPoint point)
{
	int ItemWidth = GetItemWidth();
	int ItemIndex = (point.x - GRAPH_LEFT) / ItemWidth;
	int ItemValue = GetItemValue(point.y);
	int LastItem = m_iHighlightedItem;
	int LastValue = m_iHighlightedValue;

	m_iHighlightedItem = ItemIndex;
	m_iHighlightedValue = ItemValue;

	if (m_GraphRect.PtInRect(point) == 0 || unsigned(ItemIndex) >= m_pSequence->GetItemCount()) {
		m_iHighlightedItem = -1;
		m_iHighlightedValue = 0;
	}

	if (m_iHighlightedItem != LastItem || m_iHighlightedValue != LastValue) {
		RedrawWindow(NULL);
	}
}

int CArpeggioGraphEditor::GetItemValue(int pos)
{
	int ItemValue;
	int ItemHeight = GetItemHeight();
	const int Top = GetItemTop();		// // //

	switch (m_pSequence->GetSetting()) {		// // //
	case SETTING_ARP_FIXED:
		ItemValue = ITEMS - ((pos - Top) / ItemHeight) + m_iScrollOffset;
		if (ItemValue < 0)
			ItemValue = 0;
		if (ItemValue > 95)
			ItemValue = 95;
		break;
	case SETTING_ARP_SCHEME:
		ItemValue = (ITEMS / 2) - ((pos - Top) / ItemHeight) + m_iScrollOffset;
		if (ItemValue < ARPSCHEME_MIN)
			ItemValue = ARPSCHEME_MIN;
		if (ItemValue > ARPSCHEME_MAX)
			ItemValue = ARPSCHEME_MAX;
		break;
	default:
		ItemValue = (ITEMS / 2) - ((pos - Top) / ItemHeight) + m_iScrollOffset;
		if (ItemValue < -96)
			ItemValue = -96;
		if (ItemValue > 96)
			ItemValue = 96;
	}

	return ItemValue;
}

int CArpeggioGraphEditor::GetItemHeight() const
{
	return (GetItemBottom() - GetItemTop()) / (ITEMS + 1);		// // //
}

int CArpeggioGraphEditor::GetItemTop() const		// // //
{
	return m_GraphRect.top + m_GraphRect.Height() % (ITEMS + 1);
}

void CArpeggioGraphEditor::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	switch (nSBCode) {
		case SB_LINEDOWN:
			--m_iScrollOffset;
			break;
		case SB_LINEUP:
			++m_iScrollOffset;
			break;
		case SB_PAGEDOWN:
			m_iScrollOffset -= ITEMS / 2;
			break;
		case SB_PAGEUP:
			m_iScrollOffset += ITEMS / 2;
			break;
		case SB_TOP:
			m_iScrollOffset = m_iScrollMax;
			break;
		case SB_BOTTOM:
			m_iScrollOffset = (m_pSequence->GetSetting() != SETTING_ARP_FIXED) ? -m_iScrollMax : 0;
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			m_iScrollOffset = m_iScrollMax - (int)nPos;
			break;
		case SB_ENDSCROLL:
		default:
			return;
	}

	if (m_iScrollOffset > m_iScrollMax)
		m_iScrollOffset = m_iScrollMax;

	if (m_pSequence->GetSetting() != SETTING_ARP_FIXED) {
		if (m_iScrollOffset < -m_iScrollMax)
			m_iScrollOffset = -m_iScrollMax;
	}
	else {
		if (m_iScrollOffset < 0)
			m_iScrollOffset = 0;
	}

	pScrollBar->SetScrollPos(m_iScrollMax - m_iScrollOffset);

	RedrawWindow();

	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

BOOL CArpeggioGraphEditor::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (zDelta < 0)
		OnVScroll(SB_LINEDOWN, 0, &m_cScrollBar);
	else
		OnVScroll(SB_LINEUP, 0, &m_cScrollBar);
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

// Pitch graph editor

void CPitchGraphEditor::OnPaint()
{
	CPaintDC dc(this);

	DrawBackground(m_BackDC, 0, false, 0);
	DrawRange(m_BackDC, 127, -128);

	m_BackDC.FillSolidRect(m_GraphRect.left, m_GraphRect.top + m_GraphRect.Height() / 2, m_GraphRect.Width(), 1, COLOR_LINES);

	// Return now if no sequence is selected
	if (!m_pSequence) {
		PaintBuffer(m_BackDC, dc);
		return;
	}

	// Draw items
	int Count = m_pSequence->GetItemCount();

	if (!Count) {
		PaintBuffer(m_BackDC, dc);
		return;
	}

	int StepWidth = GetItemWidth();
	const int Top = GetItemTop();		// // //

	// One last line
	// m_BackDC.FillSolidRect(m_GraphRect.left + 1, m_GraphRect.top + 20 * StepHeight, m_GraphRect.Width() - 2, 1, COLOR_LINES);

	if (m_iHighlightedItem != -1) {
		int x = m_GraphRect.left + m_iHighlightedItem * StepWidth + 1;
		int y = Top + (m_GraphRect.bottom - Top) / 2;
		int w = StepWidth;
		int h = -(m_iHighlightedValue * m_GraphRect.Height()) / 255 ;
		DrawShadowRect(m_BackDC, x, y, w, h);
	}

	bool bHighlight = !IsEditLine() && m_iHighlightedItem >= 0 && m_iHighlightedItem < Count;		// // //
	if (bHighlight) {
		int Value = m_pSequence->GetItem(m_iHighlightedItem);
		if (m_iHighlightedValue > 0) {
			bHighlight = m_iHighlightedValue <= Value && Value >= 0;
		}
		else {
			bHighlight = m_iHighlightedValue >= Value && Value <= 0;
		}
	}

	// Draw items
	for (int i = 0; i < Count; ++i) {
		int item = m_pSequence->GetItem(i);
		int x = m_GraphRect.left + i * StepWidth + 1;
		int y = Top + m_GraphRect.Height() / 2;
		int w = StepWidth;
		int h = -(item * m_GraphRect.Height()) / 255 ;
		if (h < 0) {
			y += h;
			h = -h;
		}
		if (m_iCurrentPlayPos == i)
			DrawPlayRect(m_BackDC, x, y, w, h);
		else if (m_iHighlightedItem == i && bHighlight)
			DrawCursorRect(m_BackDC, x, y, w, h);
		else
			DrawRect(m_BackDC, x, y, w, h);
	}

	DrawLoopRelease(m_BackDC, StepWidth);		// // //
	DrawLine(m_BackDC);

	PaintBuffer(m_BackDC, dc);
}

int CPitchGraphEditor::GetItemHeight() const
{
	return (GetItemBottom() - GetItemTop()) / ITEMS;		// // //
}

int CPitchGraphEditor::GetItemTop() const		// // //
{
	return m_GraphRect.top;
}

void CPitchGraphEditor::ModifyItem(CPoint point, bool Redraw)
{
	int MouseY = (point.y - m_GraphRect.top) - (m_GraphRect.Height() / 2);
	int ItemWidth = GetItemWidth();
	int ItemIndex = (point.x - GRAPH_LEFT) / ItemWidth;
	int ItemValue = -(MouseY * 255) / m_GraphRect.Height();

	if (ItemValue < -128)
		ItemValue = -128;
	if (ItemValue > 127)
		ItemValue = 127;

	if (ItemIndex < 0 || ItemIndex >= (int)m_pSequence->GetItemCount())
		return;

	m_iHighlightedItem = ItemIndex;
	m_iHighlightedValue = ItemValue;

	m_pSequence->SetItem(ItemIndex, ItemValue);

	CGraphEditor::ModifyItem(point, Redraw);
}

void CPitchGraphEditor::HighlightItem(CPoint point)
{
	int MouseY = (point.y - m_GraphRect.top) - (m_GraphRect.Height() / 2);
	int ItemWidth = GetItemWidth();
	int ItemIndex = (point.x - GRAPH_LEFT) / ItemWidth;
	int ItemValue = -(MouseY * 255) / m_GraphRect.Height();
	int LastItem = m_iHighlightedItem;
	int LastValue = m_iHighlightedValue;

	if (ItemValue < -128)
		ItemValue = -128;
	if (ItemValue > 127)
		ItemValue = 127;

	m_iHighlightedItem = ItemIndex;
	m_iHighlightedValue = ItemValue;

	if (m_GraphRect.PtInRect(point) == 0 || ItemIndex < 0 || unsigned(ItemIndex) >= m_pSequence->GetItemCount()) {
		m_iHighlightedItem = -1;
		m_iHighlightedValue = 0;
	}

	if (m_iHighlightedItem != LastItem || m_iHighlightedValue != LastValue) {
		RedrawWindow(NULL);
	}
}

// Sunsoft noise editor

void CNoiseEditor::OnPaint()
{
	CPaintDC dc(this);

	DrawBackground(m_BackDC, m_iItems, false, 0);
	DrawRange(m_BackDC, m_iItems, 0);

	// Return now if no sequence is selected
	if (!m_pSequence) {
		PaintBuffer(m_BackDC, dc);
		return;
	}

	// Draw items
	int Count = m_pSequence->GetItemCount();

	if (!Count) {
		PaintBuffer(m_BackDC, dc);
		return;
	}

	int StepWidth = GetItemWidth();
	int StepHeight = GetItemHeight();		// // //
	const int Top = GetItemTop();		// // //

	// // // One last line
	m_BackDC.FillSolidRect(m_GraphRect.left + 1, Top + (m_iItems + 1) * StepHeight, m_GraphRect.Width() - 2, 1, COLOR_LINES);

	// Draw items
	for (int i = 0; i < Count; ++i) {
		// Draw noise frequency
		int item = m_pSequence->GetItem(i) & 0x1F;
		int x = m_GraphRect.left + i * StepWidth + 1;
		int y = Top + StepHeight * (m_iItems - item);
		int w = StepWidth;
		int h = StepHeight;//* item;

		if (m_iCurrentPlayPos == i)
			DrawPlayRect(m_BackDC, x, y, w, h);
		else
			DrawRect(m_BackDC, x, y, w, h);

		// Draw switches
//		auto flags = static_cast<s5b_mode_t>((uint8_t)m_pSequence->GetItem(i));		// // //
		auto flags = (uint8_t)m_pSequence->GetItem(i);		// // //

//		const s5b_mode_t BAR_MODE[] = {s5b_mode_t::Envelope, s5b_mode_t::Square, s5b_mode_t::Noise};		// // //
		const uint8_t BAR_MODE[] = {value_cast(s5b_mode_t::Envelope), value_cast(s5b_mode_t::Square), value_cast(s5b_mode_t::Noise)};		// // //
		const COLORREF BAR_COLOR[] = {0x00A0A0, 0xA0A000, 0xA000A0};

		for (std::size_t j = 0; j < std::size(BAR_MODE); ++j) {
			int y2 = m_GraphRect.bottom - BUTTON_MARGIN + j * BUTTON_HEIGHT + 1;
			int h2 = BUTTON_HEIGHT - 1;
			const COLORREF Color = (flags & BAR_MODE[j]) == BAR_MODE[j] ? BAR_COLOR[j] : 0x505050;
			m_BackDC.FillSolidRect(x, y2, w, h2, Color);
			m_BackDC.Draw3dRect(x, y2, w, h2, BLEND(Color, WHITE, .8), BLEND(Color, BLACK, .8));
		}
	}

	DrawLoopRelease(m_BackDC, StepWidth);		// // //
	DrawLine(m_BackDC);

	PaintBuffer(m_BackDC, dc);
}

void CNoiseEditor::ModifyItem(CPoint point, bool Redraw)
{
	int ItemValue;
	int ItemWidth = GetItemWidth();
	int ItemHeight = GetItemHeight();
	const int Top = GetItemTop();		// // //

	int ItemIndex = (point.x - GRAPH_LEFT) / ItemWidth;

	int Offset = m_GraphRect.bottom - BUTTON_MARGIN;		// // //

	if (point.y >= Offset) {
		int x = (point.y - Offset) / BUTTON_HEIGHT;		// // //
		if (m_iLastIndexY == ItemIndex && m_iLastIndexX == x)
			return;
		m_iLastIndexY = ItemIndex;
		m_iLastIndexX = x;

		switch (x) {		// // //
		case 0: ItemValue = m_pSequence->GetItem(ItemIndex) ^ value_cast(s5b_mode_t::Envelope); break;
		case 1: ItemValue = m_pSequence->GetItem(ItemIndex) ^ value_cast(s5b_mode_t::Square); break;
		case 2: ItemValue = m_pSequence->GetItem(ItemIndex) ^ value_cast(s5b_mode_t::Noise); break;
		default: return;
		}
	}
	else {

		ItemValue = m_iItems - (((point.y - Top) + (ItemHeight / 2)) / ItemHeight);

		if (ItemValue < 0)
			ItemValue = 0;
		if (ItemValue > m_iItems)
			ItemValue = m_iItems;

		ItemValue |= 0xE0 & (uint8_t)m_pSequence->GetItem(ItemIndex);		// // //
//		ItemValue |= value_cast<s5b_mode_t>((uint8_t)m_pSequence->GetItem(ItemIndex));		// // //
	}

	if (ItemIndex < 0 || ItemIndex >= (int)m_pSequence->GetItemCount())
		return;

	m_pSequence->SetItem(ItemIndex, ItemValue);

	CGraphEditor::ModifyItem(point, Redraw);
}

void CNoiseEditor::HighlightItem(CPoint point)
{
	// TODO
}

int CNoiseEditor::GetItemHeight() const
{
	return (GetItemBottom() - GetItemTop()) / (m_iItems + 1);		// // //
}

int CNoiseEditor::GetItemTop() const		// // //
{
	return m_GraphRect.top + (GetItemBottom() - m_GraphRect.top) % (m_iItems + 1);
}

int CNoiseEditor::GetItemBottom() const		// // //
{
	return m_GraphRect.bottom - BUTTON_MARGIN;
}

void CNoiseEditor::ModifyReleased()
{
	m_iLastIndexX = m_iLastIndexY = -1;		// // //
}
