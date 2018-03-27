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

/*
 * The sequence setting button. Used only by arpeggio at the moment
 */

#include "SequenceSetting.h"
#include "Sequence.h"
#include "SequenceEditorMessage.h"		// // //
#include "Instrument.h"		// // // inst_type_t

// Sequence setting menu

namespace {

const std::wstring_view SEQ_SETTING_TEXT[][SEQ_COUNT] = {		// // // 050B
	{L"16 steps", L"Absolute", L"Relative", L"", L""},
	{L"64 steps",    L"Fixed", L"Absolute", L"", L""},
#ifdef _DEBUG
	{        L"", L"Relative",    L"Sweep", L"", L""},
#else
	{        L"", L"Relative",         L"", L"", L""},
#endif
	{        L"",   L"Scheme",         L"", L"", L""},
};

} // namespace

const UINT CSequenceSetting::MENU_ID_BASE = 0x1000U;		// // //
const UINT CSequenceSetting::MENU_ID_MAX  = 0x100FU;		// // //

IMPLEMENT_DYNAMIC(CSequenceSetting, CWnd)

CSequenceSetting::CSequenceSetting(CWnd *pParent)
	: CWnd(), m_pParent(pParent), m_pSequence(NULL), m_bMouseOver(false)
{
}

CSequenceSetting::~CSequenceSetting()
{
}

BEGIN_MESSAGE_MAP(CSequenceSetting, CWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_COMMAND_RANGE(MENU_ID_BASE, MENU_ID_MAX, OnMenuSettingChanged)		// // //
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()


void CSequenceSetting::Setup(CFont *pFont)
{
	m_pFont = pFont;
}

void CSequenceSetting::OnPaint()
{
	CPaintDC dc {this};
	CRect rect;
	GetClientRect(&rect);

	unsigned mode = m_pSequence->GetSetting();		// // //
	std::wstring_view sv = mode < std::size(SEQ_SETTING_TEXT) ? SEQ_SETTING_TEXT[mode][(unsigned)m_pSequence->GetSequenceType()] : std::wstring_view { };
	if (sv.empty()) {
		dc.FillSolidRect(rect, 0xFFFFFF);
		return;
	}

	int BgColor = m_bMouseOver ? 0x303030 : 0x101010;

	dc.FillSolidRect(rect, BgColor);
	dc.DrawEdge(rect, EDGE_SUNKEN, BF_RECT);
	dc.SelectObject(m_pFont);
	dc.SetTextColor(0xFFFFFF);
	dc.SetBkColor(BgColor);

	rect.top += 2;
	dc.DrawTextW(sv.data(), sv.size(), rect, DT_CENTER);
}

void CSequenceSetting::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect rect;
	GetWindowRect(rect);

	if (m_pSequence->GetSequenceType() == sequence_t::none)
		return;
	std::size_t seqType = value_cast(m_pSequence->GetSequenceType());		// // //

	unsigned Setting = m_pSequence->GetSetting();		// // //
	if (SEQ_SETTING_COUNT[seqType] < 2) return;

	m_menuPopup.CreatePopupMenu();
	for (unsigned i = 0; i < std::size(SEQ_SETTING_TEXT); ++i)
		if (auto sv = SEQ_SETTING_TEXT[i][seqType]; !sv.empty())
			m_menuPopup.AppendMenuW(MFT_STRING, MENU_ID_BASE + i, sv.data());
	m_menuPopup.CheckMenuRadioItem(MENU_ID_BASE, MENU_ID_MAX, MENU_ID_BASE + Setting, MF_BYCOMMAND);
#ifndef _DEBUG
	if (m_pSequence->GetSequenceType() == sequence_t::Volume && m_iInstType != INST_VRC6)
		m_menuPopup.EnableMenuItem(MENU_ID_BASE + SETTING_VOL_64_STEPS, MFS_DISABLED);		// // // 050B
//	else if (m_pSequence->GetSequenceType() == sequence_t::Pitch && m_iInstType != INST_2A03)
//		m_menuPopup.EnableMenuItem(MENU_ID_BASE + SETTING_PITCH_SWEEP, MFS_DISABLED);
#endif
	m_menuPopup.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x + rect.left, point.y + rect.top, this);
	m_menuPopup.DestroyMenu();		// // //

	CWnd::OnLButtonDown(nFlags, point);
}

void CSequenceSetting::SelectSequence(std::shared_ptr<CSequence> pSequence, int InstrumentType)		// // //
{
	m_pSequence = std::move(pSequence);
	m_iInstType = InstrumentType;

	UpdateWindow();
	RedrawWindow();
}

void CSequenceSetting::OnMenuSettingChanged(UINT ID)		// // //
{
	unsigned Setting = m_pSequence->GetSetting();
	unsigned New = ID - MENU_ID_BASE;
	ASSERT(New < SEQ_SETTING_COUNT[value_cast(m_pSequence->GetSequenceType())]);

	const auto MapFunc = [&] (auto f) {
		for (unsigned int i = 0, Count = m_pSequence->GetItemCount(); i < Count; ++i)
			m_pSequence->SetItem(i, f(m_pSequence->GetItem(i)));
	};

	if (New != Setting) switch (m_pSequence->GetSequenceType()) {
	case sequence_t::Volume:
		switch (New) {
		case SETTING_VOL_16_STEPS:
			MapFunc([] (int8_t x) { return x / 4; }); break;
		case SETTING_VOL_64_STEPS:
			MapFunc([] (int8_t x) { return x * 4; }); break;
		}
		break;
	case sequence_t::Arpeggio:
		switch (Setting) {
		case SETTING_ARP_SCHEME:
			MapFunc([] (int8_t x) {
				int8_t Item = x & 0x3F;
				return Item > ARPSCHEME_MAX ? Item - 0x40 : Item;
			}); break;
		}
		switch (New) {
		case SETTING_ARP_SCHEME:
			MapFunc([] (int x) {
				return std::clamp(x, ARPSCHEME_MIN, ARPSCHEME_MAX) & 0x3F;
			}); break;
		case SETTING_ARP_FIXED:
			MapFunc([] (int x) {
				return std::clamp(x, 0, NOTE_COUNT - 1);
			}); break;
		}
		break;
	}

	m_pSequence->SetSetting(static_cast<seq_setting_t>(New));
	m_pParent->PostMessageW(WM_SETTING_CHANGED);		// // //
}

void CSequenceSetting::OnMouseMove(UINT nFlags, CPoint point)
{
	bool bOldMouseOver = m_bMouseOver;
	m_bMouseOver = true;
	if (bOldMouseOver != m_bMouseOver) {
		TRACKMOUSEEVENT mouseEvent;
		mouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
		mouseEvent.dwFlags = TME_LEAVE;
		mouseEvent.hwndTrack = m_hWnd;
		TrackMouseEvent(&mouseEvent);
		RedrawWindow();
	}

	CWnd::OnMouseMove(nFlags, point);
}

void CSequenceSetting::OnMouseLeave()
{
	m_bMouseOver = false;
	RedrawWindow();

	TRACKMOUSEEVENT mouseEvent;
	mouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
	mouseEvent.dwFlags = TME_CANCEL;
	mouseEvent.hwndTrack = m_hWnd;
	TrackMouseEvent(&mouseEvent);

	CWnd::OnMouseLeave();
}
