/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
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
** Any permitted reproduction of these routin, in whole or in part,
** must bear this legend.
*/

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "MainFrm.h"
#include "GrooveDlg.h"


// CGrooveDlg dialog

IMPLEMENT_DYNAMIC(CGrooveDlg, CDialog)

CGrooveDlg::CGrooveDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGrooveDlg::IDD, pParent)
{

}

CGrooveDlg::~CGrooveDlg()
{
}

void CGrooveDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CGrooveDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDAPPLY, OnBnClickedApply)
	ON_LBN_SELCHANGE(IDC_LIST_GROOVE_TABLE, OnLbnSelchangeListGrooveTable)
	ON_LBN_SELCHANGE(IDC_LIST_GROOVE_EDITOR, OnLbnSelchangeListCurrentGroove)
	ON_BN_CLICKED(IDC_BUTTON_GROOVEL_UP, OnBnClickedButtonGroovelUp)
	ON_BN_CLICKED(IDC_BUTTON_GROOVEL_DOWN, OnBnClickedButtonGroovelDown)
	ON_BN_CLICKED(IDC_BUTTON_GROOVEL_CLEAR, OnBnClickedButtonGroovelClear)
	ON_BN_CLICKED(IDC_BUTTON_GROOVEL_CLEARALL, OnBnClickedButtonGroovelClearall)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_PLUS, OnBnClickedButtonGroovePlus)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_MINUS, OnBnClickedButtonGrooveMinus)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_INSERT, OnBnClickedButtonGrooveInsert)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_REMOVE, OnBnClickedButtonGrooveRemove)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_UP, OnBnClickedButtonGrooveUp)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_DOWN, OnBnClickedButtonGrooveDown)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_COPY, OnBnClickedButtonGrooveCopy)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_PASTE, OnBnClickedButtonGroovePaste)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_EXPAND, OnBnClickedButtonGrooveExpand)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_SHRINK, OnBnClickedButtonGrooveShrink)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_GENERATE, OnBnClickedButtonGrooveGenerate)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_PAD, OnBnClickedButtonGroovePad)
END_MESSAGE_MAP()


// CGrooveDlg message handlers

BOOL CGrooveDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_pDocument = static_cast<CFamiTrackerDoc*>(((CFrameWnd*)AfxGetMainWnd())->GetActiveDocument());

	m_cGrooveTable = new CListBox;
	m_cCurrentGroove = new CListBox;
	m_cGrooveTable->SubclassDlgItem(IDC_LIST_GROOVE_TABLE, this);
	m_cCurrentGroove->SubclassDlgItem(IDC_LIST_GROOVE_EDITOR, this);

	CSpinButtonCtrl *SpinPad = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_PAD);
	SpinPad->SetRange(1, 255);
	SpinPad->SetPos(1);
	SetDlgItemText(IDC_EDIT_GROOVE_NUM, _T("12"));
	SetDlgItemText(IDC_EDIT_GROOVE_DENOM, _T("2"));

	for (int i = 0; i < MAX_GROOVE; i++) {
		CString String;
		String.Format(_T("%02X"), i);
		m_cGrooveTable->AddString(String);

		if (m_pDocument->GetGroove(i) != NULL)
			GrooveTable[i] = new CGroove(*m_pDocument->GetGroove(i));
		else
			GrooveTable[i] = new CGroove();
	}

	SetGrooveIndex(0);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CGrooveDlg::SetGrooveIndex(int Index)
{
	m_iGrooveIndex = Index;
	m_iGroovePos = 0;
	Groove = GrooveTable[m_iGrooveIndex];
	m_cGrooveTable->SetCurSel(Index);
	m_cCurrentGroove->SetCurSel(0);
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedOk()
{
	DestroyWindow();
	OnBnClickedApply();
	CDialog::OnOK();
}

void CGrooveDlg::OnBnClickedCancel()
{
	CDialog::OnCancel();
	DestroyWindow();
}

void CGrooveDlg::OnBnClickedApply()
{
	m_pDocument->SetModifiedFlag();
	m_pDocument->SetExceededFlag();

	int Track = static_cast<CMainFrame*>(AfxGetMainWnd())->GetSelectedTrack();

	for (int i = 0; i < MAX_GROOVE; i++)
		if (GrooveTable[i]->GetSize())
			m_pDocument->SetGroove(i, GrooveTable[i]);
		else {
			m_pDocument->SetGroove(i, NULL);
			if (m_pDocument->GetSongSpeed(Track) == i && m_pDocument->GetSongGroove(Track)) {
				m_pDocument->SetSongSpeed(Track, DEFAULT_SPEED);
				m_pDocument->SetSongGroove(Track, false);
			}
		}
}

void CGrooveDlg::OnLbnSelchangeListGrooveTable()
{
	m_iGrooveIndex = m_cGrooveTable->GetCurSel();
	UpdateCurrentGroove();
}

void CGrooveDlg::OnLbnSelchangeListCurrentGroove()
{
	m_iGroovePos = m_cCurrentGroove->GetCurSel();
}

void CGrooveDlg::UpdateCurrentGroove()
{
	CString String;
	
	Groove = GrooveTable[m_iGrooveIndex];
	m_cCurrentGroove->ResetContent();
	for (int i = 0; i < Groove->GetSize(); i++) {
		String.Format(_T("%02X: %d"), i, Groove->GetEntry(i));
		m_cCurrentGroove->InsertString(-1, String);
	}
	m_cCurrentGroove->InsertString(-1, _T("--"));

	m_cCurrentGroove->SetCurSel(m_iGroovePos);

	UpdateIndicators();
}

void CGrooveDlg::UpdateIndicators()
{
	CString String;

	String.Format(_T("Average speed: %.3f"), Groove->GetSize() ? Groove->GetAverage() : DEFAULT_SPEED);
	SetDlgItemText(IDC_STATIC_GROOVE_AVERAGE, String);
	String.Format(_T("Groove size: %d bytes"), Groove->GetSize() ? Groove->GetSize() + 2 : 0);
	SetDlgItemText(IDC_STATIC_GROOVE_SIZE, String);
	int Total = 0;
	for (int i = 0; i < MAX_GROOVE; i++) if (GrooveTable[i]->GetSize())
		Total += GrooveTable[i]->GetSize() + 2;
	String.Format(_T("Total size: %d / 255 bytes"), Total);
	SetDlgItemText(IDC_STATIC_GROOVE_TOTAL, String);

	CWnd *OKButton = GetDlgItem(IDOK);
	CWnd *ApplyButton = GetDlgItem(IDAPPLY);
	if (Total > 255) {
		OKButton->EnableWindow(false);
		ApplyButton->EnableWindow(false);
	}
	else {
		OKButton->EnableWindow(true);
		ApplyButton->EnableWindow(true);
	}
}

void CGrooveDlg::OnBnClickedButtonGroovelUp()
{
	if (m_iGrooveIndex == 0) return;
	CGroove *Temp = Groove;
	GrooveTable[m_iGrooveIndex] = GrooveTable[m_iGrooveIndex - 1];
	GrooveTable[m_iGrooveIndex - 1] = Temp;
	SetGrooveIndex(m_iGrooveIndex - 1);
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGroovelDown()
{
	if (m_iGrooveIndex == MAX_GROOVE - 1) return;
	CGroove *Temp = Groove;
	GrooveTable[m_iGrooveIndex] = GrooveTable[m_iGrooveIndex + 1];
	GrooveTable[m_iGrooveIndex + 1] = Temp;
	SetGrooveIndex(m_iGrooveIndex + 1);
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGroovelClear()
{
	Groove->Clear(0);
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGroovelClearall()
{
	for (int i = 0; i < MAX_GROOVE - 1; i++)
		GrooveTable[i]->Clear(0);
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGroovePlus()
{
	if (m_iGroovePos != Groove->GetSize() && Groove->GetEntry(m_iGroovePos) < 255)
		Groove->SetEntry(m_iGroovePos, Groove->GetEntry(m_iGroovePos) + 1);

	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveMinus()
{
	if (m_iGroovePos != Groove->GetSize() && Groove->GetEntry(m_iGroovePos) > 1)
		Groove->SetEntry(m_iGroovePos, Groove->GetEntry(m_iGroovePos) - 1);

	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveInsert()
{
	if (Groove->GetEntry(m_iGroovePos) == MAX_GROOVE_SIZE) return;
	Groove->SetSize(Groove->GetSize() + 1);
	for (int i = Groove->GetSize() - 1; i > m_iGroovePos; i--)
		Groove->SetEntry(i, Groove->GetEntry(i - 1));
	if (m_iGroovePos == Groove->GetSize() - 1)
		Groove->SetEntry(m_iGroovePos, Groove->GetEntry(m_iGroovePos - 1));
	if (Groove->GetSize() == 1)
		Groove->SetEntry(0, DEFAULT_SPEED);

	m_iGroovePos++;
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveRemove()
{
	if (m_iGroovePos > Groove->GetSize()) return;
	if (m_iGroovePos == Groove->GetSize()) m_iGroovePos--;
	for (int i = m_iGroovePos; i < Groove->GetSize() - 1; i++)
		Groove->SetEntry(i, Groove->GetEntry(i + 1));
	Groove->SetSize(Groove->GetSize() - 1);
	if (!Groove->GetSize()) Groove->Clear(0);

	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveUp()
{
	if (m_iGroovePos == 0) return;
	int Temp = Groove->GetEntry(m_iGroovePos);
	Groove->SetEntry(m_iGroovePos, Groove->GetEntry(m_iGroovePos - 1));
	Groove->SetEntry(m_iGroovePos - 1, Temp);

	m_iGroovePos--;
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveDown()
{
	if (m_iGroovePos >= Groove->GetSize() - 1) return;
	int Temp = Groove->GetEntry(m_iGroovePos);
	Groove->SetEntry(m_iGroovePos, Groove->GetEntry(m_iGroovePos + 1));
	Groove->SetEntry(m_iGroovePos + 1, Temp);

	m_iGroovePos++;
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveCopy()
{
	CString Str;

	for (int i = 0; i < Groove->GetSize(); i++)
		Str.AppendFormat(_T("%i "), Groove->GetEntry(i));

	if (!OpenClipboard())
		return;
	EmptyClipboard();
	int size = Str.GetLength() + 1;
	HANDLE hMem = GlobalAlloc(GMEM_MOVEABLE, size);
	LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hMem);
	strcpy_s(lptstrCopy, size, Str.GetBuffer());
	GlobalUnlock(hMem);
	SetClipboardData(CF_TEXT, hMem);

	CloseClipboard();
}

void CGrooveDlg::OnBnClickedButtonGroovePaste()
{
	if (!OpenClipboard())
		return;
	HANDLE hMem = GetClipboardData(CF_TEXT);
	LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hMem);
	CString Str(lptstrCopy);
	GlobalUnlock(hMem);
	CloseClipboard();

	Groove->Clear(0);
	m_iGroovePos = 0;
	while (true) {
		if (Str.IsEmpty()) break;
		Groove->SetSize(Groove->GetSize() + 1);
		int Speed = atoi(Str);
		if (Speed > 255) Speed = 255;
		if (Speed < 1)   Speed = 1;
		Groove->SetEntry(m_iGroovePos, Speed);
		m_iGroovePos++;
		if (m_iGroovePos == MAX_GROOVE_SIZE || Str.FindOneOf(_T("0123456789")) == -1 || Str.Find(_T(' ')) == -1) break;
		Str.Delete(0, Str.Find(_T(' ')) + 1);
	}
	
	m_iGroovePos = 0;
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveExpand()
{
	if (Groove->GetSize() > MAX_GROOVE_SIZE / 2) return;
	for (int i = 0; i < Groove->GetSize(); i++)
		if (Groove->GetEntry(i) < 2) return;
	Groove->SetSize(Groove->GetSize() * 2);
	for (int i = Groove->GetSize() - 1; i >= 0; i--)
		Groove->SetEntry(i, Groove->GetEntry(i / 2) / 2 + (i % 2 == 0) * (Groove->GetEntry(i / 2) % 2 == 1));

	m_iGroovePos *= 2;
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveShrink()
{
	if (Groove->GetSize() % 2 == 1) return;
	for (int i = 0; i < Groove->GetSize() / 2; i++)
		Groove->SetEntry(i, Groove->GetEntry(i * 2) + Groove->GetEntry(i * 2 + 1));
	Groove->SetSize(Groove->GetSize() / 2);

	m_iGroovePos /= 2;
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveGenerate()
{
	CString Str;
	GetDlgItemText(IDC_EDIT_GROOVE_NUM, Str);
	int Num = atoi(Str);
	GetDlgItemText(IDC_EDIT_GROOVE_DENOM, Str);
	int Denom = atoi(Str);
	if (Denom < 1 || Denom > MAX_GROOVE_SIZE || Num < Denom || Num > Denom * 255) return;

	Groove->Clear(0);
	Groove->SetSize(Denom);
	for (int i = 0; i < Num * Denom; i += Num)
		Groove->SetEntry(Denom - i / Num - 1, (i + Num) / Denom - i / Denom);

	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGroovePad()
{
	CString Str;
	GetDlgItemText(IDC_EDIT_GROOVE_PAD, Str);
	int Amount = atoi(Str);
	if (Groove->GetSize() > MAX_GROOVE_SIZE / 2) return;
	for (int i = 0; i < Groove->GetSize(); i++)
		if (Groove->GetEntry(i) <= Amount) return;

	Groove->SetSize(Groove->GetSize() * 2);
	for (int i = Groove->GetSize() - 1; i >= 0; i--)
		Groove->SetEntry(i, i % 2 == 1 ? Amount : Groove->GetEntry(i / 2) - Amount);

	m_iGroovePos *= 2;
	UpdateCurrentGroove();
}