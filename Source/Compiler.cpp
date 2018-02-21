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

#include "Compiler.h"
#include "version.h"		// // //
#include "../resource.h"		// // //
#include "FamiTrackerEnv.h"		// // //
#include "FamiTrackerModule.h"		// // //
#include "SongData.h"		// // //
#include "SongView.h"		// // //
#include "PatternNote.h"		// // //
#include "SeqInstrument.h"		// // //
#include "Instrument2A03.h"		// // //
#include "InstrumentFDS.h"		// // //
#include "InstrumentN163.h"		// // //
#include "PatternCompiler.h"
#include "ft0cc/doc/dpcm_sample.hpp"		// // //
#include "ft0cc/doc/groove.hpp"		// // //
#include "Chunk.h"
#include "ChunkRenderText.h"
#include "ChunkRenderBinary.h"
#include "Driver.h"
#include "SoundGen.h"
#include "APU/APU.h"
#include "DSampleManager.h"		// // //
#include "InstrumentManager.h"		// // //
#include "InstrumentService.h"		// // //
#include "InstCompiler.h"		// // //
#include "SongLengthScanner.h"		// // //
#include "str_conv/str_conv.hpp"		// // //

//
// This is the new NSF data compiler, music is compiled to an object list instead of a binary chunk
//
// The list can be translated to both a binary chunk and an assembly file
//

/*
 * TODO:
 *  - Remove duplicated FDS waves
 *  - Remove the bank value in CHUNK_SONG??
 *  - Derive classes for each output format instead of separate functions
 *  - Create a config file for NSF driver optimizations
 *  - Pattern hash collisions prevents detecting similar patterns, fix that
 *  - Add bankswitching schemes for other memory mappers
 *
 */

/*
 * Notes:
 *
 *  - DPCM samples and instruments is currently stored as a linear list,
 *    which currently limits the number of possible DPCM configurations
 *    to 127.
 *  - Instrument data is non bankswitched, it might be possible to create
 *    instrument data of a size that makes export impossible.
 *
 */

/*
 * Bankswitched file layout:
 *
 * - $8000 - $AFFF: Music driver and song data (instruments, frames & patterns, unpaged)
 * - $B000 - $BFFF: Swichted part of song data (frames + patterns, 1 page only)
 * - $C000 - $EFFF: Samples (3 pages)
 * - $F000 - $FFFF: Fixed to last bank for compatibility with TNS HFC carts
 *
 * Non-bankswitched, compressed layout:
 *
 * - Music data, driver, DPCM samples
 *
 * Non-bankswitched + bankswitched, default layout:
 *
 * - Driver, music data, DPCM samples
 *
 */

// Note: Each CCompiler object may only be used once (fix this)

// Remove duplicated patterns (default on)
#define REMOVE_DUPLICATE_PATTERNS

// Don't remove patterns across different tracks (default off)
//#define LOCAL_DUPLICATE_PATTERN_REMOVAL

// Enable bankswitching on all songs (default off)
//#define FORCE_BANKSWITCH

// // //
#define DATA_HEADER_SIZE 8

const int CCompiler::PATTERN_CHUNK_INDEX		= 0;		// Fixed at 0 for the moment

const int CCompiler::PAGE_SIZE					= 0x1000;
const int CCompiler::PAGE_START					= 0x8000;
const int CCompiler::PAGE_BANKED				= 0xB000;	// 0xB000 -> 0xBFFF
const int CCompiler::PAGE_SAMPLES				= 0xC000;

const int CCompiler::PATTERN_SWITCH_BANK		= 3;		// 0xB000 -> 0xBFFF

const int CCompiler::DPCM_PAGE_WINDOW			= 3;		// Number of switchable pages in the DPCM area
const int CCompiler::DPCM_SWITCH_ADDRESS		= 0xF000;	// Switch to new banks when reaching this address

const bool CCompiler::LAST_BANK_FIXED			= true;		// Fix for TNS carts

// Flag byte flags
const int CCompiler::FLAG_BANKSWITCHED	= 1 << 0;
const int CCompiler::FLAG_VIBRATO		= 1 << 1;
const int CCompiler::FLAG_LINEARPITCH	= 1 << 2;		// // //

unsigned int CCompiler::AdjustSampleAddress(unsigned int Address)
{
	// Align samples to 64-byte pages
	return (0x40 - (Address & 0x3F)) & 0x3F;
}

// CCompiler

CCompiler::CCompiler(const CFamiTrackerModule &modfile, std::shared_ptr<CCompilerLog> pLogger) :
	m_pModule(&modfile),
	m_ChannelOrder(m_pModule->GetChannelOrder().Canonicalize()),		// // //
	title_(m_pModule->GetModuleName()),
	artist_(m_pModule->GetModuleArtist()),
	copyright_(m_pModule->GetModuleCopyright()),
	m_pLogger(std::move(pLogger))
{
}

CCompiler::~CCompiler() {
}

template <typename... Args>
void CCompiler::Print(LPCWSTR text, Args&&... args) const		// // //
{
 	static WCHAR buf[256];

	if (!m_pLogger || !text)
		return;

	_sntprintf_s(buf, sizeof(buf), _TRUNCATE, text, std::forward<Args>(args)...);

	size_t len = wcslen(buf);

	if (buf[len - 1] == '\n' && len < (sizeof(buf) - 1)) {
		buf[len - 1] = '\r';
		buf[len] = '\n';
		buf[len + 1] = 0;
	}

	m_pLogger->WriteLog(buf);
}

void CCompiler::ClearLog() const
{
	if (m_pLogger != NULL)
		m_pLogger->Clear();
}

bool CCompiler::OpenFile(LPCWSTR lpszFileName, CFile &file) const
{
	CFileException ex;

	if (!file.Open(lpszFileName, CFile::modeWrite | CFile::modeCreate, &ex)) {
		// Display formatted file exception message
		WCHAR szCause[255];
		CStringW strFormatted;
		ex.GetErrorMessage(szCause, 255);
		AfxFormatString1(strFormatted, IDS_OPEN_FILE_ERROR, szCause);
		AfxMessageBox(strFormatted, MB_OK | MB_ICONERROR);
		return false;
	}

	return true;
}

static void NSFEWriteBlockIdent(CFile &file, const char (&ident)[5], uint32_t sz) {		// // //
	file.Write(reinterpret_cast<const char *>(&sz), sizeof(sz));
	file.Write(ident, 4);
}

static ULONGLONG NSFEWriteBlocks(CFile &file, const CFamiTrackerModule &modfile,
	std::string_view title, std::string_view artist, std::string_view copyright) {		// // //
	int iAuthSize = 0, iTimeSize = 0, iTlblSize = 0;
	CStringA str = "0CC-FamiTracker ";
	str.Append(Get0CCFTVersionString());		// // //
	iAuthSize = title.size() + artist.size() + copyright.size() + str.GetLength() + 4;

	NSFEWriteBlockIdent(file, "auth", iAuthSize);

	const unsigned char nullch = 0;
	file.Write(title.data(), title.size());
	file.Write(&nullch, 1);
	file.Write(artist.data(), artist.size());
	file.Write(&nullch, 1);
	file.Write(copyright.data(), copyright.size());
	file.Write(&nullch, 1);
	file.Write((LPCSTR)str, str.GetLength() + 1);

	modfile.VisitSongs([&] (const CSongData &song) {
		iTimeSize += 4;
		iTlblSize += song.GetTitle().size() + 1;
	});

	NSFEWriteBlockIdent(file, "time", iTimeSize);

	modfile.VisitSongs([&] (const CSongData &song, unsigned i) {
		auto pSongView = modfile.MakeSongView(i);
		CSongLengthScanner scanner {modfile, *pSongView};
		auto [FirstLoop, SecondLoop] = scanner.GetSecondsCount();
		int t = static_cast<int>((FirstLoop + SecondLoop) * 1000.0 + 0.5);
		file.Write(reinterpret_cast<const char *>(&t), sizeof(int));
	});

	NSFEWriteBlockIdent(file, "tlbl", iTlblSize);

	modfile.VisitSongs([&] (const CSongData &song) {
		file.Write(song.GetTitle().data(), song.GetTitle().size() + 1);
	});

	ULONGLONG iDataSizePos = file.GetPosition();
	NSFEWriteBlockIdent(file, "DATA", 0);
	return iDataSizePos;
}

void CCompiler::ExportNSF_NSFE(LPCWSTR lpszFileName, int MachineType, bool isNSFE) {
	if (m_bBankSwitched) {
		// Expand and allocate label addresses
		AddBankswitching();
		if (!ResolveLabelsBankswitched()) {
			return;
		}
		// Write bank data
		UpdateFrameBanks();
		UpdateSongBanks();
		// Make driver aware of bankswitching
		EnableBankswitching();
	}
	else {
		ResolveLabels();
		ClearSongBanks();
	}

	// Rewrite DPCM sample pointers
	UpdateSamplePointers(m_iSampleStart);

	m_iLoadAddress = PAGE_START;
	m_iDriverAddress = PAGE_START;
	unsigned short MusicDataAddress = m_iLoadAddress + m_iDriverSize;

	// Compressed mode means that driver and music is located just below the
	// sample space, no space is lost even when samples are used
	bool bCompressedMode = (PAGE_SAMPLES - m_iDriverSize - m_iMusicDataSize) >= 0x8000 &&
		!m_bBankSwitched && !m_pModule->GetSoundChipSet().IsMultiChip();
	if (bCompressedMode) {
		// Locate driver at $C000 - (driver size)
		m_iLoadAddress = PAGE_SAMPLES - m_iDriverSize - m_iMusicDataSize;
		m_iDriverAddress = PAGE_SAMPLES - m_iDriverSize;
		MusicDataAddress = m_iLoadAddress;
	}

	// Init is located first at the driver
	m_iInitAddress = m_iDriverAddress + DATA_HEADER_SIZE;		// // //

	// Load driver
	auto pDriverPtr = LoadDriver(*m_pDriverData, m_iDriverAddress);		// // //
	unsigned char *pDriver = pDriverPtr.get();

	// Write music data address
	SetDriverSongAddress(pDriver, MusicDataAddress);

	// Open output file
	CFile OutputFile;
	if (!OpenFile(lpszFileName, OutputFile)) {
		Print(L"Error: Could not open output file\n");
		return;
	}

	// Create NSF header
	ULONGLONG iDataSizePos = 0;		// // //
	if (isNSFE) {
		auto Header = CreateNSFeHeader(MachineType);		// // //
		OutputFile.Write(&Header, sizeof(Header));
		iDataSizePos = NSFEWriteBlocks(OutputFile, *m_pModule, title_, artist_, copyright_);
	}
	else {
		auto Header = CreateHeader(MachineType);		// // //
		OutputFile.Write(&Header, sizeof(Header));
	}

	// Write NSF data
	CChunkRenderNSF Render(&OutputFile, m_iLoadAddress);

	if (m_bBankSwitched) {
		Render.StoreDriver({pDriver, m_iDriverSize});
		Render.StoreChunksBankswitched(m_vChunks);
		Render.StoreSamplesBankswitched(m_vSamples);
	}
	else {
		if (bCompressedMode) {
			Render.StoreChunks(m_vChunks);
			Render.StoreDriver({pDriver, m_iDriverSize});
			Render.StoreSamples(m_vSamples);
		}
		else {
			Render.StoreDriver({pDriver, m_iDriverSize});
			Render.StoreChunks(m_vChunks);
			Render.StoreSamples(m_vSamples);
		}
	}

	if (isNSFE) {
		NSFEWriteBlockIdent(OutputFile, "NEND", 0);		// // //
		OutputFile.Seek(iDataSizePos, CFile::begin);
		NSFEWriteBlockIdent(OutputFile, "DATA", m_bBankSwitched ? 0x1000 * (Render.GetBankCount() - 1) :
			m_iDriverSize + m_iMusicDataSize + m_iSamplesSize);		// // //
	}

	// Writing done, print some stats
	Print(L" * NSF load address: $%04X\n", m_iLoadAddress);
	Print(L"Writing output file...\n");
	Print(L" * Driver size: %i bytes\n", m_iDriverSize);

	if (m_bBankSwitched) {
		int Percent = (100 * m_iMusicDataSize) / (0x80000 - m_iDriverSize - m_iSamplesSize);
		int Banks = Render.GetBankCount();
		Print(L" * Song data size: %i bytes (%i%%)\n", m_iMusicDataSize, Percent);
		Print(L" * NSF type: Bankswitched (%i banks)\n", Banks - 1);
	}
	else {
		int Percent = (100 * m_iMusicDataSize) / (0x8000 - m_iDriverSize - m_iSamplesSize);
		Print(L" * Song data size: %i bytes (%i%%)\n", m_iMusicDataSize, Percent);
		Print(L" * NSF type: Linear (driver @ $%04X)\n", m_iDriverAddress);
	}

	Print(L"Done, total file size: %i bytes\n", OutputFile.GetLength());

	OutputFile.Close();
}

void CCompiler::ExportNES_PRG(LPCWSTR lpszFileName, bool EnablePAL, bool isPRG) {
	if (m_bBankSwitched) {
		Print(L"Error: Can't write bankswitched songs!\n");
		return;
	}

	// Convert to binary
	ResolveLabels();
	ClearSongBanks();

	// Rewrite DPCM sample pointers
	UpdateSamplePointers(m_iSampleStart);

	// Locate driver at $8000
	m_iLoadAddress = PAGE_START;
	m_iDriverAddress = PAGE_START;
	unsigned short MusicDataAddress = m_iLoadAddress + m_iDriverSize;

	// Init is located first at the driver
	m_iInitAddress = m_iDriverAddress + DATA_HEADER_SIZE;		// // //

	// Load driver
	auto pDriverPtr = LoadDriver(*m_pDriverData, m_iDriverAddress);		// // //
	unsigned char *pDriver = pDriverPtr.get();

	// Write music data address
	SetDriverSongAddress(pDriver, MusicDataAddress);

	CFile OutputFile;
	if (!OpenFile(lpszFileName, OutputFile))
		return;

	Print(L"Writing output file...\n");

	// Write header
	static const char NES_HEADER[] = { // 32kb NROM, no CHR
		0x4E, 0x45, 0x53, 0x1A, 0x02, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	if (!isPRG)		// // //
		OutputFile.Write(NES_HEADER, std::size(NES_HEADER));

	// Write NES data
	CChunkRenderNES Render(&OutputFile, m_iLoadAddress);
	Render.StoreDriver({pDriver, m_iDriverSize});
	Render.StoreChunks(m_vChunks);
	Render.StoreSamples(m_vSamples);
	Render.StoreCaller(NSF_CALLER_BIN);

	int Percent = (100 * m_iMusicDataSize) / (0x8000 - m_iDriverSize - m_iSamplesSize);
	Print(L" * Driver size: %i bytes\n", m_iDriverSize);
	Print(L" * Song data size: %i bytes (%i%%)\n", m_iMusicDataSize, Percent);
	Print(L"Done, total file size: %i bytes\n", 0x8000 + (isPRG ? 0 : std::size(NES_HEADER)));

	// Done
	OutputFile.Close();
}

void CCompiler::ExportBIN_ASM(LPCWSTR lpszFileName, LPCWSTR lpszDPCM_File, bool isASM) {
	if (m_bBankSwitched) {
		Print(L"Error: Can't write bankswitched songs!\n");
		return;
	}

	bool exportDPCM = wcslen(lpszDPCM_File) > 0;		// // //

	// Convert to binary
	ResolveLabels();
	ClearSongBanks();
	if (isASM)
		UpdateSamplePointers(PAGE_SAMPLES);		// Always start at C000 when exporting to ASM

	CFile OutputFile;
	if (!OpenFile(lpszFileName, OutputFile))
		return;

	CFile OutputFileDPCM;
	if (exportDPCM) {
		if (!OpenFile(lpszDPCM_File, OutputFileDPCM)) {
			OutputFile.Close();
			return;
		}
	}

	Print(L"Writing output files...\n");

	if (isASM) {
		CChunkRenderText Render(OutputFile);		// // //
		Render.StoreChunks(m_vChunks);
		Render.StoreSamples(m_vSamples);
	}
	else {
		CChunkRenderBinary Render(&OutputFile);
		Render.StoreChunks(m_vChunks);

		if (exportDPCM) {
			CChunkRenderBinary RenderDPCM(&OutputFileDPCM);
			RenderDPCM.StoreSamples(m_vSamples);
		}
	}

	Print(L" * Music data size: %i bytes\n", m_iMusicDataSize);
	Print(L" * DPCM samples size: %i bytes\n", m_iSamplesSize);
	Print(L"Done\n");

	// Done
	OutputFile.Close();
	if (exportDPCM)
		OutputFileDPCM.Close();
}

void CCompiler::ExportNSF(LPCWSTR lpszFileName, int MachineType)
{
	ClearLog();
	if (!CompileData())
		return;
	ExportNSF_NSFE(lpszFileName, MachineType, false);		// // //
}

void CCompiler::ExportNSFE(LPCWSTR lpszFileName, int MachineType)		// // //
{
	ClearLog();
	if (!CompileData())
		return;
	ExportNSF_NSFE(lpszFileName, MachineType, true);
}

void CCompiler::ExportNES(LPCWSTR lpszFileName, bool EnablePAL)
{
	ClearLog();
	if (m_pModule->HasExpansionChips()) {
		Print(L"Error: Expansion chips not supported.\n");
		AfxMessageBox(L"Expansion chips are currently not supported!", 0, 0);
		return;
	}
	if (!CompileData())
		return;
	ExportNES_PRG(lpszFileName, EnablePAL, false);		// // //
}

void CCompiler::ExportPRG(LPCWSTR lpszFileName, bool EnablePAL)
{
	// Same as export to .NES but without the header

	ClearLog();
	if (m_pModule->HasExpansionChips()) {
		Print(L"Error: Expansion chips not supported.\n");
		AfxMessageBox(L"Expansion chips are currently not supported!", 0, 0);
		return;
	}
	if (!CompileData())
		return;
	ExportNES_PRG(lpszFileName, EnablePAL, true);		// // //
}

void CCompiler::ExportBIN(LPCWSTR lpszBIN_File, LPCWSTR lpszDPCM_File)
{
	ClearLog();
	if (!CompileData())
		return;
	ExportBIN_ASM(lpszBIN_File, lpszDPCM_File, false);		// // //
}

void CCompiler::ExportASM(LPCWSTR lpszFileName)
{
	ClearLog();
	if (!CompileData())
		return;
	ExportBIN_ASM(lpszFileName, L"", true);		// // //
}

void CCompiler::SetMetadata(std::string_view title, std::string_view artist, std::string_view copyright) {		// // //
	title_ = title.substr(0, CFamiTrackerModule::METADATA_FIELD_LENGTH - 1);
	artist_ = artist.substr(0, CFamiTrackerModule::METADATA_FIELD_LENGTH - 1);
	copyright_ = copyright.substr(0, CFamiTrackerModule::METADATA_FIELD_LENGTH - 1);
}

std::unique_ptr<unsigned char[]> CCompiler::LoadDriver(const driver_t &Driver, unsigned short Origin) const {		// // //
	// Copy embedded driver
	auto pData = std::make_unique<unsigned char[]>(Driver.driver.size());
	memcpy(pData.get(), Driver.driver.data(), Driver.driver.size());

	// // // Custom pitch tables
	const CSoundGen *pSoundGen = Env.GetSoundGenerator();		// // //
	for (size_t i = 0; i < Driver.freq_table.size(); i += 2) {		// // //
		int Table = Driver.freq_table[i + 1];
		switch (Table) {
		case CDetuneTable::DETUNE_NTSC:
		case CDetuneTable::DETUNE_PAL:
		case CDetuneTable::DETUNE_SAW:
		case CDetuneTable::DETUNE_FDS:
		case CDetuneTable::DETUNE_N163:
			for (int j = 0; j < NOTE_COUNT; ++j) {
				int Reg = pSoundGen->ReadPeriodTable(j, Table);
				pData[Driver.freq_table[i] + 2 * j    ] = Reg & 0xFF;
				pData[Driver.freq_table[i] + 2 * j + 1] = Reg >> 8;
			} break;
		case CDetuneTable::DETUNE_VRC7:
			for (int j = 0; j <= NOTE_RANGE; ++j) { // one extra item
				int Reg = pSoundGen->ReadPeriodTable(j % NOTE_RANGE, Table) * 4;
				if (j == NOTE_RANGE) Reg <<= 1;
				pData[Driver.freq_table[i] + j                 ] = Reg & 0xFF;
				pData[Driver.freq_table[i] + j + NOTE_RANGE + 1] = Reg >> 8;
			} break;
		default:
			AfxDebugBreak();
		}
	}

	// Relocate driver
	for (size_t i = 0; i < Driver.word_reloc.size(); ++i) {
		// Words
		unsigned short value = pData[Driver.word_reloc[i]] + (pData[Driver.word_reloc[i] + 1] << 8);
		value += Origin;
		pData[Driver.word_reloc[i]] = value & 0xFF;
		pData[Driver.word_reloc[i] + 1] = value >> 8;
	}

	for (size_t i = 0; i < Driver.adr_reloc.size(); i += 2) {		// // //
		unsigned short value = pData[Driver.adr_reloc[i]] + (pData[Driver.adr_reloc[i + 1]] << 8);
		value += Origin;
		pData[Driver.adr_reloc[i]] = value & 0xFF;
		pData[Driver.adr_reloc[i + 1]] = value >> 8;
	}

	if (m_pModule->HasExpansionChip(sound_chip_t::N163)) {
		pData[m_iDriverSize - 2 - 0x100 - 0xC0 * 2 - 8 - 1 - MAX_CHANNELS_N163 + m_pModule->GetNamcoChannels()] = 3;
	}

	if (m_pModule->GetSoundChipSet().IsMultiChip()) {		// // // special processing for multichip
		int ptr = FT_UPDATE_EXT_ADR;
		for (auto chip : EXPANSION_CHIPS) {
			ASSERT(pData[ptr] == 0x20); // jsr
			if (!m_pModule->HasExpansionChip(chip)) {
				pData[ptr++] = 0xEA; // nop
				pData[ptr++] = 0xEA;
				pData[ptr++] = 0xEA;
			}
			else
				ptr += 3;
		}

		const int CH_MAP[] = {
			0, 1, 2, 3, 27,
			6, 7, 8,
			4, 5, -1,
			9, 10, 11, 12, 13, 14, 15, 16,
			17,
			21, 22, 23, 24, 25, 26,
			18, 19, 20,
		};

		for (int i = 0; i < CHANID_COUNT; ++i)
			pData[FT_CH_ENABLE_ADR + i] = 0;
		m_ChannelOrder.ForeachChannel([&] (chan_id_t x) {
			pData[FT_CH_ENABLE_ADR + CH_MAP[value_cast(x)]] = 1;
		});
	}

	// // // Copy the vibrato table, the stock one only works for new vibrato mode
	for (int i = 0; i < 256; ++i)
		pData[m_iVibratoTableLocation + i] = (char)pSoundGen->ReadVibratoTable(i);

	return pData;
}

void CCompiler::SetDriverSongAddress(unsigned char *pDriver, unsigned short Address) const
{
	// Write start address of music data
	pDriver[m_iDriverSize - 2] = Address & 0xFF;
	pDriver[m_iDriverSize - 1] = Address >> 8;
}

stNSFHeader CCompiler::CreateHeader(int MachineType) const		// // //
{
	// Fill the NSF header
	//
	// Speed will be the same for NTSC/PAL
	//

	stNSFHeader Header;		// // //
	Header.TotalSongs = (uint8_t)m_pModule->GetSongCount();
	Header.LoadAddr = m_iLoadAddress;
	Header.InitAddr = m_iInitAddress;
	Header.PlayAddr = m_iInitAddress + 3;
	strncpy((char *)Header.SongName,   title_.data(), std::size(Header.SongName));
	strncpy((char *)Header.ArtistName, artist_.data(), std::size(Header.ArtistName));
	strncpy((char *)Header.Copyright,  copyright_.data(), std::size(Header.Copyright));
	Header.SoundChip = m_pModule->GetSoundChipSet().GetNSFFlag();

	// If speed is default, write correct NTSC/PAL speed periods
	// else, set the same custom speed for both
	int Speed = m_pModule->GetEngineSpeed();
	Header.Speed_NTSC = Speed ? 1000000 / Speed : 1000000 / 60; //0x411A; // default ntsc speed
	Header.Speed_PAL = Speed ? 1000000 / Speed : 1000000 / 50; //0x4E20; // default pal speed

	if (m_bBankSwitched) {
		for (int i = 0; i < 4; ++i) {
			unsigned int SampleBank = m_iFirstSampleBank + i;
			Header.BankValues[i] = i;
			Header.BankValues[i + 4] = (SampleBank < m_iLastBank) ? SampleBank : m_iLastBank;
		}
		// Bind last page to last bank
		if constexpr (LAST_BANK_FIXED)
			Header.BankValues[7] = m_iLastBank;
	}

	// Allow PAL or dual tunes only if no expansion chip is selected
	// Expansion chips weren't available in PAL areas
	if (!m_pModule->HasExpansionChips())
		Header.Flags = MachineType;

	return Header;
}

stNSFeHeader CCompiler::CreateNSFeHeader(int MachineType)		// // //
{
	stNSFeHeader Header;

	Header.TotalSongs = (uint8_t)m_pModule->GetSongCount();
	Header.LoadAddr = m_iLoadAddress;
	Header.InitAddr = m_iInitAddress;
	Header.PlayAddr = m_iInitAddress + 3;
	Header.SoundChip = m_pModule->GetSoundChipSet().GetNSFFlag();

	int Speed = m_pModule->GetEngineSpeed();
	Header.Speed_NTSC = Speed ? 1000000 / Speed : 1000000 / 60; //0x411A; // default ntsc speed

	if (m_bBankSwitched) {
		for (int i = 0; i < 4; ++i) {
			unsigned int SampleBank = m_iFirstSampleBank + i;
			Header.BankValues[i] = i;
			Header.BankValues[i + 4] = (SampleBank < m_iLastBank) ? SampleBank : m_iLastBank;
		}
		// Bind last page to last bank
		if constexpr (LAST_BANK_FIXED)
			Header.BankValues[7] = m_iLastBank;
	}

	if (!m_pModule->HasExpansionChips())
		Header.Flags = MachineType;

	return Header;
}


void CCompiler::UpdateSamplePointers(unsigned int Origin)
{
	// Rewrite sample pointer list with valid addresses
	//
	// TODO: rewrite this to utilize the CChunkDataBank to resolve bank numbers automatically
	//

	ASSERT(m_pSamplePointersChunk != NULL);

	unsigned int Address = Origin;
	unsigned int Bank = m_iFirstSampleBank;

	if (!m_bBankSwitched)
		Bank = 0;			// Disable DPCM bank switching

	m_pSamplePointersChunk->Clear();

	// The list is stored in the same order as the samples vector

	for (auto pDSample : m_vSamples) {
		unsigned int Size = pDSample->size();

		if (m_bBankSwitched) {
			if ((Address + Size) >= DPCM_SWITCH_ADDRESS) {
				Address = PAGE_SAMPLES;
				Bank += DPCM_PAGE_WINDOW;
			}
		}

		// Store
		m_pSamplePointersChunk->StoreByte(Address >> 6);
		m_pSamplePointersChunk->StoreByte(Size >> 4);
		m_pSamplePointersChunk->StoreByte(Bank);

#ifdef _DEBUG
		auto name = conv::to_wide(pDSample->name());
		Print(L" * DPCM sample %.*s: $%04X, bank %i (%i bytes)\n", name.size(), name.data(), Address, Bank, Size);
#endif
		Address += Size;
		Address += AdjustSampleAddress(Address);
	}
#ifdef _DEBUG
	Print(L" * DPCM sample banks: %i\n", Bank - m_iFirstSampleBank + DPCM_PAGE_WINDOW);
#endif

	// Save last bank number for NSF header
	m_iLastBank = Bank + 1;
}

void CCompiler::UpdateFrameBanks()
{
	// Write bank numbers to frame lists (can only be used when bankswitching is used)

	int Channels = m_ChannelOrder.GetChannelCount();		// // //

	for (CChunk *pChunk : m_vFrameChunks) {
		// Add bank data
		for (int j = 0; j < Channels; ++j) {
			unsigned char bank = GetObjectByLabel(pChunk->GetDataPointerTarget(j))->GetBank();
			if (bank < PATTERN_SWITCH_BANK)
				bank = PATTERN_SWITCH_BANK;
			pChunk->SetupBankData(j + Channels, bank);
		}
	}
}

void CCompiler::UpdateSongBanks()
{
	// Write bank numbers to song lists (can only be used when bankswitching is used)
	for (CChunk *pChunk : m_vSongChunks) {
		int bank = GetObjectByLabel(pChunk->GetDataPointerTarget(0))->GetBank();
		if (bank < PATTERN_SWITCH_BANK)
			bank = PATTERN_SWITCH_BANK;
		pChunk->SetupBankData(m_iSongBankReference, bank);
	}
}

void CCompiler::ClearSongBanks()
{
	// Clear bank data in song chunks
	for (CChunk *pChunk : m_vSongChunks)
		pChunk->SetupBankData(m_iSongBankReference, 0);
}

void CCompiler::EnableBankswitching()
{
	// Set bankswitching flag in the song header
	ASSERT(m_pHeaderChunk != NULL);
	unsigned char flags = (unsigned char)m_pHeaderChunk->GetData(m_iHeaderFlagOffset);
	flags |= FLAG_BANKSWITCHED;
	m_pHeaderChunk->ChangeByte(m_iHeaderFlagOffset, flags);
}

void CCompiler::ResolveLabels()
{
	// Resolve label addresses, no banks since bankswitching is disabled
	std::map<stChunkLabel, int> labelMap;		// // //

	// Pass 1, collect labels
	CollectLabels(labelMap);

	// Pass 2
	AssignLabels(labelMap);
}

bool CCompiler::ResolveLabelsBankswitched()
{
	// Resolve label addresses and banks
	std::map<stChunkLabel, int> labelMap;		// // //

	// Pass 1, collect labels
	if (!CollectLabelsBankswitched(labelMap))
		return false;

	// Pass 2
	AssignLabels(labelMap);

	return true;
}

void CCompiler::CollectLabels(std::map<stChunkLabel, int> &labelMap) const		// // //
{
	// Collect labels and assign offsets
	int Offset = 0;
	for (const auto &pChunk : m_vChunks) {
		labelMap[pChunk->GetLabel()] = Offset;
		Offset += pChunk->CountDataSize();
	}
}

bool CCompiler::CollectLabelsBankswitched(std::map<stChunkLabel, int> &labelMap)		// // //
{
	int Offset = 0;
	int Bank = PATTERN_SWITCH_BANK;

	// Instruments and stuff
	for (const auto &pChunk : m_vChunks) {
		int Size = pChunk->CountDataSize();

		switch (pChunk->GetType()) {
			case CHUNK_FRAME_LIST:
			case CHUNK_FRAME:
			case CHUNK_PATTERN:
				break;
			default:
				labelMap[pChunk->GetLabel()] = Offset;
				Offset += Size;
		}
	}

	if (Offset + m_iDriverSize > 0x3000) {
		// Instrument data did not fit within the limit, display an error and abort?
		Print(L"Error: Instrument data overflow, can't export file!\n");
		return false;
	}

	unsigned int Track = 0;

	// The switchable area is $B000-$C000
	for (auto &pChunk : m_vChunks) {
		int Size = pChunk->CountDataSize();

		switch (pChunk->GetType()) {
			case CHUNK_FRAME_LIST:
				// Make sure the entire frame list will fit, if not then allocate a new bank
				if (Offset + m_iDriverSize + m_iTrackFrameSize[Track++] > 0x4000) {
					Offset = 0x3000 - m_iDriverSize;
					++Bank;
				}
				[[fallthrough]];		// // //
			case CHUNK_FRAME:
				labelMap[pChunk->GetLabel()] = Offset;
				pChunk->SetBank(Bank < 4 ? ((Offset + m_iDriverSize) >> 12) : Bank);
				Offset += Size;
				break;
			case CHUNK_PATTERN:
				// Make sure entire pattern will fit
				if (Offset + m_iDriverSize + Size > 0x4000) {
					Offset = 0x3000 - m_iDriverSize;
					++Bank;
				}
				labelMap[pChunk->GetLabel()] = Offset;
				pChunk->SetBank(Bank < 4 ? ((Offset + m_iDriverSize) >> 12) : Bank);
				Offset += Size;
			default:
				break;
		}
	}

	if (m_bBankSwitched)
		m_iFirstSampleBank = ((Bank < 4) ? ((Offset + m_iDriverSize) >> 12) : Bank) + 1;

	m_iLastBank = m_iFirstSampleBank;

	return true;
}

void CCompiler::AssignLabels(std::map<stChunkLabel, int> &labelMap)		// // //
{
	// Pass 2: assign addresses to labels
	for (auto &pChunk : m_vChunks)
		pChunk->AssignLabels(labelMap);
}

bool CCompiler::CompileData()
{
	// Compile music data to an object tree
	//

	// // // Full chip export

	// Select driver and channel order
	const CSoundChipSet &Chip = m_pModule->GetSoundChipSet();
	if (!Chip.IsMultiChip())
		switch (Chip.WithoutChip(sound_chip_t::APU).GetSoundChip()) {
		case sound_chip_t::NONE:
			m_pDriverData = &DRIVER_PACK_2A03;
			m_iVibratoTableLocation = VIBRATO_TABLE_LOCATION_2A03;
			Print(L" * No expansion chip\n");
			break;
		case sound_chip_t::VRC6:
			m_pDriverData = &DRIVER_PACK_VRC6;
			m_iVibratoTableLocation = VIBRATO_TABLE_LOCATION_VRC6;
			Print(L" * VRC6 expansion enabled\n");
			break;
		case sound_chip_t::MMC5:
			m_pDriverData = &DRIVER_PACK_MMC5;
			m_iVibratoTableLocation = VIBRATO_TABLE_LOCATION_MMC5;
			Print(L" * MMC5 expansion enabled\n");
			break;
		case sound_chip_t::VRC7:
			m_pDriverData = &DRIVER_PACK_VRC7;
			m_iVibratoTableLocation = VIBRATO_TABLE_LOCATION_VRC7;
			Print(L" * VRC7 expansion enabled\n");
			break;
		case sound_chip_t::FDS:
			m_pDriverData = &DRIVER_PACK_FDS;
			m_iVibratoTableLocation = VIBRATO_TABLE_LOCATION_FDS;
			Print(L" * FDS expansion enabled\n");
			break;
		case sound_chip_t::N163:
			m_pDriverData = &DRIVER_PACK_N163;
			m_iVibratoTableLocation = VIBRATO_TABLE_LOCATION_N163;
			Print(L" * N163 expansion enabled\n");
			break;
		case sound_chip_t::S5B:
			m_pDriverData = &DRIVER_PACK_S5B;
			m_iVibratoTableLocation = VIBRATO_TABLE_LOCATION_S5B;
			Print(L" * S5B expansion enabled\n");
			break;
		}
	else {		// // //
		m_pDriverData = &DRIVER_PACK_ALL;
		m_iVibratoTableLocation = VIBRATO_TABLE_LOCATION_ALL;
		Print(L" * Multiple expansion chips enabled\n");
	}

	// Driver size
	m_iDriverSize = m_pDriverData->driver.size();		// // //

	// Scan and optimize song
	ScanSong();

	Print(L"Building music data...\n");

	// Build music data
	CreateMainHeader();
	CreateSequenceList();
	CreateInstrumentList();
	CreateSampleList();
	StoreSamples();
	StoreGrooves();		// // //
	StoreSongs();

	// Determine if bankswitching is needed
	m_bBankSwitched = false;
	m_iMusicDataSize = CountData();

	// Get samples start address
	m_iSampleStart = m_iDriverSize + m_iMusicDataSize;

	if (m_iSampleStart < 0x4000)
		m_iSampleStart = PAGE_SAMPLES;
	else
		m_iSampleStart += AdjustSampleAddress(m_iSampleStart) + PAGE_START;

	if (m_iSampleStart + m_iSamplesSize > 0xFFFF)
		m_bBankSwitched = true;

	if (m_iSamplesSize > 0x4000)
		m_bBankSwitched = true;

	if ((m_iMusicDataSize + m_iSamplesSize + m_iDriverSize) > 0x8000)
		m_bBankSwitched = true;

	if (m_bBankSwitched)
		m_iSampleStart = PAGE_SAMPLES;

	// Compiling done
	Print(L" * Samples located at: $%04X\n", m_iSampleStart);

#ifdef FORCE_BANKSWITCH
	m_bBankSwitched = true;
#endif /* FORCE_BANKSWITCH */

	return true;
}

void CCompiler::AddBankswitching()
{
	// Add bankswitching data

	for (auto &pChunk : m_vChunks) {
		// Frame chunks
		if (pChunk->GetType() == CHUNK_FRAME) {
			int Length = pChunk->GetLength();
			// Bank data is located at end
			for (int j = 0; j < Length; ++j) {
				pChunk->StoreBankReference(pChunk->GetDataPointerTarget(j), 0);
			}
		}
	}

	// Frame lists sizes has changed
	m_pModule->VisitSongs([&] (const CSongData &song, unsigned i) {		// // //
		m_iTrackFrameSize[i] += m_ChannelOrder.GetChannelCount() * song.GetFrameCount();
	});

	// Data size has changed
	m_iMusicDataSize = CountData();
}

void CCompiler::ScanSong()
{
	// Scan and optimize song
	//

	// Re-assign instruments
	m_iAssignedInstruments.clear();		// // //

	// TODO: remove these
	m_bSequencesUsed2A03.fill({ });
	m_bSequencesUsedVRC6.fill({ });
	m_bSequencesUsedN163.fill({ });
	m_bSequencesUsedS5B.fill({ });

	static const inst_type_t INST[] = {INST_2A03, INST_VRC6, INST_N163, INST_S5B};		// // //
	decltype(m_bSequencesUsed2A03) *used[] = {&m_bSequencesUsed2A03, &m_bSequencesUsedVRC6, &m_bSequencesUsedN163, &m_bSequencesUsedS5B};

	bool inst_used[MAX_INSTRUMENTS] = { };		// // //

	auto &Im = *m_pModule->GetInstrumentManager();

	// // // Scan patterns in entire module
	m_pModule->VisitSongs([&] (const CSongData &song) {
		int PatternLength = song.GetPatternLength();
		m_ChannelOrder.ForeachChannel([&] (chan_id_t j) {
			for (int k = 0; k < MAX_PATTERN; ++k)
				for (int l = 0; l < PatternLength; ++l) {
					const auto &note = song.GetPattern(j, k).GetNoteOn(l);
					if (note.Instrument < std::size(inst_used))		// // //
						inst_used[note.Instrument] = true;
				}
		});
	});

	Im.VisitInstruments([&] (const CInstrument &inst, std::size_t i) {
		if (inst_used[i]) {		// // //
			// List of used instruments
			m_iAssignedInstruments.push_back(i);

			// Create a list of used sequences
			inst_type_t it = Im.GetInstrumentType(i);		// // //
			for (size_t z = 0; z < std::size(used); z++) if (it == INST[z]) {
				auto pInstrument = static_cast<const CSeqInstrument *>(&inst);
				foreachSeq([&] (sequence_t j) {
					if (pInstrument->GetSeqEnable(j))
						(*used[z])[pInstrument->GetSeqIndex(j)][(unsigned)j] = true;
				});
				break;
			}
		}
	});

	// See which samples are used
	m_iSamplesUsed = 0;

	m_bSamplesAccessed.fill({ });		// // //

	// Get DPCM channel index
	unsigned int Instrument = 0;

	m_pModule->VisitSongs([&] (const CSongData &song) {		// // //
		const int patternlen = song.GetPatternLength();
		const int frames = song.GetFrameCount();
		const auto *pTrack = song.GetTrack(chan_id_t::DPCM);
		for (int j = 0; j < frames; ++j) {
			const auto &Pattern = pTrack->GetPatternOnFrame(j);
			for (int k = 0; k < patternlen; ++k) {
				const auto &Note = Pattern.GetNoteOn(k);
				if (Note.Instrument < MAX_INSTRUMENTS)
					Instrument = Note.Instrument;
				if (IsNote(Note.Note))		// // //
					m_bSamplesAccessed[Instrument][MIDI_NOTE(Note.Octave, Note.Note)] = true;
			}
		}
	});
}

void CCompiler::CreateMainHeader()
{
	const CSoundChipSet &Chip = m_pModule->GetSoundChipSet();		// // //

	unsigned char Flags =		// // // bankswitch flag is set later
		(m_pModule->GetVibratoStyle() == VIBRATO_OLD ? FLAG_VIBRATO : 0) |
		(m_pModule->GetLinearPitch() ? FLAG_LINEARPITCH : 0);

	CChunk &Chunk = CreateChunk({CHUNK_HEADER});		// // //
	Chunk.StorePointer({CHUNK_SONG_LIST});		// // //
	Chunk.StorePointer({CHUNK_INSTRUMENT_LIST});
	Chunk.StorePointer({CHUNK_SAMPLE_LIST});
	Chunk.StorePointer({CHUNK_SAMPLE_POINTERS});
	Chunk.StorePointer({CHUNK_GROOVE_LIST});		// // //

	m_iHeaderFlagOffset = Chunk.GetLength();		// Save the flags offset
	Chunk.StoreByte(Flags);

	// FDS table, only if FDS is enabled
	if (Chip.ContainsChip(sound_chip_t::FDS) || Chip.IsMultiChip())
		Chunk.StorePointer({CHUNK_WAVETABLE});		// // //

	const int TicksPerSec = m_pModule->GetEngineSpeed();
	Chunk.StoreWord((TicksPerSec ? TicksPerSec : CAPU::FRAME_RATE_NTSC) * 60);
	Chunk.StoreWord((TicksPerSec ? TicksPerSec : CAPU::FRAME_RATE_PAL) * 60);

	// N163 channel count
	if (Chip.ContainsChip(sound_chip_t::N163) || Chip.IsMultiChip())
		Chunk.StoreByte(m_pModule->GetNamcoChannels() ? m_pModule->GetNamcoChannels() : 1);

	m_pHeaderChunk = &Chunk;
}

// Sequences

void CCompiler::CreateSequenceList()
{
	// Create sequence lists
	//

	unsigned int Size = 0, StoredCount = 0;
	const inst_type_t inst[] = {INST_2A03, INST_VRC6, INST_N163, INST_S5B};
	decltype(m_bSequencesUsed2A03) *used[] = {&m_bSequencesUsed2A03, &m_bSequencesUsedVRC6, &m_bSequencesUsedN163, &m_bSequencesUsedS5B};

	auto &Im = *m_pModule->GetInstrumentManager();

	// TODO: use the CSeqInstrument::GetSequence
	// TODO: merge identical sequences from all chips
	for (size_t c = 0; c < std::size(inst); c++) {
		for (int i = 0; i < MAX_SEQUENCES; ++i) foreachSeq([&] (sequence_t j) {
			const auto pSeq = Im.GetSequence(inst[c], j, i);
			if ((*used[c])[i][(unsigned)j] && pSeq->GetItemCount() > 0) {
				Size += StoreSequence(*pSeq, {CHUNK_SEQUENCE, i * SEQ_COUNT + (unsigned)j, (unsigned)inst[c]});
				++StoredCount;
			}
		});
	}

	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (auto pInstrument = std::dynamic_pointer_cast<CInstrumentFDS>(Im.GetInstrument(i))) {
			foreachSeq([&] (sequence_t j) {
				const auto pSeq = pInstrument->GetSequence(j);		// // //
				if (pSeq && pSeq->GetItemCount() > 0) {
					unsigned Index = i * SEQ_COUNT + (unsigned)j;
					Size += StoreSequence(*pSeq, {CHUNK_SEQUENCE, Index, INST_FDS});		// // //
					++StoredCount;
				}
			});
		}
	}

	Print(L" * Sequences used: %i (%i bytes)\n", StoredCount, Size);
}

int CCompiler::StoreSequence(const CSequence &Seq, const stChunkLabel &label)		// // //
{
	CChunk &Chunk = CreateChunk(label);		// // //

	// Store the sequence
	int iItemCount	  = Seq.GetItemCount();
	int iLoopPoint	  = Seq.GetLoopPoint();
	int iReleasePoint = Seq.GetReleasePoint();
	int iSetting	  = Seq.GetSetting();

	if (iReleasePoint != -1)
		iReleasePoint += 1;
	else
		iReleasePoint = 0;

	if (iLoopPoint > iItemCount)
		iLoopPoint = -1;

	Chunk.StoreByte((unsigned char)iItemCount);
	Chunk.StoreByte((unsigned char)iLoopPoint);
	Chunk.StoreByte((unsigned char)iReleasePoint);
	Chunk.StoreByte((unsigned char)iSetting);

	for (int i = 0; i < iItemCount; ++i) {
		Chunk.StoreByte(Seq.GetItem(i));
	}

	// Return size of this chunk
	return iItemCount + 4;
}

// Instruments

void CCompiler::CreateInstrumentList()
{
	/*
	 * Create the instrument list
	 *
	 * The format of instruments depends on the type
	 *
	 */

	unsigned int iTotalSize = 0;
	CChunk *pWavetableChunk = NULL;	// FDS
	CChunk *pWavesChunk = NULL;		// N163
	int iWaveSize = 0;				// N163 waves size

	CChunk &InstListChunk = CreateChunk({CHUNK_INSTRUMENT_LIST});		// // //

	auto &Im = *m_pModule->GetInstrumentManager();

	if (m_pModule->HasExpansionChip(sound_chip_t::FDS) || m_pModule->GetSoundChipSet().IsMultiChip())
		pWavetableChunk = &CreateChunk({CHUNK_WAVETABLE});		// // //

	m_iWaveBanks.fill(-1);		// // //

	// Collect N163 waves
	const CInstCompilerN163 n163_c;		// // //
	for (unsigned int i = 0; i < m_iAssignedInstruments.size(); ++i) {
		unsigned iIndex = m_iAssignedInstruments[i];
		if (Im.GetInstrumentType(iIndex) == INST_N163 && m_iWaveBanks[i] == (unsigned)-1) {
			auto pInstrument = std::static_pointer_cast<CInstrumentN163>(Im.GetInstrument(iIndex));
			for (unsigned int j = i + 1; j < m_iAssignedInstruments.size(); ++j) {
				unsigned inst = m_iAssignedInstruments[j];
				if (Im.GetInstrumentType(inst) == INST_N163 && m_iWaveBanks[j] == (unsigned)-1) {
					auto pNewInst = std::static_pointer_cast<CInstrumentN163>(Im.GetInstrument(inst));
					if (pInstrument->IsWaveEqual(*pNewInst))
						m_iWaveBanks[j] = iIndex;
				}
			}
			if (m_iWaveBanks[i] == (unsigned)-1) {
				m_iWaveBanks[i] = iIndex;
				pWavesChunk = &CreateChunk({CHUNK_WAVES, iIndex});		// // //
				n163_c.StoreWaves(*pInstrument, *pWavesChunk);		// // //
			}
		}
	}

	// Store instruments
	for (unsigned int i = 0; i < m_iAssignedInstruments.size(); ++i) {
		CChunk &Chunk = AddChunkToList(InstListChunk, {CHUNK_INSTRUMENT, i});		// // //
		iTotalSize += 2;

		unsigned iIndex = m_iAssignedInstruments[i];
		auto pInstrument = Im.GetInstrument(iIndex);
/*
		if (pInstrument->GetType() == INST_N163) {
			pWavesChunk = CreateChunk(CHUNK_WAVES, FormattedW(LABEL_WAVES, iIndex));
			// Store waves
			iWaveSize += ((CInstrumentN163*)pInstrument)->StoreWave(pWavesChunk);
		}
*/

		if (pInstrument->GetType() == INST_N163) {
			// Translate wave index
			iIndex = m_iWaveBanks[i];
		}

		// Returns number of bytes
		const auto &compiler = Env.GetInstrumentService()->GetChunkCompiler(pInstrument->GetType());		// // //
		iTotalSize += compiler.CompileChunk(*pInstrument, Chunk, iIndex);

		// // // Check if FDS
		if (pInstrument->GetType() == INST_FDS && pWavetableChunk != NULL) {
			// Store wave
			AddWavetable(std::static_pointer_cast<CInstrumentFDS>(pInstrument).get(), pWavetableChunk);
			Chunk.StoreByte(m_iWaveTables - 1);
		}
	}

	Print(L" * Instruments used: %i (%i bytes)\n", m_iAssignedInstruments.size(), iTotalSize);

	if (iWaveSize > 0)
		Print(L" * N163 waves size: %i bytes\n", iWaveSize);
}

// Samples

void CCompiler::CreateSampleList()
{
	/*
	 * DPCM instrument list
	 *
	 * Each item is stored as a pair of the sample pitch and pointer to the sample table
	 *
	 */

	const int SAMPLE_ITEM_WIDTH = 3;	// 3 bytes / sample item

	// Clear the sample list
	m_iSampleBank.fill(0xFFu);		// // //

	auto &Im = *m_pModule->GetInstrumentManager();		// // //
	auto &Dm = *m_pModule->GetDSampleManager();		// // //

	CChunk &Chunk = CreateChunk({CHUNK_SAMPLE_LIST});		// // //

	// Store sample instruments
	unsigned int Item = 0;
	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (Im.IsInstrumentUsed(i) && Im.GetInstrumentType(i) == INST_2A03) {
			auto pInstrument = std::static_pointer_cast<CInstrument2A03>(Im.GetInstrument(i));

			for (int n = 0; n < NOTE_COUNT; ++n) {
				// Get sample
				unsigned char iSample = pInstrument->GetSampleIndex(n);
				if ((iSample > 0) && m_bSamplesAccessed[i][n] && Dm.IsSampleUsed(iSample - 1)) {

					unsigned char SamplePitch = pInstrument->GetSamplePitch(n);
					unsigned char SampleIndex = GetSampleIndex(iSample - 1);
					unsigned int  SampleDelta = pInstrument->GetSampleDeltaValue(n);
					SamplePitch |= (SamplePitch & 0x80) >> 1;

					// Save a reference to this item
					m_iSamplesLookUp[i][n] = ++Item;

					Chunk.StoreByte(SamplePitch);
					Chunk.StoreByte(SampleDelta);
					Chunk.StoreByte(SampleIndex * SAMPLE_ITEM_WIDTH);
				}
				else
					// No instrument here
					m_iSamplesLookUp[i][n] = 0;
			}
		}
	}
}

void CCompiler::StoreSamples()
{
	/*
	 * DPCM sample list
	 *
	 * Each sample is stored as a pair of the sample address and sample size
	 *
	 */

	unsigned int iAddedSamples = 0;
	unsigned int iSampleAddress = 0x0000;

	auto &Dm = *m_pModule->GetDSampleManager();		// // //

	// Get sample start address
	m_iSamplesSize = 0;

	CChunk &Chunk = CreateChunk({CHUNK_SAMPLE_POINTERS});		// // //
	m_pSamplePointersChunk = &Chunk;

	// Store DPCM samples in a separate array
	for (unsigned int i = 0; i < m_iSamplesUsed; ++i) {

		unsigned int iIndex = m_iSampleBank[i];
		ASSERT(iIndex != 0xFF);
		auto pDSample = Dm.GetDSample(iIndex);
		unsigned int iSize = pDSample->size();

		if (iSize > 0) {
			// Fill sample list
			unsigned char iSampleAddr = iSampleAddress >> 6;
			unsigned char iSampleSize = iSize >> 4;
			unsigned char iSampleBank = 0;

			// Update SAMPLE_ITEM_WIDTH here
			Chunk.StoreByte(iSampleAddr);
			Chunk.StoreByte(iSampleSize);
			Chunk.StoreByte(iSampleBank);

			// Add this sample to storage
			m_vSamples.push_back(pDSample);

			// Pad end of samples
			unsigned int iAdjust = AdjustSampleAddress(iSampleAddress + iSize);

			iAddedSamples++;
			iSampleAddress += iSize + iAdjust;
			m_iSamplesSize += iSize + iAdjust;
		}
	}

	Print(L" * DPCM samples used: %i (%i bytes)\n", m_iSamplesUsed, m_iSamplesSize);
}

int CCompiler::GetSampleIndex(int SampleNumber)
{
	// Returns a sample pos from the sample bank
	for (int i = 0; i < MAX_DSAMPLES; i++) {
		if (m_iSampleBank[i] == SampleNumber)
			return i;							// Sample is already stored
		else if(m_iSampleBank[i] == 0xFF) {
			m_iSampleBank[i] = SampleNumber;	// Allocate new position
			m_iSamplesUsed++;
			return i;
		}
	}

	// TODO: Fail if getting here!!!
	return SampleNumber;
}

// // // Groove list

void CCompiler::StoreGrooves()
{
	/*
	 * Store grooves
	 */

	unsigned int Size = 1, Count = 0;

	CChunk &GrooveListChunk = CreateChunk({CHUNK_GROOVE_LIST});
	GrooveListChunk.StoreByte(0); // padding; possibly used to disable groove

	for (unsigned i = 0; i < MAX_GROOVE; i++) {
		if (const auto pGroove = m_pModule->GetGroove(i)) {
			unsigned int Pos = Size;
			CChunk &Chunk = CreateChunk({CHUNK_GROOVE, i});
			for (uint8_t entry : *pGroove)
				Chunk.StoreByte(entry);
			Chunk.StoreByte(0);
			Chunk.StoreByte(Pos);
			Size += Chunk.CountDataSize();
			++Count;
		}
	}

	Print(L" * Grooves used: %i (%i bytes)\n", Count, Size);
}

// Songs

void CCompiler::StoreSongs()
{
	/*
	 * Store patterns and frames for each song
	 *
	 */

	CChunk &SongListChunk = CreateChunk({CHUNK_SONG_LIST});		// // //

	m_iDuplicatePatterns = 0;

	// Store song info
	m_pModule->VisitSongs([&] (const CSongData &song, unsigned index) {
		// Create song
		CChunk &Chunk = AddChunkToList(SongListChunk, {CHUNK_SONG, index});		// // //
		m_vSongChunks.push_back(&Chunk);

		// Store reference to song
		Chunk.StorePointer({CHUNK_FRAME_LIST, index});		// // //
		Chunk.StoreByte(song.GetFrameCount());
		Chunk.StoreByte(song.GetPatternLength());

		bool useGroove = song.GetSongGroove();
		auto pGroove = useGroove ? m_pModule->GetGroove(song.GetSongSpeed()) : nullptr;

		if (useGroove && pGroove) {		// // //
			Chunk.StoreByte(0);
			Chunk.StoreByte(song.GetSongTempo());
			int Pos = 1;
			for (unsigned int j = 0; j < song.GetSongSpeed(); ++j)
				if (const auto pPrev = m_pModule->GetGroove(j))
					Pos += pPrev->compiled_size();
			Chunk.StoreByte(Pos);
		}
		else {
			Chunk.StoreByte(useGroove ? DEFAULT_SPEED : song.GetSongSpeed());
			Chunk.StoreByte(song.GetSongTempo());
			Chunk.StoreByte(0);
		}

		Chunk.StoreBankReference({CHUNK_FRAME_LIST, index}, 0);		// // //
	});

	m_iSongBankReference = m_vSongChunks[0]->GetLength() - 1;	// Save bank value position (all songs are equal)

	// Store actual songs
	m_pModule->VisitSongs([this] (const CSongData &, unsigned i) {
		Print(L" * Song %i: ", i);
		// Store frames
		CreateFrameList(i);
		// Store pattern data
		StorePatterns(i);
	});

	if (m_iDuplicatePatterns > 0)
		Print(L" * %i duplicated pattern(s) removed\n", m_iDuplicatePatterns);

#ifdef _DEBUG
	Print(L"Hash collisions: %i (of %i items)\r\n", m_iHashCollisions, m_PatternMap.size());		// // //
#endif
}

// Frames

void CCompiler::CreateFrameList(unsigned int Track)
{
	/*
	 * Creates a frame list
	 *
	 * The pointer list is just pointing to each item in the frame list
	 * and the frame list holds the offset addresses for the patterns for all channels
	 *
	 * ---------------------
	 *  Frame entry pointers
	 *  $XXXX (2 bytes, offset to a frame entry)
	 *  ...
	 * ---------------------
	 *
	 * ---------------------
	 *  Frame entries
	 *  $XXXX * 4 (2 * 2 bytes, each pair is an offset to the pattern)
	 * ---------------------
	 *
	 */

	const auto *pSong = m_pModule->GetSong(Track);		// // //
	const unsigned FrameCount = pSong->GetFrameCount();		// // //

	// Create frame list
	CChunk &FrameListChunk = CreateChunk({CHUNK_FRAME_LIST, Track});		// // //

	unsigned int TotalSize = 0;

	// Store addresses to patterns
	for (unsigned i = 0; i < FrameCount; ++i) {
		// Store frame item
		CChunk &Chunk = AddChunkToList(FrameListChunk, {CHUNK_FRAME, Track, i});		// // //
		m_vFrameChunks.push_back(&Chunk);
		TotalSize += 2;

		// Pattern pointers
		m_ChannelOrder.ForeachChannel([&] (chan_id_t Chan) {
			unsigned Pattern = pSong->GetFramePattern(i, Chan);
			Chunk.StorePointer({CHUNK_PATTERN, Track, Pattern, value_cast(Chan)});		// // //
			TotalSize += 2;
		});
	}

	m_iTrackFrameSize[Track] = TotalSize;

	Print(L"%i frames (%i bytes), ", FrameCount, TotalSize);
}

// Patterns

void CCompiler::StorePatterns(unsigned int Track)
{
	/*
	 * Store patterns and save references to them for the frame list
	 *
	 */

	CPatternCompiler PatternCompiler(*m_pModule, m_iAssignedInstruments, (const DPCM_List_t *)m_iSamplesLookUp.data(), m_pLogger);		// // //

	int PatternCount = 0;
	int PatternSize = 0;

	// Iterate through all patterns
	for (unsigned i = 0; i < MAX_PATTERN; ++i) {
		m_ChannelOrder.ForeachChannel([&] (chan_id_t j) {		// // //
			// And store only used ones
			if (IsPatternAddressed(Track, i, j)) {

				// Compile pattern data
				PatternCompiler.CompileData(Track, i, j);

				auto label = stChunkLabel {CHUNK_PATTERN, Track, i, value_cast(j)};		// // //

				bool StoreNew = true;

#ifdef REMOVE_DUPLICATE_PATTERNS
				unsigned int Hash = PatternCompiler.GetHash();

				// Check for duplicate patterns
				if (auto it = m_PatternMap.find(Hash); it != m_PatternMap.end()) {
					const CChunk *pDuplicate = it->second;
					// Hash only indicates that patterns may be equal, check exact data
					if (PatternCompiler.CompareData(pDuplicate->GetStringData(PATTERN_CHUNK_INDEX))) {
						// Duplicate was found, store a reference to existing pattern
						m_DuplicateMap.try_emplace(label, pDuplicate->GetLabel());		// // //
						++m_iDuplicatePatterns;
						StoreNew = false;
					}
				}
#endif /* REMOVE_DUPLICATE_PATTERNS */

				if (StoreNew) {
					// Store new pattern
					CChunk &Chunk = CreateChunk(label);		// // //

#ifdef REMOVE_DUPLICATE_PATTERNS
					if (m_PatternMap.count(Hash))
						++m_iHashCollisions;
					m_PatternMap[Hash] = &Chunk;
#endif /* REMOVE_DUPLICATE_PATTERNS */

					// Store pattern data as string
					Chunk.StoreString(PatternCompiler.GetData());

					PatternSize += PatternCompiler.GetDataSize();
					++PatternCount;
				}
			}
		});
	}

#ifdef REMOVE_DUPLICATE_PATTERNS
	// Update references to duplicates
	for (const auto pChunk : m_vFrameChunks)
		for (int j = 0, n = pChunk->GetLength(); j < n; ++j)
			if (auto it = m_DuplicateMap.find(pChunk->GetDataPointerTarget(j)); it != m_DuplicateMap.cend())		// // //
				pChunk->SetDataPointerTarget(j, it->second);
#endif /* REMOVE_DUPLICATE_PATTERNS */

#ifdef LOCAL_DUPLICATE_PATTERN_REMOVAL
	// Forget patterns when one whole track is stored
	m_PatternMap.RemoveAll();
	m_DuplicateMap.RemoveAll();
#endif /* LOCAL_DUPLICATE_PATTERN_REMOVAL */

	Print(L"%i patterns (%i bytes)\r\n", PatternCount, PatternSize);
}

bool CCompiler::IsPatternAddressed(unsigned int Track, int Pattern, chan_id_t Channel) const
{
	// Scan the frame list to see if a pattern is accessed for that frame
	if (const auto *pSong = m_pModule->GetSong(Track))		// // //
		if (const auto *pTrack = pSong->GetTrack(Channel))
			for (int i = 0, n = pSong->GetFrameCount(); i < n; ++i)
				if (pTrack->GetFramePattern(i) == Pattern)
					return true;

	return false;
}

void CCompiler::AddWavetable(CInstrumentFDS *pInstrument, CChunk *pChunk)
{
	// TODO Find equal existing waves

	// Allocate new wave
	for (int i = 0; i < 64; ++i)
		pChunk->StoreByte(pInstrument->GetSample(i));

	m_iWaveTables++;
}

// Object list functions

CChunk &CCompiler::CreateChunk(const stChunkLabel &Label) {		// // //
	return *m_vChunks.emplace_back(std::make_shared<CChunk>(Label));
}

CChunk &CCompiler::AddChunkToList(CChunk &Chunk, const stChunkLabel &Label) {		// // //
	Chunk.StorePointer(Label);
	return CreateChunk(Label);
}

int CCompiler::CountData() const
{
	// Only count data
	int Offset = 0;

	for (const auto &pChunk : m_vChunks)
		Offset += pChunk->CountDataSize();

	return Offset;
}

CChunk *CCompiler::GetObjectByLabel(const stChunkLabel &Label) const		// // //
{
	for (const auto &pChunk : m_vChunks)
		if (Label == pChunk->GetLabel())		// // //
			return pChunk.get();
	return nullptr;
}

#if 0

void CCompiler::WriteChannelMap()
{
	CChunk &Chunk = CreateChunk(CHUNK_CHANNEL_MAP, "");

	pChunk->StoreByte(chan_id_t::SQUARE1 + 1);
	pChunk->StoreByte(chan_id_t::SQUARE2 + 1);
	pChunk->StoreByte(chan_id_t::TRIANGLE + 1);
	pChunk->StoreByte(chan_id_t::NOISE + 1);

	if (m_pDocument->ExpansionEnabled(sound_chip_t::VRC6)) {
		pChunk->StoreByte(chan_id_t::VRC6_PULSE1 + 1);
		pChunk->StoreByte(chan_id_t::VRC6_PULSE2 + 1);
		pChunk->StoreByte(chan_id_t::VRC6_SAWTOOTH + 1);
	}

	if (m_pDocument->ExpansionEnabled(sound_chip_t::VRC7)) {
		pChunk->StoreByte(chan_id_t::VRC7_CH1 + 1);
		pChunk->StoreByte(chan_id_t::VRC7_CH2 + 1);
		pChunk->StoreByte(chan_id_t::VRC7_CH3 + 1);
		pChunk->StoreByte(chan_id_t::VRC7_CH4 + 1);
		pChunk->StoreByte(chan_id_t::VRC7_CH5 + 1);
		pChunk->StoreByte(chan_id_t::VRC7_CH6 + 1);
	}

	if (m_pDocument->ExpansionEnabled(sound_chip_t::FDS)) {
		pChunk->StoreByte(chan_id_t::FDS + 1);
	}

	if (m_pDocument->ExpansionEnabled(sound_chip_t::MMC5)) {
		pChunk->StoreByte(chan_id_t::MMC5_SQUARE1 + 1);
		pChunk->StoreByte(chan_id_t::MMC5_SQUARE2 + 1);
	}

	if (m_pDocument->ExpansionEnabled(sound_chip_t::N163)) {
		for (unsigned int i = 0; i < m_pDocument->GetNamcoChannels(); ++i) {
			pChunk->StoreByte(chan_id_t::N163_CH1 + i + 1);
		}
	}

	pChunk->StoreByte(chan_id_t::DPCM + 1);
}

void CCompiler::WriteChannelTypes()
{
	const int TYPE_2A03 = 0;
	const int TYPE_VRC6 = 2;
	const int TYPE_VRC7 = 4;
	const int TYPE_FDS	= 6;
	const int TYPE_MMC5 = 8;
	const int TYPE_N163 = 10;
	const int TYPE_S5B	= 12;

	CChunk &Chunk = CreateChunk(CHUNK_CHANNEL_TYPES, "");

	for (int i = 0; i < MAX_CHANNELS_2A03 - 1; ++i)
		pChunk->StoreByte(TYPE_2A03);

	if (m_pDocument->ExpansionEnabled(sound_chip_t::VRC6)) {
		for (int i = 0; i < MAX_CHANNELS_VRC6; ++i)
			pChunk->StoreByte(TYPE_VRC6);
	}

	if (m_pDocument->ExpansionEnabled(sound_chip_t::VRC7)) {
		for (int i = 0; i < MAX_CHANNELS_VRC7; ++i)
			pChunk->StoreByte(TYPE_VRC7);
	}

	if (m_pDocument->ExpansionEnabled(sound_chip_t::FDS)) {
		pChunk->StoreByte(TYPE_FDS);
	}

	if (m_pDocument->ExpansionEnabled(sound_chip_t::MMC5)) {
		for (int i = 0; i < MAX_CHANNELS_MMC5 - 1; ++i)
			pChunk->StoreByte(TYPE_MMC5);
	}

	if (m_pDocument->ExpansionEnabled(sound_chip_t::N163)) {
		for (unsigned int i = 0; i < m_pDocument->GetNamcoChannels(); ++i)
			pChunk->StoreByte(TYPE_N163);
	}

	pChunk->StoreByte(TYPE_2A03);
}

#endif
