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

#include "ConfigVersion.h"
#include "FamiTrackerEnv.h"
#include "FamiTrackerTypes.h"
#include "Settings.h"
#include "ModuleException.h"

namespace {

const LPCWSTR MODULE_ERROR_DESC[] = {
	L"None: Perform no validation at all while loading or saving modules. "
	 "The tracker might crash or enter an inconsistent state.",
	L"Default: Perform the usual error checking according to the most recent official stable build.",
	L"Official: Perform extra bounds checking to ensure that modules are openable in the official build.",
	L"Strict: Validate all modules so that they do not contain any illegal data. "
	 "Modules openable in the official build might be rejected.",
};

} // namespace

// CConfigVersion dialog

IMPLEMENT_DYNAMIC(CConfigVersion, CPropertyPage)
CConfigVersion::CConfigVersion()
	: CPropertyPage(CConfigVersion::IDD)
{
}

void CConfigVersion::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

void CConfigVersion::UpdateInfo()
{
	GetDlgItem(IDC_STATIC_VERSION_ERROR)->SetWindowTextW(MODULE_ERROR_DESC[m_iModuleErrorLevel]);
}

BEGIN_MESSAGE_MAP(CConfigVersion, CPropertyPage)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_VERSION_ERRORLEVEL, OnNMCustomdrawSliderVersionErrorlevel)
END_MESSAGE_MAP()


// CConfigVersion message handlers

BOOL CConfigVersion::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_iModuleErrorLevel = Env.GetSettings()->Version.iErrorLevel;

	m_cSliderErrorLevel.SubclassDlgItem(IDC_SLIDER_VERSION_ERRORLEVEL, this);
	m_cSliderErrorLevel.SetRange(MODULE_ERROR_NONE, MODULE_ERROR_STRICT);
	m_cSliderErrorLevel.SetPos(MODULE_ERROR_STRICT - m_iModuleErrorLevel);

	UpdateInfo();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CConfigVersion::OnApply()
{
#ifndef _DEBUG
	if (m_iModuleErrorLevel == MODULE_ERROR_NONE) {
		AfxMessageBox(IDS_MODULE_ERROR_NONE, MB_ICONERROR);
		return false;
	}
#endif
	Env.GetSettings()->Version.iErrorLevel = m_iModuleErrorLevel;

	return CPropertyPage::OnApply();
}

void CConfigVersion::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
{
	UpdateInfo();
	CPropertyPage::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CConfigVersion::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
{
	UpdateInfo();
	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CConfigVersion::OnNMCustomdrawSliderVersionErrorlevel(NMHDR *pNMHDR, LRESULT *pResult)
{
	int NewLevel = MODULE_ERROR_STRICT - m_cSliderErrorLevel.GetPos();
	if (m_iModuleErrorLevel != NewLevel) {
		m_iModuleErrorLevel = NewLevel;
		SetModified();
	}

	*pResult = 0;
}
