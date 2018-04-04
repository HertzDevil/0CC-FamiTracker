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

#include <vector>		// // //
#include "ChannelOrder.h"		// // //
#include "SoundChipSet.h"		// // //

class CTrackerChannel;		// // //

// CChannelMap

class CChannelMap
{
public:
	CChannelMap() = default;		// // //
	CChannelMap(CSoundChipSet chips, unsigned n163chs);

	CChannelOrder &GetChannelOrder();
	const CChannelOrder &GetChannelOrder() const;

	bool			SupportsChannel(stChannelID ch) const;		// // //

	CSoundChipSet	GetExpansionFlag() const noexcept;		// // //
	unsigned		GetChipChannelCount(sound_chip_t chip) const;
	bool			HasExpansionChip(sound_chip_t chip) const noexcept; // all

private:		// // //
	CChannelOrder order_;		// // //

	CSoundChipSet chips_;		// // //
	unsigned n163chs_ = 0u;		// // //
};
