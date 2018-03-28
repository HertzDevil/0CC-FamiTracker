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

#include "GraphEditorComponent.h"
#include "GraphEditor.h"
#include "Graphics.h"
#include "Color.h"
#include "Sequence.h"

using namespace GraphEditorComponents;



bool CGraphEditorComponent::ContainsPoint(CPoint point) const {
	return region_.PtInRect(point) == TRUE;
}



CLoopReleaseBar::CLoopReleaseBar(CGraphEditor &parent, CRect region) :
	CGraphEditorComponent(parent, region)
{
	font_.CreateFontW(-10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"Verdana");		// // //
}

void CLoopReleaseBar::ModifyLoopPoint(CPoint point, bool Redraw) {
	if (auto pSeq = parent_.GetSequence()) {
		pSeq->SetLoopPoint(parent_.GetItemGridIndex(point));		// // // -1 handled by sequence class
		parent_.ItemModified(Redraw);
	}
}

void CLoopReleaseBar::ModifyReleasePoint(CPoint point, bool Redraw) {
	if (auto pSeq = parent_.GetSequence()) {
		pSeq->SetReleasePoint(parent_.GetItemGridIndex(point));		// // // -1 handled by sequence class
		parent_.ItemModified(Redraw);
	}
}

void CLoopReleaseBar::DoOnLButtonDown(CPoint point) {
	ModifyLoopPoint(point, true);
}

void CLoopReleaseBar::DoOnLButtonMove(CPoint point) {
	ModifyLoopPoint(point, true);
}

void CLoopReleaseBar::DoOnRButtonDown(CPoint point) {
	ModifyReleasePoint(point, true);
}

void CLoopReleaseBar::DoOnRButtonMove(CPoint point) {
	ModifyReleasePoint(point, true);
}

void CLoopReleaseBar::DrawTagPoint(CDC &dc, int index, LPCWSTR str, COLORREF col) {
	if (index > -1) {
		CFont *pOldFont = dc.SelectObject(&font_);

		int x = parent_.GetItemWidth() * index + CGraphEditor::GRAPH_LEFT + 1;
		GradientBar(dc, x + 1, region_.top, region_.right - x, region_.Height(), DIM(col, 8. / 15.), DIM(col, 2. / 15.));		// // //
		dc.FillSolidRect(x, region_.top, 1, region_.bottom, col);

		dc.SetTextColor(0xFFFFFF);
		dc.SetBkMode(TRANSPARENT);
		dc.SetTextAlign(TA_LEFT);
		dc.TextOutW(x + 4, region_.top, str);

		dc.SelectObject(pOldFont);
	}
}

void CLoopReleaseBar::DoOnPaint(CDC &dc) {
	const COLORREF COL_BOTTOM = MakeRGB(64, 64, 64);
	dc.FillSolidRect(region_, COL_BOTTOM);

	if (auto pSeq = parent_.GetSequence()) {
		int LoopPoint = pSeq->GetLoopPoint();
		int ReleasePoint = pSeq->GetReleasePoint();

		if (ReleasePoint > LoopPoint) {
			DrawTagPoint(dc, LoopPoint, L"Loop", MakeRGB(0, 240, 240));
			DrawTagPoint(dc, ReleasePoint, L"Release", MakeRGB(240, 0, 240));
		}
		else if (ReleasePoint < LoopPoint) {
			DrawTagPoint(dc, ReleasePoint, L"Release", MakeRGB(240, 0, 240));
			DrawTagPoint(dc, LoopPoint, L"Loop", MakeRGB(0, 240, 240));
		}
		else if (LoopPoint > -1) { // LoopPoint == ReleasePoint
			DrawTagPoint(dc, LoopPoint, L"Loop, Release", MakeRGB(240, 240, 0));
		}
	}

#ifdef _DEBUG
	const COLORREF COL_DEBUG_RECT = MakeRGB(0, 255, 255);
	dc.FillSolidRect(region_.left, region_.top, region_.Width(), 1, COL_DEBUG_RECT);
	dc.FillSolidRect(region_.left, region_.top, 1, region_.Height(), COL_DEBUG_RECT);
	dc.FillSolidRect(region_.left, region_.bottom - 1, region_.Width(), 1, COL_DEBUG_RECT);
	dc.FillSolidRect(region_.right - 1, region_.top, 1, region_.Height(), COL_DEBUG_RECT);
#endif
}
