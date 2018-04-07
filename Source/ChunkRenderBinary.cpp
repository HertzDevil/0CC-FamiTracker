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

#include "ChunkRenderBinary.h"
#include <vector>
#include "FamiTrackerTypes.h"		// // //
#include "Compiler.h"
#include "Chunk.h"
#include "ft0cc/doc/dpcm_sample.hpp"		// // //
#include "SimpleFile.h"		// // //

/**
 * Binary file writer, base class binary renderers
 */

CBinaryFileWriter::CBinaryFileWriter(CSimpleFile &File) : m_fFile(File), m_iDataWritten(0)		// // //
{
}

void CBinaryFileWriter::Store(array_view<std::uint8_t> Data)
{
	m_fFile.WriteBytes(Data);
	m_iDataWritten += Data.size();
}

void CBinaryFileWriter::Fill(unsigned int Size)
{
	for (unsigned int i = 0; i < Size; ++i)
		m_fFile.WriteInt8(0);
	m_iDataWritten += Size;
}

unsigned int CBinaryFileWriter::GetWritten() const
{
	return m_iDataWritten;
}

/**
 * Binary chunk render, used to write binary files
 *
 */

void CChunkRenderBinary::StoreChunks(const std::vector<std::shared_ptr<CChunk>> &Chunks)
{
	for (auto &ptr : Chunks)		// // //
		StoreChunk(*ptr);
}

void CChunkRenderBinary::StoreSamples(const std::vector<std::shared_ptr<const ft0cc::doc::dpcm_sample>> &Samples)
{
	for (auto ptr : Samples)		// // //
		StoreSample(*ptr);
}

void CChunkRenderBinary::StoreChunk(const CChunk &Chunk)		// // //
{
	for (int i = 0, n = Chunk.GetLength(); i < n; ++i) {
		if (Chunk.GetType() == CHUNK_PATTERN)
			Store(Chunk.GetStringData(CCompiler::PATTERN_CHUNK_INDEX));
		else {
			unsigned short data = Chunk.GetData(i);
			unsigned short size = Chunk.GetDataSize(i);
			Store({reinterpret_cast<const std::uint8_t *>(&data), size});
		}
	}
}

void CChunkRenderBinary::StoreSample(const ft0cc::doc::dpcm_sample &DSample)
{
	unsigned int SampleSize = DSample.size();

	Store(DSample);
	m_iSampleAddress += SampleSize;

	// Adjust size
	if ((m_iSampleAddress & 0x3F) > 0) {
		int PadSize = 0x40 - (m_iSampleAddress & 0x3F);
		m_iSampleAddress += PadSize;
		Fill(PadSize);
	}
}


/**
 * NSF chunk render, used to write NSF files
 *
 */

CChunkRenderNSF::CChunkRenderNSF(CSimpleFile &File, unsigned int StartAddr) :
	CBinaryFileWriter(File),
	m_iStartAddr(StartAddr),
	m_iSampleAddr(0)
{
}

void CChunkRenderNSF::StoreDriver(array_view<std::uint8_t> Driver)		// // //
{
	// Store NSF driver
	Store(Driver);
}

void CChunkRenderNSF::StoreChunks(const std::vector<std::shared_ptr<CChunk>> &Chunks)		// // //
{
	// Store chunks into NSF banks
	for (const auto &ptr : Chunks)		// // //
		StoreChunk(*ptr);
}

void CChunkRenderNSF::StoreChunksBankswitched(const std::vector<std::shared_ptr<CChunk>> &Chunks)		// // //
{
	// Store chunks into NSF banks with bankswitching
	for (const auto &ptr : Chunks)		// // //
		StoreChunkBankswitched(*ptr);
}

void CChunkRenderNSF::StoreSamples(const std::vector<std::shared_ptr<const ft0cc::doc::dpcm_sample>> &Samples)
{
	// Align samples to $C000
	while (GetAbsoluteAddr() < CCompiler::PAGE_SAMPLES)
		AllocateNewBank();

	// Align first sample to valid address
	Fill(CCompiler::AdjustSampleAddress(GetAbsoluteAddr()));
	for (auto ptr : Samples)		// // //
		StoreSample(*ptr);
}

void CChunkRenderNSF::StoreSamplesBankswitched(const std::vector<std::shared_ptr<const ft0cc::doc::dpcm_sample>> &Samples)
{
	// Start samples on a clean bank
	if ((GetAbsoluteAddr() & 0xFFF) != 0)
		AllocateNewBank();

	m_iSampleAddr = CCompiler::PAGE_SAMPLES;
	for (auto ptr : Samples)		// // //
		StoreSampleBankswitched(*ptr);
}

void CChunkRenderNSF::StoreSample(const ft0cc::doc::dpcm_sample &DSample)
{
	// Store sample and fill with zeros
	Store(DSample);
	Fill(CCompiler::AdjustSampleAddress(GetAbsoluteAddr()));
}

void CChunkRenderNSF::StoreSampleBankswitched(const ft0cc::doc::dpcm_sample &DSample)
{
	unsigned int SampleSize = DSample.size();

	if (m_iSampleAddr + SampleSize >= (unsigned)CCompiler::DPCM_SWITCH_ADDRESS) {
		// Allocate new bank
		if (GetRemainingSize() != 0x1000)	// Skip if already on beginning of new bank
			AllocateNewBank();
		m_iSampleAddr = CCompiler::PAGE_SAMPLES;
	}

	int Adjust = CCompiler::AdjustSampleAddress(m_iSampleAddr + SampleSize);
	Store(DSample);
	Fill(Adjust);
	m_iSampleAddr += SampleSize + Adjust;
}

int CChunkRenderNSF::GetBankCount() const
{
	return GetBank() + 1;
}

void CChunkRenderNSF::StoreChunkBankswitched(const CChunk &Chunk)		// // //
{
	switch (Chunk.GetType()) {
		case CHUNK_FRAME_LIST:
		case CHUNK_FRAME:
		case CHUNK_PATTERN:
			// Switchable data
			while ((GetBank() + 1) <= Chunk.GetBank() && Chunk.GetBank() > CCompiler::PATTERN_SWITCH_BANK)
				AllocateNewBank();
	}

	// Write chunk
	StoreChunk(Chunk);
}

void CChunkRenderNSF::StoreChunk(const CChunk &Chunk)		// // //
{
	for (int i = 0, n = Chunk.GetLength(); i < n; ++i) {
		if (Chunk.GetType() == CHUNK_PATTERN)
			Store(Chunk.GetStringData(CCompiler::PATTERN_CHUNK_INDEX));
		else {
			unsigned short data = Chunk.GetData(i);
			unsigned short size = Chunk.GetDataSize(i);
			Store({reinterpret_cast<const std::uint8_t *>(&data), size});
		}
	}
}

int CChunkRenderNSF::GetRemainingSize() const
{
	// Return remaining bank size
	return 0x1000 - (GetWritten() & 0xFFF);
}

void CChunkRenderNSF::AllocateNewBank()
{
	// Get new NSF bank
	int Remaining = GetRemainingSize();
	Fill(Remaining);
}

int CChunkRenderNSF::GetBank() const
{
	return GetWritten() >> 12;
}

int CChunkRenderNSF::GetAbsoluteAddr() const
{
	// Return NSF address
	return m_iStartAddr + GetWritten();
}


/**
 * NES chunk render, borrows from NSF render
 *
 */

CChunkRenderNES::CChunkRenderNES(CSimpleFile &File, unsigned int StartAddr) : CChunkRenderNSF(File, StartAddr)
{
}

void CChunkRenderNES::StoreCaller(array_view<std::uint8_t> Data) {		// // //
	while (GetBank() < 7)
		AllocateNewBank();

	int FillSize = (0x10000 - GetAbsoluteAddr()) - Data.size();
	Fill(FillSize);
	Store(Data);
}
