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

#include "stdafx.h"
#include "ChunkRenderText.h"
#include "ft0cc/doc/dpcm_sample.hpp"		// // //
#include "FamiTrackerDoc.h"		// // // output title
#include "Compiler.h"
#include "Chunk.h"
#include "NumConv.h"		// // //
#include "Instrument.h"		// // //
#include "str_conv/str_conv.hpp"		// // //

/**
 * Text chunk render, these methods will always output single byte strings
 *
 */

static const int DEFAULT_LINE_BREAK = 20;

// // // TODO: remove
CStringA CChunkRenderText::GetLabelString(const stChunkLabel &label) {
	const auto APPEND0 = [&] (std::string_view text) -> CStringA {
		return text.data();
	};
	const auto APPEND1 = [&] (std::string_view text) -> CStringA {
		return FormattedA(text.data(), label.Param1);
	};
	const auto APPEND2 = [&] (std::string_view text) -> CStringA {
		return FormattedA(text.data(), label.Param1, label.Param2);
	};
	const auto APPEND3 = [&] (std::string_view text) -> CStringA {
		return FormattedA(text.data(), label.Param1, label.Param2, label.Param3);
	};

	switch (label.Type) {
	case CHUNK_NONE:            break;
	case CHUNK_HEADER:          break;
	case CHUNK_SEQUENCE:
		switch (label.Param2) {
		case INST_2A03: return APPEND1("ft_seq_2a03_%u");
		case INST_VRC6: return APPEND1("ft_seq_vrc6_%u");
		case INST_FDS:  return APPEND1("ft_seq_fds_%u");
		case INST_N163: return APPEND1("ft_seq_n163_%u");
		case INST_S5B:  return APPEND1("ft_seq_s5b_%u");
		}
		break;
	case CHUNK_INSTRUMENT_LIST: return APPEND0("ft_instrument_list");
	case CHUNK_INSTRUMENT:      return APPEND1("ft_inst_%i");
	case CHUNK_SAMPLE_LIST:     return APPEND0("ft_sample_list");
	case CHUNK_SAMPLE_POINTERS: return APPEND0("ft_sample_%i");
//	case CHUNK_SAMPLE:          return APPEND1("ft_sample_%i");
	case CHUNK_GROOVE_LIST:     return APPEND0("ft_groove_list");
	case CHUNK_GROOVE:          return APPEND1("ft_groove_%i");
	case CHUNK_SONG_LIST:       return APPEND0("ft_song_list");
	case CHUNK_SONG:            return APPEND1("ft_song_%i");
	case CHUNK_FRAME_LIST:      return APPEND1("ft_s%i_frames");
	case CHUNK_FRAME:           return APPEND2("ft_s%if%i");
	case CHUNK_PATTERN:         return APPEND3("ft_s%ip%ic%i");
	case CHUNK_WAVETABLE:       return APPEND0("ft_wave_table");
	case CHUNK_WAVES:           return APPEND1("ft_waves_%i");
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

CChunkRenderText::CChunkRenderText(CFile &File) : m_File(File)
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
	WriteFileString(conv::to_utf8(CFamiTrackerDoc::GetDoc()->GetTitle()));
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
		const unsigned char *pData = pDSample->data();

		CStringA label = FormattedA("ft_sample_%i", i);		// // //
		CStringA str = FormattedA("%s: ; %.*s\n", (LPCSTR)label, pDSample->name().size(), pDSample->name().data());
		str += GetByteString(*pDSample, DEFAULT_LINE_BREAK).data();
		Address += SampleSize;

		// Adjust if necessary
		if ((Address & 0x3F) > 0) {
			int PadSize = 0x40 - (Address & 0x3F);
			Address	+= PadSize;
			str.Append("\n\t.align 64\n");
		}

		str.Append("\n");
		WriteFileString((LPCSTR)str);
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
	CStringA str;
	int len = pChunk->GetLength();
	int i = 0;

	AppendFormatA(str, "\t.word %s\n", GetLabelString(pChunk->GetDataPointerTarget(i++)));		// // //
	AppendFormatA(str, "\t.word %s\n", GetLabelString(pChunk->GetDataPointerTarget(i++)));
	AppendFormatA(str, "\t.word %s\n", GetLabelString(pChunk->GetDataPointerTarget(i++)));
	AppendFormatA(str, "\t.word %s\n", GetLabelString(pChunk->GetDataPointerTarget(i++)));
	AppendFormatA(str, "\t.word %s\n", GetLabelString(pChunk->GetDataPointerTarget(i++)));		// // // groove
	AppendFormatA(str, "\t.byte %i ; flags\n", pChunk->GetData(i++));
	if (pChunk->IsDataPointer(i))
		AppendFormatA(str, "\t.word %s\n", GetLabelString(pChunk->GetDataPointerTarget(i++)));	// FDS waves
	AppendFormatA(str, "\t.word %i ; NTSC speed\n", pChunk->GetData(i++));
	AppendFormatA(str, "\t.word %i ; PAL speed\n", pChunk->GetData(i++));
	if (i < pChunk->GetLength())
		AppendFormatA(str, "\t.word %i ; N163 channels\n", pChunk->GetData(i++));	// N163 channels

	m_headerStrings.push_back((LPCSTR)str);
}

void CChunkRenderText::StoreInstrumentListChunk(const CChunk *pChunk)
{
	// Store instrument pointers
	CStringA str = FormattedA("%s:\n", GetLabelString(pChunk->GetLabel()));

	for (int i = 0; i < pChunk->GetLength(); ++i) {
		AppendFormatA(str, "\t.word %s\n", GetLabelString(pChunk->GetDataPointerTarget(i)));
	}

	m_instrumentListStrings.push_back((LPCSTR)str);
}

void CChunkRenderText::StoreInstrumentChunk(const CChunk *pChunk)
{
	int len = pChunk->GetLength();

	CStringA str = FormattedA("%s:\n\t.byte %i\n", GetLabelString(pChunk->GetLabel()), pChunk->GetData(0));

	for (int i = 1; i < len; ++i) {
		if (pChunk->IsDataPointer(i)) {
			AppendFormatA(str, "\t.word %s\n", GetLabelString(pChunk->GetDataPointerTarget(i)));
		}
		else {
			if (pChunk->GetDataSize(i) == 1) {
				AppendFormatA(str, "\t.byte $%02X\n", pChunk->GetData(i));
			}
			else {
				AppendFormatA(str, "\t.word $%04X\n", pChunk->GetData(i));
			}
		}
	}

	str.Append("\n");

	m_instrumentStrings.push_back((LPCSTR)str);
}

void CChunkRenderText::StoreSequenceChunk(const CChunk *pChunk)
{
	CStringA str = FormattedA("%s:\n", GetLabelString(pChunk->GetLabel()));
	str.Append(GetByteString(pChunk, DEFAULT_LINE_BREAK).data());		// // //

	m_sequenceStrings.push_back((LPCSTR)str);
}

void CChunkRenderText::StoreSampleListChunk(const CChunk *pChunk)
{
	// Store sample list
	CStringA str = FormattedA("%s:\n", GetLabelString(pChunk->GetLabel()));

	for (int i = 0; i < pChunk->GetLength(); i += 3) {
		AppendFormatA(str, "\t.byte %i, %i, %i\n", pChunk->GetData(i + 0), pChunk->GetData(i + 1), pChunk->GetData(i + 2));
	}

	m_sampleListStrings.push_back((LPCSTR)str);
}

void CChunkRenderText::StoreSamplePointersChunk(const CChunk *pChunk)
{
	int len = pChunk->GetLength();

	// Store sample pointer
	CStringA str = FormattedA("%s:\n", GetLabelString(pChunk->GetLabel()));

	if (len > 0) {
		str.Append("\t.byte ");

		int len = pChunk->GetLength();
		for (int i = 0; i < len; ++i) {
			AppendFormatA(str, "%i%s", pChunk->GetData(i), (i < len - 1) && (i % 3 != 2) ? ", " : "");
			if (i % 3 == 2 && i < (len - 1))
				str.Append("\n\t.byte ");
		}
	}

	str.Append("\n");

	m_samplePointersStrings.push_back((LPCSTR)str);
}

void CChunkRenderText::StoreGrooveListChunk(const CChunk *pChunk)		// // //
{
	CStringA str = FormattedA("%s:\n", GetLabelString(pChunk->GetLabel()));

	for (int i = 0; i < pChunk->GetLength(); ++i) {
		AppendFormatA(str, "\t.byte $%02X\n", pChunk->GetData(i));
	}

	m_grooveListStrings.push_back((LPCSTR)str);
}

void CChunkRenderText::StoreGrooveChunk(const CChunk *pChunk)		// // //
{
	CStringA str;

	// CStringA str = FormattedA("%s:\n", GetLabelString(pChunk->GetLabel()));
	str.Append(GetByteString(pChunk, DEFAULT_LINE_BREAK).data());

	m_grooveStrings.push_back((LPCSTR)str);
}

void CChunkRenderText::StoreSongListChunk(const CChunk *pChunk)
{
	CStringA str = FormattedA("%s:\n", GetLabelString(pChunk->GetLabel()));

	for (int i = 0; i < pChunk->GetLength(); ++i) {
		AppendFormatA(str, "\t.word %s\n", GetLabelString(pChunk->GetDataPointerTarget(i)));
	}

	m_songListStrings.push_back((LPCSTR)str);
}

void CChunkRenderText::StoreSongChunk(const CChunk *pChunk)
{
	CStringA str = FormattedA("%s:\n", GetLabelString(pChunk->GetLabel()));

	for (int i = 0; i < pChunk->GetLength();) {
		AppendFormatA(str, "\t.word %s\n", GetLabelString(pChunk->GetDataPointerTarget(i++)));
		AppendFormatA(str, "\t.byte %i\t; frame count\n", pChunk->GetData(i++));
		AppendFormatA(str, "\t.byte %i\t; pattern length\n", pChunk->GetData(i++));
		AppendFormatA(str, "\t.byte %i\t; speed\n", pChunk->GetData(i++));
		AppendFormatA(str, "\t.byte %i\t; tempo\n", pChunk->GetData(i++));
		AppendFormatA(str, "\t.byte %i\t; groove position\n", pChunk->GetData(i++));		// // //
		AppendFormatA(str, "\t.byte %i\t; initial bank\n", pChunk->GetData(i++));
	}

	str.Append("\n");

	m_songStrings.push_back((LPCSTR)str);
}

void CChunkRenderText::StoreFrameListChunk(const CChunk *pChunk)
{
	// Pointers to frames
	CStringA str = FormattedA("; Bank %i\n", pChunk->GetBank());
	AppendFormatA(str, "%s:\n", GetLabelString(pChunk->GetLabel()));

	for (int i = 0; i < pChunk->GetLength(); ++i) {
		AppendFormatA(str, "\t.word %s\n", GetLabelString(pChunk->GetDataPointerTarget(i)));
	}

	m_songDataStrings.push_back((LPCSTR)str);
}

void CChunkRenderText::StoreFrameChunk(const CChunk *pChunk)
{
	int len = pChunk->GetLength();

	// Frame list
	CStringA str = FormattedA("%s:\n\t.word ", GetLabelString(pChunk->GetLabel()));

	for (int i = 0, j = 0; i < len; ++i) {
		if (pChunk->IsDataPointer(i))
			AppendFormatA(str, "%s%s", (j++ > 0) ? ", " : "", GetLabelString(pChunk->GetDataPointerTarget(i)));
	}

	// Bank values
	for (int i = 0, j = 0; i < len; ++i) {
		if (pChunk->IsDataBank(i)) {
			if (j == 0) {
				AppendFormatA(str, "\n\t.byte ", GetLabelString(pChunk->GetLabel()));
			}
			AppendFormatA(str, "%s$%02X", (j++ > 0) ? ", " : "", pChunk->GetData(i));
		}
	}

	str.Append("\n");

	m_songDataStrings.push_back((LPCSTR)str);
}

void CChunkRenderText::StorePatternChunk(const CChunk *pChunk)
{
	// Patterns
	CStringA str = FormattedA("; Bank %i\n", pChunk->GetBank());
	AppendFormatA(str, "%s:\n", GetLabelString(pChunk->GetLabel()));

	const std::vector<unsigned char> &vec = pChunk->GetStringData(0);		// // //
	str += GetByteString(vec, DEFAULT_LINE_BREAK).data();
/*
	len = vec.size();
	for (int i = 0; i < len; ++i) {
		AppendFormatA(str, "$%02X", (unsigned char)vec[i]);
		if ((i % 20 == 19) && (i < len - 1))
			str.Append("\n\t.byte ");
		else {
			if (i < len - 1)
				str.Append(", ");
		}
	}
*/
	str.Append("\n");

	m_songDataStrings.push_back((LPCSTR)str);
}

void CChunkRenderText::StoreWavetableChunk(const CChunk *pChunk)
{
	int len = pChunk->GetLength();

	// FDS waves
	CStringA str = FormattedA("%s:\n", GetLabelString(pChunk->GetLabel()));
	str.Append("\t.byte ");

	for (int i = 0; i < len; ++i) {
		AppendFormatA(str, "$%02X", pChunk->GetData(i));
		if ((i % 64 == 63) && (i < len - 1))
			str.Append("\n\t.byte ");
		else {
			if (i < len - 1)
				str.Append(", ");
		}
	}

	str.Append("\n");

	m_wavetableStrings.push_back((LPCSTR)str);
}

void CChunkRenderText::StoreWavesChunk(const CChunk *pChunk)
{
	int len = pChunk->GetLength();

//				int waves = pChunk->GetData(0);
	int wave_len = 16;//(len - 1) / waves;

	// Namco waves
	CStringA str = FormattedA("%s:\n", GetLabelString(pChunk->GetLabel()));
//				AppendFormatA(str, "\t.byte %i\n", waves);

	str.Append("\t.byte ");

	for (int i = 0; i < len; ++i) {
		AppendFormatA(str, "$%02X", pChunk->GetData(i));
		if ((i % wave_len == (wave_len - 1)) && (i < len - 1))
			str.Append("\n\t.byte ");
		else {
			if (i < len - 1)
				str.Append(", ");
		}
	}

	str.Append("\n");

	m_wavesStrings.push_back((LPCSTR)str);
}

void CChunkRenderText::WriteFileString(std::string_view sv) const		// // //
{
	m_File.Write(sv.data(), sv.size());		// // //
}

std::string CChunkRenderText::GetByteString(array_view<unsigned char> Data, int LineBreak) {		// // //
	std::string str = "\t.byte ";

	int i = 0;
	while (!Data.empty()) {
		str += '$';
		str += conv::sv_from_uint_hex(Data.front(), 2);
		Data.pop_front();

		if (!Data.empty())
			if ((i % LineBreak == (LineBreak - 1)))
				str += "\n\t.byte ";
			else
				str += ", ";

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
