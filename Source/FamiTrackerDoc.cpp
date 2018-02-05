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
#include <algorithm>
#include "FamiTracker.h"
#include "FamiTrackerViewMessage.h"		// // //
#include "Instrument.h"		// // //
#include "SeqInstrument.h"		// // //
#include "Instrument2A03.h"		// // //
#include "ModuleException.h"		// // //
#include "DocumentFile.h"
#include "SoundGen.h"
#include "SequenceCollection.h"		// // //
#include "SequenceManager.h"		// // //
#include "DSampleManager.h"			// // //
#include "InstrumentManager.h"		// // //
#include "APU/APU.h"
#include "APU/Types.h"		// // //
#include "SimpleFile.h"		// // //
#include "ChannelMap.h"		// // //
#include "FamiTrackerDocIO.h"		// // //
#include "SongData.h"		// // //
#include "SongView.h"		// // //
#include "FamiTrackerDocOldIO.h"		// // //
#include "NumConv.h"		// // //

#include "Sequence.h"		// // //

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
	module_(std::make_unique<CFamiTrackerModule>(*this))		// // //
{
	// Register this object to the sound generator
	if (CSoundGen *pSoundGen = theApp.GetSoundGenerator())
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

// Synchronization
BOOL CFamiTrackerDoc::LockDocument() const
{
	return m_csDocumentLock.Lock();
}

BOOL CFamiTrackerDoc::LockDocument(DWORD dwTimeout) const
{
	return m_csDocumentLock.Lock(dwTimeout);
}

BOOL CFamiTrackerDoc::UnlockDocument() const
{
	return m_csDocumentLock.Unlock();
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

BOOL CFamiTrackerDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	// This function is called by the GUI to load a file

	//DeleteContents();
	theApp.GetSoundGenerator()->ResetDumpInstrument();
	theApp.GetSoundGenerator()->SetRecordChannel(chan_id_t::NONE);		// // //

	LockDocument();

	// Load file
	if (!OpenDocument(lpszPathName)) {
		// Loading failed, create empty document
		UnlockDocument();
		/*
		DeleteContents();
		CreateEmpty();
		for (int i = UPDATE_TRACK; i <= UPDATE_COLUMNS; ++i)		// // // test
			UpdateAllViews(NULL, i);
		*/
		// and tell doctemplate that loading failed
		return FALSE;
	}

	UnlockDocument();

	// Update main frame
	ApplyExpansionChip();

#ifdef AUTOSAVE
	SetupAutoSave();
#endif

	// Remove modified flag
	SetModifiedFlag(FALSE);

	SetExceededFlag(FALSE);		// // //

	return TRUE;
}

BOOL CFamiTrackerDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
#ifdef DISABLE_SAVE		// // //
	static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(IDS_DISABLE_SAVE);
	return FALSE;
#endif

	// This function is called by the GUI to save the file

	if (!IsFileLoaded())
		return FALSE;

	// File backup, now performed on save instead of open
	if ((m_bForceBackup || theApp.GetSettings()->General.bBackups) && !m_bBackupDone) {
		CString BakName;
		BakName.Format(_T("%s.bak"), lpszPathName);
		CopyFile(lpszPathName, BakName.GetString(), FALSE);
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
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	if (pSoundGen)
		pSoundGen->RemoveDocument();

	CDocument::OnCloseDocument();
}

void CFamiTrackerDoc::DeleteContents()
{
	// Current document is being unloaded, clear and reset variables and memory
	// Delete everything because the current object is being reused in SDI

	// Make sure player is stopped
	theApp.StopPlayerAndWait();

	LockDocument();

	// Mark file as unloaded
	m_bFileLoaded = false;
	m_bForceBackup = false;
	m_bBackupDone = true;	// No backup on new modules

	UpdateAllViews(NULL, UPDATE_CLOSE);	// TODO remove
	module_ = std::make_unique<CFamiTrackerModule>(*this);		// // //

#ifdef AUTOSAVE
	ClearAutoSave();
#endif

	// Remove modified flag
	SetModifiedFlag(FALSE);
	SetExceededFlag(FALSE);		// // //

	UnlockDocument();

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

	CFrameWnd *pFrameWnd = dynamic_cast<CFrameWnd*>(theApp.m_pMainWnd);
	if (pFrameWnd != NULL) {
		if (pFrameWnd->GetActiveDocument() == this && bWasModified != bModified) {
			pFrameWnd->OnUpdateFrameTitle(TRUE);
		}
	}
}

void CFamiTrackerDoc::CreateEmpty()
{
	DeleteContents();		// // //

	LockDocument();

	// and select 2A03 only
	SelectExpansionChip(sound_chip_t::APU, 0);		// // //
	SetModifiedFlag(FALSE);
	SetExceededFlag(FALSE);		// // //

#ifdef AUTOSAVE
	SetupAutoSave();
#endif

	// Document is avaliable
	m_bFileLoaded = true;

	UnlockDocument();

	theApp.GetSoundGenerator()->DocumentPropertiesChanged(this);
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
	CString newName = GetPathName();

	if (!AfxGetApp()->DoPromptFileName(newName, AFX_IDS_SAVEFILE, OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, FALSE, NULL))
		return;

	theApp.GetSettings()->SetPath(newName, PATH_FTM);

	DoSave(newName);
}


// CFamiTrackerDoc diagnostics

#ifdef _DEBUG
void CFamiTrackerDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CFamiTrackerDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CFamiTrackerDoc commands

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Document store functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CFamiTrackerDoc::SaveDocument(LPCTSTR lpszPathName) const
{
	CDocumentFile DocumentFile;
	CFileException ex;
	TCHAR TempPath[MAX_PATH], TempFile[MAX_PATH];

	// First write to a temp file (if saving fails, the original is not destroyed)
	GetTempPath(MAX_PATH, TempPath);
	GetTempFileName(TempPath, _T("FTM"), 0, TempFile);

	if (!DocumentFile.Open(TempFile, CFile::modeWrite | CFile::modeCreate, &ex)) {
		// Could not open file
		TCHAR szCause[255];
		CString strFormatted;
		ex.GetErrorMessage(szCause, 255);
		AfxFormatString1(strFormatted, IDS_SAVE_FILE_ERROR, szCause);
		AfxMessageBox(strFormatted, MB_OK | MB_ICONERROR);
		return FALSE;
	}

	if (!CFamiTrackerDocIO {DocumentFile}.Save(*this)) {		// // //
		// The save process failed, delete temp file
		DocumentFile.Close();
		DeleteFile(TempFile);
		// Display error
		CString	ErrorMsg;
		ErrorMsg.LoadString(IDS_SAVE_ERROR);
		AfxMessageBox(ErrorMsg, MB_OK | MB_ICONERROR);
		return FALSE;
	}

	ULONGLONG FileSize = DocumentFile.GetLength();

	DocumentFile.Close();

	// Save old creation date
	HANDLE hOldFile;
	FILETIME creationTime;

	hOldFile = CreateFile(lpszPathName, FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	GetFileTime(hOldFile, &creationTime, NULL, NULL);
	CloseHandle(hOldFile);

	// Everything is done and the program cannot crash at this point
	// Replace the original
	if (!MoveFileEx(TempFile, lpszPathName, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
		// Display message if saving failed
		AfxDebugBreak();		// // //
		LPTSTR lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
		CString	strFormatted;
		AfxFormatString1(strFormatted, IDS_SAVE_FILE_ERROR, lpMsgBuf);
		AfxMessageBox(strFormatted, MB_OK | MB_ICONERROR);
		LocalFree(lpMsgBuf);
		// Remove temp file
		DeleteFile(TempFile);
		return FALSE;
	}

	// Restore creation date
	hOldFile = CreateFile(lpszPathName, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	SetFileTime(hOldFile, &creationTime, NULL, NULL);
	CloseHandle(hOldFile);

	// Todo: avoid calling the main window from document class
	if (CFrameWnd *pMainFrame = static_cast<CFrameWnd*>(AfxGetMainWnd())) {		// // //
		CString text;
		AfxFormatString1(text, IDS_FILE_SAVED, std::to_string(FileSize).c_str());		// // //
		pMainFrame->SetMessageText(text);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Document load functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CFamiTrackerDoc::OpenDocument(LPCTSTR lpszPathName)
{
	m_bFileLoadFailed = true;

	CFileException ex;
	CDocumentFile  OpenFile;

	// Open file
	if (!OpenFile.Open(lpszPathName, CFile::modeRead | CFile::shareDenyWrite, &ex)) {
		TCHAR   szCause[1024];		// // //
		CString strFormatted;
		ex.GetErrorMessage(szCause, sizeof(szCause));
		strFormatted = _T("Could not open file.\n\n");
		strFormatted += szCause;
		AfxMessageBox(strFormatted);
		//OnNewDocument();
		return FALSE;
	}

	// Check if empty file
	if (OpenFile.GetLength() == 0) {
		// Setup default settings
		CreateEmpty();
		return TRUE;
	}

	try {		// // //
		// Read header ID and version
		OpenFile.ValidateFile();

		m_iFileVersion = OpenFile.GetFileVersion();
		DeleteContents();		// // //

		if (m_iFileVersion < 0x0200U) {
			if (!OpenDocumentOld(&OpenFile))
				OpenFile.RaiseModuleException("General error");

			// Create a backup of this file, since it's an old version
			// and something might go wrong when converting
			m_bForceBackup = true;
		}
		else {
			if (!OpenDocumentNew(OpenFile))
				OpenFile.RaiseModuleException("General error");

			// Backup if files was of an older version
			m_bForceBackup = m_iFileVersion < CDocumentFile::FILE_VER;
		}
	}
	catch (CModuleException e) {
		AfxMessageBox(e.GetErrorString().c_str(), MB_ICONERROR);
		return FALSE;
	}

	// File is loaded
	m_bFileLoaded = true;
	m_bFileLoadFailed = false;
	m_bBackupDone = false;		// // //

	theApp.GetSoundGenerator()->DocumentPropertiesChanged(this);

	return TRUE;
}

/**
 * This function reads the old obsolete file version.
 */
BOOL CFamiTrackerDoc::OpenDocumentOld(CFile *pOpenFile)
{
	return compat::OpenDocumentOld(*this, pOpenFile);		// // //
}

/**
 *  This function opens the most recent file version
 *
 */
BOOL CFamiTrackerDoc::OpenDocumentNew(CDocumentFile &DocumentFile)
{
	if (!CFamiTrackerDocIO {DocumentFile}.Load(*this)) {
		DocumentFile.Close();
		AfxMessageBox(IDS_FILE_LOAD_ERROR, MB_ICONERROR);
		return FALSE;
	}

	DocumentFile.Close();
	return TRUE;
}

// FTM import ////

std::unique_ptr<CFamiTrackerDoc> CFamiTrackerDoc::LoadImportFile(LPCTSTR lpszPathName) {		// // //
	// Import a module as new subtunes
	auto pImported = std::make_unique<CFamiTrackerDoc>(ctor_t { });

	pImported->CreateEmpty();

	// Load into a new document
	if (!pImported->OpenDocument(lpszPathName))
		return nullptr;

	return pImported;
}

bool CFamiTrackerDoc::ImportInstruments(CFamiTrackerModule &Imported, int *pInstTable)		// // //
{
	// Copy instruments to current module
	//
	// pInstTable must point to an int array of size MAX_INSTRUMENTS
	//

	int SamplesTable[MAX_DSAMPLES] = { };
	int SequenceTable2A03[MAX_SEQUENCES][SEQ_COUNT] = { };
	int SequenceTableVRC6[MAX_SEQUENCES][SEQ_COUNT] = { };
	int SequenceTableN163[MAX_SEQUENCES][SEQ_COUNT] = { };
	int SequenceTableS5B[MAX_SEQUENCES][SEQ_COUNT] = { };		// // //

	auto *pManager = GetModule()->GetInstrumentManager();
	auto *pImportedInst = Imported.GetInstrumentManager();

	// Check instrument count
	if (pManager->GetInstrumentCount() + pImportedInst->GetInstrumentCount() > MAX_INSTRUMENTS) {
		// Out of instrument slots
		AfxMessageBox(IDS_IMPORT_INSTRUMENT_COUNT, MB_ICONERROR);
		return false;
	}

	static const inst_type_t inst[] = {INST_2A03, INST_VRC6, INST_N163, INST_S5B};		// // //
	int (*seqTable[])[SEQ_COUNT] = {SequenceTable2A03, SequenceTableVRC6, SequenceTableN163, SequenceTableS5B};

	// Copy sequences
	for (size_t i = 0; i < std::size(inst); i++) for (int t = 0; t < SEQ_COUNT; ++t) {
		if (pManager->GetSequenceCount(inst[i], (sequence_t)t) + pImportedInst->GetSequenceCount(inst[i], (sequence_t)t) > MAX_SEQUENCES) {		// // //
			AfxMessageBox(IDS_IMPORT_SEQUENCE_COUNT, MB_ICONERROR);
			return false;
		}
	}
	for (size_t i = 0; i < std::size(inst); i++) foreachSeq([&] (sequence_t t) {
		for (unsigned int s = 0; s < MAX_SEQUENCES; ++s)
			if (auto pImportSeq = pImportedInst->GetSequence(inst[i], t, s); pImportSeq && pImportSeq->GetItemCount() > 0) {
				int index = -1;
				for (unsigned j = 0; j < MAX_SEQUENCES; ++j) {
					auto pSeq = pManager->GetSequence(inst[i], t, j);
					if (pSeq && pSeq->GetItemCount() > 0)
						continue;
					// TODO: continue if blank sequence is used by some instrument
					*pSeq = *pImportSeq;		// // //
					// Save a reference to this sequence
					seqTable[i][s][value_cast(t)] = j;
					break;
				}
			}
	});

	bool bOutOfSampleSpace = false;
	auto *pDSampleManager = GetModule()->GetDSampleManager();
	auto *pImportedSamps = Imported.GetDSampleManager();		// // //

	// Copy DPCM samples
	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (auto pImportDSample = pImportedSamps->ReleaseDSample(i)) {		// // //
			int Index = pDSampleManager->GetFirstFree();
			if (Index != -1) {
				pDSampleManager->SetDSample(Index, pImportDSample);
				// Save a reference to this DPCM sample
				SamplesTable[i] = Index;
			}
			else
				bOutOfSampleSpace = true;
		}
	}

	if (bOutOfSampleSpace) {
		// Out of sample space
		AfxMessageBox(IDS_IMPORT_SAMPLE_SLOTS, MB_ICONEXCLAMATION);
		return false;
	}

	// Copy instruments
	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (pImportedInst->IsInstrumentUsed(i)) {
			auto pInst = pImportedInst->GetInstrument(i)->Clone();		// // //

			// Update references
			if (auto pSeq = dynamic_cast<CSeqInstrument *>(pInst.get())) {
				foreachSeq([&] (sequence_t t) {
					if (pSeq->GetSeqEnable(t)) {
						for (size_t j = 0; j < std::size(inst); j++)
							if (inst[j] == pInst->GetType()) {
								pSeq->SetSeqIndex(t, seqTable[j][pSeq->GetSeqIndex(t)][value_cast(t)]);
								break;
							}
					}
				});
				// Update DPCM samples
				if (auto p2A03 = dynamic_cast<CInstrument2A03 *>(pSeq))
					for (int o = 0; o < OCTAVE_RANGE; ++o) for (int n = 0; n < NOTE_RANGE; ++n) {
						int Sample = p2A03->GetSampleIndex(o, n);
						if (Sample != 0)
							p2A03->SetSampleIndex(o, n, SamplesTable[Sample - 1] + 1);
					}
			}

			int Index = pManager->GetFirstUnused();
			pManager->InsertInstrument(Index, std::move(pInst));		// // //
			// Save a reference to this instrument
			pInstTable[i] = Index;
		}
	}

	return true;
}

bool CFamiTrackerDoc::ImportGrooves(CFamiTrackerModule &Imported, int *pGrooveMap)		// // //
{
	int Index = 0;
	for (int i = 0; i < MAX_GROOVE; ++i) {
		if (auto pGroove = Imported.GetGroove(i)) {
			while (GetModule()->HasGroove(Index))
				if (++Index >= MAX_GROOVE) {
					AfxMessageBox(IDS_IMPORT_GROOVE_SLOTS, MB_ICONEXCLAMATION);
					return false;
				}
			pGrooveMap[i] = Index;
			GetModule()->SetGroove(Index, std::move(pGroove));
		}
	}

	return true;
}

bool CFamiTrackerDoc::ImportDetune(CFamiTrackerModule &Imported)		// // //
{
	for (int i = 0; i < 6; i++) for (int j = 0; j < NOTE_COUNT; j++)
		GetModule()->SetDetuneOffset(i, j, Imported.GetDetuneOffset(i, j));
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Document access functions
// ---------------------------------------------------------------------------------------------------------

//
// Instruments
//

void CFamiTrackerDoc::SaveInstrument(unsigned int Index, CSimpleFile &file) const {
	GetInstrumentManager()->GetInstrument(Index)->SaveFTI(file);
}

bool CFamiTrackerDoc::LoadInstrument(unsigned Index, CSimpleFile &File) {		// // //
	// Loads an instrument from file, return allocated slot or INVALID_INSTRUMENT if failed

	// FTI instruments files
	static const char INST_HEADER[] = "FTI";
//	static const char INST_VERSION[] = "2.4";

	static const unsigned I_CURRENT_VER_MAJ = 2;		// // // 050B
	static const unsigned I_CURRENT_VER_MIN = 5;		// // // 050B

	if (Index >= MAX_INSTRUMENTS)		// // //
		return false;

	// Signature
	for (std::size_t i = 0; i < std::size(INST_HEADER) - 1; ++i)
		if (File.ReadChar() != INST_HEADER[i]) {
			AfxMessageBox(IDS_INSTRUMENT_FILE_FAIL, MB_ICONERROR);
			return false;
		}

	// Version
	unsigned iInstMaj = conv::from_digit(File.ReadChar());
	if (File.ReadChar() != '.') {
		AfxMessageBox(IDS_INST_VERSION_UNSUPPORTED, MB_ICONERROR);
		return false;
	}
	unsigned iInstMin = conv::from_digit(File.ReadChar());
	if (std::tie(iInstMaj, iInstMin) > std::tie(I_CURRENT_VER_MAJ, I_CURRENT_VER_MIN)) {
		AfxMessageBox(IDS_INST_VERSION_UNSUPPORTED, MB_ICONERROR);
		return false;
	}

	try {
		CSingleLock lock {&m_csDocumentLock, TRUE};

		inst_type_t InstType = static_cast<inst_type_t>(File.ReadChar());
		if (auto pInstrument = GetInstrumentManager()->CreateNew(InstType != INST_NONE ? InstType : INST_2A03)) {
			pInstrument->OnBlankInstrument();
			pInstrument->LoadFTI(File, iInstMaj * 10 + iInstMin);		// // //
			return GetInstrumentManager()->InsertInstrument(Index, std::move(pInstrument));
		}

		AfxMessageBox("Failed to create instrument", MB_ICONERROR);
		return false;
	}
	catch (CModuleException e) {
		AfxMessageBox(e.GetErrorString().c_str(), MB_ICONERROR);
		return false;
	}
}

//// Track functions //////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerDoc::SelectExpansionChip(const CSoundChipSet &chips, unsigned n163chs) {		// // //
	ASSERT(n163chs <= MAX_CHANNELS_N163 && (chips.ContainsChip(sound_chip_t::N163) == (n163chs != 0)));

	// This will select a chip in the sound emulator
	LockDocument();
	GetModule()->SetChannelMap(theApp.GetSoundGenerator()->MakeChannelMap(chips, n163chs));		// // //
	UnlockDocument();

	ApplyExpansionChip();
}

void CFamiTrackerDoc::ApplyExpansionChip() const {
	// Tell the sound emulator to switch expansion chip
	theApp.GetSoundGenerator()->SelectChip(GetModule()->GetSoundChipSet());

	// Change period tables
	theApp.GetSoundGenerator()->LoadMachineSettings();		// // //
}

//
// from the compoment interface
//

CSequenceManager *const CFamiTrackerDoc::GetSequenceManager(int InstType) const
{
	return GetInstrumentManager()->GetSequenceManager(InstType);
}

CDSampleManager *const CFamiTrackerDoc::GetDSampleManager() const
{
	return GetInstrumentManager()->GetDSampleManager();
}

void CFamiTrackerDoc::Modify(bool Change)
{
	SetModifiedFlag(Change ? TRUE : FALSE);
}

void CFamiTrackerDoc::ModifyIrreversible()
{
	SetModifiedFlag(TRUE);
	SetExceededFlag(TRUE);
}

// Channel interface, these functions must be synchronized!!!

// Attributes

CFamiTrackerModule *CFamiTrackerDoc::GetModule() noexcept {		// // //
	return module_.get();
}

const CFamiTrackerModule *CFamiTrackerDoc::GetModule() const noexcept {
	return module_.get();
}

CString CFamiTrackerDoc::GetFileTitle() const
{
	// Return file name without extension
	CString FileName = GetTitle();

	static const LPCTSTR EXT[] = {_T(".ftm"), _T(".0cc"), _T(".ftm.bak"), _T(".0cc.bak")};		// // //
	// Remove extension

	for (const auto &str : EXT) {
		int Len = lstrlen(str);
		if (FileName.Right(Len).CompareNoCase(str) == 0)
			return FileName.Left(FileName.GetLength() - Len);
	}

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
	TCHAR TempPath[MAX_PATH], TempFile[MAX_PATH];

	GetTempPath(MAX_PATH, TempPath);
	GetTempFileName(TempPath, _T("Aut"), 21587, TempFile);

	// Check if file exists
	CFile file;
	if (file.Open(TempFile, CFile::modeRead)) {
		file.Close();
		if (AfxMessageBox(_T("It might be possible to recover last document, do you want to try?"), MB_YESNO) == IDYES) {
			OpenDocument(TempFile);
			SelectExpansionChip(GetExpansionChip(), GetNamcoChannels());
		}
		else {
			DeleteFile(TempFile);
		}
	}

	TRACE("Doc: Allocated file for auto save: ");
	TRACE(TempFile);
	TRACE("\n");

	m_sAutoSaveFile = TempFile;
}

void CFamiTrackerDoc::ClearAutoSave()
{
	if (m_sAutoSaveFile.GetLength() == 0)
		return;

	DeleteFile(m_sAutoSaveFile);

	m_sAutoSaveFile = _T("");
	m_iAutoSaveCounter = 0;

	TRACE("Doc: Removed auto save file\n");
}

void CFamiTrackerDoc::AutoSave()
{
	// Autosave
	if (!m_iAutoSaveCounter || !m_bFileLoaded || m_sAutoSaveFile.GetLength() == 0)
		return;

	m_iAutoSaveCounter--;

	if (m_iAutoSaveCounter == 0) {
		TRACE("Doc: Performing auto save\n");
		SaveDocument(m_sAutoSaveFile);
	}
}

#endif

// Operations

bool CFamiTrackerDoc::GetExceededFlag() const {		// // //
	return m_bExceeded;
}

void CFamiTrackerDoc::SetExceededFlag(bool Exceed) {		// // //
	m_bExceeded = Exceed;
}



// // // delegates

#pragma region delegates

CChannelOrder &CFamiTrackerDoc::GetChannelOrder() const {		// // //
	return GetModule()->GetChannelOrder();
}

int CFamiTrackerDoc::GetNamcoChannels() const {
	return GetModule()->GetNamcoChannels();
}

CInstrumentManager *const CFamiTrackerDoc::GetInstrumentManager() const {
	return GetModule()->GetInstrumentManager();
}

#pragma endregion
