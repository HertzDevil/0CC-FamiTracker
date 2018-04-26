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

#include "TransposeDlg.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"
#include "SongData.h"
#include "FamiTrackerViewMessage.h"
#include "PatternNote.h"
#include "Instrument.h"
#include "InstrumentManager.h"
#include "MainFrm.h"
#include "DPI.h"
#include "APU/Types.h"
#include <algorithm>

// CTransposeDlg dialog

bool CTransposeDlg::s_bDisableInst[MAX_INSTRUMENTS] = {false};
const UINT CTransposeDlg::BUTTON_ID = 0xDD00; // large enough

IMPLEMENT_DYNAMIC(CTransposeDlg, CDialog)

CTransposeDlg::CTransposeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_TRANSPOSE, pParent), m_iTrack(0)
{

}

void CTransposeDlg::SetTrack(unsigned int Track)
{
	m_iTrack = Track;
}

void CTransposeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

void CTransposeDlg::Transpose(int Trsp, CSongData &song) {
	song.VisitPatterns([&] (CPatternData &pat, stChannelID c, unsigned) {
		if (IsAPUNoise(c) || IsDPCM(c))
			return;
		pat.VisitRows([&] (stChanNote &note) {
			if (note.Instrument == MAX_INSTRUMENTS || note.Instrument == HOLD_INSTRUMENT)
				return;
			if (ft0cc::doc::is_note(note.Note) && !s_bDisableInst[note.Instrument]) {
				int MIDI = std::clamp(ft0cc::doc::midi_note(note.Octave, note.Note) + Trsp, 0, NOTE_COUNT - 1);
				note.Octave = ft0cc::doc::oct_from_midi(MIDI);
				note.Note = ft0cc::doc::pitch_from_midi(MIDI);
			}
		});
	});
}

BEGIN_MESSAGE_MAP(CTransposeDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CTransposeDlg::OnBnClickedOk)
	ON_CONTROL_RANGE(BN_CLICKED, BUTTON_ID, BUTTON_ID + MAX_INSTRUMENTS - 1, OnBnClickedInst)
	ON_BN_CLICKED(IDC_BUTTON_TRSP_REVERSE, &CTransposeDlg::OnBnClickedButtonTrspReverse)
	ON_BN_CLICKED(IDC_BUTTON_TRSP_CLEAR, &CTransposeDlg::OnBnClickedButtonTrspClear)
END_MESSAGE_MAP()


// CTransposeDlg message handlers

BOOL CTransposeDlg::OnInitDialog()
{
	m_cFont.CreateFontW(-DPI::SY(10), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"Verdana");

	m_pDocument = CFamiTrackerDoc::GetDoc();
	CRect r;
	GetClientRect(&r);

	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		CStringW Name = FormattedW(L"%02X", i);
		int x = DPI::SX(20) + i % 8 * ((r.Width() - DPI::SX(30)) / 8);
		int y = DPI::SY(104) + i / 8 * DPI::SY(20);
		m_cInstButton[i].Create(Name, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
								CRect(x, y, x + DPI::SX(30), y + DPI::SY(18)), this, BUTTON_ID + i);
		m_cInstButton[i].SetCheck(s_bDisableInst[i] ? BST_CHECKED : BST_UNCHECKED);
		m_cInstButton[i].SetFont(&m_cFont);
		m_cInstButton[i].EnableWindow(m_pDocument->GetModule()->GetInstrumentManager()->IsInstrumentUsed(i));
	}

	CSpinButtonCtrl *pSpin = static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_SPIN_TRSP_SEMITONE));
	pSpin->SetRange(0, 96);
	pSpin->SetPos(0);

	CheckRadioButton(IDC_RADIO_SEMITONE_INC, IDC_RADIO_SEMITONE_DEC, IDC_RADIO_SEMITONE_INC);

	return CDialog::OnInitDialog();
}

void CTransposeDlg::OnBnClickedInst(UINT nID)
{
	s_bDisableInst[nID - BUTTON_ID] = !s_bDisableInst[nID - BUTTON_ID];
}

void CTransposeDlg::OnBnClickedOk()
{
	int Trsp = GetDlgItemInt(IDC_EDIT_TRSP_SEMITONE);
	bool All = IsDlgButtonChecked(IDC_CHECK_TRSP_ALL) != 0;
	if (GetCheckedRadioButton(IDC_RADIO_SEMITONE_INC, IDC_RADIO_SEMITONE_DEC) == IDC_RADIO_SEMITONE_DEC)
		Trsp = -Trsp;

	if (Trsp) {
		auto *pModule = m_pDocument->GetModule();
		if (All)
			pModule->VisitSongs([Trsp] (CSongData &song) {
				Transpose(Trsp, song);
			});
		else
			Transpose(Trsp, *pModule->GetSong(m_iTrack));

		m_pDocument->UpdateAllViews(NULL, UPDATE_PATTERN);
		m_pDocument->ModifyIrreversible();
		((CMainFrame *)AfxGetMainWnd())->ResetUndo();
	}

	CDialog::OnOK();
}

void CTransposeDlg::OnBnClickedButtonTrspReverse()
{
	for (int i = 0; i < MAX_INSTRUMENTS; ++i)
		CheckDlgButton(BUTTON_ID + i, IsDlgButtonChecked(BUTTON_ID + i) == BST_UNCHECKED);
}

void CTransposeDlg::OnBnClickedButtonTrspClear()
{
	for (int i = 0; i < MAX_INSTRUMENTS; ++i)
		CheckDlgButton(BUTTON_ID + i, BST_UNCHECKED);
}
