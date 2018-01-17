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

#include "SongView.h"
#include "SongData.h"
#include "TrackData.h"

CSongView::CSongView(const CChannelOrder &order, CSongData &song) :
	order_(order), song_(song)
{
}

const CChannelOrder &CSongView::GetChannelOrder() const {
	return order_;
}

CSongData &CSongView::GetSong() {
	return song_;
}

const CSongData &CSongView::GetSong() const {
	return song_;
}

CPatternData &CSongView::GetPattern(std::size_t index, unsigned Pattern) {
	auto pTrack = GetTrack(index);		// // //
	if (!pTrack)
		throw std::out_of_range {"Bad chan_id_t in " __FUNCTION__};
	return pTrack->GetPattern(Pattern);
}

const CPatternData &CSongView::GetPattern(std::size_t index, unsigned Pattern) const {
	return const_cast<CSongView *>(this)->GetPattern(index, Pattern);
}

CPatternData &CSongView::GetPatternOnFrame(std::size_t index, unsigned Frame) {
	return GetPattern(index, GetFramePattern(index, Frame));
}

const CPatternData &CSongView::GetPatternOnFrame(std::size_t index, unsigned Frame) const {
	return GetPattern(index, GetFramePattern(index, Frame));
}

unsigned int CSongView::GetFramePattern(std::size_t index, unsigned Frame) const {
	auto pTrack = GetTrack(index);
	return pTrack ? pTrack->GetFramePattern(Frame) : MAX_PATTERN;
}

void CSongView::SetFramePattern(std::size_t index, unsigned Frame, unsigned Pattern) {
	if (auto pTrack = GetTrack(index))
		pTrack->SetFramePattern(Frame, Pattern);
}

unsigned CSongView::GetEffectColumnCount(std::size_t index) const {
	auto pTrack = GetTrack(index);
	return pTrack ? pTrack->GetEffectColumnCount() : 0;
}

void CSongView::SetEffectColumnCount(std::size_t index, unsigned Count) {
	if (auto pTrack = GetTrack(index))
		pTrack->SetEffectColumnCount(Count);
}

CTrackData *CSongView::GetTrack(std::size_t index) {
	return song_.GetTrack(order_.TranslateChannel(index));
}

const CTrackData *CSongView::GetTrack(std::size_t index) const {
	return song_.GetTrack(order_.TranslateChannel(index));
}
