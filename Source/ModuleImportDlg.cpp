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
#include "ChannelMap.h"		// // //
#include "ModuleImporter.h"		// // //

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
END_MESSAGE_MAP()


// CModuleImportDlg message handlers

BOOL CModuleImportDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_ctlTrackList.SubclassDlgItem(IDC_TRACKS, this);

	m_pImportedDoc->GetModule()->VisitSongs([&] (const CSongData &song, unsigned i) {
		CStringW str;
		auto sv = song.GetTitle();
		str.Format(L"#%02i %.*s", i + 1, sv.size(), sv.data());		// // //
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
	auto &oldModule = *m_pDocument->GetModule();
	auto &newModule = *m_pImportedDoc->GetModule();

	// // // union of sound chip configurations
	const CSoundChipSet &c1 = newModule.GetSoundChipSet();
	const CSoundChipSet &c2 = oldModule.GetSoundChipSet();
	unsigned n1 = newModule.GetNamcoChannels();
	unsigned n2 = oldModule.GetNamcoChannels();
	if (n1 != n2 || c1 != c2) {
		CSoundChipSet merged = c1.MergedWith(c2);
		unsigned n163chs = std::max(n1, n2);
		oldModule.SetChannelMap(Env.GetSoundGenerator()->MakeChannelMap(merged, n163chs));
		newModule.SetChannelMap(Env.GetSoundGenerator()->MakeChannelMap(merged, n163chs));
		Env.GetSoundGenerator()->ModuleChipChanged();		// // //
	}

	// // // remove non-imported songs in advance
	unsigned track = 0;
	for (int i = 0; track < newModule.GetSongCount(); ++i) {
		if (m_ctlTrackList.GetCheck(i) == BST_CHECKED)
			++track;
		else
			newModule.RemoveSong(track);
	}

	CModuleImporter importer {*m_pDocument->GetModule(), newModule};
	if (importer.Validate()) {
		importer.DoImport(
			IsDlgButtonChecked(IDC_INSTRUMENTS) == BST_CHECKED,
			IsDlgButtonChecked(IDC_IMPORT_GROOVE) == BST_CHECKED,
			IsDlgButtonChecked(IDC_IMPORT_DETUNE) == BST_CHECKED);

		// TODO another way to do this?
		m_pDocument->ModifyIrreversible();		// // //
		m_pDocument->UpdateAllViews(NULL, UPDATE_TRACK);		// // //
		m_pDocument->UpdateAllViews(NULL, UPDATE_PATTERN);
		m_pDocument->UpdateAllViews(NULL, UPDATE_FRAME);
		m_pDocument->UpdateAllViews(NULL, UPDATE_INSTRUMENT);
		Env.GetSoundGenerator()->DocumentPropertiesChanged(m_pDocument);
	}

	OnOK();
}

bool CModuleImportDlg::LoadFile(CStringW Path)		// // //
{
	m_pImportedDoc = CFamiTrackerDoc::LoadImportFile(Path);
	return m_pImportedDoc != nullptr;
}
