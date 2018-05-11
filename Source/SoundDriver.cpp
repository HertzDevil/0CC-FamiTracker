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

#include "SoundDriver.h"
#include "SoundGenBase.h"
#include "FamiTrackerModule.h"
#include "SongData.h"
#include "TempoCounter.h"
#include "ChannelHandler.h"
#include "ChipHandler.h"
#include "FamiTrackerEnv.h"
#include "SoundChipService.h"
#include "TrackerChannel.h"
#include "PlayerCursor.h"
#include "SongState.h"
#include "ChannelMap.h"
#include "Assertion.h"



CSoundDriver::CSoundDriver(CSoundGenBase *parent) : parent_(parent) {
}

CSoundDriver::~CSoundDriver() {
}

void CSoundDriver::SetupTracks() {
	// Only called once!

	// Clear all channels
	tracks_.clear();		// // //

	constexpr std::uint8_t INSTANCE_ID = 0u;

	auto *pSCS = Env.GetSoundChipService();
	pSCS->ForeachTrack([&] (stChannelID id) {
		tracks_.try_emplace(id, nullptr, std::make_unique<CTrackerChannel>());
	});
	pSCS->ForeachType([&] (sound_chip_t c) {
		chips_.push_back(Env.GetSoundChipService()->MakeChipHandler(c, INSTANCE_ID));
	});

	for (auto &x : chips_) {
		x->VisitChannelHandlers([&] (CChannelHandler &ch) {
			tracks_[ch.GetChannelID()].first = &ch;
		});
	}
}

void CSoundDriver::AssignModule(const CFamiTrackerModule &modfile) {
	modfile_ = &modfile;
}

void CSoundDriver::LoadAPU(CAPUInterface &apu) {
	apu_ = &apu;

	// Setup all channels
	ForeachTrack([&] (CChannelHandler &ch, CTrackerChannel &) {
		ch.InitChannel(apu, m_iVibratoTable, parent_);
	});
}

void CSoundDriver::ConfigureDocument() {
	SetupVibrato();
	SetupPeriodTables();

	ForeachTrack([&] (CChannelHandler &ch, CTrackerChannel &) {
		ch.ConfigureDocument(*modfile_);
	});
}

CChannelHandler *CSoundDriver::GetChannelHandler(stChannelID chan) const {
	if (chan.Chip != sound_chip_t::none)
		if (auto it = tracks_.find(chan); it != tracks_.end())
			return it->second.first;
	return nullptr;
}

CTrackerChannel *CSoundDriver::GetTrackerChannel(stChannelID chan) {
	if (chan.Chip != sound_chip_t::none)
		if (auto it = tracks_.find(chan); it != tracks_.end())
			return it->second.second.get();
	return nullptr;
}

const CTrackerChannel *CSoundDriver::GetTrackerChannel(stChannelID chan) const {
	return const_cast<CSoundDriver *>(this)->GetTrackerChannel(chan);
}

void CSoundDriver::StartPlayer(std::unique_ptr<CPlayerCursor> cur) {
	m_pPlayerCursor = std::move(cur);		// // //
	m_bPlaying = true;
	m_bHaltRequest = false;

	m_iJumpToPattern = -1;
	m_iSkipToRow = -1;
	m_bDoHalt = false;		// // //
}

void CSoundDriver::StopPlayer() {
	m_bPlaying = false;
	m_bDoHalt = false;
	m_bHaltRequest = false;
}

void CSoundDriver::ResetTracks() {
	for (auto &chip : chips_)		// // //
		chip->ResetChip(*apu_);

	ForeachTrack([&] (CChannelHandler &ch, CTrackerChannel &tr) {		// // //
		ch.ResetChannel();
		tr.Reset();
	});
}

void CSoundDriver::LoadSoundState(const CSongState &state) {
	if (m_pTempoCounter)
		m_pTempoCounter->LoadSoundState(state);
	ForeachTrack([&] (CChannelHandler &ch, CTrackerChannel &tr, stChannelID id) {		// // //
		if (modfile_->GetChannelOrder().HasChannel(id))
			if (auto it = state.State.find(id); it != state.State.end())
				ch.ApplyChannelState(it->second);
	});
}

void CSoundDriver::SetTempoCounter(std::shared_ptr<CTempoCounter> tempo) {
	if (m_pTempoCounter = std::move(tempo); m_pTempoCounter)
		m_pTempoCounter->AssignModule(*modfile_);
}

void CSoundDriver::Tick() {
	if (IsPlaying())
		PlayerTick();
	UpdateChannels();
}

void CSoundDriver::StepRow(stChannelID chan) {
	stChanNote NoteData = m_pPlayerCursor->GetSong().GetActiveNote(
		chan, m_pPlayerCursor->GetCurrentFrame(), m_pPlayerCursor->GetCurrentRow());		// // //
	for (auto &cmd : NoteData.Effects)
		if (HandleGlobalEffect(cmd))
			cmd = { };
	if (!parent_ || !parent_->IsChannelMuted(chan))
		QueueNote(chan, NoteData, NOTE_PRIO_1);
	// Let view know what is about to play
	if (parent_)
		parent_->OnPlayNote(chan, NoteData);
}

void CSoundDriver::PlayerTick() {
	if (!m_pTempoCounter)
		return;
	m_pPlayerCursor->Tick();
	if (parent_)
		parent_->OnTick();

	int SteppedRows = 0;		// // //

	// Fetch next row
//	while (m_pTempoCounter->CanStepRow()) {
	if (m_pTempoCounter->CanStepRow()) {
		if (m_bDoHalt)
			m_bHaltRequest = true;
		else
			++SteppedRows;
		m_pTempoCounter->StepRow();		// // //

		modfile_->GetChannelOrder().ForeachChannel([&] (stChannelID i) {
			StepRow(i);
		});

		if (parent_)
			parent_->OnStepRow();
	}
	m_pTempoCounter->Tick();		// // //

	// Update player
	if (parent_ && parent_->ShouldStopPlayer())		// // //
		m_bHaltRequest = true;
	else if (SteppedRows > 0 && !m_bDoHalt) {
		// Jump
		if (m_iJumpToPattern != -1)
			m_pPlayerCursor->DoBxx(m_iJumpToPattern);
		// Skip
		else if (m_iSkipToRow != -1)
			m_pPlayerCursor->DoDxx(m_iSkipToRow);
		// or just move on
		else
			while (SteppedRows--)
				m_pPlayerCursor->StepRow();

		m_iJumpToPattern = -1;
		m_iSkipToRow = -1;

		if (parent_)
			parent_->OnUpdateRow(m_pPlayerCursor->GetCurrentFrame(), m_pPlayerCursor->GetCurrentRow());
	}
}

void CSoundDriver::UpdateChannels() {
	for (auto &chip : chips_)
		chip->RefreshBefore(*apu_);

	ForeachTrack([&] (CChannelHandler &Chan, CTrackerChannel &TrackerChan, stChannelID ID) {		// // //
		if (!modfile_->GetChannelOrder().HasChannel(ID))
			return;

		// Run auto-arpeggio, if enabled
		if (int Arpeggio = parent_ ? parent_->GetArpNote(ID) : -1; Arpeggio > 0)		// // //
			Chan.Arpeggiate(Arpeggio);

		// Check if new note data has been queued for playing
		if (TrackerChan.NewNoteData())
			Chan.PlayNote(TrackerChan.GetNote());		// // //

		// Pitch wheel
		Chan.SetPitch(TrackerChan.GetPitch());

		// Channel updates (instruments, effects etc)
		m_bHaltRequest ? Chan.ResetChannel() : Chan.ProcessChannel();
		Chan.RefreshChannel();
		Chan.FinishTick();		// // //
	});

	for (auto &chip : chips_)
		chip->RefreshAfter(*apu_);
}

void CSoundDriver::QueueNote(stChannelID chan, const stChanNote &note, note_prio_t priority) {
	if (auto *track = GetTrackerChannel(chan))
		track->SetNote(note, priority);
}

void CSoundDriver::ForceReloadInstrument(stChannelID chan) {
	if (modfile_)
		if (auto *handler = GetChannelHandler(chan))
			handler->ForceReloadInstrument();
}

bool CSoundDriver::IsPlaying() const {
	return m_bPlaying;
}

bool CSoundDriver::ShouldHalt() const {
	return m_bHaltRequest;
}

CPlayerCursor *CSoundDriver::GetPlayerCursor() const {
	return m_pPlayerCursor.get();
}

int CSoundDriver::GetChannelNote(stChannelID chan) const {
	const auto *ch = GetChannelHandler(chan);
	return ch ? ch->GetActiveNote() : -1;
}

int CSoundDriver::GetChannelVolume(stChannelID chan) const {
	const auto *ch = GetChannelHandler(chan);
	return ch ? ch->GetChannelVolume() : 0;
}

std::string CSoundDriver::GetChannelStateString(stChannelID chan) const {
	const auto *ch = GetChannelHandler(chan);
	return ch ? ch->GetStateString() : "";
}

int CSoundDriver::ReadPeriodTable(int Index, int Table) const {
	return m_iNoteLookupTable.ReadTable(Index, Table);		// // //
}

int CSoundDriver::ReadVibratoTable(int index) const {
	return m_iVibratoTable[index];
}

void CSoundDriver::SetupVibrato() {
	m_iVibratoTable = modfile_->MakeVibratoTable();		// // //
}

void CSoundDriver::SetupPeriodTables() {
	m_iNoteLookupTable = modfile_->MakePeriodTables();		// // //

	// // // Setup note tables
	const auto GetNoteTable = [&] (stChannelID ch) -> array_view<unsigned> {
		switch (ch.Chip) {
		case sound_chip_t::APU:
			if (!IsAPUNoise(ch) && !IsDPCM(ch))
				return modfile_->GetMachine() == machine_t::PAL ? m_iNoteLookupTable.pal_period : m_iNoteLookupTable.ntsc_period;
			break;
		case sound_chip_t::VRC6:
			return IsVRC6Sawtooth(ch) ? m_iNoteLookupTable.saw_period : m_iNoteLookupTable.ntsc_period;
		case sound_chip_t::VRC7:
			return m_iNoteLookupTable.vrc7_freq;
		case sound_chip_t::FDS:
			return m_iNoteLookupTable.fds_freq;
		case sound_chip_t::MMC5:
			return m_iNoteLookupTable.ntsc_period;
		case sound_chip_t::N163:
			return m_iNoteLookupTable.n163_freq;
		case sound_chip_t::S5B:
			return m_iNoteLookupTable.s5b_period;
		}
		return { };
	};

	ForeachTrack([&] (CChannelHandler &ch, CTrackerChannel &, stChannelID id) {
		if (auto Table = GetNoteTable(id); !Table.empty())
			ch.SetNoteTable(Table);
	});
}

bool CSoundDriver::HandleGlobalEffect(stEffectCommand cmd) {
	switch (cmd.fx) {
	// Fxx: Sets speed to xx
	case effect_t::SPEED:
		if (m_pTempoCounter)
			m_pTempoCounter->DoFxx(cmd.param ? cmd.param : 1);		// // //
		return true;
	// Oxx: Sets groove to xx
	case effect_t::GROOVE:		// // //
		if (m_pTempoCounter)
			m_pTempoCounter->DoOxx(cmd.param % MAX_GROOVE);		// // //
		return true;
	// Bxx: Jump to pattern xx
	case effect_t::JUMP:
		m_iJumpToPattern = cmd.param;
		return true;
	// Dxx: Skip to next track and start at row xx
	case effect_t::SKIP:
		m_iSkipToRow = cmd.param;
		return true;
	// Cxx: Halt playback
	case effect_t::HALT:
		m_bDoHalt = true;		// // //
		m_pPlayerCursor->DoCxx();		// // //
		return true;
	}

	return false;
}
