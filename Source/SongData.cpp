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
#include "ChannelOrder.h"		// // //

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
		throw std::out_of_range {"Bad chan_id_t in CSongData::GetPattern(chan_id_t, unsigned)"};
	return track->GetPattern(Pattern);
}

const CPatternData &CSongData::GetPattern(chan_id_t Channel, unsigned Pattern) const {
	return const_cast<CSongData *>(this)->GetPattern(Channel, Pattern);
}

CPatternData &CSongData::GetPatternOnFrame(chan_id_t Channel, unsigned Frame) {
	auto track = GetTrack(Channel);		// // //
	if (!track)
		throw std::out_of_range {"Bad chan_id_t in CSongData::GetPattern(chan_id_t, unsigned)"};
	return track->GetPatternOnFrame(Frame);
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

stHighlight CSongData::GetHighlightAt(unsigned Frame, unsigned Row) const {		// // //
	while (Frame < 0)
		Frame += GetFrameCount();
	Frame %= GetFrameCount();

	stHighlight Hl = GetRowHighlight();

	const CBookmark Zero { };
	const CBookmarkCollection &Col = GetBookmarks();
	if (const unsigned Count = Col.GetCount()) {
		CBookmark tmp(Frame, Row);
		unsigned int Min = tmp.Distance(Zero);
		for (unsigned i = 0; i < Count; ++i) {
			CBookmark *pMark = Col.GetBookmark(i);
			unsigned Dist = tmp.Distance(*pMark);
			if (Dist <= Min) {
				Min = Dist;
				if (pMark->m_Highlight.First != -1 && (pMark->m_bPersist || pMark->m_iFrame == Frame))
					Hl.First = pMark->m_Highlight.First;
				if (pMark->m_Highlight.Second != -1 && (pMark->m_bPersist || pMark->m_iFrame == Frame))
					Hl.Second = pMark->m_Highlight.Second;
				Hl.Offset = pMark->m_Highlight.Offset + pMark->m_iRow;
			}
		}
	}

	return Hl;
}

highlight_state_t CSongData::GetHighlightState(unsigned Frame, unsigned Row) const {		// // //
	stHighlight Hl = GetHighlightAt(Frame, Row);
	if (Hl.Second > 0 && !((Row - Hl.Offset) % Hl.Second))
		return highlight_state_t::measure;
	if (Hl.First > 0 && !((Row - Hl.Offset) % Hl.First))
		return highlight_state_t::beat;
	return highlight_state_t::none;
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

bool CSongData::AddFrames(unsigned Frame, unsigned Count) {
	const unsigned FrameCount = GetFrameCount();
	if (FrameCount + Count > MAX_FRAMES)
		return false;

	SetFrameCount(FrameCount + Count);
	VisitTracks([&] (CTrackData &track) {
		for (unsigned int i = FrameCount + Count - 1; i >= Frame + Count; --i)
			track.SetFramePattern(i, track.GetFramePattern(i - Count));
		for (unsigned i = 0; i < Count; ++i)		// // //
			track.SetFramePattern(Frame + i, 0);
	});

	GetBookmarks().InsertFrames(Frame, Count);		// // //
	return true;
}

bool CSongData::DeleteFrames(unsigned Frame, unsigned Count) {
	const unsigned FrameCount = GetFrameCount();
	if (Frame >= FrameCount)
		return true;
	if (Count > FrameCount - Frame)
		Count = FrameCount - Frame;
	if (Count >= FrameCount)
		return false;

	VisitTracks([&] (CTrackData &track) {
		for (unsigned i = Frame; i < FrameCount - Count; ++i)
			track.SetFramePattern(i, track.GetFramePattern(i + 1));
		for (unsigned i = FrameCount - Count; i < FrameCount; ++i)
			track.SetFramePattern(i, 0);		// // //
	});
	SetFrameCount(FrameCount - Count);

	GetBookmarks().RemoveFrames(Frame, Count);		// // //
	return true;
}

bool CSongData::SwapFrames(unsigned First, unsigned Second) {
	const unsigned FrameCount = GetFrameCount();
	if (First >= FrameCount || Second >= FrameCount)
		return false;

	VisitTracks([&] (CTrackData &track) {
		auto Pattern = track.GetFramePattern(First);
		track.SetFramePattern(First, track.GetFramePattern(Second));
		track.SetFramePattern(Second, Pattern);
	});

	GetBookmarks().SwapFrames(First, Second);		// // //
	return true;
}

bool CSongData::InsertFrame(unsigned Frame) {
	if (!AddFrames(Frame, 1))
		return false;

	// Select free patterns
	VisitTracks([&] (CTrackData &track, chan_id_t ch) {
		unsigned Pattern = GetFreePatternIndex(ch);		// // //
		track.SetFramePattern(Frame, Pattern < MAX_PATTERN ? Pattern : 0);
	});

	return true;
}

bool CSongData::DuplicateFrame(unsigned Frame) {
	if (!AddFrames(Frame + 1, 1))		// // //
		return false;

	VisitTracks([&] (CTrackData &track) {
		track.SetFramePattern(Frame + 1, track.GetFramePattern(Frame));
	});

	return true;
}

bool CSongData::CloneFrame(unsigned Frame) {
	// insert new frame with next free pattern numbers
	if (!InsertFrame(Frame))
		return false;

	// copy old patterns into new
	VisitTracks([&] (CTrackData &, chan_id_t ch) {
		GetPatternOnFrame(ch, Frame) = GetPatternOnFrame(ch, Frame - 1);
	});

	return true;
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
