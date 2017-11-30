/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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
#include "../version.h"		// // //

#include "DSample.h"		// // //
#include "Groove.h"		// // //
#include "InstrumentFactory.h"		// // //
#include "Instrument2A03.h"		// // //
#include "InstrumentVRC7.h"		// // //
#include "InstrumentFDS.h"		// // //
#include "InstrumentN163.h"		// // //

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

const char *Charify(CString& s) {		// // //
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

	stChanNote ImportCellText(unsigned fxMax, unsigned chip, bool isNoise) {		// // //
		stChanNote Cell;		// // //

		CString sNote = ReadToken();
		if (sNote == _T("...")) { Cell.Note = NONE; }
		else if (sNote == _T("---")) { Cell.Note = HALT; }
		else if (sNote == _T("===")) { Cell.Note = RELEASE; }
		else {
			if (sNote.GetLength() != 3)
				throw MakeError(_T("note column should be 3 characters wide, '%s' found."), sNote);

			if (isNoise) {		// // // noise
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
				effect_t Eff = GetEffectFromChar(sEff.GetAt(0), chip, &Valid);
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

// =============================================================================

CString CTextExport::ExportCellText(const stChanNote &stCell, unsigned int nEffects, bool bNoise)		// // //
{
	CString tmp;

	static LPCTSTR TEXT_NOTE[ECHO+1] = {		// // //
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
	s += (stCell.Instrument == MAX_INSTRUMENTS) ? _T(" ..") :
		(stCell.Instrument == HOLD_INSTRUMENT) ? _T(" &&") : tmp;		// // // 050B

	tmp.Format(_T(" %01X"), stCell.Vol);
	s += (stCell.Vol == 0x10) ? _T(" .") : tmp;

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

#define CHECK(x) (x)

#define CHECK_SYMBOL(x) do { \
		if (CString symbol_ = t.ReadToken(); symbol_ != _T(x)) \
			return Formatted(_T("Line %d column %d: expected '%s', '%s' found."), t.line, t.GetColumn(), _T(x), symbol_); \
	} while (false)

#define CHECK_COLON() CHECK_SYMBOL(":")

CString CTextExport::ImportFile(LPCTSTR FileName, CFamiTrackerDoc *pDoc) try {
	// begin a new document
	if (!pDoc->OnNewDocument())
		return _T("Unable to create new Famitracker document.");

	// parse the file
	Tokenizer t(FileName);		// // //
	CHECK((void)0);

	int i; // generic integer for reading
	unsigned int dpcm_index = 0;
	unsigned int dpcm_pos = 0;
	CDSample *dpcm_sample = nullptr;
	unsigned int track = 0;
	unsigned int pattern = 0;
	int N163count = -1;		// // //
	bool UseGroove[MAX_TRACKS] = {};		// // //
	while (!t.Finished())
	{
		// read first token on line
		if (t.IsEOL()) continue; // blank line
		CString command = t.ReadToken().MakeUpper();		// // //

		int c = 0;
		for (; c < CT_COUNT; ++c)
			if (command == CT[c]) break;

		//DEBUG_OUT("Command read: %s\n", command);
		switch (c)
		{
			case CT_COMMENTLINE:
				t.FinishLine();
				break;
			case CT_TITLE:
				pDoc->SetModuleName(Charify(t.ReadToken()));
				CHECK(t.ReadEOL());
				break;
			case CT_AUTHOR:
				pDoc->SetModuleArtist(Charify(t.ReadToken()));
				CHECK(t.ReadEOL());
				break;
			case CT_COPYRIGHT:
				pDoc->SetModuleCopyright(Charify(t.ReadToken()));
				CHECK(t.ReadEOL());
				break;
			case CT_COMMENT:
				{
					std::string sComment = pDoc->GetComment();		// // //
					if (!sComment.empty())
						sComment += "\r\n";
					sComment += t.ReadToken();
					pDoc->SetComment(sComment, pDoc->ShowCommentOnOpen());
					CHECK(t.ReadEOL());
				}
				break;
			case CT_MACHINE:
				CHECK(i = t.ReadInt(0,PAL));
				pDoc->SetMachine(static_cast<machine_t>(i));
				CHECK(t.ReadEOL());
				break;
			case CT_FRAMERATE:
				CHECK(i = t.ReadInt(0,800));
				pDoc->SetEngineSpeed(i);
				CHECK(t.ReadEOL());
				break;
			case CT_EXPANSION:
				CHECK(i = t.ReadInt(0,255));
				pDoc->SelectExpansionChip(i);
				CHECK(t.ReadEOL());
				break;
			case CT_VIBRATO:
				CHECK(i = t.ReadInt(0,VIBRATO_NEW));
				pDoc->SetVibratoStyle((vibrato_t)i);
				CHECK(t.ReadEOL());
				break;
			case CT_SPLIT:
				CHECK(i = t.ReadInt(0,255));
				pDoc->SetSpeedSplitPoint(i);
				CHECK(t.ReadEOL());
				break;
			case CT_PLAYBACKRATE:		// // // 050B
				CHECK(i = t.ReadInt(0,2));

				CHECK(i = t.ReadInt(0,0xFFFF));

				CHECK(t.ReadEOL());
				break;
			case CT_TUNING:		// // // 050B
			{
				CHECK(i = t.ReadInt(-12,12));
				int cent;
				CHECK(cent = t.ReadInt(-100,100));
				pDoc->SetTuning(i, cent);
				CHECK(t.ReadEOL());
			}
				break;
			case CT_N163CHANNELS:
				CHECK(i = t.ReadInt(1,8));
				N163count = i;		// // //
				pDoc->SetNamcoChannels(8);
				pDoc->SelectExpansionChip(pDoc->GetExpansionChip());
				CHECK(t.ReadEOL());
				break;
			case CT_MACRO:
			case CT_MACROVRC6:
			case CT_MACRON163:
			case CT_MACROS5B:
				{
					static const inst_type_t CHIP_MACRO[4] = { INST_2A03, INST_VRC6, INST_N163, INST_S5B };		// // //
					int chip = c - CT_MACRO;

					int mt;
					CHECK(mt = t.ReadInt(0,SEQ_COUNT-1));
					CHECK(i = t.ReadInt(0,MAX_SEQUENCES-1));
					CSequence* pSeq = pDoc->GetSequence(CHIP_MACRO[chip], i, mt);

					int loop, release;		// // //
					CHECK(loop = t.ReadInt(-1,MAX_SEQUENCE_ITEMS));
					CHECK(release = t.ReadInt(-1,MAX_SEQUENCE_ITEMS));
					CHECK(i = t.ReadInt(0,255));
					pSeq->SetSetting(static_cast<seq_setting_t>(i));		// // //

					CHECK_COLON();

					int count = 0;
					while (!t.IsEOL()) {
						CHECK(i = t.ReadInt(-128,127));
						if (count >= MAX_SEQUENCE_ITEMS) {
							return Formatted(_T("Line %d column %d: macro overflow, max size: %d."), t.line, t.GetColumn(), MAX_SEQUENCE_ITEMS);
						}
						pSeq->SetItem(count, i);
						++count;
					}
					pSeq->SetItemCount(count);
					pSeq->SetLoopPoint(loop);		// // //
					pSeq->SetReleasePoint(release);
				}
				break;
			case CT_DPCMDEF:
				{
					CHECK(i = t.ReadInt(0,MAX_DSAMPLES-1));
					dpcm_index = i;
					dpcm_pos = 0;

					CHECK(i = t.ReadInt(0,CDSample::MAX_SIZE));
					dpcm_sample = new CDSample();		// // //
					pDoc->SetSample(dpcm_index, dpcm_sample);
					char *blank = new char[i]();
					dpcm_sample->SetData(i, blank);
					dpcm_sample->SetName(Charify(t.ReadToken()));

					CHECK(t.ReadEOL());
				}
				break;
			case CT_DPCM:
				{
					CHECK_COLON();
					while (!t.IsEOL())
					{
						CHECK(i = t.ReadHex(0x00,0xFF));
						if (dpcm_pos >= dpcm_sample->GetSize())
						{
							return Formatted(_T("Line %d column %d: DPCM sample %d overflow, increase size used in %s."), t.line, t.GetColumn(), dpcm_index, CT[CT_DPCMDEF]);
						}
						*(dpcm_sample->GetData() + dpcm_pos) = (char)(i);
						++dpcm_pos;
					}
				}
				break;
			case CT_DETUNE:		// // //
				{
					int oct, note, offset;
					CHECK(i = t.ReadInt(0,5));
					CHECK(oct = t.ReadInt(0,OCTAVE_RANGE-1));
					CHECK(note = t.ReadInt(0,NOTE_RANGE-1));
					CHECK(offset = t.ReadInt(-32768,32767));
					pDoc->SetDetuneOffset(i, oct * NOTE_RANGE + note, offset);
					CHECK(t.ReadEOL());
				}
				break;
			case CT_GROOVE:		// // //
				{
					int size, entry;
					CHECK(i = t.ReadInt(0,MAX_GROOVE-1));
					CHECK(size = t.ReadInt(1,MAX_GROOVE_SIZE));
					auto Groove = std::make_unique<CGroove>();
					Groove->SetSize(size);
					CHECK_COLON();
					for (int j = 0; j < size; j++) {
						CHECK(entry = t.ReadInt(1,255));
						Groove->SetEntry(j, entry);
					}
					pDoc->SetGroove(i, std::move(Groove));
					CHECK(t.ReadEOL());
				}
				break;
			case CT_USEGROOVE:		// // //
				{
					CHECK_COLON();
					int oldTrack = track;
					while (!t.IsEOL()) {
						CHECK(i = t.ReadInt(1,MAX_TRACKS));
						UseGroove[--i] = true;
						if (static_cast<unsigned int>(i) < pDoc->GetTrackCount()) {
							pDoc->SetSongGroove(i, true);
						}
					}
				}
				break;
			case CT_INST2A03:		// // //
			case CT_INSTVRC6:
			case CT_INSTN163:
			case CT_INSTS5B:
				{
					size_t Type = [c] {
						switch (c) {
						case CT_INST2A03: return INST_2A03;
						case CT_INSTVRC6: return INST_VRC6;
						case CT_INSTN163: return INST_N163;
						case CT_INSTS5B:  return INST_S5B;
						}
						return INST_NONE;
					}();
					int inst_index;		// // //
					CHECK(inst_index = t.ReadInt(0,MAX_INSTRUMENTS-1));
					auto pInst = FTExt::InstrumentFactory::Make(Type);		// // //
					auto seqInst = static_cast<CSeqInstrument *>(pInst.get());		// // //
					for (int s=0; s < SEQ_COUNT; ++s)
					{
						CHECK(i = t.ReadInt(-1,MAX_SEQUENCES-1));
						seqInst->SetSeqEnable(s, (i == -1) ? 0 : 1);
						seqInst->SetSeqIndex(s, (i == -1) ? 0 : i);
					}
					if (c == CT_INSTN163) {
						auto pInst = static_cast<CInstrumentN163*>(seqInst);
						CHECK(i = t.ReadInt(0,256-16*N163count));		// // //
						pInst->SetWaveSize(i);
						CHECK(i = t.ReadInt(0,256-16*N163count-1));		// // //
						pInst->SetWavePos(i);
						CHECK(i = t.ReadInt(0,CInstrumentN163::MAX_WAVE_COUNT));
						pInst->SetWaveCount(i);
					}
					seqInst->SetName(Charify(t.ReadToken()));
					pDoc->AddInstrument(std::move(pInst), inst_index);
					CHECK(t.ReadEOL());
				}
				break;
			case CT_INSTVRC7:
				{
					CHECK(i = t.ReadInt(0,MAX_INSTRUMENTS-1));
					auto pInst = std::make_unique<CInstrumentVRC7>();		// // //
					CHECK(i = t.ReadInt(0,15));
					pInst->SetPatch(i);
					for (int r=0; r < 8; ++r)
					{
						CHECK(i = t.ReadHex(0x00,0xFF));
						pInst->SetCustomReg(r, i);
					}
					pInst->SetName(Charify(t.ReadToken()));
					pDoc->AddInstrument(std::move(pInst), i);		// // //
					CHECK(t.ReadEOL());
				}
				break;
			case CT_INSTFDS:
				{
					CHECK(i = t.ReadInt(0,MAX_INSTRUMENTS-1));
					auto pInst = std::make_unique<CInstrumentFDS>();		// // //
					CHECK(i = t.ReadInt(0,1));
					pInst->SetModulationEnable(i==1);
					CHECK(i = t.ReadInt(0,4095));
					pInst->SetModulationSpeed(i);
					CHECK(i = t.ReadInt(0,63));
					pInst->SetModulationDepth(i);
					CHECK(i = t.ReadInt(0,255));
					pInst->SetModulationDelay(i);
					pInst->SetName(Charify(t.ReadToken()));
					pDoc->AddInstrument(std::move(pInst), i);		// // //
					CHECK(t.ReadEOL());
				}
				break;
			case CT_KEYDPCM:
				{
					CHECK(i = t.ReadInt(0,MAX_INSTRUMENTS-1));
					if (pDoc->GetInstrumentType(i) != INST_2A03)
					{
						return Formatted(_T("Line %d column %d: instrument %d is not defined as a 2A03 instrument."), t.line, t.GetColumn(), i);
					}
					auto pInst = std::static_pointer_cast<CInstrument2A03>(pDoc->GetInstrument(i));

					int io, in;
					CHECK(io = t.ReadInt(0,OCTAVE_RANGE));
					CHECK(in = t.ReadInt(0,12));

					CHECK(i = t.ReadInt(0,MAX_DSAMPLES-1));
					pInst->SetSampleIndex(io, in, i+1);
					CHECK(i = t.ReadInt(0,15));
					pInst->SetSamplePitch(io, in, i);
					CHECK(i = t.ReadInt(0,1));
					pInst->SetSampleLoop(io, in, i==1);
					CHECK(i = t.ReadInt(0,255));
					pInst->SetSampleLoopOffset(io, in, i);
					CHECK(i = t.ReadInt(-1,127));
					pInst->SetSampleDeltaValue(io, in, i);
					CHECK(t.ReadEOL());
				}
				break;
			case CT_FDSWAVE:
				{
					CHECK(i = t.ReadInt(0,MAX_INSTRUMENTS-1));
					if (pDoc->GetInstrumentType(i) != INST_FDS)
					{
						return Formatted(_T("Line %d column %d: instrument %d is not defined as an FDS instrument."), t.line, t.GetColumn(), i);
					}
					auto pInst = std::static_pointer_cast<CInstrumentFDS>(pDoc->GetInstrument(i));
					CHECK_COLON();
					for (int s=0; s < CInstrumentFDS::WAVE_SIZE; ++s)
					{
						CHECK(i = t.ReadInt(0,63));
						pInst->SetSample(s, i);
					}
					CHECK(t.ReadEOL());
				}
				break;
			case CT_FDSMOD:
				{
					CHECK(i = t.ReadInt(0,MAX_INSTRUMENTS-1));
					if (pDoc->GetInstrumentType(i) != INST_FDS)
					{
						return Formatted(_T("Line %d column %d: instrument %d is not defined as an FDS instrument."), t.line, t.GetColumn(), i);
					}
					auto pInst = std::static_pointer_cast<CInstrumentFDS>(pDoc->GetInstrument(i));
					CHECK_COLON();
					for (int s=0; s < CInstrumentFDS::MOD_SIZE; ++s)
					{
						CHECK(i = t.ReadInt(0,7));
						pInst->SetModulation(s, i);
					}
					CHECK(t.ReadEOL());
				}
				break;
			case CT_FDSMACRO:
				{
					CHECK(i = t.ReadInt(0,MAX_INSTRUMENTS-1));
					if (pDoc->GetInstrumentType(i) != INST_FDS)
					{
						return Formatted(_T("Line %d column %d: instrument %d is not defined as an FDS instrument."), t.line, t.GetColumn(), i);
					}
					auto pInst = std::static_pointer_cast<CInstrumentFDS>(pDoc->GetInstrument(i));

					CHECK(i = t.ReadInt(0,CInstrumentFDS::SEQUENCE_COUNT-1));
					CSequence *pSeq = new CSequence();		// // //
					pInst->SetSequence(i, pSeq);
					CHECK(i = t.ReadInt(-1,MAX_SEQUENCE_ITEMS));
					pSeq->SetLoopPoint(i);
					CHECK(i = t.ReadInt(-1,MAX_SEQUENCE_ITEMS));
					pSeq->SetReleasePoint(i);
					CHECK(i = t.ReadInt(0,255));
					pSeq->SetSetting(static_cast<seq_setting_t>(i));		// // //

					CHECK_COLON();

					int count = 0;
					while (!t.IsEOL())
					{
						CHECK(i = t.ReadInt(-128,127));
						if (count >= MAX_SEQUENCE_ITEMS)
						{
							return Formatted(_T("Line %d column %d: macro overflow, max size: %d."), t.line, t.GetColumn(), MAX_SEQUENCE_ITEMS);
						}
						pSeq->SetItem(count, i);
						++count;
					}
					pSeq->SetItemCount(count);
				}
				break;
			case CT_N163WAVE:
				{
					CHECK(i = t.ReadInt(0,MAX_INSTRUMENTS-1));
					if (pDoc->GetInstrumentType(i) != INST_N163)
					{
						return Formatted(_T("Line %d column %d: instrument %d is not defined as an N163 instrument."), t.line, t.GetColumn(), i);
					}
					auto pInst = std::static_pointer_cast<CInstrumentN163>(pDoc->GetInstrument(i));

					int iw;
					CHECK(iw = t.ReadInt(0,CInstrumentN163::MAX_WAVE_COUNT-1));
					CHECK_COLON();
					for (int s=0; s < pInst->GetWaveSize(); ++s)
					{
						CHECK(i = t.ReadInt(0,15));
						pInst->SetSample(iw, s, i);
					}
					CHECK(t.ReadEOL());
				}
				break;
			case CT_TRACK:
				{
					if (track != 0)
					{
						if(pDoc->AddTrack() == -1)
						{
							return Formatted(_T("Line %d column %d: unable to add new track."), t.line, t.GetColumn());
						}
					}
					
					CHECK(i = t.ReadInt(1,MAX_PATTERN_LENGTH));		// // //
					pDoc->SetPatternLength(track, i);
					pDoc->SetSongGroove(track, UseGroove[track]);		// // //
					CHECK(i = t.ReadInt(0,MAX_TEMPO));
					pDoc->SetSongSpeed(track, i);
					CHECK(i = t.ReadInt(0,MAX_TEMPO));
					pDoc->SetSongTempo(track, i);
					pDoc->SetTrackTitle(track, (LPCTSTR)t.ReadToken());		// // //

					CHECK(t.ReadEOL());
					++track;
				}
				break;
			case CT_COLUMNS:
				{
					CHECK_COLON();
					for (int c=0; c < pDoc->GetChannelCount(); ++c)
					{
						CHECK(i = t.ReadInt(1,MAX_EFFECT_COLUMNS));
						pDoc->SetEffColumns(track-1,c,i-1);
					}
					CHECK(t.ReadEOL());
				}
				break;
			case CT_ORDER:
				{
					int ifr;
					CHECK(ifr = t.ReadHex(0,MAX_FRAMES-1));
					if (ifr >= (int)pDoc->GetFrameCount(track-1)) // expand to accept frames
					{
						pDoc->SetFrameCount(track-1,ifr+1);
					}
					CHECK_COLON();
					for (int c=0; c < pDoc->GetChannelCount(); ++c)
					{
						CHECK(i = t.ReadHex(0,MAX_PATTERN-1));
						pDoc->SetPatternAtFrame(track-1,ifr, c, i);
					}
					CHECK(t.ReadEOL());
				}
				break;
			case CT_PATTERN:
				CHECK(i = t.ReadHex(0,MAX_PATTERN-1));
				pattern = i;
				CHECK(t.ReadEOL());
				break;
			case CT_ROW:
				{
					if (track == 0)
					{
						return Formatted(_T("Line %d column %d: no TRACK defined, cannot add ROW data."), t.line, t.GetColumn());
					}

					CHECK(i = t.ReadHex(0,MAX_PATTERN_LENGTH-1));
					for (int c=0; c < pDoc->GetChannelCount(); ++c)
					{
						CHECK_COLON();
						stChanNote &&stCell = t.ImportCellText(pDoc->GetEffColumns(track - 1, c),
							pDoc->GetChipType(c), pDoc->GetChannelType(c) == CHANID_NOISE);		// // //
						pDoc->SetDataAtPattern(track - 1, pattern, c, i, std::move(stCell));		// // //
					}
					CHECK(t.ReadEOL());
				}
				break;
			case CT_COUNT:
			default:
				return Formatted(_T("Unrecognized command at line %d: '%s'."), t.line, command);
		}
	}

	if (N163count != -1) {		// // //
		pDoc->SetNamcoChannels(N163count, true);
		pDoc->SelectExpansionChip(pDoc->GetExpansionChip()); // calls ApplyExpansionChip()
	}
	return _T("");
}
catch (std::runtime_error err) {
	return err.what();
}
catch (CString err) {
	return err;
}

// =============================================================================

CString CTextExport::ExportRows(LPCTSTR FileName, CFamiTrackerDoc *pDoc) {		// // //
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

	pDoc->VisitSongs([&] (const CSongData &song, unsigned t) {
		unsigned rows = song.GetPatternLength();
		song.VisitPatterns([&] (const CPatternData &pat, unsigned c, unsigned p) {
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

CString CTextExport::ExportFile(LPCTSTR FileName, CFamiTrackerDoc *pDoc) {		// // //
	CStdioFile f;
	CFileException oFileException;
	if (!f.Open(FileName, CFile::modeCreate | CFile::modeWrite | CFile::typeText, &oFileException))
	{
		TCHAR szError[256];
		oFileException.GetErrorMessage(szError, 256);
		
		return Formatted(_T("Unable to open file:\n%s"), szError);
	}

	CString s;

	s.Format(_T("# 0CC-FamiTracker text export %s\n\n"), Get0CCFTVersionString());		// // //
	f.WriteString(s);

	s.Format(_T("# Module information\n"
	            "%-15s %s\n"
	            "%-15s %s\n"
	            "%-15s %s\n"
	            "\n"),
	            CT[CT_TITLE],     ExportString(pDoc->GetModuleName().data()),
	            CT[CT_AUTHOR],    ExportString(pDoc->GetModuleArtist().data()),
	            CT[CT_COPYRIGHT], ExportString(pDoc->GetModuleCopyright().data()));
	f.WriteString(s);

	f.WriteString(_T("# Module comment\n"));
	CString sComment = pDoc->GetComment().c_str();		// // //
	bool bCommentLines = false;
	do
	{
		int nPos = sComment.Find(_T('\r'));
		bCommentLines = (nPos >= 0);
		if (bCommentLines)
		{
			CString sLine = sComment.Left(nPos);
			s.Format(_T("%s %s\n"), CT[CT_COMMENT], ExportString(sLine));
			f.WriteString(s);
			sComment = sComment.Mid(nPos+2); // +2 skips \r\n
		}
		else
		{
			s.Format(_T("%s %s\n"), CT[CT_COMMENT], ExportString(sComment));
			f.WriteString(s);
		}
	} while (bCommentLines);
	f.WriteString(_T("\n"));

	s.Format(_T("# Global settings\n"
				"%-15s %d\n"
				"%-15s %d\n"
				"%-15s %d\n"
				"%-15s %d\n"
				"%-15s %d\n"
//				"%-15s %d %d\n"		// // // 050B
				),
				CT[CT_MACHINE],   pDoc->GetMachine(),
				CT[CT_FRAMERATE], pDoc->GetEngineSpeed(),
				CT[CT_EXPANSION], pDoc->GetExpansionChip(),
				CT[CT_VIBRATO],   pDoc->GetVibratoStyle(),
				CT[CT_SPLIT],     pDoc->GetSpeedSplitPoint()
//				,CT[CT_PLAYBACKRATE], pDoc->, pDoc->
				);
	if (pDoc->GetTuningSemitone() || pDoc->GetTuningCent())		// // // 050B
		s.AppendFormat(_T("%-15s %d %d\n"), CT[CT_TUNING], pDoc->GetTuningSemitone(), pDoc->GetTuningCent());
	f.WriteString(s);
	f.WriteString(_T("\n"));

	int N163count = -1;		// // //
	if (pDoc->ExpansionEnabled(SNDCHIP_N163))
	{
		N163count = pDoc->GetNamcoChannels();
		pDoc->SetNamcoChannels(8, true);
		pDoc->SelectExpansionChip(pDoc->GetExpansionChip()); // calls ApplyExpansionChip()
		s.Format(_T("# Namco 163 global settings\n"
		            "%-15s %d\n"
		            "\n"),
		            CT[CT_N163CHANNELS], N163count);
		f.WriteString(s);
	}

	f.WriteString(_T("# Macros\n"));
	for (int c=0; c<4; ++c)
	{
		const inst_type_t CHIP_MACRO[4] = { INST_2A03, INST_VRC6, INST_N163, INST_S5B };

		for (int st=0; st < SEQ_COUNT; ++st)
		for (int seq=0; seq < MAX_SEQUENCES; ++seq)
		{
			CSequence* pSequence = pDoc->GetSequence(CHIP_MACRO[c], seq, st);
			if (pSequence && pSequence->GetItemCount() > 0)
			{
				s.Format(_T("%-9s %3d %3d %3d %3d %3d :"),
					CT[CT_MACRO+c],
					st,
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
			}
		}
	}
	f.WriteString(_T("\n"));

	f.WriteString(_T("# DPCM samples\n"));
	for (int smp=0; smp < MAX_DSAMPLES; ++smp)
	{
		if (const CDSample* pSample = pDoc->GetSample(smp))		// // //
		{
			const unsigned int size = pSample->GetSize();
			s.Format(_T("%s %3d %5d %s\n"),
				CT[CT_DPCMDEF],
				smp,
				size,
				ExportString(pSample->GetName()));
			f.WriteString(s);

			for (unsigned int i=0; i < size; i += 32)
			{
				s.Format(_T("%s :"), CT[CT_DPCM]);
				f.WriteString(s);
				for (unsigned int j=0; j<32 && (i+j)<size; ++j)
				{
					s.Format(_T(" %02X"), (unsigned char)(*(pSample->GetData() + (i+j))));
					f.WriteString(s);
				}
				f.WriteString(_T("\n"));
			}
		}
	}
	f.WriteString(_T("\n"));

	f.WriteString(_T("# Detune settings\n"));		// // //
	for (int i = 0; i < 6; i++) for (int j = 0; j < NOTE_COUNT; j++) {
		int Offset = pDoc->GetDetuneOffset(i, j);
		if (Offset != 0) {
			s.Format(_T("%s %3d %3d %3d %5d\n"), CT[CT_DETUNE], i, j / NOTE_RANGE, j % NOTE_RANGE, Offset);
			f.WriteString(s);
		}
	}
	f.WriteString(_T("\n"));
	
	f.WriteString(_T("# Grooves\n"));		// // //
	for (int i = 0; i < MAX_GROOVE; i++) {
		CGroove *Groove = pDoc->GetGroove(i);
		if (Groove != NULL) {
			s.Format(_T("%s %3d %3d :"), CT[CT_GROOVE], i, Groove->GetSize());
			f.WriteString(s);
			for (int j = 0; j < Groove->GetSize(); j++) {
				s.Format(" %d", Groove->GetEntry(j));
				f.WriteString(s);
			}
			f.WriteString(_T("\n"));
		}
	}
	f.WriteString(_T("\n"));
	
	f.WriteString(_T("# Tracks using default groove\n"));		// // //
	bool UsedGroove = false;
	for (unsigned int i = 0; i < pDoc->GetTrackCount(); i++)
		if (pDoc->GetSongGroove(i)) UsedGroove = true;
	if (UsedGroove) {
		s.Format(_T("%s :"), CT[CT_USEGROOVE]);
		f.WriteString(s);
		for (unsigned int i = 0; i < pDoc->GetTrackCount(); i++) if (pDoc->GetSongGroove(i)) {
			s.Format(_T(" %d"), i + 1);
			f.WriteString(s);
		}
		f.WriteString(_T("\n\n"));
	}
	
	f.WriteString(_T("# Instruments\n"));
	for (unsigned int i=0; i<MAX_INSTRUMENTS; ++i)
	{
		auto pInst = pDoc->GetInstrument(i);
		if (!pInst) continue;

		const TCHAR *CTstr = nullptr;		// // //
		switch (pInst->GetType()) {
		case INST_2A03:	CTstr = CT[CT_INST2A03]; break;
		case INST_VRC6:	CTstr = CT[CT_INSTVRC6]; break;
		case INST_VRC7:	CTstr = CT[CT_INSTVRC7]; break;
		case INST_FDS:	CTstr = CT[CT_INSTFDS];  break;
		case INST_N163:	CTstr = CT[CT_INSTN163]; break;
		case INST_S5B:	CTstr = CT[CT_INSTS5B];  break;
		case INST_NONE: default:
			pDoc->GetInstrument(i).reset(); continue;
		}
		s.Format(_T("%-8s %3d   "), CTstr, i);
		f.WriteString(s);

		if (auto seqInst = std::dynamic_pointer_cast<CSeqInstrument>(pInst)) {
			s.Empty();
			for (int j = 0; j < SEQ_COUNT; j++)
				s.AppendFormat(_T("%3d "), seqInst->GetSeqEnable(j) ? seqInst->GetSeqIndex(j) : -1);
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

		f.WriteString(ExportString(pInst->GetName().data()));
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

					for (int smp=0; smp < pDI->GetWaveSize(); ++smp)
					{
						s.Format(_T(" %d"), pDI->GetSample(w, smp));
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
				for (int smp=0; smp < CInstrumentFDS::WAVE_SIZE; ++smp)
				{
					s.Format(_T(" %2d"), pDI->GetSample(smp));
					f.WriteString(s);
				}
				f.WriteString(_T("\n"));

				s.Format(_T("%-8s %3d :"), CT[CT_FDSMOD], i);
				f.WriteString(s);
				for (int smp=0; smp < CInstrumentFDS::MOD_SIZE; ++smp)
				{
					s.Format(_T(" %2d"), pDI->GetModulation(smp));
					f.WriteString(s);
				}
				f.WriteString(_T("\n"));

				for (int seq=0; seq < 3; ++seq)
				{
					const CSequence* pSequence = pDI->GetSequence(seq);		// // //
					if (!pSequence || pSequence->GetItemCount() < 1) continue;

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
				}
			}
			break;
		}
	}
	f.WriteString(_T("\n"));

	f.WriteString(_T("# Tracks\n\n"));

	for (unsigned int t=0; t < pDoc->GetTrackCount(); ++t)
	{
		const char* zpTitle = pDoc->GetTrackTitle(t).c_str();		// // //
		if (zpTitle == NULL) zpTitle = "";

		s.Format(_T("%s %3d %3d %3d %s\n"),
			CT[CT_TRACK],
			pDoc->GetPatternLength(t),
			pDoc->GetSongSpeed(t),
			pDoc->GetSongTempo(t),
			ExportString(zpTitle));
		f.WriteString(s);

		s.Format(_T("%s :"), CT[CT_COLUMNS]);
		f.WriteString(s);
		for (int c=0; c < pDoc->GetChannelCount(); ++c)
		{
			s.Format(_T(" %d"), pDoc->GetEffColumns(t, c)+1);
			f.WriteString(s);
		}
		f.WriteString(_T("\n\n"));

		for (unsigned int o=0; o < pDoc->GetFrameCount(t); ++o)
		{
			s.Format(_T("%s %02X :"), CT[CT_ORDER], o);
			f.WriteString(s);
			for (int c=0; c < pDoc->GetChannelCount(); ++c)
			{
				s.Format(_T(" %02X"), pDoc->GetPatternAtFrame(t, o, c));
				f.WriteString(s);
			}
			f.WriteString(_T("\n"));
		}
		f.WriteString(_T("\n"));

		for (int p=0; p < MAX_PATTERN; ++p)
		{
			// detect and skip empty patterns
			bool bUsed = false;
			for (int c=0; c < pDoc->GetChannelCount(); ++c)
			{
				if (!pDoc->IsPatternEmpty(t, c, p))
				{
					bUsed = true;
					break;
				}
			}
			if (!bUsed) continue;

			s.Format(_T("%s %02X\n"), CT[CT_PATTERN], p);
			f.WriteString(s);

			for (unsigned int r=0; r < pDoc->GetPatternLength(t); ++r)
			{
				s.Format(_T("%s %02X"), CT[CT_ROW], r);
				f.WriteString(s);
				for (int c=0; c < pDoc->GetChannelCount(); ++c)
				{
					f.WriteString(_T(" : "));
					f.WriteString(ExportCellText(pDoc->GetDataAtPattern(t,p,c,r), pDoc->GetEffColumns(t, c)+1, c==3));		// // //
				}
				f.WriteString(_T("\n"));
			}
			f.WriteString(_T("\n"));
		}
	}

	if (N163count != -1) {		// // //
		pDoc->SetNamcoChannels(N163count, true);
		pDoc->SelectExpansionChip(pDoc->GetExpansionChip()); // calls ApplyExpansionChip()
	}
	f.WriteString(_T("# End of export\n"));
	pDoc->UpdateAllViews(NULL, UPDATE_FRAME);
	pDoc->UpdateAllViews(NULL, UPDATE_PATTERN);
	return _T("");
}

// end of file
