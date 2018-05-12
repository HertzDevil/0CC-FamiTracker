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

#include "ModulePropertiesDlg.h"
#include "FamiTrackerEnv.h"		// // //
#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"		// // //
#include "FamiTrackerViewMessage.h"		// // //
#include "FileDialogs.h"		// // //
#include "SongData.h"		// // //
#include "MainFrm.h"
#include "ModuleImportDlg.h"
#include "SoundGen.h"
#include "SoundChipService.h"		// // //
#include "ChannelMap.h"		// // //
#include "str_conv/str_conv.hpp"		// // //

LPCWSTR TRACK_FORMAT = L"#%02i %.*s";		// // //

// CModulePropertiesDlg dialog

//
// Contains song list editor and expansion chip selector
//

IMPLEMENT_DYNAMIC(CModulePropertiesDlg, CDialog)
CModulePropertiesDlg::CModulePropertiesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CModulePropertiesDlg::IDD, pParent), m_iSelectedSong(0), m_iExpansions(sound_chip_t::APU)
{
}

CModulePropertiesDlg::~CModulePropertiesDlg()
{
}

void CModulePropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CModulePropertiesDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_SONG_ADD, OnBnClickedSongAdd)
	ON_BN_CLICKED(IDC_SONG_INSERT, OnBnClickedSongInsert)		// // //
	ON_BN_CLICKED(IDC_SONG_REMOVE, OnBnClickedSongRemove)
	ON_BN_CLICKED(IDC_SONG_UP, OnBnClickedSongUp)
	ON_BN_CLICKED(IDC_SONG_DOWN, OnBnClickedSongDown)
	ON_EN_CHANGE(IDC_SONGNAME, OnEnChangeSongname)
	ON_BN_CLICKED(IDC_SONG_IMPORT, OnBnClickedSongImport)
	// ON_CBN_SELCHANGE(IDC_EXPANSION, OnCbnSelchangeExpansion)
	ON_WM_HSCROLL()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_SONGLIST, OnLvnItemchangedSonglist)
	ON_BN_CLICKED(IDC_EXPANSION_VRC6, OnBnClickedExpansionVRC6)
	ON_BN_CLICKED(IDC_EXPANSION_VRC7, OnBnClickedExpansionVRC7)
	ON_BN_CLICKED(IDC_EXPANSION_FDS, OnBnClickedExpansionFDS)
	ON_BN_CLICKED(IDC_EXPANSION_MMC5, OnBnClickedExpansionMMC5)
	ON_BN_CLICKED(IDC_EXPANSION_S5B, OnBnClickedExpansionS5B)
	ON_BN_CLICKED(IDC_EXPANSION_N163, OnBnClickedExpansionN163)
	ON_CBN_SELCHANGE(IDC_COMBO_LINEARPITCH, OnCbnSelchangeComboLinearpitch)
END_MESSAGE_MAP()


// CModulePropertiesDlg message handlers

BOOL CModulePropertiesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// // //
	m_cListSongs.SubclassDlgItem(IDC_SONGLIST, this);
	m_cButtonEnableVRC6.SubclassDlgItem(IDC_EXPANSION_VRC6, this);
	m_cButtonEnableVRC7.SubclassDlgItem(IDC_EXPANSION_VRC7, this);
	m_cButtonEnableFDS .SubclassDlgItem(IDC_EXPANSION_FDS , this);
	m_cButtonEnableMMC5.SubclassDlgItem(IDC_EXPANSION_MMC5, this);
	m_cButtonEnableN163.SubclassDlgItem(IDC_EXPANSION_N163, this);
	m_cButtonEnableS5B .SubclassDlgItem(IDC_EXPANSION_S5B , this);
	m_cSliderN163Chans.SubclassDlgItem(IDC_CHANNELS, this);
	m_cStaticN163Chans.SubclassDlgItem(IDC_CHANNELS_NR, this);
	m_cComboVibrato.SubclassDlgItem(IDC_VIBRATO, this);
	m_cComboLinearPitch.SubclassDlgItem(IDC_COMBO_LINEARPITCH, this);

	// Get active document
	CFrameWnd *pFrameWnd = static_cast<CFrameWnd*>(GetParent());
	m_pDocument = static_cast<CFamiTrackerDoc*>(pFrameWnd->GetActiveDocument());
	m_pModule = m_pDocument->GetModule();		// // //

	m_cListSongs.InsertColumn(0, L"Songs", 0, 150);
	m_cListSongs.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	FillSongList();
	SelectSong(0);		// // //

	// Expansion chips
	m_iExpansions = m_pModule->GetSoundChipSet();
	m_cButtonEnableVRC6.SetCheck(m_iExpansions.ContainsChip(sound_chip_t::VRC6));
	m_cButtonEnableVRC7.SetCheck(m_iExpansions.ContainsChip(sound_chip_t::VRC7));
	m_cButtonEnableFDS .SetCheck(m_iExpansions.ContainsChip(sound_chip_t::FDS ));
	m_cButtonEnableMMC5.SetCheck(m_iExpansions.ContainsChip(sound_chip_t::MMC5));
	m_cButtonEnableN163.SetCheck(m_iExpansions.ContainsChip(sound_chip_t::N163));
	m_cButtonEnableS5B .SetCheck(m_iExpansions.ContainsChip(sound_chip_t::S5B ));

	// Vibrato
	m_cComboVibrato.SetCurSel((m_pModule->GetVibratoStyle() == vibrato_t::Bidir) ? 0 : 1);
	m_cComboLinearPitch.SetCurSel(m_pModule->GetLinearPitch() ? 1 : 0);

	// Namco channel count
	m_cSliderN163Chans.SetRange(1, MAX_CHANNELS_N163);		// // //

	CStringW channelsStr(MAKEINTRESOURCEW(IDS_PROPERTIES_CHANNELS));		// // //
	if (m_iExpansions.ContainsChip(sound_chip_t::N163)) {
		m_iN163Channels = m_pModule->GetNamcoChannels();

		m_cSliderN163Chans.SetPos(m_iN163Channels);
		m_cSliderN163Chans.EnableWindow(TRUE);
		m_cStaticN163Chans.EnableWindow(TRUE);
		AppendFormatW(channelsStr, L" %i", m_iN163Channels);
	}
	else {
		m_iN163Channels = 1;

		m_cSliderN163Chans.SetPos(0);
		m_cSliderN163Chans.EnableWindow(FALSE);
		channelsStr.Append(L" N/A");
	}
	m_cStaticN163Chans.SetWindowTextW(channelsStr);

	auto *pFxx = static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN_FXX_SPLIT));		// // //
	pFxx->SetRange(MIN_SPEED + 1, 0xFF);
	pFxx->SetPos(m_pModule->GetSpeedSplitPoint());

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CModulePropertiesDlg::OnBnClickedOk()
{
	CMainFrame *pMainFrame = static_cast<CMainFrame*>(GetParentFrame());

	if (!m_iExpansions.ContainsChip(sound_chip_t::N163))
		m_iN163Channels = 0;
	if (m_pModule->GetNamcoChannels() != m_iN163Channels || m_pModule->GetSoundChipSet() != m_iExpansions) {		// // //
		auto pMap = Env.GetSoundChipService()->MakeChannelMap(m_iExpansions, m_iN163Channels);
		pMap->GetChannelOrder() = pMap->GetChannelOrder().BuiltinOrder();
		m_pModule->SetChannelMap(std::move(pMap));
		m_pDocument->ModifyIrreversible();
		m_pDocument->UpdateAllViews(NULL, UPDATE_PROPERTIES);
		Env.GetSoundGenerator()->ModuleChipChanged();
	}

	// Vibrato
	vibrato_t newVib = m_cComboVibrato.GetCurSel() == 0 ? vibrato_t::Bidir : vibrato_t::Up;		// // //
	bool newLinear = m_cComboLinearPitch.GetCurSel() == 1;
	int fxx = static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN_FXX_SPLIT))->GetPos();
	if (newVib != m_pModule->GetVibratoStyle() || newLinear != m_pModule->GetLinearPitch() || fxx != m_pModule->GetSpeedSplitPoint())
		m_pDocument->ModifyIrreversible();
	m_pModule->SetVibratoStyle(newVib);
	m_pModule->SetLinearPitch(newLinear);
	m_pModule->SetSpeedSplitPoint(fxx);

	if (pMainFrame->GetSelectedTrack() != m_iSelectedSong)
		pMainFrame->SelectTrack(m_iSelectedSong);

	pMainFrame->UpdateControls();

	Env.GetSoundGenerator()->DocumentPropertiesChanged(m_pDocument);

	OnOK();
}

void CModulePropertiesDlg::OnBnClickedSongAdd()
{
	// Try to add a track
	unsigned int NewTrack = m_pModule->GetSongCount();		// // //
	if (!m_pModule->InsertSong(NewTrack, m_pModule->MakeNewSong()))
		return;
	m_pDocument->ModifyIrreversible();		// // //
	m_pDocument->UpdateAllViews(NULL, UPDATE_TRACK);

	m_cListSongs.InsertItem(NewTrack, GetSongString(NewTrack));

	SelectSong(NewTrack);
}

void CModulePropertiesDlg::OnBnClickedSongInsert()		// // //
{
	// Try to add a track
	unsigned int NewTrack = m_iSelectedSong + 1;		// // //
	if (!m_pModule->InsertSong(NewTrack, m_pModule->MakeNewSong()))
		return;
	m_pDocument->ModifyIrreversible();		// // //
	m_pDocument->UpdateAllViews(NULL, UPDATE_TRACK);

	m_cListSongs.InsertItem(NewTrack, GetSongString(NewTrack));
	m_pModule->VisitSongs([&] (const CSongData &, unsigned index) {
		m_cListSongs.SetItemText(index, 0, GetSongString(index));
	});

	SelectSong(NewTrack);
}

void CModulePropertiesDlg::OnBnClickedSongRemove()
{
	ASSERT(m_iSelectedSong != -1);

	unsigned Count = m_pModule->GetSongCount();

	int SelCount = m_cListSongs.GetSelectedCount();		// // //
	if (Count <= static_cast<unsigned>(SelCount))
		return; // Single track

	// Display warning first
	if (AfxMessageBox(IDS_SONG_DELETE, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO)
		return;

	for (unsigned i = Count - 1; i < Count; --i)		// // //
		if (m_cListSongs.GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED) {
			if (m_iSelectedSong > i)
				--m_iSelectedSong;
			m_cListSongs.DeleteItem(i);
			m_pModule->RemoveSong(i);
		}
	m_pDocument->ModifyIrreversible();		// // //
	m_pDocument->UpdateAllViews(NULL, UPDATE_TRACK);

	Count = m_pModule->GetSongCount();	// Get new track count

	// Redraw track list
	m_pModule->VisitSongs([&] (const CSongData &song, unsigned index) {
		m_cListSongs.SetItemText(index, 0, GetSongString(index));		// // //
	});

	if (m_iSelectedSong >= Count)
		m_iSelectedSong = Count - 1;
	SelectSong(m_iSelectedSong);
}

void CModulePropertiesDlg::OnBnClickedSongUp()
{
	unsigned Song = m_iSelectedSong;
	if (Song <= 0)
		return;

	m_pModule->SwapSongs(Song, Song - 1);
	m_pDocument->ModifyIrreversible();		// // //
	m_pDocument->UpdateAllViews(NULL, UPDATE_TRACK);

	m_cListSongs.SetItemText(Song, 0, GetSongString(Song));
	m_cListSongs.SetItemText(Song - 1, 0, GetSongString(Song - 1));

	SelectSong(Song - 1);
}

void CModulePropertiesDlg::OnBnClickedSongDown()
{
	unsigned Song = m_iSelectedSong;
	if (Song >= (m_pModule->GetSongCount() - 1))
		return;

	m_pModule->SwapSongs(Song, Song + 1);
	m_pDocument->ModifyIrreversible();		// // //
	m_pDocument->UpdateAllViews(NULL, UPDATE_TRACK);

	m_cListSongs.SetItemText(Song, 0, GetSongString(Song));
	m_cListSongs.SetItemText(Song + 1, 0, GetSongString(Song + 1));

	SelectSong(Song + 1);
}

void CModulePropertiesDlg::OnEnChangeSongname()
{
	if (m_iSelectedSong == -1 || !m_bSingleSelection)
		return;

	CStringW WText;
	GetDlgItemTextW(IDC_SONGNAME, WText);
	auto Text = conv::to_utf8(WText);

	CSongData *pSong = m_pModule->GetSong(m_iSelectedSong);		// // //
	if (pSong->GetTitle() != Text)		// // //
		m_pDocument->ModifyIrreversible();
	pSong->SetTitle(Text);
	m_cListSongs.SetItemText(m_iSelectedSong, 0, GetSongString(m_iSelectedSong));
	m_pDocument->UpdateAllViews(NULL, UPDATE_TRACK);
}

void CModulePropertiesDlg::SelectSong(int Song)
{
	ASSERT(Song >= 0);

	for (int i = m_cListSongs.GetItemCount() - 1; i >= 0; --i)		// // //
		m_cListSongs.SetItemState(i, i == Song ? (LVIS_SELECTED | LVIS_FOCUSED) : 0, LVIS_SELECTED | LVIS_FOCUSED);
	m_iSelectedSong = Song;
	m_bSingleSelection = true;
	UpdateSongButtons();
	m_cListSongs.EnsureVisible(Song, FALSE);
	m_cListSongs.SetFocus();
}

void CModulePropertiesDlg::UpdateSongButtons()
{
	unsigned TrackCount = m_pModule->GetSongCount();
	bool Empty = m_cListSongs.GetSelectedCount() == 0;

	GetDlgItem(IDC_SONG_ADD)->EnableWindow((TrackCount == MAX_TRACKS) ? FALSE : TRUE);
	GetDlgItem(IDC_SONG_INSERT)->EnableWindow((TrackCount == MAX_TRACKS || !m_bSingleSelection || Empty) ? FALSE : TRUE);
	GetDlgItem(IDC_SONG_REMOVE)->EnableWindow((TrackCount == 1 || Empty) ? FALSE : TRUE);
	GetDlgItem(IDC_SONG_UP)->EnableWindow((m_iSelectedSong == 0 || !m_bSingleSelection || Empty) ? FALSE : TRUE);
	GetDlgItem(IDC_SONG_DOWN)->EnableWindow((m_iSelectedSong == TrackCount - 1 || !m_bSingleSelection || Empty) ? FALSE : TRUE);
	GetDlgItem(IDC_SONG_IMPORT)->EnableWindow((TrackCount == MAX_TRACKS) ? FALSE : TRUE);
}

void CModulePropertiesDlg::OnBnClickedSongImport()
{
	CModuleImportDlg importDlg(m_pDocument);

	CFileDialog OpenFileDlg(TRUE, L"0cc", 0, OFN_HIDEREADONLY, LoadDefaultFilter(IDS_FILTER_0CC, L"*.0cc; *.ftm"));		// // //

	if (OpenFileDlg.DoModal() == IDCANCEL)
		return;

	if (!importDlg.LoadFile(OpenFileDlg.GetPathName()))		// // //
		return;

	if (!importDlg.DoModal())		// // //
		return;

	FillSongList();
	SelectSong(m_pModule->GetSongCount() - 1);		// // //

	m_iExpansions = m_pModule->GetSoundChipSet();		// // //
	m_iN163Channels = m_pModule->GetNamcoChannels();
	m_cButtonEnableVRC6.SetCheck(m_iExpansions.ContainsChip(sound_chip_t::VRC6));
	m_cButtonEnableVRC7.SetCheck(m_iExpansions.ContainsChip(sound_chip_t::VRC7));
	m_cButtonEnableFDS .SetCheck(m_iExpansions.ContainsChip(sound_chip_t::FDS ));
	m_cButtonEnableMMC5.SetCheck(m_iExpansions.ContainsChip(sound_chip_t::MMC5));
	m_cButtonEnableN163.SetCheck(m_iExpansions.ContainsChip(sound_chip_t::N163));
	m_cButtonEnableS5B .SetCheck(m_iExpansions.ContainsChip(sound_chip_t::S5B ));
	m_pDocument->UpdateAllViews(NULL, UPDATE_PROPERTIES);
}
/*
void CModulePropertiesDlg::OnCbnSelchangeExpansion()
{
	CComboBox *pExpansionChipBox = static_cast<CComboBox*>(GetDlgItem(IDC_EXPANSION));
	CSliderCtrl *pSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_CHANNELS));

	// Expansion chip
	unsigned int iExpansionChip = theApp.GetChannelMap()->GetChipIdent(pExpansionChipBox->GetCurSel());

	CStringW channelsStr(MAKEINTRESOURCEW(IDS_PROPERTIES_CHANNELS));		// // //
	if (iExpansionChip == sound_chip_t::N163) {
		pSlider->EnableWindow(TRUE);
		int Channels = m_pDocument->GetNamcoChannels();
		pSlider->SetPos(Channels);
		AppendFormatW(channelsStr, L" %i", Channels);
	}
	else {
		pSlider->EnableWindow(FALSE);
		channelsStr.Append(L" N/A");
	}
	SetDlgItemTextW(IDC_CHANNELS_NR, channelsStr);
}
*/
void CModulePropertiesDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	m_iN163Channels = m_cSliderN163Chans.GetPos();

	CStringW text(MAKEINTRESOURCEW(IDS_PROPERTIES_CHANNELS));		// // //
	AppendFormatW(text, L" %i", m_iN163Channels);
	m_cStaticN163Chans.SetWindowTextW(text);

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CModulePropertiesDlg::OnLvnItemchangedSonglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if (pNMLV->uChanged & LVIF_STATE) {
		if (pNMLV->uNewState & LVNI_SELECTED) {		// // //
			m_iSelectedSong = pNMLV->iItem;
			SetDlgItemTextW(IDC_SONGNAME, conv::to_wide(m_pModule->GetSong(m_iSelectedSong)->GetTitle()).data());		// // //
		}
		m_bSingleSelection = m_cListSongs.GetSelectedCount() == 1;
		UpdateSongButtons();
	}

	*pResult = 0;
}

CStringW CModulePropertiesDlg::GetSongString(unsigned index) const {		// // //
	const auto *pSong = m_pModule->GetSong(index);
	auto sv = conv::to_wide(pSong ? pSong->GetTitle() : "(N/A)");
	return FormattedW(TRACK_FORMAT, index + 1, sv.size(), sv.data());		// // // start counting songs from 1
}

void CModulePropertiesDlg::FillSongList()
{
	m_cListSongs.DeleteAllItems();

	// Song editor
	m_pModule->VisitSongs([&] (const CSongData &, unsigned index) {
		m_cListSongs.InsertItem(index, GetSongString(index));
	});
}

BOOL CModulePropertiesDlg::PreTranslateMessage(MSG* pMsg)
{
	if (GetFocus() == &m_cListSongs) {
		if (pMsg->message == WM_KEYDOWN) {
			switch (pMsg->wParam) {
				case VK_DELETE:
					// Delete song
					OnBnClickedSongRemove();		// // //
					break;
				case VK_INSERT:
					// Insert song
					if (m_bSingleSelection)		// // //
						OnBnClickedSongInsert();
					break;
			}
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CModulePropertiesDlg::OnBnClickedExpansionVRC6()
{
	m_iExpansions = m_iExpansions.EnableChip(sound_chip_t::VRC6, m_cButtonEnableVRC6.GetCheck() == BST_CHECKED);		// // //
}

void CModulePropertiesDlg::OnBnClickedExpansionVRC7()
{
	m_iExpansions = m_iExpansions.EnableChip(sound_chip_t::VRC7, m_cButtonEnableVRC7.GetCheck() == BST_CHECKED);		// // //
}

void CModulePropertiesDlg::OnBnClickedExpansionFDS()
{
	m_iExpansions = m_iExpansions.EnableChip(sound_chip_t::FDS, m_cButtonEnableFDS.GetCheck() == BST_CHECKED);		// // //
}

void CModulePropertiesDlg::OnBnClickedExpansionMMC5()
{
	m_iExpansions = m_iExpansions.EnableChip(sound_chip_t::MMC5, m_cButtonEnableMMC5.GetCheck() == BST_CHECKED);		// // //
}

void CModulePropertiesDlg::OnBnClickedExpansionS5B()
{
	m_iExpansions = m_iExpansions.EnableChip(sound_chip_t::S5B, m_cButtonEnableS5B.GetCheck() == BST_CHECKED);		// // //
}

void CModulePropertiesDlg::OnBnClickedExpansionN163()
{
	CStringW channelsStr(MAKEINTRESOURCEW(IDS_PROPERTIES_CHANNELS));		// // //

	// Expansion chip
	if (m_cButtonEnableN163.GetCheck() == BST_CHECKED) {
		m_iExpansions = m_iExpansions.WithChip(sound_chip_t::N163);

		if (!m_iN163Channels)
			m_iN163Channels = 1;		// // //
		m_cSliderN163Chans.SetPos(m_iN163Channels);
		m_cSliderN163Chans.EnableWindow(TRUE);
		m_cStaticN163Chans.EnableWindow(TRUE);
		AppendFormatW(channelsStr, L" %i", m_iN163Channels);
	}
	else {
		m_iExpansions = m_iExpansions.WithoutChip(sound_chip_t::N163);

		m_cSliderN163Chans.SetPos(m_iN163Channels = 0);
		m_cSliderN163Chans.EnableWindow(FALSE);
		m_cStaticN163Chans.EnableWindow(FALSE);
		channelsStr.Append(L" N/A");
	}
	m_cStaticN163Chans.SetWindowTextW(channelsStr);
}

void CModulePropertiesDlg::OnCbnSelchangeComboLinearpitch()
{
	static bool First = true;
	if (First) {
		First = false;
		AfxMessageBox(
			L"Because linear pitch mode is a planned feature in the official build, "
			L"changes to this setting might not be reflected when the current module is loaded from "
			L"a future official release that implements this feature.", MB_OK | MB_ICONINFORMATION);
	}
}
