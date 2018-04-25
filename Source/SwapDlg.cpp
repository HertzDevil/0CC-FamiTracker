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
#include "FamiTrackerEnv.h"
#include "SoundChipService.h"
#include "SoundChipSet.h"
#include "FamiTrackerViewMessage.h"
#include "SongData.h"
#include "APU/Types.h"
#include "str_conv/str_conv.hpp"
#include "NumConv.h"

// CSwapDlg dialog

IMPLEMENT_DYNAMIC(CSwapDlg, CDialog)

CSwapDlg::CSwapDlg(CWnd* pParent /*=NULL*/) :
	CDialog(IDD_SWAP, pParent),
	m_iDestChip1(sound_chip_t::APU),
	m_iDestChip2(sound_chip_t::APU)
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
	m_cSubindexFirst.SubclassDlgItem(IDC_EDIT_SWAP_CHAN1, this);
	m_cSubindexSecond.SubclassDlgItem(IDC_EDIT_SWAP_CHAN2, this);
	m_cChipFirst.SubclassDlgItem(IDC_COMBO_SWAP_CHIP1, this);
	m_cChipSecond.SubclassDlgItem(IDC_COMBO_SWAP_CHIP2, this);

	const auto &chips = CFamiTrackerDoc::GetDoc()->GetModule()->GetSoundChipSet();
	int i = 0;
	Env.GetSoundChipService()->ForeachType([&] (sound_chip_t ch) {
		if (chips.ContainsChip(ch)) {
			auto wstr = conv::to_wide(Env.GetSoundChipService()->GetChipShortName(ch));
			m_cChipFirst.AddString(wstr.data());
			m_cChipSecond.AddString(wstr.data());
			m_cChipFirst.SetItemData(i, value_cast(ch));
			m_cChipSecond.SetItemData(i, value_cast(ch));
			++i;
		}
	});

	m_cSubindexFirst.SetWindowTextW(L"1");
	m_cSubindexSecond.SetWindowTextW(L"2");
	m_cChipFirst.SetCurSel(0);
	m_cChipSecond.SetCurSel(0);
	CheckDlgButton(IDC_CHECK_SWAP_ALL, BST_UNCHECKED);

	m_cSubindexFirst.SetFocus();

	return CDialog::OnInitDialog();
}

void CSwapDlg::CheckDestination() const
{
	auto *pSCS = Env.GetSoundChipService();

	GetDlgItem(IDOK)->EnableWindow(
		m_iDestChip1 != sound_chip_t::none && m_iDestChip2 != sound_chip_t::none &&
		m_iDestChannel1 < pSCS->GetSupportedChannelCount(m_iDestChip1) &&
		m_iDestChannel2 < pSCS->GetSupportedChannelCount(m_iDestChip2) &&
		(m_iDestChannel1 != m_iDestChannel2 || m_iDestChip1 != m_iDestChip2));
}

void CSwapDlg::OnEnChangeEditSwapChan1()
{
	CStringW str;
	m_cSubindexFirst.GetWindowTextW(str);
	m_iDestChannel1 = conv::to_uint(str).value_or(0) - 1;
	CheckDestination();
}

void CSwapDlg::OnEnChangeEditSwapChan2()
{
	CStringW str;
	m_cSubindexSecond.GetWindowTextW(str);
	m_iDestChannel2 = conv::to_uint(str).value_or(0) - 1;
	CheckDestination();
}

void CSwapDlg::OnCbnSelchangeComboSwapChip1()
{
	m_iDestChip1 = enum_cast<sound_chip_t>(m_cChipFirst.GetItemData(m_cChipFirst.GetCurSel()));
	CheckDestination();
}

void CSwapDlg::OnCbnSelchangeComboSwapChip2()
{
	m_iDestChip2 = enum_cast<sound_chip_t>(m_cChipSecond.GetItemData(m_cChipSecond.GetCurSel()));
	CheckDestination();
}

void CSwapDlg::OnBnClickedOk()
{
	auto lhs = stChannelID {m_iDestChip1, static_cast<std::uint8_t>(m_iDestChannel1)};
	auto rhs = stChannelID {m_iDestChip2, static_cast<std::uint8_t>(m_iDestChannel2)};

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
