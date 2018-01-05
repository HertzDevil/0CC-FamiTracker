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

#include <vector>
#include <memory>		// // //
#include <string>		// // //

//
// Text chunk renderer
//

class CFile;		// // //

class CChunkRenderText;
class CChunk;		// // //
namespace ft0cc::doc {
class dpcm_sample;
} // namespace ft0cc::doc
enum chunk_type_t : int;		// // //
struct stChunkLabel;		// // //

class CChunkRenderText
{
	using renderFunc_t = void (CChunkRenderText::*)(const CChunk *);
	struct stChunkRenderFunc {
		chunk_type_t type;
		renderFunc_t function;
	};

public:
	explicit CChunkRenderText(CFile &File);		// // //

	void StoreChunks(const std::vector<std::shared_ptr<CChunk>> &Chunks);		// // //
	void StoreSamples(const std::vector<std::shared_ptr<const ft0cc::doc::dpcm_sample>> &Samples);		// // //

private:
	static const stChunkRenderFunc RENDER_FUNCTIONS[];
	static std::string GetLabelString(const stChunkLabel &label);		// // //
	static std::string GetByteString(const unsigned char *pData, int Len, int LineBreak);		// // //
	static std::string GetByteString(const CChunk *pChunk, int LineBreak);		// // //

private:
	void DumpStrings(std::string_view preStr, std::string_view postStr, const std::vector<std::string> &stringArray) const;		// // //
	void WriteFileString(std::string_view sv) const;		// // //

private:
	void StoreHeaderChunk(const CChunk *pChunk);
	void StoreInstrumentListChunk(const CChunk *pChunk);
	void StoreInstrumentChunk(const CChunk *pChunk);
	void StoreSequenceChunk(const CChunk *pChunk);
	void StoreSampleListChunk(const CChunk *pChunk);
	void StoreSamplePointersChunk(const CChunk *pChunk);
	void StoreGrooveListChunk(const CChunk *pChunk);		// // //
	void StoreGrooveChunk(const CChunk *pChunk);		// // //
	void StoreSongListChunk(const CChunk *pChunk);
	void StoreSongChunk(const CChunk *pChunk);
	void StoreFrameListChunk(const CChunk *pChunk);
	void StoreFrameChunk(const CChunk *pChunk);
	void StorePatternChunk(const CChunk *pChunk);
	void StoreWavetableChunk(const CChunk *pChunk);
	void StoreWavesChunk(const CChunk *pChunk);

private:
	std::vector<std::string> m_headerStrings;		// // //
	std::vector<std::string> m_instrumentListStrings;
	std::vector<std::string> m_instrumentStrings;
	std::vector<std::string> m_sequenceStrings;
	std::vector<std::string> m_sampleListStrings;
	std::vector<std::string> m_samplePointersStrings;
	std::vector<std::string> m_grooveListStrings;		// // //
	std::vector<std::string> m_grooveStrings;		// // //
	std::vector<std::string> m_songListStrings;
	std::vector<std::string> m_songStrings;
	std::vector<std::string> m_songDataStrings;
	std::vector<std::string> m_wavetableStrings;
	std::vector<std::string> m_wavesStrings;

	CFile &m_File;
};
