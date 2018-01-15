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

 Document file version changes

 Ver 4.0
  - Header block, added song names

 Ver 3.0
  - Sequences are stored in the way they are represented in the instrument editor
  - Added separate speed and tempo settings
  - Changed automatic portamento to 3xx and added 1xx & 2xx portamento

 Ver 2.1
  - Made some additions to support multiple effect columns and prepared for more channels
  - Made some speed adjustments, increase speed effects by one if it's below 20

 Ver 2.0
  - Files are small

*/

#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"		// // //
#include <algorithm>
#include <bitset>		// // //
#include "FamiTracker.h"
#include "Instrument.h"		// // //
#include "SeqInstrument.h"		// // //
#include "Instrument2A03.h"		// // //
#include "ModuleException.h"		// // //
#include "TrackerChannel.h"
#include "DocumentFile.h"
#include "SoundGen.h"
#include "SequenceCollection.h"		// // //
#include "SequenceManager.h"		// // //
#include "DSampleManager.h"			// // //
#include "InstrumentManager.h"		// // //
#include "Bookmark.h"		// // //
#include "BookmarkCollection.h"		// // //
#include "APU/APU.h"
#include "SimpleFile.h"		// // //
//#include "SongView.h"		// // //
#include "ChannelMap.h"		// // //
#include "FamiTrackerDocIO.h"		// // //
#include "SongData.h"		// // //
#include "FamiTrackerDocOldIO.h"		// // //
#include "NumConv.h"		// // //
#include "PatternEditorTypes.h"		// // // TODO: remove

#include "ft0cc/doc/groove.hpp"		// // //
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
	m_pChannelMap(std::make_unique<CChannelMap>()),		// // //
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
	theApp.GetSoundGenerator()->SetRecordChannel(-1);		// // //

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

	// Reset variables to default
	m_pChannelMap = std::make_unique<CChannelMap>();		// // //
	m_iChannelsAvailable = CHANNELS_DEFAULT;

	// Auto save
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
	SelectExpansionChip(SNDCHIP_NONE, 0);		// // //
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

bool CFamiTrackerDoc::ImportInstruments(CFamiTrackerDoc &Imported, int *pInstTable)
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

	// Check instrument count
	if (GetInstrumentCount() + Imported.GetInstrumentCount() > MAX_INSTRUMENTS) {
		// Out of instrument slots
		AfxMessageBox(IDS_IMPORT_INSTRUMENT_COUNT, MB_ICONERROR);
		return false;
	}

	static const inst_type_t inst[] = {INST_2A03, INST_VRC6, INST_N163, INST_S5B};		// // //
	int (*seqTable[])[SEQ_COUNT] = {SequenceTable2A03, SequenceTableVRC6, SequenceTableN163, SequenceTableS5B};

	// Copy sequences
	for (size_t i = 0; i < std::size(inst); i++) for (int t = 0; t < SEQ_COUNT; ++t) {
		if (GetSequenceCount(inst[i], (sequence_t)t) + Imported.GetSequenceCount(inst[i], (sequence_t)t) > MAX_SEQUENCES) {		// // //
			AfxMessageBox(IDS_IMPORT_SEQUENCE_COUNT, MB_ICONERROR);
			return false;
		}
	}
	for (size_t i = 0; i < std::size(inst); i++) foreachSeq([&] (sequence_t t) {
		for (unsigned int s = 0; s < MAX_SEQUENCES; ++s) if (Imported.GetSequenceItemCount(inst[i], s, t) > 0) {
			auto pImportSeq = Imported.GetSequence(inst[i], s, t);
			int index = -1;
			for (unsigned j = 0; j < MAX_SEQUENCES; ++j) {
				if (GetSequenceItemCount(inst[i], j, t))
					continue;
				// TODO: continue if blank sequence is used by some instrument
				auto pSeq = GetSequence(inst[i], j, t);
				*pSeq = *pImportSeq;		// // //
				// Save a reference to this sequence
				seqTable[i][s][value_cast(t)] = j;
				break;
			}
		}
	});

	bool bOutOfSampleSpace = false;
	auto &Manager = *Imported.GetDSampleManager();		// // //

	// Copy DPCM samples
	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (auto pImportDSample = Manager.ReleaseDSample(i)) {		// // //
			int Index = GetFreeSampleSlot();
			if (Index != -1) {
				SetSample(Index, pImportDSample);
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
		if (Imported.IsInstrumentUsed(i)) {
			auto pInst = Imported.GetInstrument(i)->Clone();		// // //

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

			int Index = GetFreeInstrumentIndex();
			AddInstrument(std::move(pInst), Index);		// // //
			// Save a reference to this instrument
			pInstTable[i] = Index;
		}
	}

	return true;
}

bool CFamiTrackerDoc::ImportGrooves(CFamiTrackerDoc &Imported, int *pGrooveMap)		// // //
{
	int Index = 0;
	for (int i = 0; i < MAX_GROOVE; ++i) {
		if (const auto pGroove = Imported.GetGroove(i)) {
			while (HasGroove(Index))
				if (++Index >= MAX_GROOVE) {
					AfxMessageBox(IDS_IMPORT_GROOVE_SLOTS, MB_ICONEXCLAMATION);
					return false;
				}
			pGrooveMap[i] = Index;
			SetGroove(Index, std::move(pGroove));
		}
	}

	return true;
}

bool CFamiTrackerDoc::ImportDetune(CFamiTrackerDoc &Imported)		// // //
{
	for (int i = 0; i < 6; i++) for (int j = 0; j < NOTE_COUNT; j++)
		SetDetuneOffset(i, j, Imported.GetDetuneOffset(i, j));

	theApp.GetSoundGenerator()->DocumentPropertiesChanged(this);		// // //
	return true;
}

// End of file load/save

// DMC Stuff

std::shared_ptr<ft0cc::doc::dpcm_sample> CFamiTrackerDoc::GetSample(unsigned int Index)
{
	return GetDSampleManager()->GetDSample(Index);		// // //
}

std::shared_ptr<const ft0cc::doc::dpcm_sample> CFamiTrackerDoc::GetSample(unsigned int Index) const
{
	return GetDSampleManager()->GetDSample(Index);		// // //
}

void CFamiTrackerDoc::SetSample(unsigned int Index, std::shared_ptr<dpcm_sample> pSamp)		// // //
{
	if (GetDSampleManager()->SetDSample(Index, std::move(pSamp)))
		ModifyIrreversible();		// // //
}

bool CFamiTrackerDoc::IsSampleUsed(unsigned int Index) const
{
	return GetDSampleManager()->IsSampleUsed(Index);		// // //
}

unsigned int CFamiTrackerDoc::GetSampleCount() const
{
	return GetDSampleManager()->GetSampleCount();
}

int CFamiTrackerDoc::GetFreeSampleSlot() const
{
	return GetDSampleManager()->GetFirstFree();
}

void CFamiTrackerDoc::RemoveSample(unsigned int Index)
{
	(void)GetDSampleManager()->ReleaseDSample(Index);		// // //
}

unsigned int CFamiTrackerDoc::GetTotalSampleSize() const
{
	return GetDSampleManager()->GetTotalSize();
}

// ---------------------------------------------------------------------------------------------------------
// Document access functions
// ---------------------------------------------------------------------------------------------------------

//
// Sequences
//

std::shared_ptr<CSequence> CFamiTrackerDoc::GetSequence(inst_type_t InstType, unsigned Index, sequence_t Type) const		// // //
{
	return GetInstrumentManager()->GetSequence(InstType, Type, Index);
}

unsigned int CFamiTrackerDoc::GetSequenceItemCount(inst_type_t InstType, unsigned Index, sequence_t Type) const		// // //
{
	if (const auto pSeq = GetSequence(InstType, Index, Type))
		return pSeq->GetItemCount();
	return 0;
}

int CFamiTrackerDoc::GetFreeSequence(inst_type_t InstType, sequence_t Type) const		// // //
{
	return GetInstrumentManager()->GetFreeSequenceIndex(InstType, Type, nullptr);
}

int CFamiTrackerDoc::GetSequenceCount(inst_type_t InstType, sequence_t Type) const		// // //
{
	// Return number of allocated sequences of Type

	int Count = 0;
	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		if (GetSequenceItemCount(InstType, i, Type) > 0) // TODO: fix this and the instrument interface
			++Count;
	}
	return Count;
}

int CFamiTrackerDoc::GetTotalSequenceCount(inst_type_t InstType) const {		// // //
	int Count = 0;
	foreachSeq([&] (sequence_t i) {
		Count += GetSequenceCount(InstType, i);
	});
	return Count;
}

//
// Instruments
//

std::shared_ptr<CInstrument> CFamiTrackerDoc::GetInstrument(unsigned int Index) const
{
	return GetInstrumentManager()->GetInstrument(Index);
}

unsigned int CFamiTrackerDoc::GetInstrumentCount() const
{
	return GetInstrumentManager()->GetInstrumentCount();
}

unsigned CFamiTrackerDoc::GetFreeInstrumentIndex() const {		// // //
	return GetInstrumentManager()->GetFirstUnused();
}

bool CFamiTrackerDoc::IsInstrumentUsed(unsigned int Index) const
{
	return GetInstrumentManager()->IsInstrumentUsed(Index);
}

bool CFamiTrackerDoc::AddInstrument(std::unique_ptr<CInstrument> pInstrument, unsigned int Slot)		// // //
{
	return GetInstrumentManager()->InsertInstrument(Slot, std::move(pInstrument));
}

bool CFamiTrackerDoc::RemoveInstrument(unsigned int Index)		// // //
{
	return GetInstrumentManager()->RemoveInstrument(Index);
}

inst_type_t CFamiTrackerDoc::GetInstrumentType(unsigned int Index) const
{
	return GetInstrumentManager()->GetInstrumentType(Index);
}

void CFamiTrackerDoc::SaveInstrument(unsigned int Index, CSimpleFile &file) const
{
	GetInstrument(Index)->SaveFTI(file);
}

bool CFamiTrackerDoc::LoadInstrument(unsigned Index, CSimpleFile &File) {		// / //
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
		LockDocument();

		inst_type_t InstType = static_cast<inst_type_t>(File.ReadChar());
		auto pInstrument = GetInstrumentManager()->CreateNew(InstType != INST_NONE ? InstType : INST_2A03);
		if (!pInstrument) {
			UnlockDocument();
			AfxMessageBox("Failed to create instrument", MB_ICONERROR);
			return false;
		}

		pInstrument->OnBlankInstrument();
		pInstrument->LoadFTI(File, iInstMaj * 10 + iInstMin);		// // //
		bool res = AddInstrument(std::move(pInstrument), Index);
		UnlockDocument();
		return res;
	}
	catch (CModuleException e) {
		UnlockDocument();
		AfxMessageBox(e.GetErrorString().c_str(), MB_ICONERROR);
		return false;
	}
}

//
// // // General document
//

void CFamiTrackerDoc::SetFrameCount(unsigned int Track, unsigned int Count)
{
	ASSERT(Count > 0 && Count <= MAX_FRAMES);		// // //

	GetSongData(Track).SetFrameCount(Count);
}

void CFamiTrackerDoc::SetPatternLength(unsigned int Track, unsigned int Length)
{
	ASSERT(Length <= MAX_PATTERN_LENGTH);

	GetSongData(Track).SetPatternLength(Length);
}

void CFamiTrackerDoc::SetSongSpeed(unsigned int Track, unsigned int Speed)
{
	auto &Song = GetSongData(Track);
	if (Song.GetSongGroove())		// // //
		ASSERT(Speed < MAX_GROOVE);
	else
		ASSERT(Speed <= MAX_TEMPO);

	Song.SetSongSpeed(Speed);
}

void CFamiTrackerDoc::SetSongTempo(unsigned int Track, unsigned int Tempo)
{
	GetSongData(Track).SetSongTempo(Tempo);
}

void CFamiTrackerDoc::SetSongGroove(unsigned int Track, bool Groove)		// // //
{
	GetSongData(Track).SetSongGroove(Groove);
}

unsigned int CFamiTrackerDoc::GetPatternLength(unsigned int Track) const
{
	return GetSongData(Track).GetPatternLength();
}

unsigned int CFamiTrackerDoc::GetCurrentPatternLength(unsigned int Track, int Frame) const		// // //
{
	if (theApp.GetSettings()->General.bShowSkippedRows)		// // //
		return GetPatternLength(Track);

	int Frames = GetFrameCount(Track);
	Frame %= Frames;
	if (Frame < 0) Frame += Frames;
	return GetFrameLength(Track, Frame);
}

unsigned int CFamiTrackerDoc::GetFrameCount(unsigned int Track) const
{
	return GetSongData(Track).GetFrameCount();
}

unsigned int CFamiTrackerDoc::GetSongSpeed(unsigned int Track) const
{
	return GetSongData(Track).GetSongSpeed();
}

unsigned int CFamiTrackerDoc::GetSongTempo(unsigned int Track) const
{
	return GetSongData(Track).GetSongTempo();
}

bool CFamiTrackerDoc::GetSongGroove(unsigned int Track) const		// // //
{
	return GetSongData(Track).GetSongGroove();
}

unsigned int CFamiTrackerDoc::GetEffColumns(unsigned int Track, chan_id_t Channel) const
{
	return GetSongData(Track).GetEffectColumnCount(Channel);
}

void CFamiTrackerDoc::SetEffColumns(unsigned int Track, chan_id_t Channel, unsigned int Columns)
{
	ASSERT(Columns < MAX_EFFECT_COLUMNS);

	GetSongData(Track).SetEffectColumnCount(Channel, Columns);
}

unsigned int CFamiTrackerDoc::GetPatternAtFrame(unsigned int Track, unsigned int Frame, chan_id_t Channel) const
{
	return GetSongData(Track).GetFramePattern(Frame, Channel);
}

void CFamiTrackerDoc::SetPatternAtFrame(unsigned int Track, unsigned int Frame, chan_id_t Channel, unsigned int Pattern)
{
	ASSERT(Pattern < MAX_PATTERN);

	GetSongData(Track).SetFramePattern(Frame, Channel, Pattern);
}

unsigned int CFamiTrackerDoc::GetFrameRate() const
{
	unsigned Rate = GetEngineSpeed();		// // //
	if (Rate == 0)
		return (GetMachine() == NTSC) ? CAPU::FRAME_RATE_NTSC : CAPU::FRAME_RATE_PAL;
	return Rate;
}

//// Pattern functions ////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerDoc::SetNoteData(unsigned Track, unsigned Frame, chan_id_t Channel, unsigned Row, const stChanNote &Data)		// // //
{
	GetSongData(Track).GetPatternOnFrame(Channel, Frame).SetNoteOn(Row, Data);		// // //
}

const stChanNote &CFamiTrackerDoc::GetNoteData(unsigned Track, unsigned Frame, chan_id_t Channel, unsigned Row) const
{
	return GetSongData(Track).GetPatternOnFrame(Channel, Frame).GetNoteOn(Row);		// // //
}

stChanNote CFamiTrackerDoc::GetActiveNote(unsigned Track, unsigned Frame, chan_id_t Channel, unsigned Row) const {		// // //
	auto Note = GetNoteData(Track, Frame, Channel, Row);
	for (int i = GetEffColumns(Track, Channel) + 1; i < MAX_EFFECT_COLUMNS; ++i)
		Note.EffNumber[i] = EF_NONE;
	return Note;
}

void CFamiTrackerDoc::SetDataAtPattern(unsigned Track, unsigned Pattern, chan_id_t Channel, unsigned Row, const stChanNote &Data)		// // //
{
	// Set a note to a direct pattern
	GetSongData(Track).SetPatternData(Channel, Pattern, Row, Data);		// // //
}

const stChanNote &CFamiTrackerDoc::GetDataAtPattern(unsigned Track, unsigned Pattern, chan_id_t Channel, unsigned Row) const		// // //
{
	// Get note from a direct pattern
	return GetSongData(Track).GetPatternData(Channel, Pattern, Row);		// // //
}

bool CFamiTrackerDoc::InsertRow(unsigned int Track, unsigned int Frame, chan_id_t Channel, unsigned int Row)
{
	auto &Song = GetSongData(Track);
	auto &Pattern = Song.GetPatternOnFrame(Channel, Frame);		// // //

	for (unsigned int i = Song.GetPatternLength() - 1; i > Row; --i)
		Pattern.SetNoteOn(i, Pattern.GetNoteOn(i - 1));
	Pattern.SetNoteOn(Row, { });

	return true;
}

void CFamiTrackerDoc::ClearPattern(unsigned int Track, unsigned int Frame, chan_id_t Channel)
{
	// Clear entire pattern
	auto &Song = GetSongData(Track);
	Song.GetPatternOnFrame(Channel, Frame) = CPatternData { };		// // //
}

bool CFamiTrackerDoc::ClearRowField(unsigned int Track, unsigned int Frame, chan_id_t Channel, unsigned int Row, cursor_column_t Column)
{
	auto &Song = GetSongData(Track);
	stChanNote &Note = Song.GetPatternOnFrame(Channel, Frame).GetNoteOn(Row);		// // //

	switch (Column) {
	case C_NOTE:			// Note
		Note.Note = NONE;
		Note.Octave = 0;
		Note.Instrument = MAX_INSTRUMENTS;	// Fix the old behaviour
		Note.Vol = MAX_VOLUME;
		break;
	case C_INSTRUMENT1:		// Instrument
	case C_INSTRUMENT2:
		Note.Instrument = MAX_INSTRUMENTS;
		break;
	case C_VOLUME:			// Volume
		Note.Vol = MAX_VOLUME;
		break;
	case C_EFF1_NUM:			// Effect 1
	case C_EFF1_PARAM1:
	case C_EFF1_PARAM2:
		Note.EffNumber[0] = EF_NONE;
		Note.EffParam[0] = 0;
		break;
	case C_EFF2_NUM:		// Effect 2
	case C_EFF2_PARAM1:
	case C_EFF2_PARAM2:
		Note.EffNumber[1] = EF_NONE;
		Note.EffParam[1] = 0;
		break;
	case C_EFF3_NUM:		// Effect 3
	case C_EFF3_PARAM1:
	case C_EFF3_PARAM2:
		Note.EffNumber[2] = EF_NONE;
		Note.EffParam[2] = 0;
		break;
	case C_EFF4_NUM:		// Effect 4
	case C_EFF4_PARAM1:
	case C_EFF4_PARAM2:
		Note.EffNumber[3] = EF_NONE;
		Note.EffParam[3] = 0;
		break;
	}

	return true;
}

bool CFamiTrackerDoc::PullUp(unsigned int Track, unsigned int Frame, chan_id_t Channel, unsigned int Row)
{
	auto &Song = GetSongData(Track);		// // //
	auto &Pattern = Song.GetPatternOnFrame(Channel, Frame);
	int PatternLen = Song.GetPatternLength();

	for (int i = Row; i < PatternLen - 1; ++i)
		Pattern.SetNoteOn(i, Pattern.GetNoteOn(i + 1));		// // //
	Pattern.SetNoteOn(PatternLen - 1, { });

	return true;
}

void CFamiTrackerDoc::CopyPattern(unsigned int Track, int Target, int Source, chan_id_t Channel)
{
	// Copy one pattern to another

	auto &Song = GetSongData(Track);
	Song.GetPattern(Channel, Target) = Song.GetPattern(Channel, Source);		// // //
}

void CFamiTrackerDoc::SwapChannels(unsigned int Track, chan_id_t First, chan_id_t Second)		// // //
{
	GetSongData(Track).SwapChannels(First, Second);
}

//// Frame functions //////////////////////////////////////////////////////////////////////////////////

bool CFamiTrackerDoc::InsertFrame(unsigned int Track, unsigned int Frame)
{
	ASSERT(Frame < MAX_FRAMES);

	if (!AddFrames(Track, Frame, 1))
		return false;

	// Select free patterns
	auto &Song = GetSongData(Track);		// // //
	ForeachChannel([&] (chan_id_t i) {
		unsigned Pattern = Song.GetFreePatternIndex(i);		// // //
		Song.SetFramePattern(Frame, i, Pattern == -1 ? 0 : Pattern);
	});

	return true;
}

bool CFamiTrackerDoc::RemoveFrame(unsigned int Track, unsigned int Frame)
{
	return DeleteFrames(Track, Frame, 1);		// // //
}

bool CFamiTrackerDoc::DuplicateFrame(unsigned int Track, unsigned int Frame)
{
	// Create a copy of selected frame
	if (!AddFrames(Track, Frame + 1, 1))		// // //
		return false;

	auto &Song = GetSongData(Track);		// // //
	ForeachChannel([&] (chan_id_t i) {
		Song.SetFramePattern(Frame + 1, i, Song.GetFramePattern(Frame, i));
	});

	return true;
}

bool CFamiTrackerDoc::CloneFrame(unsigned int Track, unsigned int Frame)		// // // renamed
{
	// Create a copy of selected frame including patterns

	// insert new frame with next free pattern numbers
	if (!InsertFrame(Track, Frame))
		return false;

	// copy old patterns into new
	auto &Song = GetSongData(Track);		// / ///
	ForeachChannel([&] (chan_id_t i) {
		Song.GetPatternOnFrame(i, Frame) = Song.GetPatternOnFrame(i, Frame - 1);
	});

	return true;
}

static void SwapFrames(const CFamiTrackerDoc &doc, CSongData &Song, unsigned f1, unsigned f2) {		// // //
	doc.ForeachChannel([&] (chan_id_t ch) {
		int Pattern = Song.GetFramePattern(f1, ch);
		Song.SetFramePattern(f1, ch, Song.GetFramePattern(f2, ch));
		Song.SetFramePattern(f2, ch, Pattern);
	});

	Song.GetBookmarks().SwapFrames(f1, f2);		// // //
}

bool CFamiTrackerDoc::MoveFrameDown(unsigned int Track, unsigned int Frame)
{
	auto &Song = GetSongData(Track);		// / ///
	if (Frame == Song.GetFrameCount() - 1)
		return false;

	SwapFrames(*this, Song, Frame, Frame + 1);
	return true;
}

bool CFamiTrackerDoc::MoveFrameUp(unsigned int Track, unsigned int Frame)
{
	if (Frame == 0)
		return false;

	SwapFrames(*this, GetSongData(Track), Frame, Frame - 1);
	return true;
}

bool CFamiTrackerDoc::AddFrames(unsigned int Track, unsigned int Frame, unsigned Count) {		// // //
	ASSERT(Frame < MAX_FRAMES);

	auto &Song = GetSongData(Track);		// // //
	const unsigned FrameCount = Song.GetFrameCount();
	const unsigned Channels = GetAvailableChannels();

	if (FrameCount + Count > MAX_FRAMES)
		return false;

	Song.SetFrameCount(FrameCount + Count);

	ForeachChannel([&] (chan_id_t j) {
		for (unsigned int i = FrameCount + Count - 1; i >= Frame + Count; --i)
			Song.SetFramePattern(i, j, Song.GetFramePattern(i - Count, j));
		for (unsigned i = 0; i < Count; ++i)		// // //
			Song.SetFramePattern(Frame + i, j, 0);
	});

	Song.GetBookmarks().InsertFrames(Frame, Count);		// // //

	return true;
}

bool CFamiTrackerDoc::DeleteFrames(unsigned int Track, unsigned int Frame, unsigned Count) {		// // //
	ASSERT(Frame < MAX_FRAMES);

	auto &Song = GetSongData(Track);		// // //
	const unsigned FrameCount = Song.GetFrameCount();
	const unsigned Channels = GetAvailableChannels();

	if (Frame >= FrameCount)
		return true;
	if (Count > FrameCount - Frame)
		Count = FrameCount - Frame;
	if (Count >= FrameCount)
		return false;

	ForeachChannel([&] (chan_id_t j) {
		for (unsigned i = Frame; i < FrameCount - Count; ++i)
			Song.SetFramePattern(i, j, Song.GetFramePattern(i + 1, j));
		for (unsigned i = FrameCount - Count; i < FrameCount; ++i)
			Song.SetFramePattern(i, j, 0);		// // //
	});

	Song.GetBookmarks().RemoveFrames(Frame, Count);		// // //
	Song.SetFrameCount(FrameCount - Count);

	return true;
}

//// Track functions //////////////////////////////////////////////////////////////////////////////////

const std::string &CFamiTrackerDoc::GetTrackTitle(unsigned int Track) const		// // //
{
	if (Track < GetTrackCount())
		return GetSongData(Track).GetTitle();
	return CSongData::DEFAULT_TITLE;
}

void CFamiTrackerDoc::SetTrackTitle(unsigned int Track, const std::string &title)		// // //
{
	GetSongData(Track).SetTitle(title);
}

void CFamiTrackerDoc::MoveTrackUp(unsigned int Track)
{
	GetModule()->SwapSongs(Track, Track - 1);
}

void CFamiTrackerDoc::MoveTrackDown(unsigned int Track)
{
	GetModule()->SwapSongs(Track, Track + 1);
}

CSongData &CFamiTrackerDoc::GetSongData(unsigned int Index)		// // //
{
	// Ensure track is allocated
	return *GetSong(Index);
}

const CSongData &CFamiTrackerDoc::GetSongData(unsigned int Index) const		// // //
{
	return *GetSong(Index);
}

void CFamiTrackerDoc::SelectExpansionChip(unsigned chips, unsigned n163chs) {		// // //
	ASSERT(n163chs <= 8 && !(chips & SNDCHIP_N163) == !n163chs);

	// // // Complete sound chip setup
	if (chips != SNDCHIP_NONE)
		SetMachine(NTSC);

	// This will select a chip in the sound emulator
	auto newMap = theApp.GetSoundGenerator()->MakeChannelMap(chips, n163chs);		// // //
	LockDocument();
	m_pChannelMap = std::move(newMap);		// // //
	m_iChannelsAvailable = GetChannelCount();
	UnlockDocument();

	ApplyExpansionChip();
	ModifyIrreversible();
}

unsigned char CFamiTrackerDoc::GetExpansionChip() const {
	return m_pChannelMap->GetExpansionFlag();		// // //
}

void CFamiTrackerDoc::ApplyExpansionChip() const {
	// Tell the sound emulator to switch expansion chip
	theApp.GetSoundGenerator()->SelectChip(GetExpansionChip());

	// Change period tables
	theApp.GetSoundGenerator()->LoadMachineSettings();		// // //
}

//
// from the compoment interface
//

CChannelMap *const CFamiTrackerDoc::GetChannelMap() const {
	return m_pChannelMap.get();
}

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

bool CFamiTrackerDoc::ExpansionEnabled(int Chip) const {
	return m_pChannelMap->HasExpansionChip(Chip);		// // //
}

int CFamiTrackerDoc::GetNamcoChannels() const {
	return m_pChannelMap->GetChipChannelCount(SNDCHIP_N163);		// // //
}

unsigned int CFamiTrackerDoc::GetFirstFreePattern(unsigned int Track, chan_id_t Channel) const
{
	auto &Song = GetSongData(Track);
	return Song.GetFreePatternIndex(Channel);		// // //
}

bool CFamiTrackerDoc::IsPatternEmpty(unsigned int Track, chan_id_t Channel, unsigned int Pattern) const
{
	auto &Song = GetSongData(Track);
	return Song.GetPattern(Channel, Pattern).IsEmpty();
}

// Channel interface, these functions must be synchronized!!!

int CFamiTrackerDoc::GetChannelType(int Channel) const
{
	return m_pChannelMap->GetChannelType(Channel);		// // //
}

int CFamiTrackerDoc::GetChipType(int Channel) const
{
	return m_pChannelMap->GetChipType(Channel);		// // //
}

int CFamiTrackerDoc::GetChannelCount() const
{
	return m_pChannelMap->GetChannelCount();		// // //
}

chan_id_t CFamiTrackerDoc::TranslateChannel(unsigned Index) const {		// // //
	return GetChannelMap()->GetChannelType(Index);
}

CTrackerChannel &CFamiTrackerDoc::GetChannel(int Index) const		// // //
{
	return m_pChannelMap->GetChannel(Index);		// // //
}

int CFamiTrackerDoc::GetChannelIndex(int Channel) const
{
	return m_pChannelMap->GetChannelIndex(Channel);		// // //
}

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
			SelectExpansionChip(m_iExpansionChip);
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

//
// Comment functions
//

void CFamiTrackerDoc::SetHighlight(unsigned int Track, const stHighlight &Hl)		// // //
{
	GetSongData(Track).SetHighlight(Hl);
}

const stHighlight &CFamiTrackerDoc::GetHighlight(unsigned int Track) const		// // //
{
	return GetSongData(Track).GetRowHighlight();
}

void CFamiTrackerDoc::SetHighlight(const stHighlight &Hl)		// // //
{
	VisitSongs([&Hl] (CSongData &song) {
		song.SetHighlight(Hl);
	});
}

const stHighlight &CFamiTrackerDoc::GetHighlight() const		// // //
{
	return GetSongData(0).GetRowHighlight();
}

stHighlight CFamiTrackerDoc::GetHighlightAt(unsigned int Track, unsigned int Frame, unsigned int Row) const		// // //
{
	while (Frame < 0) Frame += GetFrameCount(Track);
	Frame %= GetFrameCount(Track);

	stHighlight Hl = GetHighlight(Track);

	const CBookmark Zero { };
	const CBookmarkCollection *pCol = GetBookmarkCollection(Track);
	if (const unsigned Count = pCol->GetCount()) {
		CBookmark tmp(Frame, Row);
		unsigned int Min = tmp.Distance(Zero);
		for (unsigned i = 0; i < Count; ++i) {
			CBookmark *pMark = pCol->GetBookmark(i);
			unsigned Dist = tmp.Distance(*pMark);
			if (Dist <= Min) {
				Min = Dist;
				if (pMark->m_Highlight.First != -1 && (pMark->m_bPersist || pMark->m_iFrame == Frame))
					Hl.First = pMark->m_Highlight.First;
				if (pMark->m_Highlight.Second != -1 && (pMark->m_bPersist || pMark->m_iFrame == Frame))
					Hl.Second = pMark->m_Highlight.Second;
				Hl.Offset = pMark->m_Highlight.Offset + pMark->m_iRow;
			}
		}
	}

	return Hl;
}

unsigned int CFamiTrackerDoc::GetHighlightState(unsigned int Track, unsigned int Frame, unsigned int Row) const		// // //
{
	stHighlight Hl = GetHighlightAt(Track, Frame, Row);
	if (Hl.Second > 0 && !((Row - Hl.Offset) % Hl.Second))
		return 2;
	if (Hl.First > 0 && !((Row - Hl.Offset) % Hl.First))
		return 1;
	return 0;
}

CBookmarkCollection *CFamiTrackerDoc::GetBookmarkCollection(unsigned track) {		// // //
	return track < GetTrackCount() ? &GetSongData(track).GetBookmarks() : nullptr;
}

const CBookmarkCollection *CFamiTrackerDoc::GetBookmarkCollection(unsigned track) const {		// // //
	return track < GetTrackCount() ? &GetSongData(track).GetBookmarks() : nullptr;
}

unsigned CFamiTrackerDoc::GetTotalBookmarkCount() const {		// // //
	unsigned count = 0;
	VisitSongs([&] (const CSongData &song) {
		count += song.GetBookmarks().GetCount();
	});
	return count;
}

CBookmark *CFamiTrackerDoc::GetBookmarkAt(unsigned int Track, unsigned int Frame, unsigned int Row) const		// // //
{
	if (const CBookmarkCollection *pCol = GetBookmarkCollection(Track)) {
		for (unsigned i = 0, Count = pCol->GetCount(); i < Count; ++i) {
			CBookmark *pMark = pCol->GetBookmark(i);
			if (pMark->m_iFrame == Frame && pMark->m_iRow == Row)
				return pMark;
		}
	}
	return nullptr;
}

class loop_visitor {
public:
	loop_visitor(const CChannelMap &map, const CSongData &song, int channels) :
		map_(map), song_(song), channels_(channels) { }

	template <typename F, typename G>
	void Visit(F cb, G fx) {
		unsigned FrameCount = song_.GetFrameCount();
		unsigned Rows = song_.GetPatternLength();

		std::bitset<MAX_PATTERN_LENGTH> RowVisited[MAX_FRAMES] = { };		// // //
		while (!RowVisited[f_][r_]) {
			RowVisited[f_][r_] = true;

			int Bxx = -1;
			int Dxx = -1;
			bool Cxx = false;
			for (int ch = 0; ch < channels_; ++ch) {
				const auto &Note = song_.GetPatternOnFrame(map_.GetChannelType(ch), f_).GetNoteOn(r_);		// // //
				for (int l = 0, m = song_.GetEffectColumnCount(map_.GetChannelType(ch)); l <= m; ++l) {
					switch (Note.EffNumber[l]) {
					case EF_JUMP:
						Bxx = Note.EffParam[l];
						break;
					case EF_SKIP:
						Dxx = Note.EffParam[l];
						break;
					case EF_HALT:
						if (!first_)
							return;
						first_ = false;
						Cxx = true;
					default:
						if constexpr (std::is_invocable_v<G, int, effect_t, unsigned char>)
							fx(ch, Note.EffNumber[l], Note.EffParam[l]);
					}
				}
			}

			if constexpr (std::is_invocable_v<F, unsigned, unsigned>)
				cb(f_, r_);
			else
				cb();

			if (Cxx)
				return;
			if (Bxx != -1) {
				f_ = std::min(static_cast<unsigned int>(Bxx), FrameCount - 1);
				r_ = 0;
			}
			else if (Dxx != -1)
				r_ = std::min(static_cast<unsigned int>(Dxx), Rows - 1);
			else if (++r_ >= Rows) {		// // //
				r_ = 0;
				if (++f_ >= FrameCount)
					f_ = 0;
			}
		}
	}

private:
	const CChannelMap &map_;
	const CSongData &song_;
	int channels_;
	unsigned f_ = 0;
	unsigned r_ = 0;
	bool first_ = true;
};

unsigned int CFamiTrackerDoc::ScanActualLength(unsigned int Track, unsigned int Count) const		// // //
{
	// Return number for frames played for a certain number of loops
	int FirstLoop = 0;
	int SecondLoop = 0;

	loop_visitor visitor(*GetChannelMap(), GetSongData(Track), GetChannelCount());
	visitor.Visit([&] { ++FirstLoop; }, [] { });
	visitor.Visit([&] { ++SecondLoop; }, [] { });

	return FirstLoop + SecondLoop * (Count - 1);		// // //
}

double CFamiTrackerDoc::GetStandardLength(int Track, unsigned int ExtraLoops) const		// // //
{
	const auto &song = GetSongData(Track);

	double Tempo = song.GetSongTempo();
	if (!Tempo)
		Tempo = 2.5 * GetFrameRate();
	const bool AllowTempo = song.GetSongTempo() != 0;
	const int Split = GetSpeedSplitPoint();
	int Speed = song.GetSongSpeed();
	int GrooveIndex = song.GetSongGroove() ? Speed : -1;
	int GroovePointer = 0;

	if (GrooveIndex != -1 && !HasGroove(Speed)) {
		GrooveIndex = -1;
		Speed = DEFAULT_SPEED;
	}

	const auto fxhandler = [&] (int ch, effect_t fx, uint8_t param) {
		switch (fx) {
		case EF_SPEED:
			if (AllowTempo && param >= Split)
				Tempo = param;
			else {
				GrooveIndex = -1;
				Speed = param;
			}
			break;
		case EF_GROOVE:
			if (HasGroove(param)) {
				GrooveIndex = param;
				GroovePointer = 0;
			}
			break;
		}
	};

	double FirstLoop = 0.0;
	double SecondLoop = 0.0;

	loop_visitor visitor(*GetChannelMap(), song, GetChannelCount());
	visitor.Visit([&] {
		if (auto pGroove = GetGroove(GrooveIndex))
			Speed = pGroove->entry(GroovePointer++);
		FirstLoop += Speed / Tempo;
	}, fxhandler);
	visitor.Visit([&] {
		if (auto pGroove = GetGroove(GrooveIndex))
			Speed = pGroove->entry(GroovePointer++);
		SecondLoop += Speed / Tempo;
	}, fxhandler);

	return (2.5 * (FirstLoop + SecondLoop * ExtraLoops));
}

// Operations

void CFamiTrackerDoc::RemoveUnusedInstruments()
{
	bool used[MAX_INSTRUMENTS] = { };		// // //

	VisitSongs([&] (const CSongData &song) {		// // //
		int length = song.GetPatternLength();
		ForeachChannel([&] (chan_id_t Channel) {
			for (unsigned int Frame = 0; Frame < song.GetFrameCount(); ++Frame)
				song.GetPatternOnFrame(Channel, Frame).VisitRows(length, [&] (const stChanNote &note) {
					if (note.Instrument < MAX_INSTRUMENTS)
						used[note.Instrument] = true;
				});
		});
	});

	for (int i = 0; i < MAX_INSTRUMENTS; ++i)
		if (IsInstrumentUsed(i) && !used[i])
			RemoveInstrument(i);

	static const inst_type_t inst[] = {INST_2A03, INST_VRC6, INST_N163, INST_S5B};

	// Also remove unused sequences
	for (unsigned int i = 0; i < MAX_SEQUENCES; ++i)
		foreachSeq([&] (sequence_t j) {		// // //
			for (size_t c = 0; c < std::size(inst); ++c)
				if (GetSequenceItemCount(inst[c], i, j) > 0) {		// // //
					bool Used = false;
					for (int k = 0; k < MAX_INSTRUMENTS; ++k) {
						if (IsInstrumentUsed(k) && GetInstrumentType(k) == inst[c]) {
							auto pInstrument = std::static_pointer_cast<CSeqInstrument>(GetInstrument(k));
							if (pInstrument->GetSeqIndex(j) == i && pInstrument->GetSeqEnable(j)) {		// // //
								Used = true; break;
							}
						}
					}
					if (!Used)
						GetSequence(inst[c], i, j)->Clear();		// // //
				}
	});
}

void CFamiTrackerDoc::RemoveUnusedPatterns()
{
	VisitSongs([&] (CSongData &song) {
		for (unsigned i = 0; i < CHANNELS; ++i)
			if (GetChannelIndex(i) == (unsigned)-1)
				for (int p = 0; p < MAX_PATTERN; ++p)
					song.GetPattern((chan_id_t)i, p) = CPatternData { };
		song.VisitPatterns([&song] (CPatternData &pattern, chan_id_t c, unsigned p) {
			if (!song.IsPatternInUse(c, p))
				pattern = CPatternData { };
		});
	});
}

void CFamiTrackerDoc::RemoveUnusedSamples()		// // //
{
	bool AssignUsed[MAX_INSTRUMENTS][OCTAVE_RANGE][NOTE_RANGE] = { };

	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (GetDSampleManager()->IsSampleUsed(i)) {
			bool Used = false;
			VisitSongs([&] (const CSongData &song) {
				for (unsigned int Frame = 0; Frame < song.GetFrameCount(); ++Frame) {
					unsigned int Pattern = song.GetFramePattern(Frame, CHANID_DPCM);
					for (unsigned int Row = 0; Row < song.GetPatternLength(); ++Row) {
						const auto &Note = song.GetPatternData(CHANID_DPCM, Pattern, Row);		// // //
						int Index = Note.Instrument;
						if (Note.Note < NOTE_C || Note.Note > NOTE_B || Index == MAX_INSTRUMENTS)
							continue;		// // //
						if (GetInstrumentType(Index) != INST_2A03)
							continue;
						AssignUsed[Index][Note.Octave][Note.Note - 1] = true;
						auto pInst = std::static_pointer_cast<CInstrument2A03>(GetInstrument(Index));
						if (pInst->GetSampleIndex(Note.Octave, Note.Note - 1) == i + 1)
							Used = true;
					}
				}
			});
			if (!Used)
				RemoveSample(i);
		}
	}
	// also remove unused assignments
	for (int i = 0; i < MAX_INSTRUMENTS; i++) if (IsInstrumentUsed(i))
		if (auto pInst = std::dynamic_pointer_cast<CInstrument2A03>(GetInstrument(i)))
			for (int o = 0; o < OCTAVE_RANGE; o++) for (int n = 0; n < NOTE_RANGE; n++)
				if (!AssignUsed[i][o][n])
					pInst->SetSampleIndex(o, n, 0);
}

bool CFamiTrackerDoc::ArePatternsSame(unsigned int Track, chan_id_t Channel, unsigned int Pattern1, unsigned int Pattern2) const		// // //
{
	const auto &song = GetSongData(Track);
	return song.GetPattern(Channel, Pattern1) == song.GetPattern(Channel, Pattern2);
}

void CFamiTrackerDoc::SwapInstruments(int First, int Second)
{
	// Swap instruments
	GetInstrumentManager()->SwapInstruments(First, Second);		// // //

	// Scan patterns
	VisitSongs([&] (CSongData &song) {
		song.VisitPatterns([&] (CPatternData &pat) {
			pat.VisitRows([&] (stChanNote &note, unsigned row) {
				if (note.Instrument == First)
					note.Instrument = Second;
				else if (note.Instrument == Second)
					note.Instrument = First;
			});
		});
	});
}

void CFamiTrackerDoc::SetExceededFlag(bool Exceed)
{
	m_bExceeded = Exceed;
}

int CFamiTrackerDoc::GetFrameLength(unsigned int Track, unsigned int Frame) const
{
	// // // moved from PatternEditor.cpp
	auto &Song = GetSongData(Track);
	const unsigned PatternLength = Song.GetPatternLength();	// default length
	unsigned HaltPoint = PatternLength;

	ForeachChannel([&] (chan_id_t i) {
		unsigned halt = [&] {
			const int Columns = Song.GetEffectColumnCount(i) + 1;
			const auto &pat = Song.GetPatternOnFrame(i, Frame);
			for (unsigned j = 0; j < PatternLength - 1; ++j) {
				const auto &Note = pat.GetNoteOn(j);
				for (int k = 0; k < Columns; ++k)
					switch (Note.EffNumber[k])
					case EF_SKIP: case EF_JUMP: case EF_HALT:
						return j + 1;
			}
			return PatternLength;
		}();
		if (halt < HaltPoint)
			HaltPoint = halt;
	});

	return HaltPoint;
}



// // // delegates to CFamiTrackerModule

#pragma region delegates to CFamiTrackerModule

std::string_view CFamiTrackerDoc::GetModuleName() const {
	return GetModule()->GetModuleName();
}

std::string_view CFamiTrackerDoc::GetModuleArtist() const {
	return GetModule()->GetModuleArtist();
}

std::string_view CFamiTrackerDoc::GetModuleCopyright() const {
	return GetModule()->GetModuleCopyright();
}

void CFamiTrackerDoc::SetModuleName(std::string_view pName) {
	GetModule()->SetModuleName(pName);
}

void CFamiTrackerDoc::SetModuleArtist(std::string_view pArtist) {
	GetModule()->SetModuleArtist(pArtist);
}

void CFamiTrackerDoc::SetModuleCopyright(std::string_view pCopyright) {
	GetModule()->SetModuleCopyright(pCopyright);
}

machine_t CFamiTrackerDoc::GetMachine() const {
	return GetModule()->GetMachine();
}

unsigned int CFamiTrackerDoc::GetEngineSpeed() const {
	return GetModule()->GetEngineSpeed();
}

vibrato_t CFamiTrackerDoc::GetVibratoStyle() const {
	return GetModule()->GetVibratoStyle();
}

bool CFamiTrackerDoc::GetLinearPitch() const {
	return GetModule()->GetLinearPitch();
}

int CFamiTrackerDoc::GetSpeedSplitPoint() const {
	return GetModule()->GetSpeedSplitPoint();
}

void CFamiTrackerDoc::SetMachine(machine_t Machine) {
	GetModule()->SetMachine(Machine);
}

void CFamiTrackerDoc::SetEngineSpeed(unsigned int Speed) {
	GetModule()->SetEngineSpeed(Speed);
}

void CFamiTrackerDoc::SetVibratoStyle(vibrato_t Style) {
	GetModule()->SetVibratoStyle(Style);
}

void CFamiTrackerDoc::SetLinearPitch(bool Enable) {
	GetModule()->SetLinearPitch(Enable);
}

void CFamiTrackerDoc::SetSpeedSplitPoint(int SplitPoint) {
	GetModule()->SetSpeedSplitPoint(SplitPoint);
}

int CFamiTrackerDoc::GetDetuneOffset(int Chip, int Note) const {
	return GetModule()->GetDetuneOffset(Chip, Note);
}

void CFamiTrackerDoc::SetDetuneOffset(int Chip, int Note, int Detune) {
	GetModule()->SetDetuneOffset(Chip, Note, Detune);
}

void CFamiTrackerDoc::ResetDetuneTables() {
	GetModule()->ResetDetuneTables();
}

int CFamiTrackerDoc::GetTuningSemitone() const {
	return GetModule()->GetTuningSemitone();
}

int CFamiTrackerDoc::GetTuningCent() const {
	return GetModule()->GetTuningCent();
}

void CFamiTrackerDoc::SetTuning(int Semitone, int Cent) {
	GetModule()->SetTuning(Semitone, Cent);
}

CSongData *CFamiTrackerDoc::GetSong(unsigned int Index) {		// // //
	return GetModule()->GetSong(Index);
}

const CSongData *CFamiTrackerDoc::GetSong(unsigned int Index) const {		// // //
	return GetModule()->GetSong(Index);
}

unsigned int CFamiTrackerDoc::GetTrackCount() const {
	return GetModule()->GetSongCount();
}

bool CFamiTrackerDoc::InsertSong(unsigned Index, std::unique_ptr<CSongData> pSong) {
	return GetModule()->InsertSong(Index, std::move(pSong));
}

void CFamiTrackerDoc::RemoveTrack(unsigned int Track) {
	GetModule()->RemoveSong(Track);
}

std::unique_ptr<CSongData> CFamiTrackerDoc::ReleaseTrack(unsigned int Track) {		// // //
	return GetModule()->ReleaseSong(Track);
}

std::unique_ptr<CSongData> CFamiTrackerDoc::ReplaceSong(unsigned Index, std::unique_ptr<CSongData> pSong) {		// // //
	return GetModule()->ReplaceSong(Index, std::move(pSong));
}

CInstrumentManager *const CFamiTrackerDoc::GetInstrumentManager() const {
	return GetModule()->GetInstrumentManager();
}

std::shared_ptr<ft0cc::doc::groove> CFamiTrackerDoc::GetGroove(unsigned Index) {
	return GetModule()->GetGroove(Index);
}

std::shared_ptr<const ft0cc::doc::groove> CFamiTrackerDoc::GetGroove(unsigned Index) const {
	return GetModule()->GetGroove(Index);
}

bool CFamiTrackerDoc::HasGroove(unsigned Index) const {
	return GetModule()->HasGroove(Index);
}

void CFamiTrackerDoc::SetGroove(unsigned Index, std::shared_ptr<groove> pGroove) {
	GetModule()->SetGroove(Index, std::move(pGroove));
}

#pragma endregion
