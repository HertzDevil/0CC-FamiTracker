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

#include "PCMImport.h"
#include "FamiTrackerEnv.h"
#include "ft0cc/doc/dpcm_sample.hpp"		// // //
#include "APU/Types.h"		// // //
#include "Settings.h"
#include "SoundGen.h"
#include "APU/DPCM.h"
#include "FileDialogs.h"		// // //
#include "resampler/resample.hpp"
#include "resampler/resample.inl"
#include <algorithm>		// // //
#include "str_conv/str_conv.hpp"		// // //
#include <MMSystem.h>		// // //

const int CPCMImport::QUALITY_RANGE = 16;
const int CPCMImport::VOLUME_RANGE = 12;		// +/- dB

// Derive a new class from CFileDialog with implemented preview of audio files

class CFileSoundDialog : public CFileDialog
{
public:
	using CFileDialog::CFileDialog;		// // //
	virtual ~CFileSoundDialog();

protected:
	void OnFileNameChange() override;
};

//	CFileSoundDialog

CFileSoundDialog::~CFileSoundDialog()
{
	// Stop any possible playing sound
	PlaySoundW(NULL, NULL, SND_NODEFAULT | SND_SYNC);
}

void CFileSoundDialog::OnFileNameChange()
{
	// Preview wave file

	if (!GetFileExt().CompareNoCase(L"wav") && FTEnv.GetSettings()->General.bWavePreview)
		PlaySoundW(GetPathName(), NULL, SND_FILENAME | SND_NODEFAULT | SND_ASYNC | SND_NOWAIT);

	CFileDialog::OnFileNameChange();
}

// CPCMImport dialog

IMPLEMENT_DYNAMIC(CPCMImport, CDialog)
CPCMImport::CPCMImport(CWnd* pParent /*=NULL*/) :
	CDialog(CPCMImport::IDD, pParent)
{
}

CPCMImport::~CPCMImport()
{
}

void CPCMImport::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPCMImport, CDialog)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_PREVIEW, &CPCMImport::OnBnClickedPreview)
END_MESSAGE_MAP()

std::shared_ptr<ft0cc::doc::dpcm_sample> CPCMImport::ShowDialog() {		// // //
	// Return imported sample, or NULL if cancel/error

	CFileSoundDialog OpenFileDialog(TRUE, 0, 0, OFN_HIDEREADONLY, LoadDefaultFilter(IDS_FILTER_WAV, L"*.wav"));

	auto path = FTEnv.GetSettings()->GetPath(PATH_WAV);		// // //
	OpenFileDialog.m_pOFN->lpstrInitialDir = path.c_str();
	if (OpenFileDialog.DoModal() == IDCANCEL)
		return nullptr;

	// Stop any preview
	PlaySoundW(NULL, NULL, SND_NODEFAULT | SND_SYNC);

	FTEnv.GetSettings()->SetPath(fs::path {(LPCWSTR)OpenFileDialog.GetPathName()}.parent_path(), PATH_WAV);

	m_strPath	  = OpenFileDialog.GetPathName();
	m_strFileName = OpenFileDialog.GetFileName();
	m_pImported.reset();		// // //

	// Open file and read header
	if (!OpenWaveFile())
		return nullptr;

	CDialog::DoModal();

	return m_pImported;
}

// CPCMImport message handlers

BOOL CPCMImport::OnInitDialog()
{
	CDialog::OnInitDialog();

	CSliderCtrl *pQualitySlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_QUALITY));
	CSliderCtrl *pVolumeSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_VOLUME));

	// Initial volume & quality
	m_iQuality = QUALITY_RANGE - 1;	// Max quality
	m_iVolume = 0;					// 0dB

	pQualitySlider->SetRange(0, QUALITY_RANGE - 1);
	pQualitySlider->SetPos(m_iQuality);

	pVolumeSlider->SetRange(0, VOLUME_RANGE * 2);
	pVolumeSlider->SetPos(m_iVolume + VOLUME_RANGE);
	pVolumeSlider->SetTicFreq(3);	// 3dB/tick

	UpdateText();

	SetDlgItemTextW(IDC_SAMPLESIZE, AfxFormattedW(IDS_DPCM_IMPORT_SIZE_FORMAT, L"(unknown)"));		// // //

	UpdateFileInfo();

	SetWindowTextW(AfxFormattedW(IDS_DPCM_IMPORT_TITLE_FORMAT, m_strFileName));

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPCMImport::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CSliderCtrl *pQualitySlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_QUALITY));
	CSliderCtrl *pVolumeSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_VOLUME));

	m_iQuality = pQualitySlider->GetPos();
	m_iVolume = pVolumeSlider->GetPos() - VOLUME_RANGE;

	UpdateText();
	UpdateFileInfo();

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPCMImport::UpdateText()
{
	SetDlgItemTextW(IDC_QUALITY_FRM, AfxFormattedW(IDS_DPCM_IMPORT_QUALITY_FORMAT, FormattedW(L"%i", m_iQuality)));		// // //
	SetDlgItemTextW(IDC_VOLUME_FRM, AfxFormattedW(IDS_DPCM_IMPORT_GAIN_FORMAT, FormattedW(L"%+.0f", (float)m_iVolume)));
}

void CPCMImport::OnBnClickedCancel()
{
	m_iQuality = 0;
	m_iVolume = 0;
	m_pImported = NULL;

	FTEnv.GetSoundGenerator()->CancelPreviewSample();

	OnCancel();
}

void CPCMImport::OnBnClickedOk()
{
	if (auto pSample = GetSample()) {		// // //
		// remove .wav
		m_strFileName.Truncate(m_strFileName.GetLength() - 4);

		// Set the name
		pSample->rename(conv::to_utf8(m_strFileName));

		m_pImported = std::move(pSample);
		m_pCachedSample.reset();

		OnOK();
	}
}

void CPCMImport::OnBnClickedPreview()
{
	if (auto pSample = GetSample()) {		// // //
		SetDlgItemTextW(IDC_SAMPLESIZE, AfxFormattedW(IDS_DPCM_IMPORT_SIZE_FORMAT, FormattedW(L"%i", pSample->size())));

		// Preview the sample
		FTEnv.GetSoundGenerator()->PreviewSample(std::move(pSample), 0, m_iQuality);
	}
}

void CPCMImport::UpdateFileInfo()
{
	SetDlgItemTextW(IDC_SAMPLE_RATE, AfxFormattedW(IDS_DPCM_IMPORT_WAVE_FORMAT,
		FormattedW(L"%i", m_Importer.GetWaveSampleRate()),
		FormattedW(L"%i", m_Importer.GetWaveSampleSize() * 8),
		m_Importer.GetWaveChannelCount() == 1 ? L"Mono" : L"Stereo"));		// // //

	float base_freq = (float)MASTER_CLOCK_NTSC / (float)CDPCM::DMC_PERIODS_NTSC[m_iQuality];

	SetDlgItemTextW(IDC_RESAMPLING, AfxFormattedW(IDS_DPCM_IMPORT_TARGET_FORMAT, FormattedW(L"%g", base_freq)));
}

std::shared_ptr<ft0cc::doc::dpcm_sample> CPCMImport::GetSample() {		// // //
	if (!m_pCachedSample || m_iCachedQuality != m_iQuality || m_iCachedVolume != m_iVolume) {
		// // // Display wait cursor
		CWaitCursor wait;

		m_pCachedSample = ConvertFile();		// // //
	}

	m_iCachedQuality = m_iQuality;
	m_iCachedVolume = m_iVolume;

	return m_pCachedSample;
}

std::shared_ptr<ft0cc::doc::dpcm_sample> CPCMImport::ConvertFile() {		// // //
	return m_Importer.ConvertFile(m_iVolume, m_iQuality);
}

bool CPCMImport::OpenWaveFile() {
	return m_Importer.OpenWaveFile((LPCWSTR)m_strPath);
}
