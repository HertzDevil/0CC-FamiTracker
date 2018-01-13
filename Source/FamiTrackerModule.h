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

#include <string>
#include <memory>
#include <vector>
#include "FamiTrackerTypes.h"

class CSongData;

class CFamiTrackerModule {
public:
	static constexpr std::size_t METADATA_FIELD_LENGTH		= 32;

	static constexpr machine_t	 DEFAULT_MACHINE_TYPE		= machine_t::NTSC;
	static constexpr vibrato_t	 DEFAULT_VIBRATO_STYLE		= vibrato_t::VIBRATO_NEW;
	static constexpr bool		 DEFAULT_LINEAR_PITCH		= false;
	static constexpr unsigned	 DEFAULT_SPEED_SPLIT_POINT	= 32;
	static constexpr unsigned	 OLD_SPEED_SPLIT_POINT		= 21;

	CFamiTrackerModule();
	~CFamiTrackerModule();

	// module metadata
	std::string_view GetModuleName() const;
	std::string_view GetModuleArtist() const;
	std::string_view GetModuleCopyright() const;
	std::string_view GetComment() const;
	bool ShowsCommentOnOpen() const;
	void SetModuleName(std::string_view sv);
	void SetModuleArtist(std::string_view sv);
	void SetModuleCopyright(std::string_view sv);
	void SetComment(std::string_view comment, bool showOnOpen);

	// global info
	machine_t GetMachine() const;
	unsigned int GetEngineSpeed() const;
	vibrato_t GetVibratoStyle() const;
	bool GetLinearPitch() const;
	int GetSpeedSplitPoint() const;

	void SetMachine(machine_t machine);
	void SetEngineSpeed(unsigned int speed);
	void SetVibratoStyle(vibrato_t style);
	void SetLinearPitch(bool enable);
	void SetSpeedSplitPoint(int splitPoint);

	// detune
	int GetDetuneOffset(int chip, int note) const;
	void SetDetuneOffset(int chip, int note, int offset);
	void ResetDetuneTables();
	int GetTuningSemitone() const;		// // // 050B
	int GetTuningCent() const;		// // // 050B
	void SetTuning(int semitone, int cent);		// // // 050B

	// songs
	CSongData *GetSong(unsigned index);
	const CSongData *GetSong(unsigned index) const;
	std::size_t GetSongCount() const;
	bool AllocateSong(unsigned index);
	int AddSong();
	int AddSong(std::unique_ptr<CSongData> pSong);
	bool InsertSong(unsigned index, std::unique_ptr<CSongData> pSong);
	std::unique_ptr<CSongData> ReplaceSong(unsigned index, std::unique_ptr<CSongData> pSong);
	std::unique_ptr<CSongData> ReleaseSong(unsigned index);
	void RemoveSong(unsigned index);
	void SwapSongs(unsigned lhs, unsigned rhs);

	// void (*F)(CSongData &song [, unsigned index])
	template <typename F>
	void VisitSongs(F f) {
		if constexpr (std::is_invocable_v<F, CSongData &, unsigned>) {
			unsigned index = 0;
			for (auto &song : m_pTracks)
				f(*song, index++);
		}
		else if constexpr (std::is_invocable_v<F, CSongData &>) {
			for (auto &song : m_pTracks)
				f(*song);
		}
		else
			static_assert(false, "Unknown function signature");
	}
	// void (*F)(const CSongData &song [, unsigned index])
	template <typename F>
	void VisitSongs(F f) const {
		if constexpr (std::is_invocable_v<F, const CSongData &, unsigned>) {
			unsigned index = 0;
			for (auto &song : m_pTracks)
				f(*song, index++);
		}
		else if constexpr (std::is_invocable_v<F, const CSongData &>) {
			for (auto &song : m_pTracks)
				f(*song);
		}
		else
			static_assert(false, "Unknown function signature");
	}

private:
	std::unique_ptr<CSongData> MakeNewSong() const;

private:
	machine_t		m_iMachine = DEFAULT_MACHINE_TYPE;
	unsigned int	m_iEngineSpeed = 0;
	vibrato_t		m_iVibratoStyle = DEFAULT_VIBRATO_STYLE;
	bool			m_bLinearPitch = DEFAULT_LINEAR_PITCH;
	unsigned int	m_iSpeedSplitPoint = DEFAULT_SPEED_SPLIT_POINT;
	int				m_iDetuneTable[6][96] = { };		// // // Detune tables
	int				m_iDetuneSemitone = 0;		// // // 050B tuning
	int				m_iDetuneCent = 0;		// // // 050B tuning

	std::string m_strName;
	std::string m_strArtist;
	std::string m_strCopyright;

	std::string m_strComment;
	bool m_bDisplayComment = false;

	std::vector<std::unique_ptr<CSongData>> m_pTracks;
};
