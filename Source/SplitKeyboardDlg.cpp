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

#include "SplitKeyboardDlg.h"
#include "FamiTrackerTypes.h"
#include "PatternNote.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"
#include "InstrumentManager.h"
#include "APU/Types.h"
#include "ChannelName.h"
#include "ChannelOrder.h"
#include "str_conv/str_conv.hpp"
#include "NumConv.h"

// CSplitKeyboardDlg dialog

const CStringW KEEP_INST_STRING = L"Keep";
const int CSplitKeyboardDlg::MAX_TRANSPOSE = 24;

IMPLEMENT_DYNAMIC(CSplitKeyboardDlg, CDialog)

CSplitKeyboardDlg::CSplitKeyboardDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_SPLIT_KEYBOARD, pParent),
	m_bSplitEnable(false),
	m_iSplitChannel(chan_id_t::NONE),
	m_iSplitNote(-1),
	m_iSplitInstrument(MAX_INSTRUMENTS),
	m_iSplitTranspose(0)
{

}

CSplitKeyboardDlg::~CSplitKeyboardDlg()
{
}

void CSplitKeyboardDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSplitKeyboardDlg, CDialog)
	ON_BN_CLICKED(IDC_CHECK_SPLIT_ENABLE, OnBnClickedCheckSplitEnable)
	ON_CBN_SELCHANGE(IDC_COMBO_SPLIT_NOTE, OnCbnSelchangeComboSplitNote)
	ON_CBN_SELCHANGE(IDC_COMBO_SPLIT_OCTAVE, OnCbnSelchangeComboSplitNote)
	ON_CBN_SELCHANGE(IDC_COMBO_SPLIT_CHAN, OnCbnSelchangeComboSplitChan)
	ON_CBN_SELCHANGE(IDC_COMBO_SPLIT_INST, OnCbnSelchangeComboSplitInst)
	ON_CBN_SELCHANGE(IDC_COMBO_SPLIT_TRSP, OnCbnSelchangeComboSplitTrsp)
END_MESSAGE_MAP()


// CSplitKeyboardDlg message handlers

BOOL CSplitKeyboardDlg::OnInitDialog()
{
	CComboBox *pCombo;
	CStringW str;
	const auto pDoc = CFamiTrackerDoc::GetDoc();

	pCombo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_NOTE));
	for (auto n : stChanNote::NOTE_NAME) {
		if (n.back() == '-')
			n.pop_back();
		pCombo->AddString(conv::to_wide(n).data());
	}
	pCombo->SetCurSel(m_iSplitNote != -1 ? (GET_NOTE(m_iSplitNote) - 1) : 0);

	pCombo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_OCTAVE));
	for (int i = 0; i < OCTAVE_RANGE; ++i) {
		str.Format(L"%d", i);
		pCombo->AddString(str);
	}
	pCombo->SetCurSel(m_iSplitNote != -1 ? GET_OCTAVE(m_iSplitNote) : 3);

	pCombo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_CHAN));
	pCombo->AddString(KEEP_INST_STRING);
	pCombo->SetCurSel(0);
	int i = 0;
	pDoc->GetModule()->GetChannelOrder().ForeachChannel([&] (chan_id_t ch) {
		pCombo->AddString(conv::to_wide(GetChannelFullName(ch)).data());
		if (m_iSplitChannel == ch)
			pCombo->SetCurSel(i + 1);
		++i;
	});

	pCombo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_INST));
	pCombo->AddString(KEEP_INST_STRING);
	for (int i = 0; i < MAX_INSTRUMENTS; ++i)
		if (pDoc->GetModule()->GetInstrumentManager()->IsInstrumentUsed(i)) {
			str.Format(L"%02X", i);
			pCombo->AddString(str);
		}
	str.Format(L"%02X", m_iSplitInstrument);
	if (pCombo->SelectString(-1, str) == CB_ERR)
		pCombo->SelectString(-1, KEEP_INST_STRING);

	pCombo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_TRSP));
	for (int i = -MAX_TRANSPOSE; i <= MAX_TRANSPOSE; ++i) {
		str.Format(L"%+d", i);
		pCombo->AddString(str);
	}
	str.Format(L"%+d", m_iSplitTranspose);
	pCombo->SelectString(-1, str);

	CheckDlgButton(IDC_CHECK_SPLIT_ENABLE, m_bSplitEnable ? BST_CHECKED : BST_UNCHECKED);
	OnBnClickedCheckSplitEnable();

	return CDialog::OnInitDialog();
}

void CSplitKeyboardDlg::OnBnClickedCheckSplitEnable()
{
	if (m_bSplitEnable = (IsDlgButtonChecked(IDC_CHECK_SPLIT_ENABLE) == BST_CHECKED)) {
		OnCbnSelchangeComboSplitNote();
		OnCbnSelchangeComboSplitChan();
		OnCbnSelchangeComboSplitInst();
		OnCbnSelchangeComboSplitTrsp();
	}
	GetDlgItem(IDC_COMBO_SPLIT_NOTE)->EnableWindow(m_bSplitEnable);
	GetDlgItem(IDC_COMBO_SPLIT_OCTAVE)->EnableWindow(m_bSplitEnable);
	GetDlgItem(IDC_COMBO_SPLIT_CHAN)->EnableWindow(m_bSplitEnable);
	GetDlgItem(IDC_COMBO_SPLIT_INST)->EnableWindow(m_bSplitEnable);
	GetDlgItem(IDC_COMBO_SPLIT_TRSP)->EnableWindow(m_bSplitEnable);
}

void CSplitKeyboardDlg::OnCbnSelchangeComboSplitNote()
{
	m_iSplitNote = MIDI_NOTE(
		static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_OCTAVE))->GetCurSel(),
		static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_NOTE))->GetCurSel() + 1
	);
}

void CSplitKeyboardDlg::OnCbnSelchangeComboSplitChan()
{
	if (int Pos = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_CHAN))->GetCurSel())
		m_iSplitChannel = CFamiTrackerDoc::GetDoc()->GetModule()->GetChannelOrder().TranslateChannel(Pos - 1);
	else
		m_iSplitChannel = chan_id_t::NONE;
}

void CSplitKeyboardDlg::OnCbnSelchangeComboSplitInst()
{
	CStringW str;
	GetDlgItemTextW(IDC_COMBO_SPLIT_INST, str);
	m_iSplitInstrument = str == KEEP_INST_STRING ? MAX_INSTRUMENTS : conv::to_int(conv::to_utf8(str), 16u).value_or(MAX_INSTRUMENTS);
}

void CSplitKeyboardDlg::OnCbnSelchangeComboSplitTrsp()
{
	m_iSplitTranspose = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_TRSP))->GetCurSel() - MAX_TRANSPOSE;
}
