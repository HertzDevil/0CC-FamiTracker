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

#include "InstrumentEditPanel.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "Sequence.h"		// // //
#include "SequenceEditor.h"
#include "SequenceParser.h"		// // //
#include "DPI.h"		// // //
#include "NumConv.h"		// // //
#include "str_conv/str_conv.hpp"		// // //

// CInstrumentEditPanel dialog
//
// Base class for instrument editors
//

IMPLEMENT_DYNAMIC(CInstrumentEditPanel, CDialog)
CInstrumentEditPanel::CInstrumentEditPanel(UINT nIDTemplate, CWnd* pParent) : CDialog(nIDTemplate, pParent),
	m_pInstManager(nullptr)		// // //, m_bShow(false)
{
}

CInstrumentEditPanel::~CInstrumentEditPanel()
{
}

void CInstrumentEditPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CInstrumentEditPanel, CDialog)
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_WM_LBUTTONDOWN()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

//COLORREF m_iBGColor = 0xFF0000;

BOOL CInstrumentEditPanel::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

HBRUSH CInstrumentEditPanel::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO: Find a proper way to get the background color
	//m_iBGColor = GetPixel(pDC->m_hDC, 2, 2);

	if (!theApp.IsThemeActive())
		return hbr;

	switch (nCtlColor) {
		case CTLCOLOR_STATIC:
//		case CTLCOLOR_DLG:
			pDC->SetBkMode(TRANSPARENT);
			// TODO: this might fail on some themes?
			//return NULL;
			return GetSysColorBrush(COLOR_3DHILIGHT);
			//return CreateSolidBrush(m_iBGColor);
	}

	return hbr;
}

BOOL CInstrumentEditPanel::PreTranslateMessage(MSG* pMsg)
{
	WCHAR ClassName[256];

	switch (pMsg->message) {
		case WM_KEYDOWN:
			switch (pMsg->wParam) {
				case VK_RETURN:	// Return
					pMsg->wParam = 0;
					OnKeyReturn();
					return TRUE;
				case VK_ESCAPE:	// Esc, close the dialog
					GetParent()->DestroyWindow();		// // // no need for static_cast
					return TRUE;
				case VK_TAB:
				case VK_DOWN:
				case VK_UP:
				case VK_LEFT:
				case VK_RIGHT:
				case VK_SPACE:
					// Do nothing
					break;
				default:	// Note keys
					// Make sure the dialog is selected when previewing
					GetClassNameW(pMsg->hwnd, ClassName, std::size(ClassName));
					if (0 != wcscmp(ClassName, L"Edit")) {
						// Remove repeated keys
						if ((pMsg->lParam & (1 << 30)) == 0)
							PreviewNote((unsigned char)pMsg->wParam);
						return TRUE;
					}
			}
			break;
		case WM_KEYUP:
			PreviewRelease((unsigned char)pMsg->wParam);
			return TRUE;
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CInstrumentEditPanel::OnLButtonDown(UINT nFlags, CPoint point)
{
	// Set focus on mouse clicks to enable note preview from keyboard
	SetFocus();
	CDialog::OnLButtonDown(nFlags, point);
}

void CInstrumentEditPanel::OnKeyReturn()
{
	// Empty
}

void CInstrumentEditPanel::OnSetFocus(CWnd* pOldWnd)
{
	// Kill the default handler to avoid setting focus to a child control
	//Invalidate();
	CDialog::OnSetFocus(pOldWnd);
}

void CInstrumentEditPanel::SetInstrumentManager(CInstrumentManager *pManager)		// // //
{
	m_pInstManager = pManager;
}

CFamiTrackerDoc *CInstrumentEditPanel::GetDocument() const
{
	// Return selected document
	return CFamiTrackerDoc::GetDoc();
}

void CInstrumentEditPanel::PreviewNote(unsigned char Key)
{
	CFamiTrackerView::GetView()->PreviewNote(Key);
}

void CInstrumentEditPanel::PreviewRelease(unsigned char Key)
{
	CFamiTrackerView::GetView()->PreviewRelease(Key);
}

//
// CSequenceInstrumentEditPanel
//
// For dialog panels with sequence editors. Can translate MML strings
//

IMPLEMENT_DYNAMIC(CSequenceInstrumentEditPanel, CInstrumentEditPanel)

CSequenceInstrumentEditPanel::CSequenceInstrumentEditPanel(UINT nIDTemplate, CWnd* pParent) :
	CInstrumentEditPanel(nIDTemplate, pParent),
	m_pSequenceEditor(std::make_unique<CSequenceEditor>()),
	m_pSequence(NULL),
	m_pParentWin(pParent),
	m_iSelectedSetting(sequence_t::Volume),		// // //
	m_pParser(std::make_unique<CSequenceParser>())		// // //
{
}

CSequenceInstrumentEditPanel::~CSequenceInstrumentEditPanel()
{
}

void CSequenceInstrumentEditPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSequenceInstrumentEditPanel, CInstrumentEditPanel)
	ON_NOTIFY(NM_RCLICK, IDC_INSTSETTINGS, OnRClickInstSettings)
END_MESSAGE_MAP()

void CSequenceInstrumentEditPanel::SetupDialog(const LPCSTR *pListItems)		// // //
{
	// Instrument settings
	CListCtrl *pList = static_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS));

	CRect r;		// // // 050B
	pList->GetClientRect(&r);
	int Width = r.Width();

	pList->DeleteAllItems();
	pList->InsertColumn(0, L"", LVCFMT_LEFT, static_cast<int>(.18 * Width));
	pList->InsertColumn(1, L"#", LVCFMT_LEFT, static_cast<int>(.22 * Width));
	pList->InsertColumn(2, L"Effect name", LVCFMT_LEFT, static_cast<int>(.6 * Width));
	pList->SendMessageW(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);

	for (auto i : enum_values<sequence_t>()) {
		int nItem = value_cast(i);
		pList->InsertItem(nItem, L"", 0);
		pList->SetCheck(nItem, 0);
		pList->SetItemText(nItem, 1, L"0");
		pList->SetItemText(nItem, 2, conv::to_wide(pListItems[nItem]).data());
	}

	pList->SetItemState(value_cast(m_iSelectedSetting), LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

	SetDlgItemInt(IDC_SEQ_INDEX, value_cast(m_iSelectedSetting));

	CSpinButtonCtrl *pSequenceSpin = static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_SEQUENCE_SPIN));
	pSequenceSpin->SetRange(0, MAX_SEQUENCES - 1);

	GetDlgItem(IDC_INST_SEQUENCE_GRAPH)->GetWindowRect(&r);		// // //
	GetDesktopWindow()->MapWindowPoints(this, &r);
	m_pSequenceEditor->CreateEditor(this, r);
	m_pSequenceEditor->ShowWindow(SW_SHOW);
}

int CSequenceInstrumentEditPanel::ReadStringValue(std::string_view str)		// // //
{
	// Translate a string number to integer, accepts '$' for hexadecimal notation
	// // // 'x' has been disabled due to arp scheme support
	if (str[0] == '$')
		if (auto x = conv::to_int(str.substr(1), 16))
			return *x;
	if (auto x = conv::to_int(str))
		return *x;
	return 0;
}

void CSequenceInstrumentEditPanel::PreviewNote(unsigned char Key)
{
	// Skip if MML window has focus
	if (GetDlgItem(IDC_SEQUENCE_STRING) != GetFocus())
		CInstrumentEditPanel::PreviewNote(Key);
}

void CSequenceInstrumentEditPanel::TranslateMML(const CStringW &String) const
{
	// Takes a string and translates it into a sequence
	m_pParser->ParseSequence(conv::to_utf8(String));		// // //

	// Update editor
	if (m_pSequenceEditor != nullptr)
		m_pSequenceEditor->RedrawWindow();

	// Register a document change
	GetDocument()->ModifyIrreversible();		// // //
}

void CSequenceInstrumentEditPanel::OnRClickInstSettings(NMHDR* pNMHDR, LRESULT* pResult)
{
	POINT oPoint;
	GetCursorPos(&oPoint);

	if (m_pSequence == NULL)
		return;

	// Display clone menu
	CMenu contextMenu;
	contextMenu.LoadMenuW(IDR_SEQUENCE_POPUP);
	CMenu *pMenu = contextMenu.GetSubMenu(0);
	pMenu->EnableMenuItem(ID_CLONE_SEQUENCE, (m_pSequence->GetItemCount() != 0) ? MFS_ENABLED : MFS_DISABLED);
	pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, oPoint.x, oPoint.y, this);
}
