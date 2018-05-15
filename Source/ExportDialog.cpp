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
#include "FamiTrackerEnv.h"		// // //
#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"		// // //
#include "DSampleManager.h"		// // //
#include "Compiler.h"
#include "Settings.h"
#include "FileDialogs.h"		// // //
#include "SimpleFile.h"		// // //
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
const LPCWSTR CExportDialog::DEFAULT_FILTERS[][2] = {
	{L"NSF file (*.nsf)"        , L"*.nsf" },
	{L"NES ROM image (*.nes)"   , L"*.nes" },
	{L"Raw song data (*.bin)"   , L"*.bin" },
	{L"NES program bank (*.prg)", L"*.prg" },
	{L"Assembly text (*.asm)"   , L"*.asm" },
	{L"NSFe file (*.nsfe)"      , L"*.nsfe"},		// // //
};

const LPCWSTR CExportDialog::DPCMS_FILTER[] = {L"DPCM sample bank (*.bin)", L"*.bin"};

// Compiler logger

class CEditLog : public CCompilerLog
{
public:
	explicit CEditLog(CWnd *pEdit) : m_pEdit(static_cast<CEdit *>(pEdit)) { }
	void WriteLog(std::string_view text) override {		// // //
		int Len = m_pEdit->GetWindowTextLengthW();
		m_pEdit->SetSel(Len, Len, 0);
		m_pEdit->ReplaceSel(conv::to_wide(text).data(), 0);
		m_pEdit->RedrawWindow();
	}
	void Clear() override {
		m_pEdit->SetWindowTextW(L"");
		m_pEdit->RedrawWindow();
	}
private:
	CEdit *m_pEdit;
};

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
	m_cExportTypes.SubclassDlgItem(IDC_TYPE, this);		// // //

	// Add built in exporters
	for (int i = 0; i < DEFAULT_EXPORTERS; ++i)
		m_cExportTypes.AddString(DEFAULT_EXPORT_NAMES[i]);

	// // //

	// Set default selection
	m_cExportTypes.SetCurSel(m_iExportOption);

#ifdef _DEBUG
	GetDlgItem(IDC_PLAY)->ShowWindow(SW_SHOW);
#endif

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CExportDialog::OnBnClickedExport()
{
	CStringW ItemText;

	m_iExportOption = m_cExportTypes.GetCurSel();
	m_cExportTypes.GetLBText(m_iExportOption, ItemText);

	// Check built in exporters
	if (m_iExportOption < DEFAULT_EXPORTERS) {		// // //
		(this->*DEFAULT_EXPORT_FUNCS[m_iExportOption])();
	}
}

std::optional<CSimpleFile> CExportDialog::OpenFile(const fs::path &fileName) {		// // //
	CSimpleFile f {fileName, std::ios::out | std::ios::binary};
	if (!f) {
		AfxMessageBox(FormattedW(L"Error: Could not open output file: %s\n",
			conv::to_wide(f.GetErrorMessage()).data()), MB_ICONERROR);
		return std::nullopt;
	}

	return f;
}

template <typename F>
void CExportDialog::WithFile(const fs::path &initFName, F f) {		// // //
	WithFile(initFName, DEFAULT_FILTERS[m_iExportOption][0], DEFAULT_FILTERS[m_iExportOption][1], f);
}

template <typename F>
void CExportDialog::WithFile(const fs::path &initFName, const CStringW &filterName, const CStringW &filterExt, F f) {
//	CStringW title = CFamiTrackerDoc::GetDoc()->GetFileTitle();
//	CStringW initPath = Env.GetSettings()->GetPath(PATH_NSF).c_str();
	if (auto path = GetSavePath(initFName, Env.GetSettings()->GetPath(PATH_NSF).c_str(), filterName, filterExt)) {		// // //
		if (auto file = OpenFile(*path)) {
			f(*file);
			Env.GetSettings()->SetPath(path->parent_path(), PATH_NSF);
		}
	}
}

void CExportDialog::CreateNSF()
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();

	WithFile(pDoc->GetFileTitle(), [&] (CSimpleFile &OutputFile) {
		CWaitCursor wait;

		CCompiler Compiler(*pDoc->GetModule(), std::make_unique<CEditLog>(GetDlgItem(IDC_OUTPUT)));
		UpdateMetadata(Compiler);		// // //
		Compiler.ExportNSF(OutputFile, GetMachineType());
	});
}

void CExportDialog::CreateNSFe()		// // //
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();

	WithFile(pDoc->GetFileTitle(), [&] (CSimpleFile &OutputFile) {
		CWaitCursor wait;

		CCompiler Compiler(*pDoc->GetModule(), std::make_unique<CEditLog>(GetDlgItem(IDC_OUTPUT)));
		UpdateMetadata(Compiler);		// // //
		Compiler.ExportNSFE(OutputFile, GetMachineType());
	});
}

int CExportDialog::GetMachineType() const {		// // //
	if (IsDlgButtonChecked(IDC_NTSC) == BST_CHECKED)
		return 0;
	else if (IsDlgButtonChecked(IDC_PAL) == BST_CHECKED)
		return 1;
	else if (IsDlgButtonChecked(IDC_DUAL) == BST_CHECKED)
		return 2;
	return 0;
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

	WithFile(pDoc->GetFileTitle(), [&] (CSimpleFile &OutputFile) {
		CWaitCursor wait;

		CCompiler Compiler(*pDoc->GetModule(), std::make_unique<CEditLog>(GetDlgItem(IDC_OUTPUT)));
		Compiler.ExportNES(OutputFile, IsDlgButtonChecked(IDC_PAL) == BST_CHECKED);
	});
}

void CExportDialog::CreateBIN()
{
	if (auto path = GetSavePath("music.bin", Env.GetSettings()->GetPath(PATH_NSF).c_str(), DEFAULT_FILTERS[m_iExportOption][0], DEFAULT_FILTERS[m_iExportOption][1])) {		// // //
		if (auto BINFile = OpenFile(*path)) {
			const fs::path DEFAULT_SAMPLE_NAME = "samples.bin";		// // //
			fs::path samplePath = path->parent_path() / DEFAULT_SAMPLE_NAME;

			CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
			if (pDoc->GetModule()->GetDSampleManager()->GetDSampleCount() > 0) {
				if (auto sampPath = GetSavePath(DEFAULT_SAMPLE_NAME, *path, DPCMS_FILTER[0], DPCMS_FILTER[1]))
					samplePath = *sampPath;
				else
					return;
			}
			else if (fs::exists(samplePath)) {
				if (AfxMessageBox(AfxFormattedW(IDS_EXPORT_SAMPLES_FILE, DEFAULT_SAMPLE_NAME.c_str()), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO)
					return;
			}

			if (auto DPCMFile = OpenFile(samplePath)) {
				// Display wait cursor
				CWaitCursor wait;

				CCompiler Compiler(*pDoc->GetModule(), std::make_unique<CEditLog>(GetDlgItem(IDC_OUTPUT)));
				Compiler.ExportBIN(*BINFile, *DPCMFile);
				Env.GetSettings()->SetPath(path->parent_path(), PATH_NSF);
			}
		}
	}
}

void CExportDialog::CreatePRG()
{
	WithFile(L"music.prg", [&] (CSimpleFile &OutputFile) {
		CWaitCursor wait;

		CCompiler Compiler(*CFamiTrackerDoc::GetDoc()->GetModule(), std::make_unique<CEditLog>(GetDlgItem(IDC_OUTPUT)));
		Compiler.ExportPRG(OutputFile, IsDlgButtonChecked(IDC_PAL) == BST_CHECKED);
	});
}

void CExportDialog::CreateASM()
{
	WithFile(L"music.asm", [&] (CSimpleFile &OutputFile) {
		CWaitCursor wait;

		CCompiler Compiler(*CFamiTrackerDoc::GetDoc()->GetModule(), std::make_unique<CEditLog>(GetDlgItem(IDC_OUTPUT)));
		Compiler.ExportASM(OutputFile);
	});
}

void CExportDialog::OnBnClickedPlay()
{
#ifdef _DEBUG
	// Play exported file (available in debug)
	const LPCWSTR fname = L"d:\\test.nsf";		// // //

	if (auto file = OpenFile(fname)) {
		CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
		CCompiler Compiler(*pDoc->GetModule(), std::make_unique<CEditLog>(GetDlgItem(IDC_OUTPUT)));
		Compiler.ExportNSF(*file, IsDlgButtonChecked(IDC_PAL) == BST_CHECKED);
		ShellExecuteW(NULL, L"open", fname, NULL, NULL, SW_SHOWNORMAL);
	}
#endif
}
