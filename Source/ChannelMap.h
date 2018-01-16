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
#include <unordered_map>		// // //
#include "APU/Types_fwd.h"		// // //

class CTrackerChannel;		// // //

// CChannelMap

class CChannelMap
{
public:
	CChannelMap();		// // //
	CChannelMap(sound_chip_t chips, unsigned n163chs);

	void			ResetChannels();
	void			RegisterChannel(CTrackerChannel &Channel);		// // //
	bool			SupportsChannel(const CTrackerChannel &ch) const;		// // //

	CTrackerChannel	&GetChannel(int index) const;		// // //
	int				GetChannelIndex(chan_id_t chan) const;
	bool			HasChannel(chan_id_t chan) const;		// // //
	int				GetChannelCount() const;		// // //
	chan_id_t		GetChannelType(int index) const;		// // //
	sound_chip_t	GetChipType(int index) const;

	sound_chip_t	GetExpansionFlag() const noexcept;		// // //
	unsigned		GetChipChannelCount(sound_chip_t chip) const;
	bool			HasExpansionChip(sound_chip_t chips) const noexcept; // all

	template <typename F>
	void ForeachChannel(F f) const {
		for (std::size_t i = 0, n = m_pChannels.size(); i < n; ++i)
			f(GetChannelType(i));
	}

private:		// // //
	std::vector<CTrackerChannel *> m_pChannels;		// // //
	std::unordered_map<chan_id_t, int> m_iChannelIndices;		// // //

	sound_chip_t chips_;		// // //
	unsigned n163chs_;		// // //
};
