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
#include "PatternData.h"		// // //
#include "Highlight.h"		// // //
#include "BookmarkCollection.h"		// // //

class stChanNote;		// // //
class CFTMComponentInterface;		// // //

// CSongData holds all notes in the patterns
class CSongData
{
public:
	explicit CSongData(CFTMComponentInterface &parent, unsigned int PatternLength = DEFAULT_ROW_COUNT);		// // //
	~CSongData();

	bool IsPatternInUse(chan_id_t Channel, unsigned int Pattern) const;

	unsigned GetFreePatternIndex(chan_id_t Channel, unsigned Whence = (unsigned)-1) const;		// // //

	stChanNote &GetPatternData(chan_id_t Channel, unsigned Pattern, unsigned Row);		// // //
	const stChanNote &GetPatternData(chan_id_t Channel, unsigned Pattern, unsigned Row) const;		// // //
	void SetPatternData(chan_id_t Channel, unsigned Pattern, unsigned Row, const stChanNote &Note);		// // //

	CPatternData &GetPattern(chan_id_t Channel, unsigned Pattern);		// // //
	const CPatternData &GetPattern(chan_id_t Channel, unsigned Pattern) const;		// // //
	CPatternData &GetPatternOnFrame(chan_id_t Channel, unsigned Frame);		// // //
	const CPatternData &GetPatternOnFrame(chan_id_t Channel, unsigned Frame) const;		// // //

	const std::string &GetTitle() const;		// // //
	unsigned int GetPatternLength() const;
	unsigned int GetFrameCount() const;
	unsigned int GetSongSpeed() const;
	unsigned int GetSongTempo() const;
	int GetEffectColumnCount(chan_id_t Channel) const;
	bool GetSongGroove() const;		// // //

	void SetTitle(std::string_view str);		// // //
	void SetPatternLength(unsigned int Length);
	void SetFrameCount(unsigned int Count);
	void SetSongSpeed(unsigned int Speed);
	void SetSongTempo(unsigned int Tempo);
	void SetEffectColumnCount(chan_id_t Channel, int Count);
	void SetSongGroove(bool Groove);		// // //

	unsigned int GetFramePattern(unsigned int Frame, chan_id_t Channel) const;
	void SetFramePattern(unsigned int Frame, chan_id_t Channel, unsigned int Pattern);

	void SetHighlight(const stHighlight &Hl);		// // //
	const stHighlight &GetRowHighlight() const;

	void CopyTrack(chan_id_t Chan, const CSongData &From, chan_id_t ChanFrom);		// // //
	void SwapChannels(chan_id_t First, chan_id_t Second);		// // //

	CBookmarkCollection &GetBookmarks();		// // //
	const CBookmarkCollection &GetBookmarks() const;
	void SetBookmarks(const CBookmarkCollection &bookmarks);
	void SetBookmarks(CBookmarkCollection &&bookmarks);

	// void (*F)(CPatternData &pat [, unsigned ch, unsigned pat_index])
	template <typename F>
	void VisitPatterns(F f) {		// // //
		if constexpr (std::is_invocable_v<F, CPatternData &>) {
			unsigned ch_pos = 0;
			for (auto &ch : m_pPatternData) {
				if (GetChannelPosition((chan_id_t)ch_pos) != (unsigned)-1)
					for (auto &p : ch)
						f(p);
				++ch_pos;
			}
		}
		else {
			unsigned ch_pos = 0;
			for (auto &ch : m_pPatternData) {
				if (GetChannelPosition((chan_id_t)ch_pos) != (unsigned)-1) {
					unsigned p_index = 0;
					for (auto &p : ch)
						f(p, (chan_id_t)ch_pos, p_index++);
				}
				++ch_pos;
			}
		}
	}

	// void (*F)(const CPatternData &pat [, unsigned ch, unsigned pat_index])
	template <typename F>
	void VisitPatterns(F f) const {
		if constexpr (std::is_invocable_v<F, const CPatternData &>) {
			unsigned ch_pos = 0;
			for (auto &ch : m_pPatternData) {
				if (GetChannelPosition((chan_id_t)ch_pos) != (unsigned)-1)
					for (auto &p : ch)
						f(p);
				++ch_pos;
			}
		}
		else {
			unsigned ch_pos = 0;
			for (auto &ch : m_pPatternData) {
				if (GetChannelPosition((chan_id_t)ch_pos) != (unsigned)-1) {
					unsigned p_index = 0;
					for (auto &p : ch)
						f(p, (chan_id_t)ch_pos, p_index++);
				}
				++ch_pos;
			}
		}
	}

	unsigned GetChannelPosition(chan_id_t ChanID) const;		// // // TODO: move to CSongView

public:
	// // // moved from CFamiTrackerDoc
	static const std::string DEFAULT_TITLE;
	static const stHighlight DEFAULT_HIGHLIGHT;

private:
	static const unsigned DEFAULT_ROW_COUNT;

	CFTMComponentInterface &parent_;		// // //

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

	// Number of visible effect columns for each channel
	std::array<unsigned char, CHANNELS> m_iEffectColumns = { };		// // //

	// List of the patterns assigned to frames
	std::array<std::array<unsigned char, CHANNELS>, MAX_FRAMES> m_iFrameList = { };		// // //

	// All accesses to m_pPatternData must go through GetPatternData()
	std::array<std::array<CPatternData, MAX_PATTERN>, CHANNELS> m_pPatternData = { };		// // //
};
