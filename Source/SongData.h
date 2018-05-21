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

#include <map>		// // //
#include <string>		// // //
#include "APU/Types.h"		// // //
#include "TrackData.h"		// // //
#include "Highlight.h"		// // //
#include "BookmarkCollection.h"		// // //

namespace ft0cc::doc {
class pattern_note;
} // namespace ft0cc::doc

// CSongData holds all notes in the patterns
class CSongData
{
public:
	explicit CSongData(unsigned int PatternLength);		// // //
	~CSongData();

	CTrackData *GetTrack(stChannelID chan);		// // //
	const CTrackData *GetTrack(stChannelID chan) const;

	bool IsPatternInUse(stChannelID Channel, unsigned int Pattern) const;

	unsigned GetFreePatternIndex(stChannelID Channel, unsigned Whence = (unsigned)-1) const;		// // //

	ft0cc::doc::pattern_note &GetPatternData(stChannelID Channel, unsigned Pattern, unsigned Row);		// // //
	const ft0cc::doc::pattern_note &GetPatternData(stChannelID Channel, unsigned Pattern, unsigned Row) const;		// // //
	void SetPatternData(stChannelID Channel, unsigned Pattern, unsigned Row, const ft0cc::doc::pattern_note &Note);		// // //

	CPatternData &GetPattern(stChannelID Channel, unsigned Pattern);		// // //
	const CPatternData &GetPattern(stChannelID Channel, unsigned Pattern) const;		// // //
	CPatternData &GetPatternOnFrame(stChannelID Channel, unsigned Frame);		// // //
	const CPatternData &GetPatternOnFrame(stChannelID Channel, unsigned Frame) const;		// // //

	ft0cc::doc::pattern_note GetActiveNote(stChannelID Channel, unsigned Frame, unsigned Row) const;		// // //

	std::string_view GetTitle() const;		// // //
	unsigned int GetPatternLength() const;
	unsigned int GetFrameCount() const;
	unsigned int GetSongSpeed() const;
	unsigned int GetSongTempo() const;
	unsigned GetEffectColumnCount(stChannelID Channel) const;
	bool GetSongGroove() const;		// // //

	void SetTitle(std::string_view str);		// // //
	void SetPatternLength(unsigned int Length);
	void SetFrameCount(unsigned int Count);
	void SetSongSpeed(unsigned int Speed);
	void SetSongTempo(unsigned int Tempo);
	void SetEffectColumnCount(stChannelID Channel, unsigned Count);
	void SetSongGroove(bool Groove);		// // //

	unsigned int GetFramePattern(unsigned int Frame, stChannelID Channel) const;
	void SetFramePattern(unsigned int Frame, stChannelID Channel, unsigned int Pattern);

	const stHighlight &GetRowHighlight() const;
	void SetRowHighlight(const stHighlight &Hl);		// // //

	stHighlight GetHighlightAt(unsigned Frame, unsigned Row) const;		// // //
	highlight_state_t GetHighlightState(unsigned Frame, unsigned Row) const;		// // //

	void PullUp(stChannelID Chan, unsigned Frame, unsigned Row);
	void InsertRow(stChannelID Chan, unsigned Frame, unsigned Row);
	void CopyTrack(stChannelID Chan, const CSongData &From, stChannelID ChanFrom);		// // //
	void SwapChannels(stChannelID First, stChannelID Second);		// // //

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

	// void (*F)(CTrackData &track [, stChannelID ch])
	template <typename F>
	void VisitTracks(F f) {
		if constexpr (std::is_invocable_v<F, CTrackData &, stChannelID>) {
			for (auto &[id, track] : tracks_)
				f(track, id);
		}
		else if constexpr (std::is_invocable_v<F, CTrackData &>) {
			for (auto &x : tracks_)
				f(x.second);
		}
		else
			static_assert(!sizeof(F), "Unknown function signature");
	}

	// void (*F)(const CTrackData &track [, stChannelID ch])
	template <typename F>
	void VisitTracks(F f) const {
		if constexpr (std::is_invocable_v<F, const CTrackData &, stChannelID>) {
			for (auto &[id, track] : tracks_)
				f(track, id);
		}
		else if constexpr (std::is_invocable_v<F, const CTrackData &>) {
			for (auto &x : tracks_)
				f(x.second);
		}
		else
			static_assert(!sizeof(F), "Unknown function signature");
	}

	// void (*F)(CPatternData &pattern [, stChannelID ch, std::size_t p_index])
	template <typename F>
	void VisitPatterns(F f) {		// // //
		if constexpr (std::is_invocable_v<F, CPatternData &, stChannelID, std::size_t>)
			VisitTracks([&] (CTrackData &track, stChannelID ch) {
				track.VisitPatterns([&] (CPatternData &pattern, std::size_t p_index) {
					f(pattern, ch, p_index);
				});
			});
		else if constexpr (std::is_invocable_v<F, CPatternData &>)
			VisitTracks([&] (CTrackData &track) {
				track.VisitPatterns(f);
			});
		else
			static_assert(!sizeof(F), "Unknown function signature");
	}

	// void (*F)(const CPatternData &pattern [, stChannelID ch, std::size_t p_index])
	template <typename F>
	void VisitPatterns(F f) const {
		if constexpr (std::is_invocable_v<F, const CPatternData &, stChannelID, std::size_t>)
			VisitTracks([&] (const CTrackData &track, stChannelID ch) {
				track.VisitPatterns([&] (const CPatternData &pattern, std::size_t p_index) {
					f(pattern, ch, p_index);
				});
			});
		else if constexpr (std::is_invocable_v<F, const CPatternData &>)
			VisitTracks([&] (const CTrackData &track) {
				track.VisitPatterns(f);
			});
		else
			static_assert(!sizeof(F), "Unknown function signature");
	}

public:
	// // // moved from CFamiTrackerDoc
	static const stHighlight DEFAULT_HIGHLIGHT;
	static const unsigned DEFAULT_ROW_COUNT;

private:
	// Track parameters
	std::string	 m_sTrackName;							// // // moved
	unsigned int m_iPatternLength;						// Amount of rows in one pattern
	unsigned int m_iFrameCount = 1;						// Number of frames
	unsigned int m_iSongSpeed = DEFAULT_SPEED;			// Song speed
	unsigned int m_iSongTempo = DEFAULT_TEMPO_NTSC;		// Song tempo
	bool		 m_bUseGroove = false;					// // // Groove

	// Row highlight settings
	stHighlight  m_vRowHighlight = DEFAULT_HIGHLIGHT;		// // //

	// Bookmarks
	CBookmarkCollection bookmarks_;		// // //

	std::map<stChannelID, CTrackData> tracks_;		// // //
};
