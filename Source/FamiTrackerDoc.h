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

#include <vector>
#include <memory>		// // //
#include <array>		// // //

// Get access to some APU constants
#include "APU/Types.h"
// Constants, types and enums
#include "FamiTrackerTypes.h"
// // //
#include "FTMComponentInterface.h"
#include "Settings.h"		// // //

#define TRANSPOSE_FDS

// #define DISABLE_SAVE		// // //

// View update modes (TODO check these and remove inappropriate flags)
enum {
	UPDATE_NONE = 0,		// No update
	UPDATE_TRACK = 1,		// Track has been added, removed or changed
	UPDATE_PATTERN,			// Pattern data has been edited
	UPDATE_FRAME,			// Frame data has been edited
	UPDATE_INSTRUMENT,		// Instrument has been added / removed
	UPDATE_PROPERTIES,		// Module properties has changed (including channel count)
	UPDATE_HIGHLIGHT,		// Row highlight option has changed
	UPDATE_COLUMNS,			// Effect columns has changed
	UPDATE_CLOSE			// Document is closing (TODO remove)
};

// Access data types used by the document class
#include "Instrument.h"
#include "Sequence.h"
#include "OldSequence.h"		// // //
#include "Bookmark.h"		// // //

#include "PatternEditorTypes.h"		// // //
// #include "FrameEditorTypes.h"		// // //

// External classes
class CSongData;		// // //
class CChannelMap;		// // //
class CTrackerChannel;
class CDocumentFile;
class CSeqInstrument;		// // // TODO: move to instrument manager
class CSimpleFile;		// // //
class CBookmarkCollection;		// // //

namespace ft0cc::doc {		// // //
class groove;
class dpcm_sample;
} // namespace ft0cc::doc

//
// I'll try to organize this class, things are quite messy right now!
//

class CFamiTrackerDoc : public CDocument, public CFTMComponentInterface
{
	using groove = ft0cc::doc::groove;
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
	static CFamiTrackerDoc* GetDoc();


	// Other
#ifdef AUTOSAVE
	void AutoSave();
#endif

	//
	// Public functions
	//
public:

	CString GetFileTitle() const;

	//
	// Document file I/O
	//
	bool IsFileLoaded() const;
	bool HasLastLoadFailed() const;
	void CreateEmpty();		// // //

	// Import
	static std::unique_ptr<CFamiTrackerDoc> LoadImportFile(LPCTSTR lpszPathName);		// // //
	bool ImportInstruments(CFamiTrackerDoc &Imported, int *pInstTable);
	bool ImportGrooves(CFamiTrackerDoc &Imported, int *pGrooveMap);		// // //
	bool ImportDetune(CFamiTrackerDoc &Imported);			// // //

	//
	// Interface functions (not related to document data) TODO move this?
	//
	CTrackerChannel	&GetChannel(int Index) const;		// // //
	int				GetChannelIndex(int Channel) const;

	int				GetChannelType(int Channel) const;
	int				GetChipType(int Channel) const;
	int				GetChannelCount() const;

	// Synchronization
	BOOL			LockDocument() const;
	BOOL			LockDocument(DWORD dwTimeout) const;
	BOOL			UnlockDocument() const;

	//
	// Document data access functions
	//

	// Local (song) data
	CSongData		*GetSong(unsigned int Index);		// // //
	const CSongData	*GetSong(unsigned int Index) const;		// // //
	std::unique_ptr<CSongData> ReplaceSong(unsigned Index, std::unique_ptr<CSongData> pSong);		// // // returns old song

	void			SetPatternLength(unsigned int Track, unsigned int Length);
	void			SetFrameCount(unsigned int Track, unsigned int Count);
	void			SetSongSpeed(unsigned int Track, unsigned int Speed);
	void			SetSongTempo(unsigned int Track, unsigned int Tempo);
	void			SetSongGroove(unsigned int Track, bool Groove);		// // //

	unsigned int	GetPatternLength(unsigned int Track) const;
	unsigned int	GetFrameCount(unsigned int Track) const;
	unsigned int	GetSongSpeed(unsigned int Track) const;
	unsigned int	GetSongTempo(unsigned int Track) const;
	bool			GetSongGroove(unsigned int Track) const;		// // //

	unsigned int	GetCurrentPatternLength(unsigned int Track, int Frame) const;		// // // moved from pattern editor

	unsigned int	GetEffColumns(unsigned int Track, unsigned int Channel) const;
	void			SetEffColumns(unsigned int Track, unsigned int Channel, unsigned int Columns);

	unsigned int 	GetPatternAtFrame(unsigned int Track, unsigned int Frame, unsigned int Channel) const;
	void			SetPatternAtFrame(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Pattern);

	bool			IsPatternEmpty(unsigned int Track, unsigned int Channel, unsigned int Pattern) const;
	bool			ArePatternsSame(unsigned int Track, unsigned int Channel, unsigned int Pattern1, unsigned int Pattern2) const;		// // //

	// Pattern editing
	void			SetNoteData(unsigned Track, unsigned Frame, unsigned Channel, unsigned Row, const stChanNote &Data);		// // //
	const stChanNote &GetNoteData(unsigned Track, unsigned Frame, unsigned Channel, unsigned Row) const;		// // //
	stChanNote		GetActiveNote(unsigned Track, unsigned Frame, unsigned Channel, unsigned Row) const;		// // // remove hidden fx commands

	void			SetDataAtPattern(unsigned Track, unsigned Pattern, unsigned Channel, unsigned Row, const stChanNote &pData);		// // //
	const stChanNote &GetDataAtPattern(unsigned Track, unsigned Pattern, unsigned Channel, unsigned Row) const;		// // //

	void			ClearPattern(unsigned int Track, unsigned int Frame, unsigned int Channel);

	bool			InsertRow(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row);
	bool			ClearRowField(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row, cursor_column_t Column);
	bool			PullUp(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row);
	void			CopyPattern(unsigned int Track, int Target, int Source, int Channel);

	void			SwapChannels(unsigned int Track, unsigned int First, unsigned int Second);		// // //

	// Frame editing
	bool			InsertFrame(unsigned int Track, unsigned int Frame);
	bool			RemoveFrame(unsigned int Track, unsigned int Frame);
	bool			DuplicateFrame(unsigned int Track, unsigned int Frame);
	bool			CloneFrame(unsigned int Track, unsigned int Frame);		// // // renamed
	bool			MoveFrameDown(unsigned int Track, unsigned int Frame);
	bool			MoveFrameUp(unsigned int Track, unsigned int Frame);
	bool			AddFrames(unsigned int Track, unsigned int Frame, unsigned Count);		// // //
	bool			DeleteFrames(unsigned int Track, unsigned int Frame, unsigned Count);		// // //

	// Global (module) data
	void			SetEngineSpeed(unsigned int Speed);
	void			SetMachine(machine_t Machine);		// // //
	machine_t		GetMachine() const		{ return m_iMachine; }		// // //
	unsigned int	GetEngineSpeed() const	{ return m_iEngineSpeed; }
	unsigned int	GetFrameRate() const;

	void			SelectExpansionChip(unsigned chips, unsigned n163chs, bool Move);		// // //
	unsigned char	GetExpansionChip() const;
	bool			ExpansionEnabled(int Chip) const;
	int				GetNamcoChannels() const;

	// Todo: remove this, use getchannelcount instead
	unsigned int	GetAvailableChannels()	const { return m_iChannelsAvailable; }
	void			SetAvailableChannels(unsigned channels) { m_iChannelsAvailable = channels; }		// // //

	std::string_view GetModuleName() const;		// // //
	std::string_view GetModuleArtist() const;
	std::string_view GetModuleCopyright() const;
	void			SetModuleName(std::string_view pName);
	void			SetModuleArtist(std::string_view pArtist);
	void			SetModuleCopyright(std::string_view pCopyright);

	vibrato_t		GetVibratoStyle() const;
	void			SetVibratoStyle(vibrato_t Style);

	bool			GetLinearPitch() const;
	void			SetLinearPitch(bool Enable);

	void			SetComment(const std::string &comment, bool bShowOnLoad);		// // //
	const std::string &GetComment() const;		// // //
	bool			ShowCommentOnOpen() const;

	void			SetSpeedSplitPoint(int SplitPoint);
	int				GetSpeedSplitPoint() const;

	void			SetHighlight(unsigned int Track, const stHighlight &Hl);		// // //
	const stHighlight &GetHighlight(unsigned int Track) const;

	void			SetHighlight(const stHighlight &Hl);		// // //
	const stHighlight &GetHighlight() const;
	stHighlight		GetHighlightAt(unsigned int Track, unsigned int Frame, unsigned int Row) const;		// // //
	unsigned int	GetHighlightState(unsigned int Track, unsigned int Frame, unsigned int Row) const;		// // //

	CBookmarkCollection *GetBookmarkCollection(unsigned track);		// // //
	const CBookmarkCollection *GetBookmarkCollection(unsigned track) const;		// // //
	unsigned		GetTotalBookmarkCount() const;		// // //
	CBookmark		*GetBookmarkAt(unsigned int Track, unsigned int Frame, unsigned int Row) const;		// // //

	void			SetDetuneOffset(int Chip, int Note, int Detune);		// // //
	int				GetDetuneOffset(int Chip, int Note) const;
	void			ResetDetuneTables();
	void			SetTuning(int Semitone, int Cent);		// // // 050B
	int				GetTuningSemitone() const;		// // // 050B
	int				GetTuningCent() const;		// // // 050B

	groove			*GetGroove(unsigned Index) const;		// // //
	void			SetGroove(unsigned Index, std::unique_ptr<groove> Groove);

	int				GetFrameLength(unsigned int Track, unsigned int Frame) const;

	// Track management functions
	int				AddTrack();
	int				AddTrack(std::unique_ptr<CSongData> song);		// // //
	void			RemoveTrack(unsigned int Track);
	std::unique_ptr<CSongData> ReleaseTrack(unsigned int Track);		// // //
	unsigned int	GetTrackCount() const;
	const std::string &GetTrackTitle(unsigned int Track) const;		// // //
	void			SetTrackTitle(unsigned int Track, const std::string &title);		// // //
	void			MoveTrackUp(unsigned int Track);
	void			MoveTrackDown(unsigned int Track);

	// Instruments functions
	std::shared_ptr<CInstrument>	GetInstrument(unsigned int Index) const;
	unsigned int	GetInstrumentCount() const;
	unsigned		GetFreeInstrumentIndex() const;		// // //
	bool			IsInstrumentUsed(unsigned int Index) const;
	bool			AddInstrument(std::unique_ptr<CInstrument> pInstrument, unsigned int Slot);		// // //
	bool			RemoveInstrument(unsigned int Index);							// // // Remove an instrument
	inst_type_t		GetInstrumentType(unsigned int Index) const;
	void			SaveInstrument(unsigned int Index, CSimpleFile &file) const;		// // //
	bool 			LoadInstrument(unsigned Index, CSimpleFile &File);		// // //

	// Sequences functions
	// // // take instrument type as parameter rather than chip type
	std::shared_ptr<CSequence> GetSequence(inst_type_t InstType, unsigned int Index, int Type) const;		// // //
	unsigned int	GetSequenceItemCount(inst_type_t InstType, unsigned int Index, int Type) const;		// // //
	int				GetFreeSequence(inst_type_t InstType, int Type) const;		// // //
	int				GetSequenceCount(inst_type_t InstType, int Type) const;		// // //
	int				GetTotalSequenceCount(inst_type_t InstType) const;		// // //

	// DPCM samples
	std::shared_ptr<dpcm_sample> GetSample(unsigned int Index);		// // //
	std::shared_ptr<const dpcm_sample> GetSample(unsigned int Index) const;		// // //
	void			SetSample(unsigned int Index, std::shared_ptr<dpcm_sample> pSamp);		// // //
	bool			IsSampleUsed(unsigned int Index) const;
	unsigned int	GetSampleCount() const;
	int				GetFreeSampleSlot() const;
	void			RemoveSample(unsigned int Index);
	unsigned int	GetTotalSampleSize() const;

	// Other
	unsigned int	ScanActualLength(unsigned int Track, unsigned int Count) const;		// // //
	double			GetStandardLength(int Track, unsigned int ExtraLoops) const;		// // //
	unsigned int	GetFirstFreePattern(unsigned int Track, unsigned int Channel) const;		// // //

	// Operations
	void			RemoveUnusedInstruments();
	void			RemoveUnusedSamples();		// // //
	void			RemoveUnusedPatterns();
	void			SwapInstruments(int First, int Second);

	bool			GetExceededFlag() { return m_bExceeded; };
	void			SetExceededFlag(bool Exceed = 1);		// // //

	// // // from the component interface
	CChannelMap *const GetChannelMap() const override;
	CSequenceManager *const GetSequenceManager(int InstType) const override;
	CInstrumentManager *const GetInstrumentManager() const override;
	CDSampleManager *const GetDSampleManager() const override;
	void			Modify(bool Change);
	void			ModifyIrreversible();

	// void (*F)(CSongData &song [, unsigned index])
	template <typename F>
	void VisitSongs(F f) {
		if constexpr (std::is_invocable_v<F, CSongData &>)
			for (auto &song : m_pTracks)
				f(*song);
		else {
			unsigned index = 0;
			for (auto &song : m_pTracks)
				f(*song, index++);
		}
	}
	// void (*F)(const CSongData &song [, unsigned index])
	template <typename F>
	void VisitSongs(F f) const {
		if constexpr (std::is_invocable_v<F, const CSongData &>)
			for (auto &song : m_pTracks)
				f(*song);
		else {
			unsigned index = 0;
			for (auto &song : m_pTracks)
				f(*song, index++);
		}
	}

	// Constants
public:
	// Default song settings
	static const vibrato_t	 DEFAULT_VIBRATO_STYLE		= vibrato_t::VIBRATO_NEW;		// // //
	static const bool		 DEFAULT_LINEAR_PITCH		= false;
	static const machine_t	 DEFAULT_MACHINE_TYPE		= machine_t::NTSC;
	static const unsigned	 DEFAULT_SPEED_SPLIT_POINT	= 32;
	static const unsigned	 OLD_SPEED_SPLIT_POINT		= 21;

	static const std::size_t METADATA_FIELD_LENGTH		= 32;		// // //

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

	void			AllocateSong(unsigned int Index);		// // //
	void			SwapSongs(unsigned int First, unsigned int Second);		// // //

	CSongData		&GetSongData(unsigned int Index);		// // //
	const CSongData	&GetSongData(unsigned int Index) const;		// // //

	void			ApplyExpansionChip() const;

	//
	// Private variables
	//
private:

	//
	// Interface variables
	//

	// Channels (TODO: run-time state, remove or move these?)
	std::unique_ptr<CChannelMap> m_pChannelMap;		// // //


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

	//
	// Document data
	//

	// Patterns and song data
	std::vector<std::unique_ptr<CSongData>> m_pTracks;		// // // List of all tracks

	unsigned int	m_iChannelsAvailable;						// Number of channels added

	// Instruments, samples and sequences
	std::unique_ptr<CInstrumentManager> m_pInstrumentManager;	// // //
	std::array<std::unique_ptr<groove>, MAX_GROOVE> m_pGrooveTable;		// // // Grooves

	// Module properties
	vibrato_t		m_iVibratoStyle;							// 0 = old style, 1 = new style
	bool			m_bLinearPitch;
	machine_t		m_iMachine;									// // // NTSC / PAL
	unsigned int	m_iEngineSpeed;								// Refresh rate
	unsigned int	m_iSpeedSplitPoint;							// Speed/tempo split-point
	int				m_iDetuneTable[6][96] = { };				// // // Detune tables
	int				m_iDetuneSemitone, m_iDetuneCent;			// // // 050B tuning

	// NSF info
	std::string		m_strName;									// Song name
	std::string		m_strArtist;								// Song artist
	std::string		m_strCopyright;								// Song copyright

	// Comments
	std::string		m_strComment;								// // //
	bool			m_bDisplayComment = false;

	// Row highlight (TODO remove)
	stHighlight		m_vHighlight;								// // //

	//
	// End of document data
	//

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
