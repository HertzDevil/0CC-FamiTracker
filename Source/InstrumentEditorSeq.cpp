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

#include "InstrumentEditorSeq.h"
#include "SeqInstrument.h"		// // //
#include "Sequence.h"		// // //
#include "InstrumentManager.h"		// // //
#include "SequenceEditor.h"
#include "SequenceParser.h"		// // //
#include "FamiTrackerDoc.h"		// // //
#include "Assertion.h"		// // //

// // // CInstrumentEditorSeq dialog

/*
LPCTSTR CInstrumentEditorSeq::INST_SETTINGS[] = {_T(""), _T(""), _T(""), _T(""), _T("")};
const int CInstrumentEditorSeq::MAX_VOLUME = 0;
const int CInstrumentEditorSeq::MAX_DUTY = 0;
const inst_type_t CInstrumentEditorSeq::INST_TYPE = INST_NONE;
*/

IMPLEMENT_DYNAMIC(CInstrumentEditorSeq, CSequenceInstrumentEditPanel)
CInstrumentEditorSeq::CInstrumentEditorSeq(CWnd* pParent, LPCTSTR Title, const LPCTSTR *SeqName, int Vol, int Duty, inst_type_t Type) :
	CSequenceInstrumentEditPanel(CInstrumentEditorSeq::IDD, pParent),
	m_pTitle(Title),
	m_pSequenceName(SeqName),
	m_iMaxVolume(Vol),
	m_iMaxDuty(Duty),
	m_iInstType(Type)
{
}

void CInstrumentEditorSeq::SelectInstrument(std::shared_ptr<CInstrument> pInst)
{
	m_pInstrument = std::dynamic_pointer_cast<CSeqInstrument>(pInst);
	ASSERT(m_pInstrument && m_pInstrument->GetType() == m_iInstType);

	sequence_t Sel = m_iSelectedSetting;

	// Update instrument setting list
	if (CListCtrl *pList = static_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS))) {		// // //
		pList->SetRedraw(FALSE);
		CString str;
		foreachSeq([&] (sequence_t i) {
			pList->SetCheck(value_cast(i), m_pInstrument->GetSeqEnable(i));
			str.Format(_T("%i"), m_pInstrument->GetSeqIndex(i));
			pList->SetItemText(value_cast(i), 1, str);
		});
		pList->SetRedraw();
		pList->RedrawWindow();
	}

	// Setting text box
	SetDlgItemInt(IDC_SEQ_INDEX, m_pInstrument->GetSeqIndex(m_iSelectedSetting = Sel));

	SelectSequence(m_iSelectedSetting);

	SetFocus();
}

void CInstrumentEditorSeq::SelectSequence(sequence_t Type)
{
	// Selects the current sequence in the sequence editor
	m_pSequence = m_pInstrument->GetSequence(Type);		// // //
	if (CListCtrl *pList = static_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS)))
		pList->SetItemState(value_cast(Type), LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_pSequenceEditor->SelectSequence(m_pSequence, m_iInstType);
	SetupParser();		// // //
}

void CInstrumentEditorSeq::SetupParser() const		// // //
{
	const auto MakeParser = [this] (sequence_t typ, seq_setting_t setting) -> std::unique_ptr<CSeqConversionBase> {
		switch (typ) {
		case sequence_t::Volume:
			return std::make_unique<CSeqConversionDefault>(0, setting == SETTING_VOL_64_STEPS ? 0x3F : this->m_iMaxVolume);
		case sequence_t::Arpeggio:
			switch (setting) {
			case SETTING_ARP_SCHEME:		// // //
				return std::make_unique<CSeqConversionArpScheme>(ARPSCHEME_MIN);
			case SETTING_ARP_FIXED:
				return std::make_unique<CSeqConversionArpFixed>();
			default:
				return std::make_unique<CSeqConversionDefault>(-NOTE_COUNT, NOTE_COUNT);
			}
		case sequence_t::Pitch: case sequence_t::HiPitch:
			return std::make_unique<CSeqConversionDefault>(-128, 127);
		case sequence_t::DutyCycle:
			if (this->m_iInstType == INST_S5B)
				return std::make_unique<CSeqConversion5B>();
			return std::make_unique<CSeqConversionDefault>(0, this->m_iMaxDuty);
		}
		DEBUG_BREAK(); return nullptr;
	};

	auto pConv = MakeParser(m_pSequence->GetSequenceType(), m_pSequence->GetSetting());
	m_pSequenceEditor->SetConversion(*pConv);		// // //
	m_pParser->SetSequence(m_pSequence);
	m_pParser->SetConversion(std::move(pConv));
}

void CInstrumentEditorSeq::UpdateSequenceString(bool Changed)		// // //
{
	// Update sequence string
	SetupParser();		// // //
	SetDlgItemText(IDC_SEQUENCE_STRING, m_pParser->PrintSequence().c_str());		// // //
	// If the sequence was changed, assume the user wants to enable it
	if (Changed) {
		static_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS))->SetCheck(value_cast(m_iSelectedSetting), 1);
	}
}

BEGIN_MESSAGE_MAP(CInstrumentEditorSeq, CSequenceInstrumentEditPanel)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_INSTSETTINGS, OnLvnItemchangedInstsettings)
	ON_EN_CHANGE(IDC_SEQ_INDEX, OnEnChangeSeqIndex)
	ON_BN_CLICKED(IDC_FREE_SEQ, OnBnClickedFreeSeq)
	ON_COMMAND(ID_CLONE_SEQUENCE, OnCloneSequence)
END_MESSAGE_MAP()

// CInstrumentSettings message handlers

BOOL CInstrumentEditorSeq::OnInitDialog()
{
	CInstrumentEditPanel::OnInitDialog();

	SetupDialog(m_pSequenceName);
	m_pSequenceEditor->SetMaxValues(m_iMaxVolume, m_iMaxDuty);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentEditorSeq::OnLvnItemchangedInstsettings(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	auto mask = pNMLV->uNewState & pNMLV->uOldState;		// // // wine compatibility
	pNMLV->uNewState &= ~mask;
	pNMLV->uOldState &= ~mask;

	CListCtrl *pList = static_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS));
	if (pNMLV->uChanged & LVIF_STATE && m_pInstrument != NULL) {
		// Selected new setting
		if (pNMLV->uNewState & LVIS_SELECTED || pNMLV->uNewState & LCTRL_CHECKBOX_STATE) {
			m_iSelectedSetting = enum_cast<sequence_t>((unsigned)pNMLV->iItem);		// // //
			int Sequence = m_pInstrument->GetSeqIndex(m_iSelectedSetting);
			SetDlgItemInt(IDC_SEQ_INDEX, Sequence);
			SelectSequence(m_iSelectedSetting);
			pList->SetSelectionMark(value_cast(m_iSelectedSetting));
			pList->SetItemState(value_cast(m_iSelectedSetting), LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		}

		// Changed checkbox
		switch (pNMLV->uNewState & LCTRL_CHECKBOX_STATE) {
		case LCTRL_CHECKBOX_CHECKED:	// Checked/
			if (!m_pInstrument->GetSeqEnable(m_iSelectedSetting)) {		// // //
				m_pInstrument->SetSeqEnable(m_iSelectedSetting, true);
				GetDocument()->ModifyIrreversible();
			}
			break;
		case LCTRL_CHECKBOX_UNCHECKED:	// Unchecked
			if (m_pInstrument->GetSeqEnable(m_iSelectedSetting)) {		// // //
				m_pInstrument->SetSeqEnable(m_iSelectedSetting, false);
				GetDocument()->ModifyIrreversible();
			}
			break;
		}
	}

	*pResult = 0;
}

void CInstrumentEditorSeq::OnEnChangeSeqIndex()
{
	// Selected sequence changed
	CListCtrl *pList = static_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS));
	int Index = std::clamp((int)GetDlgItemInt(IDC_SEQ_INDEX), 0, MAX_SEQUENCES - 1);		// // //

	if (m_pInstrument != nullptr) {
		// Update list
		CString str;		// // //
		str.Format(_T("%i"), Index);
		pList->SetItemText(value_cast(m_iSelectedSetting), 1, str);
		if (m_pInstrument->GetSeqIndex(m_iSelectedSetting) != Index) {
			m_pInstrument->SetSeqIndex(m_iSelectedSetting, Index);
			GetDocument()->ModifyIrreversible();		// // //
		}
		SelectSequence(m_iSelectedSetting);
	}
}

void CInstrumentEditorSeq::OnBnClickedFreeSeq()
{
	int FreeIndex = m_pInstManager->AddSequence(m_iInstType, m_iSelectedSetting, nullptr, m_pInstrument.get());		// // //
	if (FreeIndex == -1)
		FreeIndex = 0;
	SetDlgItemInt(IDC_SEQ_INDEX, FreeIndex, FALSE);	// Things will update automatically by changing this
}

BOOL CInstrumentEditorSeq::DestroyWindow()
{
	m_pSequenceEditor->DestroyWindow();
	return CDialog::DestroyWindow();
}

void CInstrumentEditorSeq::OnKeyReturn()
{
	// Translate the sequence text string to a sequence
	CString Text;
	GetDlgItemText(IDC_SEQUENCE_STRING, Text);
	TranslateMML(Text);		// // //

	// Enable setting
	static_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS))->SetCheck(value_cast(m_iSelectedSetting), 1);
}

void CInstrumentEditorSeq::OnCloneSequence()
{
	auto pSeq = std::make_shared<CSequence>(*m_pSequence);		// // //
	int FreeIndex = m_pInstManager->AddSequence(m_iInstType, m_iSelectedSetting, std::move(pSeq), m_pInstrument.get());
	if (FreeIndex != -1)
		SetDlgItemInt(IDC_SEQ_INDEX, FreeIndex, FALSE);
}
