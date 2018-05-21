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
#include "ft0cc/doc/pattern_note.hpp"
#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"
#include "FamiTrackerEnv.h"
#include "SoundChipService.h"
#include "InstrumentManager.h"
#include "APU/Types.h"
#include "ChannelOrder.h"
#include "str_conv/str_conv.hpp"
#include "NumConv.h"
#include "NoteName.h"

// CSplitKeyboardDlg dialog

const LPCWSTR KEEP_INST_STRING = L"Keep";
const int CSplitKeyboardDlg::MAX_TRANSPOSE = 24;

IMPLEMENT_DYNAMIC(CSplitKeyboardDlg, CDialog)

CSplitKeyboardDlg::CSplitKeyboardDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_SPLIT_KEYBOARD, pParent),
	m_bSplitEnable(false),
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
	const auto pDoc = CFamiTrackerDoc::GetDoc();

	pCombo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_NOTE));
	for (auto n : KEY_NAME)
		pCombo->AddString(conv::to_wide(n).data());
	pCombo->SetCurSel(m_iSplitNote != -1 ? (value_cast(ft0cc::doc::pitch_from_midi(m_iSplitNote)) - 1) : 0);

	pCombo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_OCTAVE));
	for (int i = 0; i < OCTAVE_RANGE; ++i)
		pCombo->AddString(conv::to_wide(conv::from_int(i)).data());
	pCombo->SetCurSel(m_iSplitNote != -1 ? ft0cc::doc::oct_from_midi(m_iSplitNote) : 3);

	pCombo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_CHAN));
	pCombo->AddString(KEEP_INST_STRING);
	pCombo->SetCurSel(0);
	auto *pSCS = FTEnv.GetSoundChipService();
	int i = 0;
	pDoc->GetModule()->GetChannelOrder().ForeachChannel([&] (stChannelID ch) {
		pCombo->AddString(conv::to_wide(pSCS->GetChannelFullName(ch)).data());
		if (m_iSplitChannel == ch)
			pCombo->SetCurSel(i + 1);
		++i;
	});

	pCombo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_INST));
	pCombo->AddString(KEEP_INST_STRING);
	pDoc->GetModule()->GetInstrumentManager()->VisitInstruments([&] (const CInstrument &, std::size_t i) {
		pCombo->AddString(conv::to_wide(conv::from_int_hex(i, 2)).data());
	});
	if (pCombo->SelectString(-1, FormattedW(L"%02X", m_iSplitInstrument)) == CB_ERR)
		pCombo->SelectString(-1, KEEP_INST_STRING);

	pCombo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_TRSP));
	for (int j = -MAX_TRANSPOSE; j <= MAX_TRANSPOSE; ++j)
		pCombo->AddString(FormattedW(L"%+d", j));
	pCombo->SelectString(-1, FormattedW(L"%+d", m_iSplitTranspose));

	CheckDlgButton(IDC_CHECK_SPLIT_ENABLE, m_bSplitEnable ? BST_CHECKED : BST_UNCHECKED);
	OnBnClickedCheckSplitEnable();

	return CDialog::OnInitDialog();
}

void CSplitKeyboardDlg::OnBnClickedCheckSplitEnable()
{
	m_bSplitEnable = (IsDlgButtonChecked(IDC_CHECK_SPLIT_ENABLE) == BST_CHECKED);
	if (m_bSplitEnable) {
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
	m_iSplitNote = ft0cc::doc::midi_note(
		static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_OCTAVE))->GetCurSel(),
		enum_cast<ft0cc::doc::pitch>(static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_NOTE))->GetCurSel() + 1));
}

void CSplitKeyboardDlg::OnCbnSelchangeComboSplitChan()
{
	if (int Pos = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_CHAN))->GetCurSel())
		m_iSplitChannel = CFamiTrackerDoc::GetDoc()->GetModule()->GetChannelOrder().TranslateChannel(Pos - 1);
	else
		m_iSplitChannel = { };
}

void CSplitKeyboardDlg::OnCbnSelchangeComboSplitInst()
{
	CStringW str;
	GetDlgItemTextW(IDC_COMBO_SPLIT_INST, str);
	m_iSplitInstrument = str == KEEP_INST_STRING ? MAX_INSTRUMENTS : conv::to_int(str, 16u).value_or(MAX_INSTRUMENTS);
}

void CSplitKeyboardDlg::OnCbnSelchangeComboSplitTrsp()
{
	m_iSplitTranspose = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_TRSP))->GetCurSel() - MAX_TRANSPOSE;
}
