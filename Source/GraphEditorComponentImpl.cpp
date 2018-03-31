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

#include "GraphEditorComponentImpl.h"
#include "GraphEditor.h"
#include "Graphics.h"
#include "Color.h"
#include "Sequence.h"
#include "NoteName.h"
#include "NumConv.h"
#include "str_conv/str_conv.hpp"
#include "DPI.h"

using namespace GraphEditorComponents;



void CGraphBase::ModifyItems(bool redraw) {
	if (auto pSeq = parent_.GetSequence()) {
		m_iHighlightedItem = -1;
		m_iHighlightedValue = 0;

		int index1 = GetItemIndex(m_ptLineStart);
		int index2 = GetItemIndex(m_ptLineEnd);
		int value1 = GetItemValue(m_ptLineStart);
		int value2 = GetItemValue(m_ptLineEnd);

		if (index1 == index2)
			pSeq->SetItem(index1, GetSequenceItemValue(index1, std::clamp(value1, GetMinItemValue(), GetMaxItemValue())));
		else {
			if (index1 > index2) {
				std::swap(index1, index2);
				std::swap(value1, value2);
			}
			if (value1 <= value2) {
				int offs = 0;
				for (int i = index1; i <= index2; ++i) {
					auto delta = static_cast<int>(.5 + static_cast<double>(offs) / (index2 - index1));
					pSeq->SetItem(i, GetSequenceItemValue(i, std::clamp(value1 + delta, GetMinItemValue(), GetMaxItemValue())));
					offs += value2 - value1;
				}
			}
			else {
				int offs = 0;
				for (int i = index2; i >= index1; --i) {
					auto delta = static_cast<int>(.5 + static_cast<double>(offs) / (index2 - index1));
					pSeq->SetItem(i, GetSequenceItemValue(i, std::clamp(value2 + delta, GetMinItemValue(), GetMaxItemValue())));
					offs += value1 - value2;
				}
			}
		}

		if (redraw)
			parent_.ItemModified();		// // //
	}
}

int CGraphBase::GetSequenceItemValue(int idx, int val) const {
	return val;
}

template <COLORREF COL_BG1, COLORREF COL_BG2, COLORREF COL_EDGE1, COLORREF COL_EDGE2>
void CGraphBase::DrawRect(CDC &dc, int x, int y, int w, int h, bool flat)
{
	if (h == 0)
		h = 1;

	if (flat) {
		GradientRect(dc, x + 1, y + 1, w - 2, h - 2, COL_BG1, COL_BG2);		// // //
		dc.Draw3dRect(x, y, w, h, COL_EDGE2, COL_EDGE2);
	}
	else {
		GradientRect(dc, x + 1, y + 1, w - 2, h - 2, COL_BG1, COL_BG2);
		dc.Draw3dRect(x, y, w, h, COL_EDGE1, COL_EDGE2);
	}
}

int CGraphBase::GetItemBottom() const {
	return region_.bottom;
}

int CGraphBase::GetItemWidth() const {
	unsigned Count = parent_.GetItemCount();		// // //
	return Count ? std::min(region_.Width() / static_cast<int>(Count), ITEM_MAX_WIDTH) : 0;		// // //
}

int CGraphBase::GetItemIndex(CPoint point) const {		// // //
	return parent_.GetItemCount() && point.x >= CGraphEditor::GRAPH_LEFT ? (point.x - CGraphEditor::GRAPH_LEFT) / GetItemWidth() : -1;
}

int CGraphBase::GetItemGridIndex(CPoint point) const {		// // //
	int ItemWidth = GetItemWidth();
	int idx = point.x - CGraphEditor::GRAPH_LEFT + ItemWidth / 2;
	return idx >= 0 ? idx / ItemWidth : -1;
}

void CGraphBase::DrawRect(CDC &dc, int x, int y, int w, int h) {
	DrawRect<GREY(240), GREY(208), WHITE, GREY(160)>(dc, x, y, w, h, false);
}

void CGraphBase::DrawPlayRect(CDC &dc, int x, int y, int w, int h) {
	DrawRect<MakeRGB(160, 240, 160), MakeRGB(134, 220, 134), MakeRGB(198, 242, 198), MakeRGB(106, 223, 106)>(dc, x, y, w, h, false);
}

void CGraphBase::DrawCursorRect(CDC &dc, int x, int y, int w, int h) {
	DrawRect<MakeRGB(192, 224, 255), MakeRGB(153, 198, 229), MakeRGB(215, 235, 253), MakeRGB(120, 182, 226)>(dc, x, y, w, h, m_iEditing == edit_t::Point);
}

void CGraphBase::DrawShadowRect(CDC &dc, int x, int y, int w, int h) {
	if (h == 0)
		h = 1;

	GradientBar(dc, x, y, w, h, GREY(72), GREY(68));		// // //
}

void CGraphBase::DrawLine(CDC &dc) {
	if (m_iEditing == edit_t::Line) {		// // //
		CPen Pen;
		Pen.CreatePen(1, 3, WHITE);
		CPen *pOldPen = dc.SelectObject(&Pen);
		dc.MoveTo(m_ptLineStart);
		dc.LineTo(m_ptLineEnd);
		dc.SelectObject(pOldPen);
	}
}

void CGraphBase::DrawBackground(CDC &dc, int Lines, bool DrawMarks, int MarkOffset) {
	const COLORREF COL_BOTTOM		= GREY(64);
	const COLORREF COL_HORZ_BAR		= GREY(32);

	const int Top = GetItemTop();		// // //
	const int Bottom = GetItemBottom();

	// Fill background
	dc.FillSolidRect(region_, BLACK);
	dc.FillSolidRect(region_.left, Top, 1, region_.bottom, COL_BOTTOM);

	if (auto pSeq = parent_.GetSequence()) {
		int ItemWidth = GetItemWidth();

		// Draw horizontal bars
		int count = pSeq->GetItemCount();
		for (int i = 1; i < count; i += 2) {
			int x = region_.left + i * ItemWidth + 1;
			int y = Top + 1;
			int w = ItemWidth;
			int h = Bottom - Top;		// // //
			dc.FillSolidRect(x, y, w, h, COL_HORZ_BAR);
		}
	}

	int marker = MarkOffset;

	if (Lines > 0) {
		int StepHeight = (Bottom - Top) / Lines;

		// Draw vertical bars
		for (int i = 0; i <= Lines; ++i) {
			int x = region_.left + 1;
			int y = Top + StepHeight * i;
			int w = region_.Width() - 2;
			int h = 1;

			if (DrawMarks) {
				if ((++marker + 1) % 12 == 0) {
					dc.FillSolidRect(x, y, w, StepHeight, COL_HORZ_BAR);
				}
			}

			dc.FillSolidRect(x, y, w, h, GREY(64));
		}
	}
}

void CGraphBase::DrawRange(CDC &dc, int Max, int Min) {
	CFont *pOldFont = dc.SelectObject(&font_);		// // //
	const int Top = GetItemTop();		// // //
	const int Bottom = GetItemBottom();

	dc.FillSolidRect(region_.left - DPI::SX(CGraphEditor::GRAPH_LEFT), Top, region_.left, Bottom, BLACK);

	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(WHITE);
	dc.SetTextAlign(TA_RIGHT);

	dc.TextOutW(region_.left - 4, Top - 3, FormattedW(L"%02i", Max));
	dc.TextOutW(region_.left - 4, Bottom - 10, FormattedW(L"%02i", Min));

	dc.SelectObject(pOldFont);
}

void CGraphBase::DoOnMouseHover(CPoint point) {
	const int ItemIndex = GetItemIndex(point);
	const int ItemValue = GetItemValue(point);
	const int LastItem = m_iHighlightedItem;
	const int LastValue = m_iHighlightedValue;

	if (ItemIndex < 0 || ItemIndex >= parent_.GetItemCount() || ItemValue < GetMinItemValue() || ItemValue > GetMaxItemValue()) {
		m_iHighlightedItem = -1;
		m_iHighlightedValue = 0;
	}
	else {
		m_iHighlightedItem = ItemIndex;
		m_iHighlightedValue = ItemValue;
	}

	if (m_iHighlightedItem != LastItem || m_iHighlightedValue != LastValue)
		parent_.RedrawWindow();

	CursorChanged(ItemIndex);
}

void CGraphBase::DoOnLButtonDown(CPoint point) {
	m_iEditing = edit_t::Point;
	m_ptLineStart = m_ptLineEnd = point;
	ModifyItems(true);
	CursorChanged(GetItemIndex(point));
}

void CGraphBase::DoOnLButtonMove(CPoint point) {
	if (m_iEditing == edit_t::Point) {
		m_ptLineEnd = point;		// // //
		ModifyItems(true);
		m_ptLineStart = m_ptLineEnd;
	}
	CursorChanged(GetItemIndex(point));
}

void CGraphBase::DoOnRButtonDown(CPoint point) {
	m_iEditing = edit_t::Line;
	m_ptLineStart = m_ptLineEnd = point;
	ModifyItems(true);
	CursorChanged(GetItemIndex(point));
}

void CGraphBase::DoOnRButtonMove(CPoint point) {
	if (m_iEditing == edit_t::Line) {
		m_ptLineEnd = point;		// // //
		ModifyItems(true);
	}
	CursorChanged(GetItemIndex(point));
}

void CGraphBase::DoOnRButtonUp(CPoint point) {
	m_ptLineStart = { };
	m_iEditing = edit_t::None;
	CursorChanged(GetItemIndex(point));
}



CBarGraph::CBarGraph(CGraphEditor &parent, CRect region, int items) :
	CGraphBase(parent, region), items_(items)
{
}

int CBarGraph::GetItemValue(CPoint point) const {
	int ItemHeight = GetItemHeight();
	return items_ - (((int)point.y - GetItemTop() + ItemHeight / 2) / ItemHeight);
}

int CBarGraph::GetMinItemValue() const {
	return 0;
}

int CBarGraph::GetMaxItemValue() const {
	return items_;
}

int CBarGraph::GetItemTop() const {
	return region_.top + region_.Height() % items_;
}

int CBarGraph::GetItemHeight() const {
	return (GetItemBottom() - GetItemTop()) / items_;		// // //
}

void CBarGraph::DoOnPaint(CDC &dc) {
	DrawBackground(dc, items_, false, 0);
	DrawRange(dc, items_, 0);

	auto pSeq = parent_.GetSequence();
	if (!pSeq)
		return;
	int Count = pSeq->GetItemCount();

	const int StepWidth = GetItemWidth();
	const int StepHeight = GetItemHeight();		// // //
	const int Top = GetItemTop();		// // //

	if (m_iHighlightedValue > 0 && m_iHighlightedItem >= 0 && m_iHighlightedItem < Count) {
		int x = region_.left + m_iHighlightedItem * StepWidth + 1;
		int y = Top + StepHeight * (items_ - m_iHighlightedValue);
		int w = StepWidth;
		int h = StepHeight * m_iHighlightedValue;
		DrawShadowRect(dc, x, y, w, h);
	}

	// Draw items
	for (int i = 0; i < Count; ++i) {
		int x = region_.left + i * StepWidth + 1;
		int y = Top + StepHeight * (items_ - pSeq->GetItem(i));
		int w = StepWidth;
		int h = StepHeight * pSeq->GetItem(i);

		if (parent_.GetCurrentPlayPos() == i)
			DrawPlayRect(dc, x, y, w, h);
		else if ((m_iHighlightedItem == i) && (pSeq->GetItem(i) >= m_iHighlightedValue))
			DrawCursorRect(dc, x, y, w, h);
		else
			DrawRect(dc, x, y, w, h);
	}

	DrawLine(dc);
}



CCellGraph::CCellGraph(CGraphEditor &parent, CRect region, int items) :
	CGraphBase(parent, region), items_(items)
{
}

int CCellGraph::GetItemValue(CPoint point) const {
	const int ScrollOffset = dynamic_cast<CArpeggioGraphEditor &>(parent_).GetGraphScrollOffset();
	int y = (point.y - GetItemTop()) / GetItemHeight() - ScrollOffset;		// // //

	if (auto pSeq = parent_.GetSequence())
		switch (pSeq->GetSetting()) {		// // //
		case SETTING_ARP_FIXED:  return items_ - 1 - y;
		}
	return items_ / 2 - y;
}

int CCellGraph::GetMinItemValue() const {
	if (auto pSeq = parent_.GetSequence())
		switch (pSeq->GetSetting()) {		// // //
		case SETTING_ARP_FIXED:  return 0;
		case SETTING_ARP_SCHEME: return ARPSCHEME_MIN;
		}
	return -NOTE_COUNT;
}

int CCellGraph::GetMaxItemValue() const {
	if (auto pSeq = parent_.GetSequence())
		switch (pSeq->GetSetting()) {		// // //
		case SETTING_ARP_FIXED:  return NOTE_COUNT - 1;
		case SETTING_ARP_SCHEME: return ARPSCHEME_MAX;
		}
	return NOTE_COUNT;
}

int CCellGraph::GetSequenceItemValue(int idx, int val) const {
	if (auto pSeq = parent_.GetSequence())
		if (pSeq->GetSetting() == SETTING_ARP_SCHEME)		// // //
			return (pSeq->GetItem(idx) & 0xC0) | static_cast<uint8_t>(val & 0x3F);
	return val;
}

int CCellGraph::GetItemTop() const {
	return region_.top + region_.Height() % items_;
}

int CCellGraph::GetItemHeight() const {
	return (GetItemBottom() - GetItemTop()) / items_;		// // //
}

void CCellGraph::DrawRange(CDC &dc, int Max, int Min) {
	if (auto pSeq = parent_.GetSequence(); pSeq && pSeq->GetSetting() == SETTING_ARP_FIXED) {
		dc.FillSolidRect(region_.left - DPI::SX(CGraphEditor::GRAPH_LEFT), region_.top, region_.left, region_.bottom, BLACK);

		CFont *pOldFont = dc.SelectObject(&font_);

		dc.SetTextColor(WHITE);
		dc.SetTextAlign(TA_RIGHT);

		// Top
		int NoteValue1 = dynamic_cast<CArpeggioGraphEditor &>(parent_).GetGraphScrollOffset() + 20;
		auto label1 = conv::to_wide(GetNoteString(GET_NOTE(NoteValue1), GET_OCTAVE(NoteValue1)));
		dc.TextOutW(region_.left - 4, GetItemTop() - 3, label1.data());		// // //

		// Bottom
		int NoteValue2 = dynamic_cast<CArpeggioGraphEditor &>(parent_).GetGraphScrollOffset();
		auto label2 = conv::to_wide(GetNoteString(GET_NOTE(NoteValue2), GET_OCTAVE(NoteValue2)));
		dc.TextOutW(region_.left - 4, GetItemBottom() - 10, label2.data());

		dc.SelectObject(pOldFont);
		return;
	}

	CGraphBase::DrawRange(dc, Max, Min);
}

void CCellGraph::DoOnPaint(CDC &dc) {
	auto pSeq = parent_.GetSequence();
	const int ScrollOffset = dynamic_cast<CArpeggioGraphEditor &>(parent_).GetGraphScrollOffset();
	DrawBackground(dc, items_, true, pSeq && pSeq->GetSetting() == SETTING_ARP_FIXED ? 2 - ScrollOffset : -ScrollOffset);		// // //
	DrawRange(dc, ScrollOffset + 10, ScrollOffset - 10);

	if (!pSeq)
		return;
	int Count = pSeq->GetItemCount();

	const int StepWidth = GetItemWidth();
	const int StepHeight = GetItemHeight();		// // //
	const int Top = GetItemTop();		// // //

	// One last line
//	dc.FillSolidRect(region_.left + 1, Top + items_ * StepHeight, region_.Width() - 2, 1, COLOR_LINES);

	if (m_iHighlightedItem >= 0 && m_iHighlightedItem < Count) {
		int item;			// // //
		if (pSeq->GetSetting() == SETTING_ARP_SCHEME) {
			int value = (m_iHighlightedValue + 0x100) % 0x40;
			if (value > ARPSCHEME_MAX) value -= 0x40;
			item = (items_ / 2) - value + ScrollOffset;
		}
		else
			item = (items_ / 2) - m_iHighlightedValue + ScrollOffset;
		if (pSeq->GetSetting() == SETTING_ARP_FIXED)
			item += (items_ / 2);
		if (item >= 0 && item <= 20) {
			int x = region_.left + m_iHighlightedItem * StepWidth + 1;
			int y = Top + StepHeight * item + 1;
			int w = StepWidth;
			int h = StepHeight - 1;
			DrawShadowRect(dc, x, y, w, h);
		}
	}

	// Draw items
	for (int i = 0; i < Count; ++i) {
		int item;			// // //
		if (pSeq->GetSetting() == SETTING_ARP_SCHEME) {
			int value = (pSeq->GetItem(i) + 0x100) % 0x40;
			if (value > ARPSCHEME_MAX) value -= 0x40;
			item = (items_ / 2) - value + ScrollOffset;
		}
		else
			item = (items_ / 2) - pSeq->GetItem(i) + ScrollOffset;
		if (pSeq->GetSetting() == SETTING_ARP_FIXED)
			item += (items_ / 2);
		if (item >= 0 && item < items_) {
			int x = region_.left + i * StepWidth + 1;
			int y = Top + StepHeight * item;
			int w = StepWidth;
			int h = StepHeight;

			if (parent_.GetCurrentPlayPos() == i)
				DrawPlayRect(dc, x, y, w, h);
			else if (m_iHighlightedItem == i && (pSeq->GetItem(i) == m_iHighlightedValue) && m_iEditing != edit_t::Line)
				DrawCursorRect(dc, x, y, w, h);
			else
				DrawRect(dc, x, y, w, h);
		}
	}

	DrawLine(dc);
}



int CPitchGraph::GetItemValue(CPoint point) const {
	int MouseY = point.y - region_.top - region_.Height() / 2;
	return -MouseY * 255 / region_.Height();
}

int CPitchGraph::GetMinItemValue() const {
	return -128;
}

int CPitchGraph::GetMaxItemValue() const {
	return 127;
}

int CPitchGraph::GetItemTop() const {
	return region_.top;
}

int CPitchGraph::GetItemHeight() const {
	return 1;
}

void CPitchGraph::DoOnPaint(CDC &dc) {
	DrawBackground(dc, 0, false, 0);
	DrawRange(dc, 127, -128);

	auto pSeq = parent_.GetSequence();
	if (!pSeq) {
		dc.FillSolidRect(region_.left, region_.top + region_.Height() / 2, region_.Width(), 1, GREY(64));
		return;
	}
	int Count = pSeq->GetItemCount();

	const int StepWidth = GetItemWidth();
	const int Top = GetItemTop();		// // //

	// One last line
	// dc.FillSolidRect(region_.left + 1, region_.top + 20 * StepHeight, region_.Width() - 2, 1, COLOR_LINES);

	if (m_iHighlightedItem != -1) {
		int x = region_.left + m_iHighlightedItem * StepWidth + 1;
		int y = Top + (region_.bottom - Top) / 2;
		int w = StepWidth;
		int h = -(m_iHighlightedValue * region_.Height()) / 255;
		DrawShadowRect(dc, x, y, w, h);
	}

	bool bHighlight = false;		// // //
	if (m_iEditing != edit_t::Line && m_iHighlightedItem >= 0 && m_iHighlightedItem < Count) {
		int Value = pSeq->GetItem(m_iHighlightedItem);
		if (m_iHighlightedValue > 0)
			bHighlight = m_iHighlightedValue <= Value && Value >= 0;
		else
			bHighlight = m_iHighlightedValue >= Value && Value <= 0;
	}

	// Draw items
	for (int i = 0; i < Count; ++i) {
		int item = pSeq->GetItem(i);
		int x = region_.left + i * StepWidth + 1;
		int y = Top + region_.Height() / 2;
		int w = StepWidth;
		int h = -(item * region_.Height()) / 255;
		if (h < 0) {
			y += h;
			h = -h;
		}
		if (parent_.GetCurrentPlayPos() == i)
			DrawPlayRect(dc, x, y, w, h);
		else if (m_iHighlightedItem == i && bHighlight)
			DrawCursorRect(dc, x, y, w, h);
		else
			DrawRect(dc, x, y, w, h);
	}

	DrawLine(dc);
	dc.FillSolidRect(region_.left, region_.top + region_.Height() / 2, region_.Width(), 1, GREY(64));
}



CNoiseGraph::CNoiseGraph(CGraphEditor &parent, CRect region, int items) :
	CGraphBase(parent, region), items_(items)
{
}

int CNoiseGraph::GetItemValue(CPoint point) const {
	int ItemHeight = GetItemHeight();
	return items_ - 1 - ((int)point.y - GetItemTop() + ItemHeight / 2) / ItemHeight;
}

int CNoiseGraph::GetMinItemValue() const {
	return 0;
}

int CNoiseGraph::GetMaxItemValue() const {
	return items_ - 1;
}

int CNoiseGraph::GetSequenceItemValue(int idx, int value) const {
	auto pSeq = parent_.GetSequence();
	return value | value_cast(enum_cast<s5b_mode_t>(static_cast<uint8_t>(pSeq ? pSeq->GetItem(idx) : 0u)));		// // //
}

int CNoiseGraph::GetItemTop() const {
	return region_.top + region_.Height() % items_;
}

int CNoiseGraph::GetItemHeight() const {
	return (GetItemBottom() - GetItemTop()) / items_;		// // //
}

void CNoiseGraph::DoOnPaint(CDC &dc) {
	DrawBackground(dc, items_, false, 0);
	DrawRange(dc, items_ - 1, 0);

	auto pSeq = parent_.GetSequence();
	if (!pSeq)
		return;
	int Count = pSeq->GetItemCount();

	const int StepWidth = GetItemWidth();
	const int StepHeight = GetItemHeight();		// // //
	const int Top = GetItemTop();		// // //

	// // // One last line
	dc.FillSolidRect(region_.left + 1, Top + items_ * StepHeight, region_.Width() - 2, 1, GREY(64));

	if (m_iHighlightedItem >= 0 && m_iHighlightedItem < Count) {		// // //
		int x = region_.left + m_iHighlightedItem * StepWidth + 1;
		int y = Top + StepHeight * (items_ - 1 - m_iHighlightedValue) + 1;
		int w = StepWidth;
		int h = StepHeight - 1;
		DrawShadowRect(dc, x, y, w, h);
	}

	// Draw items
	for (int i = 0; i < Count; ++i) {
		// Draw noise frequency
		int item = pSeq->GetItem(i) & 0x1F;
		int x = region_.left + i * StepWidth + 1;
		int y = Top + StepHeight * (items_ - 1 - item);
		int w = StepWidth;
		int h = StepHeight;//* item;

		if (parent_.GetCurrentPlayPos() == i)
			DrawPlayRect(dc, x, y, w, h);
		else
			DrawRect(dc, x, y, w, h);
	}

	DrawLine(dc);
}



CLoopReleaseBar::CLoopReleaseBar(CGraphEditor &parent, CRect region, CGraphBase &graph_parent) :
	CGraphEditorComponent(parent, region), graph_parent_(graph_parent)
{
}

void CLoopReleaseBar::DoOnMouseHover(CPoint point) {
	int ItemIndex = graph_parent_.GetItemIndex(point);
	int LastItem = highlighted_;

	if (ItemIndex < 0 || ItemIndex >= parent_.GetItemCount() || !ContainsPoint(point))
		highlighted_ = -1;
	else
		highlighted_ = ItemIndex;

	if (highlighted_ != LastItem)
		parent_.RedrawWindow();

	CursorChanged(ItemIndex);
}

void CLoopReleaseBar::DoOnLButtonDown(CPoint point) {
	if (auto pSeq = parent_.GetSequence()) {
		int idx = std::clamp(graph_parent_.GetItemGridIndex(point), 0, (int)pSeq->GetItemCount() - 1);
		enable_loop_ = pSeq->GetLoopPoint() != idx;
		pSeq->SetLoopPoint(enable_loop_ ? idx : -1);
		parent_.ItemModified();
	}
}

void CLoopReleaseBar::DoOnLButtonMove(CPoint point) {
	if (auto pSeq = parent_.GetSequence(); pSeq && enable_loop_) {
		int idx = std::clamp(graph_parent_.GetItemGridIndex(point), 0, (int)pSeq->GetItemCount() - 1);
		pSeq->SetLoopPoint(idx);
		parent_.ItemModified();
	}
}

void CLoopReleaseBar::DoOnRButtonDown(CPoint point) {
	if (auto pSeq = parent_.GetSequence()) {
		int idx = std::clamp(graph_parent_.GetItemGridIndex(point), 0, (int)pSeq->GetItemCount() - 1);
		enable_loop_ = pSeq->GetReleasePoint() != idx;
		pSeq->SetReleasePoint(enable_loop_ ? idx : -1);
		parent_.ItemModified();
	}
}

void CLoopReleaseBar::DoOnRButtonMove(CPoint point) {
	if (auto pSeq = parent_.GetSequence(); pSeq && enable_loop_) {
		int idx = std::clamp(graph_parent_.GetItemGridIndex(point), 0, (int)pSeq->GetItemCount() - 1);
		pSeq->SetReleasePoint(idx);
		parent_.ItemModified();
	}
}

void CLoopReleaseBar::DrawTagPoint(CDC &dc, int index, LPCWSTR str, COLORREF col) {
	if (index > -1) {
		CFont *pOldFont = dc.SelectObject(&font_);

		int x = graph_parent_.GetItemWidth() * index + CGraphEditor::GRAPH_LEFT + 1;
		GradientBar(dc, x + 1, region_.top, region_.right - x, region_.Height(), DIM(col, 2. / 3.), DIM(col, 1. / 6.));		// // //
		dc.FillSolidRect(x, region_.top, 1, region_.bottom, col);

		dc.SetTextColor(WHITE);
		dc.SetBkMode(TRANSPARENT);
		dc.SetTextAlign(TA_LEFT);
		dc.TextOutW(x + 4, region_.top, str);

		dc.SelectObject(pOldFont);
	}
}

void CLoopReleaseBar::DoOnPaint(CDC &dc) {
	const COLORREF COL_BOTTOM = GREY(64);
	dc.FillSolidRect(region_, COL_BOTTOM);

	DrawTagPoint(dc, highlighted_, L"", GREY(128));

	if (auto pSeq = parent_.GetSequence()) {
		int LoopPoint = pSeq->GetLoopPoint();
		int ReleasePoint = pSeq->GetReleasePoint();

		if (ReleasePoint > LoopPoint) {
			DrawTagPoint(dc, LoopPoint, L"Loop", MakeRGB(0, 192, 192));
			DrawTagPoint(dc, ReleasePoint, L"Release", MakeRGB(192, 0, 192));
		}
		else if (ReleasePoint < LoopPoint) {
			DrawTagPoint(dc, ReleasePoint, L"Release", MakeRGB(192, 0, 192));
			DrawTagPoint(dc, LoopPoint, L"Loop", MakeRGB(0, 192, 192));
		}
		else if (LoopPoint > -1) { // LoopPoint == ReleasePoint
			DrawTagPoint(dc, LoopPoint, L"Loop, Release", MakeRGB(192, 192, 0));
		}
	}
}



CArpSchemeSelector::CArpSchemeSelector(CGraphEditor &parent, CRect region, CGraphBase &graph_parent) :
	CGraphEditorComponent(parent, region), graph_parent_(graph_parent)
{
}

void CArpSchemeSelector::ModifyArpScheme(int idx) {
	if (auto pSeq = parent_.GetSequence()) {
		auto ItemValue = static_cast<uint8_t>(pSeq->GetItem(idx) & 0x3F) | static_cast<uint8_t>(value_cast(next_scheme_));
		pSeq->SetItem(idx, ItemValue);
		parent_.ItemModified();		// // //
	}
}

arp_scheme_mode_t CArpSchemeSelector::GetNextScheme(arp_scheme_mode_t scheme, bool fwd) {
	switch (scheme) {
	case arp_scheme_mode_t::none:
		return fwd ? arp_scheme_mode_t::X : arp_scheme_mode_t::NegY;
	case arp_scheme_mode_t::X:
		return fwd ? arp_scheme_mode_t::Y : arp_scheme_mode_t::none;
	case arp_scheme_mode_t::Y:
		return fwd ? arp_scheme_mode_t::NegY : arp_scheme_mode_t::X;
	case arp_scheme_mode_t::NegY:
		return fwd ? arp_scheme_mode_t::none : arp_scheme_mode_t::Y;
	}
	return arp_scheme_mode_t::none;
}

void CArpSchemeSelector::DoOnLButtonDown(CPoint point) {
	if (auto pSeq = parent_.GetSequence()) {
		int idx = graph_parent_.GetItemIndex(point);
		if (idx < 0 || idx >= static_cast<int>(pSeq->GetItemCount()))
			return;
		next_scheme_ = GetNextScheme(enum_cast<arp_scheme_mode_t>(static_cast<uint8_t>(pSeq->GetItem(idx) & 0xC0)), true);
		ModifyArpScheme(idx);
		CursorChanged(idx);
	}
}

void CArpSchemeSelector::DoOnLButtonMove(CPoint point) {
	if (auto pSeq = parent_.GetSequence()) {
		int idx = graph_parent_.GetItemIndex(point);
		if (idx < 0 || idx >= static_cast<int>(pSeq->GetItemCount()))
			return;
		ModifyArpScheme(idx);
		CursorChanged(idx);
	}
}

void CArpSchemeSelector::DoOnRButtonDown(CPoint point) {
	if (auto pSeq = parent_.GetSequence()) {
		int idx = graph_parent_.GetItemIndex(point);
		if (idx < 0 || idx >= static_cast<int>(pSeq->GetItemCount()))
			return;
		next_scheme_ = GetNextScheme(enum_cast<arp_scheme_mode_t>(static_cast<uint8_t>(pSeq->GetItem(idx) & 0xC0)), false);
		ModifyArpScheme(idx);
		CursorChanged(idx);
	}
}

void CArpSchemeSelector::DoOnRButtonMove(CPoint point) {
	if (auto pSeq = parent_.GetSequence()) {
		int idx = graph_parent_.GetItemIndex(point);
		if (idx < 0 || idx >= static_cast<int>(pSeq->GetItemCount()))
			return;
		ModifyArpScheme(idx);
		CursorChanged(idx);
	}
}

void CArpSchemeSelector::DoOnPaint(CDC &dc) {
	dc.FillSolidRect(region_, BLACK);

	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(WHITE);
	dc.SetTextAlign(TA_CENTER);

	CFont *oldFont = dc.SelectObject(&font_);

	if (auto pSeq = parent_.GetSequence()) {
		int Count = pSeq->GetItemCount();

		int StepWidth = graph_parent_.GetItemWidth();

		for (int i = 0; i < Count; ++i) {
			int x = region_.left + i * StepWidth + 1;
			int y = region_.top;
			int w = StepWidth;
			int h = region_.Height();

			auto scheme = enum_cast<arp_scheme_mode_t>(static_cast<uint8_t>(pSeq->GetItem(i) & 0xC0u));		// // //
			const COLORREF col = [] (arp_scheme_mode_t scheme) {
				switch (scheme) {
				case arp_scheme_mode_t::X: return MakeRGB(192, 32, 32);
				case arp_scheme_mode_t::Y: return MakeRGB(32, 160, 32);
				case arp_scheme_mode_t::NegY: return MakeRGB(32, 32, 255);
				case arp_scheme_mode_t::none: default: return GREY(96);
				}
			}(scheme);
			const LPCWSTR label = [] (arp_scheme_mode_t scheme) {
				switch (scheme) {
				case arp_scheme_mode_t::X: return L"x";
				case arp_scheme_mode_t::Y: return L"y";
				case arp_scheme_mode_t::NegY: return L"-y";
				case arp_scheme_mode_t::none: default: return L"";
				}
			}(scheme);

			dc.FillSolidRect(x, y, w, h, col);
			dc.Draw3dRect(x, y, w, h, BLEND(col, WHITE, .8), BLEND(col, BLACK, .8));
			dc.TextOutW(x + w / 2, y - 2, label);
		}
	}

	dc.SelectObject(oldFont);
}



namespace {

constexpr s5b_mode_t S5B_FLAGS[] = {s5b_mode_t::Envelope, s5b_mode_t::Square, s5b_mode_t::Noise};
constexpr auto BUTTON_MARGIN = CNoiseSelector::BUTTON_HEIGHT * 3;

} // namespace

CNoiseSelector::CNoiseSelector(CGraphEditor &parent, CRect region, CGraphBase &graph_parent) :
	CGraphEditorComponent(parent, region), graph_parent_(graph_parent)
{
}

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
		parent_.ItemModified();		// // //
	}
}

bool CNoiseSelector::CheckFlag(int8_t val, int flag) {
	if (flag >= 0 && flag < (int)std::size(S5B_FLAGS))
		return val & value_cast(S5B_FLAGS[flag]);
	return false;
}

void CNoiseSelector::DoOnLButtonDown(CPoint point) {
	if (auto pSeq = parent_.GetSequence()) {
		int idx = graph_parent_.GetItemIndex(point);
		if (idx < 0 || idx >= static_cast<int>(pSeq->GetItemCount()))
			return;
		int flag = (point.y - (region_.bottom - BUTTON_MARGIN)) / BUTTON_HEIGHT;		// // //
		if (flag < 0 || flag >= static_cast<int>(std::size(S5B_FLAGS)))
			return;
		enable_flag_ = !CheckFlag(pSeq->GetItem(idx), flag);
		ModifyFlag(idx, flag);
		CursorChanged(idx);
	}
}

void CNoiseSelector::DoOnLButtonMove(CPoint point) {
	if (auto pSeq = parent_.GetSequence()) {
		int idx = graph_parent_.GetItemIndex(point);
		if (idx < 0 || idx >= static_cast<int>(pSeq->GetItemCount()))
			return;
		int flag = (point.y - (region_.bottom - BUTTON_MARGIN)) / BUTTON_HEIGHT;		// // //
		if (flag < 0 || flag >= static_cast<int>(std::size(S5B_FLAGS)))
			return;
		if (idx != last_idx_ || flag != last_flag_)
			ModifyFlag(idx, flag);
		CursorChanged(idx);
	}
}

void CNoiseSelector::DoOnPaint(CDC &dc) {
	dc.FillSolidRect(region_, BLACK);

	if (auto pSeq = parent_.GetSequence()) {
		int Count = pSeq->GetItemCount();

		int StepWidth = graph_parent_.GetItemWidth();

		for (int i = 0; i < Count; ++i) {
			int x = region_.left + i * StepWidth + 1;
			int w = StepWidth;

			auto flags = enum_cast<s5b_mode_t>(static_cast<uint8_t>(pSeq->GetItem(i)));		// // //

			const COLORREF BAR_COLOR[] = {MakeRGB(160, 160, 0), MakeRGB(0, 160, 160), MakeRGB(160, 0, 160)};

			for (std::size_t j = 0; j < std::size(S5B_FLAGS); ++j) {
				int y = region_.bottom - BUTTON_MARGIN + j * BUTTON_HEIGHT + 1;
				int h = BUTTON_HEIGHT - 1;
				const COLORREF Color = (flags & S5B_FLAGS[j]) == S5B_FLAGS[j] ? BAR_COLOR[j] : GREY(80);
				dc.FillSolidRect(x, y, w, h, Color);
				dc.Draw3dRect(x, y, w, h, BLEND(Color, WHITE, .8), BLEND(Color, BLACK, .8));
			}
		}
	}

	CFont *pOldFont = dc.SelectObject(&font_);
	const int Bottom = graph_parent_.GetItemBottom();

	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(WHITE);
	dc.SetTextAlign(TA_RIGHT);

	const LPCWSTR FLAG_STR[] = {L"T", L"N", L"E"};
	for (std::size_t i = 0; i < std::size(FLAG_STR); ++i)
		dc.TextOutW(CGraphEditor::GRAPH_LEFT - 4, Bottom + BUTTON_HEIGHT * i - 1, FLAG_STR[i]);

	dc.SelectObject(pOldFont);
}
