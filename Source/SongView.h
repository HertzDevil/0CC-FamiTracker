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

class CConstSongView {
	friend class CSongView;

public:
	CConstSongView(const CChannelOrder &order, const CSongData &song);

	const CChannelOrder &GetChannelOrder() const;
	const CSongData &GetSong() const;

	const CTrackData *GetTrack(std::size_t index) const;

	const CPatternData &GetPattern(std::size_t index, unsigned Pattern) const;
	const CPatternData &GetPatternOnFrame(std::size_t index, unsigned Frame) const;
	unsigned GetFramePattern(std::size_t index, unsigned Frame) const;
	unsigned GetEffectColumnCount(std::size_t index) const;

	unsigned GetFrameLength(unsigned Frame) const;
	unsigned GetCurrentPatternLength(unsigned Frame) const;

	// void (*F)(std::size_t index)
	template <typename F>
	void ForeachChannel(F f) const {
		for (std::size_t i = 0, n = order_.GetChannelCount(); i < n; ++i)
			f(i);
	}

	// void (*F)(const CTrackData &track [, chan_id_t id])
	template <typename F>
	void ForeachTrack(F f) const {
		if constexpr (std::is_invocable_v<F, const CTrackData &>) {
			for (std::size_t i = 0, n = order_.GetChannelCount(); i < n; ++i)
				f(*GetTrack(i));
		}
		else if constexpr (std::is_invocable_v<F, const CTrackData &, chan_id_t>) {
			for (std::size_t i = 0, n = order_.GetChannelCount(); i < n; ++i)
				f(*GetTrack(i), GetChannelOrder().TranslateChannel(i));
		}
		else
			static_assert(false, "Unknown function signature");
	}

private:
	CChannelOrder order_;
	const CSongData &song_;
};

class CSongView : public CConstSongView {
public:
	CSongView(const CChannelOrder &order, CSongData &song);

	using CConstSongView::GetSong;
	CSongData &GetSong();
	using CConstSongView::GetTrack;
	CTrackData *GetTrack(std::size_t index);
	using CConstSongView::GetPattern;
	CPatternData &GetPattern(std::size_t index, unsigned Pattern);
	using CConstSongView::GetPatternOnFrame;
	CPatternData &GetPatternOnFrame(std::size_t index, unsigned Frame);

	void SetFramePattern(std::size_t index, unsigned Frame, unsigned Pattern);
	void SetEffectColumnCount(std::size_t index, unsigned Count);

	void PullUp(std::size_t index, unsigned Frame, unsigned Row);
	void InsertRow(std::size_t index, unsigned Frame, unsigned Row);
};
