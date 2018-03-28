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

void CLoopReleaseBar::DoOnLButtonDown(CPoint point) {
	if (auto pSeq = parent_.GetSequence()) {
		int idx = std::clamp(parent_.GetItemGridIndex(point), 0, (int)pSeq->GetItemCount() - 1);
		enable_loop_ = pSeq->GetLoopPoint() != idx;
		pSeq->SetLoopPoint(enable_loop_ ? idx : -1);
		parent_.ItemModified(true);
	}
}

void CLoopReleaseBar::DoOnLButtonMove(CPoint point) {
	if (auto pSeq = parent_.GetSequence(); pSeq && enable_loop_) {
		int idx = std::clamp(parent_.GetItemGridIndex(point), 0, (int)pSeq->GetItemCount() - 1);
		pSeq->SetLoopPoint(idx);
		parent_.ItemModified(true);
	}
}

void CLoopReleaseBar::DoOnRButtonDown(CPoint point) {
	if (auto pSeq = parent_.GetSequence()) {
		int idx = std::clamp(parent_.GetItemGridIndex(point), 0, (int)pSeq->GetItemCount() - 1);
		enable_loop_ = pSeq->GetReleasePoint() != idx;
		pSeq->SetReleasePoint(enable_loop_ ? idx : -1);
		parent_.ItemModified(true);
	}
}

void CLoopReleaseBar::DoOnRButtonMove(CPoint point) {
	if (auto pSeq = parent_.GetSequence(); pSeq && enable_loop_) {
		int idx = std::clamp(parent_.GetItemGridIndex(point), 0, (int)pSeq->GetItemCount() - 1);
		pSeq->SetReleasePoint(idx);
		parent_.ItemModified(true);
	}
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



namespace {

constexpr s5b_mode_t S5B_FLAGS[] = {s5b_mode_t::Envelope, s5b_mode_t::Square, s5b_mode_t::Noise};
constexpr auto BUTTON_MARGIN = CNoiseSelector::BUTTON_HEIGHT * 3;

} // namespace

void CNoiseSelector::ModifyFlag(int idx, int flag) {
	if (auto pSeq = parent_.GetSequence()) {
		last_idx_ = idx;
		last_flag_ = flag;
		int8_t ItemValue = pSeq->GetItem(idx);
		auto flagMode = flag == 0 ? s5b_mode_t::Envelope : flag == 1 ? s5b_mode_t::Square : s5b_mode_t::Noise;
		if (enable_flag_)
			ItemValue |= value_cast(flagMode);
		else
			ItemValue &= ~value_cast(flagMode);

		pSeq->SetItem(idx, ItemValue);
		parent_.ItemModified(true);		// // //
	}
}

bool CNoiseSelector::CheckFlag(int8_t val, int flag) {
	if (flag >= 0 && flag < (int)std::size(S5B_FLAGS))
		return val & value_cast(S5B_FLAGS[flag]);
	return false;
}

void CNoiseSelector::DoOnLButtonDown(CPoint point) {
	if (auto pSeq = parent_.GetSequence()) {
		int idx = parent_.GetItemIndex(point);
		if (idx < 0 || idx >= static_cast<int>(pSeq->GetItemCount()))
			return;
		int flag = (point.y - (region_.bottom - BUTTON_MARGIN)) / BUTTON_HEIGHT;		// // //
		if (flag < 0 || flag >= static_cast<int>(std::size(S5B_FLAGS)))
			return;
		enable_flag_ = !CheckFlag(pSeq->GetItem(idx), flag);
		ModifyFlag(idx, flag);
	}
}

void CNoiseSelector::DoOnLButtonMove(CPoint point) {
	if (auto pSeq = parent_.GetSequence()) {
		int idx = parent_.GetItemIndex(point);
		if (idx < 0 || idx >= static_cast<int>(pSeq->GetItemCount()))
			return;
		int flag = (point.y - (region_.bottom - BUTTON_MARGIN)) / BUTTON_HEIGHT;		// // //
		if (flag < 0 || flag >= static_cast<int>(std::size(S5B_FLAGS)))
			return;
		if (idx != last_idx_ || flag != last_flag_)
			ModifyFlag(idx, flag);
	}
}

void CNoiseSelector::DoOnPaint(CDC &dc) {
	auto pSeq = parent_.GetSequence();
	if (!pSeq)
		return;
	int Count = pSeq->GetItemCount();

	int StepWidth = parent_.GetItemWidth();

	for (int i = 0; i < Count; ++i) {
		int x = region_.left + i * StepWidth + 1;
		int w = StepWidth;

		auto flags = enum_cast<s5b_mode_t>(static_cast<uint8_t>(pSeq->GetItem(i)));		// // //

		const COLORREF BAR_COLOR[] = {0x00A0A0, 0xA0A000, 0xA000A0};

		for (std::size_t j = 0; j < std::size(S5B_FLAGS); ++j) {
			int y = region_.bottom - BUTTON_MARGIN + j * BUTTON_HEIGHT + 1;
			int h = BUTTON_HEIGHT - 1;
			const COLORREF Color = (flags & S5B_FLAGS[j]) == S5B_FLAGS[j] ? BAR_COLOR[j] : 0x505050;
			dc.FillSolidRect(x, y, w, h, Color);
			dc.Draw3dRect(x, y, w, h, BLEND(Color, WHITE, .8), BLEND(Color, BLACK, .8));
		}
	}

#ifdef _DEBUG
	const COLORREF COL_DEBUG_RECT = MakeRGB(255, 128, 255);
	dc.FillSolidRect(region_.left, region_.top, region_.Width(), 1, COL_DEBUG_RECT);
	dc.FillSolidRect(region_.left, region_.top, 1, region_.Height(), COL_DEBUG_RECT);
	dc.FillSolidRect(region_.left, region_.bottom - 1, region_.Width(), 1, COL_DEBUG_RECT);
	dc.FillSolidRect(region_.right - 1, region_.top, 1, region_.Height(), COL_DEBUG_RECT);
#endif
}
