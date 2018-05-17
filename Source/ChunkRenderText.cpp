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

#include "ChunkRenderText.h"
#include "SimpleFile.h"		// // //
#include "ft0cc/doc/dpcm_sample.hpp"		// // //
#include "FamiTrackerEnv.h"		// // // output title
#include "Compiler.h"
#include "Chunk.h"
#include "NumConv.h"		// // //
#include "Instrument.h"		// // //
#include "str_conv/str_conv.hpp"		// // //

/**
 * Text chunk render, these methods will always output single byte strings
 *
 */

namespace {

const int DEFAULT_LINE_BREAK = 20;

} // namespace

// // // TODO: remove
std::string CChunkRenderText::GetLabelString(const stChunkLabel &label) {
	const auto p1 = [&] { return conv::from_uint(label.Param1); };
	const auto p2 = [&] { return conv::from_uint(label.Param2); };
	const auto p3 = [&] { return conv::from_uint(label.Param3); };

	switch (label.Type) {
	case CHUNK_NONE:            break;
	case CHUNK_HEADER:          break;
	case CHUNK_SEQUENCE:
		switch (label.Param2) {
		case INST_2A03: return "ft_seq_2a03_" + p1();
		case INST_VRC6: return "ft_seq_vrc6_" + p1();
		case INST_FDS:  return "ft_seq_fds_" + p1();
		case INST_N163: return "ft_seq_n163_" + p1();
		case INST_S5B:  return "ft_seq_s5b_" + p1();
		}
		break;
	case CHUNK_INSTRUMENT_LIST: return "ft_instrument_list";
	case CHUNK_INSTRUMENT:      return "ft_inst_" + p1();
	case CHUNK_SAMPLE_LIST:     return "ft_sample_list";
	case CHUNK_SAMPLE_POINTERS: return "ft_sample_" + p1();
//	case CHUNK_SAMPLE:          return "ft_sample_" + p1();
	case CHUNK_GROOVE_LIST:     return "ft_groove_list";
	case CHUNK_GROOVE:          return "ft_groove_" + p1();
	case CHUNK_SONG_LIST:       return "ft_song_list";
	case CHUNK_SONG:            return "ft_song_" + p1();
	case CHUNK_FRAME_LIST:      return "ft_s" + p1() + "_frames";
	case CHUNK_FRAME:           return "ft_s" + p1() + "f" + p2();
	case CHUNK_PATTERN:         return "ft_s" + p1() + "p" + p2() + "c" + p3();
	case CHUNK_WAVETABLE:       return "ft_wave_table";
	case CHUNK_WAVES:           return "ft_waves_" + p1();
	case CHUNK_CHANNEL_MAP:     break;
	case CHUNK_CHANNEL_TYPES:   break;
	}
	return "";
}

// String render functions
const CChunkRenderText::stChunkRenderFunc CChunkRenderText::RENDER_FUNCTIONS[] = {
	{CHUNK_HEADER,			&CChunkRenderText::StoreHeaderChunk},
	{CHUNK_SEQUENCE,		&CChunkRenderText::StoreSequenceChunk},
	{CHUNK_INSTRUMENT_LIST,	&CChunkRenderText::StoreInstrumentListChunk},
	{CHUNK_INSTRUMENT,		&CChunkRenderText::StoreInstrumentChunk},
	{CHUNK_SAMPLE_LIST,		&CChunkRenderText::StoreSampleListChunk},
	{CHUNK_SAMPLE_POINTERS,	&CChunkRenderText::StoreSamplePointersChunk},
	{CHUNK_GROOVE_LIST,		&CChunkRenderText::StoreGrooveListChunk},		// // //
	{CHUNK_GROOVE,			&CChunkRenderText::StoreGrooveChunk},		// // //
	{CHUNK_SONG_LIST,		&CChunkRenderText::StoreSongListChunk},
	{CHUNK_SONG,			&CChunkRenderText::StoreSongChunk},
	{CHUNK_FRAME_LIST,		&CChunkRenderText::StoreFrameListChunk},
	{CHUNK_FRAME,			&CChunkRenderText::StoreFrameChunk},
	{CHUNK_PATTERN,			&CChunkRenderText::StorePatternChunk},
	{CHUNK_WAVETABLE,		&CChunkRenderText::StoreWavetableChunk},
	{CHUNK_WAVES,			&CChunkRenderText::StoreWavesChunk},
};

CChunkRenderText::CChunkRenderText(CSimpleFile &File) : m_File(File)
{
}

void CChunkRenderText::StoreChunks(const std::vector<std::shared_ptr<CChunk>> &Chunks)		// // //
{
	// Generate strings
	for (auto &pChunk : Chunks)
		for (const auto &f : RENDER_FUNCTIONS)
			if (pChunk->GetType() == f.type)
				(this->*(f.function))(pChunk.get());

	// Write strings to file
	WriteFileString("; 0CC-FamiTracker exported music data: ");
	WriteFileString(FTEnv.GetDocumentTitle());		// // //
	WriteFileString("\n;\n\n");

	// Module header
	DumpStrings("; Module header\n", "\n", m_headerStrings);

	// Instrument list
	DumpStrings("; Instrument pointer list\n", "\n", m_instrumentListStrings);
	DumpStrings("; Instruments\n", "", m_instrumentStrings);

	// Sequences
	DumpStrings("; Sequences\n", "\n", m_sequenceStrings);

	// Waves (FDS & N163)
	if (!m_wavetableStrings.empty())
		DumpStrings("; FDS waves\n", "\n", m_wavetableStrings);

	if (!m_wavesStrings.empty())
		DumpStrings("; N163 waves\n", "\n", m_wavesStrings);

	// Samples
	DumpStrings("; DPCM instrument list (pitch, sample index)\n", "\n", m_sampleListStrings);
	DumpStrings("; DPCM samples list (location, size, bank)\n", "\n", m_samplePointersStrings);

	// // // Grooves
	DumpStrings("; Groove list\n", "", m_grooveListStrings);
	DumpStrings("; Grooves (size, terms)\n", "\n", m_grooveStrings);

	// Songs
	DumpStrings("; Song pointer list\n", "\n", m_songListStrings);
	DumpStrings("; Song info\n", "\n", m_songStrings);

	// Song data
	DumpStrings(";\n; Pattern and frame data for all songs below\n;\n\n", "", m_songDataStrings);

	// Actual DPCM samples are stored later
}

void CChunkRenderText::StoreSamples(const std::vector<std::shared_ptr<const ft0cc::doc::dpcm_sample>> &Samples)		// // //
{
	// Store DPCM samples in file, assembly format
	WriteFileString("\n; DPCM samples (located at DPCM segment)\n");

	if (!Samples.empty())
		WriteFileString("\n\t.segment \"DPCM\"\n");

	unsigned int Address = CCompiler::PAGE_SAMPLES;
	for (size_t i = 0; i < Samples.size(); ++i) if (const auto &pDSample = Samples[i]) {		// // //
		const unsigned int SampleSize = pDSample->size();

		std::string label = "ft_sample_" + conv::from_uint(i);		// // //
		std::string str = label + ": ; " + std::string {pDSample->name()} + "\n";
		str += GetByteString(*pDSample, DEFAULT_LINE_BREAK);
		Address += SampleSize;

		// Adjust if necessary
		if ((Address & 0x3F) > 0) {
			int PadSize = 0x40 - (Address & 0x3F);
			Address	+= PadSize;
			str += "\n\t.align 64\n";
		}

		str.push_back('\n');
		WriteFileString(str);
	}
}

void CChunkRenderText::DumpStrings(std::string_view preStr, std::string_view postStr, const std::vector<std::string> &stringArray) const		// // //
{
	WriteFileString(preStr);

	for (auto &str : stringArray)
		WriteFileString(str);

	WriteFileString(postStr);
}

void CChunkRenderText::StoreHeaderChunk(const CChunk *pChunk)
{
	std::string str;
	int i = 0;

	str += "\t.word " + GetLabelString(pChunk->GetDataPointerTarget(i++)) + "\n";		// // //
	str += "\t.word " + GetLabelString(pChunk->GetDataPointerTarget(i++)) + "\n";
	str += "\t.word " + GetLabelString(pChunk->GetDataPointerTarget(i++)) + "\n";
	str += "\t.word " + GetLabelString(pChunk->GetDataPointerTarget(i++)) + "\n";
	str += "\t.word " + GetLabelString(pChunk->GetDataPointerTarget(i++)) + "\n";		// // //		groove
	str += "\t.byte %i ; flags\n", pChunk->GetData(i++);
	if (pChunk->IsDataPointer(i))
		str += "\t.word " + GetLabelString(pChunk->GetDataPointerTarget(i++)) + "\n";	// FDS waves
	str += "\t.word %i ; NTSC speed\n", pChunk->GetData(i++);
	str += "\t.word %i ; PAL speed\n", pChunk->GetData(i++);
	if (i < pChunk->GetLength())
		str += "\t.word %i ; N163 channels\n", pChunk->GetData(i++);	// N163 channels

	m_headerStrings.push_back(std::move(str));
}

void CChunkRenderText::StoreInstrumentListChunk(const CChunk *pChunk)
{
	// Store instrument pointers
	std::string str = GetLabelString(pChunk->GetLabel()) + ":\n";

	for (int i = 0; i < pChunk->GetLength(); ++i)
		str += "\t.word " + GetLabelString(pChunk->GetDataPointerTarget(i)) + "\n";

	m_instrumentListStrings.push_back(std::move(str));
}

void CChunkRenderText::StoreInstrumentChunk(const CChunk *pChunk)
{
	int len = pChunk->GetLength();

	std::string str = GetLabelString(pChunk->GetLabel()) + ":\n\t.byte " + conv::from_uint(pChunk->GetData(0)) + "\n";

	for (int i = 1; i < len; ++i) {
		if (pChunk->IsDataPointer(i))
			str += "\t.word " + GetLabelString(pChunk->GetDataPointerTarget(i)) + "\n";
		else if (pChunk->GetDataSize(i) == 1)
			str += "\t.byte $" + conv::from_uint_hex(pChunk->GetData(i), 2) + "\n";
		else
			str += "\t.word $" + conv::from_uint_hex(pChunk->GetData(i), 4) + "\n";
	}

	str.push_back('\n');

	m_instrumentStrings.push_back(std::move(str));
}

void CChunkRenderText::StoreSequenceChunk(const CChunk *pChunk)
{
	std::string str = GetLabelString(pChunk->GetLabel()) + ":\n";
	str += GetByteString(pChunk, DEFAULT_LINE_BREAK);		// // //

	m_sequenceStrings.push_back(std::move(str));
}

void CChunkRenderText::StoreSampleListChunk(const CChunk *pChunk)
{
	// Store sample list
	std::string str = GetLabelString(pChunk->GetLabel()) + ":\n";

	for (int i = 0; i < pChunk->GetLength(); i += 3)
		str += "\t.byte " + conv::from_uint(pChunk->GetData(i + 0)) + ", " +
			conv::from_uint(pChunk->GetData(i + 1)) + ", " +
			conv::from_uint(pChunk->GetData(i + 2)) + "\n";

	m_sampleListStrings.push_back(std::move(str));
}

void CChunkRenderText::StoreSamplePointersChunk(const CChunk *pChunk)
{
	int len = pChunk->GetLength();

	// Store sample pointer
	std::string str = GetLabelString(pChunk->GetLabel()) + ":\n";

	if (len > 0) {
		str += "\t.byte ";

		for (int i = 0; i < len; ++i) {
			str += conv::from_uint(pChunk->GetData(i));
			if ((i < len - 1) && (i % 3 != 2))
				str += ", ";
			if (i % 3 == 2 && i < (len - 1))
				str += "\n\t.byte ";
		}
	}

	str.push_back('\n');

	m_samplePointersStrings.push_back(std::move(str));
}

void CChunkRenderText::StoreGrooveListChunk(const CChunk *pChunk)		// // //
{
	std::string str = GetLabelString(pChunk->GetLabel()) + ":\n";

	for (int i = 0; i < pChunk->GetLength(); ++i)
		str += "\t.byte $" + conv::from_uint_hex(pChunk->GetData(i), 2) + "\n";

	m_grooveListStrings.push_back(std::move(str));
}

void CChunkRenderText::StoreGrooveChunk(const CChunk *pChunk)		// // //
{
	std::string str;

	// std::string str = GetLabelString(pChunk->GetLabel()) + ":\n";
	str += GetByteString(pChunk, DEFAULT_LINE_BREAK);

	m_grooveStrings.push_back(std::move(str));
}

void CChunkRenderText::StoreSongListChunk(const CChunk *pChunk)
{
	std::string str = GetLabelString(pChunk->GetLabel()) + ":\n";

	for (int i = 0; i < pChunk->GetLength(); ++i) {
		str += "\t.word " + GetLabelString(pChunk->GetDataPointerTarget(i)) + "\n";
	}

	m_songListStrings.push_back(std::move(str));
}

void CChunkRenderText::StoreSongChunk(const CChunk *pChunk)
{
	std::string str = GetLabelString(pChunk->GetLabel()) + ":\n";

	for (int i = 0; i < pChunk->GetLength();) {
		str += "\t.word " + GetLabelString(pChunk->GetDataPointerTarget(i++)) + "\n";
		str += "\t.byte " + conv::from_uint(pChunk->GetData(i++)) + "\t; frame count\n";
		str += "\t.byte " + conv::from_uint(pChunk->GetData(i++)) + "\t; pattern length\n";
		str += "\t.byte " + conv::from_uint(pChunk->GetData(i++)) + "\t; speed\n";
		str += "\t.byte " + conv::from_uint(pChunk->GetData(i++)) + "\t; tempo\n";
		str += "\t.byte " + conv::from_uint(pChunk->GetData(i++)) + "\t; groove position\n";		// // //
		str += "\t.byte " + conv::from_uint(pChunk->GetData(i++)) + "\t; initial bank\n";
	}

	str.push_back('\n');

	m_songStrings.push_back(std::move(str));
}

void CChunkRenderText::StoreFrameListChunk(const CChunk *pChunk)
{
	// Pointers to frames
	std::string str = "; Bank " + conv::from_uint(pChunk->GetBank()) + "\n";
	str += GetLabelString(pChunk->GetLabel()) + ":\n";

	for (int i = 0; i < pChunk->GetLength(); ++i) {
		str += "\t.word " + GetLabelString(pChunk->GetDataPointerTarget(i)) + "\n";
	}

	m_songDataStrings.push_back(std::move(str));
}

void CChunkRenderText::StoreFrameChunk(const CChunk *pChunk)
{
	int len = pChunk->GetLength();

	// Frame list
	std::string str = GetLabelString(pChunk->GetLabel()) + ":\n\t.word ";

	for (int i = 0, j = 0; i < len; ++i) {
		if (pChunk->IsDataPointer(i)) {
			if (j++ > 0)
				str += ", ";
			str += GetLabelString(pChunk->GetDataPointerTarget(i));
		}
	}

	// Bank values
	for (int i = 0, j = 0; i < len; ++i) {
		if (pChunk->IsDataBank(i)) {
			if (j == 0)
				str += "\n\t.byte ";
			if (j++ > 0)
				str += ", ";
			str += "$" + conv::from_uint_hex(pChunk->GetData(i), 2);
		}
	}

	str.push_back('\n');

	m_songDataStrings.push_back(std::move(str));
}

void CChunkRenderText::StorePatternChunk(const CChunk *pChunk)
{
	// Patterns
	std::string str = "; Bank " + conv::from_uint(pChunk->GetBank()) + "\n";
	str += GetLabelString(pChunk->GetLabel()) + ":\n";

	const std::vector<unsigned char> &vec = pChunk->GetStringData(0);		// // //
	str += GetByteString(vec, DEFAULT_LINE_BREAK).data();
/*
	len = vec.size();
	for (int i = 0; i < len; ++i) {
		str += "$%02X", (unsigned char)vec[i]);
		if ((i % 20 == 19) && (i < len - 1))
			str += "\n\t.byte ";
		else {
			if (i < len - 1)
				str += ", ";
		}
	}
*/
	str.push_back('\n');

	m_songDataStrings.push_back(std::move(str));
}

void CChunkRenderText::StoreWavetableChunk(const CChunk *pChunk)
{
	int len = pChunk->GetLength();

	// FDS waves
	std::string str = GetLabelString(pChunk->GetLabel()) + ":\n";
	str += "\t.byte ";

	for (int i = 0; i < len; ++i) {
		str += "$" + conv::from_uint_hex(pChunk->GetData(i), 2);
		if ((i % 64 == 63) && (i < len - 1))
			str += "\n\t.byte ";
		else {
			if (i < len - 1)
				str += ", ";
		}
	}

	str.push_back('\n');

	m_wavetableStrings.push_back(std::move(str));
}

void CChunkRenderText::StoreWavesChunk(const CChunk *pChunk)
{
	int len = pChunk->GetLength();

//				int waves = pChunk->GetData(0);
	int wave_len = 16;//(len - 1) / waves;

	// Namco waves
	std::string str = GetLabelString(pChunk->GetLabel()) + ":\n";
//				str += "\t.byte %i\n", waves);

	str += "\t.byte ";

	for (int i = 0; i < len; ++i) {
		str += "$" + conv::from_uint_hex(pChunk->GetData(i), 2);
		if ((i % wave_len == (wave_len - 1)) && (i < len - 1))
			str += "\n\t.byte ";
		else {
			if (i < len - 1)
				str += ", ";
		}
	}

	str.push_back('\n');

	m_wavesStrings.push_back(std::move(str));
}

void CChunkRenderText::WriteFileString(std::string_view sv) const		// // //
{
	m_File.WriteBytes(sv);		// // //
}

std::string CChunkRenderText::GetByteString(array_view<unsigned char> Data, int LineBreak) {		// // //
	std::string str = "\t.byte ";

	int i = 0;
	while (!Data.empty()) {
		str += '$';
		str += conv::sv_from_uint_hex(Data.front(), 2);
		Data.pop_front();

		if (!Data.empty()) {
			if ((i % LineBreak == (LineBreak - 1)))
				str += "\n\t.byte ";
			else
				str += ", ";
		}

		++i;
	}

	str += '\n';
	return str;
}

std::string CChunkRenderText::GetByteString(const CChunk *pChunk, int LineBreak) {		// // //
	int len = pChunk->GetLength();

	std::string str = "\t.byte ";

	for (int i = 0; i < len; ++i) {
		str += '$';
		str += conv::sv_from_uint_hex(pChunk->GetData(i), 2);

		if ((i % LineBreak == (LineBreak - 1)) && (i < len - 1))
			str += "\n\t.byte ";
		else if (i < len - 1)
			str += ", ";
	}

	str += '\n';
	return str;
}
