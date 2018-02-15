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
#include <vector>		// // //
#include <array>		// // //
#include <memory>
#include <string>		// // //
#include <map>		// // //
#include <cstdint>		// // //
#include "FamiTrackerTypes.h"
#include "SoundChipSet.h"		// // //
#include "ChannelOrder.h"		// // //
#include "Sequence.h"		// // // TODO: remove

// NSF file header
struct stNSFHeader {
	uint8_t		Ident[5] = {'N', 'E', 'S', 'M', '\x1A'};		// // //
	uint8_t		Version = 1;
	uint8_t		TotalSongs;
	uint8_t		StartSong = 1;
	uint16_t	LoadAddr;
	uint16_t	InitAddr;
	uint16_t	PlayAddr;
	uint8_t		SongName[32] = { };
	uint8_t		ArtistName[32] = { };
	uint8_t		Copyright[32] = { };
	uint16_t	Speed_NTSC;
	uint8_t		BankValues[8] = { };
	uint16_t	Speed_PAL;
	uint8_t		Flags = 0; // NTSC
	uint8_t		SoundChip;
	uint8_t		Reserved[4] = { };
};

struct stNSFeHeader {		// // //
	uint8_t		NSFeIdent[4] = {'N', 'S', 'F', 'E'};
	uint32_t	InfoSize = 12;
	uint8_t		InfoIdent[4] = {'I', 'N', 'F', 'O'};
	uint16_t	LoadAddr;
	uint16_t	InitAddr;
	uint16_t	PlayAddr;
	uint8_t		Flags = 0; // NTSC
	uint8_t		SoundChip;
	uint8_t		TotalSongs;
	uint8_t		StartSong = 0;
	uint16_t	Speed_NTSC;
	uint32_t	BankSize = 8;
	uint8_t		BankIdent[4] = {'B', 'A', 'N', 'K'};
	uint8_t		BankValues[8] = { };
};

struct driver_t;
class CChunk;
enum chunk_type_t : int;
struct stChunkLabel;		// // //
namespace ft0cc::doc {
class dpcm_sample;
} // namespace ft0cc::doc
class CFamiTrackerModule;		// // //
class CSequence;		// // //
class CInstrumentFDS;		// // //
class CConstSongView;		// // //

/*
 * Logger class
 */
class CCompilerLog
{
public:
	virtual ~CCompilerLog() noexcept = default;
	virtual void WriteLog(LPCWSTR text) = 0;
	virtual void Clear() = 0;
};

/*
 * The compiler
 */
class CCompiler
{
public:
	CCompiler(const CFamiTrackerModule &modfile, std::shared_ptr<CCompilerLog> pLogger);		// // //
	~CCompiler();

	void	ExportNSF(LPCWSTR lpszFileName, int MachineType);
	void	ExportNSFE(LPCWSTR lpszFileName, int MachineType);		// // //
	void	ExportNES(LPCWSTR lpszFileName, bool EnablePAL);
	void	ExportBIN(LPCWSTR lpszBIN_File, LPCWSTR lpszDPCM_File);
	void	ExportPRG(LPCWSTR lpszFileName, bool EnablePAL);
	void	ExportASM(LPCWSTR lpszFileName);

	void	SetMetadata(std::string_view title, std::string_view artist, std::string_view copyright);		// // //

private:
	void	ExportNSF_NSFE(LPCWSTR lpszFileName, int MachineType, bool isNSFE);		// // //
	void	ExportNES_PRG(LPCWSTR lpszFileName, bool EnablePAL, bool isPRG);		// // //
	void	ExportBIN_ASM(LPCWSTR lpszFileName, LPCWSTR lpszDPCM_File, bool isASM);		// // //

	bool	OpenFile(LPCWSTR lpszFileName, CFile &file) const;

	stNSFHeader CreateHeader(int MachineType) const;		// // //
	stNSFeHeader CreateNSFeHeader(int MachineType);		// // //
	void	SetDriverSongAddress(unsigned char *pDriver, unsigned short Address) const;		// // //
#if 0
	void	WriteChannelMap();
	void	WriteChannelTypes();
#endif

	std::unique_ptr<unsigned char[]> LoadDriver(const driver_t &Driver, unsigned short Origin) const;		// // //

	// Compiler
	bool	CompileData();
	void	ResolveLabels();
	bool	ResolveLabelsBankswitched();
	void	CollectLabels(std::map<stChunkLabel, int> &labelMap) const;		// // //
	bool	CollectLabelsBankswitched(std::map<stChunkLabel, int> &labelMap);
	void	AssignLabels(std::map<stChunkLabel, int> &labelMap);
	void	AddBankswitching();

	void	ScanSong();
	int		GetSampleIndex(int SampleNumber);
	bool	IsPatternAddressed(unsigned int Track, int Pattern, chan_id_t Channel) const;

	void	CreateMainHeader();
	void	CreateSequenceList();
	void	CreateInstrumentList();
	void	CreateSampleList();
	void	CreateFrameList(unsigned int Track);

	int		StoreSequence(const CSequence &Seq, const stChunkLabel &label);		// // //
	void	StoreSamples();
	void	StoreGrooves();		// // //
	void	StoreSongs();
	void	StorePatterns(unsigned int Track);

	// Bankswitching functions
	void	UpdateSamplePointers(unsigned int Origin);
	void	UpdateFrameBanks();
	void	UpdateSongBanks();
	void	ClearSongBanks();
	void	EnableBankswitching();

	// FDS
	void	AddWavetable(CInstrumentFDS *pInstrument, CChunk *pChunk);

	// Object list functions
	CChunk	&CreateChunk(const stChunkLabel &Label);		// // //
	CChunk	&AddChunkToList(CChunk &Chunk, const stChunkLabel &Label);		// // //
	CChunk	*GetObjectByLabel(const stChunkLabel &Label) const;		// // //
	int		CountData() const;

	// Debugging
	template <typename... Args>
	void	Print(LPCWSTR text, Args&&... args) const;		// // //
	void	ClearLog() const;

public:
	static const int PATTERN_CHUNK_INDEX;

	static const int PAGE_SIZE;
	static const int PAGE_START;
	static const int PAGE_BANKED;
	static const int PAGE_SAMPLES;

	static const int PATTERN_SWITCH_BANK;

	static const int DPCM_PAGE_WINDOW;
	static const int DPCM_SWITCH_ADDRESS;

	static const bool LAST_BANK_FIXED;

	// Flags
	static const int FLAG_BANKSWITCHED;
	static const int FLAG_VIBRATO;
	static const int FLAG_LINEARPITCH;		// // //

public:
	static unsigned int AdjustSampleAddress(unsigned int Address);

private:
	const CFamiTrackerModule *m_pModule;		// // //
	CChannelOrder m_ChannelOrder;		// // //

	std::string		title_, artist_, copyright_;		// // //

	// Object lists
	std::vector<std::shared_ptr<CChunk>> m_vChunks;		// // //
	std::vector<CChunk*> m_vSongChunks;
	std::vector<CChunk*> m_vFrameChunks;
	//std::vector<CChunk*> m_vWaveChunks;

	// Special objects
	CChunk			*m_pSamplePointersChunk = nullptr;
	CChunk			*m_pHeaderChunk = nullptr;

	// Samples
	std::vector<std::shared_ptr<const ft0cc::doc::dpcm_sample>> m_vSamples;		// // //

	// Flags
	bool			m_bBankSwitched = false;

	// Driver
	const driver_t	*m_pDriverData = nullptr;
	unsigned int	m_iVibratoTableLocation;

	// Sequences and instruments
	unsigned int	m_iInstruments;
	std::array<unsigned, MAX_INSTRUMENTS> m_iAssignedInstruments = { };		// // //
	std::array<std::array<bool, SEQ_COUNT>, MAX_SEQUENCES> m_bSequencesUsed2A03 = { };
	std::array<std::array<bool, SEQ_COUNT>, MAX_SEQUENCES> m_bSequencesUsedVRC6 = { };
	std::array<std::array<bool, SEQ_COUNT>, MAX_SEQUENCES> m_bSequencesUsedN163 = { };
	std::array<std::array<bool, SEQ_COUNT>, MAX_SEQUENCES> m_bSequencesUsedS5B  = { };		// // //

	std::array<unsigned, MAX_INSTRUMENTS> m_iWaveBanks = { };		// // // N163 waves

	// Sample variables
	std::array<std::array<std::array<unsigned char, NOTE_RANGE>, OCTAVE_RANGE>, MAX_INSTRUMENTS> m_iSamplesLookUp = { };
	std::array<std::array<std::array<bool, NOTE_RANGE>, OCTAVE_RANGE>, MAX_INSTRUMENTS> m_bSamplesAccessed = { };
	std::array<unsigned char, MAX_DSAMPLES> m_iSampleBank = { };
	unsigned int	m_iSampleStart;
	unsigned int	m_iSamplesUsed;

	// General
	unsigned int	m_iMusicDataSize;		// All music data
	unsigned int	m_iDriverSize;			// Size of selected music driver
	unsigned int	m_iSamplesSize;

	unsigned int	m_iLoadAddress;			// NSF load address
	unsigned int	m_iInitAddress;			// NSF init address
	unsigned int	m_iDriverAddress;		// Music driver location

	unsigned int	m_iTrackFrameSize[MAX_TRACKS];	// Cached song frame sizes

	unsigned int	m_iHeaderFlagOffset;	// Offset to flag location in main header
	unsigned int	m_iSongBankReference;	// Offset to bank value in song header

	unsigned int	m_iDuplicatePatterns;	// Number of duplicated patterns removed

	// NSF banks
	unsigned int	m_iFirstSampleBank;		// Bank number with the first DPCM sample
	unsigned int	m_iLastBank = 0;		// Last bank in the NSF file

	unsigned int	m_iSamplePointerBank;
	unsigned int	m_iSamplePointerOffset;

	// FDS
	unsigned int	m_iWaveTables = 0;

	// Optimization
	CMap<UINT, UINT, CChunk*, CChunk*> m_PatternMap;
	std::map<stChunkLabel, stChunkLabel> m_DuplicateMap;		// // //

	// Debugging
	std::shared_ptr<CCompilerLog> m_pLogger;		// // //

	// Diagnostics
	unsigned int	m_iHashCollisions = 0u;
};
