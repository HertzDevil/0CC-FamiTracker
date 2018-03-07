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
#include "FamiTrackerTypes.h"
#include "PatternNote.h"
#ifndef AFL_FUZZ_ENABLED
#include "FamiTrackerEnv.h"
#include "Settings.h"
#endif
#include "NumConv.h"

namespace {

using namespace std::string_view_literals;

constexpr std::string_view NOTE_NAME[] = {
	"C-"sv, "C#"sv, "D-"sv, "D#"sv, "E-"sv, "F-"sv,
	"F#"sv, "G-"sv, "G#"sv, "A-"sv, "A#"sv, "B-"sv,
};

constexpr std::string_view NOTE_NAME_FLAT[] = {
	"C-"sv, "Db"sv, "D-"sv, "Eb"sv, "E-"sv, "F-"sv,
	"Gb"sv, "G-"sv, "Ab"sv, "A-"sv, "Bb"sv, "B-"sv,
};

} // namespace

std::string GetNoteString(note_t note, int octave) {
	using namespace std::string_literals;

	switch (note) {
	case note_t::NONE:
		return "..."s;
	case note_t::HALT:
		return "---"s;
	case note_t::RELEASE:
		return "==="s;
	case note_t::ECHO:
		return "^-"s + std::to_string(octave);
	default:
		if (IsNote(note))
#ifndef AFL_FUZZ_ENABLED
			return std::string((Env.GetSettings()->Appearance.bDisplayFlats ? NOTE_NAME_FLAT : NOTE_NAME)[value_cast(note) - 1]) + std::to_string(octave);
#else
			return std::string((NOTE_NAME)[value_cast(note) - 1]) + std::to_string(octave);
#endif
		return "..."s;
	}
}

std::string GetNoteString(const stChanNote &note) {
	return GetNoteString(note.Note, note.Octave);
}

std::pair<note_t, int> ReadNoteFromString(std::string_view sv) {
	if (sv == "...")
		return {note_t::NONE, 0};
	if (sv == "---")
		return {note_t::HALT, 0};
	if (sv == "===")
		return {note_t::RELEASE, 0};

	auto pre = sv.substr(0, 2);
	if (auto o = conv::to_uint(sv.substr(2))) {
		if (pre == "^-" && *o <= ECHO_BUFFER_LENGTH)
			return {note_t::ECHO, (int)*o};
		for (std::size_t i = 0; i < std::size(NOTE_NAME); ++i)
			if (pre == NOTE_NAME[i] || pre == NOTE_NAME_FLAT[i])
				return {GET_NOTE(i), (int)*o};
	}

	return {note_t::NONE, 0};
}
