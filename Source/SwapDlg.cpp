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

#include "SwapDlg.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"
#include "SoundChipSet.h"
#include "FamiTrackerViewMessage.h"
#include "SongData.h"
#include "APU/Types.h"

// CSwapDlg dialog

IMPLEMENT_DYNAMIC(CSwapDlg, CDialog)

CSwapDlg::CSwapDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_SWAP, pParent)
{
}

void CSwapDlg::SetTrack(unsigned int Track)
{
	m_iTrack = Track;
}

void CSwapDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSwapDlg, CDialog)
	ON_EN_CHANGE(IDC_EDIT_SWAP_CHAN1, OnEnChangeEditSwapChan1)
	ON_EN_CHANGE(IDC_EDIT_SWAP_CHAN2, OnEnChangeEditSwapChan2)
	ON_CBN_SELCHANGE(IDC_COMBO_SWAP_CHIP1, OnCbnSelchangeComboSwapChip1)
	ON_CBN_SELCHANGE(IDC_COMBO_SWAP_CHIP2, OnCbnSelchangeComboSwapChip2)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CSwapDlg message handlers

BOOL CSwapDlg::OnInitDialog()
{
	m_cChannelFirst.SubclassDlgItem(IDC_EDIT_SWAP_CHAN1, this);
	m_cChannelSecond.SubclassDlgItem(IDC_EDIT_SWAP_CHAN2, this);
	m_cChipFirst.SubclassDlgItem(IDC_COMBO_SWAP_CHIP1, this);
	m_cChipSecond.SubclassDlgItem(IDC_COMBO_SWAP_CHIP2, this);

	const auto &chips = CFamiTrackerDoc::GetDoc()->GetModule()->GetSoundChipSet();
	if (chips.ContainsChip(sound_chip_t::APU))
		m_cChipFirst.AddString(_T("2A03"));
	if (chips.ContainsChip(sound_chip_t::VRC6))
		m_cChipFirst.AddString(_T("VRC6"));
	if (chips.ContainsChip(sound_chip_t::VRC7))
		m_cChipFirst.AddString(_T("VRC7"));
	if (chips.ContainsChip(sound_chip_t::FDS))
		m_cChipFirst.AddString(_T("FDS"));
	if (chips.ContainsChip(sound_chip_t::MMC5))
		m_cChipFirst.AddString(_T("MMC5"));
	if (chips.ContainsChip(sound_chip_t::N163))
		m_cChipFirst.AddString(_T("N163"));
	if (chips.ContainsChip(sound_chip_t::S5B))
		m_cChipFirst.AddString(_T("5B"));

	CString str;
	for (int i = 0; i < m_cChipFirst.GetCount(); i++)
	{
	   m_cChipFirst.GetLBText(i, str);
	   m_cChipSecond.AddString(str);
	}
	m_cChannelFirst.SetWindowText(_T("1"));
	m_cChannelSecond.SetWindowText(_T("2"));
	m_cChipFirst.SetCurSel(0);
	m_cChipSecond.SetCurSel(0);
	CheckDlgButton(IDC_CHECK_SWAP_ALL, BST_UNCHECKED);

	m_cChannelFirst.SetFocus();

	return CDialog::OnInitDialog();
}

void CSwapDlg::CheckDestination() const
{
	GetDlgItem(IDOK)->EnableWindow(MakeChannelIndex(m_iDestChip1, m_iDestChannel1) != chan_id_t::NONE &&
								   MakeChannelIndex(m_iDestChip2, m_iDestChannel2) != chan_id_t::NONE &&
								   (m_iDestChannel1 != m_iDestChannel2 || m_iDestChip1 != m_iDestChip2));
}

sound_chip_t CSwapDlg::GetChipFromString(const CString &str)
{
	if (str == _T("2A03"))
		return sound_chip_t::APU;
	if (str == _T("VRC6"))
		return sound_chip_t::VRC6;
	if (str == _T("VRC7"))
		return sound_chip_t::VRC7;
	if (str == _T("FDS"))
		return sound_chip_t::FDS;
	if (str == _T("MMC5"))
		return sound_chip_t::MMC5;
	if (str == _T("N163"))
		return sound_chip_t::N163;
	if (str == _T("5B"))
		return sound_chip_t::S5B;
	return sound_chip_t::NONE;
}

void CSwapDlg::OnEnChangeEditSwapChan1()
{
	CString str;
	m_cChannelFirst.GetWindowText(str);
	m_iDestChannel1 = atoi(str) - 1;
	CheckDestination();
}

void CSwapDlg::OnEnChangeEditSwapChan2()
{
	CString str;
	m_cChannelSecond.GetWindowText(str);
	m_iDestChannel2 = atoi(str) - 1;
	CheckDestination();
}

void CSwapDlg::OnCbnSelchangeComboSwapChip1()
{
	CString str;
	m_cChipFirst.GetWindowText(str);
	m_iDestChip1 = GetChipFromString(str);
	CheckDestination();
}

void CSwapDlg::OnCbnSelchangeComboSwapChip2()
{
	CString str;
	m_cChipSecond.GetWindowText(str);
	m_iDestChip2 = GetChipFromString(str);
	CheckDestination();
}

void CSwapDlg::OnBnClickedOk()
{
	auto lhs = MakeChannelIndex(m_iDestChip1, m_iDestChannel1);
	auto rhs = MakeChannelIndex(m_iDestChip2, m_iDestChannel2);

	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	CFamiTrackerModule *pModule = pDoc->GetModule();
	if (IsDlgButtonChecked(IDC_CHECK_SWAP_ALL) == BST_CHECKED)
		pModule->VisitSongs([lhs, rhs] (CSongData &song) {
			song.SwapChannels(lhs, rhs);
		});
	else
		pModule->GetSong(m_iTrack)->SwapChannels(lhs, rhs);

	pDoc->ModifyIrreversible();
	pDoc->UpdateAllViews(NULL, UPDATE_PATTERN);
	pDoc->UpdateAllViews(NULL, UPDATE_FRAME);
	pDoc->UpdateAllViews(NULL, UPDATE_COLUMNS);

	CDialog::OnOK();
}
