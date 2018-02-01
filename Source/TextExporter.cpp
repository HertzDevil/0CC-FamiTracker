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
#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"		// // //
#include "version.h"		// // //
#include "FamiTrackerEnv.h"		// // //
#include "FamiTrackerViewMessage.h"		// // //

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
	std::is_convertible<T, LPCTSTR>
> { };

template <typename... Args>		// // // TODO: put this into stdafx.h perhaps
CString Formatted(const char *fmt, Args&&... args) {
	CString str;
	static_assert(std::conjunction_v<printf_arg<Args>...>,
		"Only scalar types allowed in printf");
	str.Format(fmt, std::forward<Args>(args)...);
	return str;
}

const char *Charify(const CString &s) {		// // //
	return CT2CA((LPCTSTR)s, CP_UTF8);
}

} // namespace

#define DEBUG_OUT(...) { OutputDebugString(Formatted(__VA_ARGS__)); }

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

static const TCHAR* CT[CT_COUNT] =
{
	// comment
	_T("#"),
	// song info
	_T("TITLE"),
	_T("AUTHOR"),
	_T("COPYRIGHT"),
	_T("COMMENT"),
	// global settings
	_T("MACHINE"),
	_T("FRAMERATE"),
	_T("EXPANSION"),
	_T("VIBRATO"),
	_T("SPLIT"),
	// // // 050B
	_T("PLAYBACKRATE"),
	_T("TUNING"),
	// namco global settings
	_T("N163CHANNELS"),
	// macros
	_T("MACRO"),
	_T("MACROVRC6"),
	_T("MACRON163"),
	_T("MACROS5B"),
	// dpcm
	_T("DPCMDEF"),
	_T("DPCM"),
	// // // detune settings
	_T("DETUNE"),
	// // // grooves
	_T("GROOVE"),
	_T("USEGROOVE"),
	// instruments
	_T("INST2A03"),
	_T("INSTVRC6"),
	_T("INSTVRC7"),
	_T("INSTFDS"),
	_T("INSTN163"),
	_T("INSTS5B"),
	// instrument data
	_T("KEYDPCM"),
	_T("FDSWAVE"),
	_T("FDSMOD"),
	_T("FDSMACRO"),
	_T("N163WAVE"),
	// track info
	_T("TRACK"),
	_T("COLUMNS"),
	_T("ORDER"),
	// pattern data
	_T("PATTERN"),
	_T("ROW"),
};

// =============================================================================

class Tokenizer
{
public:
	explicit Tokenizer(LPCTSTR FileName) : text(ReadFromFile(FileName)) { }
	~Tokenizer() = default;

	void Reset() {
		last_pos_ = pos = 0;
		line = 1;
		linestart = 0;
	}

	void FinishLine() {
		if (int newpos = text.Find(_T('\n'), pos); newpos >= 0) {		// // //
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

	CString ReadToken() {
		ConsumeSpace();
		CString t = _T("");

		bool isQuoted = TrimChar(_T('\"'));		// // //
		while (!Finished())
			switch (TCHAR c = text.GetAt(pos)) {
			case _T('\r'): case _T('\n'):
				if (isQuoted)
					throw MakeError(_T("incomplete quoted string."));
				goto outer;
			case _T('\"'):
				if (!isQuoted)
					goto outer;
				++pos;
				if (!TrimChar('\"'))
					goto outer;
				t += c;
				break;
			case _T(' '): case _T('\t'):
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
		if (CString t = ReadToken(); t.IsEmpty())
			throw MakeError(_T("expected integer, no token found."));
		else if (::sscanf(t, "%d", &i) != 1)
			throw MakeError(_T("expected integer, '%s' found."), t);
		else if (i < range_min || i > range_max)
			throw MakeError(_T("expected integer in range [%d,%d], %d found."), range_min, range_max, i);
		last_pos_ = pos;
		return i;
	}

	int ReadHex(int range_min, int range_max) {
		int i = 0;
		int c = GetColumn();
		if (CString t = ReadToken(); t.IsEmpty())
			throw MakeError(_T("expected hexadecimal, no token found."));
		else if (::sscanf(t, "%x", &i) != 1)
			throw MakeError(_T("expected hexadecimal, '%s' found."), t);
		else if (i < range_min || i > range_max)
			throw MakeError(_T("expected hexidecmal in range [%X,%X], %X found."), range_min, range_max, i);
		last_pos_ = pos;
		return i;
	}

	// note: finishes line if found
	void ReadEOL() {
		int c = GetColumn();
		ConsumeSpace();
		if (CString s = ReadToken(); !s.IsEmpty())
			throw MakeError(_T("expected end of line, '%s' found."), s);
		if (!Finished()) {
			if (TCHAR eol = text.GetAt(pos); eol != _T('\r') && eol != _T('\n'))
				throw MakeError(_T("expected end of line, '%c' found."), eol);
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

		if (TrimChar(_T('\n'))) {		// // //
			++line;
			last_pos_ = linestart = pos;
			return true;
		}

		return false;
	}

	int ImportHex(const CString &sToken) {		// // //
		int i = 0;
		for (int d=0; d < sToken.GetLength(); ++d)
		{
			const TCHAR HEX_TEXT[16] = {
				_T('0'), _T('1'), _T('2'), _T('3'), _T('4'), _T('5'), _T('6'), _T('7'),
				_T('8'), _T('9'), _T('A'), _T('B'), _T('C'), _T('D'), _T('E'), _T('F'),
			};

			i <<= 4;
			TCHAR t = sToken.Mid(d,1).MakeUpper().GetAt(0);		// // //
			int h = 0;
			for (h=0; h < 16; ++h)
				if (t == HEX_TEXT[h]) {
					i += h;
					break;
				}
			if (h >= 16)
				throw MakeError(_T("hexadecimal number expected, '%s' found."), sToken);
		}
		return i;
	}

	template <typename... Args>
	auto MakeError(LPCTSTR fmt, Args&&... args) const {		// // //
		CString str = Formatted(_T("Line %d column %d: "), line, GetColumn());
		str.AppendFormat(fmt, args...);
		return std::runtime_error {(LPCTSTR)str};
	}

	stChanNote ImportCellText(unsigned fxMax, chan_id_t chan) {		// // //
		stChanNote Cell;		// // //

		CString sNote = ReadToken();
		if (sNote == _T("...")) { Cell.Note = NONE; }
		else if (sNote == _T("---")) { Cell.Note = HALT; }
		else if (sNote == _T("===")) { Cell.Note = RELEASE; }
		else {
			if (sNote.GetLength() != 3)
				throw MakeError(_T("note column should be 3 characters wide, '%s' found."), sNote);

			if (chan == chan_id_t::NOISE) {		// // // noise
				int h = ImportHex(sNote.Left(1));		// // //
				Cell.Note = (h % NOTE_RANGE) + 1;
				Cell.Octave = h / NOTE_RANGE;

				// importer is very tolerant about the second and third characters
				// in a noise note, they can be anything
			}
			else if (sNote.GetAt(0) == _T('^') && sNote.GetAt(1) == _T('-')) {		// // //
				int o = sNote.GetAt(2) - _T('0');
				if (o < 0 || o > ECHO_BUFFER_LENGTH)
					throw MakeError(_T("out-of-bound echo buffer accessed."));
				Cell.Note = ECHO;
				Cell.Octave = o;
			}
			else {
				int n = 0;
				switch (sNote.GetAt(0)) {
				case _T('c'): case _T('C'): n = 0; break;
				case _T('d'): case _T('D'): n = 2; break;
				case _T('e'): case _T('E'): n = 4; break;
				case _T('f'): case _T('F'): n = 5; break;
				case _T('g'): case _T('G'): n = 7; break;
				case _T('a'): case _T('A'): n = 9; break;
				case _T('b'): case _T('B'): n = 11; break;
				default:
					throw MakeError(_T("unrecognized note '%s'."), sNote);
				}
				switch (sNote.GetAt(1)) {
				case _T('-'): case _T('.'): break;
				case _T('#'): case _T('+'): n += 1; break;
				case _T('b'): case _T('f'): n -= 1; break;
				default:
					throw MakeError(_T("unrecognized note '%s'."), sNote);
				}
				while (n < 0) n += NOTE_RANGE;
				while (n >= NOTE_RANGE) n -= NOTE_RANGE;
				Cell.Note = n + 1;

				int o = sNote.GetAt(2) - _T('0');
				if (o < 0 || o >= OCTAVE_RANGE) {
					throw MakeError(_T("unrecognized octave '%s'."), sNote);
				}
				Cell.Octave = o;
			}
		}

		CString sInst = ReadToken();
		if (sInst == _T("..")) { Cell.Instrument = MAX_INSTRUMENTS; }
		else if (sInst == _T("&&")) { Cell.Instrument = HOLD_INSTRUMENT; }		// // // 050B
		else {
			if (sInst.GetLength() != 2)
				throw MakeError(_T("instrument column should be 2 characters wide, '%s' found."), sInst);
			int h = ImportHex(sInst);		// // //
			if (h >= MAX_INSTRUMENTS)
				throw MakeError(_T("instrument '%s' is out of bounds."), sInst);
			Cell.Instrument = h;
		}

		Cell.Vol = [&] (const CString &str) -> unsigned {
			if (str == _T("."))
				return MAX_VOLUME;
			const LPCTSTR VOL_TEXT[] = {
				_T("0"), _T("1"), _T("2"), _T("3"), _T("4"), _T("5"), _T("6"), _T("7"),
				_T("8"), _T("9"), _T("A"), _T("B"), _T("C"), _T("D"), _T("E"), _T("F"),
			};
			unsigned v = 0;
			for (; v < std::size(VOL_TEXT); ++v)
				if (str == VOL_TEXT[v])
					return v;
			throw MakeError(_T("unrecognized volume token '%s'."), str);
		}(ReadToken().MakeUpper());

		for (unsigned int e = 0; e <= fxMax; ++e) {		// // //
			CString sEff = ReadToken().MakeUpper();
			if (sEff.GetLength() != 3)
				throw MakeError(_T("effect column should be 3 characters wide, '%s' found."), sEff);

			if (sEff != _T("...")) {
				bool Valid;		// // //
				effect_t Eff = GetEffectFromChar(sEff.GetAt(0), GetChipFromChannel(chan), &Valid);
				if (!Valid)
					throw MakeError(_T("unrecognized effect '%s'."), sEff);
				Cell.EffNumber[e] = Eff;

				Cell.EffParam[e] = ImportHex(sEff.Right(2));		// // //
			}
		}

		last_pos_ = pos;
		return Cell;
	}

private:
	bool TrimChar(TCHAR ch) {
		if (!Finished())
			if (TCHAR x = text.GetAt(pos); x == ch) {
				++pos;
				return true;
			}
		return false;
	}

	void ConsumeSpace() {
		while (TrimChar(_T(' ')) || TrimChar(_T('\t')))		// // //
			;
	}

	CString ReadFromFile(LPCTSTR FileName) {
		CString buf;
		CFileException oFileException;
		if (CStdioFile f; f.Open(FileName, CFile::modeRead | CFile::typeText, &oFileException)) {
			CString line;
			while (f.ReadString(line))
				buf += line + _T('\n');
			f.Close();
			return buf;
		}

		TCHAR szError[256];
		oFileException.GetErrorMessage(szError, std::size(szError));
		throw std::runtime_error {std::string {"Unable to open file:\n"} + szError};
	}

public:
	int line = 1;

private:
	const CString text; // TODO: move outside and use string_view
	int pos = 0;
	int linestart = 0;
	int last_pos_ = 0;		// // //
};

// =============================================================================

CString CTextExport::ExportString(const CString& s)
{
	// puts " at beginning and end of string, replace " with ""
	CString r = _T("\"");
	for (int i=0; i < s.GetLength(); ++i)
	{
		TCHAR c = s.GetAt(i);
		if (c == _T('\"'))
			r += c;
		r += c;
	}
	r += _T("\"");
	return r;
}

CString CTextExport::ExportString(std::string_view s)		// // //
{
	// puts " at beginning and end of string, replace " with ""
	CString r = _T("\"");
	for (char c : s) {
		if (c == _T('\"'))
			r += c;
		r += c;
	}
	r += _T("\"");
	return r;
}

// =============================================================================

CString CTextExport::ExportCellText(const stChanNote &stCell, unsigned int nEffects, bool bNoise)		// // //
{
	CString tmp;

	static const LPCTSTR TEXT_NOTE[ECHO+1] = {		// // //
		_T("..."),
		_T("C-?"), _T("C#?"), _T("D-?"), _T("D#?"), _T("E-?"), _T("F-?"),
		_T("F#?"), _T("G-?"), _T("G#?"), _T("A-?"), _T("A#?"), _T("B-?"),
		_T("==="), _T("---"), _T("^-?") };

	CString s = (stCell.Note <= ECHO) ? TEXT_NOTE[stCell.Note] : "...";		// // //
	if (stCell.Note >= NOTE_C && stCell.Note <= NOTE_B || stCell.Note == ECHO)		// // //
	{
		if (bNoise)
		{
			char nNoiseFreq = (stCell.Note - 1 + stCell.Octave * NOTE_RANGE) & 0x0F;
			s.Format(_T("%01X-#"), nNoiseFreq);
		}
		else
		{
			s = s.Left(2);
			tmp.Format(_T("%01d"), stCell.Octave);
			s += tmp;
		}
	}

	tmp.Format(_T(" %02X"), stCell.Instrument);
	s += (stCell.Instrument == MAX_INSTRUMENTS) ? CString(_T(" ..")) :
		(stCell.Instrument == HOLD_INSTRUMENT) ? CString(_T(" &&")) : tmp;		// // // 050B

	tmp.Format(_T(" %01X"), stCell.Vol);
	s += (stCell.Vol == 0x10) ? CString(_T(" .")) : tmp;		// // //

	for (unsigned int e=0; e < nEffects; ++e)
	{
		if (stCell.EffNumber[e] == 0)
		{
			s += _T(" ...");
		}
		else
		{
			tmp.Format(_T(" %c%02X"), EFF_CHAR[stCell.EffNumber[e]-1], stCell.EffParam[e]);
			s += tmp;
		}
	}

	return s;
}

// =============================================================================

#define CHECK_SYMBOL(x) do { \
		if (CString symbol_ = t.ReadToken(); symbol_ != _T(x)) \
			throw t.MakeError(_T("expected '%s', '%s' found."), _T(x), symbol_); \
	} while (false)

#define CHECK_COLON() CHECK_SYMBOL(":")

void CTextExport::ImportFile(LPCTSTR FileName, CFamiTrackerDoc &Doc) {
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
		CString command = t.ReadToken().MakeUpper();		// // //

		int c = 0;
		for (; c < CT_COUNT; ++c)
			if (command == CT[c]) break;

		//DEBUG_OUT("Command read: %s\n", command);
		switch (c) {
		case CT_COMMENTLINE:
			t.FinishLine();
			break;
		case CT_TITLE:
			modfile.SetModuleName(Charify(t.ReadToken()));
			t.ReadEOL();
			break;
		case CT_AUTHOR:
			modfile.SetModuleArtist(Charify(t.ReadToken()));
			t.ReadEOL();
			break;
		case CT_COPYRIGHT:
			modfile.SetModuleCopyright(Charify(t.ReadToken()));
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
			Doc.SelectExpansionChip(CSoundChipSet::FromNSFFlag(flag), modfile.GetNamcoChannels());		// // //
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
			Doc.SelectExpansionChip(modfile.GetSoundChipSet(), MAX_CHANNELS_N163);
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
					throw t.MakeError(_T("macro overflow, max size: %d."), MAX_SEQUENCE_ITEMS);
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
			dpcm_sample->rename(Charify(t.ReadToken()));

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
					throw t.MakeError(_T("DPCM sample %d overflow, increase size used in %s."), dpcm_index, CT[CT_DPCMDEF]);
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
			seqInst->SetName(Charify(t.ReadToken()));
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
			pInst->SetName(Charify(t.ReadToken()));
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
			pInst->SetName(Charify(t.ReadToken()));
			InstManager.InsertInstrument(inst_index, std::move(pInst));
			t.ReadEOL();
		}
		break;
		case CT_KEYDPCM:
		{
			int inst_index = t.ReadInt(0, MAX_INSTRUMENTS - 1);
			if (InstManager.GetInstrumentType(inst_index) != INST_2A03)
				throw t.MakeError(_T("instrument %d is not defined as a 2A03 instrument."), inst_index);
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
				throw t.MakeError(_T("instrument %d is not defined as an FDS instrument."), inst_index);
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
				throw t.MakeError(_T("instrument %d is not defined as an FDS instrument."), inst_index);
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
				throw t.MakeError(_T("instrument %d is not defined as an FDS instrument."), inst_index);
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
					throw t.MakeError(_T("macro overflow, max size: %d."), MAX_SEQUENCE_ITEMS);
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
				throw t.MakeError(_T("instrument %d is not defined as an N163 instrument."), inst_index);
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
				throw t.MakeError(_T("unable to add new track."));

			CSongData *pSong = modfile.GetSong(track);
			pSong->SetPatternLength(t.ReadInt(1, MAX_PATTERN_LENGTH));		// // //
			pSong->SetSongGroove(UseGroove[track]);		// // //
			pSong->SetSongSpeed(t.ReadInt(0, MAX_TEMPO));
			pSong->SetSongTempo(t.ReadInt(0, MAX_TEMPO));
			pSong->SetTitle((LPCTSTR)t.ReadToken());		// // //

			t.ReadEOL();
			++track;
		}
		break;
		case CT_COLUMNS:
		{
			CHECK_COLON();
			CSongData *pSong = modfile.GetSong(track - 1);		// // //
			Doc.ForeachChannel([&] (chan_id_t c) {
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
			Doc.ForeachChannel([&] (chan_id_t c) {
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
				throw t.MakeError(_T("no TRACK defined, cannot add ROW data."));

			int row = t.ReadHex(0, MAX_PATTERN_LENGTH - 1);
			Doc.ForeachChannel([&] (chan_id_t c) {
				CHECK_COLON();
				auto *pTrack = modfile.GetSong(track - 1)->GetTrack(c);		// // //
				pTrack->GetPattern(pattern).SetNoteOn(row, t.ImportCellText(pTrack->GetEffectColumnCount(), c));
			});
			t.ReadEOL();
		}
		break;
		case CT_COUNT:
		default:
			throw t.MakeError(_T("Unrecognized command: '%s'."), command);
		}
	}

	if (dpcm_sample) {
		if (dpcm_sample->size() < dpcm_size)
			dpcm_sample->resize(dpcm_size);
		modfile.GetDSampleManager()->SetDSample(dpcm_index, std::move(dpcm_sample));
	}
	if (N163count != -1)		// // //
		Doc.SelectExpansionChip(modfile.GetSoundChipSet(), N163count); // calls ApplyExpansionChip()
}

// =============================================================================

CString CTextExport::ExportRows(LPCTSTR FileName, const CFamiTrackerModule &modfile) {		// // //
	CStdioFile f;
	CFileException oFileException;
	if (!f.Open(FileName, CFile::modeCreate | CFile::modeWrite | CFile::typeText, &oFileException))
	{
		TCHAR szError[256];
		oFileException.GetErrorMessage(szError, 256);

		return Formatted(_T("Unable to open file:\n%s"), szError);
	}

	f.WriteString(_T("ID,TRACK,CHANNEL,PATTERN,ROW,NOTE,OCTAVE,INST,VOLUME,FX1,FX1PARAM,FX2,FX2PARAM,FX3,FX3PARAM,FX4,FX4PARAM\n"));

	const CString FMT = _T("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n");
	int id = 0;

	modfile.VisitSongs([&] (const CSongData &song, unsigned t) {
		unsigned rows = song.GetPatternLength();
		song.VisitPatterns([&] (const CPatternData &pat, chan_id_t c, unsigned p) {
			if (song.IsPatternInUse(c, p))
				pat.VisitRows(rows, [&] (const stChanNote &stCell, unsigned r) {
					if (stCell != stChanNote { })
						f.WriteString(Formatted(FMT, id++, t, c, p, r,
							stCell.Note, stCell.Octave, stCell.Instrument, stCell.Vol,
							stCell.EffNumber[0], stCell.EffParam[0],
							stCell.EffNumber[1], stCell.EffParam[1],
							stCell.EffNumber[2], stCell.EffParam[2],
							stCell.EffNumber[3], stCell.EffParam[3]));
				});
		});
	});

	return _T("");
}

CString CTextExport::ExportFile(LPCTSTR FileName, CFamiTrackerDoc &Doc) {		// // //
	CStdioFile f;
	CFileException oFileException;
	if (!f.Open(FileName, CFile::modeCreate | CFile::modeWrite | CFile::typeText, &oFileException)) {
		TCHAR szError[256];
		oFileException.GetErrorMessage(szError, 256);

		return Formatted(_T("Unable to open file:\n%s"), szError);
	}

	CString s;
	const auto &modfile = *Doc.GetModule();		// // //

	f.WriteString(Formatted("# 0CC-FamiTracker text export %s\n\n", Get0CCFTVersionString()));		// // //

	f.WriteString("# Module information\n");
	f.WriteString(Formatted("%-15s %s\n", CT[CT_TITLE],     ExportString(modfile.GetModuleName())));
	f.WriteString(Formatted("%-15s %s\n", CT[CT_AUTHOR],    ExportString(modfile.GetModuleArtist())));
	f.WriteString(Formatted("%-15s %s\n", CT[CT_COPYRIGHT], ExportString(modfile.GetModuleCopyright())));
	f.WriteString("\n");

	f.WriteString("# Module comment\n");
	std::string_view sComment = modfile.GetComment();		// // //
	while (true) {
		auto n = sComment.find_first_of("\r\n");
		s.Format("%s %s\n", CT[CT_COMMENT], ExportString(sComment.substr(0, n)));
		f.WriteString(s);
		if (n == std::string_view::npos)
			break;
		sComment.remove_prefix(sComment.find_first_not_of("\r\n", n));
	}
	f.WriteString("\n");

	f.WriteString("# Global settings\n");
	f.WriteString(Formatted("%-15s %d\n", CT[CT_MACHINE],   modfile.GetMachine()));
	f.WriteString(Formatted("%-15s %d\n", CT[CT_FRAMERATE], modfile.GetEngineSpeed()));
	f.WriteString(Formatted("%-15s %d\n", CT[CT_EXPANSION], modfile.GetSoundChipSet().GetNSFFlag()));		// // //
	f.WriteString(Formatted("%-15s %d\n", CT[CT_VIBRATO],   modfile.GetVibratoStyle()));
	f.WriteString(Formatted("%-15s %d\n", CT[CT_SPLIT],     modfile.GetSpeedSplitPoint()));
//	f.WriteString(Formatted("%-15s %d %d\n", CT[CT_PLAYBACKRATE]));
	if (modfile.GetTuningSemitone() || modfile.GetTuningCent())		// // // 050B
		f.WriteString(Formatted("%-15s %d %d\n", CT[CT_TUNING], modfile.GetTuningSemitone(), modfile.GetTuningCent()));
	f.WriteString("\n");

	int N163count = -1;		// // //
	if (modfile.HasExpansionChip(sound_chip_t::N163)) {
		N163count = modfile.GetNamcoChannels();
		Doc.SelectExpansionChip(modfile.GetSoundChipSet(), MAX_CHANNELS_N163); // calls ApplyExpansionChip()
		s.Format(_T("# Namco 163 global settings\n"
		            "%-15s %d\n"
		            "\n"),
		            CT[CT_N163CHANNELS], N163count);
		f.WriteString(s);
	}

	f.WriteString(_T("# Macros\n"));
	const auto &InstManager = *modfile.GetInstrumentManager();
	const inst_type_t CHIP_MACRO[4] = { INST_2A03, INST_VRC6, INST_N163, INST_S5B };
	for (int c=0; c<4; ++c) {
		foreachSeq([&] (sequence_t st) {
			for (int seq = 0; seq < MAX_SEQUENCES; ++seq) {
				const auto pSequence = InstManager.GetSequence(CHIP_MACRO[c], st, seq);
				if (pSequence && pSequence->GetItemCount() > 0) {
					s.Format(_T("%-9s %3d %3d %3d %3d %3d :"),
						CT[CT_MACRO + c],
						st,
						seq,
						pSequence->GetLoopPoint(),
						pSequence->GetReleasePoint(),
						pSequence->GetSetting());
					f.WriteString(s);
					for (unsigned int i = 0; i < pSequence->GetItemCount(); ++i) {
						s.Format(_T(" %d"), pSequence->GetItem(i));
						f.WriteString(s);
					}
					f.WriteString(_T("\n"));
				}
			}
		});
	}
	f.WriteString(_T("\n"));

	f.WriteString(_T("# DPCM samples\n"));
	for (int smp=0; smp < MAX_DSAMPLES; ++smp)
	{
		if (auto pSample = modfile.GetDSampleManager()->GetDSample(smp)) {		// // //
			const unsigned int size = pSample->size();
			s.Format(_T("%s %3d %5d %s\n"),
				CT[CT_DPCMDEF],
				smp,
				size,
				ExportString(pSample->name()));
			f.WriteString(s);

			for (unsigned int i=0; i < size; i += 32)
			{
				s.Format(_T("%s :"), CT[CT_DPCM]);
				f.WriteString(s);
				for (unsigned int j=0; j<32 && (i+j)<size; ++j)
				{
					s.Format(_T(" %02X"), pSample->sample_at(i + j));		// // //
					f.WriteString(s);
				}
				f.WriteString(_T("\n"));
			}
		}
	}
	f.WriteString(_T("\n"));

	f.WriteString(_T("# Detune settings\n"));		// // //
	for (int i = 0; i < 6; i++) for (int j = 0; j < NOTE_COUNT; j++) {
		int Offset = modfile.GetDetuneOffset(i, j);
		if (Offset != 0) {
			s.Format(_T("%s %3d %3d %3d %5d\n"), CT[CT_DETUNE], i, j / NOTE_RANGE, j % NOTE_RANGE, Offset);
			f.WriteString(s);
		}
	}
	f.WriteString(_T("\n"));

	f.WriteString(_T("# Grooves\n"));		// // //
	for (int i = 0; i < MAX_GROOVE; i++) {
		if (const auto pGroove = modfile.GetGroove(i)) {
			s.Format(_T("%s %3d %3d :"), CT[CT_GROOVE], i, pGroove->size());
			f.WriteString(s);
			for (uint8_t entry : *pGroove) {
				s.Format(" %d", entry);
				f.WriteString(s);
			}
			f.WriteString(_T("\n"));
		}
	}
	f.WriteString(_T("\n"));

	f.WriteString(_T("# Tracks using default groove\n"));		// // //
	bool UsedGroove = false;
	modfile.VisitSongs([&] (const CSongData &song) {
		if (song.GetSongGroove())
			UsedGroove = true;
	});
	if (UsedGroove) {
		s.Format(_T("%s :"), CT[CT_USEGROOVE]);
		modfile.VisitSongs([&] (const CSongData &song, unsigned index) {
			if (song.GetSongGroove())
				s.AppendFormat(_T(" %d"), index + 1);
		});
		f.WriteString(s);
		f.WriteString(_T("\n\n"));
	}

	f.WriteString(_T("# Instruments\n"));
	for (unsigned int i=0; i<MAX_INSTRUMENTS; ++i) {
		auto pInst = InstManager.GetInstrument(i);
		if (!pInst) continue;

		LPCTSTR CTstr = nullptr;		// // //
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
		s.Format(_T("%-8s %3d   "), CTstr, i);
		f.WriteString(s);

		if (auto seqInst = std::dynamic_pointer_cast<CSeqInstrument>(pInst)) {
			s.Empty();
			foreachSeq([&] (sequence_t j) {
				s.AppendFormat(_T("%3d "), seqInst->GetSeqEnable(j) ? seqInst->GetSeqIndex(j) : -1);
			});
			f.WriteString(s);
		}

		switch (pInst->GetType())
		{
		case INST_N163:
			{
				auto pDI = std::static_pointer_cast<CInstrumentN163>(pInst);
				s.Format(_T("%3d %3d %3d "),
					pDI->GetWaveSize(),
					pDI->GetWavePos(),
					pDI->GetWaveCount());
				f.WriteString(s);
			}
			break;
		case INST_VRC7:
			{
				auto pDI = std::static_pointer_cast<CInstrumentVRC7>(pInst);
				s.Format(_T("%3d "), pDI->GetPatch());
				for (int j = 0; j < 8; j++)
					s.AppendFormat(_T("%02X "), pDI->GetCustomReg(j));
				f.WriteString(s);
			}
			break;
		case INST_FDS:
			{
				auto pDI = std::static_pointer_cast<CInstrumentFDS>(pInst);
				s.Format(_T("%3d %3d %3d %3d "),
					pDI->GetModulationEnable(),
					pDI->GetModulationSpeed(),
					pDI->GetModulationDepth(),
					pDI->GetModulationDelay());
				f.WriteString(s);
			}
			break;
		}

		f.WriteString(ExportString(pInst->GetName()));
		f.WriteString(_T("\n"));

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
						s.Format(_T("%s %3d %3d %3d   %3d %3d %3d %5d %3d\n"),
							CT[CT_KEYDPCM],
							i,
							oct, key,
							smp - 1,
							pDI->GetSamplePitch(oct, key) & 0x0F,
							pDI->GetSampleLoop(oct, key) ? 1 : 0,
							pDI->GetSampleLoopOffset(oct, key),
							(d >= 0 && d <= 127) ? d : -1);
						f.WriteString(s);
					}
				}
			}
			break;
		case INST_N163:
			{
				auto pDI = std::static_pointer_cast<CInstrumentN163>(pInst);
				for (int w=0; w < pDI->GetWaveCount(); ++w)
				{
					s.Format(_T("%s %3d %3d :"),
						CT[CT_N163WAVE], i, w);
					f.WriteString(s);

					for (int smp : pDI->GetSamples(w)) {		// // //
						s.Format(_T(" %d"), smp);
						f.WriteString(s);
					}
					f.WriteString(_T("\n"));
				}
			}
			break;
		case INST_FDS:
			{
				auto pDI = std::static_pointer_cast<CInstrumentFDS>(pInst);
				s.Format(_T("%-8s %3d :"), CT[CT_FDSWAVE], i);
				f.WriteString(s);
				for (unsigned char smp : pDI->GetSamples()) {		// // //
					s.Format(_T(" %2d"), smp);
					f.WriteString(s);
				}
				f.WriteString(_T("\n"));

				s.Format(_T("%-8s %3d :"), CT[CT_FDSMOD], i);
				f.WriteString(s);
				for (unsigned char m : pDI->GetModTable()) {		// // //
					s.Format(_T(" %2d"), m);
					f.WriteString(s);
				}
				f.WriteString(_T("\n"));

				foreachSeq([&] (sequence_t seq) {
					const auto pSequence = pDI->GetSequence(seq);		// // //
					if (!pSequence || pSequence->GetItemCount() < 1)
						return;

					s.Format(_T("%-8s %3d %3d %3d %3d %3d :"),
						CT[CT_FDSMACRO],
						i,
						seq,
						pSequence->GetLoopPoint(),
						pSequence->GetReleasePoint(),
						pSequence->GetSetting());
					f.WriteString(s);
					for (unsigned int i=0; i < pSequence->GetItemCount(); ++i)
					{
						s.Format(_T(" %d"), pSequence->GetItem(i));
						f.WriteString(s);
					}
					f.WriteString(_T("\n"));
				});
			}
			break;
		}
	}
	f.WriteString(_T("\n"));

	f.WriteString(_T("# Tracks\n\n"));

	modfile.VisitSongs([&] (const CSongData &song) {
		s.Format(_T("%s %3d %3d %3d %s\n"),
			CT[CT_TRACK],
			song.GetPatternLength(),
			song.GetSongSpeed(),
			song.GetSongTempo(),
			ExportString(song.GetTitle()));		// // //
		f.WriteString(s);

		s.Format(_T("%s :"), CT[CT_COLUMNS]);
		f.WriteString(s);
		Doc.ForeachChannel([&] (chan_id_t c) {
			s.Format(_T(" %d"), song.GetEffectColumnCount(c)+1);
			f.WriteString(s);
		});
		f.WriteString(_T("\n\n"));

		for (unsigned int o=0; o < song.GetFrameCount(); ++o) {
			s.Format(_T("%s %02X :"), CT[CT_ORDER], o);
			f.WriteString(s);
			Doc.ForeachChannel([&] (chan_id_t c) {
				s.Format(_T(" %02X"), song.GetFramePattern(o, c));
				f.WriteString(s);
			});
			f.WriteString(_T("\n"));
		}
		f.WriteString(_T("\n"));

		for (int p=0; p < MAX_PATTERN; ++p)
		{
			// detect and skip empty patterns
			bool bUsed = false;
			Doc.ForeachChannel([&] (chan_id_t c) {
				if (!bUsed && !song.GetPattern(c, p).IsEmpty())
					bUsed = true;
			});
			if (!bUsed)
				continue;

			s.Format(_T("%s %02X\n"), CT[CT_PATTERN], p);
			f.WriteString(s);

			for (unsigned int r=0; r < song.GetPatternLength(); ++r) {
				s.Format(_T("%s %02X"), CT[CT_ROW], r);
				f.WriteString(s);
				Doc.ForeachChannel([&] (chan_id_t c) {
					f.WriteString(_T(" : "));
					f.WriteString(ExportCellText(song.GetPattern(c, p).GetNoteOn(r), song.GetEffectColumnCount(c)+1, c==chan_id_t::NOISE));		// // //
				});
				f.WriteString(_T("\n"));
			}
			f.WriteString(_T("\n"));
		}
	});

	if (N163count != -1)		// // //
		Doc.SelectExpansionChip(modfile.GetSoundChipSet(), N163count); // calls ApplyExpansionChip()
	f.WriteString(_T("# End of export\n"));
	Doc.UpdateAllViews(NULL, UPDATE_FRAME);
	Doc.UpdateAllViews(NULL, UPDATE_PATTERN);
	return _T("");
}

// end of file
