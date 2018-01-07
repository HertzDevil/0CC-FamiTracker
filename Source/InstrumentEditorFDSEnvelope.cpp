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

#include "InstrumentEditorFDSEnvelope.h"
#include "Instrument.h"
#include "SeqInstrument.h"		// // //
#include "InstrumentFDS.h"		// // //
#include "Sequence.h"
#include "SequenceEditor.h"
#include "SequenceParser.h"		// // //
#include "DPI.h"		// // //

// CInstrumentEditorFDSEnvelope dialog

IMPLEMENT_DYNAMIC(CInstrumentEditorFDSEnvelope, CSequenceInstrumentEditPanel)

CInstrumentEditorFDSEnvelope::CInstrumentEditorFDSEnvelope(CWnd* pParent) :
	CSequenceInstrumentEditPanel(CInstrumentEditorFDSEnvelope::IDD, pParent)
{
}

CInstrumentEditorFDSEnvelope::~CInstrumentEditorFDSEnvelope()
{
}

void CInstrumentEditorFDSEnvelope::DoDataExchange(CDataExchange* pDX)
{
	CInstrumentEditPanel::DoDataExchange(pDX);
}

void CInstrumentEditorFDSEnvelope::SelectInstrument(std::shared_ptr<CInstrument> pInst)
{
	m_pInstrument = std::dynamic_pointer_cast<CInstrumentFDS>(pInst);
	ASSERT(m_pInstrument);

	LoadSequence();

	SetFocus();
}


BEGIN_MESSAGE_MAP(CInstrumentEditorFDSEnvelope, CInstrumentEditPanel)
	ON_CBN_SELCHANGE(IDC_TYPE, &CInstrumentEditorFDSEnvelope::OnCbnSelchangeType)
END_MESSAGE_MAP()

// CInstrumentEditorFDSEnvelope message handlers

BOOL CInstrumentEditorFDSEnvelope::OnInitDialog()
{
	CInstrumentEditPanel::OnInitDialog();

	CRect rect;
	GetClientRect(&rect);
	rect.DeflateRect(DPI::SX(10), DPI::SY(10), DPI::SX(10), DPI::SY(45));		// // //

	m_pSequenceEditor->CreateEditor(this, rect);
	m_pSequenceEditor->SetMaxValues(MAX_VOLUME, 0);

	static_cast<CComboBox*>(GetDlgItem(IDC_TYPE))->SetCurSel(0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentEditorFDSEnvelope::UpdateSequenceString(bool Changed)		// // //
{
	SetupParser();		// // //
	SetDlgItemText(IDC_SEQUENCE_STRING, m_pParser->PrintSequence().c_str());		// // //
}

void CInstrumentEditorFDSEnvelope::SetupParser() const		// // //
{
	const auto MakeParser = [] (unsigned typ, seq_setting_t setting) -> std::unique_ptr<CSeqConversionBase> {
		switch (static_cast<sequence_t>(typ)) {
		case SEQ_VOLUME:
			return std::make_unique<CSeqConversionDefault>(0, CInstrumentEditorFDSEnvelope::MAX_VOLUME);
		case SEQ_ARPEGGIO:
			switch (setting) {
			case SETTING_ARP_SCHEME:		// // //
				return std::make_unique<CSeqConversionArpScheme>(ARPSCHEME_MIN);
			case SETTING_ARP_FIXED:
				return std::make_unique<CSeqConversionArpFixed>();
			default:
				return std::make_unique<CSeqConversionDefault>(-NOTE_COUNT, NOTE_COUNT);
			}
		case SEQ_PITCH:
			return std::make_unique<CSeqConversionDefault>(-128, 127);
		}
		__debugbreak(); return nullptr;
	};

	auto pConv = MakeParser(m_iSelectedSetting, m_pSequence->GetSetting());
	m_pSequenceEditor->SetConversion(*pConv);		// // //
	m_pParser->SetSequence(m_pSequence);
	m_pParser->SetConversion(std::move(pConv));
}

void CInstrumentEditorFDSEnvelope::OnCbnSelchangeType()
{
	CComboBox *pTypeBox = static_cast<CComboBox*>(GetDlgItem(IDC_TYPE));
	m_iSelectedSetting = (sequence_t)pTypeBox->GetCurSel();
	LoadSequence();
}

void CInstrumentEditorFDSEnvelope::LoadSequence()
{
	m_pSequence = m_pInstrument->GetSequence(m_iSelectedSetting);		// // //
	m_pSequenceEditor->SelectSequence(m_pSequence, m_iSelectedSetting, INST_FDS);
	SetupParser();		// // //
}

void CInstrumentEditorFDSEnvelope::OnKeyReturn()
{
	CString string;
	GetDlgItemText(IDC_SEQUENCE_STRING, string);
	TranslateMML(string);		// // //
}
