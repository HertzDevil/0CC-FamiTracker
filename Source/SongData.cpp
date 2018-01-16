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

#include "SongData.h"
#include "PatternData.h"		// // //
#include "Bookmark.h"		// // //

#include "FTMComponentInterface.h"		// // //
#include "ChannelMap.h"		// // //

// Defaults when creating new modules
const unsigned CSongData::DEFAULT_ROW_COUNT	= 64;
const char CSongData::DEFAULT_TITLE[] = "New song";		// // //
const stHighlight CSongData::DEFAULT_HIGHLIGHT = {4, 16, 0};		// // //

// This class contains pattern data
// A list of these objects exists inside the document one for each song

CSongData::CSongData(CFTMComponentInterface &parent, unsigned int PatternLength) :		// // //
	parent_(parent),
	m_iPatternLength(PatternLength)
{
	// // // Pre-allocate pattern 0 for all channels
//	for (int i = 0; i < MAX_CHANNELS; ++i)
//		m_pPatternData[i][0].Allocate();		// // //
}

CSongData::~CSongData() {
}

bool CSongData::IsPatternInUse(chan_id_t Channel, unsigned int Pattern) const
{
	// Check if pattern is addressed in frame list
	for (unsigned i = 0; i < m_iFrameCount; ++i)
		if (GetFramePattern(i, Channel) == Pattern)
			return true;
	return false;
}

unsigned CSongData::GetFreePatternIndex(chan_id_t Channel, unsigned Whence) const {		// // //
	while (++Whence < MAX_PATTERN)
		if (!IsPatternInUse(Channel, Whence) && GetPattern(Channel, Whence).IsEmpty())
			return Whence;

	return -1;
}

stChanNote &CSongData::GetPatternData(chan_id_t Channel, unsigned Pattern, unsigned Row)		// // //
{
	return GetPattern(Channel, Pattern).GetNoteOn(Row);
}

const stChanNote &CSongData::GetPatternData(chan_id_t Channel, unsigned Pattern, unsigned Row) const		// // //
{
	return GetPattern(Channel, Pattern).GetNoteOn(Row);
}

void CSongData::SetPatternData(chan_id_t Channel, unsigned Pattern, unsigned Row, const stChanNote &Note)		// // //
{
	GetPattern(Channel, Pattern).SetNoteOn(Row, Note);
}

CPatternData &CSongData::GetPattern(chan_id_t Channel, unsigned Pattern) {
	auto idx = value_cast(Channel);		// // //
	if (idx == value_cast(chan_id_t::NONE))
		throw std::out_of_range {"Bad chan_id_t in " __FUNCTION__};
	return m_pPatternData[idx][Pattern];
}

const CPatternData &CSongData::GetPattern(chan_id_t Channel, unsigned Pattern) const {
	return const_cast<CSongData *>(this)->GetPattern(Channel, Pattern);
}

CPatternData &CSongData::GetPatternOnFrame(chan_id_t Channel, unsigned Frame) {
	return GetPattern(Channel, GetFramePattern(Frame, Channel));
}

const CPatternData &CSongData::GetPatternOnFrame(chan_id_t Channel, unsigned Frame) const {
	return const_cast<CSongData *>(this)->GetPatternOnFrame(Channel, Frame);
}

std::string_view CSongData::GetTitle() const		// // //
{
	return m_sTrackName;
}

unsigned int CSongData::GetPatternLength() const
{
	return m_iPatternLength;
}

unsigned int CSongData::GetFrameCount() const
{
	return m_iFrameCount;
}

unsigned int CSongData::GetSongSpeed() const
{
	return m_iSongSpeed;
}

unsigned int CSongData::GetSongTempo() const
{
	return m_iSongTempo;
}

int CSongData::GetEffectColumnCount(chan_id_t Channel) const
{
	auto idx = value_cast(Channel);		// // //
	return idx != value_cast(chan_id_t::NONE) ? m_iEffectColumns[idx] : 0;
}

bool CSongData::GetSongGroove() const		// // //
{
	return m_bUseGroove;
}

void CSongData::SetTitle(std::string_view str)		// // //
{
	m_sTrackName = str;
}

void CSongData::SetPatternLength(unsigned int Length)
{
	m_iPatternLength = Length;
}

void CSongData::SetFrameCount(unsigned int Count)
{
	m_iFrameCount = Count;
}

void CSongData::SetSongSpeed(unsigned int Speed)
{
	m_iSongSpeed = Speed;
}

void CSongData::SetSongTempo(unsigned int Tempo)
{
	m_iSongTempo = Tempo;
}

void CSongData::SetEffectColumnCount(chan_id_t Channel, int Count)
{
	if (auto idx = value_cast(Channel); idx != value_cast(chan_id_t::NONE))		// // //
		m_iEffectColumns[idx] = Count;
}

void CSongData::SetSongGroove(bool Groove)		// // //
{
	m_bUseGroove = Groove;
}

unsigned int CSongData::GetFramePattern(unsigned int Frame, chan_id_t Channel) const
{
	auto idx = value_cast(Channel);		// // //
	return idx != value_cast(chan_id_t::NONE) ? m_iFrameList[Frame][idx] : MAX_PATTERN;
}

void CSongData::SetFramePattern(unsigned int Frame, chan_id_t Channel, unsigned int Pattern)
{
	if (auto idx = value_cast(Channel); idx != value_cast(chan_id_t::NONE))		// // //
		m_iFrameList[Frame][idx] = Pattern;
}

const stHighlight &CSongData::GetRowHighlight() const
{
	return m_vRowHighlight;
}

void CSongData::SetRowHighlight(const stHighlight &Hl)		// // //
{
	m_vRowHighlight = Hl;
}

void CSongData::CopyTrack(chan_id_t Chan, const CSongData &From, chan_id_t ChanFrom) {
	SetEffectColumnCount(Chan, From.GetEffectColumnCount(ChanFrom));
	for (int f = 0; f < MAX_FRAMES; f++)
		SetFramePattern(f, Chan, From.GetFramePattern(f, ChanFrom));
	for (int p = 0; p < MAX_PATTERN; p++)
		GetPattern(Chan, p) = From.GetPattern(ChanFrom, p);
}

void CSongData::SwapChannels(chan_id_t First, chan_id_t Second)		// // //
{
	auto lhs = value_cast(First);
	auto rhs = value_cast(Second);
	if (lhs != value_cast(chan_id_t::NONE) && rhs != value_cast(chan_id_t::NONE)) {
		std::swap(m_iEffectColumns[lhs], m_iEffectColumns[rhs]);
		for (int i = 0; i < MAX_FRAMES; i++)
			std::swap(m_iFrameList[i][lhs], m_iFrameList[i][rhs]);
		std::swap(m_pPatternData[lhs], m_pPatternData[rhs]);
	}
}

CBookmarkCollection &CSongData::GetBookmarks() {
	return bookmarks_;
}

const CBookmarkCollection &CSongData::GetBookmarks() const {
	return bookmarks_;
}

void CSongData::SetBookmarks(const CBookmarkCollection &bookmarks) {
	bookmarks_.ClearBookmarks();
	for (const auto &bm : bookmarks)
		bookmarks_.AddBookmark(std::make_unique<CBookmark>(*bm));
}

void CSongData::SetBookmarks(CBookmarkCollection &&bookmarks) {
	bookmarks_ = std::move(bookmarks);
}

unsigned CSongData::GetChannelPosition(chan_id_t ChanID) const {
	return parent_.GetChannelMap()->GetChannelIndex(ChanID);
}
