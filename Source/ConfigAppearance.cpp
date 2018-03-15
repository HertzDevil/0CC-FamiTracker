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

#include "ConfigAppearance.h"
#include "FamiTracker.h"
#include "Settings.h"
#include "ColorScheme.h"
#include "Graphics.h"
#include "Color.h"		// // //
#include <fstream>
#include <string>
#include "FileDialogs.h"		// // //
#include "NumConv.h"		// // //
#include "str_conv/str_conv.hpp"		// // //

const std::string_view CConfigAppearance::COLOR_ITEMS[] = {		// // //
	"Background",
	"Highlighted background",
	"Highlighted background 2",
	"Pattern text",
	"Highlighted pattern text",
	"Highlighted pattern text 2",
	"Instrument column",
	"Volume column",
	"Effect number column",
	"Selection",
	"Cursor",
	"Current row (normal mode)",		// // //
	"Current row (edit mode)",
	"Current row (playing)",
};

const char CConfigAppearance::SETTING_SEPARATOR[] = " : ";		// // // 050B
const char CConfigAppearance::HEX_PREFIX[] = "0x";		// // // 050B

// Pre-defined color schemes
const COLOR_SCHEME *const CConfigAppearance::COLOR_SCHEMES[] = {
	&DEFAULT_COLOR_SCHEME,
	&MONOCHROME_COLOR_SCHEME,
	&RENOISE_COLOR_SCHEME,
	&WHITE_COLOR_SCHEME,
	&SATURDAY_COLOR_SCHEME,		// // //
};

const int CConfigAppearance::FONT_SIZES[] = {10, 11, 12, 14, 16, 18, 20, 22};

int CALLBACK CConfigAppearance::EnumFontFamExProc(ENUMLOGFONTEXW *lpelfe, NEWTEXTMETRICEXW *lpntme, DWORD FontType, LPARAM lParam)
{
	if (lpelfe->elfLogFont.lfCharSet == ANSI_CHARSET && lpelfe->elfFullName[0] != L'@')
		reinterpret_cast<CConfigAppearance *>(lParam)->AddFontName((LPCWSTR)&lpelfe->elfFullName);

	return 1;
}

// CConfigAppearance dialog

IMPLEMENT_DYNAMIC(CConfigAppearance, CPropertyPage)
CConfigAppearance::CConfigAppearance()
	: CPropertyPage(CConfigAppearance::IDD)
{
}

CConfigAppearance::~CConfigAppearance()
{
}

void CConfigAppearance::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CConfigAppearance, CPropertyPage)
	ON_WM_PAINT()
	ON_CBN_SELCHANGE(IDC_FONT, OnCbnSelchangeFont)
	ON_BN_CLICKED(IDC_PICK_COL, OnBnClickedPickCol)
	ON_CBN_SELCHANGE(IDC_COL_ITEM, OnCbnSelchangeColItem)
	ON_CBN_SELCHANGE(IDC_SCHEME, OnCbnSelchangeScheme)
	ON_CBN_SELCHANGE(IDC_FONT_SIZE, OnCbnSelchangeFontSize)
	ON_BN_CLICKED(IDC_PATTERNCOLORS, OnBnClickedPatterncolors)
	ON_BN_CLICKED(IDC_DISPLAYFLATS, OnBnClickedDisplayFlats)
	ON_CBN_EDITCHANGE(IDC_FONT_SIZE, OnCbnEditchangeFontSize)
	ON_BN_CLICKED(IDC_BUTTON_APPEARANCE_SAVE, OnBnClickedButtonAppearanceSave)
	ON_BN_CLICKED(IDC_BUTTON_APPEARANCE_LOAD, OnBnClickedButtonAppearanceLoad)
END_MESSAGE_MAP()


// CConfigAppearance message handlers

void CConfigAppearance::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// Do not call CPropertyPage::OnPaint() for painting messages

	CRect Rect, ParentRect;
	GetWindowRect(ParentRect);

	CWnd *pWnd = GetDlgItem(IDC_COL_PREVIEW);
	pWnd->GetWindowRect(Rect);

	Rect.top -= ParentRect.top;
	Rect.bottom -= ParentRect.top;
	Rect.left -= ParentRect.left;
	Rect.right -= ParentRect.left;

	CBrush BrushColor;
	BrushColor.CreateSolidBrush(m_iColors[m_iSelectedItem]);

	// Solid color box
	CBrush *pOldBrush = dc.SelectObject(&BrushColor);
	dc.Rectangle(Rect);
	dc.SelectObject(pOldBrush);

	// Preview all colors

	pWnd = GetDlgItem(IDC_PREVIEW);
	pWnd->GetWindowRect(Rect);

	Rect.top -= ParentRect.top;
	Rect.bottom -= ParentRect.top;// - 16;
	Rect.left -= ParentRect.left;
	Rect.right -= ParentRect.left;

	int WinHeight = Rect.bottom - Rect.top;

	CFont Font;		// // //
	Font.CreateFontW(-m_iFontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, m_strFont.data());
	CFont *OldFont = OldFont = dc.SelectObject(&Font);

	// Background
	dc.FillSolidRect(Rect, GetColor(COL_BACKGROUND));
	dc.SetBkMode(TRANSPARENT);

	COLORREF ShadedCol = DIM(GetColor(COL_PATTERN_TEXT), .5);
	COLORREF ShadedHiCol = DIM(GetColor(COL_PATTERN_TEXT_HILITE), .5);

	int iRowSize = m_iFontSize;
	int iRows = (WinHeight - 12) / iRowSize;// 12;

	COLORREF CursorCol = GetColor(COL_CURSOR);
	COLORREF CursorShadedCol = DIM(CursorCol, .5);
	COLORREF BgCol = GetColor(COL_BACKGROUND);
	COLORREF HilightBgCol = GetColor(COL_BACKGROUND_HILITE);
	COLORREF Hilight2BgCol = GetColor(COL_BACKGROUND_HILITE2);

	const auto BAR = [&] (int x, int y) {
		dc.FillSolidRect(x + 3, y + (iRowSize / 2) + 1, 10 - 7, 1, ShadedCol);
	};

	for (int i = 0; i < iRows; ++i) {

		int OffsetTop = Rect.top + (i * iRowSize) + 6;
		int OffsetLeft = Rect.left + 9;

		if (OffsetTop > (Rect.bottom - iRowSize))
			break;

		if ((i & 3) == 0) {
			if ((i & 6) == 0)
				GradientBar(dc, Rect.left, OffsetTop, Rect.right - Rect.left, iRowSize, Hilight2BgCol, BgCol);		// // //
			else
				GradientBar(dc, Rect.left, OffsetTop, Rect.right - Rect.left, iRowSize, HilightBgCol, BgCol);

			if (i == 0) {
				dc.SetTextColor(GetColor(COL_PATTERN_TEXT_HILITE));
				GradientBar(dc, Rect.left + 5, OffsetTop, 40, iRowSize, CursorCol, GetColor(COL_BACKGROUND));
				dc.Draw3dRect(Rect.left + 5, OffsetTop, 40, iRowSize, CursorCol, CursorShadedCol);
			}
			else
				dc.SetTextColor(ShadedHiCol);
		}
		else {
			dc.SetTextColor(ShadedCol);
		}

		if (i == 0) {
			dc.TextOutW(OffsetLeft, OffsetTop - 2, L"C");
			dc.TextOutW(OffsetLeft + 12, OffsetTop - 2, L"-");
			dc.TextOutW(OffsetLeft + 24, OffsetTop - 2, L"4");
		}
		else {
			BAR(OffsetLeft, OffsetTop - 2);
			BAR(OffsetLeft + 12, OffsetTop - 2);
			BAR(OffsetLeft + 24, OffsetTop - 2);
		}

		if ((i & 3) == 0) {
			dc.SetTextColor(ShadedHiCol);
		}
		else {
			dc.SetTextColor(ShadedCol);
		}

		BAR(OffsetLeft + 40, OffsetTop - 2);
		BAR(OffsetLeft + 52, OffsetTop - 2);
		BAR(OffsetLeft + 68, OffsetTop - 2);
		BAR(OffsetLeft + 84, OffsetTop - 2);
		BAR(OffsetLeft + 96, OffsetTop - 2);
		BAR(OffsetLeft + 108, OffsetTop - 2);
	}

	dc.SelectObject(OldFont);
}

BOOL CConfigAppearance::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	const CSettings *pSettings = theApp.GetSettings();
	m_strFont = pSettings->Appearance.strFont;		// // //

	CDC *pDC = GetDC();
	if (pDC != NULL) {
		LOGFONTW LogFont = { };		// // //
		LogFont.lfCharSet = ANSI_CHARSET;
		EnumFontFamiliesExW(pDC->m_hDC, &LogFont, (FONTENUMPROC)EnumFontFamExProc, (LPARAM)this, 0);
		ReleaseDC(pDC);
	}

	CComboBox *pFontSizeList = static_cast<CComboBox*>(GetDlgItem(IDC_FONT_SIZE));
	CComboBox *pItemsBox = static_cast<CComboBox*>(GetDlgItem(IDC_COL_ITEM));

	for (int i = 0; i < COLOR_ITEM_COUNT; ++i) {
		pItemsBox->AddString(conv::to_wide(COLOR_ITEMS[i]).data());
	}

	pItemsBox->SelectString(0, conv::to_wide(COLOR_ITEMS[0]).data());

	m_iSelectedItem = 0;

	m_iColors[COL_BACKGROUND]			= pSettings->Appearance.iColBackground;
	m_iColors[COL_BACKGROUND_HILITE]	= pSettings->Appearance.iColBackgroundHilite;
	m_iColors[COL_BACKGROUND_HILITE2]	= pSettings->Appearance.iColBackgroundHilite2;
	m_iColors[COL_PATTERN_TEXT]			= pSettings->Appearance.iColPatternText;
	m_iColors[COL_PATTERN_TEXT_HILITE]	= pSettings->Appearance.iColPatternTextHilite;
	m_iColors[COL_PATTERN_TEXT_HILITE2]	= pSettings->Appearance.iColPatternTextHilite2;
	m_iColors[COL_PATTERN_INSTRUMENT]	= pSettings->Appearance.iColPatternInstrument;
	m_iColors[COL_PATTERN_VOLUME]		= pSettings->Appearance.iColPatternVolume;
	m_iColors[COL_PATTERN_EFF_NUM]		= pSettings->Appearance.iColPatternEffect;
	m_iColors[COL_SELECTION]			= pSettings->Appearance.iColSelection;
	m_iColors[COL_CURSOR]				= pSettings->Appearance.iColCursor;
	m_iColors[COL_CURRENT_ROW_NORMAL]	= pSettings->Appearance.iColCurrentRowNormal;		// // //
	m_iColors[COL_CURRENT_ROW_EDIT]		= pSettings->Appearance.iColCurrentRowEdit;
	m_iColors[COL_CURRENT_ROW_PLAYING]	= pSettings->Appearance.iColCurrentRowPlaying;

	m_iFontSize	= pSettings->Appearance.iFontSize;		// // //

	m_bPatternColors = pSettings->Appearance.bPatternColor;		// // //
	m_bDisplayFlats = pSettings->Appearance.bDisplayFlats;		// // //

	pItemsBox = static_cast<CComboBox*>(GetDlgItem(IDC_SCHEME));

	for (auto *scheme : COLOR_SCHEMES)
		pItemsBox->AddString(scheme->NAME);

	for (int pt : FONT_SIZES)		// // //
		pFontSizeList->AddString(conv::to_wide(conv::from_int(pt)).data());
	pFontSizeList->SetWindowTextW(conv::to_wide(conv::from_int(m_iFontSize)).data());

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigAppearance::AddFontName(std::wstring_view Name)		// // //
{
	if (Name.size() >= LF_FACESIZE)		// // //
		return;

	CComboBox *pFontList = static_cast<CComboBox*>(GetDlgItem(IDC_FONT));

	pFontList->AddString(Name.data());

	if (m_strFont == Name)
		pFontList->SelectString(0, Name.data());
}

BOOL CConfigAppearance::OnApply()
{
	CSettings *pSettings = theApp.GetSettings();

	pSettings->Appearance.strFont	 = m_strFont;		// // //
	pSettings->Appearance.iFontSize	 = m_iFontSize;		// // //
	pSettings->Appearance.bPatternColor = m_bPatternColors;		// // //
	pSettings->Appearance.bDisplayFlats = m_bDisplayFlats;		// // //

	pSettings->Appearance.iColBackground			= m_iColors[COL_BACKGROUND];
	pSettings->Appearance.iColBackgroundHilite		= m_iColors[COL_BACKGROUND_HILITE];
	pSettings->Appearance.iColBackgroundHilite2		= m_iColors[COL_BACKGROUND_HILITE2];
	pSettings->Appearance.iColPatternText			= m_iColors[COL_PATTERN_TEXT];
	pSettings->Appearance.iColPatternTextHilite		= m_iColors[COL_PATTERN_TEXT_HILITE];
	pSettings->Appearance.iColPatternTextHilite2	= m_iColors[COL_PATTERN_TEXT_HILITE2];
	pSettings->Appearance.iColPatternInstrument		= m_iColors[COL_PATTERN_INSTRUMENT];
	pSettings->Appearance.iColPatternVolume			= m_iColors[COL_PATTERN_VOLUME];
	pSettings->Appearance.iColPatternEffect			= m_iColors[COL_PATTERN_EFF_NUM];
	pSettings->Appearance.iColSelection				= m_iColors[COL_SELECTION];
	pSettings->Appearance.iColCursor				= m_iColors[COL_CURSOR];
	pSettings->Appearance.iColCurrentRowNormal		= m_iColors[COL_CURRENT_ROW_NORMAL];		// // //
	pSettings->Appearance.iColCurrentRowEdit		= m_iColors[COL_CURRENT_ROW_EDIT];
	pSettings->Appearance.iColCurrentRowPlaying		= m_iColors[COL_CURRENT_ROW_PLAYING];

	theApp.ReloadColorScheme();

	return CPropertyPage::OnApply();
}

void CConfigAppearance::OnCbnSelchangeFont()
{
	CComboBox *pFontList = static_cast<CComboBox*>(GetDlgItem(IDC_FONT));
	pFontList->GetLBText(pFontList->GetCurSel(), m_strFont.data());
	RedrawWindow();
	SetModified();
}

BOOL CConfigAppearance::OnSetActive()
{
	CheckDlgButton(IDC_PATTERNCOLORS, m_bPatternColors);
	CheckDlgButton(IDC_DISPLAYFLATS, m_bDisplayFlats);
	return CPropertyPage::OnSetActive();
}

void CConfigAppearance::OnBnClickedPickCol()
{
	CColorDialog ColorDialog;

	ColorDialog.m_cc.Flags |= CC_FULLOPEN | CC_RGBINIT;
	ColorDialog.m_cc.rgbResult = m_iColors[m_iSelectedItem];
	ColorDialog.DoModal();

	m_iColors[m_iSelectedItem] = ColorDialog.GetColor();

	SetModified();
	RedrawWindow();
}

void CConfigAppearance::OnCbnSelchangeColItem()
{
	CComboBox *List = static_cast<CComboBox*>(GetDlgItem(IDC_COL_ITEM));
	m_iSelectedItem = List->GetCurSel();
	RedrawWindow();
}

void CConfigAppearance::OnCbnSelchangeScheme()
{
	CComboBox *pList = static_cast<CComboBox*>(GetDlgItem(IDC_SCHEME));

	SelectColorScheme(COLOR_SCHEMES[pList->GetCurSel()]);

	SetModified();
	RedrawWindow();
}

void CConfigAppearance::SelectColorScheme(const COLOR_SCHEME *pColorScheme)
{
	CComboBox *pFontList = static_cast<CComboBox*>(GetDlgItem(IDC_FONT));
	CComboBox *pFontSizeList = static_cast<CComboBox*>(GetDlgItem(IDC_FONT_SIZE));

	SetColor(COL_BACKGROUND, pColorScheme->BACKGROUND);
	SetColor(COL_BACKGROUND_HILITE, pColorScheme->BACKGROUND_HILITE);
	SetColor(COL_BACKGROUND_HILITE2, pColorScheme->BACKGROUND_HILITE2);
	SetColor(COL_PATTERN_TEXT, pColorScheme->TEXT_NORMAL);
	SetColor(COL_PATTERN_TEXT_HILITE, pColorScheme->TEXT_HILITE);
	SetColor(COL_PATTERN_TEXT_HILITE2, pColorScheme->TEXT_HILITE2);
	SetColor(COL_PATTERN_INSTRUMENT, pColorScheme->TEXT_INSTRUMENT);
	SetColor(COL_PATTERN_VOLUME, pColorScheme->TEXT_VOLUME);
	SetColor(COL_PATTERN_EFF_NUM, pColorScheme->TEXT_EFFECT);
	SetColor(COL_SELECTION, pColorScheme->SELECTION);
	SetColor(COL_CURSOR, pColorScheme->CURSOR);
	SetColor(COL_CURRENT_ROW_NORMAL, pColorScheme->ROW_NORMAL);		// // //
	SetColor(COL_CURRENT_ROW_EDIT, pColorScheme->ROW_EDIT);
	SetColor(COL_CURRENT_ROW_PLAYING, pColorScheme->ROW_PLAYING);

	m_strFont = pColorScheme->FONT_FACE;
	m_iFontSize = pColorScheme->FONT_SIZE;
	pFontList->SelectString(0, m_strFont.data());
	pFontSizeList->SelectString(0, FormattedW(L"%i", m_iFontSize));
}

void CConfigAppearance::SetColor(int Index, int Color)
{
	m_iColors[Index] = Color;
}

int CConfigAppearance::GetColor(int Index) const
{
	return m_iColors[Index];
}

void CConfigAppearance::OnCbnSelchangeFontSize()
{
	CStringW str;
	CComboBox *pFontSizeList = static_cast<CComboBox*>(GetDlgItem(IDC_FONT_SIZE));
	pFontSizeList->GetLBText(pFontSizeList->GetCurSel(), str);
	if (auto newSize = conv::to_int(str)) {
		if (*newSize < 5 || *newSize > 30)
			return; // arbitrary
		m_iFontSize = *newSize;
		RedrawWindow();
		SetModified();
	}
}

void CConfigAppearance::OnCbnEditchangeFontSize()		// // //
{
	CStringW str;
	CComboBox *pFontSizeList = static_cast<CComboBox*>(GetDlgItem(IDC_FONT_SIZE));
	pFontSizeList->GetWindowTextW(str);
	if (auto newSize = conv::to_int(str)) {
		if (*newSize < 5 || *newSize > 30)
			return; // arbitrary
		m_iFontSize = *newSize;
		RedrawWindow();
		SetModified();
	}
}

void CConfigAppearance::OnBnClickedPatterncolors()
{
	m_bPatternColors = IsDlgButtonChecked(IDC_PATTERNCOLORS) != 0;
	SetModified();
}

void CConfigAppearance::OnBnClickedDisplayFlats()
{
	m_bDisplayFlats = IsDlgButtonChecked(IDC_DISPLAYFLATS) != 0;
	SetModified();
}

void CConfigAppearance::OnBnClickedButtonAppearanceSave()		// // // 050B
{
	if (auto path = GetSavePath(L"Theme.txt", L"", IDS_FILTER_TXT, L"*.txt"))
		ExportSettings(*path);
}

void CConfigAppearance::OnBnClickedButtonAppearanceLoad()		// // // 050B
{
	CFileDialog fileDialog {TRUE, L"txt", L"Theme.txt", OFN_HIDEREADONLY, LoadDefaultFilter(IDS_FILTER_TXT, L"*.txt")};
	if (fileDialog.DoModal() == IDOK) {
		ImportSettings(fileDialog.GetPathName());
		static_cast<CComboBox*>(GetDlgItem(IDC_FONT))->SelectString(0, m_strFont.data());
		static_cast<CComboBox*>(GetDlgItem(IDC_FONT_SIZE))->SelectString(0, FormattedW(L"%i", m_iFontSize));
		RedrawWindow();
		SetModified();
	}
}

void CConfigAppearance::ExportSettings(LPCWSTR Path) const		// // // 050B
{
	if (auto file = std::fstream {Path, std::ios_base::out}) {
		file << "# 0CC-FamiTracker appearance" << std::endl;
		for (size_t i = 0; i < std::size(m_iColors); ++i)
			file << COLOR_ITEMS[i] << SETTING_SEPARATOR << HEX_PREFIX << conv::from_uint_hex(m_iColors[i], 6) << std::endl;
		file << "Pattern colors" << SETTING_SEPARATOR << m_bPatternColors << std::endl;
		file << "Flags" << SETTING_SEPARATOR << m_bDisplayFlats << std::endl;
		file << "Font" << SETTING_SEPARATOR << conv::to_utf8(m_strFont) << std::endl;
		file << "Font size" << SETTING_SEPARATOR << m_iFontSize << std::endl;
	}
}

void CConfigAppearance::ImportSettings(LPCWSTR Path)		// // // 050B
{
	std::fstream file {Path, std::ios_base::in};
	std::string Line;

	while (true) {
		std::getline(file, Line);
		if (!file) break;
		size_t Pos = Line.find(SETTING_SEPARATOR);
		if (Pos == std::string::npos)
			continue;
		auto sv = std::string_view(Line).substr(Pos + std::size(SETTING_SEPARATOR) - 1);		// // //

		for (size_t i = 0; i < std::size(m_iColors); ++i) {
			if (Line.find(COLOR_ITEMS[i]) == std::string::npos)
				continue;
			size_t n = sv.find(HEX_PREFIX);
			if (n == std::string_view::npos)
				continue;
			if (auto c = conv::to_uint32(sv.substr(n + std::size(HEX_PREFIX) - 1), 16))
				m_iColors[i] = *c;
		}

		if (Line.find("Pattern colors") != std::string::npos) {
			if (auto x = conv::to_uint(sv))
				m_bPatternColors = (bool)*x;
		}
		else if (Line.find("Flags") != std::string::npos) {
			if (auto x = conv::to_uint(sv))
				m_bDisplayFlats = (bool)*x;
		}
		else if (Line.find("Font size") != std::string::npos) {
			if (auto x = conv::to_uint(sv))
				m_iFontSize = *x;
		}
		else if (Line.find("Font") != std::string::npos)
			m_strFont = conv::to_wide(sv.data()).data();
	}

	file.close();
}
