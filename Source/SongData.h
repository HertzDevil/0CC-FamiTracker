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

#include <array>		// // //
#include <string>		// // //
#include "FamiTrackerTypes.h"		// // //
#include "APU/Types.h"		// // //
#include "TrackData.h"		// // //
#include "Highlight.h"		// // //
#include "BookmarkCollection.h"		// // //

class stChanNote;		// // //
class CFTMComponentInterface;		// // //

// CSongData holds all notes in the patterns
class CSongData
{
public:
	explicit CSongData(unsigned int PatternLength);		// // //
	~CSongData();

	CTrackData *GetTrack(chan_id_t chan);		// // //
	const CTrackData *GetTrack(chan_id_t chan) const;

	bool IsPatternInUse(chan_id_t Channel, unsigned int Pattern) const;

	unsigned GetFreePatternIndex(chan_id_t Channel, unsigned Whence = (unsigned)-1) const;		// // //

	stChanNote &GetPatternData(chan_id_t Channel, unsigned Pattern, unsigned Row);		// // //
	const stChanNote &GetPatternData(chan_id_t Channel, unsigned Pattern, unsigned Row) const;		// // //
	void SetPatternData(chan_id_t Channel, unsigned Pattern, unsigned Row, const stChanNote &Note);		// // //

	CPatternData &GetPattern(chan_id_t Channel, unsigned Pattern);		// // //
	const CPatternData &GetPattern(chan_id_t Channel, unsigned Pattern) const;		// // //
	CPatternData &GetPatternOnFrame(chan_id_t Channel, unsigned Frame);		// // //
	const CPatternData &GetPatternOnFrame(chan_id_t Channel, unsigned Frame) const;		// // //

	stChanNote GetActiveNote(chan_id_t Channel, unsigned Frame, unsigned Row) const;		// // //

	std::string_view GetTitle() const;		// // //
	unsigned int GetPatternLength() const;
	unsigned int GetFrameCount() const;
	unsigned int GetSongSpeed() const;
	unsigned int GetSongTempo() const;
	unsigned GetEffectColumnCount(chan_id_t Channel) const;
	bool GetSongGroove() const;		// // //

	void SetTitle(std::string_view str);		// // //
	void SetPatternLength(unsigned int Length);
	void SetFrameCount(unsigned int Count);
	void SetSongSpeed(unsigned int Speed);
	void SetSongTempo(unsigned int Tempo);
	void SetEffectColumnCount(chan_id_t Channel, unsigned Count);
	void SetSongGroove(bool Groove);		// // //

	unsigned int GetFramePattern(unsigned int Frame, chan_id_t Channel) const;
	void SetFramePattern(unsigned int Frame, chan_id_t Channel, unsigned int Pattern);

	const stHighlight &GetRowHighlight() const;
	void SetRowHighlight(const stHighlight &Hl);		// // //

	stHighlight GetHighlightAt(unsigned Frame, unsigned Row) const;		// // //
	highlight_state_t GetHighlightState(unsigned Frame, unsigned Row) const;		// // //

	void PullUp(chan_id_t Chan, unsigned Frame, unsigned Row);
	void InsertRow(chan_id_t Chan, unsigned Frame, unsigned Row);
	void CopyTrack(chan_id_t Chan, const CSongData &From, chan_id_t ChanFrom);		// // //
	void SwapChannels(chan_id_t First, chan_id_t Second);		// // //

	// // // Frame operations

	bool AddFrames(unsigned Frame, unsigned Count);
	bool DeleteFrames(unsigned Frame, unsigned Count);
	bool SwapFrames(unsigned First, unsigned Second);
	bool InsertFrame(unsigned Frame);
	bool DuplicateFrame(unsigned Frame);
	bool CloneFrame(unsigned Frame);

	CBookmarkCollection &GetBookmarks();		// // //
	const CBookmarkCollection &GetBookmarks() const;
	void SetBookmarks(const CBookmarkCollection &bookmarks);
	void SetBookmarks(CBookmarkCollection &&bookmarks);

	// void (*F)(CTrackData &track [, chan_id_t ch])
	template <typename F>
	void VisitTracks(F f) {
		if constexpr (std::is_invocable_v<F, CTrackData &, chan_id_t>) {
			unsigned ch_pos = 0;
			for (auto &track : tracks_) {
				f(track, (chan_id_t)ch_pos);
				++ch_pos;
			}
		}
		else if constexpr (std::is_invocable_v<F, CTrackData &>) {
			for (auto &track : tracks_)
				f(track);
		}
		else
			static_assert(sizeof(F) == 0, "Unknown function signature");
	}

	// void (*F)(const CTrackData &track [, chan_id_t ch])
	template <typename F>
	void VisitTracks(F f) const {
		if constexpr (std::is_invocable_v<F, const CTrackData &, chan_id_t>) {
			unsigned ch_pos = 0;
			for (auto &track : tracks_) {
				f(track, (chan_id_t)ch_pos);
				++ch_pos;
			}
		}
		else if constexpr (std::is_invocable_v<F, const CTrackData &>) {
			for (auto &track : tracks_)
				f(track);
		}
		else
			static_assert(sizeof(F) == 0, "Unknown function signature");
	}

	// void (*F)(CPatternData &pattern [, chan_id_t ch, std::size_t p_index])
	template <typename F>
	void VisitPatterns(F f) {		// // //
		if constexpr (std::is_invocable_v<F, CPatternData &, chan_id_t, std::size_t>)
			VisitTracks([&] (CTrackData &track, chan_id_t ch) {
				track.VisitPatterns([&] (CPatternData &pattern, std::size_t p_index) {
					f(pattern, ch, p_index);
				});
			});
		else if constexpr (std::is_invocable_v<F, CPatternData &>)
			VisitTracks([&] (CTrackData &track) {
				track.VisitPatterns(f);
			});
		else
			static_assert(sizeof(F) == 0, "Unknown function signature");
	}

	// void (*F)(const CPatternData &pattern [, chan_id_t ch, std::size_t p_index])
	template <typename F>
	void VisitPatterns(F f) const {
		if constexpr (std::is_invocable_v<F, const CPatternData &, chan_id_t, std::size_t>)
			VisitTracks([&] (const CTrackData &track, chan_id_t ch) {
				track.VisitPatterns([&] (const CPatternData &pattern, std::size_t p_index) {
					f(pattern, ch, p_index);
				});
			});
		else if constexpr (std::is_invocable_v<F, const CPatternData &>)
			VisitTracks([&] (const CTrackData &track) {
				track.VisitPatterns(f);
			});
		else
			static_assert(sizeof(F) == 0, "Unknown function signature");
	}

public:
	// // // moved from CFamiTrackerDoc
	static const char DEFAULT_TITLE[];
	static const stHighlight DEFAULT_HIGHLIGHT;
	static const unsigned DEFAULT_ROW_COUNT;

private:
	// Track parameters
	std::string	 m_sTrackName = DEFAULT_TITLE;			// // // moved
	unsigned int m_iPatternLength;						// Amount of rows in one pattern
	unsigned int m_iFrameCount = 1;						// Number of frames
	unsigned int m_iSongSpeed = DEFAULT_SPEED;			// Song speed
	unsigned int m_iSongTempo = DEFAULT_TEMPO_NTSC;		// Song tempo
	bool		 m_bUseGroove = false;					// // // Groove

	// Row highlight settings
	stHighlight  m_vRowHighlight = DEFAULT_HIGHLIGHT;		// // //

	// Bookmarks
	CBookmarkCollection bookmarks_;		// // //

	std::array<CTrackData, CHANID_COUNT> tracks_ = { };		// // //
};
