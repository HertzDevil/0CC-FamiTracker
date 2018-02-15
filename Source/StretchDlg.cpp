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

#include "StretchDlg.h"
#include <vector>
#include "sv_regex.h"
#include "str_conv/str_conv.hpp"
#include "NumConv.h"

const size_t STRETCH_MAP_TEST_LEN = 16;

// CStretchDlg dialog

IMPLEMENT_DYNAMIC(CStretchDlg, CDialog)

CStretchDlg::CStretchDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CStretchDlg::IDD, pParent), m_iStretchMap(), m_bValid(true)
{

}

CStretchDlg::~CStretchDlg()
{
}

void CStretchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CStretchDlg, CDialog)
	ON_EN_CHANGE(IDC_EDIT_STRETCH_MAP, OnEnChangeEditStretchMap)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_STRETCH_EXPAND, OnBnClickedButtonStretchExpand)
	ON_BN_CLICKED(IDC_BUTTON_STRETCH_SHRINK, OnBnClickedButtonStretchShrink)
	ON_BN_CLICKED(IDC_BUTTON_STRETCH_RESET, OnBnClickedButtonStretchReset)
	ON_BN_CLICKED(IDC_BUTTON_STRETCH_INVERT, OnBnClickedButtonStretchInvert)
END_MESSAGE_MAP()


// CStretchDlg message handlers


std::vector<int> CStretchDlg::GetStretchMap()
{
	CDialog::DoModal();
	return m_iStretchMap;
}

BOOL CStretchDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	OnBnClickedButtonStretchReset();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CStretchDlg::UpdateStretch()
{
	CStringW wstr;
	GetDlgItemTextW(IDC_EDIT_STRETCH_MAP, wstr);
	auto str = conv::to_utf8(wstr);

	int pos = 0;
	m_iStretchMap.clear();

	for (auto x : re::tokens(str))
		if (auto e = conv::to_uint8(x.str()))
			m_iStretchMap.push_back(*e);

	if (m_iStretchMap.empty()) m_bValid = false;
	else if (m_iStretchMap[0] == 0) m_bValid = false;
	else {
		m_bValid = true;
		UpdateTest();
	}
}

void CStretchDlg::UpdateTest()
{
	std::string str = "Test:";
	unsigned int count = 0, mapPos = 0;
	for (int i = 0; i < STRETCH_MAP_TEST_LEN; i++) {
		if (count < STRETCH_MAP_TEST_LEN && m_iStretchMap[mapPos] != 0)
			str += ' ' + conv::from_uint(count);
		else
			str += " -";
		count += m_iStretchMap[mapPos];
		if (++mapPos == m_iStretchMap.size())
			mapPos = 0;
	}
	SetDlgItemTextW(IDC_STRETCH_TEST, conv::to_wide(str).data());
}

void CStretchDlg::OnEnChangeEditStretchMap()
{
	CStringW str;
	UpdateStretch();

	if (!m_bValid)
		SetDlgItemTextW(IDC_STRETCH_TEST, L" Invalid stretch map");
	GetDlgItem(IDOK)->EnableWindow(m_bValid);
	GetDlgItem(IDC_BUTTON_STRETCH_INVERT)->EnableWindow(m_bValid);
}

void CStretchDlg::OnBnClickedCancel()
{
	m_iStretchMap.clear();
	CDialog::OnCancel();
}

void CStretchDlg::OnBnClickedButtonStretchExpand()
{
	SetDlgItemTextW(IDC_EDIT_STRETCH_MAP, L"1 0");
}

void CStretchDlg::OnBnClickedButtonStretchShrink()
{
	SetDlgItemTextW(IDC_EDIT_STRETCH_MAP, L"2");
}

void CStretchDlg::OnBnClickedButtonStretchReset()
{
	SetDlgItemTextW(IDC_EDIT_STRETCH_MAP, L"1");
}

void CStretchDlg::OnBnClickedButtonStretchInvert()
{
	if (!m_bValid) return;
	CStringA str;
	unsigned int pos = 0;

	while (pos < m_iStretchMap.size()) {
		int x = m_iStretchMap[pos++];
		int y = 0;
		while (m_iStretchMap[pos] == 0) {
			if (pos >= m_iStretchMap.size())
				break;
			pos++;
			y++;
		}
		str.AppendFormat(" %d", y + 1);
		for (int i = 0; i < x - 1; i++)
			str.AppendFormat(" %d", 0);
	}

	str.Delete(0);
	SetDlgItemTextW(IDC_EDIT_STRETCH_MAP, conv::to_wide(str).data());
}
