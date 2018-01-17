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

#include <vector>
#include "ChannelOrder.h"

class CSongData;
class CPatternData;
class CTrackData;

class CSongView {
public:
	CSongView(const CChannelOrder &order, CSongData &song);

	// similar interface as CTrackData
	CPatternData &GetPattern(std::size_t index, unsigned Pattern);
	const CPatternData &GetPattern(std::size_t index, unsigned Pattern) const;

	unsigned int GetFramePattern(std::size_t index, unsigned Frame) const;
	void SetFramePattern(std::size_t index, unsigned Frame, unsigned Pattern);

	int GetEffectColumnCount(std::size_t index) const;
	void SetEffectColumnCount(std::size_t index, int Count);

	// void (*F)(chan_id_t chan)
	template <typename F>
	void ForeachChannel(F f) const {
		return order_.ForeachChannel(f);
	}

private:
	CTrackData *GetTrack(std::size_t index);
	const CTrackData *GetTrack(std::size_t index) const;

	CChannelOrder order_;
	CSongData &song_;
};
