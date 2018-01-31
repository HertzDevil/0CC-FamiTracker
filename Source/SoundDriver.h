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

#include <memory>
#include <vector>
#include <string>
#include "FamiTrackerTypes.h"
#include "APU/Types_fwd.h"

const int VIBRATO_LENGTH = 256;
const int TREMOLO_LENGTH = 256;

class CFamiTrackerModule;
class CChannelMap;
class CTempoCounter;
class CPlayerCursor;
class CChannelHandler;
class CTrackerChannel;
class CAPU;
class CSongState;
class stChanNote;
class CSoundGenBase;
class CSoundChipSet;
enum note_prio_t : unsigned;

class CSoundDriver {
public:
	explicit CSoundDriver(CSoundGenBase *parent = nullptr);
	~CSoundDriver();

	void SetupTracks();
	void LoadDocument(const CFamiTrackerModule &modfile, CAPU &apu);
	void ConfigureDocument();

	std::unique_ptr<CChannelMap> MakeChannelMap(const CSoundChipSet &chips, unsigned n163chs) const;
	CTrackerChannel *GetTrackerChannel(chan_id_t chan);
	const CTrackerChannel *GetTrackerChannel(chan_id_t chan) const;

	void StartPlayer(std::unique_ptr<CPlayerCursor> cur);
	void StopPlayer();
	void ResetTracks();

	void LoadSoundState(const CSongState &state);
	void SetTempoCounter(std::shared_ptr<CTempoCounter> tempo);

	void Tick();
	void UpdateAPU(int cycles);

	void QueueNote(chan_id_t chan, const stChanNote &note, note_prio_t priority);
	void ForceReloadInstrument(chan_id_t chan);

	bool IsPlaying() const;
	bool ShouldHalt() const;

	CPlayerCursor *GetPlayerCursor() const;

	int GetChannelNote(chan_id_t chan) const;
	int GetChannelVolume(chan_id_t chan) const;
	std::string GetChannelStateString(chan_id_t chan) const;

	int ReadPeriodTable(int Index, int Table) const;
	int ReadVibratoTable(int index) const;

private:
	CChannelHandler *GetChannelHandler(chan_id_t chan) const;

	void SetupVibrato();
	void SetupPeriodTables();

	void PlayerTick();
	void StepRow(chan_id_t chan);
	void UpdateChannels();
	void HandleGlobalEffects(stChanNote &note);

	// void (*F)(CChannelHandler &channel, CTrackerChannel &track [, chan_id_t id])
	template <typename F>
	void ForeachTrack(F f) const {
		if constexpr (std::is_invocable_v<F, CChannelHandler &, CTrackerChannel &>) {
			for (auto &[ch, tr] : tracks_)
				if (ch && tr)
					f(*ch, *tr);
		}
		else if constexpr (std::is_invocable_v<F, CChannelHandler &, CTrackerChannel &, chan_id_t>) {
			std::size_t x = 0;
			for (auto &[ch, tr] : tracks_) {
				if (ch && tr)
					f(*ch, *tr, (chan_id_t)x);
				++x;
			}
		}
		else
			static_assert(sizeof(F) == 0, "Unknown function signature");
	}

private:
	std::vector<std::pair<
		std::unique_ptr<CChannelHandler>, std::unique_ptr<CTrackerChannel>
	>> tracks_;
	const CFamiTrackerModule *modfile_ = nullptr;		// // //
	CAPU *apu_ = nullptr;		// // //
	CSoundGenBase *parent_ = nullptr;		// // //

	bool				m_bPlaying = false;
	bool				m_bHaltRequest = false;

	// Play control
	int					m_iJumpToPattern = -1;
	int					m_iSkipToRow = -1;
	bool				m_bDoHalt = false;					// // // Cxx effect

	unsigned int		m_iNoteLookupTableNTSC[NOTE_COUNT];			// For 2A03
	unsigned int		m_iNoteLookupTablePAL[NOTE_COUNT];			// For 2A07
	unsigned int		m_iNoteLookupTableSaw[NOTE_COUNT];			// For VRC6 sawtooth
	unsigned int		m_iNoteLookupTableVRC7[NOTE_COUNT];			// // // For VRC7
	unsigned int		m_iNoteLookupTableFDS[NOTE_COUNT];			// For FDS
	unsigned int		m_iNoteLookupTableN163[NOTE_COUNT];			// For N163
	unsigned int		m_iNoteLookupTableS5B[NOTE_COUNT];			// // // For 5B, internal use only
	int					m_iVibratoTable[VIBRATO_LENGTH];

	std::shared_ptr<CTempoCounter> m_pTempoCounter;			// // // tempo calculation
	// Player state
	std::unique_ptr<CPlayerCursor> m_pPlayerCursor;			// // //
};
