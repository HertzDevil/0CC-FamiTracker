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
#include "Bookmark.h"		// // //

#include "FTMComponentInterface.h"		// // //
#include "ChannelMap.h"		// // //

// Defaults when creating new modules
const unsigned CSongData::DEFAULT_ROW_COUNT	= 64;
const char CSongData::DEFAULT_TITLE[] = "New song";		// // //
const stHighlight CSongData::DEFAULT_HIGHLIGHT = {4, 16, 0};		// // //

// This class contains pattern data
// A list of these objects exists inside the document one for each song

CSongData::CSongData(unsigned int PatternLength) :		// // //
	m_iPatternLength(PatternLength)
{
	// // // Pre-allocate pattern 0 for all channels
//	for (int i = 0; i < MAX_CHANNELS; ++i)
//		tracks_[i].m_pPatternData[0].Allocate();		// // //
}

CSongData::~CSongData() {
}

CTrackData *CSongData::GetTrack(chan_id_t chan) {		// // //
	auto idx = value_cast(chan);
	return idx != value_cast(chan_id_t::NONE) ? &tracks_[idx] : nullptr;
}

const CTrackData *CSongData::GetTrack(chan_id_t chan) const {
	return const_cast<CSongData *>(this)->GetTrack(chan);
}

bool CSongData::IsPatternInUse(chan_id_t Channel, unsigned int Pattern) const
{
	// Check if pattern is addressed in frame list
	if (Pattern < MAX_PATTERN)
		if (auto *pTrack = GetTrack(Channel))
			for (unsigned i = 0; i < GetFrameCount(); ++i)
				if (pTrack->GetFramePattern(i) == Pattern)
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

stChanNote CSongData::GetActiveNote(chan_id_t Channel, unsigned Frame, unsigned Row) const {		// // //
	stChanNote Note = GetPatternOnFrame(Channel, Frame).GetNoteOn(Row);
	for (int i = GetEffectColumnCount(Channel) + 1; i < MAX_EFFECT_COLUMNS; ++i)
		Note.EffNumber[i] = EF_NONE;
	return Note;
}

void CSongData::SetPatternData(chan_id_t Channel, unsigned Pattern, unsigned Row, const stChanNote &Note)		// // //
{
	GetPattern(Channel, Pattern).SetNoteOn(Row, Note);
}

CPatternData &CSongData::GetPattern(chan_id_t Channel, unsigned Pattern) {
	auto track = GetTrack(Channel);		// // //
	if (!track)
		throw std::out_of_range {"Bad chan_id_t in " __FUNCTION__};
	return track->GetPattern(Pattern);
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

unsigned CSongData::GetEffectColumnCount(chan_id_t Channel) const
{
	auto track = GetTrack(Channel);		// // //
	return track ? track->GetEffectColumnCount() : 0;
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

void CSongData::SetEffectColumnCount(chan_id_t Channel, unsigned Count)
{
	if (auto track = GetTrack(Channel))		// // //
		track->SetEffectColumnCount(Count);
}

void CSongData::SetSongGroove(bool Groove)		// // //
{
	m_bUseGroove = Groove;
}

unsigned int CSongData::GetFramePattern(unsigned int Frame, chan_id_t Channel) const
{
	auto track = GetTrack(Channel);		// // //
	return track ? track->GetFramePattern(Frame) : MAX_PATTERN;
}

void CSongData::SetFramePattern(unsigned int Frame, chan_id_t Channel, unsigned int Pattern)
{
	if (auto track = GetTrack(Channel))		// // //
		track->SetFramePattern(Frame, Pattern);
}

const stHighlight &CSongData::GetRowHighlight() const
{
	return m_vRowHighlight;
}

void CSongData::SetRowHighlight(const stHighlight &Hl)		// // //
{
	m_vRowHighlight = Hl;
}

void CSongData::PullUp(chan_id_t Chan, unsigned Frame, unsigned Row) {
	auto &Pattern = GetPatternOnFrame(Chan, Frame);
	int PatternLen = GetPatternLength();

	for (int i = Row; i < PatternLen - 1; ++i)
		Pattern.SetNoteOn(i, Pattern.GetNoteOn(i + 1));		// // //
	Pattern.SetNoteOn(PatternLen - 1, { });
}

void CSongData::InsertRow(chan_id_t Chan, unsigned Frame, unsigned Row) {
	auto &Pattern = GetPatternOnFrame(Chan, Frame);		// // //

	for (unsigned int i = GetPatternLength() - 1; i > Row; --i)
		Pattern.SetNoteOn(i, Pattern.GetNoteOn(i - 1));
	Pattern.SetNoteOn(Row, { });
}

void CSongData::CopyTrack(chan_id_t Chan, const CSongData &From, chan_id_t ChanFrom) {
	if (auto *lhs = GetTrack(Chan))
		if (auto *rhs = From.GetTrack(ChanFrom))
			*lhs = *rhs;
}

void CSongData::SwapChannels(chan_id_t First, chan_id_t Second)		// // //
{
	if (auto *lhs = GetTrack(First))
		if (auto *rhs = GetTrack(Second))
			std::swap(*lhs, *rhs);
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
