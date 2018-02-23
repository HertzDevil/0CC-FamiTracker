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

#include "ConfigGeneral.h"
#include "FamiTracker.h"
#include "Settings.h"

const LPCWSTR CConfigGeneral::CONFIG_STR[] = {		// // //
	L"Wrap cursor",
	L"Wrap across frames",
	L"Free cursor edit",
	L"Preview wave-files",
	L"Key repeat",
	L"Show row numbers in hex",
	L"Preview next/previous frames",
	L"Don't reset DPCM on note stop",
	L"Ignore Step when moving",
	L"Delete-key pulls up rows",
	L"Backup files",
	L"Single instance",
	L"Preview full row",
	L"Don't select on double-click",
	L"Warp pattern values",
	L"Cut sub-volume",
	L"Use old FDS volume table",
	L"Retrieve channel state",
	L"Overflow paste mode",
	L"Show skipped rows",
	L"Hexadecimal keypad",
	L"Multi-frame selection",
	L"Check version on startup",
};

const LPCWSTR CConfigGeneral::CONFIG_DESC[] = {		// // //
	L"Wrap the cursor around the edges of the pattern editor.",
	L"Move to next or previous frame when reaching top or bottom in the pattern editor.",
	L"Unlock the cursor from the center of the pattern editor.",
	L"Preview wave and DPCM files in the open file dialog when loading samples to the module.",
	L"Enable key repetition in the pattern editor.",
	L"Display row numbers and the frame count on the status bar in hexadecimal.",
	L"Preview next and previous frames in the pattern editor.",
	L"Prevent resetting the DPCM channel after previewing any DPCM sample.",
	L"Ignore the pattern step setting when moving the cursor, only use it when inserting notes.",
	L"Make delete key pull up rows rather than only deleting the value, as if by Shift+Delete.",
	L"Create a backup copy of the existing file when saving a module.",
	L"Only allow one single instance of the 0CC-FamiTracker application.",
	L"Preview all channels when inserting notes in the pattern editor.",
	L"Do not select the whole channel when double-clicking in the pattern editor.",
	L"When using Shift + Mouse Wheel to modify a pattern value, allow the parameter to wrap around its limit values.",
	L"Always silent volume values below 1 due to Axy or 7xy effects.",
	L"Use the existing volume table for the FDS channel which has higher precision than in exported NSFs.",
	L"Reconstruct the current channel's state from previous frames upon playing (except when playing one row).",
	L"Move pasted pattern data outside the rows of the current frame to subsequent frames.",
	L"Display rows that are truncated by Bxx, Cxx, or Dxx effects.",
	L"Use the extra keys on the keypad as hexadecimal digits in the pattern editor.",
	L"Allow pattern selections to span across multiple frames.",
	L"Check for new 0CC-FamiTracker versions on startup if an internet connection could be established.",
};

// CConfigGeneral dialog

IMPLEMENT_DYNAMIC(CConfigGeneral, CPropertyPage)
CConfigGeneral::CConfigGeneral()
	: CPropertyPage(CConfigGeneral::IDD)
{
}

CConfigGeneral::~CConfigGeneral()
{
}

void CConfigGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CConfigGeneral, CPropertyPage)
	ON_CBN_EDITUPDATE(IDC_PAGELENGTH, OnCbnEditupdatePagelength)
	ON_CBN_SELENDOK(IDC_PAGELENGTH, OnCbnSelendokPagelength)
	ON_CBN_SELCHANGE(IDC_COMBO_STYLE, OnCbnSelchangeComboStyle)
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_CONFIG_LIST, OnLvnItemchangedConfigList)
END_MESSAGE_MAP()


// CConfigGeneral message handlers

BOOL CConfigGeneral::OnSetActive()
{
	SetDlgItemInt(IDC_PAGELENGTH, m_iPageStepSize, FALSE);
	static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_STYLE))->SetCurSel(m_iEditStyle);		// // //

	return CPropertyPage::OnSetActive();
}

void CConfigGeneral::OnOK()
{
	CPropertyPage::OnOK();
}

BOOL CConfigGeneral::OnApply()
{
	// Translate page length
	BOOL Trans;

	m_iPageStepSize = GetDlgItemInt(IDC_PAGELENGTH, &Trans, FALSE);

	if (Trans == FALSE)
		m_iPageStepSize = 4;
	else if (m_iPageStepSize > 256 /*MAX_PATTERN_LENGTH*/)
		m_iPageStepSize = 256 /*MAX_PATTERN_LENGTH*/;

	if (m_bCheckVersion && !theApp.GetSettings()->General.bCheckVersion)		// // //
		theApp.CheckNewVersion(false);

	auto *pSettings = theApp.GetSettings();		// // //
	pSettings->General.bWrapCursor			= m_bWrapCursor;
	pSettings->General.bWrapFrames			= m_bWrapFrames;
	pSettings->General.bFreeCursorEdit		= m_bFreeCursorEdit;
	pSettings->General.bWavePreview			= m_bPreviewWAV;
	pSettings->General.bKeyRepeat			= m_bKeyRepeat;
	pSettings->General.bRowInHex			= m_bRowInHex;
	pSettings->General.iEditStyle			= m_iEditStyle;
	pSettings->General.bFramePreview		= m_bFramePreview;
	pSettings->General.bNoDPCMReset			= m_bNoDPCMReset;
	pSettings->General.bNoStepMove			= m_bNoStepMove;
	pSettings->General.iPageStepSize		= m_iPageStepSize;
	pSettings->General.bPullUpDelete		= m_bPullUpDelete;
	pSettings->General.bBackups				= m_bBackups;
	pSettings->General.bSingleInstance		= m_bSingleInstance;
	pSettings->General.bPreviewFullRow		= m_bPreviewFullRow;
	pSettings->General.bDblClickSelect		= m_bDisableDblClick;
	// // //
	pSettings->General.bWrapPatternValue	= m_bWrapPatternValue;
	pSettings->General.bCutVolume			= m_bCutVolume;
	pSettings->General.bFDSOldVolume		= m_bFDSOldVolume;
	pSettings->General.bRetrieveChanState	= m_bRetrieveChanState;
	pSettings->General.bOverflowPaste		= m_bOverflowPaste;
	pSettings->General.bShowSkippedRows		= m_bShowSkippedRows;
	pSettings->General.bHexKeypad			= m_bHexKeypad;
	pSettings->General.bMultiFrameSel		= m_bMultiFrameSel;
	pSettings->General.bCheckVersion		= m_bCheckVersion;

	pSettings->Keys.iKeyNoteCut				= m_iKeyNoteCut;
	pSettings->Keys.iKeyNoteRelease			= m_iKeyNoteRelease;
	pSettings->Keys.iKeyClear				= m_iKeyClear;
	pSettings->Keys.iKeyRepeat				= m_iKeyRepeat;
	pSettings->Keys.iKeyEchoBuffer			= m_iKeyEchoBuffer;		// // //

	return CPropertyPage::OnApply();
}

BOOL CConfigGeneral::OnInitDialog()
{
	WCHAR Text[64] = { };

	CPropertyPage::OnInitDialog();

	const auto *pSettings = theApp.GetSettings();		// // //

	m_bWrapCursor			= pSettings->General.bWrapCursor;
	m_bWrapFrames			= pSettings->General.bWrapFrames;
	m_bFreeCursorEdit		= pSettings->General.bFreeCursorEdit;
	m_bPreviewWAV			= pSettings->General.bWavePreview;
	m_bKeyRepeat			= pSettings->General.bKeyRepeat;
	m_bRowInHex				= pSettings->General.bRowInHex;
	m_iEditStyle			= pSettings->General.iEditStyle;
	m_bFramePreview			= pSettings->General.bFramePreview;
	m_bNoDPCMReset			= pSettings->General.bNoDPCMReset;
	m_bNoStepMove			= pSettings->General.bNoStepMove;
	m_iPageStepSize			= pSettings->General.iPageStepSize;
	m_bPullUpDelete			= pSettings->General.bPullUpDelete;
	m_bBackups				= pSettings->General.bBackups;
	m_bSingleInstance		= pSettings->General.bSingleInstance;
	m_bPreviewFullRow		= pSettings->General.bPreviewFullRow;
	m_bDisableDblClick		= pSettings->General.bDblClickSelect;
	// // //
	m_bWrapPatternValue		= pSettings->General.bWrapPatternValue;
	m_bCutVolume			= pSettings->General.bCutVolume;
	m_bFDSOldVolume			= pSettings->General.bFDSOldVolume;
	m_bRetrieveChanState	= pSettings->General.bRetrieveChanState;
	m_bOverflowPaste		= pSettings->General.bOverflowPaste;
	m_bShowSkippedRows		= pSettings->General.bShowSkippedRows;
	m_bHexKeypad			= pSettings->General.bHexKeypad;
	m_bMultiFrameSel		= pSettings->General.bMultiFrameSel;
	m_bCheckVersion			= pSettings->General.bCheckVersion;

	m_iKeyNoteCut			= pSettings->Keys.iKeyNoteCut;
	m_iKeyNoteRelease		= pSettings->Keys.iKeyNoteRelease;
	m_iKeyClear				= pSettings->Keys.iKeyClear;
	m_iKeyRepeat			= pSettings->Keys.iKeyRepeat;
	m_iKeyEchoBuffer		= pSettings->Keys.iKeyEchoBuffer;		// // //

	GetKeyNameTextW(MapVirtualKey(m_iKeyNoteCut, MAPVK_VK_TO_VSC) << 16, Text, 64);
	SetDlgItemTextW(IDC_KEY_NOTE_CUT, Text);
	GetKeyNameTextW(MapVirtualKey(m_iKeyNoteRelease, MAPVK_VK_TO_VSC) << 16, Text, 64);
	SetDlgItemTextW(IDC_KEY_NOTE_RELEASE, Text);
	GetKeyNameTextW(MapVirtualKey(m_iKeyClear, MAPVK_VK_TO_VSC) << 16, Text, 64);
	SetDlgItemTextW(IDC_KEY_CLEAR, Text);
	GetKeyNameTextW(MapVirtualKey(m_iKeyRepeat, MAPVK_VK_TO_VSC) << 16, Text, 64);
	SetDlgItemTextW(IDC_KEY_REPEAT, Text);
	GetKeyNameTextW(MapVirtualKey(m_iKeyEchoBuffer, MAPVK_VK_TO_VSC) << 16, Text, 64);		// // //
	SetDlgItemTextW(IDC_KEY_ECHO_BUFFER, Text);

	EnableToolTips(TRUE);

	m_wndToolTip.Create(this, TTS_ALWAYSTIP);
	m_wndToolTip.Activate(TRUE);

	CWnd *pWndChild = GetWindow(GW_CHILD);
	CStringW strToolTip;

	while (pWndChild) {
		int nID = pWndChild->GetDlgCtrlID();
		if (strToolTip.LoadStringW(nID)) {
			m_wndToolTip.AddTool(pWndChild, strToolTip);
		}
		pWndChild = pWndChild->GetWindow(GW_HWNDNEXT);
	}

	const bool CONFIG_BOOL[SETTINGS_BOOL_COUNT] = {		// // //
		m_bWrapCursor,
		m_bWrapFrames,
		m_bFreeCursorEdit,
		m_bPreviewWAV,
		m_bKeyRepeat,
		m_bRowInHex,
		m_bFramePreview,
		m_bNoDPCMReset,
		m_bNoStepMove,
		m_bPullUpDelete,
		m_bBackups,
		m_bSingleInstance,
		m_bPreviewFullRow,
		m_bDisableDblClick,
		m_bWrapPatternValue,
		m_bCutVolume,
		m_bFDSOldVolume,
		m_bRetrieveChanState,
		m_bOverflowPaste,
		m_bShowSkippedRows,
		m_bHexKeypad,
		m_bMultiFrameSel,
		m_bCheckVersion,
	};

	CListCtrl *pList = static_cast<CListCtrl*>(GetDlgItem(IDC_CONFIG_LIST));
	CRect r;		// // //
	pList->GetClientRect(&r);
	pList->DeleteAllItems();
	pList->InsertColumn(0, L"", LVCFMT_LEFT, 20);
	pList->InsertColumn(1, L"Option", LVCFMT_LEFT, r.Width() - 20 - ::GetSystemMetrics(SM_CXHSCROLL));
	pList->SendMessageW(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
	pList->SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

	for (int i = SETTINGS_BOOL_COUNT - 1; i > -1; i--) {
		pList->InsertItem(0, L"", 0);
		pList->SetCheck(0, CONFIG_BOOL[i]);
		pList->SetItemText(0, 1, CONFIG_STR[i]);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigGeneral::OnCbnEditupdatePagelength()
{
	SetModified();
}

void CConfigGeneral::OnCbnSelendokPagelength()
{
	SetModified();
}

void CConfigGeneral::OnCbnSelchangeComboStyle()		// // //
{
	m_iEditStyle = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_STYLE))->GetCurSel();
	SetModified();
}

void CConfigGeneral::OnLvnItemchangedConfigList(NMHDR *pNMHDR, LRESULT *pResult)		// // //
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	CListCtrl *pList = static_cast<CListCtrl*>(GetDlgItem(IDC_CONFIG_LIST));

	auto mask = pNMLV->uNewState & pNMLV->uOldState;		// // // wine compatibility
	pNMLV->uNewState &= ~mask;
	pNMLV->uOldState &= ~mask;

	using T = bool (CConfigGeneral::*);
	static T CONFIG_BOOL[SETTINGS_BOOL_COUNT] = {		// // //
		&CConfigGeneral::m_bWrapCursor,
		&CConfigGeneral::m_bWrapFrames,
		&CConfigGeneral::m_bFreeCursorEdit,
		&CConfigGeneral::m_bPreviewWAV,
		&CConfigGeneral::m_bKeyRepeat,
		&CConfigGeneral::m_bRowInHex,
		&CConfigGeneral::m_bFramePreview,
		&CConfigGeneral::m_bNoDPCMReset,
		&CConfigGeneral::m_bNoStepMove,
		&CConfigGeneral::m_bPullUpDelete,
		&CConfigGeneral::m_bBackups,
		&CConfigGeneral::m_bSingleInstance,
		&CConfigGeneral::m_bPreviewFullRow,
		&CConfigGeneral::m_bDisableDblClick,
		&CConfigGeneral::m_bWrapPatternValue,
		&CConfigGeneral::m_bCutVolume,
		&CConfigGeneral::m_bFDSOldVolume,
		&CConfigGeneral::m_bRetrieveChanState,
		&CConfigGeneral::m_bOverflowPaste,
		&CConfigGeneral::m_bShowSkippedRows,
		&CConfigGeneral::m_bHexKeypad,
		&CConfigGeneral::m_bMultiFrameSel,
		&CConfigGeneral::m_bCheckVersion,
	};

	if (pNMLV->uChanged & LVIF_STATE) {
		if (pNMLV->uNewState & LVNI_SELECTED || pNMLV->uNewState & 0x3000) {
			SetDlgItemTextW(IDC_EDIT_CONFIG_DESC, FormattedW(L"Description: %s", (LPCWSTR)CONFIG_DESC[pNMLV->iItem]));

			if (pNMLV->iItem >= 0 && pNMLV->iItem < SETTINGS_BOOL_COUNT)
				pList->SetItemState(pNMLV->iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		}

		if (pNMLV->uNewState & 0x3000) {
			SetModified();
			for (int i = 0; i < SETTINGS_BOOL_COUNT; i++)
				this->*(CONFIG_BOOL[i]) = pList->GetCheck(i) != 0;
		}
	}

	*pResult = 0;
}

BOOL CConfigGeneral::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN) {
		WCHAR Text[64] = { };
		int id = GetFocus()->GetDlgCtrlID();
		int key = pMsg->wParam;

		if (key == 27)		// ESC
			key = 0;

		switch (id) {
			case IDC_KEY_NOTE_CUT:
				m_iKeyNoteCut = key;
				break;
			case IDC_KEY_NOTE_RELEASE:
				m_iKeyNoteRelease = key;
				break;
			case IDC_KEY_CLEAR:
				m_iKeyClear = key;
				break;
			case IDC_KEY_REPEAT:
				m_iKeyRepeat = key;
				break;
			case IDC_KEY_ECHO_BUFFER:		// // //
				m_iKeyEchoBuffer = key;
				break;
			default:
				return CPropertyPage::PreTranslateMessage(pMsg);
		}

		GetKeyNameTextW(key ? pMsg->lParam : 0, Text, 64);
		SetDlgItemTextW(id, Text);

		SetModified();

		return TRUE;
	}

	m_wndToolTip.RelayEvent(pMsg);

	return CPropertyPage::PreTranslateMessage(pMsg);
}
