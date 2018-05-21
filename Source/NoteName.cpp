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

#include "NoteName.h"
#include "PatternNote.h"
#ifndef AFL_FUZZ_ENABLED
#include "FamiTrackerEnv.h"
#include "Settings.h"
#endif
#include "NumConv.h"
#include "FamiTrackerDefines.h"

namespace {

using namespace std::string_view_literals;

constexpr std::string_view NOTE_NAME[] = {
	"C-", "C#", "D-", "D#", "E-", "F-",
	"F#", "G-", "G#", "A-", "A#", "B-",
};

constexpr std::string_view NOTE_NAME_FLAT[] = {
	"C-", "Db", "D-", "Eb", "E-", "F-",
	"Gb", "G-", "Ab", "A-", "Bb", "B-",
};

} // namespace

std::string GetNoteString(ft0cc::doc::pitch note, int octave) {
	using namespace std::string_literals;

	switch (note) {
	case ft0cc::doc::pitch::none:
		return "..."s;
	case ft0cc::doc::pitch::halt:
		return "---"s;
	case ft0cc::doc::pitch::release:
		return "==="s;
	case ft0cc::doc::pitch::echo:
		return "^-"s + std::to_string(octave);
	default:
		if (is_note(note))
#ifndef AFL_FUZZ_ENABLED
			return std::string((FTEnv.GetSettings()->Appearance.bDisplayFlats ? NOTE_NAME_FLAT : NOTE_NAME)[value_cast(note) - 1]) + std::to_string(octave);
#else
			return std::string((NOTE_NAME)[value_cast(note) - 1]) + std::to_string(octave);
#endif
		return "..."s;
	}
}

std::string GetNoteString(const ft0cc::doc::pattern_note &note) {
	return GetNoteString(note.note(), note.oct());
}

std::pair<ft0cc::doc::pitch, int> ReadNoteFromString(std::string_view sv) {
	if (sv == "...")
		return {ft0cc::doc::pitch::none, 0};
	if (sv == "---")
		return {ft0cc::doc::pitch::halt, 0};
	if (sv == "===")
		return {ft0cc::doc::pitch::release, 0};

	auto pre = sv.substr(0, 2);
	if (auto o = conv::to_uint(sv.substr(2))) {
		if (pre == "^-" && *o < ECHO_BUFFER_LENGTH)
			return {ft0cc::doc::pitch::echo, (int)*o};
		for (std::size_t i = 0; i < std::size(NOTE_NAME); ++i)
			if (pre == NOTE_NAME[i] || pre == NOTE_NAME_FLAT[i])
				return {ft0cc::doc::pitch_from_midi(i), (int)*o};
	}

	return {ft0cc::doc::pitch::none, 0};
}
