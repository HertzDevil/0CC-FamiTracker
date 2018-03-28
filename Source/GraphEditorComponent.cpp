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
#include "NoteName.h"
#include "NumConv.h"
#include "str_conv/str_conv.hpp"

using namespace GraphEditorComponents;



CGraphEditorComponent::CGraphEditorComponent(CGraphEditor &parent, CRect region) :
	parent_(parent), region_(region)
{
	font_.CreateFontW(-10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"Verdana");		// // //
}

bool CGraphEditorComponent::ContainsPoint(CPoint point) const {
	return region_.PtInRect(point) == TRUE;
}



void CGraphBase::ModifyItem(CPoint point, bool redraw) {
	if (auto pSeq = parent_.GetSequence()) {
		int ItemIndex = std::clamp(parent_.GetItemIndex(point), 0, (int)pSeq->GetItemCount() - 1);		// // //
		int ItemY = std::clamp(GetItemValue(point), GetMinItemValue(), GetMaxItemValue());		// // //

		m_iHighlightedItem = ItemIndex;
		m_iHighlightedValue = ItemY;

		pSeq->SetItem(ItemIndex, GetSequenceItemValue(ItemIndex, ItemY));
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
	if (m_ptLineStart.x != 0 && m_ptLineStart.y != 0) {
		CPen Pen;
		Pen.CreatePen(1, 3, WHITE);
		CPen *pOldPen = dc.SelectObject(&Pen);
		dc.MoveTo(m_ptLineStart);
		dc.LineTo(m_ptLineEnd);
		dc.SelectObject(pOldPen);
	}
}

void CGraphBase::DrawBackground(CDC &dc, int Lines, bool DrawMarks, int MarkOffset) {
	const COLORREF COL_BACKGROUND	= BLACK;
	const COLORREF COL_BOTTOM		= GREY(64);
	const COLORREF COL_HORZ_BAR		= GREY(32);

	const int Top = GetItemTop();		// // //
	const int Bottom = GetItemBottom();

	// Fill background
	dc.FillSolidRect(parent_.GetClientArea(), COL_BACKGROUND);
	dc.FillSolidRect(region_.left, Top, 1, region_.bottom, COL_BOTTOM);

	if (auto pSeq = parent_.GetSequence()) {
		int ItemWidth = parent_.GetItemWidth();

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

	dc.FillSolidRect(parent_.GetClientArea().left, Top, region_.left, Bottom, BLACK);

	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(WHITE);
	dc.SetTextAlign(TA_RIGHT);

	dc.TextOutW(CGraphEditor::GRAPH_LEFT - 4, Top - 3, FormattedW(L"%02i", Max));
	dc.TextOutW(CGraphEditor::GRAPH_LEFT - 4, Bottom - 10, FormattedW(L"%02i", Min));

	dc.SelectObject(pOldFont);
}

void CGraphBase::DoOnMouseHover(CPoint point) {
	int ItemIndex = parent_.GetItemIndex(point);
	int ItemValue = GetItemValue(point);
	int LastItem = m_iHighlightedItem;
	int LastValue = m_iHighlightedValue;

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
}

void CGraphBase::DoOnLButtonDown(CPoint point) {
	m_iEditing = edit_t::Point;
	ModifyItem(point, true);
}

void CGraphBase::DoOnLButtonMove(CPoint point) {
	if (m_iEditing == edit_t::Point)
		ModifyItem(point, true);
}

void CGraphBase::DoOnRButtonDown(CPoint point) {
	m_ptLineStart = m_ptLineEnd = point;
	m_iEditing = edit_t::Line;
}

void CGraphBase::DoOnRButtonMove(CPoint point) {
	if (m_iEditing == edit_t::Line) {
		m_ptLineEnd = point;		// // //

		const CPoint &StartPt = m_ptLineStart.x < m_ptLineEnd.x ? m_ptLineStart : m_ptLineEnd;
		const CPoint &EndPt = m_ptLineStart.x < m_ptLineEnd.x ? m_ptLineEnd : m_ptLineStart;

		auto DeltaY = (double)(EndPt.y - StartPt.y) / (EndPt.x - StartPt.x + 1);
		auto fY = (double)StartPt.y;

		for (int x = StartPt.x; x < EndPt.x; ++x) {
			ModifyItem({x, (int)fY}, false);
			fY += DeltaY;
		}

		parent_.ItemModified();
	}
}

void CGraphBase::DoOnRButtonUp(CPoint point) {
	m_ptLineStart = { };
	m_iEditing = edit_t::None;
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

int GraphEditorComponents::CBarGraph::GetItemTop() const {
	return region_.top + region_.Height() % items_;
}

int GraphEditorComponents::CBarGraph::GetItemHeight() const {
	return (GetItemBottom() - GetItemTop()) / items_;		// // //
}

void CBarGraph::DoOnPaint(CDC &dc) {
	DrawBackground(dc, items_, false, 0);
	DrawRange(dc, items_, 0);

	auto pSeq = parent_.GetSequence();
	if (!pSeq)
		return;
	int Count = pSeq->GetItemCount();

	const int StepWidth = parent_.GetItemWidth();
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
		else if ((m_iHighlightedItem == i) && (pSeq->GetItem(i) >= m_iHighlightedValue) && m_iEditing != edit_t::Line)
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
		if (pSeq->GetSetting() == SETTING_ARP_SCHEME) {		// // //
			if (::GetKeyState(VK_NUMPAD0) & 0x80)
				return (val & 0x3F) | value_cast(arp_scheme_mode_t::none);
			else if (::GetKeyState(VK_NUMPAD1) & 0x80)
				return (val & 0x3F) | value_cast(arp_scheme_mode_t::X);
			else if (::GetKeyState(VK_NUMPAD2) & 0x80)
				return (val & 0x3F) | value_cast(arp_scheme_mode_t::Y);
			else if (::GetKeyState(VK_NUMPAD3) & 0x80)
				return (val & 0x3F) | value_cast(arp_scheme_mode_t::NegY);
		}
	return val;
}

int GraphEditorComponents::CCellGraph::GetItemTop() const {
	return region_.top + region_.Height() % items_;
}

int GraphEditorComponents::CCellGraph::GetItemHeight() const {
	return (GetItemBottom() - GetItemTop()) / items_;		// // //
}

void CCellGraph::DrawRange(CDC &dc, int Max, int Min) {
	if (auto pSeq = parent_.GetSequence(); pSeq && pSeq->GetSetting() == SETTING_ARP_FIXED) {
		dc.FillSolidRect(parent_.GetClientArea().left, region_.top, region_.left, region_.bottom, BLACK);

		CFont *pOldFont = dc.SelectObject(&font_);

		dc.SetTextColor(WHITE);
		dc.SetTextAlign(TA_RIGHT);

		// Top
		int NoteValue1 = dynamic_cast<CArpeggioGraphEditor &>(parent_).GetGraphScrollOffset() + 20;
		auto label1 = conv::to_wide(GetNoteString(GET_NOTE(NoteValue1), GET_OCTAVE(NoteValue1)));
		dc.TextOutW(CGraphEditor::GRAPH_LEFT - 4, GetItemTop() - 3, label1.data());		// // //

		// Bottom
		int NoteValue2 = dynamic_cast<CArpeggioGraphEditor &>(parent_).GetGraphScrollOffset();
		auto label2 = conv::to_wide(GetNoteString(GET_NOTE(NoteValue2), GET_OCTAVE(NoteValue2)));
		dc.TextOutW(CGraphEditor::GRAPH_LEFT - 4, GetItemBottom() - 10, label2.data());

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

	const int StepWidth = parent_.GetItemWidth();
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
	CFont *pOldFont = dc.SelectObject(&font_);		// // //
	dc.SetTextAlign(TA_CENTER);
	dc.SetTextColor(WHITE);
	dc.SetBkMode(TRANSPARENT);
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

			if (pSeq->GetSetting() == SETTING_ARP_SCHEME) {
				const LPCWSTR HEAD[] = {L"", L"x", L"y", L"-y"};
				dc.TextOutW(x + w / 2, y - 2 * h, HEAD[(pSeq->GetItem(i) & 0xFF) >> 6]);
			}
		}
	}
	dc.SetTextAlign(TA_LEFT);
	dc.SelectObject(pOldFont);

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

	const int StepWidth = parent_.GetItemWidth();
	const int Top = GetItemTop();		// // //

	// One last line
	// dc.FillSolidRect(region_.left + 1, region_.top + 20 * StepHeight, region_.Width() - 2, 1, COLOR_LINES);

	if (m_iHighlightedItem != -1) {
		int x = region_.left + m_iHighlightedItem * StepWidth + 1;
		int y = Top + (region_.bottom - Top) / 2;
		int w = StepWidth;
		int h = -(m_iHighlightedValue * region_.Height()) / 255 ;
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
	return region_.top + (region_.Height() - GraphEditorComponents::CNoiseSelector::BUTTON_HEIGHT * 3) % items_;
}

int CNoiseGraph::GetItemBottom() const {
	return region_.bottom - GraphEditorComponents::CNoiseSelector::BUTTON_HEIGHT * 3;
}

int CNoiseGraph::GetItemHeight() const {
	return (GetItemBottom() - GetItemTop()) / items_;		// // //
}

void CNoiseGraph::DrawRange(CDC &dc, int Max, int Min) {
	CGraphBase::DrawRange(dc, Max, Min);

	CFont *pOldFont = dc.SelectObject(&font_);
	const int Bottom = GetItemBottom();

	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(WHITE);
	dc.SetTextAlign(TA_RIGHT);

	const LPCWSTR FLAG_STR[] = {L"T", L"N", L"E"};
	for (std::size_t i = 0; i < std::size(FLAG_STR); ++i)
		dc.TextOutW(CGraphEditor::GRAPH_LEFT - 4, Bottom + GraphEditorComponents::CNoiseSelector::BUTTON_HEIGHT * i - 1, FLAG_STR[i]);

	dc.SelectObject(pOldFont);
}

void CNoiseGraph::DoOnPaint(CDC &dc) {
	DrawBackground(dc, items_, false, 0);
	DrawRange(dc, items_ - 1, 0);

	auto pSeq = parent_.GetSequence();
	if (!pSeq)
		return;
	int Count = pSeq->GetItemCount();

	const int StepWidth = parent_.GetItemWidth();
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



void CLoopReleaseBar::DoOnLButtonDown(CPoint point) {
	if (auto pSeq = parent_.GetSequence()) {
		int idx = std::clamp(parent_.GetItemGridIndex(point), 0, (int)pSeq->GetItemCount() - 1);
		enable_loop_ = pSeq->GetLoopPoint() != idx;
		pSeq->SetLoopPoint(enable_loop_ ? idx : -1);
		parent_.ItemModified();
	}
}

void CLoopReleaseBar::DoOnLButtonMove(CPoint point) {
	if (auto pSeq = parent_.GetSequence(); pSeq && enable_loop_) {
		int idx = std::clamp(parent_.GetItemGridIndex(point), 0, (int)pSeq->GetItemCount() - 1);
		pSeq->SetLoopPoint(idx);
		parent_.ItemModified();
	}
}

void CLoopReleaseBar::DoOnRButtonDown(CPoint point) {
	if (auto pSeq = parent_.GetSequence()) {
		int idx = std::clamp(parent_.GetItemGridIndex(point), 0, (int)pSeq->GetItemCount() - 1);
		enable_loop_ = pSeq->GetReleasePoint() != idx;
		pSeq->SetReleasePoint(enable_loop_ ? idx : -1);
		parent_.ItemModified();
	}
}

void CLoopReleaseBar::DoOnRButtonMove(CPoint point) {
	if (auto pSeq = parent_.GetSequence(); pSeq && enable_loop_) {
		int idx = std::clamp(parent_.GetItemGridIndex(point), 0, (int)pSeq->GetItemCount() - 1);
		pSeq->SetReleasePoint(idx);
		parent_.ItemModified();
	}
}

void CLoopReleaseBar::DrawTagPoint(CDC &dc, int index, LPCWSTR str, COLORREF col) {
	if (index > -1) {
		CFont *pOldFont = dc.SelectObject(&font_);

		int x = parent_.GetItemWidth() * index + CGraphEditor::GRAPH_LEFT + 1;
		GradientBar(dc, x + 1, region_.top, region_.right - x, region_.Height(), DIM(col, 8. / 15.), DIM(col, 2. / 15.));		// // //
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

		const COLORREF BAR_COLOR[] = {MakeRGB(160, 160, 0), MakeRGB(0, 160, 160), MakeRGB(160, 0, 160)};

		for (std::size_t j = 0; j < std::size(S5B_FLAGS); ++j) {
			int y = region_.bottom - BUTTON_MARGIN + j * BUTTON_HEIGHT + 1;
			int h = BUTTON_HEIGHT - 1;
			const COLORREF Color = (flags & S5B_FLAGS[j]) == S5B_FLAGS[j] ? BAR_COLOR[j] : GREY(80);
			dc.FillSolidRect(x, y, w, h, Color);
			dc.Draw3dRect(x, y, w, h, BLEND(Color, WHITE, .8), BLEND(Color, BLACK, .8));
		}
	}

	/*
	CFont *pOldFont = dc.SelectObject(&font_);
	const int Bottom = GetItemBottom();

	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(WHITE);
	dc.SetTextAlign(TA_RIGHT);

	const LPCWSTR FLAG_STR[] = {L"T", L"N", L"E"};
	for (std::size_t i = 0; i < std::size(FLAG_STR); ++i)
		dc.TextOutW(CGraphEditor::GRAPH_LEFT - 4, Bottom + GraphEditorComponents::CNoiseSelector::BUTTON_HEIGHT * i - 1, FLAG_STR[i]);

	dc.SelectObject(pOldFont);
	*/

#ifdef _DEBUG
	const COLORREF COL_DEBUG_RECT = MakeRGB(255, 128, 255);
	dc.FillSolidRect(region_.left, region_.top, region_.Width(), 1, COL_DEBUG_RECT);
	dc.FillSolidRect(region_.left, region_.top, 1, region_.Height(), COL_DEBUG_RECT);
	dc.FillSolidRect(region_.left, region_.bottom - 1, region_.Width(), 1, COL_DEBUG_RECT);
	dc.FillSolidRect(region_.right - 1, region_.top, 1, region_.Height(), COL_DEBUG_RECT);
#endif
}
