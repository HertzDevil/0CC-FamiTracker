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
#include <array>
#include "FamiTrackerTypes.h"
#include "FTMComponentInterface.h"

class CSongData;
class CChannelMap;
class CSoundChipSet;
class CChannelOrder;
class CConstSongView;
class CSongView;
class CInstrumentManager;
struct stHighlight;

namespace ft0cc::doc {
class groove;
} // namespace ft0cc::doc

class CFamiTrackerModule : public CFTMComponentInterface {
	using groove = ft0cc::doc::groove;

public:
	static constexpr std::size_t METADATA_FIELD_LENGTH		= 32;

	static constexpr vibrato_t	 DEFAULT_VIBRATO_STYLE		= vibrato_t::VIBRATO_NEW;
	static constexpr bool		 DEFAULT_LINEAR_PITCH		= false;
	static constexpr unsigned	 DEFAULT_SPEED_SPLIT_POINT	= 32;
	static constexpr unsigned	 OLD_SPEED_SPLIT_POINT		= 21;

	explicit CFamiTrackerModule(CFTMComponentInterface &parent);
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

	// sound chip
	CChannelOrder &GetChannelOrder() const override;
	const CSoundChipSet &GetSoundChipSet() const;
	void SetChannelMap(std::unique_ptr<CChannelMap> pMap);

	bool HasExpansionChips() const;
	bool HasExpansionChip(sound_chip_t Chip) const;
	int GetNamcoChannels() const;

	std::unique_ptr<CSongView> MakeSongView(unsigned index);
	std::unique_ptr<CConstSongView> MakeSongView(unsigned index) const;

	// global info
	machine_t GetMachine() const;
	unsigned int GetEngineSpeed() const;
	unsigned int GetFrameRate() const;
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
	std::unique_ptr<CSongData> MakeNewSong() const;
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
			static_assert(sizeof(F) == 0, "Unknown function signature");
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
			static_assert(sizeof(F) == 0, "Unknown function signature");
	}

	// instruments
	CInstrumentManager *const GetInstrumentManager() const override;
	CSequenceManager *const GetSequenceManager(int InstType) const override;
	CDSampleManager *const GetDSampleManager() const override;

	void SwapInstruments(unsigned first, unsigned second);

	// grooves
	std::shared_ptr<groove> GetGroove(unsigned index);		// // //
	std::shared_ptr<const groove> GetGroove(unsigned index) const;		// // //
	bool HasGroove(unsigned index) const;		// // //
	void SetGroove(unsigned index, std::shared_ptr<groove> pGroove);

	// highlight
	const stHighlight &GetHighlight(unsigned song) const;
	void SetHighlight(const stHighlight &hl);		// // //
	void SetHighlight(unsigned song, const stHighlight &hl);		// // //

	// cleanup
	void RemoveUnusedPatterns();
	void RemoveUnusedInstruments();
	void RemoveUnusedDSamples();		// // //

private:
	bool AllocateSong(unsigned index);

	// TODO: remove these from base class
	void Modify(bool Change) override;
	void ModifyIrreversible() override;

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

	// Channels (no longer contain run-time state)
	std::unique_ptr<CChannelMap> m_pChannelMap;		// // //

	std::vector<std::unique_ptr<CSongData>> m_pTracks;

	std::unique_ptr<CInstrumentManager> m_pInstrumentManager;

	std::array<std::shared_ptr<groove>, 32/*MAX_GROOVE*/> m_pGrooveTable;		// // // Grooves
};
