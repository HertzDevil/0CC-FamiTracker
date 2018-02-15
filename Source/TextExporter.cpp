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

#include "TextExporter.h"
#include "FamiTrackerTypes.h"		// // //
#include "SongData.h"		// // //
#include "SoundGen.h"		// // //
#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"		// // //
#include "ChannelMap.h"		// // //
#include "version.h"		// // //
#include "FamiTrackerEnv.h"		// // //
#include "FamiTrackerViewMessage.h"		// // //
#include "str_conv/str_conv.hpp"		// // //

#include "ft0cc/doc/dpcm_sample.hpp"		// // //
#include "ft0cc/doc/groove.hpp"		// // //
#include "Sequence.h"		// // //
#include "InstrumentService.h"		// // //
#include "InstrumentManager.h"		// // //
#include "DSampleManager.h"		// // //
#include "Instrument2A03.h"		// // //
#include "InstrumentVRC7.h"		// // //
#include "InstrumentFDS.h"		// // //
#include "InstrumentN163.h"		// // //
#include "SoundChipSet.h"		// // //

#include <type_traits>		// // //

namespace {

template <typename T>
struct printf_arg : std::disjunction<
	std::is_scalar<std::decay_t<T>>,
	std::is_convertible<T, const char *>
> { };

template <typename... Args>		// // // TODO: put this into stdafx.h perhaps
CStringA FormattedA(const char *fmt, Args&&... args) {
	CStringA str;
	static_assert(std::conjunction_v<printf_arg<Args>...>,
		"Only scalar types allowed in printf");
	str.Format(fmt, std::forward<Args>(args)...);
	return str;
}

} // namespace

#define DEBUG_OUT(...) { OutputDebugString(FormattedA(__VA_ARGS__)); }

// command tokens
enum
{
	CT_COMMENTLINE,    // anything may follow
	// song info
	CT_TITLE,          // string
	CT_AUTHOR,         // string
	CT_COPYRIGHT,      // string
	CT_COMMENT,        // string (concatenates line)
	// global settings
	CT_MACHINE,        // uint (0=NTSC, 1=PAL)
	CT_FRAMERATE,      // uint (0=default)
	CT_EXPANSION,      // uint (0=none, 1=VRC6, 2=VRC7, 4=FDS, 8=MMC5, 16=N163, 32=S5B)
	CT_VIBRATO,        // uint (0=old, 1=new)
	CT_SPLIT,          // uint (32=default)
	// // // 050B
	CT_PLAYBACKRATE,   // uint (0=default, 1=custom, 2=video) uint (us)
	CT_TUNING,         // uint (semitones) uint (cents)
	// namco global settings
	CT_N163CHANNELS,   // uint
	// macros
	CT_MACRO,          // uint (type) uint (index) int (loop) int (release) int (setting) : int_list
	CT_MACROVRC6,      // uint (type) uint (index) int (loop) int (release) int (setting) : int_list
	CT_MACRON163,      // uint (type) uint (index) int (loop) int (release) int (setting) : int_list
	CT_MACROS5B,       // uint (type) uint (index) int (loop) int (release) int (setting) : int_list
	// dpcm samples
	CT_DPCMDEF,        // uint (index) uint (size) string (name)
	CT_DPCM,           // : hex_list
	// // // detune settings
	CT_DETUNE,         // uint (chip) uint (oct) uint (note) int (offset)
	// // // grooves
	CT_GROOVE,         // uint (index) uint (size) : int_list
	CT_USEGROOVE,      // : int_list
	// instruments
	CT_INST2A03,       // uint (index) int int int int int string (name)
	CT_INSTVRC6,       // uint (index) int int int int int string (name)
	CT_INSTVRC7,       // uint (index) int (patch) hex hex hex hex hex hex hex hex string (name)
	CT_INSTFDS,        // uint (index) int (mod enable) int (m speed) int (m depth) int (m delay) string (name)
	CT_INSTN163,       // uint (index) int int int int int uint (w size) uint (w pos) uint (w count) string (name)
	CT_INSTS5B,        // uint (index) int int int int int  string (name)
	// instrument data
	CT_KEYDPCM,        // uint (inst) uint (oct) uint (note) uint (sample) uint (pitch) uint (loop) uint (loop_point)
	CT_FDSWAVE,        // uint (inst) : uint_list x 64
	CT_FDSMOD,         // uint (inst) : uint_list x 32
	CT_FDSMACRO,       // uint (inst) uint (type) int (loop) int (release) int (setting) : int_list
	CT_N163WAVE,       // uint (inst) uint (wave) : uint_list
	// track info
	CT_TRACK,          // uint (pat length) uint (speed) uint (tempo) string (name)
	CT_COLUMNS,        // : uint_list (effect columns)
	CT_ORDER,          // hex (frame) : hex_list
	// pattern data
	CT_PATTERN,        // hex (pattern)
	CT_ROW,            // row data
	// end of command list
	CT_COUNT
};

static const char *CT[CT_COUNT] =
{
	// comment
	"#",
	// song info
	"TITLE",
	"AUTHOR",
	"COPYRIGHT",
	"COMMENT",
	// global settings
	"MACHINE",
	"FRAMERATE",
	"EXPANSION",
	"VIBRATO",
	"SPLIT",
	// // // 050B
	"PLAYBACKRATE",
	"TUNING",
	// namco global settings
	"N163CHANNELS",
	// macros
	"MACRO",
	"MACROVRC6",
	"MACRON163",
	"MACROS5B",
	// dpcm
	"DPCMDEF",
	"DPCM",
	// // // detune settings
	"DETUNE",
	// // // grooves
	"GROOVE",
	"USEGROOVE",
	// instruments
	"INST2A03",
	"INSTVRC6",
	"INSTVRC7",
	"INSTFDS",
	"INSTN163",
	"INSTS5B",
	// instrument data
	"KEYDPCM",
	"FDSWAVE",
	"FDSMOD",
	"FDSMACRO",
	"N163WAVE",
	// track info
	"TRACK",
	"COLUMNS",
	"ORDER",
	// pattern data
	"PATTERN",
	"ROW",
};

// =============================================================================

class Tokenizer
{
public:
	explicit Tokenizer(LPCWSTR FileName) : text(ReadFromFile(FileName)) { }
	~Tokenizer() = default;

	void Reset() {
		last_pos_ = pos = 0;
		line = 1;
		linestart = 0;
	}

	void FinishLine() {
		if (int newpos = text.Find(L'\n', pos); newpos >= 0) {		// // //
			++line;
			pos = newpos + 1;
		}
		else
			pos = text.GetLength();
		last_pos_ = linestart = pos;
	}

	int GetColumn() const {
		return 1 + pos - linestart;
	}

	bool Finished() const {
		return pos >= text.GetLength();
	}

	CStringA ReadToken() {
		ConsumeSpace();
		CStringA t = "";

		bool isQuoted = TrimChar('\"');		// // //
		while (!Finished())
			switch (char c = text.GetAt(pos)) {
			case '\r': case '\n':
				if (isQuoted)
					throw MakeError("incomplete quoted string.");
				goto outer;
			case '\"':
				if (!isQuoted)
					goto outer;
				++pos;
				if (!TrimChar('\"'))
					goto outer;
				t += c;
				break;
			case ' ': case '\t':
				if (!isQuoted)
					goto outer;
				[[fallthrough]];
			default:
				t += c;
				++pos;
			}
		outer:

		//DEBUG_OUT("ReadToken(%d,%d): '%s'\n", line, GetColumn(), t);
		last_pos_ = pos;
		return t;
	}

	int ReadInt(int range_min, int range_max) {
		int i = 0;
		int c = GetColumn();
		if (CStringA t = ReadToken(); t.IsEmpty())
			throw MakeError("expected integer, no token found.");
		else if (::sscanf(t, "%d", &i) != 1)
			throw MakeError("expected integer, '%s' found.", t);
		else if (i < range_min || i > range_max)
			throw MakeError("expected integer in range [%d,%d], %d found.", range_min, range_max, i);
		last_pos_ = pos;
		return i;
	}

	int ReadHex(int range_min, int range_max) {
		int i = 0;
		int c = GetColumn();
		if (CStringA t = ReadToken(); t.IsEmpty())
			throw MakeError("expected hexadecimal, no token found.");
		else if (::sscanf(t, "%x", &i) != 1)
			throw MakeError("expected hexadecimal, '%s' found.", t);
		else if (i < range_min || i > range_max)
			throw MakeError("expected hexidecmal in range [%X,%X], %X found.", range_min, range_max, i);
		last_pos_ = pos;
		return i;
	}

	// note: finishes line if found
	void ReadEOL() {
		int c = GetColumn();
		ConsumeSpace();
		if (CStringA s = ReadToken(); !s.IsEmpty())
			throw MakeError("expected end of line, '%s' found.", s);
		if (!Finished()) {
			if (WCHAR eol = text.GetAt(pos); eol != L'\r' && eol != L'\n')
				throw MakeError("expected end of line, '%c' found.", eol);
			FinishLine();
		}
		last_pos_ = pos;
	}

	// note: finishes line if found
	bool IsEOL() {
		ConsumeSpace();
		if (Finished()) {
			last_pos_ = pos;
			return true;
		}

		if (TrimChar(L'\n')) {		// // //
			++line;
			last_pos_ = linestart = pos;
			return true;
		}

		return false;
	}

	int ImportHex(const CStringA &sToken) {		// // //
		int i = 0;
		for (int d=0; d < sToken.GetLength(); ++d)
		{
			const char HEX_TEXT[16] = {
				'0', '1', '2', '3', '4', '5', '6', '7',
				'8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
			};

			i <<= 4;
			char t = sToken.Mid(d,1).MakeUpper().GetAt(0);		// // //
			int h = 0;
			for (h=0; h < 16; ++h)
				if (t == HEX_TEXT[h]) {
					i += h;
					break;
				}
			if (h >= 16)
				throw MakeError("hexadecimal number expected, '%s' found.", sToken);
		}
		return i;
	}

	template <typename... Args>
	auto MakeError(LPCSTR fmt, Args&&... args) const {		// // //
		CStringA str = FormattedA("Line %d column %d: ", line, GetColumn());
		str.AppendFormat(fmt, args...);
		return std::runtime_error {(LPCSTR)str};
	}

	stChanNote ImportCellText(unsigned fxMax, chan_id_t chan) {		// // //
		stChanNote Cell;		// // //

		CStringA sNote = ReadToken();
		if (sNote == "...") { Cell.Note = NONE; }
		else if (sNote == "---") { Cell.Note = HALT; }
		else if (sNote == "===") { Cell.Note = RELEASE; }
		else {
			if (sNote.GetLength() != 3)
				throw MakeError("note column should be 3 characters wide, '%s' found.", sNote);

			if (chan == chan_id_t::NOISE) {		// // // noise
				int h = ImportHex(sNote.Left(1));		// // //
				Cell.Note = (h % NOTE_RANGE) + 1;
				Cell.Octave = h / NOTE_RANGE;

				// importer is very tolerant about the second and third characters
				// in a noise note, they can be anything
			}
			else if (sNote.GetAt(0) == L'^' && sNote.GetAt(1) == L'-') {		// // //
				int o = sNote.GetAt(2) - L'0';
				if (o < 0 || o > ECHO_BUFFER_LENGTH)
					throw MakeError("out-of-bound echo buffer accessed.");
				Cell.Note = ECHO;
				Cell.Octave = o;
			}
			else {
				int n = 0;
				switch (sNote.GetAt(0)) {
				case L'c': case L'C': n = 0; break;
				case L'd': case L'D': n = 2; break;
				case L'e': case L'E': n = 4; break;
				case L'f': case L'F': n = 5; break;
				case L'g': case L'G': n = 7; break;
				case L'a': case L'A': n = 9; break;
				case L'b': case L'B': n = 11; break;
				default:
					throw MakeError("unrecognized note '%s'.", sNote);
				}
				switch (sNote.GetAt(1)) {
				case L'-': case L'.': break;
				case L'#': case L'+': n += 1; break;
				case L'b': case L'f': n -= 1; break;
				default:
					throw MakeError("unrecognized note '%s'.", sNote);
				}
				while (n < 0) n += NOTE_RANGE;
				while (n >= NOTE_RANGE) n -= NOTE_RANGE;
				Cell.Note = n + 1;

				int o = sNote.GetAt(2) - L'0';
				if (o < 0 || o >= OCTAVE_RANGE) {
					throw MakeError("unrecognized octave '%s'.", sNote);
				}
				Cell.Octave = o;
			}
		}

		CStringA sInst = ReadToken();
		if (sInst == "..") { Cell.Instrument = MAX_INSTRUMENTS; }
		else if (sInst == "&&") { Cell.Instrument = HOLD_INSTRUMENT; }		// // // 050B
		else {
			if (sInst.GetLength() != 2)
				throw MakeError("instrument column should be 2 characters wide, '%s' found.", sInst);
			int h = ImportHex(sInst);		// // //
			if (h >= MAX_INSTRUMENTS)
				throw MakeError("instrument '%s' is out of bounds.", sInst);
			Cell.Instrument = h;
		}

		Cell.Vol = [&] (const CStringA &str) -> unsigned {
			if (str == ".")
				return MAX_VOLUME;
			const LPCSTR VOL_TEXT[] = {
				"0", "1", "2", "3", "4", "5", "6", "7",
				"8", "9", "A", "B", "C", "D", "E", "F",
			};
			unsigned v = 0;
			for (; v < std::size(VOL_TEXT); ++v)
				if (str == VOL_TEXT[v])
					return v;
			throw MakeError("unrecognized volume token '%s'.", str);
		}(ReadToken().MakeUpper());

		for (unsigned int e = 0; e <= fxMax; ++e) {		// // //
			CStringA sEff = ReadToken().MakeUpper();
			if (sEff.GetLength() != 3)
				throw MakeError("effect column should be 3 characters wide, '%s' found.", sEff);

			if (sEff != "...") {
				bool Valid;		// // //
				effect_t Eff = GetEffectFromChar(sEff.GetAt(0), GetChipFromChannel(chan), &Valid);
				if (!Valid)
					throw MakeError("unrecognized effect '%s'.", sEff);
				Cell.EffNumber[e] = Eff;

				Cell.EffParam[e] = ImportHex(sEff.Right(2));		// // //
			}
		}

		last_pos_ = pos;
		return Cell;
	}

private:
	bool TrimChar(WCHAR ch) {
		if (!Finished())
			if (WCHAR x = text.GetAt(pos); x == ch) {
				++pos;
				return true;
			}
		return false;
	}

	void ConsumeSpace() {
		while (TrimChar(L' ') || TrimChar(L'\t'))		// // //
			;
	}

	CStringA ReadFromFile(LPCWSTR FileName) {
		CFileException oFileException;
		if (CStdioFile f; f.Open(FileName, CFile::modeRead | CFile::typeBinary, &oFileException)) {
			CStringA str;
			while (f.GetPosition() < f.GetLength()) {
				char buf[1024] = { };
				ULONGLONG sz = std::min(f.GetLength() - f.GetPosition(), (ULONGLONG)std::size(buf));
				f.Read(buf, (UINT)sz);
				CStringA part(buf, (int)sz);
				part.Replace("\r", "");
				str += part;
			}
			f.Close();
			return str;
		}

		WCHAR szError[256];
		oFileException.GetErrorMessage(szError, std::size(szError));
		throw std::runtime_error {"Unable to open file:\n" + conv::to_utf8(szError)};
	}

public:
	int line = 1;

private:
	const CStringA text; // TODO: move outside and use string_view
	int pos = 0;
	int linestart = 0;
	int last_pos_ = 0;		// // //
};

// =============================================================================

CStringA CTextExport::ExportString(const CStringA &s)
{
	return ExportString(std::string_view {(LPCSTR)s});
}

CStringA CTextExport::ExportString(std::string_view s)		// // //
{
	// puts " at beginning and end of string, replace " with ""
	CStringA r = "\"";
	for (char c : s) {
		if (c == '\"')
			r += c;
		r += c;
	}
	r += "\"";
	return r;
}

// =============================================================================

CStringA CTextExport::ExportCellText(const stChanNote &stCell, unsigned int nEffects, bool bNoise)		// // //
{
	CStringA tmp;

	static const LPCSTR TEXT_NOTE[ECHO+1] = {		// // //
		"...",
		"C-?", "C#?", "D-?", "D#?", "E-?", "F-?",
		"F#?", "G-?", "G#?", "A-?", "A#?", "B-?",
		"===", "---", "^-?",
	};

	CStringA s = (stCell.Note <= ECHO) ? TEXT_NOTE[stCell.Note] : "...";		// // //
	if (stCell.Note >= NOTE_C && stCell.Note <= NOTE_B || stCell.Note == ECHO)		// // //
	{
		if (bNoise)
		{
			char nNoiseFreq = (stCell.Note - 1 + stCell.Octave * NOTE_RANGE) & 0x0F;
			s.Format("%01X-#", nNoiseFreq);
		}
		else
		{
			s = s.Left(2);
			tmp.Format("%01d", stCell.Octave);
			s += tmp;
		}
	}

	tmp.Format(" %02X", stCell.Instrument);
	s += (stCell.Instrument == MAX_INSTRUMENTS) ? CStringA(" ..") :
		(stCell.Instrument == HOLD_INSTRUMENT) ? CStringA(" &&") : tmp;		// // // 050B

	tmp.Format(" %01X", stCell.Vol);
	s += (stCell.Vol == 0x10) ? CStringA(" .") : tmp;		// // //

	for (unsigned int e=0; e < nEffects; ++e)
	{
		if (stCell.EffNumber[e] == 0)
		{
			s += " ...";
		}
		else
		{
			tmp.Format(" %c%02X", EFF_CHAR[stCell.EffNumber[e]-1], stCell.EffParam[e]);
			s += tmp;
		}
	}

	return s;
}

// =============================================================================

#define CHECK_SYMBOL(x) do { \
		if (CStringA symbol_ = t.ReadToken(); symbol_ != x) \
			throw t.MakeError("expected '%s', '%s' found.", x, symbol_); \
	} while (false)

#define CHECK_COLON() CHECK_SYMBOL(":")

void CTextExport::ImportFile(LPCWSTR FileName, CFamiTrackerDoc &Doc) {
	// begin a new document
	if (!Doc.OnNewDocument())
		throw std::runtime_error {"Unable to create new Famitracker document."};

	// parse the file
	Tokenizer t(FileName);		// // //

	auto &modfile = *Doc.GetModule();		// // //
	auto &InstManager = *modfile.GetInstrumentManager();

	unsigned int dpcm_index = 0;
	unsigned int dpcm_size = 0;
	std::shared_ptr<ft0cc::doc::dpcm_sample> dpcm_sample;		// // //
	unsigned int track = 0;
	unsigned int pattern = 0;
	int N163count = -1;		// // //
	bool UseGroove[MAX_TRACKS] = {};		// // //
	while (!t.Finished()) {
		// read first token on line
		if (t.IsEOL()) continue; // blank line
		CStringA command = t.ReadToken().MakeUpper();		// // //

		int c = 0;
		for (; c < CT_COUNT; ++c)
			if (command == CT[c]) break;

		//DEBUG_OUT("Command read: %s\n", command);
		switch (c) {
		case CT_COMMENTLINE:
			t.FinishLine();
			break;
		case CT_TITLE:
			modfile.SetModuleName(LPCSTR(t.ReadToken()));
			t.ReadEOL();
			break;
		case CT_AUTHOR:
			modfile.SetModuleArtist(LPCSTR(t.ReadToken()));
			t.ReadEOL();
			break;
		case CT_COPYRIGHT:
			modfile.SetModuleCopyright(LPCSTR(t.ReadToken()));
			t.ReadEOL();
			break;
		case CT_COMMENT:
		{
			auto sComment = std::string {modfile.GetComment()};		// // //
			if (!sComment.empty())
				sComment += "\r\n";
			sComment += t.ReadToken();
			modfile.SetComment(sComment, modfile.ShowsCommentOnOpen());
			t.ReadEOL();
		}
		break;
		case CT_MACHINE:
			modfile.SetMachine(static_cast<machine_t>(t.ReadInt(0, PAL)));
			t.ReadEOL();
			break;
		case CT_FRAMERATE:
			modfile.SetEngineSpeed(t.ReadInt(0, 800));
			t.ReadEOL();
			break;
		case CT_EXPANSION: {
			auto flag = t.ReadInt(0, CSoundChipSet::NSF_MAX_FLAG);		// // //
			modfile.SetChannelMap(Env.GetSoundGenerator()->MakeChannelMap(CSoundChipSet::FromNSFFlag(flag), modfile.GetNamcoChannels()));
			t.ReadEOL();
			break;
		}
		case CT_VIBRATO:
			modfile.SetVibratoStyle(static_cast<vibrato_t>(t.ReadInt(0, VIBRATO_NEW)));
			t.ReadEOL();
			break;
		case CT_SPLIT:
			modfile.SetSpeedSplitPoint(t.ReadInt(0, 255));
			t.ReadEOL();
			break;
		case CT_PLAYBACKRATE:		// // // 050B
			t.ReadInt(0, 2);

			t.ReadInt(0, 0xFFFF);

			t.ReadEOL();
			break;
		case CT_TUNING:		// // // 050B
		{
			int octave = t.ReadInt(-12, 12);
			int cent = t.ReadInt(-100, 100);
			t.ReadEOL();
			modfile.SetTuning(octave, cent);
		}
		break;
		case CT_N163CHANNELS:
			N163count = t.ReadInt(1, MAX_CHANNELS_N163);		// // //
			t.ReadEOL();
			modfile.SetChannelMap(Env.GetSoundGenerator()->MakeChannelMap(modfile.GetSoundChipSet(), MAX_CHANNELS_N163));
			break;
		case CT_MACRO:
		case CT_MACROVRC6:
		case CT_MACRON163:
		case CT_MACROS5B:
		{
			static const inst_type_t CHIP_MACRO[4] = {INST_2A03, INST_VRC6, INST_N163, INST_S5B};		// // //
			int chip = c - CT_MACRO;

			auto mt = (sequence_t)t.ReadInt(0, SEQ_COUNT - 1);
			int index = t.ReadInt(0, MAX_SEQUENCES - 1);
			const auto pSeq = InstManager.GetSequence(CHIP_MACRO[chip], mt, index);

			int loop = t.ReadInt(-1, MAX_SEQUENCE_ITEMS);		// // //
			int release = t.ReadInt(-1, MAX_SEQUENCE_ITEMS);
			pSeq->SetSetting(static_cast<seq_setting_t>(t.ReadInt(0, 255)));		// // //

			CHECK_COLON();

			int count = 0;
			while (!t.IsEOL()) {
				int item = t.ReadInt(-128, 127);
				if (count >= MAX_SEQUENCE_ITEMS)
					throw t.MakeError("macro overflow, max size: %d.", MAX_SEQUENCE_ITEMS);
				pSeq->SetItem(count, item);
				++count;
			}
			pSeq->SetItemCount(count);
			pSeq->SetLoopPoint(loop);		// // //
			pSeq->SetReleasePoint(release);
		}
		break;
		case CT_DPCMDEF:
		{
			if (dpcm_sample) {
				if (dpcm_sample->size() < dpcm_size)
					dpcm_sample->resize(dpcm_size);
				modfile.GetDSampleManager()->SetDSample(dpcm_index, std::move(dpcm_sample));
			}

			dpcm_index = t.ReadInt(0, MAX_DSAMPLES - 1);
			dpcm_size = t.ReadInt(0, ft0cc::doc::dpcm_sample::max_size);
			dpcm_sample = std::make_shared<ft0cc::doc::dpcm_sample>();		// // //
			dpcm_sample->rename(LPCSTR(t.ReadToken()));

			t.ReadEOL();
		}
		break;
		case CT_DPCM:
		{
			CHECK_COLON();
			while (!t.IsEOL()) {
				auto sample = static_cast<ft0cc::doc::dpcm_sample::sample_t>(t.ReadHex(0x00, 0xFF));
				std::size_t pos = dpcm_sample->size();
				if (pos >= dpcm_size)
					throw t.MakeError("DPCM sample %d overflow, increase size used in %s.", dpcm_index, CT[CT_DPCMDEF]);
				dpcm_sample->resize(pos + 1);		// // //
				dpcm_sample->set_sample_at(pos, sample);
			}
		}
		break;
		case CT_DETUNE: {		// // //
			int table = t.ReadInt(0, 5);
			int oct = t.ReadInt(0, OCTAVE_RANGE - 1);
			int note = t.ReadInt(0, NOTE_RANGE - 1);
			int offset = t.ReadInt(-32768, 32767);
			modfile.SetDetuneOffset(table, oct * NOTE_RANGE + note, offset);
			t.ReadEOL();
			break;
		}
		case CT_GROOVE:		// // //
		{
			int index = t.ReadInt(0, MAX_GROOVE - 1);
			int size = t.ReadInt(1, ft0cc::doc::groove::max_size);
			auto pGroove = std::make_unique<ft0cc::doc::groove>();
			pGroove->resize(size);
			CHECK_COLON();
			for (uint8_t &x : *pGroove)
				x = t.ReadInt(1, 255);
			modfile.SetGroove(index, std::move(pGroove));
			t.ReadEOL();
		}
		break;
		case CT_USEGROOVE:		// // //
		{
			CHECK_COLON();
			while (!t.IsEOL()) {
				unsigned index = (unsigned)t.ReadInt(1, MAX_TRACKS) - 1;
				UseGroove[index] = true;
				if (index < modfile.GetSongCount())
					modfile.GetSong(index)->SetSongGroove(true);
			}
		}
		break;
		case CT_INST2A03:		// // //
		case CT_INSTVRC6:
		case CT_INSTN163:
		case CT_INSTS5B:
		{
			inst_type_t Type = [c] {
				switch (c) {
				case CT_INST2A03: return INST_2A03;
				case CT_INSTVRC6: return INST_VRC6;
				case CT_INSTN163: return INST_N163;
				case CT_INSTS5B:  return INST_S5B;
				}
				return INST_NONE;
			}();
			int inst_index = t.ReadInt(0, MAX_INSTRUMENTS - 1);		// // //
			auto pInst = Env.GetInstrumentService()->Make(Type);
			auto seqInst = static_cast<CSeqInstrument *>(pInst.get());
			foreachSeq([&] (sequence_t s) {
				int seqindex = t.ReadInt(-1, MAX_SEQUENCES - 1);
				seqInst->SetSeqEnable(s, seqindex != -1);
				seqInst->SetSeqIndex(s, seqindex != -1 ? seqindex : 0);
			});
			if (c == CT_INSTN163) {
				auto pInst = static_cast<CInstrumentN163*>(seqInst);
				pInst->SetWaveSize(t.ReadInt(0, 256 - 16 * N163count));		// // //
				pInst->SetWavePos(t.ReadInt(0, 256 - 16 * N163count - 1));
				pInst->SetWaveCount(t.ReadInt(1, CInstrumentN163::MAX_WAVE_COUNT));
			}
			seqInst->SetName(LPCSTR(t.ReadToken()));
			InstManager.InsertInstrument(inst_index, std::move(pInst));
			t.ReadEOL();
		}
		break;
		case CT_INSTVRC7:
		{
			int inst_index = t.ReadInt(0, MAX_INSTRUMENTS - 1);
			auto pInst = std::make_unique<CInstrumentVRC7>();		// // //
			pInst->SetPatch(t.ReadInt(0, 15));
			for (int r = 0; r < 8; ++r)
				pInst->SetCustomReg(r, t.ReadHex(0x00, 0xFF));
			pInst->SetName(LPCSTR(t.ReadToken()));
			InstManager.InsertInstrument(inst_index, std::move(pInst));
			t.ReadEOL();
		}
		break;
		case CT_INSTFDS:
		{
			int inst_index = t.ReadInt(0, MAX_INSTRUMENTS - 1);
			auto pInst = std::make_unique<CInstrumentFDS>();		// // //
			pInst->SetModulationEnable(t.ReadInt(0, 1) == 1);
			pInst->SetModulationSpeed(t.ReadInt(0, 4095));
			pInst->SetModulationDepth(t.ReadInt(0, 63));
			pInst->SetModulationDelay(t.ReadInt(0, 255));
			pInst->SetName(LPCSTR(t.ReadToken()));
			InstManager.InsertInstrument(inst_index, std::move(pInst));
			t.ReadEOL();
		}
		break;
		case CT_KEYDPCM:
		{
			int inst_index = t.ReadInt(0, MAX_INSTRUMENTS - 1);
			if (InstManager.GetInstrumentType(inst_index) != INST_2A03)
				throw t.MakeError("instrument %d is not defined as a 2A03 instrument.", inst_index);
			auto pInst = std::static_pointer_cast<CInstrument2A03>(InstManager.GetInstrument(inst_index));

			int io = t.ReadInt(0, OCTAVE_RANGE);
			int in = t.ReadInt(0, 12);

			pInst->SetSampleIndex(io, in, t.ReadInt(0, MAX_DSAMPLES - 1) + 1);
			pInst->SetSamplePitch(io, in, t.ReadInt(0, 15));
			pInst->SetSampleLoop(io, in, t.ReadInt(0, 1) == 1);
			pInst->SetSampleLoopOffset(io, in, t.ReadInt(0, 255));
			pInst->SetSampleDeltaValue(io, in, t.ReadInt(-1, 127));
			t.ReadEOL();
		}
		break;
		case CT_FDSWAVE:
		{
			int inst_index = t.ReadInt(0, MAX_INSTRUMENTS - 1);
			if (InstManager.GetInstrumentType(inst_index) != INST_FDS)
				throw t.MakeError("instrument %d is not defined as an FDS instrument.", inst_index);
			auto pInst = std::static_pointer_cast<CInstrumentFDS>(InstManager.GetInstrument(inst_index));
			CHECK_COLON();
			for (int s = 0; s < CInstrumentFDS::WAVE_SIZE; ++s)
				pInst->SetSample(s, t.ReadInt(0, 63));
			t.ReadEOL();
		}
		break;
		case CT_FDSMOD:
		{
			int inst_index = t.ReadInt(0, MAX_INSTRUMENTS - 1);
			if (InstManager.GetInstrumentType(inst_index) != INST_FDS)
				throw t.MakeError("instrument %d is not defined as an FDS instrument.", inst_index);
			auto pInst = std::static_pointer_cast<CInstrumentFDS>(InstManager.GetInstrument(inst_index));
			CHECK_COLON();
			for (int s = 0; s < CInstrumentFDS::MOD_SIZE; ++s)
				pInst->SetModulation(s, t.ReadInt(0, 7));
			t.ReadEOL();
		}
		break;
		case CT_FDSMACRO:
		{
			int inst_index = t.ReadInt(0, MAX_INSTRUMENTS - 1);
			if (InstManager.GetInstrumentType(inst_index) != INST_FDS)
				throw t.MakeError("instrument %d is not defined as an FDS instrument.", inst_index);
			auto pInst = std::static_pointer_cast<CInstrumentFDS>(InstManager.GetInstrument(inst_index));

			auto SeqType = (sequence_t)t.ReadInt(0, CInstrumentFDS::SEQUENCE_COUNT - 1);		// // //
			auto pSeq = std::make_shared<CSequence>(SeqType);
			pInst->SetSequence(SeqType, pSeq);
			int loop = t.ReadInt(-1, MAX_SEQUENCE_ITEMS);
			int release = t.ReadInt(-1, MAX_SEQUENCE_ITEMS);
			pSeq->SetSetting(static_cast<seq_setting_t>(t.ReadInt(0, 255)));		// // //

			CHECK_COLON();

			int count = 0;
			while (!t.IsEOL()) {
				int item = t.ReadInt(-128, 127);
				if (count >= MAX_SEQUENCE_ITEMS)
					throw t.MakeError("macro overflow, max size: %d.", MAX_SEQUENCE_ITEMS);
				pSeq->SetItem(count, item);
				++count;
			}
			pSeq->SetItemCount(count);
			pSeq->SetLoopPoint(loop);
			pSeq->SetReleasePoint(release);
		}
		break;
		case CT_N163WAVE:
		{
			int inst_index = t.ReadInt(0, MAX_INSTRUMENTS - 1);
			if (InstManager.GetInstrumentType(inst_index) != INST_N163)
				throw t.MakeError("instrument %d is not defined as an N163 instrument.", inst_index);
			auto pInst = std::static_pointer_cast<CInstrumentN163>(InstManager.GetInstrument(inst_index));

			int iw = t.ReadInt(0, CInstrumentN163::MAX_WAVE_COUNT - 1);
			CHECK_COLON();
			for (unsigned s = 0; s < pInst->GetWaveSize(); ++s)
				pInst->SetSample(iw, s, t.ReadInt(0, 15));
			t.ReadEOL();
		}
		break;
		case CT_TRACK:
		{
			if (track != 0 && !modfile.InsertSong(track, modfile.MakeNewSong()))
				throw t.MakeError("unable to add new track.");

			CSongData *pSong = modfile.GetSong(track);
			pSong->SetPatternLength(t.ReadInt(1, MAX_PATTERN_LENGTH));		// // //
			pSong->SetSongGroove(UseGroove[track]);		// // //
			pSong->SetSongSpeed(t.ReadInt(0, MAX_TEMPO));
			pSong->SetSongTempo(t.ReadInt(0, MAX_TEMPO));
			pSong->SetTitle((LPCSTR)t.ReadToken());		// // //

			t.ReadEOL();
			++track;
		}
		break;
		case CT_COLUMNS:
		{
			CHECK_COLON();
			CSongData *pSong = modfile.GetSong(track - 1);		// // //
			modfile.GetChannelOrder().ForeachChannel([&] (chan_id_t c) {
				pSong->SetEffectColumnCount(c, t.ReadInt(1, MAX_EFFECT_COLUMNS) - 1);
			});
			t.ReadEOL();
		}
		break;
		case CT_ORDER:
		{
			CSongData *pSong = modfile.GetSong(track - 1);		// // //
			int ifr = t.ReadHex(0, MAX_FRAMES - 1);
			if (ifr >= (int)pSong->GetFrameCount()) // expand to accept frames
				pSong->SetFrameCount(ifr + 1);
			CHECK_COLON();
			modfile.GetChannelOrder().ForeachChannel([&] (chan_id_t c) {
				pSong->SetFramePattern(ifr, c, t.ReadHex(0, MAX_PATTERN - 1));
			});
			t.ReadEOL();
		}
		break;
		case CT_PATTERN:
			pattern = t.ReadHex(0, MAX_PATTERN - 1);
			t.ReadEOL();
			break;
		case CT_ROW:
		{
			if (track == 0)
				throw t.MakeError("no TRACK defined, cannot add ROW data.");

			int row = t.ReadHex(0, MAX_PATTERN_LENGTH - 1);
			modfile.GetChannelOrder().ForeachChannel([&] (chan_id_t c) {
				CHECK_COLON();
				auto *pTrack = modfile.GetSong(track - 1)->GetTrack(c);		// // //
				pTrack->GetPattern(pattern).SetNoteOn(row, t.ImportCellText(pTrack->GetEffectColumnCount(), c));
			});
			t.ReadEOL();
		}
		break;
		case CT_COUNT:
		default:
			throw t.MakeError("Unrecognized command: '%s'.", command);
		}
	}

	if (dpcm_sample) {
		if (dpcm_sample->size() < dpcm_size)
			dpcm_sample->resize(dpcm_size);
		modfile.GetDSampleManager()->SetDSample(dpcm_index, std::move(dpcm_sample));
	}
	if (N163count != -1)		// // //
		modfile.SetChannelMap(Env.GetSoundGenerator()->MakeChannelMap(modfile.GetSoundChipSet(), N163count));

	Env.GetSoundGenerator()->AssignModule(modfile);		// / //
	Env.GetSoundGenerator()->ModuleChipChanged();		// // //
}

// =============================================================================

CStringA CTextExport::ExportRows(LPCWSTR FileName, const CFamiTrackerModule &modfile) {		// // //
	CStdioFile f;
	CFileException oFileException;
	if (!f.Open(FileName, CFile::modeCreate | CFile::modeWrite | CFile::typeText, &oFileException))
	{
		WCHAR szError[256];
		oFileException.GetErrorMessage(szError, 256);

		return FormattedA("Unable to open file:\n%s", conv::to_utf8(szError).data());
	}

	const auto WriteString = [&] (const CStringA &str) { // TODO: don't use CStdioFile
		f.Write(str.GetString(), str.GetLength());
	};

	WriteString("ID,TRACK,CHANNEL,PATTERN,ROW,NOTE,OCTAVE,INST,VOLUME,FX1,FX1PARAM,FX2,FX2PARAM,FX3,FX3PARAM,FX4,FX4PARAM\n");

	const CStringA FMT = "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n";
	int id = 0;

	modfile.VisitSongs([&] (const CSongData &song, unsigned t) {
		unsigned rows = song.GetPatternLength();
		song.VisitPatterns([&] (const CPatternData &pat, chan_id_t c, unsigned p) {
			if (song.IsPatternInUse(c, p))
				pat.VisitRows(rows, [&] (const stChanNote &stCell, unsigned r) {
					if (stCell != stChanNote { })
						WriteString(FormattedA(FMT, id++, t, c, p, r,
							stCell.Note, stCell.Octave, stCell.Instrument, stCell.Vol,
							stCell.EffNumber[0], stCell.EffParam[0],
							stCell.EffNumber[1], stCell.EffParam[1],
							stCell.EffNumber[2], stCell.EffParam[2],
							stCell.EffNumber[3], stCell.EffParam[3]));
				});
		});
	});

	return "";
}

CStringA CTextExport::ExportFile(LPCWSTR FileName, CFamiTrackerDoc &Doc) {		// // //
	CStdioFile f;
	CFileException oFileException;
	if (!f.Open(FileName, CFile::modeCreate | CFile::modeWrite | CFile::typeText, &oFileException)) {
		WCHAR szError[256];
		oFileException.GetErrorMessage(szError, 256);

		return FormattedA("Unable to open file:\n%s", conv::to_utf8(szError).data());
	}

	CStringA s;
	auto &modfile = *Doc.GetModule();		// // //

	const auto WriteString = [&] (const CStringA &str) { // TODO: don't use CStdioFile
		f.Write(str.GetString(), str.GetLength());
	};

	WriteString(FormattedA("# 0CC-FamiTracker text export %s\n\n", Get0CCFTVersionString()));		// // //

	WriteString("# Module information\n");
	WriteString(FormattedA("%-15s %s\n", CT[CT_TITLE],     ExportString(modfile.GetModuleName())));
	WriteString(FormattedA("%-15s %s\n", CT[CT_AUTHOR],    ExportString(modfile.GetModuleArtist())));
	WriteString(FormattedA("%-15s %s\n", CT[CT_COPYRIGHT], ExportString(modfile.GetModuleCopyright())));
	WriteString("\n");

	WriteString("# Module comment\n");
	std::string_view sComment = modfile.GetComment();		// // //
	while (true) {
		auto n = sComment.find_first_of("\r\n");
		s.Format("%s %s\n", CT[CT_COMMENT], ExportString(sComment.substr(0, n)));
		WriteString(s);
		if (n == std::string_view::npos)
			break;
		sComment.remove_prefix(n);
		if (!sComment.empty() && sComment.front() == '\r')
			sComment.remove_prefix(1);
		if (!sComment.empty() && sComment.front() == '\n')
			sComment.remove_prefix(1);
	}
	WriteString("\n");

	WriteString("# Global settings\n");
	WriteString(FormattedA("%-15s %d\n", CT[CT_MACHINE],   modfile.GetMachine()));
	WriteString(FormattedA("%-15s %d\n", CT[CT_FRAMERATE], modfile.GetEngineSpeed()));
	WriteString(FormattedA("%-15s %d\n", CT[CT_EXPANSION], modfile.GetSoundChipSet().GetNSFFlag()));		// // //
	WriteString(FormattedA("%-15s %d\n", CT[CT_VIBRATO],   modfile.GetVibratoStyle()));
	WriteString(FormattedA("%-15s %d\n", CT[CT_SPLIT],     modfile.GetSpeedSplitPoint()));
//	WriteString(FormattedA("%-15s %d %d\n", CT[CT_PLAYBACKRATE]));
	if (modfile.GetTuningSemitone() || modfile.GetTuningCent())		// // // 050B
		WriteString(FormattedA("%-15s %d %d\n", CT[CT_TUNING], modfile.GetTuningSemitone(), modfile.GetTuningCent()));
	WriteString("\n");

	int N163count = -1;		// // //
	if (modfile.HasExpansionChip(sound_chip_t::N163)) {
		N163count = modfile.GetNamcoChannels();
		modfile.SetChannelMap(Env.GetSoundGenerator()->MakeChannelMap(modfile.GetSoundChipSet(), MAX_CHANNELS_N163));
		s.Format("# Namco 163 global settings\n"
		         "%-15s %d\n"
		         "\n",
		         CT[CT_N163CHANNELS], N163count);
		WriteString(s);
	}

	WriteString("# Macros\n");
	const auto &InstManager = *modfile.GetInstrumentManager();
	const inst_type_t CHIP_MACRO[4] = { INST_2A03, INST_VRC6, INST_N163, INST_S5B };
	for (int c=0; c<4; ++c) {
		foreachSeq([&] (sequence_t st) {
			for (int seq = 0; seq < MAX_SEQUENCES; ++seq) {
				const auto pSequence = InstManager.GetSequence(CHIP_MACRO[c], st, seq);
				if (pSequence && pSequence->GetItemCount() > 0) {
					s.Format("%-9s %3d %3d %3d %3d %3d :",
						CT[CT_MACRO + c],
						st,
						seq,
						pSequence->GetLoopPoint(),
						pSequence->GetReleasePoint(),
						pSequence->GetSetting());
					WriteString(s);
					for (unsigned int i = 0; i < pSequence->GetItemCount(); ++i) {
						s.Format(" %d", pSequence->GetItem(i));
						WriteString(s);
					}
					WriteString("\n");
				}
			}
		});
	}
	WriteString("\n");

	WriteString("# DPCM samples\n");
	for (int smp=0; smp < MAX_DSAMPLES; ++smp)
	{
		if (auto pSample = modfile.GetDSampleManager()->GetDSample(smp)) {		// // //
			const unsigned int size = pSample->size();
			s.Format("%s %3d %5d %s\n",
				CT[CT_DPCMDEF],
				smp,
				size,
				ExportString(pSample->name()));
			WriteString(s);

			for (unsigned int i=0; i < size; i += 32)
			{
				s.Format("%s :", CT[CT_DPCM]);
				WriteString(s);
				for (unsigned int j=0; j<32 && (i+j)<size; ++j)
				{
					s.Format(" %02X", pSample->sample_at(i + j));		// // //
					WriteString(s);
				}
				WriteString("\n");
			}
		}
	}
	WriteString("\n");

	WriteString("# Detune settings\n");		// // //
	for (int i = 0; i < 6; i++) for (int j = 0; j < NOTE_COUNT; j++) {
		int Offset = modfile.GetDetuneOffset(i, j);
		if (Offset != 0) {
			s.Format("%s %3d %3d %3d %5d\n", CT[CT_DETUNE], i, j / NOTE_RANGE, j % NOTE_RANGE, Offset);
			WriteString(s);
		}
	}
	WriteString("\n");

	WriteString("# Grooves\n");		// // //
	for (int i = 0; i < MAX_GROOVE; i++) {
		if (const auto pGroove = modfile.GetGroove(i)) {
			s.Format("%s %3d %3d :", CT[CT_GROOVE], i, pGroove->size());
			WriteString(s);
			for (uint8_t entry : *pGroove) {
				s.Format(" %d", entry);
				WriteString(s);
			}
			WriteString("\n");
		}
	}
	WriteString("\n");

	WriteString("# Tracks using default groove\n");		// // //
	bool UsedGroove = false;
	modfile.VisitSongs([&] (const CSongData &song) {
		if (song.GetSongGroove())
			UsedGroove = true;
	});
	if (UsedGroove) {
		s.Format("%s :", CT[CT_USEGROOVE]);
		modfile.VisitSongs([&] (const CSongData &song, unsigned index) {
			if (song.GetSongGroove())
				s.AppendFormat(" %d", index + 1);
		});
		WriteString(s);
		WriteString("\n\n");
	}

	WriteString("# Instruments\n");
	for (unsigned int i=0; i<MAX_INSTRUMENTS; ++i) {
		auto pInst = InstManager.GetInstrument(i);
		if (!pInst) continue;

		const char *CTstr = nullptr;		// // //
		switch (pInst->GetType()) {
		case INST_2A03:	CTstr = CT[CT_INST2A03]; break;
		case INST_VRC6:	CTstr = CT[CT_INSTVRC6]; break;
		case INST_VRC7:	CTstr = CT[CT_INSTVRC7]; break;
		case INST_FDS:	CTstr = CT[CT_INSTFDS];  break;
		case INST_N163:	CTstr = CT[CT_INSTN163]; break;
		case INST_S5B:	CTstr = CT[CT_INSTS5B];  break;
		case INST_NONE: default:
			continue;
		}
		s.Format("%-8s %3d   ", CTstr, i);
		WriteString(s);

		if (auto seqInst = std::dynamic_pointer_cast<CSeqInstrument>(pInst)) {
			s.Empty();
			foreachSeq([&] (sequence_t j) {
				s.AppendFormat("%3d ", seqInst->GetSeqEnable(j) ? seqInst->GetSeqIndex(j) : -1);
			});
			WriteString(s);
		}

		switch (pInst->GetType())
		{
		case INST_N163:
			{
				auto pDI = std::static_pointer_cast<CInstrumentN163>(pInst);
				s.Format("%3d %3d %3d ",
					pDI->GetWaveSize(),
					pDI->GetWavePos(),
					pDI->GetWaveCount());
				WriteString(s);
			}
			break;
		case INST_VRC7:
			{
				auto pDI = std::static_pointer_cast<CInstrumentVRC7>(pInst);
				s.Format("%3d ", pDI->GetPatch());
				for (int j = 0; j < 8; j++)
					s.AppendFormat("%02X ", pDI->GetCustomReg(j));
				WriteString(s);
			}
			break;
		case INST_FDS:
			{
				auto pDI = std::static_pointer_cast<CInstrumentFDS>(pInst);
				s.Format("%3d %3d %3d %3d ",
					pDI->GetModulationEnable(),
					pDI->GetModulationSpeed(),
					pDI->GetModulationDepth(),
					pDI->GetModulationDelay());
				WriteString(s);
			}
			break;
		}

		WriteString(ExportString(pInst->GetName()));
		WriteString("\n");

		switch (pInst->GetType())
		{
		case INST_2A03:
			{
				auto pDI = std::static_pointer_cast<CInstrument2A03>(pInst);
				for (int oct = 0; oct < OCTAVE_RANGE; ++oct)
				for (int key = 0; key < NOTE_RANGE; ++key)
				{
					int smp = pDI->GetSampleIndex(oct, key);
					if (smp != 0)
					{
						int d = pDI->GetSampleDeltaValue(oct, key);
						s.Format("%s %3d %3d %3d   %3d %3d %3d %5d %3d\n",
							CT[CT_KEYDPCM],
							i,
							oct, key,
							smp - 1,
							pDI->GetSamplePitch(oct, key) & 0x0F,
							pDI->GetSampleLoop(oct, key) ? 1 : 0,
							pDI->GetSampleLoopOffset(oct, key),
							(d >= 0 && d <= 127) ? d : -1);
						WriteString(s);
					}
				}
			}
			break;
		case INST_N163:
			{
				auto pDI = std::static_pointer_cast<CInstrumentN163>(pInst);
				for (int w=0; w < pDI->GetWaveCount(); ++w)
				{
					s.Format("%s %3d %3d :",
						CT[CT_N163WAVE], i, w);
					WriteString(s);

					for (int smp : pDI->GetSamples(w)) {		// // //
						s.Format(" %d", smp);
						WriteString(s);
					}
					WriteString("\n");
				}
			}
			break;
		case INST_FDS:
			{
				auto pDI = std::static_pointer_cast<CInstrumentFDS>(pInst);
				s.Format("%-8s %3d :", CT[CT_FDSWAVE], i);
				WriteString(s);
				for (unsigned char smp : pDI->GetSamples()) {		// // //
					s.Format(" %2d", smp);
					WriteString(s);
				}
				WriteString("\n");

				s.Format("%-8s %3d :", CT[CT_FDSMOD], i);
				WriteString(s);
				for (unsigned char m : pDI->GetModTable()) {		// // //
					s.Format(" %2d", m);
					WriteString(s);
				}
				WriteString("\n");

				foreachSeq([&] (sequence_t seq) {
					const auto pSequence = pDI->GetSequence(seq);		// // //
					if (!pSequence || pSequence->GetItemCount() < 1)
						return;

					s.Format("%-8s %3d %3d %3d %3d %3d :",
						CT[CT_FDSMACRO],
						i,
						seq,
						pSequence->GetLoopPoint(),
						pSequence->GetReleasePoint(),
						pSequence->GetSetting());
					WriteString(s);
					for (unsigned int i=0; i < pSequence->GetItemCount(); ++i)
					{
						s.Format(" %d", pSequence->GetItem(i));
						WriteString(s);
					}
					WriteString("\n");
				});
			}
			break;
		}
	}
	WriteString("\n");

	WriteString("# Tracks\n\n");

	const CChannelOrder &order = modfile.GetChannelOrder();		// // //

	modfile.VisitSongs([&] (const CSongData &song) {
		s.Format("%s %3d %3d %3d %s\n",
			CT[CT_TRACK],
			song.GetPatternLength(),
			song.GetSongSpeed(),
			song.GetSongTempo(),
			ExportString(song.GetTitle()));		// // //
		WriteString(s);

		s.Format("%s :", CT[CT_COLUMNS]);
		WriteString(s);
		order.ForeachChannel([&] (chan_id_t c) {
			s.Format(" %d", song.GetEffectColumnCount(c)+1);
			WriteString(s);
		});
		WriteString("\n\n");

		for (unsigned int o=0; o < song.GetFrameCount(); ++o) {
			s.Format("%s %02X :", CT[CT_ORDER], o);
			WriteString(s);
			order.ForeachChannel([&] (chan_id_t c) {
				s.Format(" %02X", song.GetFramePattern(o, c));
				WriteString(s);
			});
			WriteString("\n");
		}
		WriteString("\n");

		for (int p=0; p < MAX_PATTERN; ++p)
		{
			// detect and skip empty patterns
			bool bUsed = false;
			order.ForeachChannel([&] (chan_id_t c) {
				if (!bUsed && !song.GetPattern(c, p).IsEmpty())
					bUsed = true;
			});
			if (!bUsed)
				continue;

			s.Format("%s %02X\n", CT[CT_PATTERN], p);
			WriteString(s);

			for (unsigned int r=0; r < song.GetPatternLength(); ++r) {
				s.Format("%s %02X", CT[CT_ROW], r);
				WriteString(s);
				order.ForeachChannel([&] (chan_id_t c) {
					WriteString(" : ");
					WriteString(ExportCellText(song.GetPattern(c, p).GetNoteOn(r), song.GetEffectColumnCount(c)+1, c==chan_id_t::NOISE));		// // //
				});
				WriteString("\n");
			}
			WriteString("\n");
		}
	});

	if (N163count != -1)		// // //
		modfile.SetChannelMap(Env.GetSoundGenerator()->MakeChannelMap(modfile.GetSoundChipSet(), N163count));
	WriteString("# End of export\n");
	Doc.UpdateAllViews(NULL, UPDATE_FRAME);
	Doc.UpdateAllViews(NULL, UPDATE_PATTERN);
	Env.GetSoundGenerator()->ModuleChipChanged();		// // //
	return "";
}

// end of file
