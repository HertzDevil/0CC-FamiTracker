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
#include "SongData.h"		// // //
#include "MainFrm.h"
#include "ModuleImportDlg.h"
#include "SoundGen.h"

LPCTSTR TRACK_FORMAT = _T("#%02i %.*s");		// // //

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

	// Get active document
	CFrameWnd *pFrameWnd = static_cast<CFrameWnd*>(GetParent());
	m_pDocument = static_cast<CFamiTrackerDoc*>(pFrameWnd->GetActiveDocument());
	m_pModule = m_pDocument->GetModule();		// // //

	CListCtrl *pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));
	pSongList->InsertColumn(0, _T("Songs"), 0, 150);
	pSongList->SetExtendedStyle(LVS_EX_FULLROWSELECT);

	FillSongList();
	SelectSong(0);		// // //

	// Expansion chips
	m_iExpansions = m_pModule->GetSoundChipSet();
	((CButton*)GetDlgItem(IDC_EXPANSION_VRC6))->SetCheck(m_iExpansions.ContainsChip(sound_chip_t::VRC6));
	((CButton*)GetDlgItem(IDC_EXPANSION_VRC7))->SetCheck(m_iExpansions.ContainsChip(sound_chip_t::VRC7));
	((CButton*)GetDlgItem(IDC_EXPANSION_FDS ))->SetCheck(m_iExpansions.ContainsChip(sound_chip_t::FDS ));
	((CButton*)GetDlgItem(IDC_EXPANSION_MMC5))->SetCheck(m_iExpansions.ContainsChip(sound_chip_t::MMC5));
	((CButton*)GetDlgItem(IDC_EXPANSION_N163))->SetCheck(m_iExpansions.ContainsChip(sound_chip_t::N163));
	((CButton*)GetDlgItem(IDC_EXPANSION_S5B ))->SetCheck(m_iExpansions.ContainsChip(sound_chip_t::S5B ));

	// Vibrato
	CComboBox *pVibratoBox = static_cast<CComboBox*>(GetDlgItem(IDC_VIBRATO));
	pVibratoBox->SetCurSel((m_pModule->GetVibratoStyle() == VIBRATO_NEW) ? 0 : 1);
	CComboBox *pPitchBox = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_LINEARPITCH));		// // //
	pPitchBox->SetCurSel(m_pModule->GetLinearPitch() ? 1 : 0);

	// Namco channel count
	CSliderCtrl *pChanSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_CHANNELS));
	CStatic *pChannelsLabel = (CStatic*)GetDlgItem(IDC_CHANNELS_NR);
	pChanSlider->SetRange(1, MAX_CHANNELS_N163);		// // //

	CString channelsStr;
	channelsStr.LoadString(IDS_PROPERTIES_CHANNELS);
	if (m_iExpansions.ContainsChip(sound_chip_t::N163)) {
		m_iN163Channels = m_pModule->GetNamcoChannels();

		pChanSlider->SetPos(m_iN163Channels);
		pChanSlider->EnableWindow(TRUE);
		pChannelsLabel->EnableWindow(TRUE);
		channelsStr.AppendFormat(_T(" %i"), m_iN163Channels);
	}
	else {
		m_iN163Channels = 1;

		pChanSlider->SetPos(0);
		pChanSlider->EnableWindow(FALSE);
		channelsStr.Append(_T(" N/A"));
	}
	SetDlgItemText(IDC_CHANNELS_NR, channelsStr);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CModulePropertiesDlg::OnBnClickedOk()
{
	CMainFrame *pMainFrame = static_cast<CMainFrame*>(GetParentFrame());

	if (!m_iExpansions.ContainsChip(sound_chip_t::N163)) m_iN163Channels = 0;
	if (m_pModule->GetNamcoChannels() != m_iN163Channels || m_pModule->GetSoundChipSet() != m_iExpansions) {		// // //
		m_pDocument->SelectExpansionChip(m_iExpansions, m_iN163Channels);
		m_pDocument->ModifyIrreversible();
		m_pDocument->UpdateAllViews(NULL, UPDATE_PROPERTIES);
	}

	// Vibrato
	vibrato_t newVib = static_cast<CComboBox*>(GetDlgItem(IDC_VIBRATO))->GetCurSel() == 0 ? VIBRATO_NEW : VIBRATO_OLD;		// // //
	bool newLinear = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_LINEARPITCH))->GetCurSel() == 1;
	if (newVib != m_pModule->GetVibratoStyle() || newLinear != m_pModule->GetLinearPitch())
		m_pDocument->ModifyIrreversible();
	m_pModule->SetVibratoStyle(newVib);
	m_pModule->SetLinearPitch(newLinear);

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

	static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST))->InsertItem(NewTrack, GetSongString(NewTrack));

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

	auto pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));
	pSongList->InsertItem(NewTrack, GetSongString(NewTrack));
	m_pModule->VisitSongs([&] (const CSongData &, unsigned index) {
		pSongList->SetItemText(index, 0, GetSongString(index));
	});

	SelectSong(NewTrack);
}

void CModulePropertiesDlg::OnBnClickedSongRemove()
{
	ASSERT(m_iSelectedSong != -1);

	CListCtrl *pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));
	CMainFrame *pMainFrame = static_cast<CMainFrame*>(GetParentFrame());
	unsigned Count = m_pModule->GetSongCount();

	int SelCount = pSongList->GetSelectedCount();		// // //
	if (Count <= static_cast<unsigned>(SelCount))
		return; // Single track

	// Display warning first
	if (AfxMessageBox(IDS_SONG_DELETE, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO)
		return;

	for (unsigned i = Count - 1; i < Count; --i)		// // //
		if (pSongList->GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED) {
			if (m_iSelectedSong > i)
				--m_iSelectedSong;
			pSongList->DeleteItem(i);
			m_pModule->RemoveSong(i);
		}
	m_pDocument->ModifyIrreversible();		// // //
	m_pDocument->UpdateAllViews(NULL, UPDATE_TRACK);

	Count = m_pModule->GetSongCount();	// Get new track count

	// Redraw track list
	m_pModule->VisitSongs([&] (const CSongData &song, unsigned index) {
		pSongList->SetItemText(index, 0, GetSongString(index));		// // //
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

	CListCtrl *pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));
	pSongList->SetItemText(Song, 0, GetSongString(Song));
	pSongList->SetItemText(Song - 1, 0, GetSongString(Song - 1));

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

	CListCtrl *pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));
	pSongList->SetItemText(Song, 0, GetSongString(Song));
	pSongList->SetItemText(Song + 1, 0, GetSongString(Song + 1));

	SelectSong(Song + 1);
}

void CModulePropertiesDlg::OnEnChangeSongname()
{
	CListCtrl *pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));
	CEdit *pName = static_cast<CEdit*>(GetDlgItem(IDC_SONGNAME));
	CString Text;

	if (m_iSelectedSong == -1 || !m_bSingleSelection)
		return;

	pName->GetWindowText(Text);

	CSongData *pSong = m_pModule->GetSong(m_iSelectedSong);		// // //
	if (pSong->GetTitle() != (LPCTSTR)Text)		// // //
		m_pDocument->ModifyIrreversible();
	pSong->SetTitle((LPCTSTR)Text);
	pSongList->SetItemText(m_iSelectedSong, 0, GetSongString(m_iSelectedSong));
	m_pDocument->UpdateAllViews(NULL, UPDATE_TRACK);
}

void CModulePropertiesDlg::SelectSong(int Song)
{
	CListCtrl *pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));
	ASSERT(Song >= 0);

	for (int i = pSongList->GetItemCount() - 1; i >= 0; --i)		// // //
		pSongList->SetItemState(i, i == Song ? (LVIS_SELECTED | LVIS_FOCUSED) : 0, LVIS_SELECTED | LVIS_FOCUSED);
	m_iSelectedSong = Song;
	m_bSingleSelection = true;
	UpdateSongButtons();
	pSongList->EnsureVisible(Song, FALSE);
	pSongList->SetFocus();
}

void CModulePropertiesDlg::UpdateSongButtons()
{
	unsigned TrackCount = m_pModule->GetSongCount();
	bool Empty = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST))->GetSelectedCount() == 0;

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

	// TODO use string table
	CFileDialog OpenFileDlg(TRUE, _T("0cc"), 0, OFN_HIDEREADONLY,
							_T("0CC-FamiTracker modules (*.0cc;*.ftm)|*.0cc; *.ftm|All files (*.*)|*.*||"),		// // //
							AfxGetMainWnd(), 0);

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
	((CButton*)GetDlgItem(IDC_EXPANSION_VRC6))->SetCheck(m_iExpansions.ContainsChip(sound_chip_t::VRC6));
	((CButton*)GetDlgItem(IDC_EXPANSION_VRC7))->SetCheck(m_iExpansions.ContainsChip(sound_chip_t::VRC7));
	((CButton*)GetDlgItem(IDC_EXPANSION_FDS ))->SetCheck(m_iExpansions.ContainsChip(sound_chip_t::FDS ));
	((CButton*)GetDlgItem(IDC_EXPANSION_MMC5))->SetCheck(m_iExpansions.ContainsChip(sound_chip_t::MMC5));
	((CButton*)GetDlgItem(IDC_EXPANSION_N163))->SetCheck(m_iExpansions.ContainsChip(sound_chip_t::N163));
	((CButton*)GetDlgItem(IDC_EXPANSION_S5B ))->SetCheck(m_iExpansions.ContainsChip(sound_chip_t::S5B ));
	m_pDocument->UpdateAllViews(NULL, UPDATE_PROPERTIES);
}
/*
void CModulePropertiesDlg::OnCbnSelchangeExpansion()
{
	CComboBox *pExpansionChipBox = static_cast<CComboBox*>(GetDlgItem(IDC_EXPANSION));
	CSliderCtrl *pSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_CHANNELS));

	// Expansion chip
	unsigned int iExpansionChip = theApp.GetChannelMap()->GetChipIdent(pExpansionChipBox->GetCurSel());

	CString channelsStr;
	channelsStr.LoadString(IDS_PROPERTIES_CHANNELS);
	if (iExpansionChip == sound_chip_t::N163) {
		pSlider->EnableWindow(TRUE);
		int Channels = m_pDocument->GetNamcoChannels();
		pSlider->SetPos(Channels);
		channelsStr.AppendFormat(_T(" %i"), Channels);
	}
	else {
		pSlider->EnableWindow(FALSE);
		channelsStr.Append(_T(" N/A"));
	}
	SetDlgItemText(IDC_CHANNELS_NR, channelsStr);
}
*/
void CModulePropertiesDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CSliderCtrl *pSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_CHANNELS));

	m_iN163Channels = pSlider->GetPos();

	CString text;
	text.LoadString(IDS_PROPERTIES_CHANNELS);
	text.AppendFormat(_T(" %i"),  pSlider->GetPos());
	SetDlgItemText(IDC_CHANNELS_NR, text);

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CModulePropertiesDlg::OnLvnItemchangedSonglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if (pNMLV->uChanged & LVIF_STATE) {
		if (pNMLV->uNewState & LVNI_SELECTED) {		// // //
			m_iSelectedSong = pNMLV->iItem;
			GetDlgItem(IDC_SONGNAME)->SetWindowText(m_pModule->GetSong(m_iSelectedSong)->GetTitle().data());		// // //
		}
		CListCtrl *pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));
		m_bSingleSelection = pSongList->GetSelectedCount() == 1;
		UpdateSongButtons();
	}

	*pResult = 0;
}

CString CModulePropertiesDlg::GetSongString(unsigned index) const {		// // //
	CString Text;
	const auto *pSong = m_pModule->GetSong(index);
	std::string_view sv = pSong ? pSong->GetTitle() : "(N/A)";
	Text.Format(TRACK_FORMAT, index + 1, sv.size(), sv.data());		// // // start counting songs from 1
	return Text;
}

void CModulePropertiesDlg::FillSongList()
{
	CListCtrl *pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));
	CString Text;

	pSongList->DeleteAllItems();

	// Song editor
	int Songs = m_pModule->GetSongCount();

	m_pModule->VisitSongs([&] (const CSongData &song, unsigned index) {
		auto sv = song.GetTitle();
		Text.Format(TRACK_FORMAT, index + 1, sv.size(), sv.data());		// // // start counting songs from 1
		pSongList->InsertItem(index, Text);
	});
}

BOOL CModulePropertiesDlg::PreTranslateMessage(MSG* pMsg)
{
	CListCtrl *pSongList = static_cast<CListCtrl*>(GetDlgItem(IDC_SONGLIST));

	if (GetFocus() == pSongList) {
		if(pMsg->message == WM_KEYDOWN) {
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
	CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_VRC6);
	m_iExpansions = m_iExpansions.EnableChip(sound_chip_t::VRC6, pCheckBox->GetCheck() == BST_CHECKED);		// // //
}

void CModulePropertiesDlg::OnBnClickedExpansionVRC7()
{
	CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_VRC7);
	m_iExpansions = m_iExpansions.EnableChip(sound_chip_t::VRC7, pCheckBox->GetCheck() == BST_CHECKED);		// // //
}

void CModulePropertiesDlg::OnBnClickedExpansionFDS()
{
	CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_FDS);
	m_iExpansions = m_iExpansions.EnableChip(sound_chip_t::FDS, pCheckBox->GetCheck() == BST_CHECKED);		// // //
}

void CModulePropertiesDlg::OnBnClickedExpansionMMC5()
{
	CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_MMC5);
	m_iExpansions = m_iExpansions.EnableChip(sound_chip_t::MMC5, pCheckBox->GetCheck() == BST_CHECKED);		// // //
}

void CModulePropertiesDlg::OnBnClickedExpansionS5B()
{
	CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_S5B);
	m_iExpansions = m_iExpansions.EnableChip(sound_chip_t::S5B, pCheckBox->GetCheck() == BST_CHECKED);		// // //
}

void CModulePropertiesDlg::OnBnClickedExpansionN163()
{
	CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_N163);

	CSliderCtrl *pChanSlider = (CSliderCtrl*)GetDlgItem(IDC_CHANNELS);
	CStatic *pChannelsLabel = (CStatic*)GetDlgItem(IDC_CHANNELS_NR);
	CString channelsStr;
	channelsStr.LoadString(IDS_PROPERTIES_CHANNELS);

	// Expansion chip
	if (pCheckBox->GetCheck() == BST_CHECKED) {
		m_iExpansions = m_iExpansions.WithChip(sound_chip_t::N163);

		if (!m_iN163Channels)
			m_iN163Channels = 1;		// // //
		pChanSlider->SetPos(m_iN163Channels);
		pChanSlider->EnableWindow(TRUE);
		pChannelsLabel->EnableWindow(TRUE);
		channelsStr.AppendFormat(_T(" %i"), m_iN163Channels);
	}
	else {
		m_iExpansions = m_iExpansions.WithoutChip(sound_chip_t::N163);

		pChanSlider->SetPos(m_iN163Channels = 0);
		pChanSlider->EnableWindow(FALSE);
		pChannelsLabel->EnableWindow(FALSE);
		channelsStr.Append(_T(" N/A"));
	}
	SetDlgItemText(IDC_CHANNELS_NR, channelsStr);
}

void CModulePropertiesDlg::OnCbnSelchangeComboLinearpitch()
{
	static bool First = true;
	if (First) {
		First = false;
		AfxMessageBox(_T(
			"Because linear pitch mode is a planned feature in the official build, "
			"changes to this setting might not be reflected when the current module is loaded from "
			"a future official release that implements this feature."
		), MB_OK | MB_ICONINFORMATION);
	}
}
