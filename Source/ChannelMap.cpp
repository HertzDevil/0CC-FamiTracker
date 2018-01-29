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

#include "ChannelMap.h"
#include "TrackerChannel.h"
#include "APU/Types.h"		// // //

CChannelMap::CChannelMap(const CSoundChipSet &chips, unsigned n163chs) :
	chips_(chips), n163chs_(n163chs)
{
}

CChannelOrder &CChannelMap::GetChannelOrder() {
	return order_;
}

const CChannelOrder &CChannelMap::GetChannelOrder() const {
	return order_;
}

bool CChannelMap::SupportsChannel(chan_id_t ch) const {		// // //
	sound_chip_t chip = GetChipFromChannel(ch);
	return HasExpansionChip(chip) && !(chip == sound_chip_t::N163 &&
		GetChannelSubIndex(ch) >= GetChipChannelCount(sound_chip_t::N163));
}

const CSoundChipSet &CChannelMap::GetExpansionFlag() const noexcept {		// // //
	return chips_;
}

unsigned CChannelMap::GetChipChannelCount(sound_chip_t chip) const {
	if (HasExpansionChip(chip))
		switch (chip) {
		case sound_chip_t::N163: return n163chs_;
		}
	return 0;
}

bool CChannelMap::HasExpansionChip(sound_chip_t chips) const noexcept {
	return GetExpansionFlag().ContainsChip(chips);
}
