/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
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

#include "DetuneDlg.h"
#include <algorithm>
#include <cmath>
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"
#include "DetuneTable.h"
#include "str_conv/str_conv.hpp"
#include "NumConv.h"
#include "sv_regex.h"

// CDetuneDlg dialog

IMPLEMENT_DYNAMIC(CDetuneDlg, CDialog)

CDetuneDlg::CDetuneDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDetuneDlg::IDD, pParent)
{

}

CDetuneDlg::~CDetuneDlg()
{
}

void CDetuneDlg::AssignModule(CFamiTrackerModule &modfile) {
	modfile_ = &modfile;
}

void CDetuneDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDetuneDlg, CDialog)
	ON_WM_HSCROLL()
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_OCTAVE, OnDeltaposSpinOctave)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_NOTE, OnDeltaposSpinNote)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_OFFSET, OnDeltaposSpinOffset)
	ON_EN_KILLFOCUS(IDC_EDIT_OCTAVE, OnEnKillfocusEditOctave)
	ON_EN_KILLFOCUS(IDC_EDIT_NOTE, OnEnKillfocusEditNote)
	ON_EN_KILLFOCUS(IDC_EDIT_OFFSET, OnEnKillfocusEditOffset)
	ON_BN_CLICKED(IDC_RADIO_NTSC, OnBnClickedRadioNTSC)
	ON_BN_CLICKED(IDC_RADIO_PAL, OnBnClickedRadioPAL)
	ON_BN_CLICKED(IDC_RADIO_VRC6, OnBnClickedRadioVRC6)
	ON_BN_CLICKED(IDC_RADIO_VRC7, OnBnClickedRadioVRC7)
	ON_BN_CLICKED(IDC_RADIO_FDS, OnBnClickedRadioFDS)
	ON_BN_CLICKED(IDC_RADIO_N163, OnBnClickedRadioN163)
	ON_BN_CLICKED(IDC_BUTTON_RESET, OnBnClickedButtonReset)
	ON_BN_CLICKED(IDC_BUTTON_IMPORT, OnBnClickedButtonImport)
	ON_BN_CLICKED(IDC_BUTTON_EXPORT, OnBnClickedButtonExport)
END_MESSAGE_MAP()


// CDetuneDlg message handlers

const LPCWSTR CDetuneDlg::m_pNote[12] = {
	L"C" , L"C#", L"D" , L"D#", L"E" , L"F",
	L"F#", L"G" , L"G#", L"A" , L"A#", L"B",
};
const LPCWSTR CDetuneDlg::m_pNoteFlat[12] = {
	L"C" , L"D-", L"D" , L"E-", L"E" , L"F",
	L"G-", L"G" , L"A-", L"A" , L"B-", L"B",
};
const LPCWSTR CDetuneDlg::CHIP_STR[6] = {L"NTSC", L"PAL", L"Saw", L"VRC7", L"FDS", L"N163"};

BOOL CDetuneDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_iOctave = 3;
	m_iNote = 36;
	m_iCurrentChip = 0;
	for (int i = 0; i < 6; ++i) for (int j = 0; j < NOTE_COUNT; ++j)
		m_iDetuneTable[i][j] = modfile_->GetDetuneOffset(i, j);
	m_iGlobalSemitone = modfile_->GetTuningSemitone();		// // // 050B
	m_iGlobalCent = modfile_->GetTuningCent();

	m_cSliderOctave.SubclassDlgItem(IDC_SLIDER_OCTAVE, this);
	m_cSliderNote.SubclassDlgItem(IDC_SLIDER_NOTE, this);
	m_cSliderOffset.SubclassDlgItem(IDC_SLIDER_OFFSET, this);
	m_cSliderDetuneSemitone.SubclassDlgItem(IDC_SLIDER_DETUNE_SEMITONE, this);
	m_cSliderDetuneCent.SubclassDlgItem(IDC_SLIDER_DETUNE_CENT, this);

	m_cEditOctave.SubclassDlgItem(IDC_EDIT_OCTAVE, this);
	m_cEditNote.SubclassDlgItem(IDC_EDIT_NOTE, this);
	m_cEditOffset.SubclassDlgItem(IDC_EDIT_OFFSET, this);

	auto SpinOctave = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_OCTAVE);
	m_cSliderOctave.SetRange(0, OCTAVE_RANGE - 1);
	SpinOctave->SetRange(0, OCTAVE_RANGE - 1);
	m_cSliderOctave.SetPos(m_iOctave);
	SpinOctave->SetPos(m_iOctave);
	m_cSliderOctave.SetTicFreq(1);

	auto SpinNote = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_NOTE);
	m_cSliderNote.SetRange(0, NOTE_RANGE - 1);
	SpinNote->SetRange(0, NOTE_RANGE - 1);
	m_cSliderNote.SetPos(m_iNote % NOTE_RANGE);
	SpinNote->SetPos(m_iNote % NOTE_RANGE);
	m_cSliderNote.SetTicFreq(1);

	auto SpinOffset = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_OFFSET);
	m_cSliderOffset.SetRange(-16, 16);
	SpinOffset->SetRange(-16, 16);
	m_cSliderOffset.SetPos(m_iDetuneTable[m_iCurrentChip][m_iNote]);
	SpinOffset->SetPos(m_iDetuneTable[m_iCurrentChip][m_iNote]);
	m_cSliderOffset.SetTicFreq(4);

	m_cEditNote.SetWindowTextW(m_pNote[m_iNote % NOTE_RANGE]);

	UDACCEL Acc[1];
	Acc[0].nSec = 0;
	Acc[0].nInc = 1;
	SpinNote->SetAccel(1, Acc);

	CheckRadioButton(IDC_RADIO_NTSC, IDC_RADIO_N163, IDC_RADIO_NTSC);

	m_cSliderDetuneSemitone.SetRange(-NOTE_RANGE, NOTE_RANGE);
	m_cSliderDetuneSemitone.SetTicFreq(1);
	m_cSliderDetuneSemitone.SetPos(m_iGlobalSemitone);
	m_cSliderDetuneCent.SetRange(-100, 100);
	m_cSliderDetuneCent.SetTicFreq(5);
	m_cSliderDetuneCent.SetPos(m_iGlobalCent);

	UpdateOffset();

	return TRUE;  // return TRUE unless you set the focus to a control
}

const int *CDetuneDlg::GetDetuneTable() const
{
	return *m_iDetuneTable;
}

int CDetuneDlg::GetDetuneSemitone() const
{
	return m_iGlobalSemitone;
}

int CDetuneDlg::GetDetuneCent() const
{
	return m_iGlobalCent;
}

unsigned int CDetuneDlg::FreqToReg(double Freq, int Chip, int Octave)
{
	double dReg;
	switch (Chip) {
	default:
	case 0: dReg = MASTER_CLOCK_NTSC / Freq / 16.0 - 1.0; break;
	case 1: dReg = MASTER_CLOCK_PAL  / Freq / 16.0 - 1.0; break;
	case 2: dReg = MASTER_CLOCK_NTSC / Freq / 14.0 - 1.0; break;
	case 3: dReg = Freq / 49716.0 * (1 << (18 - Octave)); break;
	case 4: dReg = Freq / MASTER_CLOCK_NTSC * (1 << 20); break;
	case 5: dReg = Freq / MASTER_CLOCK_NTSC * 15.0 * (1 << 18) * modfile_->GetNamcoChannels(); break;
	}
	return (unsigned int)(dReg + .5);
}

double CDetuneDlg::RegToFreq(unsigned int Reg, int Chip, int Octave)
{
	switch (Chip) {
	default:
	case 0: return MASTER_CLOCK_NTSC / 16.0 / (Reg + 1.0); break;
	case 1: return MASTER_CLOCK_PAL  / 16.0 / (Reg + 1.0); break;
	case 2: return MASTER_CLOCK_NTSC / 14.0 / (Reg + 1.0); break;
	case 3: return 49716.0 * Reg / (1 << (18 - Octave)); break;
	case 4: return MASTER_CLOCK_NTSC * Reg / (1 << 20); break;
	case 5: return MASTER_CLOCK_NTSC * Reg / 15 / (1 << 18) / modfile_->GetNamcoChannels(); break;
	}
}

double CDetuneDlg::NoteToFreq(double Note)
{
	return 440. * pow(2., (Note - 45.) / 12.);
}

void CDetuneDlg::UpdateOctave()
{
	m_iOctave = std::clamp(m_iOctave, 0, OCTAVE_RANGE - 1);
	m_cSliderOctave.SetPos(m_iOctave);
	m_cEditOctave.SetWindowTextW(FormattedW(L"%i", m_iOctave));
	m_iNote = m_iOctave * NOTE_RANGE + m_iNote % NOTE_RANGE;
	UpdateOffset();
}

void CDetuneDlg::UpdateNote()
{
	m_iNote = std::clamp(m_iNote, 0, NOTE_COUNT - 1);
	m_cSliderNote.SetPos(m_iNote % NOTE_RANGE);
	m_cEditNote.SetWindowTextW(m_pNote[m_iNote % NOTE_RANGE]);
	m_iOctave = m_iNote / NOTE_RANGE;
	UpdateOctave();
}

void CDetuneDlg::UpdateOffset()
{
	m_cSliderOffset.SetPos(m_iDetuneTable[m_iCurrentChip][m_iNote]);
	m_cEditOffset.SetWindowTextW(FormattedW(L"%i", m_iDetuneTable[m_iCurrentChip][m_iNote]));

	if (m_iCurrentChip == 3) // VRC7
		for (int i = 0; i < OCTAVE_RANGE; ++i)
			m_iDetuneTable[3][i * NOTE_RANGE + m_iNote % NOTE_RANGE] = m_iDetuneTable[3][m_iNote];

	const auto DoubleFunc = [] (double x) {
		if (std::abs(x) >= 9999.5)
			return L"\n%.0f";
		if (std::abs(x) >= 99.995)
			return L"\n%.4g";
		return L"\n%.2f";
	};

	for (int i = 0; i < 6; ++i) {
		CStringW fmt = L"%s\n%X\n%X";
		if (i == 5 && !modfile_->GetNamcoChannels()) {
			SetDlgItemTextW(IDC_DETUNE_INFO_N163, FormattedW(L"%s\n-\n-\n-\n-\n-\n-", CHIP_STR[i]));
			continue;
		}
		double Note = m_iGlobalSemitone + .01 * m_iGlobalCent + m_iNote;
		int oldReg = FreqToReg(NoteToFreq(Note), i, m_iNote / NOTE_RANGE);
		int newReg = std::max(0, (int)oldReg + m_iDetuneTable[i][m_iNote] * (i >= 3 ? 1 : -1));
		double newFreq = RegToFreq(newReg, i, m_iNote / NOTE_RANGE);
		double values[4] = {
			RegToFreq(oldReg, i, m_iNote / NOTE_RANGE) * (i == 4 ? .25 : 1),
			newFreq * (i == 4 ? .25 : 1),
			NoteToFreq(Note) * (i == 4 ? .25 : 1),
			1200.0 * log(newFreq / NoteToFreq(Note)) / log(2.0),
		};
		for (const auto x : values)
			fmt += DoubleFunc(x);

		SetDlgItemTextW(IDC_DETUNE_INFO_NTSC + i, FormattedW(fmt, CHIP_STR[i], oldReg, newReg, values[0], values[1], values[2], values[3]));
	}

	SetDlgItemTextW(IDC_STATIC_DETUNE_SEMITONE, FormattedW(L"Semitone: %+d", m_iGlobalSemitone));
	SetDlgItemTextW(IDC_STATIC_DETUNE_CENT, FormattedW(L"Cent: %+d", m_iGlobalCent));
}

void CDetuneDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);

	auto pWnd = static_cast<CWnd *>(pScrollBar);

	if (pWnd == &m_cSliderOctave) {
		m_iOctave = m_cSliderOctave.GetPos();
		UpdateOctave();
	}
	else if (pWnd == &m_cSliderNote) {
		m_iNote = m_iOctave * NOTE_RANGE + m_cSliderNote.GetPos();
		UpdateNote();
	}
	else if (pWnd == &m_cSliderOffset) {
		m_iDetuneTable[m_iCurrentChip][m_iNote] = m_cSliderOffset.GetPos();
		UpdateOffset();
	}
	else if (pWnd == &m_cSliderDetuneSemitone) {
		m_iGlobalSemitone = m_cSliderDetuneSemitone.GetPos();
		UpdateOffset();
	}
	else if (pWnd == &m_cSliderDetuneCent) {
		m_iGlobalCent = m_cSliderDetuneCent.GetPos();
		UpdateOffset();
	}
}

void CDetuneDlg::OnDeltaposSpinOctave(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	m_iOctave += pNMUpDown->iDelta;
	UpdateOctave();

	*pResult = 0;
}

void CDetuneDlg::OnDeltaposSpinNote(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	m_iNote += pNMUpDown->iDelta;
	UpdateNote();

	*pResult = 0;
}

void CDetuneDlg::OnDeltaposSpinOffset(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	m_iDetuneTable[m_iCurrentChip][m_iNote] += pNMUpDown->iDelta;
	UpdateOffset();

	*pResult = 0;
}

void CDetuneDlg::OnEnKillfocusEditOctave()
{
	CStringW String;
	m_cEditOctave.GetWindowTextW(String);
	m_iOctave = conv::to_int(String).value_or(0);
	UpdateOctave();
}

void CDetuneDlg::OnEnKillfocusEditNote()
{
	CStringW String;
	m_cEditNote.GetWindowTextW(String);
	for (int i = 0; i < NOTE_RANGE; ++i)
		if (String == m_pNote[i] || String == m_pNoteFlat[i])
			m_iNote = m_iOctave * NOTE_RANGE + i;
	UpdateNote();
}

void CDetuneDlg::OnEnKillfocusEditOffset()
{
	CStringW String;
	m_cEditOffset.GetWindowTextW(String);
	m_iDetuneTable[m_iCurrentChip][m_iNote] = conv::to_int(String).value_or(0);
	UpdateOctave();
}

void CDetuneDlg::OnBnClickedRadioNTSC()
{
	m_iCurrentChip = 0;
	UpdateOffset();
}

void CDetuneDlg::OnBnClickedRadioPAL()
{
	m_iCurrentChip = 1;
	UpdateOffset();
}

void CDetuneDlg::OnBnClickedRadioVRC6()
{
	m_iCurrentChip = 2;
	UpdateOffset();
}

void CDetuneDlg::OnBnClickedRadioVRC7()
{
	m_iCurrentChip = 3;
	UpdateOffset();
}

void CDetuneDlg::OnBnClickedRadioFDS()
{
	m_iCurrentChip = 4;
	UpdateOffset();
}

void CDetuneDlg::OnBnClickedRadioN163()
{
	m_iCurrentChip = 5;
	UpdateOffset();
}

void CDetuneDlg::OnBnClickedButtonReset()
{
	for (auto &x : m_iDetuneTable[m_iCurrentChip])
		x = 0;
	UpdateOffset();
}


void CDetuneDlg::OnBnClickedButtonImport()
{
	CStringW    Path;
	CStdioFile csv;

	CFileDialog FileDialog(TRUE, L"csv", 0,
		OFN_HIDEREADONLY, L"Comma-separated values (*.csv)|*.csv|All files|*.*||");

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	Path = FileDialog.GetPathName();

	if (!csv.Open(Path, CFile::modeRead)) {
		AfxMessageBox(IDS_FILE_OPEN_ERROR);
		return;
	}

	static const std::regex TOKEN {R"([^,]+)", std::regex_constants::optimize};
	CStringW LineW;
	while (csv.ReadString(LineW)) {
		auto Line = conv::to_utf8(LineW);
		int Chip = -1;
		int Note = 0;
		for (auto m : re::string_gmatch(std::string_view {Line}, TOKEN)) {
			if (auto offs = conv::to_int(re::sv_from_submatch(m[0])))
				if (Chip == -1) {
					Chip = *offs;
					if (Chip < 0 || Chip >= 6) {
						AfxMessageBox(L"Invalid chip index.");
						csv.Close();
						return;
					}
				}
				else
					m_iDetuneTable[Chip][Note++] = *offs;
		}
	}

	csv.Close();
	UpdateOffset();
}


void CDetuneDlg::OnBnClickedButtonExport()
{
	CFileDialog SaveFileDialog(FALSE, L"csv", (LPCWSTR)CFamiTrackerDoc::GetDoc()->GetFileTitle(),
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, L"Comma-separated values (*.csv)|*.csv|All files|*.*||");

	if (SaveFileDialog.DoModal() == IDCANCEL)
		return;

	CStringW Path = SaveFileDialog.GetPathName();

	CStdioFile csv;
	if (!csv.Open(Path, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox(IDS_FILE_OPEN_ERROR);
		return;
	}

	for (int i = 0; i < 6; ++i) {
		CStringW Line = FormattedW(L"%i", i);
		for (int j = 0; j < NOTE_COUNT; ++j)
			AppendFormatW(Line, L",%i", m_iDetuneTable[i][j]);
		Line += L"\n";
		csv.WriteString(Line);
	}

	csv.Close();
	UpdateOffset();
}
