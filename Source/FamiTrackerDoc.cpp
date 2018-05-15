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

#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"		// // //
#include "FamiTrackerEnv.h"		// // //
#include "FamiTrackerViewMessage.h"		// // //
#include "ModuleException.h"		// // //
#include "Settings.h"		// // //
#include "DocumentFile.h"
#include "SoundGen.h"
#include "SoundChipService.h"		// // //
#include "MainFrm.h"		// // //
#include "ChannelMap.h"		// // //
#include "FamiTrackerDocIO.h"		// // //
#include "FamiTrackerDocOldIO.h"		// // //
#include "str_conv/str_conv.hpp"		// // //

//
// CFamiTrackerDoc
//

IMPLEMENT_DYNCREATE(CFamiTrackerDoc, CDocument)

BEGIN_MESSAGE_MAP(CFamiTrackerDoc, CDocument)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
END_MESSAGE_MAP()

// CFamiTrackerDoc construction/destruction

CFamiTrackerDoc::CFamiTrackerDoc() :
	module_(std::make_unique<CFamiTrackerModule>())		// // //
{
	// Register this object to the sound generator
	if (CSoundGen *pSoundGen = Env.GetSoundGenerator())
		pSoundGen->AssignDocument(this);
}

CFamiTrackerDoc::~CFamiTrackerDoc()
{
	// // //
}

//
// Static functions
//

CFamiTrackerDoc *CFamiTrackerDoc::GetDoc()
{
	CFrameWnd *pFrame = static_cast<CFrameWnd *>(AfxGetMainWnd());
	ASSERT_VALID(pFrame);

	return static_cast<CFamiTrackerDoc*>(pFrame->GetActiveDocument());
}

//
// Overrides
//

BOOL CFamiTrackerDoc::OnNewDocument()
{
	// Called by the GUI to create a new file

	// This calls DeleteContents
	if (!CDocument::OnNewDocument())
		return FALSE;

	CreateEmpty();

	return TRUE;
}

BOOL CFamiTrackerDoc::OnOpenDocument(LPCWSTR lpszPathName)
{
	// This function is called by the GUI to load a file

	//DeleteContents();
	Env.GetSoundGenerator()->ResetDumpInstrument();
	Env.GetSoundGenerator()->SetRecordChannel({ });		// // //

	// Load file
	if (!Locked([&] { return OpenDocument(lpszPathName); })) {		// // //
		// Loading failed, create empty document
		/*
		DeleteContents();
		CreateEmpty();
		for (int i = UPDATE_TRACK; i <= UPDATE_COLUMNS; ++i)		// // // test
			UpdateAllViews(NULL, i);
		*/
		// and tell doctemplate that loading failed
		return FALSE;
	}

	// Update main frame
	Env.GetSoundGenerator()->ModuleChipChanged();		// // //

#ifdef AUTOSAVE
	SetupAutoSave();
#endif

	// Remove modified flag
	SetModifiedFlag(FALSE);
	SetExceededFlag(FALSE);		// // //

	Env.GetMainFrame()->SelectTrack(0);		// // //

	return TRUE;
}

BOOL CFamiTrackerDoc::OnSaveDocument(LPCWSTR lpszPathName)
{
#ifdef DISABLE_SAVE		// // //
	static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(IDS_DISABLE_SAVE);
	return FALSE;
#endif

	// This function is called by the GUI to save the file

	if (!IsFileLoaded())
		return FALSE;

	// File backup, now performed on save instead of open
	if ((m_bForceBackup || Env.GetSettings()->General.bBackups) && !m_bBackupDone) {
		CopyFileW(lpszPathName, FormattedW(L"%s.bak", lpszPathName), FALSE);
		m_bBackupDone = true;
	}

	if (!SaveDocument(lpszPathName))
		return FALSE;

	// Reset modified flag
	SetModifiedFlag(FALSE);

	SetExceededFlag(FALSE);		// // //

	return TRUE;
}

void CFamiTrackerDoc::OnCloseDocument()
{
	// Document object is about to be deleted

	// Remove itself from sound generator
	CSoundGen *pSoundGen = Env.GetSoundGenerator();

	if (pSoundGen)
		pSoundGen->RemoveDocument();

	CDocument::OnCloseDocument();
}

void CFamiTrackerDoc::DeleteContents()
{
	// Current document is being unloaded, clear and reset variables and memory
	// Delete everything because the current object is being reused in SDI

	// Make sure player is stopped
	Env.GetSoundGenerator()->StopPlayer();		// // //
	Env.GetSoundGenerator()->WaitForStop();

	Locked([&] {		// // //
		// Mark file as unloaded
		m_bFileLoaded = false;
		m_bForceBackup = false;
		m_bBackupDone = true;	// No backup on new modules

		UpdateAllViews(NULL, UPDATE_CLOSE);	// TODO remove
		module_ = std::make_unique<CFamiTrackerModule>();		// // //
		Env.GetSoundGenerator()->DocumentPropertiesChanged(this);		// // // rebind module
		Env.GetSoundGenerator()->ModuleChipChanged();

#ifdef AUTOSAVE
		ClearAutoSave();
#endif

		// Remove modified flag
		SetModifiedFlag(FALSE);
		SetExceededFlag(FALSE);		// // //
	});

	CDocument::DeleteContents();
}

void CFamiTrackerDoc::SetModifiedFlag(BOOL bModified)
{
	// Trigger auto-save in 10 seconds
#ifdef AUTOSAVE
	if (bModified)
		m_iAutoSaveCounter = 10;
#endif

	BOOL bWasModified = IsModified();
	CDocument::SetModifiedFlag(bModified);

	if (auto *pFrameWnd = dynamic_cast<CFrameWnd *>(Env.GetMainApp()->m_pMainWnd))		// // //
		if (pFrameWnd->GetActiveDocument() == this && bWasModified != bModified)
			pFrameWnd->OnUpdateFrameTitle(TRUE);
}

void CFamiTrackerDoc::CreateEmpty()
{
	DeleteContents();		// // //

	Locked([&] {		// // //
		// and select 2A03 only
		GetModule()->SetChannelMap(Env.GetSoundChipService()->MakeChannelMap(sound_chip_t::APU, 0));		// // //

#ifdef AUTOSAVE
		SetupAutoSave();
#endif

		// Document is avaliable
		m_bFileLoaded = true;
	});

	Env.GetSoundGenerator()->ModuleChipChanged();
	Env.GetSoundGenerator()->DocumentPropertiesChanged(this);
}


bool CFamiTrackerDoc::GetExceededFlag() const {		// // //
	return m_bExceeded;
}

void CFamiTrackerDoc::SetExceededFlag(bool Exceed) {		// // //
	m_bExceeded = Exceed;
}

void CFamiTrackerDoc::Modify(bool Change) {
	SetModifiedFlag(Change ? TRUE : FALSE);
}

void CFamiTrackerDoc::ModifyIrreversible() {
	SetModifiedFlag(TRUE);
	SetExceededFlag(TRUE);
}
//
// Messages
//

void CFamiTrackerDoc::OnFileSave()
{
#ifdef DISABLE_SAVE		// // //
	static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(IDS_DISABLE_SAVE);
	return;
#endif

	if (GetPathName().GetLength() == 0)
		OnFileSaveAs();
	else
		CDocument::OnFileSave();
}

void CFamiTrackerDoc::OnFileSaveAs()
{
#ifdef DISABLE_SAVE		// // //
	static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(IDS_DISABLE_SAVE);
	return;
#endif

	// Overloaded in order to save the ftm-path
	CStringW newName = GetPathName();

	if (!AfxGetApp()->DoPromptFileName(newName, AFX_IDS_SAVEFILE, OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, FALSE, NULL))
		return;

	Env.GetSettings()->SetPath(fs::path {(LPCWSTR)newName}.parent_path(), PATH_FTM);

	DoSave(newName);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Document store functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CFamiTrackerDoc::SaveDocument(LPCWSTR lpszPathName) const
{
	CDocumentFile DocumentFile;
	CFileException ex;

	// First write to a temp file (if saving fails, the original is not destroyed)
	fs::path TempPath = fs::temp_directory_path();
	WCHAR TempFile[MAX_PATH] = { };
	GetTempFileNameW(TempPath.c_str(), L"FTM", 0, TempFile);

	try {		// // //
		DocumentFile.Open(TempFile, std::ios::out | std::ios::binary);
	}
	catch (std::runtime_error err) {
		AfxMessageBox(FormattedW(L"Could not save file: %s", conv::to_wide(err.what()).data()), MB_OK | MB_ICONERROR);
		return FALSE;
	}

	if (!CFamiTrackerDocIO {DocumentFile, (module_error_level_t)Env.GetSettings()->Version.iErrorLevel}.Save(*GetModule())) {		// // //
		// The save process failed, delete temp file
		DocumentFile.Close();
		fs::remove(TempFile);
		// Display error
		AfxMessageBox(CStringW(MAKEINTRESOURCEW(IDS_SAVE_ERROR)), MB_OK | MB_ICONERROR);
		return FALSE;
	}

	DocumentFile.Close();

	// Save old creation date
	CFileStatus stat;		// // //
	CFile::GetStatus(lpszPathName, stat);
	auto creationTime = stat.m_ctime;

	// Everything is done and the program cannot crash at this point
	// Replace the original

	if (!fs::copy_file(TempFile, lpszPathName, fs::copy_options::overwrite_existing)) {
		// Display message if saving failed
		AfxDebugBreak();		// // //
		LPWSTR lpMsgBuf;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL);
		AfxMessageBox(FormattedW(L"Could not save file: %s", lpMsgBuf), MB_OK | MB_ICONERROR);
		LocalFree(lpMsgBuf);
		// Remove temp file
		fs::remove(TempFile);
		return FALSE;
	}
	fs::remove(TempFile);

	// Restore creation date
	CFile::GetStatus(lpszPathName, stat);
	stat.m_ctime = creationTime;
	CFile::SetStatus(lpszPathName, stat);

	if (auto *pMainFrame = static_cast<CFrameWnd *>(AfxGetMainWnd()))		// // //
		pMainFrame->SetMessageText(AfxFormattedW(IDS_FILE_SAVED, conv::to_wide(std::to_string(fs::file_size(lpszPathName))).data()));

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Document load functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CFamiTrackerDoc::OpenDocument(LPCWSTR lpszPathName)
{
	m_bFileLoadFailed = true;

	CFileException ex;
	CDocumentFile  OpenFile;

	// Check if empty file
	if (!fs::file_size(lpszPathName)) {		// // //
		// Setup default settings
		CreateEmpty();
		return TRUE;
	}

	// Open file
	try {		// // //
		OpenFile.Open(lpszPathName, std::ios::in | std::ios::binary);
	}
	catch (std::runtime_error err) {
		AfxMessageBox(FormattedW(L"Could not open file: %s", conv::to_wide(err.what()).data()), MB_OK | MB_ICONERROR);
		//OnNewDocument();
		return FALSE;
	}

	try {		// // //
		// Read header ID and version
		OpenFile.ValidateFile();

		m_iFileVersion = OpenFile.GetFileVersion();
		DeleteContents();		// // //

		if (m_iFileVersion < 0x0200U) {
			if (!compat::OpenDocumentOld(*GetModule(), OpenFile.GetCSimpleFile()))
				OpenFile.RaiseModuleException("General error");

			// Create a backup of this file, since it's an old version
			// and something might go wrong when converting
			m_bForceBackup = true;
		}
		else {
			if (!CFamiTrackerDocIO {OpenFile, (module_error_level_t)Env.GetSettings()->Version.iErrorLevel}.Load(*GetModule()))
				OpenFile.RaiseModuleException((LPCSTR)CStringA(MAKEINTRESOURCEA(IDS_FILE_LOAD_ERROR)));
		}
	}
	catch (CModuleException &e) {
		AfxMessageBox(conv::to_wide(e.GetErrorString()).data(), MB_ICONERROR);
		DeleteContents();
		return FALSE;
	}

	// File is loaded
	m_bFileLoaded = true;
	m_bFileLoadFailed = false;
	m_bBackupDone = false;		// // //

	Env.GetSoundGenerator()->ModuleChipChanged();		// // //
	Env.GetSoundGenerator()->DocumentPropertiesChanged(this);

	return TRUE;
}

std::unique_ptr<CFamiTrackerDoc> CFamiTrackerDoc::LoadImportFile(LPCWSTR lpszPathName) {		// // //
	// Import a module as new subtunes
	auto pImported = std::make_unique<CFamiTrackerDoc>(ctor_t { });

	pImported->CreateEmpty();

	// Load into a new document
	if (!pImported->OpenDocument(lpszPathName))
		return nullptr;

	return pImported;
}

// Attributes

CFamiTrackerModule *CFamiTrackerDoc::GetModule() noexcept {		// // //
	return module_.get();
}

const CFamiTrackerModule *CFamiTrackerDoc::GetModule() const noexcept {
	return module_.get();
}

fs::path CFamiTrackerDoc::GetFileTitle() const
{
	// Return file name without extension
	fs::path FileName = static_cast<LPCWSTR>(GetTitle());		// // //
	FileName.replace_extension();

	if (FileName.extension() == ".bak")
		FileName.replace_extension();
	if (FileName.extension() == ".ftm" || FileName.extension() == ".0cc")
		FileName.replace_extension();

	return FileName;
}

bool CFamiTrackerDoc::IsFileLoaded() const
{
	return m_bFileLoaded;
}

bool CFamiTrackerDoc::HasLastLoadFailed() const
{
	return m_bFileLoadFailed;
}

#ifdef AUTOSAVE

// Auto-save (experimental)

void CFamiTrackerDoc::SetupAutoSave()
{
	WCHAR TempPath[MAX_PATH], TempFile[MAX_PATH];

	GetTempPathW(MAX_PATH, TempPath);
	GetTempFileNameW(TempPath, L"Aut", 21587, TempFile);

	// Check if file exists
	CFile file;
	if (file.Open(TempFile, CFile::modeRead)) {
		file.Close();
		if (AfxMessageBox(L"It might be possible to recover last document, do you want to try?", MB_YESNO) == IDYES) {
			OpenDocument(TempFile);
			SelectExpansionChip(GetModule()->GetSoundChipSet(), GetModule()->GetNamcoChannels());
		}
		else {
			fs::remove(TempFile);
		}
	}

	TRACE(L"Doc: Allocated file for auto save: ");
	TRACE(TempFile);
	TRACE(L"\n");

	m_sAutoSaveFile = TempFile;
}

void CFamiTrackerDoc::ClearAutoSave()
{
	if (m_sAutoSaveFile.GetLength() == 0)
		return;

	fs::remove(m_sAutoSaveFile);

	m_sAutoSaveFile = L"";
	m_iAutoSaveCounter = 0;

	TRACE(L"Doc: Removed auto save file\n");
}

void CFamiTrackerDoc::AutoSave()
{
	// Autosave
	if (!m_iAutoSaveCounter || !m_bFileLoaded || m_sAutoSaveFile.GetLength() == 0)
		return;

	--m_iAutoSaveCounter;

	if (m_iAutoSaveCounter == 0) {
		TRACE(L"Doc: Performing auto save\n");
		SaveDocument(m_sAutoSaveFile);
	}
}

#endif
