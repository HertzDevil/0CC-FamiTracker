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

#include "FamiTrackerTypes.h"
#include "StrongOrdering.h"		// // //
#include <array>		// // //

// // // effect command struct
struct stEffectCommand {
	effect_t fx {effect_t::NONE};
	std::uint8_t param {0u};

	constexpr int compare(const stEffectCommand &other) const noexcept {
		if (fx < other.fx)
			return -1;
		if (fx > other.fx)
			return 1;
		if (fx == effect_t::NONE)
			return 0;
		if (param < other.param)
			return -1;
		if (param > other.param)
			return 1;
		return 0;
	}
};

ENABLE_STRONG_ORDERING(stEffectCommand);

// Channel note struct, holds the data for each row in patterns
class stChanNote {
public:
	constexpr stChanNote() noexcept = default;

	constexpr bool operator==(const stChanNote &other) const noexcept {
		return Note == other.Note && Vol == other.Vol && Instrument == other.Instrument &&
			(Note == note_t::NONE || Octave == other.Octave || Note == note_t::HALT || Note == note_t::RELEASE) &&
			Effects == other.Effects;
	}
	constexpr bool operator!=(const stChanNote &other) const noexcept {
		return !operator==(other);
	}

public:
	note_t Note = note_t::NONE;
	unsigned char Octave = 0U;
	unsigned char Vol = MAX_VOLUME;
	unsigned char Instrument = MAX_INSTRUMENTS;
	std::array<stEffectCommand, MAX_EFFECT_COLUMNS> Effects = { };		// // //
};
