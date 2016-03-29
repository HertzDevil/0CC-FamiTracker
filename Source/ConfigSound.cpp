/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerTypes.h"		// // //
#include "APU/Types.h"		// // //
#include "ConfigSound.h"
#include "SoundGen.h"
#include "Settings.h"
#include "DirectSound.h"

// CConfigSound dialog

IMPLEMENT_DYNAMIC(CConfigSound, CPropertyPage)
CConfigSound::CConfigSound()
	: CPropertyPage(CConfigSound::IDD)
{
}

CConfigSound::~CConfigSound()
{
}

void CConfigSound::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CConfigSound, CPropertyPage)
	ON_WM_HSCROLL()
	ON_CBN_SELCHANGE(IDC_SAMPLE_RATE, OnCbnSelchangeSampleRate)
	ON_CBN_SELCHANGE(IDC_SAMPLE_SIZE, OnCbnSelchangeSampleSize)
	ON_CBN_SELCHANGE(IDC_DEVICES, OnCbnSelchangeDevices)
END_MESSAGE_MAP()

const int MAX_BUFFER_LEN = 500;	// 500 ms

// CConfigSound message handlers

BOOL CConfigSound::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CComboBox *pSampleRate	= static_cast<CComboBox*>(GetDlgItem(IDC_SAMPLE_RATE));
	CComboBox *pSampleSize	= static_cast<CComboBox*>(GetDlgItem(IDC_SAMPLE_SIZE));
	CComboBox *pDevices		= static_cast<CComboBox*>(GetDlgItem(IDC_DEVICES));
	
	CSliderCtrl *pBufSlider			  = static_cast<CSliderCtrl*>(GetDlgItem(IDC_BUF_LENGTH));
	CSliderCtrl *pBassSlider		  = static_cast<CSliderCtrl*>(GetDlgItem(IDC_BASS_FREQ));
	CSliderCtrl *pTrebleSliderFreq	  = static_cast<CSliderCtrl*>(GetDlgItem(IDC_TREBLE_FREQ));
	CSliderCtrl *pTrebleSliderDamping = static_cast<CSliderCtrl*>(GetDlgItem(IDC_TREBLE_DAMP));
	CSliderCtrl *pVolumeSlider		  = static_cast<CSliderCtrl*>(GetDlgItem(IDC_VOLUME));

	// Set ranges
	pBufSlider->SetRange(1, MAX_BUFFER_LEN);
	pBassSlider->SetRange(16, 4000);
	pTrebleSliderFreq->SetRange(20, 20000);
	pTrebleSliderDamping->SetRange(0, 90);
	pVolumeSlider->SetRange(0, 100);

	CSettings *pSettings = theApp.GetSettings();

	// Read settings
	switch (pSettings->Sound.iSampleRate) {
		case 11025: pSampleRate->SelectString(0, _T("11 025 Hz")); break;
		case 22050: pSampleRate->SelectString(0, _T("22 050 Hz")); break;
		case 44100: pSampleRate->SelectString(0, _T("44 100 Hz")); break;
		case 48000: pSampleRate->SelectString(0, _T("48 000 Hz")); break;
		case 96000: pSampleRate->SelectString(0, _T("96 000 Hz")); break;
	}

	switch (pSettings->Sound.iSampleSize) {
		case 16: pSampleSize->SelectString(0, _T("16 bit")); break;
		case 8:	 pSampleSize->SelectString(0, _T("8 bit")); break;
	}

	pBufSlider->SetPos(pSettings->Sound.iBufferLength);
	pBassSlider->SetPos(pSettings->Sound.iBassFilter);
	pTrebleSliderFreq->SetPos(pSettings->Sound.iTrebleFilter);
	pTrebleSliderDamping->SetPos(pSettings->Sound.iTrebleDamping);
	pVolumeSlider->SetPos(pSettings->Sound.iMixVolume);

	UpdateTexts();

	CDSound *pDSound = theApp.GetSoundGenerator()->GetSoundInterface();
	const int iCount = pDSound->GetDeviceCount();

	for (int i = 0; i < iCount; ++i)
		pDevices->AddString(pDSound->GetDeviceName(i));

	pDevices->SetCurSel(pSettings->Sound.iDevice);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigSound::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	UpdateTexts();
	SetModified();
	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}

BOOL CConfigSound::OnApply()
{
	CComboBox *pDevices = static_cast<CComboBox*>(GetDlgItem(IDC_DEVICES));
	CComboBox *pSampleRate = static_cast<CComboBox*>(GetDlgItem(IDC_SAMPLE_RATE));
	CComboBox *pSampleSize = static_cast<CComboBox*>(GetDlgItem(IDC_SAMPLE_SIZE));
	CSliderCtrl *pBufSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_BUF_LENGTH));

	switch (pSampleRate->GetCurSel()) {
		case 0: theApp.GetSettings()->Sound.iSampleRate = 11025; break;
		case 1: theApp.GetSettings()->Sound.iSampleRate = 22050; break;
		case 2: theApp.GetSettings()->Sound.iSampleRate = 44100; break;
		case 3: theApp.GetSettings()->Sound.iSampleRate = 48000; break;
		case 4: theApp.GetSettings()->Sound.iSampleRate = 96000; break;
	}

	switch (pSampleSize->GetCurSel()) {
		case 0: theApp.GetSettings()->Sound.iSampleSize = 16; break;
		case 1: theApp.GetSettings()->Sound.iSampleSize = 8; break;
	}

	theApp.GetSettings()->Sound.iBufferLength = pBufSlider->GetPos();

	theApp.GetSettings()->Sound.iBassFilter		= static_cast<CSliderCtrl*>(GetDlgItem(IDC_BASS_FREQ))->GetPos();
	theApp.GetSettings()->Sound.iTrebleFilter	= static_cast<CSliderCtrl*>(GetDlgItem(IDC_TREBLE_FREQ))->GetPos();
	theApp.GetSettings()->Sound.iTrebleDamping	= static_cast<CSliderCtrl*>(GetDlgItem(IDC_TREBLE_DAMP))->GetPos();
	theApp.GetSettings()->Sound.iMixVolume		= static_cast<CSliderCtrl*>(GetDlgItem(IDC_VOLUME))->GetPos();

	theApp.GetSettings()->Sound.iDevice	= pDevices->GetCurSel();

	theApp.LoadSoundConfig();

	return CPropertyPage::OnApply();
}

void CConfigSound::OnCbnSelchangeSampleRate()
{
	SetModified();
}

void CConfigSound::OnCbnSelchangeSampleSize()
{
	SetModified();
}

void CConfigSound::OnCbnSelchangeDevices()
{
	SetModified();
}

void CConfigSound::UpdateTexts()
{
	CString Text;

	Text.Format(_T("%i ms"), static_cast<CSliderCtrl*>(GetDlgItem(IDC_BUF_LENGTH))->GetPos());
	SetDlgItemText(IDC_BUF_LEN, Text);

	Text.Format(_T("%i Hz"), static_cast<CSliderCtrl*>(GetDlgItem(IDC_BASS_FREQ))->GetPos());
	SetDlgItemText(IDC_BASS_FREQ_T, Text);

	Text.Format(_T("%i Hz"), static_cast<CSliderCtrl*>(GetDlgItem(IDC_TREBLE_FREQ))->GetPos());
	SetDlgItemText(IDC_TREBLE_FREQ_T, Text);

	Text.Format(_T("-%i dB"), static_cast<CSliderCtrl*>(GetDlgItem(IDC_TREBLE_DAMP))->GetPos());
	SetDlgItemText(IDC_TREBLE_DAMP_T, Text);

	Text.Format(_T("%i %%"), static_cast<CSliderCtrl*>(GetDlgItem(IDC_VOLUME))->GetPos());
	SetDlgItemText(IDC_VOLUME_T, Text);
}
