/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
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

#include "GrooveDlg.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"
#include "SongData.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "PatternClipData.h"
#include "Clipboard.h"
#include "PatternNote.h"
#include "ft0cc/doc/groove.hpp"
#include "str_conv/str_conv.hpp"
#include "NumConv.h"
#include "sv_regex.h"

using groove = ft0cc::doc::groove;

// CGrooveDlg dialog

IMPLEMENT_DYNAMIC(CGrooveDlg, CDialog)

CGrooveDlg::CGrooveDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGrooveDlg::IDD, pParent)
{

}

void CGrooveDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CGrooveDlg, CDialog)
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDAPPLY, OnBnClickedApply)
	ON_LBN_SELCHANGE(IDC_LIST_GROOVE_TABLE, OnLbnSelchangeListGrooveTable)
	ON_LBN_SELCHANGE(IDC_LIST_GROOVE_EDITOR, OnLbnSelchangeListCurrentGroove)
	ON_BN_CLICKED(IDC_BUTTON_GROOVEL_UP, OnBnClickedButtonGroovelUp)
	ON_BN_CLICKED(IDC_BUTTON_GROOVEL_DOWN, OnBnClickedButtonGroovelDown)
	ON_BN_CLICKED(IDC_BUTTON_GROOVEL_CLEAR, OnBnClickedButtonGroovelClear)
	ON_BN_CLICKED(IDC_BUTTON_GROOVEL_CLEARALL, OnBnClickedButtonGroovelClearall)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_UP, OnBnClickedButtonGrooveUp)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_DOWN, OnBnClickedButtonGrooveDown)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_COPY, OnBnClickedButtonGrooveCopyFxx)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_EXPAND, OnBnClickedButtonGrooveExpand)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_SHRINK, OnBnClickedButtonGrooveShrink)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_GENERATE, OnBnClickedButtonGrooveGenerate)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_PAD, OnBnClickedButtonGroovePad)
END_MESSAGE_MAP()


// CGrooveDlg message handlers

BOOL CGrooveDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_cGrooveTable.SubclassDlgItem(IDC_LIST_GROOVE_TABLE, this);
	m_cCurrentGroove.SubclassDlgItem(IDC_LIST_GROOVE_EDITOR, this);

	CSpinButtonCtrl *SpinPad = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_PAD);
	SpinPad->SetRange(1, 255);
	SpinPad->SetPos(1);
	SetDlgItemTextW(IDC_EDIT_GROOVE_NUM, L"12");
	SetDlgItemTextW(IDC_EDIT_GROOVE_DENOM, L"2");

	ReloadGrooves();
	SetGrooveIndex(0);

	return TRUE;  // return TRUE unless you set the focus to a control
}

BOOL CGrooveDlg::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message) {
	case WM_KEYDOWN:
		switch (pMsg->wParam) {
		case VK_RETURN:	// Return
			pMsg->wParam = 0;
			ParseGrooveField();
			return TRUE;
		case VK_TAB:
		case VK_DOWN:
		case VK_UP:
		case VK_LEFT:
		case VK_RIGHT:
		case VK_SPACE:
			// Do nothing
			break;
		}
		break;
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CGrooveDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog::OnShowWindow(bShow, nStatus);
	if (bShow == TRUE) {
		ReloadGrooves();
		UpdateCurrentGroove();
	}
}

void CGrooveDlg::AssignModule(CFamiTrackerModule &modfile) {
	modfile_ = &modfile;
}

void CGrooveDlg::SetGrooveIndex(int Index)
{
	m_iGrooveIndex = Index;
	m_iGroovePos = 0;
	Groove = GrooveTable[m_iGrooveIndex].get();
	m_cGrooveTable.SetCurSel(Index);
	m_cCurrentGroove.SetCurSel(0);
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedOk()
{
	ShowWindow(SW_HIDE);
	OnBnClickedApply();
	CDialog::OnOK();
}

void CGrooveDlg::OnBnClickedCancel()
{
	ReloadGrooves();
	CDialog::OnCancel();
	ShowWindow(SW_HIDE);
}

void CGrooveDlg::OnBnClickedApply()
{
	CFamiTrackerDoc::GetDoc()->ModifyIrreversible();

	for (int i = 0; i < MAX_GROOVE; i++)
		if (GrooveTable[i]->size())
			modfile_->SetGroove(i, std::make_shared<groove>(*GrooveTable[i]));
		else {
			modfile_->SetGroove(i, nullptr);
			modfile_->VisitSongs([&] (CSongData &song) {
				if (song.GetSongSpeed() == i && song.GetSongGroove()) {
					song.SetSongSpeed(DEFAULT_SPEED);
					song.SetSongGroove(false);
				}
			});
		}
}

void CGrooveDlg::OnLbnSelchangeListGrooveTable()
{
	m_iGrooveIndex = m_cGrooveTable.GetCurSel();
	UpdateCurrentGroove();
}

void CGrooveDlg::OnLbnSelchangeListCurrentGroove()
{
	m_iGroovePos = m_cCurrentGroove.GetCurSel();
}

void CGrooveDlg::ReloadGrooves()
{
	SetDlgItemTextW(IDC_EDIT_GROOVE_FIELD, L"");

	m_cGrooveTable.ResetContent();
	m_cCurrentGroove.ResetContent();
	for (int i = 0; i < MAX_GROOVE; i++) {
		bool Used = false;
		if (const auto orig = modfile_->GetGroove(i)) {
			GrooveTable[i] = std::make_unique<groove>(*orig);
			Used = true;
		}
		else
			GrooveTable[i] = std::make_unique<groove>();

		m_cGrooveTable.AddString(FormattedW(L"%02X%s", i, Used ? L" *" : L""));
	}
}

void CGrooveDlg::UpdateCurrentGroove()
{
	CStringW disp = L"";

	Groove = GrooveTable[m_iGrooveIndex].get();
	m_cCurrentGroove.ResetContent();
	unsigned i = 0;
	for (uint8_t entry : *Groove) {
		AppendFormatW(disp, L"%d ", entry);
		m_cCurrentGroove.InsertString(-1, FormattedW(L"%02X: %d", i++, entry));
	}
	m_cCurrentGroove.InsertString(-1, L"--");

	m_cCurrentGroove.SetCurSel(m_iGroovePos);
	SetDlgItemTextW(IDC_EDIT_GROOVE_FIELD, disp);

	m_cGrooveTable.SetRedraw(FALSE);
	m_cGrooveTable.DeleteString(m_iGrooveIndex);
	m_cGrooveTable.InsertString(m_iGrooveIndex, FormattedW(L"%02X%s", m_iGrooveIndex, Groove->size() ? L" *" : L""));
	m_cGrooveTable.SetCurSel(m_iGrooveIndex);
	m_cGrooveTable.SetRedraw(TRUE);

	UpdateIndicators();
}

void CGrooveDlg::UpdateIndicators()
{
	SetDlgItemTextW(IDC_STATIC_GROOVE_AVERAGE, FormattedW(L"Speed: %.3f", Groove->average()));
	SetDlgItemTextW(IDC_STATIC_GROOVE_SIZE, FormattedW(L"Size: %d bytes", Groove->size() ? Groove->compiled_size() : 0));
	int Total = 0;
	for (int i = 0; i < MAX_GROOVE; ++i)
		if (GrooveTable[i]->size())
			Total += GrooveTable[i]->compiled_size();
	SetDlgItemTextW(IDC_STATIC_GROOVE_TOTAL, FormattedW(L"Total size: %d / 255 bytes", Total));

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
	GrooveTable[m_iGrooveIndex].swap(GrooveTable[m_iGrooveIndex - 1]);
	UpdateCurrentGroove();
	SetGrooveIndex(m_iGrooveIndex - 1);
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGroovelDown()
{
	if (m_iGrooveIndex == MAX_GROOVE - 1) return;
	GrooveTable[m_iGrooveIndex].swap(GrooveTable[m_iGrooveIndex + 1]);
	UpdateCurrentGroove();
	SetGrooveIndex(m_iGrooveIndex + 1);
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGroovelClear()
{
	*Groove = groove { };
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGroovelClearall()
{
	m_cGrooveTable.SetRedraw(FALSE);
	m_cGrooveTable.ResetContent();
	CStringW str;
	for (int i = 0; i < MAX_GROOVE; ++i) {
		*GrooveTable[i] = groove { };
		m_cGrooveTable.AddString(conv::to_wide(conv::from_int(i, 2)).data());
	}
	m_cGrooveTable.SetRedraw(TRUE);
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveUp()
{
	if (m_iGroovePos <= 0)
		return;
	int Temp = Groove->entry(m_iGroovePos);
	Groove->set_entry(m_iGroovePos, Groove->entry(m_iGroovePos - 1));
	Groove->set_entry(m_iGroovePos - 1, Temp);

	m_iGroovePos--;
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveDown()
{
	if (m_iGroovePos >= (int)Groove->size() - 1)
		return;
	int Temp = Groove->entry(m_iGroovePos);
	Groove->set_entry(m_iGroovePos, Groove->entry(m_iGroovePos + 1));
	Groove->set_entry(m_iGroovePos + 1, Temp);

	m_iGroovePos++;
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveCopyFxx()
{
	const unsigned size = Groove->size();
	if (!size) {
		MessageBeep(MB_ICONWARNING);
		return;
	}

	CClipboard Clipboard(CFamiTrackerView::GetView(), ::RegisterClipboardFormatW(CFamiTrackerView::CLIPBOARD_ID));
	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	CPatternClipData Fxx(1, size);
	Fxx.ClipInfo.StartColumn = COLUMN_EFF1;
	Fxx.ClipInfo.EndColumn   = COLUMN_EFF1;

	unsigned char prev = 0;
	for (unsigned char i = 0; i < Groove->size(); i++) {
		stChanNote row;
		unsigned char x = Groove->entry(i);
		if (x != prev || !i) {
			row.EffNumber[0] = EF_SPEED;
			row.EffParam[0] = x;
		}
		memcpy(Fxx.GetPattern(0, i), &row, sizeof(stChanNote));
		prev = x;
	}

	Clipboard.TryCopy(Fxx);		// // //
}

void CGrooveDlg::ParseGrooveField()
{
	CStringW WStr;
	GetDlgItemTextW(IDC_EDIT_GROOVE_FIELD, WStr);
	auto Str = conv::to_utf8(WStr);

	*Groove = groove { };
	m_iGroovePos = 0;

	for (auto x : re::tokens(Str)) {
		if (auto entry = conv::to_uint8(x.str())) {
			Groove->resize(Groove->size() + 1);
			int Speed = std::clamp((int)*entry, 1, 255);
			Groove->set_entry(m_iGroovePos++, Speed);
		}
	}

	m_iGroovePos = 0;
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveExpand()
{
	if (Groove->size() > groove::max_size / 2) return;
	for (uint8_t entry : *Groove)
		if (entry < 2)
			return;
	Groove->resize(Groove->size() * 2);
	for (int i = Groove->size() - 1; i >= 0; i--)
		Groove->set_entry(i, Groove->entry(i / 2) / 2 + (i % 2 == 0) * (Groove->entry(i / 2) % 2 == 1));

	m_iGroovePos *= 2;
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveShrink()
{
	if (Groove->size() % 2 == 1)
		return;
	for (unsigned i = 0; i < Groove->size() / 2; ++i)
		Groove->set_entry(i, Groove->entry(i * 2) + Groove->entry(i * 2 + 1));
	Groove->resize(Groove->size() / 2);

	m_iGroovePos /= 2;
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveGenerate()
{
	CStringW Str;
	GetDlgItemTextW(IDC_EDIT_GROOVE_NUM, Str);
	int Num = conv::to_int(conv::to_utf8(Str)).value_or(0);
	GetDlgItemTextW(IDC_EDIT_GROOVE_DENOM, Str);
	int Denom = conv::to_int(conv::to_utf8(Str)).value_or(0);
	if (Denom < 1 || Denom > groove::max_size || Num < Denom || Num > Denom * 255) return;

	*Groove = groove { };
	Groove->resize(Denom);
	for (int i = 0; i < Num * Denom; i += Num)
		Groove->set_entry(Denom - i / Num - 1, (i + Num) / Denom - i / Denom);

	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGroovePad()
{
	CStringW Str;
	GetDlgItemTextW(IDC_EDIT_GROOVE_PAD, Str);
	int Amount = conv::to_int(conv::to_utf8(Str)).value_or(0);
	if (Groove->size() > groove::max_size / 2)
		return;
	for (uint8_t entry : *Groove)
		if (entry <= Amount)
			return;

	Groove->resize(Groove->size() * 2);
	for (int i = Groove->size() - 1; i >= 0; i--)
		Groove->set_entry(i, i % 2 == 1 ? Amount : Groove->entry(i / 2) - Amount);

	m_iGroovePos *= 2;
	UpdateCurrentGroove();
}
