/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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
#include "Groove.h"		// // //
#include "SongData.h"		// // //
#include "FamiTrackerDocOldIO.h"		// // //

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace {

int GetChannelPosition(int Channel, unsigned char chips, unsigned n163chs) {		// // //
	// TODO: use information from the current channel map instead
	unsigned int pos = Channel;
	if (pos == CHANID_MMC5_VOICE) return -1;

	if (!(chips & SNDCHIP_S5B)) {
		if (pos > CHANID_S5B_CH3) pos -= 3;
		else if (pos >= CHANID_S5B_CH1) return -1;
	}
	if (!(chips & SNDCHIP_VRC7)) {
		if (pos > CHANID_VRC7_CH6) pos -= 6;
		else if (pos >= CHANID_VRC7_CH1) return -1;
	}
	if (!(chips & SNDCHIP_FDS)) {
		if (pos > CHANID_FDS) pos -= 1;
		else if (pos >= CHANID_FDS) return -1;
	}
	if (pos > CHANID_N163_CH8) pos -= 8 - (!(chips & SNDCHIP_N163) ? 0 : n163chs);
	else if (pos > CHANID_MMC5_VOICE + (!(chips & SNDCHIP_N163) ? 0 : n163chs)) return -1;
	if (pos > CHANID_MMC5_VOICE) pos -= 1;
	if (!(chips & SNDCHIP_MMC5)) {
		if (pos > CHANID_MMC5_SQUARE2) pos -= 2;
		else if (pos >= CHANID_MMC5_SQUARE1) return -1;
	}
	if (!(chips & SNDCHIP_VRC6)) {
		if (pos > CHANID_VRC6_SAWTOOTH) pos -= 3;
		else if (pos >= CHANID_VRC6_PULSE1) return -1;
	}

	return pos;
}

} // namespace

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
	m_pInstrumentManager(std::make_unique<CInstrumentManager>(this))
{
	// Initialize document object

	ResetDetuneTables();		// // //

	// Register this object to the sound generator
	if (CSoundGen *pSoundGen = theApp.GetSoundGenerator())
		pSoundGen->AssignDocument(this);

	AllocateSong(0);		// // //
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
	CFrameWnd *pFrame = static_cast<CFrameWnd*>(AfxGetApp()->m_pMainWnd);
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

	// Delete all patterns
	m_pTracks.clear();		// // //

	// // // Grooves
	for (auto &x : m_pGrooveTable)		// // //
		x.reset();

	m_pInstrumentManager->ClearAll();		// // //

	// Clear song info
	SetModuleName("");		// // //
	SetModuleArtist("");
	SetModuleCopyright("");

	// Reset variables to default
	m_pChannelMap = std::make_unique<CChannelMap>();		// // //
	m_iChannelsAvailable = CHANNELS_DEFAULT;
	SetMachine(DEFAULT_MACHINE_TYPE);
	SetEngineSpeed(0);
	SetVibratoStyle(DEFAULT_VIBRATO_STYLE);
	SetLinearPitch(DEFAULT_LINEAR_PITCH);
	SetSpeedSplitPoint(DEFAULT_SPEED_SPLIT_POINT);
	SetTuning(0, 0);		// // // 050B
	SetHighlight(CSongData::DEFAULT_HIGHLIGHT);		// // //

	ResetDetuneTables();		// // //

	// Auto save
#ifdef AUTOSAVE
	ClearAutoSave();
#endif

	SetComment("", false);		// // //

	// // // Allocate first song
	AllocateSong(0);

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
	SelectExpansionChip(SNDCHIP_NONE, 0, false);		// // //
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
// File load / save routines
////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Functions for compability with older file versions

void CFamiTrackerDoc::ReorderSequences(std::vector<COldSequence> seqs)		// // //
{
	int Slots[SEQ_COUNT] = {0, 0, 0, 0, 0};
	int Indices[MAX_SEQUENCES][SEQ_COUNT];

	memset(Indices, 0xFF, MAX_SEQUENCES * SEQ_COUNT * sizeof(int));

	// Organize sequences
	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (auto pInst = std::dynamic_pointer_cast<CInstrument2A03>(m_pInstrumentManager->GetInstrument(i))) {		// // //
			for (int j = 0; j < SEQ_COUNT; ++j) {
				if (pInst->GetSeqEnable(j)) {
					int Index = pInst->GetSeqIndex(j);
					if (Indices[Index][j] >= 0 && Indices[Index][j] != -1) {
						pInst->SetSeqIndex(j, Indices[Index][j]);
					}
					else {
						COldSequence &Seq = seqs[Index];		// // //
						if (j == SEQ_VOLUME)
							for (unsigned int k = 0; k < Seq.GetLength(); ++k)
								Seq.Value[k] = std::max(std::min<int>(Seq.Value[k], 15), 0);
						else if (j == SEQ_DUTYCYCLE)
							for (unsigned int k = 0; k < Seq.GetLength(); ++k)
								Seq.Value[k] = std::max(std::min<int>(Seq.Value[k], 3), 0);
						Indices[Index][j] = Slots[j];
						pInst->SetSeqIndex(j, Slots[j]);
						m_pInstrumentManager->SetSequence(INST_2A03, j, Slots[j]++, Seq.Convert(j).release());
					}
				}
				else
					pInst->SetSeqIndex(j, 0);
			}
		}
	}
}

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

CFamiTrackerDoc *CFamiTrackerDoc::LoadImportFile(LPCTSTR lpszPathName)
{
	// Import a module as new subtunes
	CFamiTrackerDoc *pImported = new CFamiTrackerDoc();

	pImported->CreateEmpty();

	// Load into a new document
	if (!pImported->OpenDocument(lpszPathName))
		SAFE_RELEASE(pImported);

	return pImported;
}

bool CFamiTrackerDoc::ImportInstruments(CFamiTrackerDoc *pImported, int *pInstTable)
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
	if (GetInstrumentCount() + pImported->GetInstrumentCount() > MAX_INSTRUMENTS) {
		// Out of instrument slots
		AfxMessageBox(IDS_IMPORT_INSTRUMENT_COUNT, MB_ICONERROR);
		return false;
	}

	static const inst_type_t inst[] = {INST_2A03, INST_VRC6, INST_N163, INST_S5B};		// // //
	int (*seqTable[])[SEQ_COUNT] = {SequenceTable2A03, SequenceTableVRC6, SequenceTableN163, SequenceTableS5B};

	// Copy sequences
	for (size_t i = 0; i < std::size(inst); i++) for (int t = 0; t < SEQ_COUNT; ++t) {
		if (GetSequenceCount(inst[i], t) + pImported->GetSequenceCount(inst[i], t) > MAX_SEQUENCES) {		// // //
			AfxMessageBox(IDS_IMPORT_SEQUENCE_COUNT, MB_ICONERROR);
			return false;
		}
		for (unsigned int s = 0; s < MAX_SEQUENCES; ++s) if (pImported->GetSequenceItemCount(inst[i], s, t) > 0) {
			CSequence *pImportSeq = pImported->GetSequence(inst[i], s, t);
			int index = -1;
			for (unsigned j = 0; j < MAX_SEQUENCES; ++j) {
				if (GetSequenceItemCount(inst[i], j, t)) continue;
				// TODO: continue if blank sequence is used by some instrument
				CSequence *pSeq = GetSequence(inst[i], j, t);
				pSeq->Copy(pImportSeq);
				// Save a reference to this sequence
				seqTable[i][s][t] = j;
				break;
			}
		}
	}

	bool bOutOfSampleSpace = false;

	// Copy DPCM samples
	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (const CDSample *pImportDSample = pImported->GetSample(i)) {		// // //
			int Index = GetFreeSampleSlot();
			if (Index != -1) {
				CDSample *pDSample = new CDSample(*pImportDSample);		// // //
				SetSample(Index, pDSample);
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
		if (pImported->IsInstrumentUsed(i)) {
			auto pInst = std::unique_ptr<CInstrument>(pImported->GetInstrument(i)->Clone());		// // //

			// Update references
			if (auto pSeq = dynamic_cast<CSeqInstrument *>(pInst.get())) {
				for (int t = 0; t < SEQ_COUNT; ++t)
					if (pSeq->GetSeqEnable(t)) {
						for (size_t j = 0; j < std::size(inst); j++)
							if (inst[j] == pInst->GetType()) {
								pSeq->SetSeqIndex(t, seqTable[j][pSeq->GetSeqIndex(t)][t]);
								break;
							}
					}
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

bool CFamiTrackerDoc::ImportGrooves(CFamiTrackerDoc *pImported, int *pGrooveMap)		// // //
{
	int Index = 0;
	for (int i = 0; i < MAX_GROOVE; ++i) {
		if (const CGroove *pGroove = pImported->GetGroove(i)) {
			while (GetGroove(Index))
				if (++Index >= MAX_GROOVE) {
					AfxMessageBox(IDS_IMPORT_GROOVE_SLOTS, MB_ICONEXCLAMATION);
					return false;
				}
			pGrooveMap[i] = Index;
			auto pNewGroove = std::make_unique<CGroove>();
			pNewGroove->Copy(pGroove);
			SetGroove(Index, std::move(pNewGroove));
		}
	}

	return true;
}

bool CFamiTrackerDoc::ImportDetune(CFamiTrackerDoc *pImported)		// // //
{
	for (int i = 0; i < 6; i++) for (int j = 0; j < NOTE_COUNT; j++)
		m_iDetuneTable[i][j] = pImported->GetDetuneOffset(i, j);

	theApp.GetSoundGenerator()->LoadMachineSettings();		// // //
	return true;
}

// End of file load/save

// DMC Stuff

const CDSample *CFamiTrackerDoc::GetSample(unsigned int Index) const
{
	ASSERT(Index < MAX_DSAMPLES);
	return m_pInstrumentManager->GetDSampleManager()->GetDSample(Index);		// // //
}

void CFamiTrackerDoc::SetSample(unsigned int Index, CDSample *pSamp)		// // //
{
	ASSERT(Index < MAX_DSAMPLES);
	if (m_pInstrumentManager->GetDSampleManager()->SetDSample(Index, pSamp)) {
		ModifyIrreversible();		// // //
	}
}

bool CFamiTrackerDoc::IsSampleUsed(unsigned int Index) const
{
	ASSERT(Index < MAX_DSAMPLES);
	return m_pInstrumentManager->GetDSampleManager()->IsSampleUsed(Index);		// // //
}

unsigned int CFamiTrackerDoc::GetSampleCount() const
{
	return m_pInstrumentManager->GetDSampleManager()->GetSampleCount();
}

int CFamiTrackerDoc::GetFreeSampleSlot() const
{
	return m_pInstrumentManager->GetDSampleManager()->GetFirstFree();
}

void CFamiTrackerDoc::RemoveSample(unsigned int Index)
{
	SetSample(Index, nullptr);		// // //
}

unsigned int CFamiTrackerDoc::GetTotalSampleSize() const
{
	return m_pInstrumentManager->GetDSampleManager()->GetTotalSize();
}

// ---------------------------------------------------------------------------------------------------------
// Document access functions
// ---------------------------------------------------------------------------------------------------------

//
// Sequences
//

CSequence *CFamiTrackerDoc::GetSequence(inst_type_t InstType, unsigned int Index, int Type) const		// // //
{
	return m_pInstrumentManager->GetSequence(InstType, Type, Index);
}

unsigned int CFamiTrackerDoc::GetSequenceItemCount(inst_type_t InstType, unsigned int Index, int Type) const		// // //
{
	ASSERT(Index < MAX_SEQUENCES);
	ASSERT(Type >= 0 && Type < SEQ_COUNT);

	const CSequence *pSeq = GetSequence(InstType, Index, Type);
	if (pSeq == NULL)
		return 0;
	return pSeq->GetItemCount();
}

int CFamiTrackerDoc::GetFreeSequence(inst_type_t InstType, int Type, CSeqInstrument *pInst) const		// // //
{
	ASSERT(Type >= 0 && Type < SEQ_COUNT);
	return m_pInstrumentManager->GetFreeSequenceIndex(InstType, Type, pInst);
}

int CFamiTrackerDoc::GetSequenceCount(inst_type_t InstType, int Type) const		// // //
{
	// Return number of allocated sequences of Type
	ASSERT(Type >= 0 && Type < SEQ_COUNT);

	int Count = 0;
	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		if (GetSequenceItemCount(InstType, i, Type) > 0) // TODO: fix this and the instrument interface
			++Count;
	}
	return Count;
}

int CFamiTrackerDoc::GetTotalSequenceCount(inst_type_t InstType) const {		// // //
	int Count = 0;
	for (int i = 0; i < SEQ_COUNT; ++i)
		Count += GetSequenceCount(InstType, i);
	return Count;
}

//
// Song info
//

std::string_view CFamiTrackerDoc::GetModuleName() const		// // //
{
	return m_strName; 
}

std::string_view CFamiTrackerDoc::GetModuleArtist() const
{ 
	return m_strArtist; 
}

std::string_view CFamiTrackerDoc::GetModuleCopyright() const
{ 
	return m_strCopyright; 
}

void CFamiTrackerDoc::SetModuleName(std::string_view pName)
{
	m_strName = pName.substr(0, METADATA_FIELD_LENGTH - 1);		// // //
}

void CFamiTrackerDoc::SetModuleArtist(std::string_view pArtist)
{
	m_strArtist = pArtist.substr(0, METADATA_FIELD_LENGTH - 1);		// // //
}

void CFamiTrackerDoc::SetModuleCopyright(std::string_view pCopyright)
{
	m_strCopyright = pCopyright.substr(0, METADATA_FIELD_LENGTH - 1);		// // //
}

//
// Instruments
//

std::shared_ptr<CInstrument> CFamiTrackerDoc::GetInstrument(unsigned int Index) const
{
	return m_pInstrumentManager->GetInstrument(Index);
}

unsigned int CFamiTrackerDoc::GetInstrumentCount() const
{
	return m_pInstrumentManager->GetInstrumentCount();
}

unsigned CFamiTrackerDoc::GetFreeInstrumentIndex() const {		// // //
	return m_pInstrumentManager->GetFirstUnused();
}

bool CFamiTrackerDoc::IsInstrumentUsed(unsigned int Index) const
{
	return m_pInstrumentManager->IsInstrumentUsed(Index);
}

bool CFamiTrackerDoc::AddInstrument(std::unique_ptr<CInstrument> pInstrument, unsigned int Slot)		// // //
{
	return m_pInstrumentManager->InsertInstrument(Slot, std::move(pInstrument));
}

bool CFamiTrackerDoc::RemoveInstrument(unsigned int Index)		// // //
{
	return m_pInstrumentManager->RemoveInstrument(Index);
}

int CFamiTrackerDoc::CloneInstrument(unsigned int Index)
{
	if (!IsInstrumentUsed(Index))
		return INVALID_INSTRUMENT;

	const int Slot = m_pInstrumentManager->GetFirstUnused();

	if (Slot != INVALID_INSTRUMENT) {
		auto pInst = std::unique_ptr<CInstrument>(m_pInstrumentManager->GetInstrument(Index)->Clone());
		if (!AddInstrument(std::move(pInst), Slot))		// // //
			return INVALID_INSTRUMENT;
	}

	return Slot;
}

inst_type_t CFamiTrackerDoc::GetInstrumentType(unsigned int Index) const
{
	return m_pInstrumentManager->GetInstrumentType(Index);
}

int CFamiTrackerDoc::DeepCloneInstrument(unsigned int Index) 
{
	int Slot = CloneInstrument(Index);

	if (Slot != INVALID_INSTRUMENT) {
		auto newInst = m_pInstrumentManager->GetInstrument(Slot);
		const inst_type_t it = newInst->GetType();
		if (auto pInstrument = std::dynamic_pointer_cast<CSeqInstrument>(newInst)) {
			for (int i = 0; i < SEQ_COUNT; i++) {
				int freeSeq = m_pInstrumentManager->GetFreeSequenceIndex(it, i, pInstrument.get());
				if (freeSeq != -1) {
					if (pInstrument->GetSeqEnable(i))
						GetSequence(it, unsigned(freeSeq), i)->Copy(pInstrument->GetSequence(i));
					pInstrument->SetSeqIndex(i, freeSeq);
				}
			}
		}
	}

	return Slot;
}

void CFamiTrackerDoc::SaveInstrument(unsigned int Index, CSimpleFile &file) const
{
	GetInstrument(Index)->SaveFTI(file);
}

int CFamiTrackerDoc::LoadInstrument(CString FileName)
{
	// FTI instruments files
	static const char INST_HEADER[] = "FTI";
	static const char INST_VERSION[] = "2.4";

	// Loads an instrument from file, return allocated slot or INVALID_INSTRUMENT if failed
	//
	int iInstMaj, iInstMin;
	// // // sscanf_s(INST_VERSION, "%i.%i", &iInstMaj, &iInstMin);
	static const int I_CURRENT_VER = 2 * 10 + 5;		// // // 050B
	
	int Slot = m_pInstrumentManager->GetFirstUnused();
	try {
		if (Slot == INVALID_INSTRUMENT)
			throw IDS_INST_LIMIT;

		// Open file
		// // // CFile implements RAII
		CSimpleFile file(FileName, std::ios::in | std::ios::binary);
		if (!file)
			throw IDS_FILE_OPEN_ERROR;

		// Signature
		const UINT HEADER_LEN = strlen(INST_HEADER);
		char Text[256] = {};
		file.ReadBytes(Text, HEADER_LEN);
		if (strcmp(Text, INST_HEADER) != 0)
			throw IDS_INSTRUMENT_FILE_FAIL;
		
		// Version
		file.ReadBytes(Text, static_cast<UINT>(strlen(INST_VERSION)));
		sscanf_s(Text, "%i.%i", &iInstMaj, &iInstMin);		// // //
		int iInstVer = iInstMaj * 10 + iInstMin;
		if (iInstVer > I_CURRENT_VER)
			throw IDS_INST_VERSION_UNSUPPORTED;
		
		LockDocument();

		inst_type_t InstType = static_cast<inst_type_t>(file.ReadChar());
		if (InstType == INST_NONE)
			InstType = INST_2A03;
		auto pInstrument = m_pInstrumentManager->CreateNew(InstType);
		if (!pInstrument) {
			UnlockDocument();
			AfxMessageBox("Failed to create instrument", MB_ICONERROR);
			return INVALID_INSTRUMENT;
		}
		
		// Name
		std::string InstName = file.ReadString();
		if (InstName.size() > static_cast<unsigned>(CInstrument::INST_NAME_MAX))
			InstName = InstName.substr(0, CInstrument::INST_NAME_MAX);
		pInstrument->SetName(InstName.c_str());

		pInstrument->LoadFTI(file, iInstVer);		// // //
		m_pInstrumentManager->InsertInstrument(Slot, std::move(pInstrument));
		UnlockDocument();
		return Slot;
	}
	catch (int ID) {		// // // TODO: put all error messages into string table then add exception ctor
		UnlockDocument();
		AfxMessageBox(ID, MB_ICONERROR);
		return INVALID_INSTRUMENT;
	}
	catch (CModuleException e) {
		UnlockDocument();
		m_pInstrumentManager->RemoveInstrument(Slot);
		AfxMessageBox(e.GetErrorString().c_str(), MB_ICONERROR);
		return INVALID_INSTRUMENT;
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

unsigned int CFamiTrackerDoc::GetEffColumns(unsigned int Track, unsigned int Channel) const
{
	ASSERT(Channel < MAX_CHANNELS);
	return GetSongData(Track).GetEffectColumnCount(Channel);
}

void CFamiTrackerDoc::SetEffColumns(unsigned int Track, unsigned int Channel, unsigned int Columns)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Columns < MAX_EFFECT_COLUMNS);

	GetSongData(Track).SetEffectColumnCount(Channel, Columns);
}

void CFamiTrackerDoc::SetEngineSpeed(unsigned int Speed)
{
	ASSERT(Speed >= 16 || Speed == 0);		// // //
	m_iEngineSpeed = Speed;
}

void CFamiTrackerDoc::SetMachine(machine_t Machine)
{
	ASSERT(Machine == PAL || Machine == NTSC);
	m_iMachine = Machine;
}

unsigned int CFamiTrackerDoc::GetPatternAtFrame(unsigned int Track, unsigned int Frame, unsigned int Channel) const
{
	ASSERT(Frame < MAX_FRAMES && Channel < MAX_CHANNELS);
	return GetSongData(Track).GetFramePattern(Frame, Channel);
}

void CFamiTrackerDoc::SetPatternAtFrame(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Pattern)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Pattern < MAX_PATTERN);

	GetSongData(Track).SetFramePattern(Frame, Channel, Pattern);
}

unsigned int CFamiTrackerDoc::GetFrameRate() const
{
	if (m_iEngineSpeed == 0)
		return (m_iMachine == NTSC) ? CAPU::FRAME_RATE_NTSC : CAPU::FRAME_RATE_PAL;
	
	return m_iEngineSpeed;
}

//// Pattern functions ////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerDoc::SetNoteData(unsigned Track, unsigned Frame, unsigned Channel, unsigned Row, const stChanNote &Data)		// // //
{
	GetSongData(Track).GetPatternOnFrame(Channel, Frame).SetNoteOn(Row, Data);		// // //
}

const stChanNote &CFamiTrackerDoc::GetNoteData(unsigned Track, unsigned Frame, unsigned Channel, unsigned Row) const
{
	return GetSongData(Track).GetPatternOnFrame(Channel, Frame).GetNoteOn(Row);		// // //
}

stChanNote CFamiTrackerDoc::GetActiveNote(unsigned Track, unsigned Frame, unsigned Channel, unsigned Row) const {		// // //
	auto Note = GetNoteData(Track, Frame, Channel, Row);
	for (int i = GetEffColumns(Track, Channel) + 1; i < MAX_EFFECT_COLUMNS; ++i)
		Note.EffNumber[i] = EF_NONE;
	return Note;
}

void CFamiTrackerDoc::SetDataAtPattern(unsigned Track, unsigned Pattern, unsigned Channel, unsigned Row, const stChanNote &Data)		// // //
{
	// Set a note to a direct pattern
	GetSongData(Track).SetPatternData(Channel, Pattern, Row, Data);		// // //
}

const stChanNote &CFamiTrackerDoc::GetDataAtPattern(unsigned Track, unsigned Pattern, unsigned Channel, unsigned Row) const		// // //
{
	// Get note from a direct pattern
	return GetSongData(Track).GetPatternData(Channel, Pattern, Row);		// // //
}

bool CFamiTrackerDoc::InsertRow(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	auto &Song = GetSongData(Track);
	auto &Pattern = Song.GetPatternOnFrame(Channel, Frame);		// // //

	for (unsigned int i = Song.GetPatternLength() - 1; i > Row; --i)
		Pattern.SetNoteOn(i, Pattern.GetNoteOn(i - 1));
	Pattern.SetNoteOn(Row, { });

	return true;
}

void CFamiTrackerDoc::ClearPattern(unsigned int Track, unsigned int Frame, unsigned int Channel)
{
	// Clear entire pattern
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);

	auto &Song = GetSongData(Track);
	int Pattern = Song.GetFramePattern(Frame, Channel);
	Song.ClearPattern(Channel, Pattern);
}

bool CFamiTrackerDoc::ClearRowField(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row, cursor_column_t Column)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	auto &Song = GetSongData(Track);
	int Pattern = Song.GetFramePattern(Frame, Channel);
	stChanNote &Note = Song.GetPatternData(Channel, Pattern, Row);		// // //

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

bool CFamiTrackerDoc::RemoveNote(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ASSERT(Row < MAX_PATTERN_LENGTH);

	auto &Song = GetSongData(Track);
	int Pattern = Song.GetFramePattern(Frame, Channel);

	unsigned int PatternLen = Song.GetPatternLength();

	for (unsigned int i = Row - 1; i < (PatternLen - 1); ++i)
		SetDataAtPattern(Track, Pattern, Channel, i,
			GetDataAtPattern(Track, Pattern, Channel, i + 1));		// // //
	SetDataAtPattern(Track, Pattern, Channel, PatternLen - 1, { });		// // //

	return true;
}

bool CFamiTrackerDoc::PullUp(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	auto &Song = GetSongData(Track);		// // //
	auto &Pattern = Song.GetPatternOnFrame(Channel, Frame);
	int PatternLen = Song.GetPatternLength();

	for (int i = Row; i < PatternLen - 1; ++i)
		Pattern.SetNoteOn(i, Pattern.GetNoteOn(i + 1));		// // //
	Pattern.SetNoteOn(PatternLen - 1, { });

	return true;
}

void CFamiTrackerDoc::CopyPattern(unsigned int Track, int Target, int Source, int Channel)
{
	// Copy one pattern to another

	auto &Song = GetSongData(Track);
	Song.GetPattern(Channel, Target) = Song.GetPattern(Channel, Source);		// // //
}

void CFamiTrackerDoc::SwapChannels(unsigned int Track, unsigned int First, unsigned int Second)		// // //
{
	ASSERT(First < MAX_CHANNELS);
	ASSERT(Second < MAX_CHANNELS);

	GetSongData(Track).SwapChannels(First, Second);
}

//// Frame functions //////////////////////////////////////////////////////////////////////////////////

bool CFamiTrackerDoc::InsertFrame(unsigned int Track, unsigned int Frame)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES);

	if (!AddFrames(Track, Frame, 1))
		return false;
	// Select free patterns 
	for (int i = 0, Channels = GetChannelCount(); i < Channels; ++i) {
		unsigned Pattern = GetFirstFreePattern(Track, i);		// // //
		SetPatternAtFrame(Track, Frame, i, Pattern == -1 ? 0 : Pattern);
	}

	return true;
}

bool CFamiTrackerDoc::RemoveFrame(unsigned int Track, unsigned int Frame)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES);

	const int FrameCount = GetFrameCount(Track);
	const int Channels = GetAvailableChannels();

	if (FrameCount == 1)
		return false;

	for (int i = Frame; i < FrameCount - 1; ++i)
		for (int j = 0; j < Channels; ++j)
			SetPatternAtFrame(Track, i, j, GetPatternAtFrame(Track, i + 1, j));

	for (int i = 0; i < Channels; ++i)
		SetPatternAtFrame(Track, FrameCount - 1, i, 0);		// // //
	
	GetBookmarkCollection(Track)->RemoveFrames(Frame, 1U);		// // //

	SetFrameCount(Track, FrameCount - 1);

	return true;
}

bool CFamiTrackerDoc::DuplicateFrame(unsigned int Track, unsigned int Frame)
{
	// Create a copy of selected frame
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES);

	const int Frames = GetFrameCount(Track);
	const int Channels = GetAvailableChannels();

	if (Frames == MAX_FRAMES)
		return false;

	SetFrameCount(Track, Frames + 1);

	for (unsigned int i = Frames; i > (Frame + 1); --i)
		for (int j = 0; j < Channels; ++j)
			SetPatternAtFrame(Track, i, j, GetPatternAtFrame(Track, i - 1, j));

	for (int i = 0; i < Channels; ++i) 
		SetPatternAtFrame(Track, Frame + 1, i, GetPatternAtFrame(Track, Frame, i));

	GetBookmarkCollection(Track)->InsertFrames(Frame + 1, 1U);		// // //

	return true;
}

bool CFamiTrackerDoc::CloneFrame(unsigned int Track, unsigned int Frame)		// // // renamed
{
	ASSERT(Track < MAX_TRACKS);

	// Create a copy of selected frame including patterns
	int Frames = GetFrameCount(Track);
	int Channels = GetAvailableChannels();

	// insert new frame with next free pattern numbers
	if (!InsertFrame(Track, Frame))
		return false;

	// copy old patterns into new
	auto &Song = GetSongData(Track);		// / ///
	for (int i = 0; i < Channels; ++i)
		Song.GetPattern(i, Song.GetFramePattern(Frame, i)) = Song.GetPattern(i, Song.GetFramePattern(Frame - 1, i));

	return true;
}

bool CFamiTrackerDoc::MoveFrameDown(unsigned int Track, unsigned int Frame)
{
	int Channels = GetAvailableChannels();

	if (Frame == (GetFrameCount(Track) - 1))
		return false;

	for (int i = 0; i < Channels; ++i) {
		int Pattern = GetPatternAtFrame(Track, Frame, i);
		SetPatternAtFrame(Track, Frame, i, GetPatternAtFrame(Track, Frame + 1, i));
		SetPatternAtFrame(Track, Frame + 1, i, Pattern);
	}

	GetBookmarkCollection(Track)->SwapFrames(Frame, Frame + 1);		// // //

	return true;
}

bool CFamiTrackerDoc::MoveFrameUp(unsigned int Track, unsigned int Frame)
{
	int Channels = GetAvailableChannels();

	if (Frame == 0)
		return false;

	for (int i = 0; i < Channels; ++i) {
		int Pattern = GetPatternAtFrame(Track, Frame, i);
		SetPatternAtFrame(Track, Frame, i, GetPatternAtFrame(Track, Frame - 1, i));
		SetPatternAtFrame(Track, Frame - 1, i, Pattern);
	}
	
	GetBookmarkCollection(Track)->SwapFrames(Frame, Frame - 1);		// // //

	return true;
}

bool CFamiTrackerDoc::AddFrames(unsigned int Track, unsigned int Frame, int Count)		// // //
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES);

	const int FrameCount = GetFrameCount(Track);
	const int Channels = GetAvailableChannels();

	if (FrameCount + Count > MAX_FRAMES)
		return false;

	SetFrameCount(Track, FrameCount + Count);

	for (unsigned int i = FrameCount + Count - 1; i >= Frame + Count; --i)
		for (int j = 0; j < Channels; ++j)
			SetPatternAtFrame(Track, i, j, GetPatternAtFrame(Track, i - Count, j));

	for (int i = 0; i < Channels; ++i)
		for (int f = 0; f < Count; ++f)		// // //
			SetPatternAtFrame(Track, Frame + f, i, 0);

	GetBookmarkCollection(Track)->InsertFrames(Frame, Count);		// // //

	return true;
}

bool CFamiTrackerDoc::DeleteFrames(unsigned int Track, unsigned int Frame, int Count)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES);

	for (int i = 0; i < Count; ++i)
		RemoveFrame(Track, Frame);

	return true;
}

//// Track functions //////////////////////////////////////////////////////////////////////////////////

const std::string &CFamiTrackerDoc::GetTrackTitle(unsigned int Track) const		// // //
{
	if (Track < GetTrackCount())
		return GetSongData(Track).GetTitle();
	return CSongData::DEFAULT_TITLE;
}

int CFamiTrackerDoc::AddTrack()
{
	// Add new track. Returns -1 on failure, or added track number otherwise
	int NewTrack = GetTrackCount();
	if (NewTrack >= MAX_TRACKS)
		return -1;

	AllocateSong(NewTrack);
	return NewTrack;
}

int CFamiTrackerDoc::AddTrack(std::unique_ptr<CSongData> song) {		// // //
	int NewTrack = GetTrackCount();
	if (NewTrack >= MAX_TRACKS)
		return -1;

	m_pTracks.push_back(std::move(song));
	return NewTrack;
}

void CFamiTrackerDoc::RemoveTrack(unsigned int Track)
{
	(void)ReleaseTrack(Track);
}

std::unique_ptr<CSongData> CFamiTrackerDoc::ReleaseTrack(unsigned int Track)		// // //
{
	if (Track >= GetTrackCount())
		return nullptr;

	// Move down all other tracks
	auto song = std::move(m_pTracks[Track]);
	m_pTracks.erase(m_pTracks.cbegin() + Track);		// // //
	if (!GetTrackCount())
		AllocateSong(0);

	return song;
}

void CFamiTrackerDoc::SetTrackTitle(unsigned int Track, const std::string &title)		// // //
{
	GetSongData(Track).SetTitle(title);
}

void CFamiTrackerDoc::MoveTrackUp(unsigned int Track)
{
	ASSERT(Track > 0);

	SwapSongs(Track, Track - 1);
}

void CFamiTrackerDoc::MoveTrackDown(unsigned int Track)
{
	ASSERT(Track < MAX_TRACKS);

	SwapSongs(Track, Track + 1);
}

void CFamiTrackerDoc::SwapSongs(unsigned int First, unsigned int Second)
{
	m_pTracks[First].swap(m_pTracks[Second]);		// // //
}

void CFamiTrackerDoc::AllocateSong(unsigned int Index)
{
	// Allocate a new song if not already done
	ASSERT(Index < MAX_TRACKS);
	while (Index >= m_pTracks.size()) {		// // //
		auto &pSong = m_pTracks.emplace_back(std::make_unique<CSongData>());		// // //
		pSong->SetSongTempo(m_iMachine == NTSC ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL);
	}
}

CSongData &CFamiTrackerDoc::GetSongData(unsigned int Index)		// // //
{
	// Ensure track is allocated
	AllocateSong(Index);
	return *m_pTracks[Index];
}

const CSongData &CFamiTrackerDoc::GetSongData(unsigned int Index) const		// // //
{
	return *m_pTracks[Index];
}

std::unique_ptr<CSongData> CFamiTrackerDoc::ReplaceSong(unsigned Index, std::unique_ptr<CSongData> pSong) {		// // //
	m_pTracks[Index].swap(pSong);
	return std::move(pSong);
}

unsigned int CFamiTrackerDoc::GetTrackCount() const
{
	return m_pTracks.size();
}

void CFamiTrackerDoc::SelectExpansionChip(unsigned chips, unsigned n163chs, bool Move) {		// // //
	ASSERT(n163chs <= 8 && !(chips & SNDCHIP_N163) == !n163chs);

	// // // Move pattern data upon removing expansion chips
	if (Move) {
		int oldIndex[CHANNELS] = {};
		int newIndex[CHANNELS] = {};
		for (int j = 0; j < CHANNELS; ++j) {
			oldIndex[j] = GetChannelPosition(j, GetExpansionChip(), GetNamcoChannels());
			newIndex[j] = GetChannelPosition(j, chips, n163chs);
		}
		auto it = m_pTracks.begin();
		for (const auto &pSong : m_pTracks) {
			auto pNew = std::make_unique<CSongData>(pSong->GetPatternLength());
			pNew->SetHighlight(pSong->GetRowHighlight());
			pNew->SetSongTempo(pSong->GetSongTempo());
			pNew->SetSongSpeed(pSong->GetSongSpeed());
			pNew->SetSongGroove(pSong->GetSongGroove());
			pNew->SetFrameCount(pSong->GetFrameCount());
			pNew->SetTitle(pSong->GetTitle());
			for (int j = 0; j < CHANNELS; ++j)
				if (oldIndex[j] != -1 && newIndex[j] != -1)
					pNew->CopyTrack(newIndex[j], *pSong, oldIndex[j]);
			*it++ = std::move(pNew);
		}
	}

	// Complete sound chip setup
	SetupChannels(chips, n163chs);
	ApplyExpansionChip();
	ModifyIrreversible();
}

unsigned char CFamiTrackerDoc::GetExpansionChip() const {
	return m_pChannelMap->GetExpansionFlag();		// // //
}

void CFamiTrackerDoc::SetupChannels(unsigned chips, unsigned n163chs)
{
	// This will select a chip in the sound emulator
	LockDocument();

	// Do not allow expansion chips in PAL mode
	if (chips != SNDCHIP_NONE)
		SetMachine(NTSC);

	// Register the channels
	m_pChannelMap = theApp.GetSoundGenerator()->MakeChannelMap(chips, n163chs);		// // //
	m_iChannelsAvailable = GetChannelCount();

	UnlockDocument();
	// Must call ApplyExpansionChip after this
}

void CFamiTrackerDoc::ApplyExpansionChip() const
{
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
	return m_pInstrumentManager->GetSequenceManager(InstType);
}

CInstrumentManager *const CFamiTrackerDoc::GetInstrumentManager() const
{
	return m_pInstrumentManager.get();
}

CDSampleManager *const CFamiTrackerDoc::GetDSampleManager() const
{
	return m_pInstrumentManager->GetDSampleManager();
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

unsigned int CFamiTrackerDoc::GetFirstFreePattern(unsigned int Track, unsigned int Channel) const
{
	return GetSongData(Track).GetFreePatternIndex(Channel);		// // //
}

bool CFamiTrackerDoc::IsPatternEmpty(unsigned int Track, unsigned int Channel, unsigned int Pattern) const
{
	return GetSongData(Track).IsPatternEmpty(Channel, Pattern);
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

CTrackerChannel &CFamiTrackerDoc::GetChannel(int Index) const		// // //
{
	return m_pChannelMap->GetChannel(Index);		// // //
}

int CFamiTrackerDoc::GetChannelIndex(int Channel) const
{
	return m_pChannelMap->GetChannelIndex(Channel);		// // //
}

// Vibrato functions

vibrato_t CFamiTrackerDoc::GetVibratoStyle() const
{
	return m_iVibratoStyle;
}

void CFamiTrackerDoc::SetVibratoStyle(vibrato_t Style)
{
	m_iVibratoStyle = Style;
}

// Linear pitch slides

bool CFamiTrackerDoc::GetLinearPitch() const
{
	return m_bLinearPitch;
}

void CFamiTrackerDoc::SetLinearPitch(bool Enable)
{
	m_bLinearPitch = Enable;
}

// Attributes

CString CFamiTrackerDoc::GetFileTitle() const 
{
	// Return file name without extension
	CString FileName = GetTitle();

	static const LPCSTR EXT[] = {_T(".ftm"), _T(".0cc"), _T(".ftm.bak"), _T(".0cc.bak")};		// // //
	// Remove extension

	for (size_t i = 0; i < sizeof(EXT) / sizeof(LPCSTR); ++i) {
		int Len = lstrlen(EXT[i]);
		if (FileName.Right(Len).CompareNoCase(EXT[i]) == 0)
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

void CFamiTrackerDoc::SetComment(const std::string &comment, bool bShowOnLoad)		// // //
{
	m_strComment = comment;
	m_bDisplayComment = bShowOnLoad;
}

const std::string &CFamiTrackerDoc::GetComment() const		// // //
{
	return m_strComment;
}

bool CFamiTrackerDoc::ShowCommentOnOpen() const
{
	return m_bDisplayComment;
}

void CFamiTrackerDoc::SetSpeedSplitPoint(int SplitPoint)
{
	m_iSpeedSplitPoint = SplitPoint;
}

int CFamiTrackerDoc::GetSpeedSplitPoint() const
{
	return m_iSpeedSplitPoint;
}

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
	m_vHighlight = Hl;
}

const stHighlight &CFamiTrackerDoc::GetHighlight() const		// // //
{
	return m_vHighlight;
}

stHighlight CFamiTrackerDoc::GetHighlightAt(unsigned int Track, unsigned int Frame, unsigned int Row) const		// // //
{
	while (Frame < 0) Frame += GetFrameCount(Track);
	Frame %= GetFrameCount(Track);

	stHighlight Hl = m_vHighlight;
	
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
	loop_visitor(const CSongData &song, int channels) : song_(song), channels_(channels) { }

	template <typename F>
	void Visit(F cb) {
		unsigned FrameCount = song_.GetFrameCount();
		unsigned Rows = song_.GetPatternLength();

		std::bitset<MAX_PATTERN_LENGTH> RowVisited[MAX_FRAMES] = { };		// // //
		while (!RowVisited[f_][r_]) {
			RowVisited[f_][r_] = true;

			int Bxx = -1;
			int Dxx = -1;
			bool Cxx = false;
			for (int ch = 0; ch < channels_; ++ch) {
				const auto &Note = song_.GetPatternOnFrame(ch, f_).GetNoteOn(r_);		// // //
				for (int l = 0; l <= song_.GetEffectColumnCount(ch); ++l) {
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
	const CSongData &song_;
	int channels_;
	unsigned f_ = 0;
	unsigned r_ = 0;
	bool first_ = true;
};

unsigned int CFamiTrackerDoc::ScanActualLength(unsigned int Track, unsigned int Count) const		// // //
{
	// Return number for frames played for a certain number of loops
	const auto visit = [] (const CSongData &song, int channels, auto f1, auto f2) {
		loop_visitor visitor(song, channels);
		visitor.Visit(f1);
		visitor.Visit(f2);
	};

	int FirstLoop = 0;
	int SecondLoop = 0;

	loop_visitor visitor(GetSongData(Track), GetChannelCount());
	visitor.Visit([&] { ++FirstLoop; });
	visitor.Visit([&] { ++SecondLoop; });

	return FirstLoop + SecondLoop * (Count - 1);		// // //
}

double CFamiTrackerDoc::GetStandardLength(int Track, unsigned int ExtraLoops) const		// // //
{
	char RowVisited[MAX_FRAMES][MAX_PATTERN_LENGTH] = { };
	int JumpTo = -1;
	int SkipTo = -1;
	double FirstLoop = 0.0;
	double SecondLoop = 0.0;
	bool IsGroove = GetSongGroove(Track);
	double Tempo = GetSongTempo(Track);
	double Speed = GetSongSpeed(Track);
	if (!GetSongTempo(Track))
		Tempo = 2.5 * GetFrameRate();
	int GrooveIndex = GetSongSpeed(Track) * (m_pGrooveTable[GetSongSpeed(Track)] != NULL), GroovePointer = 0;
	bool bScanning = true;
	unsigned int FrameCount = GetFrameCount(Track);

	if (IsGroove && GetGroove(GetSongSpeed(Track)) == NULL) {
		IsGroove = false;
		Speed = DEFAULT_SPEED;
	}

	unsigned int f = 0;
	unsigned int r = 0;
	while (bScanning) {
		bool hasJump = false;
		for (int j = 0; j < GetChannelCount(); ++j) {
			const auto &Note = GetNoteData(Track, f, j, r);
			for (unsigned l = 0; l < GetEffColumns(Track, j) + 1; ++l) {
				switch (Note.EffNumber[l]) {
				case EF_JUMP:
					JumpTo = Note.EffParam[l];
					SkipTo = 0;
					hasJump = true;
					break;
				case EF_SKIP:
					if (hasJump) break;
					JumpTo = (f + 1) % FrameCount;
					SkipTo = Note.EffParam[l];
					break;
				case EF_HALT:
					ExtraLoops = 0;
					bScanning = false;
					break;
				case EF_SPEED:
					if (GetSongTempo(Track) && Note.EffParam[l] >= m_iSpeedSplitPoint)
						Tempo = Note.EffParam[l];
					else {
						IsGroove = false;
						Speed = Note.EffParam[l];
					}
					break;
				case EF_GROOVE:
					if (m_pGrooveTable[Note.EffParam[l]] == NULL) break;
					IsGroove = true;
					GrooveIndex = Note.EffParam[l];
					GroovePointer = 0;
					break;
				}
			}
		}
		if (IsGroove)
			Speed = m_pGrooveTable[GrooveIndex]->GetEntry(GroovePointer++);
		
		switch (RowVisited[f][r]) {
		case 0: FirstLoop += Speed / Tempo; break;
		case 1: SecondLoop += Speed / Tempo; break;
		case 2: bScanning = false; break;
		}
		
		++RowVisited[f][r++];

		if (JumpTo > -1) {
			f = std::min(static_cast<unsigned int>(JumpTo), FrameCount - 1);
			JumpTo = -1;
		}
		if (SkipTo > -1) {
			r = std::min(static_cast<unsigned int>(SkipTo), GetPatternLength(Track) - 1);
			SkipTo = -1;
		}
		if (r >= GetPatternLength(Track)) {		// // //
			++f;
			r = 0;
		}
		if (f >= FrameCount)
			f = 0;
	}

	return (2.5 * (FirstLoop + SecondLoop * ExtraLoops));
}

// Operations

void CFamiTrackerDoc::RemoveUnusedInstruments()
{
	bool used[MAX_INSTRUMENTS] = { };		// // //

	for (const auto &pSong : m_pTracks) {		// // //
		int length = pSong->GetPatternLength();
		for (unsigned int Channel = 0; Channel < m_iChannelsAvailable; ++Channel)
			for (unsigned int Frame = 0; Frame < pSong->GetFrameCount(); ++Frame)
				pSong->GetPatternOnFrame(Channel, Frame).VisitRows(length, [&] (const stChanNote &note) {
					if (note.Instrument < MAX_INSTRUMENTS)
						used[note.Instrument] = true;
				});
	}

	for (int i = 0; i < MAX_INSTRUMENTS; ++i)
		if (IsInstrumentUsed(i) && !used[i])
			RemoveInstrument(i);

	static const inst_type_t inst[] = {INST_2A03, INST_VRC6, INST_N163, INST_S5B};

	// Also remove unused sequences
	for (unsigned int i = 0; i < MAX_SEQUENCES; ++i)
		for (int j = 0; j < SEQ_COUNT; ++j)
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
						GetSequence(inst[c], i, j)->Clear();
				}
}

void CFamiTrackerDoc::RemoveUnusedPatterns()
{
	VisitSongs([] (CSongData &song) {
		song.VisitPatterns([&song] (CPatternData &, unsigned c, unsigned p) {
			if (!song.IsPatternInUse(c, p))
				song.ClearPattern(c, p);
		});
	});
}

void CFamiTrackerDoc::RemoveUnusedSamples()		// // //
{
	bool AssignUsed[MAX_INSTRUMENTS][OCTAVE_RANGE][NOTE_RANGE] = { };

	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (IsSampleUsed(i)) {
			bool Used = false;
			for (const auto &pSong : m_pTracks) {
				for (unsigned int Frame = 0; Frame < pSong->GetFrameCount(); ++Frame) {
					unsigned int Pattern = pSong->GetFramePattern(Frame, CHANID_DPCM);
					for (unsigned int Row = 0; Row < pSong->GetPatternLength(); ++Row) {
						const auto &Note = pSong->GetPatternData(CHANID_DPCM, Pattern, Row);		// // //
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
			}
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

bool CFamiTrackerDoc::ArePatternsSame(unsigned int Track, unsigned int Channel, unsigned int Pattern1, unsigned int Pattern2) const		// // //
{
	const auto &song = GetSongData(Track);
	return song.GetPattern(Channel, Pattern1) == song.GetPattern(Channel, Pattern2);
}

void CFamiTrackerDoc::SwapInstruments(int First, int Second)
{
	// Swap instruments
	m_pInstrumentManager->SwapInstruments(First, Second);		// // //
	
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

void CFamiTrackerDoc::SetDetuneOffset(int Chip, int Note, int Detune)		// // //
{
	m_iDetuneTable[Chip][Note] = Detune;
}

int CFamiTrackerDoc::GetDetuneOffset(int Chip, int Note) const		// // //
{
	return m_iDetuneTable[Chip][Note];
}

void CFamiTrackerDoc::ResetDetuneTables()		// // //
{
	for (int i = 0; i < 6; i++) for (int j = 0; j < NOTE_COUNT; j++)
		m_iDetuneTable[i][j] = 0;
}

void CFamiTrackerDoc::SetTuning(int Semitone, int Cent)		// // // 050B
{
	m_iDetuneSemitone = Semitone;
	m_iDetuneCent = Cent;
}

int CFamiTrackerDoc::GetTuningSemitone() const		// // // 050B
{
	return m_iDetuneSemitone;
}

int CFamiTrackerDoc::GetTuningCent() const		// // // 050B
{
	return m_iDetuneCent;
}

CGroove *CFamiTrackerDoc::GetGroove(unsigned Index) const		// // //
{
	return Index < MAX_GROOVE ? m_pGrooveTable[Index].get() : nullptr;
}

void CFamiTrackerDoc::SetGroove(unsigned Index, std::unique_ptr<CGroove> Groove)
{
	m_pGrooveTable[Index] = std::move(Groove);
}

void CFamiTrackerDoc::SetExceededFlag(bool Exceed)
{
	m_bExceeded = Exceed;
}

int CFamiTrackerDoc::GetFrameLength(unsigned int Track, unsigned int Frame) const
{
	// // // moved from PatternEditor.cpp
	return GetSongData(Track).GetFrameSize(Frame, GetChannelCount());		// // //
}
