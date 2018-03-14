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

/*
 * This is the NSF (and other types) export dialog
 *
 */

#include "ExportDialog.h"
#include <map>
#include <vector>
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"		// // //
#include "DSampleManager.h"		// // //
#include "Compiler.h"
#include "Settings.h"
#include <optional>		// // //
#include "str_conv/str_conv.hpp"		// // //

// Define internal exporters
const LPCWSTR CExportDialog::DEFAULT_EXPORT_NAMES[] = {		// // //
	L"NSF - Nintendo Sound File",
	L"NES - iNES ROM image",
	L"BIN - Raw music data",
	L"PRG - Clean 32kB ROM image",
	L"ASM - Assembly source",
	L"NSFe - Extended Nintendo Sound File",		// // //
};

const exportFunc_t CExportDialog::DEFAULT_EXPORT_FUNCS[] = {
	&CExportDialog::CreateNSF,
	&CExportDialog::CreateNES,
	&CExportDialog::CreateBIN,
	&CExportDialog::CreatePRG,
	&CExportDialog::CreateASM,
	&CExportDialog::CreateNSFe,		// // //
};

const int CExportDialog::DEFAULT_EXPORTERS = 6;		// // //

// Remember last option when dialog is closed
int CExportDialog::m_iExportOption = 0;

// File filters
const LPCWSTR CExportDialog::NSF_FILTER[]   = { L"NSF file (*.nsf)", L".nsf" };
const LPCWSTR CExportDialog::NES_FILTER[]   = { L"NES ROM image (*.nes)", L".nes" };
const LPCWSTR CExportDialog::RAW_FILTER[]   = { L"Raw song data (*.bin)", L".bin" };
const LPCWSTR CExportDialog::DPCMS_FILTER[] = { L"DPCM sample bank (*.bin)", L".bin" };
const LPCWSTR CExportDialog::PRG_FILTER[]   = { L"NES program bank (*.prg)", L".prg" };
const LPCWSTR CExportDialog::ASM_FILTER[]	  = { L"Assembly text (*.asm)", L".asm" };
const LPCWSTR CExportDialog::NSFE_FILTER[]  = { L"NSFe file (*.nsfe)", L".nsfe" };		// // //

namespace {		// // //

std::optional<CStringW> GetSavePath(const CStringW &initFName, const CStringW &initPath, const CStringW &FilterName, const CStringW &FilterExt) {
	CStringW filter = LoadDefaultFilter(FilterName, FilterExt);
	CFileDialog FileDialog(FALSE, FilterExt, initFName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter);
	FileDialog.m_pOFN->lpstrInitialDir = initPath;

	if (FileDialog.DoModal() != IDOK)
		return std::nullopt;

	return FileDialog.GetPathName();
}

} // namespace

// Compiler logger

class CEditLog : public CCompilerLog
{
public:
	CEditLog(CWnd *pEdit) : m_pEdit(static_cast<CEdit*>(pEdit)) {};
	void WriteLog(LPCWSTR text);
	void Clear();
private:
	CEdit *m_pEdit;
};

void CEditLog::WriteLog(LPCWSTR text)
{
	int Len = m_pEdit->GetWindowTextLength();
	m_pEdit->SetSel(Len, Len, 0);
	m_pEdit->ReplaceSel(text, 0);
	m_pEdit->RedrawWindow();
}

void CEditLog::Clear()
{
	m_pEdit->SetWindowTextW(L"");
	m_pEdit->RedrawWindow();
}

// CExportDialog dialog

IMPLEMENT_DYNAMIC(CExportDialog, CDialog)
CExportDialog::CExportDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CExportDialog::IDD, pParent)
{
}

CExportDialog::~CExportDialog()
{
}

void CExportDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CExportDialog, CDialog)
	ON_BN_CLICKED(IDC_CLOSE, OnBnClickedClose)
	ON_BN_CLICKED(IDC_EXPORT, &CExportDialog::OnBnClickedExport)
	ON_BN_CLICKED(IDC_PLAY, OnBnClickedPlay)
END_MESSAGE_MAP()


// CExportDialog message handlers

void CExportDialog::OnBnClickedClose()
{
	EndDialog(0);
}

BOOL CExportDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	const auto *pModule = CFamiTrackerDoc::GetDoc()->GetModule();		// // //

	// Check PAL button if it's a PAL song
	if (pModule->GetMachine() == machine_t::PAL) {
		CheckDlgButton(IDC_NTSC, 0);
		CheckDlgButton(IDC_PAL, 1);
		CheckDlgButton(IDC_DUAL, 0);
	}
	else {
		CheckDlgButton(IDC_NTSC, 1);
		CheckDlgButton(IDC_PAL, 0);
		CheckDlgButton(IDC_DUAL, 0);
	}

	SetDlgItemTextW(IDC_NAME, conv::to_wide(pModule->GetModuleName()).data());		// // //
	SetDlgItemTextW(IDC_ARTIST, conv::to_wide(pModule->GetModuleArtist()).data());
	SetDlgItemTextW(IDC_COPYRIGHT, conv::to_wide(pModule->GetModuleCopyright()).data());

	// Fill the export box
	CComboBox *pTypeBox = static_cast<CComboBox*>(GetDlgItem(IDC_TYPE));

	// Add built in exporters
	for (int i = 0; i < DEFAULT_EXPORTERS; ++i)
		pTypeBox->AddString(DEFAULT_EXPORT_NAMES[i]);

	// // //

	// Set default selection
	pTypeBox->SetCurSel(m_iExportOption);

#ifdef _DEBUG
	GetDlgItem(IDC_PLAY)->ShowWindow(SW_SHOW);
#endif

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CExportDialog::OnBnClickedExport()
{
	CComboBox *pTypeCombo = static_cast<CComboBox*>(GetDlgItem(IDC_TYPE));
	CStringW ItemText;

	m_iExportOption = pTypeCombo->GetCurSel();
	pTypeCombo->GetLBText(m_iExportOption, ItemText);

	// Check built in exporters
	for (int i = 0; i < DEFAULT_EXPORTERS; ++i) {
		if (!ItemText.Compare(DEFAULT_EXPORT_NAMES[i])) {
			(this->*DEFAULT_EXPORT_FUNCS[i])();
			return;
		}
	}
}

void CExportDialog::CreateNSF()
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();

	if (auto path = GetSavePath(pDoc->GetFileTitle(), theApp.GetSettings()->GetPath(PATH_NSF).c_str(), NSF_FILTER[0], NSF_FILTER[1])) {		// // //
		CWaitCursor wait;

		// Collect header info
		int MachineType = 0;
		if (IsDlgButtonChecked(IDC_NTSC) == BST_CHECKED)
			MachineType = 0;
		else if (IsDlgButtonChecked(IDC_PAL) == BST_CHECKED)
			MachineType = 1;
		else if (IsDlgButtonChecked(IDC_DUAL) == BST_CHECKED)
			MachineType = 2;

		CCompiler Compiler(*pDoc->GetModule(), std::make_unique<CEditLog>(GetDlgItem(IDC_OUTPUT)));
		UpdateMetadata(Compiler);		// // //
		Compiler.ExportNSF(*path, MachineType);
		theApp.GetSettings()->SetDirectory((LPCWSTR)*path, PATH_NSF);
	}
}

void CExportDialog::CreateNSFe()		// // //
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();

	if (auto path = GetSavePath(pDoc->GetFileTitle(), theApp.GetSettings()->GetPath(PATH_NSF).c_str(), NSFE_FILTER[0], NSFE_FILTER[1])) {		// // //
		CWaitCursor wait;

		// Collect header info
		int MachineType = 0;
		if (IsDlgButtonChecked(IDC_NTSC) == BST_CHECKED)
			MachineType = 0;
		else if (IsDlgButtonChecked(IDC_PAL) == BST_CHECKED)
			MachineType = 1;
		else if (IsDlgButtonChecked(IDC_DUAL) == BST_CHECKED)
			MachineType = 2;

		CCompiler Compiler(*pDoc->GetModule(), std::make_unique<CEditLog>(GetDlgItem(IDC_OUTPUT)));
		UpdateMetadata(Compiler);		// // //
		Compiler.ExportNSFE(*path, MachineType);
		theApp.GetSettings()->SetDirectory((LPCWSTR)*path, PATH_NSF);
	}
}

void CExportDialog::UpdateMetadata(CCompiler &compiler) {
	CStringW Name, Artist, Copyright;
	GetDlgItemTextW(IDC_NAME, Name);
	GetDlgItemTextW(IDC_ARTIST, Artist);
	GetDlgItemTextW(IDC_COPYRIGHT, Copyright);
	compiler.SetMetadata(conv::to_utf8(Name), conv::to_utf8(Artist), conv::to_utf8(Copyright));
}

void CExportDialog::CreateNES()
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();

	if (auto path = GetSavePath(pDoc->GetFileTitle(), theApp.GetSettings()->GetPath(PATH_NSF).c_str(), NES_FILTER[0], NES_FILTER[1])) {		// // //
		CWaitCursor wait;

		CCompiler Compiler(*pDoc->GetModule(), std::make_unique<CEditLog>(GetDlgItem(IDC_OUTPUT)));
		Compiler.ExportNES(*path, IsDlgButtonChecked(IDC_PAL) == BST_CHECKED);
		theApp.GetSettings()->SetDirectory((LPCWSTR)*path, PATH_NSF);
	}
}

void CExportDialog::CreateBIN()
{
	if (auto path = GetSavePath(L"music.bin", theApp.GetSettings()->GetPath(PATH_NSF).c_str(), RAW_FILTER[0], RAW_FILTER[1])) {		// // //
		CStringW SampleDir = *path;		// // //

		const CStringW DEFAULT_SAMPLE_NAME = L"samples.bin";		// // //

		CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
		if (pDoc->GetModule()->GetDSampleManager()->GetDSampleCount() > 0) {
			if (auto sampPath = GetSavePath(DEFAULT_SAMPLE_NAME, *path, DPCMS_FILTER[0], DPCMS_FILTER[1]))
				SampleDir = *sampPath;
			else
				return;
		}
		else {
			int Pos = SampleDir.ReverseFind(L'\\');
			ASSERT(Pos != -1);
			SampleDir = SampleDir.Left(Pos + 1) + DEFAULT_SAMPLE_NAME;
			if (PathFileExists(SampleDir)) {
				CStringW msg;
				AfxFormatString1(msg, IDS_EXPORT_SAMPLES_FILE, DEFAULT_SAMPLE_NAME);
				if (AfxMessageBox(msg, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO)
					return;
			}
		}

		// Display wait cursor
		CWaitCursor wait;

		CCompiler Compiler(*pDoc->GetModule(), std::make_unique<CEditLog>(GetDlgItem(IDC_OUTPUT)));
		Compiler.ExportBIN(*path, SampleDir);
		theApp.GetSettings()->SetDirectory((LPCWSTR)*path, PATH_NSF);
	}
}

void CExportDialog::CreatePRG()
{
	if (auto path = GetSavePath(L"music.prg", theApp.GetSettings()->GetPath(PATH_NSF).c_str(), PRG_FILTER[0], PRG_FILTER[1])) {		// // //
		CWaitCursor wait;

		CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
		CCompiler Compiler(*pDoc->GetModule(), std::make_unique<CEditLog>(GetDlgItem(IDC_OUTPUT)));
		Compiler.ExportPRG(*path, IsDlgButtonChecked(IDC_PAL) == BST_CHECKED);
		theApp.GetSettings()->SetDirectory((LPCWSTR)*path, PATH_NSF);
	}
}

void CExportDialog::CreateASM()
{
	if (auto path = GetSavePath(L"music.asm", theApp.GetSettings()->GetPath(PATH_NSF).c_str(), ASM_FILTER[0], ASM_FILTER[1])) {		// // //
		CWaitCursor wait;

		CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
		CCompiler Compiler(*pDoc->GetModule(), std::make_unique<CEditLog>(GetDlgItem(IDC_OUTPUT)));
		Compiler.ExportASM(*path);
		theApp.GetSettings()->SetDirectory((LPCWSTR)*path, PATH_NSF);
	}
}

void CExportDialog::OnBnClickedPlay()
{
#ifdef _DEBUG

//	if (m_strFile.GetLength() == 0)
//		return;

	const LPCWSTR file = L"d:\\test.nsf";		// // //

	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	CCompiler Compiler(*pDoc->GetModule(), std::make_unique<CEditLog>(GetDlgItem(IDC_OUTPUT)));
	Compiler.ExportNSF(file, IsDlgButtonChecked(IDC_PAL) == BST_CHECKED);

	// Play exported file (available in debug)
	ShellExecuteW(NULL, L"open", file, NULL, NULL, SW_SHOWNORMAL);

#endif
}
