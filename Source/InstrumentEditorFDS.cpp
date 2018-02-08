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

#include "InstrumentEditorFDS.h"
#include "FamiTrackerEnv.h"		// // //
#include "Instrument.h"		// // //
#include "SeqInstrument.h"		// // //
#include "InstrumentFDS.h"		// // //
#include "DPI.h"		// // //
#include "APU/Types.h"		// // //
#include "SoundGen.h"
#include "Clipboard.h"
#include "WaveEditor.h"		// // //
#include "ModSequenceEditor.h"
#include <algorithm>		// // //
#include "sv_regex.h"		// // //
#include "FamiTrackerDoc.h"		// // //

// CInstrumentEditorFDS dialog

IMPLEMENT_DYNAMIC(CInstrumentEditorFDS, CInstrumentEditPanel)

CInstrumentEditorFDS::CInstrumentEditorFDS(CWnd* pParent) : CInstrumentEditPanel(CInstrumentEditorFDS::IDD, pParent),
	m_pWaveEditor(std::make_unique<CWaveEditorFDS>(5, 2, 64, 64)),
	m_pModSequenceEditor(std::make_unique<CModSequenceEditor>())
{
}

CInstrumentEditorFDS::~CInstrumentEditorFDS()
{
}

void CInstrumentEditorFDS::DoDataExchange(CDataExchange* pDX)
{
	CInstrumentEditPanel::DoDataExchange(pDX);
}

void CInstrumentEditorFDS::SelectInstrument(std::shared_ptr<CInstrument> pInst)
{
	m_pInstrument = std::dynamic_pointer_cast<CInstrumentFDS>(pInst);
	ASSERT(m_pInstrument);

	if (m_pWaveEditor)
		m_pWaveEditor->SetInstrument(m_pInstrument);

	if (m_pModSequenceEditor)
		m_pModSequenceEditor->SetInstrument(m_pInstrument);

	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_RATE_SPIN))->SetPos(m_pInstrument->GetModulationSpeed());
	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_DEPTH_SPIN))->SetPos(m_pInstrument->GetModulationDepth());
	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_DELAY_SPIN))->SetPos(m_pInstrument->GetModulationDelay());

//	CheckDlgButton(IDC_ENABLE_FM, m_pInstrument->GetModulationEnable() ? 1 : 0);

	EnableModControls(m_pInstrument->GetModulationEnable());
}


BEGIN_MESSAGE_MAP(CInstrumentEditorFDS, CInstrumentEditPanel)
	ON_COMMAND(IDC_PRESET_SINE, OnPresetSine)
	ON_COMMAND(IDC_PRESET_TRIANGLE, OnPresetTriangle)
	ON_COMMAND(IDC_PRESET_SAWTOOTH, OnPresetSawtooth)
	ON_COMMAND(IDC_PRESET_PULSE_50, OnPresetPulse50)
	ON_COMMAND(IDC_PRESET_PULSE_25, OnPresetPulse25)
	ON_COMMAND(IDC_MOD_PRESET_FLAT, OnModPresetFlat)
	ON_COMMAND(IDC_MOD_PRESET_SINE, OnModPresetSine)
	ON_WM_VSCROLL()
	ON_EN_CHANGE(IDC_MOD_RATE, OnModRateChange)
	ON_EN_CHANGE(IDC_MOD_DEPTH, OnModDepthChange)
	ON_EN_CHANGE(IDC_MOD_DELAY, OnModDelayChange)
	ON_BN_CLICKED(IDC_COPY_WAVE, &CInstrumentEditorFDS::OnBnClickedCopyWave)
	ON_BN_CLICKED(IDC_PASTE_WAVE, &CInstrumentEditorFDS::OnBnClickedPasteWave)
	ON_BN_CLICKED(IDC_COPY_TABLE, &CInstrumentEditorFDS::OnBnClickedCopyTable)
	ON_BN_CLICKED(IDC_PASTE_TABLE, &CInstrumentEditorFDS::OnBnClickedPasteTable)
//	ON_BN_CLICKED(IDC_ENABLE_FM, &CInstrumentEditorFDS::OnBnClickedEnableFm)
	ON_MESSAGE(WM_USER + 1, OnModChanged)
END_MESSAGE_MAP()

// CInstrumentEditorFDS message handlers

BOOL CInstrumentEditorFDS::OnInitDialog()
{
	CInstrumentEditPanel::OnInitDialog();

	// Create wave editor
	m_pWaveEditor->CreateEx(WS_EX_CLIENTEDGE, NULL, _T(""), WS_CHILD | WS_VISIBLE, DPI::Rect(20, 30, 0, 0), this);		// // //
	m_pWaveEditor->ShowWindow(SW_SHOW);
	m_pWaveEditor->UpdateWindow();

	// Create modulation sequence editor
	m_pModSequenceEditor->CreateEx(WS_EX_CLIENTEDGE, NULL, _T(""), WS_CHILD | WS_VISIBLE, DPI::Rect(10, 200, 0, 0), this);		// // //
	m_pModSequenceEditor->ShowWindow(SW_SHOW);
	m_pModSequenceEditor->UpdateWindow();

	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_RATE_SPIN))->SetRange(0, 4095);
	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_DEPTH_SPIN))->SetRange(0, 63);
	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_DELAY_SPIN))->SetRange(0, 255);
/*
	CSliderCtrl *pModSlider;
	pModSlider = (CSliderCtrl*)GetDlgItem(IDC_MOD_FREQ);
	pModSlider->SetRange(0, 0xFFF);
*/
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CInstrumentEditorFDS::PreTranslateMessage(MSG* pMsg)		// // //
{
	if (pMsg->message == WM_KEYDOWN) {
		if ((::GetKeyState(VK_CONTROL) & 0x80) == 0x80) {
			switch (pMsg->wParam) {
			case VK_LEFT:
				m_pWaveEditor->PhaseShift(1);
				return TRUE;
			case VK_RIGHT:
				m_pWaveEditor->PhaseShift(-1);
				return TRUE;
			case VK_DOWN:
				m_pWaveEditor->Invert(63);
				return TRUE;
			}
		}
	}

	return CInstrumentEditPanel::PreTranslateMessage(pMsg);
}

void CInstrumentEditorFDS::OnPresetSine()
{
	for (int i = 0; i < 64; ++i) {
		float angle = (float(i) * 3.141592f * 2.0f) / 64.0f + 0.049087375f;
		int sample = int((sinf(angle) + 1.0f) * 31.5f + 0.5f);
		m_pInstrument->SetSample(i, sample);
	}

	GetDocument()->ModifyIrreversible();		// // //
	m_pWaveEditor->RedrawWindow();
	Env.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnPresetTriangle()
{
	for (int i = 0; i < 64; ++i) {
		int sample = (i < 32 ? i << 1 : (63 - i) << 1);
		m_pInstrument->SetSample(i, sample);
	}

	GetDocument()->ModifyIrreversible();		// // //
	m_pWaveEditor->RedrawWindow();
	Env.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnPresetPulse50()
{
	for (int i = 0; i < 64; ++i) {
		int sample = (i < 32 ? 0 : 63);
		m_pInstrument->SetSample(i, sample);
	}

	GetDocument()->ModifyIrreversible();		// // //
	m_pWaveEditor->RedrawWindow();
	Env.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnPresetPulse25()
{
	for (int i = 0; i < 64; ++i) {
		int sample = (i < 16 ? 0 : 63);
		m_pInstrument->SetSample(i, sample);
	}

	GetDocument()->ModifyIrreversible();		// // //
	m_pWaveEditor->RedrawWindow();
	Env.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnPresetSawtooth()
{
	for (int i = 0; i < 64; ++i) {
		int sample = i;
		m_pInstrument->SetSample(i, sample);
	}

	GetDocument()->ModifyIrreversible();		// // //
	m_pWaveEditor->RedrawWindow();
	Env.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnModPresetFlat()
{
	for (int i = 0; i < 32; ++i) {
		m_pInstrument->SetModulation(i, 0);
	}

	GetDocument()->ModifyIrreversible();		// // //
	m_pModSequenceEditor->RedrawWindow();
	Env.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnModPresetSine()
{
	for (int i = 0; i < 8; ++i) {
		m_pInstrument->SetModulation(i, 7);
		m_pInstrument->SetModulation(i + 8, 1);
		m_pInstrument->SetModulation(i + 16, 1);
		m_pInstrument->SetModulation(i + 24, 7);
	}

	m_pInstrument->SetModulation(7, 0);
	m_pInstrument->SetModulation(8, 0);
	m_pInstrument->SetModulation(9, 0);

	m_pInstrument->SetModulation(23, 0);
	m_pInstrument->SetModulation(24, 0);
	m_pInstrument->SetModulation(25, 0);

	GetDocument()->ModifyIrreversible();		// // //
	m_pModSequenceEditor->RedrawWindow();
	Env.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int ModSpeed = GetDlgItemInt(IDC_MOD_RATE);
//	int ModDepth = GetDlgItemInt(IDC_MOD_DEPTH);
//	int ModDelay = GetDlgItemInt(IDC_MOD_DELAY);

	if (m_pInstrument->GetModulationSpeed() != ModSpeed) {
		m_pInstrument->SetModulationSpeed(ModSpeed);		// // //
		GetDocument()->ModifyIrreversible();
	}
}

void CInstrumentEditorFDS::OnModRateChange()
{
	if (m_pInstrument) {
		int ModSpeed = std::clamp(GetDlgItemInt(IDC_MOD_RATE), 0u, 4095u);
		if (m_pInstrument->GetModulationSpeed() != ModSpeed) {
			m_pInstrument->SetModulationSpeed(ModSpeed);		// // //
			GetDocument()->ModifyIrreversible();
		}
	}
	Env.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnModDepthChange()
{
	if (m_pInstrument) {
		int ModDepth = std::clamp(GetDlgItemInt(IDC_MOD_DEPTH), 0u, 63u);
		if (m_pInstrument->GetModulationDepth() != ModDepth) {
			m_pInstrument->SetModulationDepth(ModDepth);		// // //
			GetDocument()->ModifyIrreversible();
		}
	}
	Env.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnModDelayChange()
{
	if (m_pInstrument) {
		int ModDelay = std::clamp(GetDlgItemInt(IDC_MOD_DELAY), 0u, 255u);
		if (m_pInstrument->GetModulationDelay() != ModDelay) {
			m_pInstrument->SetModulationDelay(ModDelay);		// // //
			GetDocument()->ModifyIrreversible();
		}
	}
	Env.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnBnClickedCopyWave()
{
	// Assemble a MML string
	CString Str;
	for (auto x : m_pInstrument->GetSamples())		// // //
		Str.AppendFormat(_T("%i "), x);

	CClipboard Clipboard(this, CF_TEXT);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	Clipboard.SetDataPointer((LPCTSTR)Str, Str.GetLength() + 1);
}

void CInstrumentEditorFDS::OnBnClickedPasteWave()
{
	// Paste from clipboard
	CClipboard Clipboard(this, CF_TEXT);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	if (Clipboard.IsDataAvailable()) {
		LPCTSTR text = (LPCTSTR)Clipboard.GetDataPointer();
		if (text != NULL)
			ParseWaveString(text);
	}
}

void CInstrumentEditorFDS::ParseWaveString(LPCTSTR pString)
{
	int i = 0;
	for (auto x : re::tokens(pString)) {		// // //
		int value = CSequenceInstrumentEditPanel::ReadStringValue(x.str());
		m_pInstrument->SetSample(i, std::clamp(value, 0, 63));		// // //
		if (++i >= 64)
			break;
	}

	GetDocument()->ModifyIrreversible();		// // //
	m_pWaveEditor->RedrawWindow();
	Env.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnBnClickedCopyTable()
{
	// Assemble a MML string
	CString Str;
	for (auto x : m_pInstrument->GetModTable())		// // //
		Str.AppendFormat(_T("%i "), x);

	CClipboard Clipboard(this, CF_TEXT);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	Clipboard.SetDataPointer((LPCTSTR)Str, Str.GetLength() + 1);
}

void CInstrumentEditorFDS::OnBnClickedPasteTable()
{
	// Paste from clipboard
	CClipboard Clipboard(this, CF_TEXT);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	if (Clipboard.IsDataAvailable()) {
		LPCTSTR text = (LPCTSTR)Clipboard.GetDataPointer();
		if (text != NULL)
			ParseTableString(text);
	}
}

void CInstrumentEditorFDS::ParseTableString(LPCTSTR pString)
{
	int i = 0;
	for (auto x : re::tokens(pString)) {		// // //
		int value = CSequenceInstrumentEditPanel::ReadStringValue(x.str());
		m_pInstrument->SetModulation(i, std::clamp(value, 0, 7));		// // //
		if (++i >= 32)
			break;
	}

	GetDocument()->ModifyIrreversible();		// // //
	m_pModSequenceEditor->RedrawWindow();
	Env.GetSoundGenerator()->WaveChanged();
}

void CInstrumentEditorFDS::OnBnClickedEnableFm()
{
	/*
	UINT button = IsDlgButtonChecked(IDC_ENABLE_FM);

	EnableModControls(button == 1);
	*/
}

void CInstrumentEditorFDS::EnableModControls(bool enable)
{
	if (!enable) {
		GetDlgItem(IDC_MOD_RATE)->EnableWindow(FALSE);
		GetDlgItem(IDC_MOD_DEPTH)->EnableWindow(FALSE);
		GetDlgItem(IDC_MOD_DELAY)->EnableWindow(FALSE);
//		m_pInstrument->SetModulationEnable(false);
	}
	else {
		GetDlgItem(IDC_MOD_RATE)->EnableWindow(TRUE);
		GetDlgItem(IDC_MOD_DEPTH)->EnableWindow(TRUE);
		GetDlgItem(IDC_MOD_DELAY)->EnableWindow(TRUE);
//		m_pInstrument->SetModulationEnable(true);
	}
}

LRESULT CInstrumentEditorFDS::OnModChanged(WPARAM wParam, LPARAM lParam)
{
	Env.GetSoundGenerator()->WaveChanged();
	return 0;
}
