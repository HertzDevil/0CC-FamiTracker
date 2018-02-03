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


#pragma once

#include "stdafx.h"		// // //

#include <memory>		// // //
#include <string>		// // //
#include "APU/Types_fwd.h"		// // //

// Constants, types and enums
#include "FTMComponentInterface.h"

// #define DISABLE_SAVE		// // //

// External classes
class CFamiTrackerModule;		// // //
class CSongData;		// // //
class CSongView;		// // //
class CTrackerChannel;
class CDocumentFile;
class CSimpleFile;		// // //
class CBookmark;		// // //
class CBookmarkCollection;		// // //
class CInstrument;		// // //
class CSequence;		// // //
class stChanNote;		// // //
struct stHighlight;		// // //
class CSoundChipSet;		// // //

enum inst_type_t : unsigned;
enum class sequence_t : unsigned;
enum machine_t : unsigned char;
enum vibrato_t : unsigned char;

namespace ft0cc::doc {		// // //
class dpcm_sample;
} // namespace ft0cc::doc

//
// I'll try to organize this class, things are quite messy right now!
//

// // // TODO:
// // // + move core data fields into CFamiTrackerModule
// // // - move high-level pattern operations to CSongView
// // // - remove these delegations
// // //    - CFamiTrackerModule
// // //    - CSongData
// // //    - CSongView
// // // - use these classes directly in other classes
// // //    - CFamiTrackerModule
// // //    - CSongData
// // //    - CSongView
// // // - move action handler into CFamiTrackerDoc

class CFamiTrackerDoc : public CDocument, public CFTMComponentInterface
{
	using dpcm_sample = ft0cc::doc::dpcm_sample;

	struct ctor_t { };		// // //

public:
	CFamiTrackerDoc(ctor_t) : CFamiTrackerDoc() { }

protected: // create from serialization only
	CFamiTrackerDoc();
	DECLARE_DYNCREATE(CFamiTrackerDoc)

	virtual ~CFamiTrackerDoc();

	// Static functions
public:
	static CFamiTrackerDoc *GetDoc();


	// Other
#ifdef AUTOSAVE
	void AutoSave();
#endif

	//
	// Public functions
	//
public:
	// // // implementation
	CFamiTrackerModule *GetModule() noexcept;
	const CFamiTrackerModule *GetModule() const noexcept;

	CString GetFileTitle() const;

	//
	// Document file I/O
	//
	bool IsFileLoaded() const;
	bool HasLastLoadFailed() const;
	void CreateEmpty();		// // //

	// Import
	static std::unique_ptr<CFamiTrackerDoc> LoadImportFile(LPCTSTR lpszPathName);		// // //
	bool			ImportInstruments(CFamiTrackerModule &Imported, int *pInstTable);
	bool			ImportGrooves(CFamiTrackerModule &Imported, int *pGrooveMap);		// // //
	bool			ImportDetune(CFamiTrackerModule &Imported);			// // //

	// Synchronization
	BOOL			LockDocument() const;
	BOOL			LockDocument(DWORD dwTimeout) const;
	BOOL			UnlockDocument() const;

	//
	// Document data access functions
	//

	// Global (module) data
	void			SelectExpansionChip(const CSoundChipSet &chips, unsigned n163chs);		// // //

	stHighlight		GetHighlightAt(unsigned int Track, unsigned int Frame, unsigned int Row) const;		// // //
	unsigned int	GetHighlightState(unsigned int Track, unsigned int Frame, unsigned int Row) const;		// // //

	CBookmarkCollection *GetBookmarkCollection(unsigned track);		// // //
	const CBookmarkCollection *GetBookmarkCollection(unsigned track) const;		// // //
	unsigned		GetTotalBookmarkCount() const;		// // //
	CBookmark		*GetBookmarkAt(unsigned int Track, unsigned int Frame, unsigned int Row) const;		// // //

	int				GetFrameLength(unsigned int Track, unsigned int Frame) const;

	// Instruments functions
	void			SaveInstrument(unsigned int Index, CSimpleFile &file) const;		// // //
	bool 			LoadInstrument(unsigned Index, CSimpleFile &File);		// // //

	// Operations
	void			RemoveUnusedInstruments();
	void			RemoveUnusedSamples();		// // //
	void			RemoveUnusedPatterns();

	bool			GetExceededFlag() { return m_bExceeded; };
	void			SetExceededFlag(bool Exceed = 1);		// // //

	// // // from the component interface
	CChannelOrder	&GetChannelOrder() const override;		// // //
	CSequenceManager *const GetSequenceManager(int InstType) const override;
	CInstrumentManager *const GetInstrumentManager() const override;
	CDSampleManager *const GetDSampleManager() const override;
	void			Modify(bool Change);
	void			ModifyIrreversible();

// // // delegates

#pragma region delegates to CSongData
	unsigned int	GetFrameCount(unsigned int Track) const;
#pragma endregion

#pragma region delegates to CFamiTrackerModule
	int				GetNamcoChannels() const;

	machine_t		GetMachine() const;

	// void (*F)(chan_id_t chan)
	template <typename F>
	void ForeachChannel(F f) const {
		for (std::size_t i = 0, n = GetChannelCount(); i < n; ++i)
			f(TranslateChannel(i));
	}

	const stHighlight &GetHighlight(unsigned int Track) const;
#pragma endregion

#pragma region delegates to CInstrumentManager
	std::shared_ptr<CInstrument>	GetInstrument(unsigned int Index) const;
	unsigned int	GetInstrumentCount() const;
	unsigned		GetFreeInstrumentIndex() const;		// // //
	bool			IsInstrumentUsed(unsigned int Index) const;
	inst_type_t		GetInstrumentType(unsigned int Index) const;

	// // // take instrument type as parameter rather than chip type
	int				GetFreeSequence(inst_type_t InstType, sequence_t Type) const;		// // //
#pragma endregion

	//
	// Private functions
	//
private:

	//
	// File management functions (load/save)
	//

	BOOL			SaveDocument(LPCTSTR lpszPathName) const;
	BOOL			OpenDocument(LPCTSTR lpszPathName);

	BOOL			OpenDocumentOld(CFile *pOpenFile);
	BOOL			OpenDocumentNew(CDocumentFile &DocumentFile);

#ifdef AUTOSAVE
	void			SetupAutoSave();
	void			ClearAutoSave();
#endif

	//
	// Internal module operations
	//

	int				GetChannelCount() const;
	chan_id_t		TranslateChannel(unsigned Index) const;		// // // TODO: move to CSongView

	void			ApplyExpansionChip() const;

	//
	// Private variables
	//
private:

	//
	// Interface variables
	//

	std::unique_ptr<CFamiTrackerModule> module_;		// // //

	//
	// State variables
	//

	bool			m_bFileLoaded = false;			// Is a file loaded?
	bool			m_bFileLoadFailed = false;		// Last file load operation failed
	unsigned int	m_iFileVersion;					// Loaded file version

	bool			m_bForceBackup;
	bool			m_bBackupDone;
	bool			m_bExceeded;			// // //

#ifdef AUTOSAVE
	// Auto save
	int				m_iAutoSaveCounter;
	CString			m_sAutoSaveFile;
#endif

	// Thread synchronization
private:
	mutable CMutex			 m_csDocumentLock;

// Operations
public:

// Overrides
public:
	BOOL OnNewDocument() override;
	BOOL OnSaveDocument(LPCTSTR lpszPathName) override;
	BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
	void OnCloseDocument() override;
	void DeleteContents() override;
	void SetModifiedFlag(BOOL bModified = 1) override;

// Implementation
public:
#ifdef _DEBUG
	void AssertValid() const override;
	void Dump(CDumpContext& dc) const override;
#endif

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileSave();
};
