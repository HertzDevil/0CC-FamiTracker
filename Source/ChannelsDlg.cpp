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

#include "ChannelsDlg.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"		// // //
#include "SongView.h"		// // //
#include "ChannelOrder.h"		// // //
#include "TrackerChannel.h"
#include "APU/Types.h"		// // //
#include "FamiTrackerEnv.h"		// // //
#include "SoundChipService.h"		// // //
#include "str_conv/str_conv.hpp"		// // //

// CChannelsDlg dialog

IMPLEMENT_DYNAMIC(CChannelsDlg, CDialog)

CChannelsDlg::CChannelsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CChannelsDlg::IDD, pParent)
{

}

CChannelsDlg::~CChannelsDlg()
{
}

void CChannelsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CChannelsDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_NOTIFY(NM_CLICK, IDC_AVAILABLE_TREE, OnClickAvailable)
	ON_NOTIFY(NM_DBLCLK, IDC_AVAILABLE_TREE, OnDblClickAvailable)
	ON_NOTIFY(NM_DBLCLK, IDC_ADDED_LIST, OnDblClickAdded)
	ON_BN_CLICKED(IDC_MOVE_DOWN, OnBnClickedMoveDown)
	ON_NOTIFY(NM_RCLICK, IDC_AVAILABLE_TREE, OnNMRClickAvailableTree)
	ON_BN_CLICKED(IDC_MOVE_UP, OnBnClickedMoveUp)
END_MESSAGE_MAP()

// CChannelsDlg message handlers

BOOL CChannelsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_cAvailableTree.SubclassDlgItem(IDC_AVAILABLE_TREE, this);		// // //
	m_cAddedChannels.SubclassDlgItem(IDC_ADDED_LIST, this);

	CRect rect;		// // //
	m_cAddedChannels.GetClientRect(rect);
	m_cAddedChannels.InsertColumn(0, L"Name", 0, rect.Width());

	auto *pSCS = Env.GetSoundChipService();		// // //
	pSCS->ForeachType([&] (sound_chip_t ch) {
		HTREEITEM hItem = m_cAvailableTree.InsertItem(conv::to_wide(pSCS->GetChipFullName(ch)).data());
		m_cAvailableTree.SetItemData(hItem, value_cast(ch));
		for (std::size_t subindex = 0, n = pSCS->GetSupportedChannelCount(ch); subindex < n; ++subindex) {
			auto chan = pSCS->MakeChannelIndex(ch, subindex);
			HTREEITEM hChild = m_cAvailableTree.InsertItem(FormattedW(L"%u: %s", subindex, conv::to_wide(pSCS->GetChannelFullName(chan)).data()), hItem);
			m_cAvailableTree.SetItemData(hChild, value_cast(chan_id_t {chan}));
		}
		m_cAvailableTree.SortChildren(hItem);
	});

	CFamiTrackerView::GetView()->GetSongView()->GetChannelOrder().ForeachChannel([&] (stChannelID ch) {
		for (HTREEITEM hChip = m_cAvailableTree.GetRootItem(); hChip; hChip = m_cAvailableTree.GetNextItem(hChip, TVGN_NEXT)) {
			for (HTREEITEM hItem = m_cAvailableTree.GetNextItem(hChip, TVGN_CHILD); hItem; hItem = m_cAvailableTree.GetNextItem(hItem, TVGN_NEXT)) {
				if (enum_cast<chan_id_t>(m_cAvailableTree.GetItemData(hItem)) == chan_id_t {ch}) {		// // //
					InsertChannel(hItem);
					return;
				}
			}
		}
	});

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CChannelsDlg::OnBnClickedOk() {		// // //
	if (!m_cAddedChannels.GetItemCount()) {
		AfxMessageBox(L"At least one track must be added.", MB_ICONERROR);
		return;
	}

	OnOK();
}

void CChannelsDlg::OnClickAvailable(NMHDR *pNMHDR, LRESULT *result)
{

}

void CChannelsDlg::OnDblClickAvailable(NMHDR *pNMHDR, LRESULT *result)
{
	// Add channel/chip

	m_cAvailableTree.SetRedraw(FALSE);
	m_cAddedChannels.SetRedraw(FALSE);

	if (HTREEITEM hItem = m_cAvailableTree.GetSelectedItem()) {		// // //
		if (!m_cAvailableTree.GetNextItem(hItem, TVGN_PARENT)) {
			if (m_cAvailableTree.ItemHasChildren(hItem)) {
				while (m_cAvailableTree.ItemHasChildren(hItem))
					InsertChannel(m_cAvailableTree.GetNextItem(hItem, TVGN_CHILD));
			}
			else {
				auto Chip = enum_cast<sound_chip_t>(m_cAvailableTree.GetItemData(hItem));
				auto *pSCS = Env.GetSoundChipService();		// // //
				int i = 0;
				while (i < m_cAddedChannels.GetItemCount())
					if (stChannelID {enum_cast<chan_id_t>(m_cAddedChannels.GetItemData(i))}.Chip == Chip)
						RemoveChannel(i);
					else
						++i;
			}
		}
		else
			InsertChannel(hItem);
	}

	m_cAvailableTree.SetRedraw(TRUE);
	m_cAddedChannels.SetRedraw(TRUE);
	*result = 0;
}

void CChannelsDlg::OnDblClickAdded(NMHDR *pNMHDR, LRESULT *result)
{
	if (int Index = m_cAddedChannels.GetSelectionMark(); Index != -1)		// // //
		RemoveChannel(Index);
}

void CChannelsDlg::InsertChannel(HTREEITEM hItem) {
	auto *pSCS = Env.GetSoundChipService();		// // //

	if (HTREEITEM hParentItem = m_cAvailableTree.GetParentItem(hItem)) {
		auto iData = m_cAvailableTree.GetItemData(hItem);
		auto ChanId = stChannelID {enum_cast<chan_id_t>(iData)};

		auto AddStr = std::string {pSCS->GetChipShortName(ChanId.Chip)};
		AddStr += " :: ";
		AddStr += pSCS->GetChannelFullName(ChanId);

		int ChansAdded = m_cAddedChannels.GetItemCount();
		int Index = m_cAddedChannels.InsertItem(ChansAdded, conv::to_wide(AddStr).data());

		m_cAddedChannels.SetItemData(Index, iData);

		// Remove channel from available list
		m_cAvailableTree.DeleteItem(hItem);
	}
}

void CChannelsDlg::RemoveChannel(int nId) {		// // //
	auto *pSCS = Env.GetSoundChipService();		// // //
	auto iData = m_cAddedChannels.GetItemData(nId);
	auto ChanID = stChannelID {enum_cast<chan_id_t>(iData)};

	// Put back in available list
	for (HTREEITEM hChip = m_cAvailableTree.GetRootItem(); hChip; hChip = m_cAvailableTree.GetNextItem(hChip, TVGN_NEXT)) {
		if (enum_cast<sound_chip_t>(m_cAvailableTree.GetItemData(hChip)) == ChanID.Chip) {
			HTREEITEM hChild = m_cAvailableTree.InsertItem(FormattedW(L"%u: %s", ChanID.Subindex, conv::to_wide(pSCS->GetChannelFullName(ChanID)).data()), hChip);
			m_cAvailableTree.SetItemData(hChild, iData);
			m_cAvailableTree.Expand(hChip, TVE_EXPAND);
			m_cAvailableTree.SortChildren(hChip);
			m_cAddedChannels.DeleteItem(nId);
			return;
		}
	}
}

void CChannelsDlg::OnBnClickedMoveDown()
{
	int Index = m_cAddedChannels.GetSelectionMark();

	if (Index >= m_cAddedChannels.GetItemCount() - 1 || Index == -1)
		return;

	CStringW text = m_cAddedChannels.GetItemText(Index, 0);
	int data = m_cAddedChannels.GetItemData(Index);

	m_cAddedChannels.SetItemText(Index, 0, m_cAddedChannels.GetItemText(Index + 1, 0));
	m_cAddedChannels.SetItemData(Index, m_cAddedChannels.GetItemData(Index + 1));

	m_cAddedChannels.SetItemText(Index + 1, 0, text);
	m_cAddedChannels.SetItemData(Index + 1, data);

	m_cAddedChannels.SetSelectionMark(Index + 1);
	m_cAddedChannels.SetItemState(Index + 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_cAddedChannels.EnsureVisible(Index + 1, FALSE);
}

void CChannelsDlg::OnBnClickedMoveUp()
{
	int Index = m_cAddedChannels.GetSelectionMark();

	if (Index == 0 || Index == -1)
		return;

	CStringW text = m_cAddedChannels.GetItemText(Index, 0);
	int data = m_cAddedChannels.GetItemData(Index);

	m_cAddedChannels.SetItemText(Index, 0, m_cAddedChannels.GetItemText(Index - 1, 0));
	m_cAddedChannels.SetItemData(Index, m_cAddedChannels.GetItemData(Index - 1));

	m_cAddedChannels.SetItemText(Index - 1, 0, text);
	m_cAddedChannels.SetItemData(Index - 1, data);

	m_cAddedChannels.SetSelectionMark(Index - 1);
	m_cAddedChannels.SetItemState(Index - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_cAddedChannels.EnsureVisible(Index - 1, FALSE);
}

void CChannelsDlg::OnNMRClickAvailableTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: Add your control notification handler code here
	*pResult = 0;
}
