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
#include <map>
#include <array>
#include <string>
#include "APU/Types.h"
#include "PeriodTables.h"

class CFamiTrackerModule;
class CTempoCounter;
class CPlayerCursor;
class CChannelHandler;
class CChipHandler;
class CTrackerChannel;
class CAPUInterface;
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
	void AssignModule(const CFamiTrackerModule &modfile);
	void LoadAPU(CAPUInterface &apu);
	void ConfigureDocument();

	CTrackerChannel *GetTrackerChannel(stChannelID chan);
	const CTrackerChannel *GetTrackerChannel(stChannelID chan) const;

	void StartPlayer(std::unique_ptr<CPlayerCursor> cur);
	void StopPlayer();
	void ResetTracks();

	void LoadSoundState(const CSongState &state);
	void SetTempoCounter(std::shared_ptr<CTempoCounter> tempo);

	void Tick();

	void QueueNote(stChannelID chan, const stChanNote &note, note_prio_t priority);
	void ForceReloadInstrument(stChannelID chan);

	bool IsPlaying() const;
	bool ShouldHalt() const;

	CPlayerCursor *GetPlayerCursor() const;

	int GetChannelNote(stChannelID chan) const;
	int GetChannelVolume(stChannelID chan) const;
	std::string GetChannelStateString(stChannelID chan) const;

	int ReadPeriodTable(int Index, int Table) const;
	int ReadVibratoTable(int index) const;

	// void (*F)(CChannelHandler &channel, CTrackerChannel &track [, stChannelID id])
	template <typename F>
	void ForeachTrack(F f) const {
		if constexpr (std::is_invocable_v<F, CChannelHandler &, CTrackerChannel &>) {
			for (auto &x : tracks_)
				if (auto &[ch, tr] = x.second; ch && tr)
					f(*ch, *tr);
		}
		else if constexpr (std::is_invocable_v<F, CChannelHandler &, CTrackerChannel &, stChannelID>) {
			for (auto &[id, x] : tracks_)
				if (auto &[ch, tr] = x; ch && tr)
					f(*ch, *tr, id);
		}
		else
			static_assert(sizeof(F) == 0, "Unknown function signature");
	}

private:
	CChannelHandler *GetChannelHandler(stChannelID chan) const;

	void SetupVibrato();
	void SetupPeriodTables();

	void PlayerTick();
	void StepRow(stChannelID chan);
	void UpdateChannels();
	void HandleGlobalEffects(stChanNote &note);

private:
	struct stChannelID_ident_less {
		static constexpr int compare(const stChannelID &lhs, const stChannelID &rhs) noexcept {
			if (lhs.Chip == sound_chip_t::none && rhs.Chip == sound_chip_t::none)
				return 0;
			if (lhs.Ident > rhs.Ident)
				return 1;
			if (lhs.Ident < rhs.Ident)
				return -1;
			if (lhs.Chip > rhs.Chip)
				return 1;
			if (lhs.Chip < rhs.Chip)
				return -1;
			if (lhs.Subindex > rhs.Subindex)
				return 1;
			if (lhs.Subindex < rhs.Subindex)
				return -1;
			return 0;
		}
		constexpr bool operator()(const stChannelID &lhs, const stChannelID &rhs) const noexcept {
			return compare(lhs, rhs) < 0;
		}
	};

	std::map<stChannelID, std::pair<CChannelHandler *, std::unique_ptr<CTrackerChannel>>, stChannelID_ident_less> tracks_;
	std::vector<std::unique_ptr<CChipHandler>> chips_;		// // //
	const CFamiTrackerModule *modfile_ = nullptr;		// // //
	CSoundGenBase *parent_ = nullptr;		// // //
	CAPUInterface *apu_ = nullptr;		// // //

	bool				m_bPlaying = false;
	bool				m_bHaltRequest = false;

	// Play control
	int					m_iJumpToPattern = -1;
	int					m_iSkipToRow = -1;
	bool				m_bDoHalt = false;					// // // Cxx effect

	CPeriodTables		m_iNoteLookupTable;		// // //
	std::array<int, 256> m_iVibratoTable = { };

	std::shared_ptr<CTempoCounter> m_pTempoCounter;			// // // tempo calculation
	// Player state
	std::unique_ptr<CPlayerCursor> m_pPlayerCursor;			// // //
};
