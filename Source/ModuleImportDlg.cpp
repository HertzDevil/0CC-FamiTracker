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

#include "ModuleImportDlg.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"		// // //
#include "SongData.h"		// // //
#include "SoundChipSet.h"		// // //
#include "FamiTrackerViewMessage.h"		// // //
#include "FamiTrackerEnv.h"		// // //
#include "SoundGen.h"		// // //
//#include <algorithm>

// CModuleImportDlg dialog

IMPLEMENT_DYNAMIC(CModuleImportDlg, CDialog)

CModuleImportDlg::CModuleImportDlg(CFamiTrackerDoc *pDoc)
	: CDialog(CModuleImportDlg::IDD, NULL), m_pDocument(pDoc)
{
}

CModuleImportDlg::~CModuleImportDlg()
{
}

void CModuleImportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CModuleImportDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CModuleImportDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CModuleImportDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CModuleImportDlg message handlers

BOOL CModuleImportDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_ctlTrackList.SubclassDlgItem(IDC_TRACKS, this);

	m_pImportedDoc->GetModule()->VisitSongs([&] (const CSongData &song, unsigned i) {
		CString str;
		auto sv = song.GetTitle();
		str.Format(_T("#%02i %.*s"), i + 1, sv.size(), sv.data());		// // //
		m_ctlTrackList.AddString(str);
		m_ctlTrackList.SetCheck(i, 1);
	});

	CheckDlgButton(IDC_INSTRUMENTS, BST_CHECKED);
	CheckDlgButton(IDC_IMPORT_GROOVE, BST_CHECKED);		// // //
	CheckDlgButton(IDC_IMPORT_DETUNE, BST_UNCHECKED);		// // //

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CModuleImportDlg::OnBnClickedOk()
{
	if (!(ImportInstruments() && ImportGrooves() && ImportDetune() && ImportTracks()))		// // //
		AfxMessageBox(IDS_IMPORT_FAILED, MB_ICONERROR);

	// TODO another way to do this?
	m_pDocument->ModifyIrreversible();		// // //
	m_pDocument->UpdateAllViews(NULL, UPDATE_TRACK);		// // //
	m_pDocument->UpdateAllViews(NULL, UPDATE_PATTERN);
	m_pDocument->UpdateAllViews(NULL, UPDATE_FRAME);
	m_pDocument->UpdateAllViews(NULL, UPDATE_INSTRUMENT);
	Env.GetSoundGenerator()->DocumentPropertiesChanged(m_pDocument);

	OnOK();
}

void CModuleImportDlg::OnBnClickedCancel()
{
	OnCancel();
}

bool CModuleImportDlg::LoadFile(CString Path)		// // //
{
	m_pImportedDoc = CFamiTrackerDoc::LoadImportFile(Path);
	if (!m_pImportedDoc)
		return false;

	auto &oldModule = *m_pDocument->GetModule();
	auto &newModule = *m_pImportedDoc->GetModule();

	// Check expansion chip match
	// // // import as superset of expansion chip configurations
	const CSoundChipSet &c1 = newModule.GetSoundChipSet();
	const CSoundChipSet &c2 = oldModule.GetSoundChipSet();
	unsigned n1 = newModule.GetNamcoChannels();
	unsigned n2 = oldModule.GetNamcoChannels();
	if (n1 != n2 || c1 != c2) {
		m_pImportedDoc->SelectExpansionChip(c1.MergedWith(c2), std::max(n1, n2));
		m_pDocument->SelectExpansionChip(c1.MergedWith(c2), std::max(n1, n2));
	}

	return true;
}

bool CModuleImportDlg::ImportInstruments()
{
	for (int i = 0; i < MAX_INSTRUMENTS; ++i)		// // //
		m_iInstrumentTable[i] = i;

	if (IsDlgButtonChecked(IDC_INSTRUMENTS) == BST_CHECKED)
		if (!m_pDocument->ImportInstruments(*m_pImportedDoc->GetModule(), m_iInstrumentTable))
			return false;

	return true;
}

bool CModuleImportDlg::ImportGrooves()		// // //
{
	for (int i = 0; i < MAX_GROOVE; ++i)
		m_iGrooveMap[i] = i;

	if (IsDlgButtonChecked(IDC_IMPORT_GROOVE) == BST_CHECKED)
		if (!m_pDocument->ImportGrooves(*m_pImportedDoc->GetModule(), m_iGrooveMap))
			return false;

	return true;
}

bool CModuleImportDlg::ImportDetune()		// // //
{
	if (IsDlgButtonChecked(IDC_IMPORT_DETUNE) == BST_CHECKED)
		if (!m_pDocument->ImportDetune(*m_pImportedDoc->GetModule()))
			return false;

	return true;
}

bool CModuleImportDlg::ImportTracks() {
	auto &oldModule = *m_pDocument->GetModule();
	auto &newModule = *m_pImportedDoc->GetModule();

	// // // ensure there are enough track slots
	unsigned count = 0;
	newModule.VisitSongs([&] (const CSongData &, unsigned i) {
		if (m_ctlTrackList.GetCheck(i) == BST_CHECKED)
			++count;
	});
	if (count + oldModule.GetSongCount() > MAX_TRACKS)
		return false;

	// Import track
	newModule.VisitSongs([&] (const CSongData &, unsigned i) {
		if (m_ctlTrackList.GetCheck(i) == BST_CHECKED) {
			auto pSong = newModule.ReleaseSong(i);		// // //
			auto &song = *pSong;
			oldModule.InsertSong(oldModule.GetSongCount(), std::move(pSong));

			// // // translate instruments and grooves outside modules
			if (song.GetSongGroove())
				song.SetSongSpeed(m_iGrooveMap[song.GetSongSpeed()]);
			song.VisitPatterns([this] (CPatternData &pat) {
				pat.VisitRows([this] (stChanNote &note) {
					// Translate instrument number
					if (note.Instrument < MAX_INSTRUMENTS)
						note.Instrument = m_iInstrumentTable[note.Instrument];
					// // // Translate groove commands
					for (int i = 0; i < MAX_EFFECT_COLUMNS; ++i)
						if (note.EffNumber[i] == EF_GROOVE && note.EffParam[i] < MAX_GROOVE)
							note.EffParam[i] = m_iGrooveMap[note.EffParam[i]];
				});
			});
		}
	});

	return true;
}
