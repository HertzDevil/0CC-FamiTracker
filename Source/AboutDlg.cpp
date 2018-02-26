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

#include "AboutDlg.h"
#include "version.h"
#include "str_conv/str_conv.hpp"		// // //

// CAboutDlg dialog used for App About

namespace {

const LPCWSTR LINK_WEB  = L"http://hertzdevil.info/programs/";
const LPCWSTR LINK_BUG  = L"http://hertzdevil.info/forum/index.php";		// // //
const LPCWSTR LINK_MAIL = L"mailto:nicetas.c@gmail.com";

} // namespace

// CLinkLabel

BEGIN_MESSAGE_MAP(CLinkLabel, CStatic)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSELEAVE()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

CLinkLabel::CLinkLabel(CStringW address)
{
	m_strAddress = address;
	m_bHover = false;
}

HBRUSH CLinkLabel::CtlColor(CDC* pDC, UINT /*nCtlColor*/)
{
	pDC->SetTextColor(m_bHover ? 0x0000FF : 0xFF0000);
	pDC->SetBkMode(TRANSPARENT);
	return (HBRUSH)GetStockObject(NULL_BRUSH);
}

void CLinkLabel::OnLButtonUp(UINT nFlags, CPoint point)
{
	ShellExecuteW(NULL, L"open", m_strAddress, NULL, NULL, SW_SHOWNORMAL);
	CStatic::OnLButtonUp(nFlags, point);
}

void CLinkLabel::OnMouseLeave()
{
	m_bHover = false;
	CRect rect, parentRect;
	GetWindowRect(&rect);
	GetParent()->GetWindowRect(parentRect);
	rect.OffsetRect(-parentRect.left - GetSystemMetrics(SM_CXDLGFRAME), -parentRect.top - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYDLGFRAME));
	GetParent()->RedrawWindow(rect);
	CStatic::OnMouseLeave();
}

void CLinkLabel::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bHover) {
		m_bHover = true;
		CRect rect, parentRect;
		GetWindowRect(&rect);
		GetParent()->GetWindowRect(parentRect);
		rect.OffsetRect(-parentRect.left - GetSystemMetrics(SM_CXDLGFRAME), -parentRect.top - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYDLGFRAME));
		GetParent()->RedrawWindow(rect);

		TRACKMOUSEEVENT t;
		t.cbSize = sizeof(TRACKMOUSEEVENT);
		t.dwFlags = TME_LEAVE;
		t.hwndTrack = m_hWnd;
		TrackMouseEvent(&t);
	}

	CStatic::OnMouseMove(nFlags, point);
}

// CHead

BEGIN_MESSAGE_MAP(CHead, CStatic)
END_MESSAGE_MAP()

CHead::CHead()
{
}

void CHead::DrawItem(LPDRAWITEMSTRUCT lpDraw)
{
	CDC *pDC = CDC::FromHandle(lpDraw->hDC);

	CBitmap bmp;
	bmp.LoadBitmap(IDB_ABOUT);

	CDC dcImage;
	dcImage.CreateCompatibleDC(pDC);
	dcImage.SelectObject(bmp);

	pDC->BitBlt(0, 0, 434, 80, &dcImage, 0, 0, SRCCOPY);
}

// CAboutDlg

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

CAboutDlg::CAboutDlg() :
	CDialog(CAboutDlg::IDD),
	m_cMail(LINK_MAIL),
	m_cWeb(LINK_WEB),
	m_cBug(LINK_BUG)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL CAboutDlg::OnInitDialog()
{
#ifdef WIP
	CStringW aboutString = FormattedW(L"0CC-FamiTracker %s", conv::to_wide(Get0CCFTVersionString()).data());		// // //
#else
	CStringW aboutString;
	AfxFormatString1(aboutString, IDS_ABOUT_VERSION_FORMAT, Get0CCFTVersionString());
#endif

	SetDlgItemTextW(IDC_ABOUT1, aboutString);
	SetDlgItemTextW(IDC_ABOUT_CONTRIB,
		L"- Original software by jsr\r\n"
		L"- Export plugin support by Gradualore\r\n"
		L"- Toolbar icons are made by ilkke\r\n"
		L"- DPCM import resampler by Jarhmander\r\n"
		L"- Module text import/export by rainwarrior");		// // //
	SetDlgItemTextW(IDC_ABOUT_LIB,
		L"- Blip_buffer 0.4.0 is Copyright (C) blargg (LGPL v2.1)\r\n"
		L"- Fast Fourier Transform code is (C) 2017 Project Nayuki (MIT)\r\n"
		L"- YM2413 emulator is written by Mitsutaka Okazaki (MIT)\r\n"
		L"- FDS sound emulator from rainwarrior's NSFPlay\r\n"
		L"- JSON for Modern C++ is Copyright (C) Niels Lohmann (MIT)");

	m_cHead.SubclassDlgItem(IDC_HEAD, this);

	EnableToolTips(TRUE);

	m_wndToolTip.Create(this, TTS_ALWAYSTIP);
	m_wndToolTip.Activate(TRUE);

	m_cMail.SubclassDlgItem(IDC_MAIL, this);

	LOGFONTW LogFont;
	CFont *pFont;
	pFont = m_cMail.GetFont();
	pFont->GetLogFont(&LogFont);
	LogFont.lfUnderline = 1;
	m_cLinkFont.CreateFontIndirectW(&LogFont);

	m_cMail.SetFont(&m_cLinkFont);
	m_wndToolTip.AddTool(&m_cMail, IDS_ABOUT_TOOLTIP_MAIL);

	m_cWeb.SubclassDlgItem(IDC_WEBPAGE, this);
	m_cWeb.SetFont(&m_cLinkFont);
	m_wndToolTip.AddTool(&m_cWeb, IDS_ABOUT_TOOLTIP_WEB);

	m_cBug.SubclassDlgItem(IDC_BUG, this);
	m_cBug.SetFont(&m_cLinkFont);
	m_wndToolTip.AddTool(&m_cBug, IDS_ABOUT_TOOLTIP_BUG);

	CStatic *pStatic = static_cast<CStatic*>(GetDlgItem(IDC_ABOUT1));
	CFont *pOldFont = pStatic->GetFont();
	LOGFONTW NewLogFont;
	pOldFont->GetLogFont(&NewLogFont);
	NewLogFont.lfWeight = FW_BOLD;
	m_cBoldFont.CreateFontIndirectW(&NewLogFont);
	NewLogFont.lfHeight = 18;
//	NewLogFont.lfUnderline = TRUE;
	m_cTitleFont.CreateFontIndirectW(&NewLogFont);
	static_cast<CStatic*>(GetDlgItem(IDC_ABOUT1))->SetFont(&m_cTitleFont);
	static_cast<CStatic*>(GetDlgItem(IDC_ABOUT2))->SetFont(&m_cBoldFont);
	static_cast<CStatic*>(GetDlgItem(IDC_ABOUT3))->SetFont(&m_cBoldFont);

	return TRUE;
}

BOOL CAboutDlg::PreTranslateMessage(MSG* pMsg)
{
	m_wndToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}
