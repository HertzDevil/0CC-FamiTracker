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

enum class highlight_state_t : unsigned char {		// // //
	none, beat, measure,
};

// // // Highlight settings
struct stHighlight {
	int First;
	int Second;
	int Offset = 0;

	constexpr bool operator==(const stHighlight &other) const noexcept {
		return First == other.First && Second == other.Second && Offset == other.Offset;
	}

	constexpr bool operator!=(const stHighlight &other) const noexcept {
		return !(*this == other);
	}
};
