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

#include "FamiTrackerTypes.h"
#include "APU/Types.h"

effect_t GetEffectFromChar(char ch, sound_chip_t Chip, bool *bValid)		// // //
{
	effect_t Eff = effect_t::NONE;

	bool Found = false;
	for (int i = value_cast(effect_t::NONE) + 1; i < EFFECT_COUNT; ++i)
		if (EFF_CHAR[i - 1] == ch) {
			Eff = static_cast<effect_t>(i);
			Found = true; break;
		}
	if (!Found) {
		if (bValid != nullptr)
			*bValid = false;
		return Eff;
	}
	else if (bValid != nullptr)
		*bValid = true;

	switch (Chip) {
	case sound_chip_t::FDS:
		for (const auto &x : FDS_EFFECTS)
			if (ch == EFF_CHAR[value_cast(x)])
				return x;
		break;
	case sound_chip_t::N163:
		for (const auto &x : N163_EFFECTS)
			if (ch == EFF_CHAR[value_cast(x)])
				return x;
		break;
	case sound_chip_t::S5B:
		for (const auto &x : S5B_EFFECTS)
			if (ch == EFF_CHAR[value_cast(x)])
				return x;
		break;
	case sound_chip_t::VRC7:
		for (const auto &x : VRC7_EFFECTS)
			if (ch == EFF_CHAR[value_cast(x)])
				return x;
		break;
	}

	return Eff;
}
