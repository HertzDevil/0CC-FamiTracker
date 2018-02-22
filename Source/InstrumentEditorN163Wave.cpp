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

#include "InstrumentEditorN163Wave.h"
#include "WaveEditor.h"		// // //
#include "FamiTrackerEnv.h"		// // //
#include "DPI.h"		// // //
#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"		// // //
#include "SeqInstrument.h"		// // //
#include "InstrumentN163.h"		// // //
#include "SoundGen.h"
#include "Clipboard.h"
#include "WavegenBuiltin.h" // test
#include "NumConv.h"		// // //
#include "sv_regex.h"		// // //
#include <array>		// // //
#include "str_conv/str_conv.hpp"		// // //

// CInstrumentEditorN163Wave dialog

IMPLEMENT_DYNAMIC(CInstrumentEditorN163Wave, CInstrumentEditPanel)

CInstrumentEditorN163Wave::CInstrumentEditorN163Wave(CWnd* pParent) : CInstrumentEditPanel(CInstrumentEditorN163Wave::IDD, pParent),
	m_pWaveEditor(std::make_unique<CWaveEditorN163>(10, 8, 32, 16)),
	m_iWaveIndex(0)
{
}

CInstrumentEditorN163Wave::~CInstrumentEditorN163Wave()
{
}

void CInstrumentEditorN163Wave::DoDataExchange(CDataExchange* pDX)
{
	CInstrumentEditPanel::DoDataExchange(pDX);
}

void CInstrumentEditorN163Wave::SelectInstrument(std::shared_ptr<CInstrument> pInst)
{
	m_pInstrument = std::dynamic_pointer_cast<CInstrumentN163>(pInst);
	ASSERT(m_pInstrument);

	CComboBox *pSizeBox = static_cast<CComboBox*>(GetDlgItem(IDC_WAVE_SIZE));
	CComboBox *pPosBox = static_cast<CComboBox*>(GetDlgItem(IDC_WAVE_POS));

	pSizeBox->SelectString(-1, conv::to_wide(conv::sv_from_int(m_pInstrument->GetWaveSize())).data());		// // //
	FillPosBox(m_pInstrument->GetWaveSize());
	pPosBox->SetWindowTextW(conv::to_wide(conv::sv_from_int(m_pInstrument->GetWavePos())).data());

	/*
	if (m_pInstrument->GetAutoWavePos()) {
		CheckDlgButton(IDC_POSITION, 1);
		GetDlgItem(IDC_WAVE_POS)->EnableWindow(FALSE);
	}
	else {
		CheckDlgButton(IDC_POSITION, 0);
		GetDlgItem(IDC_WAVE_POS)->EnableWindow(TRUE);
	}
	*/

	if (m_pWaveEditor) {
		m_pWaveEditor->SetInstrument(m_pInstrument);
		m_pWaveEditor->SetLength(m_pInstrument->GetWaveSize());
	}

	m_iWaveIndex = 0;

	PopulateWaveBox();
}

BEGIN_MESSAGE_MAP(CInstrumentEditorN163Wave, CInstrumentEditPanel)
	ON_COMMAND(IDC_PRESET_SINE, OnPresetSine)
	ON_COMMAND(IDC_PRESET_TRIANGLE, OnPresetTriangle)
	ON_COMMAND(IDC_PRESET_SAWTOOTH, OnPresetSawtooth)
	ON_COMMAND(IDC_PRESET_PULSE_50, OnPresetPulse50)
	ON_COMMAND(IDC_PRESET_PULSE_25, OnPresetPulse25)
	ON_MESSAGE(WM_USER_WAVE_CHANGED, OnWaveChanged)
	ON_BN_CLICKED(IDC_COPY, OnBnClickedCopy)
	ON_BN_CLICKED(IDC_PASTE, OnBnClickedPaste)
	ON_CBN_SELCHANGE(IDC_WAVE_SIZE, OnWaveSizeChange)
	ON_CBN_EDITCHANGE(IDC_WAVE_POS, OnWavePosChange)
	ON_CBN_SELCHANGE(IDC_WAVE_POS, OnWavePosSelChange)
//	ON_BN_CLICKED(IDC_POSITION, OnPositionClicked)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_N163_WAVES, OnLvnItemchangedN163Waves)		// // //
	ON_BN_CLICKED(IDC_N163_ADD, OnBnClickedN163Add)
	ON_BN_CLICKED(IDC_N163_DELETE, OnBnClickedN163Delete)
END_MESSAGE_MAP()

// CInstrumentEditorN163Wave message handlers

BOOL CInstrumentEditorN163Wave::OnInitDialog()
{
	CInstrumentEditPanel::OnInitDialog();

	// Create wave editor
	m_pWaveEditor->CreateEx(WS_EX_CLIENTEDGE, NULL, L"", WS_CHILD | WS_VISIBLE, DPI::Rect(20, 30, 0, 0), this);		// // //
	m_pWaveEditor->ShowWindow(SW_SHOW);
	m_pWaveEditor->UpdateWindow();

	CComboBox *pWaveSize = static_cast<CComboBox*>(GetDlgItem(IDC_WAVE_SIZE));

	for (int i = 4, im = WaveSizeAvailable(); i <= im; i += 4)
		pWaveSize->AddString(conv::to_wide(conv::sv_from_int(i)).data());

	int order[2] = {1, 0};		// // //
	CRect r;
	m_cWaveListCtrl.SubclassDlgItem(IDC_N163_WAVES, this);
	m_cWaveListCtrl.GetClientRect(&r);		// // // 050B
	m_cWaveListCtrl.InsertColumn(0, L"Wave", LVCFMT_LEFT, static_cast<int>(.85 * r.Width()));
	m_cWaveListCtrl.InsertColumn(1, L"#", LVCFMT_LEFT, static_cast<int>(.15 * r.Width()));
	m_cWaveListCtrl.SetColumnOrderArray(2, order);

	m_WaveImage.Create(CInstrumentN163::MAX_WAVE_SIZE, 16, ILC_COLOR8, 0, CInstrumentN163::MAX_WAVE_COUNT);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CInstrumentEditorN163Wave::PreTranslateMessage(MSG* pMsg)		// // //
{
	if (pMsg->message == WM_KEYDOWN) {
		if ((::GetKeyState(VK_CONTROL) & 0x80) == 0x80) {
			switch (pMsg->wParam) {
			case VK_LEFT:
				m_pWaveEditor->PhaseShift(1);
				UpdateWaveBox(m_iWaveIndex);
				return TRUE;
			case VK_RIGHT:
				m_pWaveEditor->PhaseShift(-1);
				UpdateWaveBox(m_iWaveIndex);
				return TRUE;
			case VK_DOWN:
				m_pWaveEditor->Invert(15);
				UpdateWaveBox(m_iWaveIndex);
				return TRUE;
			}
		}
	}

	return CInstrumentEditPanel::PreTranslateMessage(pMsg);
}

void CInstrumentEditorN163Wave::GenerateWaves(std::unique_ptr<CWaveformGenerator> pWaveGen)		// // // test
{
	int size = m_pInstrument->GetWaveSize();
	auto Buffer = std::make_unique<float[]>(size); // test
	pWaveGen->CreateWaves(Buffer.get(), size, pWaveGen->GetCount());
	for (int i = 0; i < size; ++i) {
		float Sample = std::clamp(Buffer[i] * 8.f + 8.f, 0.f, 15.f);
		m_pInstrument->SetSample(m_iWaveIndex, i, static_cast<int>(Sample));
	}

	GetDocument()->ModifyIrreversible();		// // //
	m_pWaveEditor->WaveChanged();
	Env.GetSoundGenerator()->WaveChanged();
}

// // // test

int CInstrumentEditorN163Wave::WaveSizeAvailable() const {		// // //
	return 256 - 16 * GetDocument()->GetModule()->GetNamcoChannels();
}

void CInstrumentEditorN163Wave::OnPresetSine()
{
	GenerateWaves(std::make_unique<CWavegenSine>());
}

void CInstrumentEditorN163Wave::OnPresetTriangle()
{
	GenerateWaves(std::make_unique<CWavegenTriangle>());
}

void CInstrumentEditorN163Wave::OnPresetPulse50()
{
	float PulseWidth = .5f;
	auto pWaveGen = std::make_unique<CWavegenPulse>();
	pWaveGen->GetParameter(0)->SetValue(&PulseWidth);
	GenerateWaves(std::move(pWaveGen));
}

void CInstrumentEditorN163Wave::OnPresetPulse25()
{
	float PulseWidth = .25f;
	auto pWaveGen = std::make_unique<CWavegenPulse>();
	pWaveGen->GetParameter(0)->SetValue(&PulseWidth);
	GenerateWaves(std::move(pWaveGen));
}

void CInstrumentEditorN163Wave::OnPresetSawtooth()
{
	GenerateWaves(std::make_unique<CWavegenSawtooth>());
}

void CInstrumentEditorN163Wave::OnBnClickedCopy()
{
	// Assemble a MML string
	CStringW Str;
	for (auto x : m_pInstrument->GetSamples(m_iWaveIndex))		// // //
		AppendFormatW(Str, L"%i ", x);

	if (CClipboard Clipboard(this, CF_UNICODETEXT); Clipboard.IsOpened()) {
		if (!Clipboard.SetString(Str))
			AfxMessageBox(IDS_CLIPBOARD_COPY_ERROR);
	}
	else
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
}

void CInstrumentEditorN163Wave::OnBnClickedPaste()
{
	// Copy from clipboard
	CClipboard Clipboard(this, CF_UNICODETEXT);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	if (Clipboard.IsDataAvailable()) {
		LPTSTR text = (LPTSTR)Clipboard.GetDataPointer();
		if (text != NULL)
			ParseString(conv::to_utf8(text));
	}
}

void CInstrumentEditorN163Wave::ParseString(std::string_view sv)
{
	int i = 0;
	int im = WaveSizeAvailable();
	for (auto x : re::tokens(sv)) {		// // //
		int value = CSequenceInstrumentEditPanel::ReadStringValue(x.str());
		m_pInstrument->SetSample(m_iWaveIndex, i, std::clamp(value, 0, 15));		// // //
		if (++i >= im)
			break;
	}

	int size = std::clamp(i & 0xFC, 4, im);
	m_pInstrument->SetWaveSize(size);

	static_cast<CComboBox*>(GetDlgItem(IDC_WAVE_SIZE))->SelectString(0, conv::to_wide(conv::sv_from_int(size)).data());

	FillPosBox(size);

	GetDocument()->ModifyIrreversible();		// // //
	m_pWaveEditor->SetLength(size);
	m_pWaveEditor->WaveChanged();
}

LRESULT CInstrumentEditorN163Wave::OnWaveChanged(WPARAM wParam, LPARAM lParam)
{
	CStringW str;
	for (auto x : m_pInstrument->GetSamples(m_iWaveIndex))		// // //
		AppendFormatW(str, L"%i ", x);

	SetDlgItemTextW(IDC_MML, str);
	UpdateWaveBox(m_iWaveIndex);		// // //
	return 0;
}

void CInstrumentEditorN163Wave::OnWaveSizeChange()
{
	BOOL trans;
	int size = GetDlgItemInt(IDC_WAVE_SIZE, &trans, FALSE);
	size = std::clamp(size & 0xFC, 4, WaveSizeAvailable());		// // //

	if (m_pInstrument->GetWaveSize() != size) {
		m_pInstrument->SetWaveSize(size);
		GetDocument()->ModifyIrreversible();
	}

	FillPosBox(size);

	m_pWaveEditor->SetLength(size);
	m_pWaveEditor->WaveChanged();
	PopulateWaveBox();		// // //
}

void CInstrumentEditorN163Wave::OnWavePosChange()
{
	BOOL trans;
	int pos = GetDlgItemInt(IDC_WAVE_POS, &trans, FALSE);

	m_pInstrument->SetWavePos(std::clamp(pos, 0, 255));		// // //
	GetDocument()->ModifyIrreversible();
}

void CInstrumentEditorN163Wave::OnWavePosSelChange()
{
	CStringW str;
	CComboBox *pPosBox = static_cast<CComboBox*>(GetDlgItem(IDC_WAVE_POS));
	pPosBox->GetLBText(pPosBox->GetCurSel(), str);

	int pos = _ttoi(str);

	m_pInstrument->SetWavePos(std::clamp(pos, 0, 255));		// // //
	GetDocument()->ModifyIrreversible();
}

void CInstrumentEditorN163Wave::FillPosBox(int size)
{
	CComboBox *pPosBox = static_cast<CComboBox*>(GetDlgItem(IDC_WAVE_POS));
	pPosBox->ResetContent();

	for (int i = 0; i <= WaveSizeAvailable() - size; i += size)		// // // prevent reading non-wave n163 registers
		pPosBox->AddString(conv::to_wide(conv::sv_from_int(i)).data());
}

void CInstrumentEditorN163Wave::PopulateWaveBox()		// // //
{
	int Width = m_pInstrument->GetWaveSize();

	CBitmap Waveforms;
	Waveforms.CreateBitmap(Width, 16, 1, 1, NULL);
	m_WaveImage.DeleteImageList();
	m_WaveImage.Create(Width, 16, ILC_COLOR8, 0, CInstrumentN163::MAX_WAVE_COUNT);
	for (int i = 0; i < CInstrumentN163::MAX_WAVE_COUNT; i++)
		m_WaveImage.Add(&Waveforms, &Waveforms);
	m_cWaveListCtrl.SetImageList(&m_WaveImage, LVSIL_SMALL);

	m_cWaveListCtrl.DeleteAllItems();
	for (int i = 0, Count = m_pInstrument->GetWaveCount(); i < Count; ++i) {
		UpdateWaveBox(i);
		m_cWaveListCtrl.InsertItem(i, L"", i);
		m_cWaveListCtrl.SetItemText(i, 1, conv::to_wide(conv::from_uint_hex(i)).data());
	}
	m_cWaveListCtrl.RedrawWindow();
	SelectWave(m_iWaveIndex);
}

void CInstrumentEditorN163Wave::UpdateWaveBox(int Index)		// // //
{
	CBitmap Waveform;
	std::array<unsigned char, CInstrumentN163::MAX_WAVE_SIZE * 2> WaveBits = { };

	WaveBits.fill(0xFFu);
	int Width = m_pInstrument->GetWaveSize();
	if (Width % 16) Width += 16 - Width % 16;
	for (int j = 0; j < Width; j++) {
		int b = Width * (15 - m_pInstrument->GetSample(Index, j)) + j;
		WaveBits[b / 8] &= ~static_cast<char>(1 << (7 - b % 8));
	}

	Waveform.CreateBitmap(m_pInstrument->GetWaveSize(), 16, 1, 1, WaveBits.data());
	m_WaveImage.Replace(Index, &Waveform, &Waveform);
	m_cWaveListCtrl.SetImageList(&m_WaveImage, LVSIL_SMALL);
	m_cWaveListCtrl.RedrawItems(Index, Index);
}

/*
void CInstrumentEditorN163Wave::OnPositionClicked()
{
	if (IsDlgButtonChecked(IDC_POSITION)) {
		GetDlgItem(IDC_WAVE_POS)->EnableWindow(FALSE);
		m_pInstrument->SetAutoWavePos(true);
	}
	else {
		GetDlgItem(IDC_WAVE_POS)->EnableWindow(TRUE);
		m_pInstrument->SetAutoWavePos(false);
	}
}
*/

void CInstrumentEditorN163Wave::OnKeyReturn()
{
	// Parse MML string
	CStringW text;
	GetDlgItemTextW(IDC_MML, text);
	ParseString(conv::to_utf8(text));
}

void CInstrumentEditorN163Wave::OnLvnItemchangedN163Waves(NMHDR *pNMHDR, LRESULT *pResult)		// // //
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (pNMLV->iItem != -1)
		SelectWave(pNMLV->iItem);
	*pResult = 0;
}

void CInstrumentEditorN163Wave::SelectWave(int Index)		// // //
{
	m_iWaveIndex = Index;
	if (m_pWaveEditor != NULL) {
		m_pWaveEditor->SetWave(m_iWaveIndex);
		m_pWaveEditor->WaveChanged();
	}
}

void CInstrumentEditorN163Wave::OnBnClickedN163Add()		// // //
{
	if (m_pInstrument->InsertNewWave(m_iWaveIndex + 1)) {
		GetDocument()->ModifyIrreversible();
		PopulateWaveBox();
		m_cWaveListCtrl.SetItemState(++m_iWaveIndex, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
}

void CInstrumentEditorN163Wave::OnBnClickedN163Delete()		// // //
{
	if (m_pInstrument->RemoveWave(m_iWaveIndex)) {
		GetDocument()->ModifyIrreversible();
		PopulateWaveBox();
		if (m_iWaveIndex == m_pInstrument->GetWaveCount())
			m_iWaveIndex--;
		m_cWaveListCtrl.SetItemState(m_iWaveIndex, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
}
