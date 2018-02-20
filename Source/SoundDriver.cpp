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
#include "ChannelFactory.h"
#include "TrackerChannel.h"
#include "PlayerCursor.h"
#include "DetuneTable.h"
#include "SongState.h"
#include "APU/APU.h"
#include "ChannelsN163.h"
#include "ChannelMap.h"
#include <cmath>
#include "Assertion.h"
#include "ChannelsSN7.h"



namespace {

const double NEW_VIBRATO_DEPTH[] = {
	1.0, 1.5, 2.5, 4.0, 5.0, 7.0, 10.0, 12.0, 14.0, 17.0, 22.0, 30.0, 44.0, 64.0, 96.0, 128.0
};

const double OLD_VIBRATO_DEPTH[] = {
	1.0, 1.0, 2.0, 3.0, 4.0, 7.0, 8.0, 15.0, 16.0, 31.0, 32.0, 63.0, 64.0, 127.0, 128.0, 255.0
};

} // namespace



CSoundDriver::CSoundDriver(CSoundGenBase *parent) : parent_(parent) {
}

CSoundDriver::~CSoundDriver() {
}

void CSoundDriver::SetupTracks() {
	// Only called once!

	// Clear all channels
	tracks_.clear();		// // //

	const auto AssignTrack = [&] (chan_id_t id) {		// // //
		tracks_.emplace_back(CChannelFactory::Make(id), std::make_unique<CTrackerChannel>());
	};

	// 2A03/2A07
	// // // Short header names
	AssignTrack(chan_id_t::SQUARE1);
	AssignTrack(chan_id_t::SQUARE2);
	AssignTrack(chan_id_t::TRIANGLE);
	AssignTrack(chan_id_t::NOISE);
	AssignTrack(chan_id_t::DPCM);

	// Konami VRC6
	AssignTrack(chan_id_t::VRC6_PULSE1);
	AssignTrack(chan_id_t::VRC6_PULSE2);
	AssignTrack(chan_id_t::VRC6_SAWTOOTH);

	// // // Nintendo MMC5
	AssignTrack(chan_id_t::MMC5_SQUARE1);
	AssignTrack(chan_id_t::MMC5_SQUARE2);
	AssignTrack(chan_id_t::MMC5_VOICE); // null channel handler

	// Namco N163
	AssignTrack(chan_id_t::N163_CH1);
	AssignTrack(chan_id_t::N163_CH2);
	AssignTrack(chan_id_t::N163_CH3);
	AssignTrack(chan_id_t::N163_CH4);
	AssignTrack(chan_id_t::N163_CH5);
	AssignTrack(chan_id_t::N163_CH6);
	AssignTrack(chan_id_t::N163_CH7);
	AssignTrack(chan_id_t::N163_CH8);

	// Nintendo FDS
	AssignTrack(chan_id_t::FDS);

	// Konami VRC7
	AssignTrack(chan_id_t::VRC7_CH1);
	AssignTrack(chan_id_t::VRC7_CH2);
	AssignTrack(chan_id_t::VRC7_CH3);
	AssignTrack(chan_id_t::VRC7_CH4);
	AssignTrack(chan_id_t::VRC7_CH5);
	AssignTrack(chan_id_t::VRC7_CH6);

	// // // Sunsoft 5B
	AssignTrack(chan_id_t::S5B_CH1);
	AssignTrack(chan_id_t::S5B_CH2);
	AssignTrack(chan_id_t::S5B_CH3);

	// // // SN76489
	AssignTrack(chan_id_t::SN76489_CH1);
	AssignTrack(chan_id_t::SN76489_CH2);
	AssignTrack(chan_id_t::SN76489_CH3);
	AssignTrack(chan_id_t::SN76489_NOISE);
}

void CSoundDriver::AssignModule(const CFamiTrackerModule &modfile) {
	modfile_ = &modfile;
}

void CSoundDriver::LoadAPU(CAPU &apu) {
	apu_ = &apu;

	// Setup all channels
	ForeachTrack([&] (CChannelHandler &ch, CTrackerChannel &) {
		ch.InitChannel(&apu, m_iVibratoTable, parent_);
	});
}

void CSoundDriver::ConfigureDocument() {
	SetupVibrato();
	SetupPeriodTables();

	ForeachTrack([&] (CChannelHandler &ch, CTrackerChannel &) {
		ch.SetVibratoStyle(modfile_->GetVibratoStyle());
		ch.SetLinearPitch(modfile_->GetLinearPitch());
		if (auto pChan = dynamic_cast<CChannelHandlerN163 *>(&ch))
			pChan->SetChannelCount(modfile_->GetNamcoChannels());
	});
}

std::unique_ptr<CChannelMap> CSoundDriver::MakeChannelMap(const CSoundChipSet &chips, unsigned n163chs) const {
	// This affects the sound channel interface so it must be synchronized
	auto map = std::make_unique<CChannelMap>(chips, n163chs);		// // //

	// Register the channels in the document
	// Expansion & internal channels
	ForeachTrack([&] (CChannelHandler &, CTrackerChannel &, chan_id_t ch) {
		if (map->SupportsChannel(ch))
			map->GetChannelOrder().AddChannel(ch);
	});

	return map;
}

CTrackerChannel *CSoundDriver::GetTrackerChannel(chan_id_t chan) {
	return chan != chan_id_t::NONE ? tracks_[value_cast(chan)].second.get() : nullptr;
}

const CTrackerChannel *CSoundDriver::GetTrackerChannel(chan_id_t chan) const {
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
	ForeachTrack([&] (CChannelHandler &ch, CTrackerChannel &tr) {		// // //
		ch.ResetChannel();
		tr.Reset();
	});
}

void CSoundDriver::LoadSoundState(const CSongState &state) {
	m_pTempoCounter->LoadSoundState(state);
	ForeachTrack([&] (CChannelHandler &ch, CTrackerChannel &tr, chan_id_t id) {		// // //
		if (modfile_->GetChannelOrder().HasChannel(id))
			ch.ApplyChannelState(state.State[value_cast(id)]);
	});
}

void CSoundDriver::SetTempoCounter(std::shared_ptr<CTempoCounter> tempo) {
	m_pTempoCounter = std::move(tempo);
	m_pTempoCounter->AssignModule(*modfile_);
}

void CSoundDriver::Tick() {
	if (IsPlaying())
		PlayerTick();
	UpdateChannels();
}

void CSoundDriver::StepRow(chan_id_t chan) {
	stChanNote NoteData = m_pPlayerCursor->GetSong().GetActiveNote(
		chan, m_pPlayerCursor->GetCurrentFrame(), m_pPlayerCursor->GetCurrentRow());		// // //
	HandleGlobalEffects(NoteData);
	if (!parent_ || !parent_->IsChannelMuted(chan))
		QueueNote(chan, NoteData, NOTE_PRIO_1);
	// Let view know what is about to play
	if (parent_)
		parent_->OnPlayNote(chan, NoteData);
}

void CSoundDriver::PlayerTick() {
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

		modfile_->GetChannelOrder().ForeachChannel([&] (chan_id_t i) {
			StepRow(i);
		});

		if (parent_)
			parent_->OnStepRow();
	}
	m_pTempoCounter->Tick();		// // //
	if (parent_)
		parent_->OnTick();

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
	ForeachTrack([&] (CChannelHandler &Chan, CTrackerChannel &TrackerChan, chan_id_t ID) {		// // //
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

		// Update volume meters
		TrackerChan.SetVolumeMeter(apu_->GetVol(ID));		// // //
	});
}

void CSoundDriver::UpdateAPU(int cycles) {
	sound_chip_t LastChip = sound_chip_t::NONE;		// // // 050B

	ForeachTrack([&] (CChannelHandler &Chan, CTrackerChannel &, chan_id_t ID) {		// // //
		if (modfile_->GetChannelOrder().HasChannel(ID)) {
			sound_chip_t Chip = GetChipFromChannel(ID);
			int Delay = (Chip == LastChip) ? 150 : 250;
			if (Delay < cycles) {
				// Add APU cycles
				cycles -= Delay;
				apu_->AddTime(Delay);
			}
			LastChip = Chip;
		}
		apu_->Process();
	});

	// Finish the audio frame
	apu_->AddTime(cycles);
	apu_->Process();
	apu_->EndFrame();		// // //
}

void CSoundDriver::QueueNote(chan_id_t chan, const stChanNote &note, note_prio_t priority) {
	if (auto &x = tracks_[value_cast(chan)].second)
		x->SetNote(note, priority);
}

void CSoundDriver::ForceReloadInstrument(chan_id_t chan) {
	if (modfile_)
		tracks_[value_cast(chan)].first->ForceReloadInstrument();
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

CChannelHandler *CSoundDriver::GetChannelHandler(chan_id_t chan) const {
	return tracks_[value_cast(chan)].first.get();
}

int CSoundDriver::GetChannelNote(chan_id_t chan) const {
	const auto &ch = GetChannelHandler(chan);
	return ch ? ch->GetActiveNote() : -1;
}

int CSoundDriver::GetChannelVolume(chan_id_t chan) const {
	const auto &ch = GetChannelHandler(chan);
	return ch ? ch->GetChannelVolume() : 0;
}

std::string CSoundDriver::GetChannelStateString(chan_id_t chan) const {
	const auto &ch = GetChannelHandler(chan);
	return ch ? ch->GetStateString() : "";
}

int CSoundDriver::ReadPeriodTable(int Index, int Table) const {
	switch (Table) {
	case CDetuneTable::DETUNE_NTSC: return m_iNoteLookupTableNTSC[Index]; break;
	case CDetuneTable::DETUNE_PAL:  return m_iNoteLookupTablePAL[Index]; break;
	case CDetuneTable::DETUNE_SAW:  return m_iNoteLookupTableSaw[Index]; break;
	case CDetuneTable::DETUNE_VRC7: return m_iNoteLookupTableVRC7[Index]; break;
	case CDetuneTable::DETUNE_FDS:  return m_iNoteLookupTableFDS[Index]; break;
	case CDetuneTable::DETUNE_N163: return m_iNoteLookupTableN163[Index]; break;
	case CDetuneTable::DETUNE_S5B:  return m_iNoteLookupTableNTSC[Index] + 1; break;
	case CDetuneTable::DETUNE_SN76489: return m_iNoteLookupTableSN76489[Index]; break;
	}
	DEBUG_BREAK();
	return m_iNoteLookupTableNTSC[Index];
}

int CSoundDriver::ReadVibratoTable(int index) const {
	return m_iVibratoTable[index];
}

void CSoundDriver::SetupVibrato() {
	const vibrato_t style = modfile_->GetVibratoStyle();

	for (int i = 0; i < 16; ++i) {	// depth
		for (int j = 0; j < 16; ++j) {	// phase
			int value = 0;
			double angle = (double(j) / 16.0) * (3.1415 / 2.0);

			if (style == VIBRATO_NEW)
				value = int(std::sin(angle) * NEW_VIBRATO_DEPTH[i] /*+ 0.5f*/);
			else {
				value = (int)((double(j * OLD_VIBRATO_DEPTH[i]) / 16.0) + 1);
			}

			m_iVibratoTable[i * 16 + j] = value;
		}
	}
}

void CSoundDriver::SetupPeriodTables() {
	machine_t Machine = modfile_->GetMachine();
	const double A440_NOTE = 45. - modfile_->GetTuningSemitone() - modfile_->GetTuningCent() / 100.;
	const double clock_ntsc = CAPU::BASE_FREQ_NTSC / 16.0;
	const double clock_pal = CAPU::BASE_FREQ_PAL / 16.0;

	for (int i = 0; i < NOTE_COUNT; ++i) {
		// Frequency (in Hz)
		double Freq = 440. * std::pow(2.0, (i - A440_NOTE) / 12.);
		double Pitch;

		// 2A07
		Pitch = (clock_pal / Freq) - 0.5;
		m_iNoteLookupTablePAL[i] = (unsigned int)(Pitch - modfile_->GetDetuneOffset(1, i));		// // //

		// 2A03 / MMC5 / VRC6
		Pitch = (clock_ntsc / Freq) - 0.5;
		m_iNoteLookupTableNTSC[i] = (unsigned int)(Pitch - modfile_->GetDetuneOffset(0, i));		// // //
		m_iNoteLookupTableS5B[i] = m_iNoteLookupTableNTSC[i] + 1;		// correction

		// VRC6 Saw
		Pitch = ((clock_ntsc * 16.0) / (Freq * 14.0)) - 0.5;
		m_iNoteLookupTableSaw[i] = (unsigned int)(Pitch - modfile_->GetDetuneOffset(2, i));		// // //

		// FDS
		Pitch = (Freq * 65536.0) / (clock_ntsc / 1.0) + 0.5;
		m_iNoteLookupTableFDS[i] = (unsigned int)(Pitch + modfile_->GetDetuneOffset(4, i));		// // //

		// N163
		Pitch = ((Freq * modfile_->GetNamcoChannels() * 983040.0) / clock_ntsc + 0.5) / 4;		// // //
		m_iNoteLookupTableN163[i] = (unsigned int)(Pitch + modfile_->GetDetuneOffset(5, i));		// // //

		if (m_iNoteLookupTableN163[i] > 0xFFFF)	// 0x3FFFF
			m_iNoteLookupTableN163[i] = 0xFFFF;	// 0x3FFFF

		// // // Sunsoft 5B uses NTSC table

		// // // SN76489
		Pitch = (clock_ntsc / Freq / 2) + 0.5;
		m_iNoteLookupTableSN76489[i] = (unsigned int)(Pitch/* + modfile_->GetDetuneOffset(6, i)*/);		// // //

		// // // VRC7
		if (i < NOTE_RANGE) {
			Pitch = Freq * 262144.0 / 49716.0 + 0.5;
			unsigned Reg = (unsigned int)(Pitch + modfile_->GetDetuneOffset(3, i));
			for (int j = 0; j < OCTAVE_RANGE; ++j)
				m_iNoteLookupTableVRC7[i + j * NOTE_RANGE] = Reg;		// // //
		}
	}

	// // // Setup note tables
	const auto GetNoteTable = [&] (chan_id_t ch) -> const unsigned * {
		switch (ch) {
		case chan_id_t::SQUARE1: case chan_id_t::SQUARE2: case chan_id_t::TRIANGLE:
			return Machine == PAL ? m_iNoteLookupTablePAL : m_iNoteLookupTableNTSC;
		case chan_id_t::VRC6_PULSE1: case chan_id_t::VRC6_PULSE2:
		case chan_id_t::MMC5_SQUARE1: case chan_id_t::MMC5_SQUARE2:
			return m_iNoteLookupTableNTSC;
		case chan_id_t::VRC6_SAWTOOTH:
			return m_iNoteLookupTableSaw;
		case chan_id_t::VRC7_CH1: case chan_id_t::VRC7_CH2: case chan_id_t::VRC7_CH3:
		case chan_id_t::VRC7_CH4: case chan_id_t::VRC7_CH5: case chan_id_t::VRC7_CH6:
			return m_iNoteLookupTableVRC7;
		case chan_id_t::FDS:
			return m_iNoteLookupTableFDS;
		case chan_id_t::N163_CH1: case chan_id_t::N163_CH2: case chan_id_t::N163_CH3: case chan_id_t::N163_CH4:
		case chan_id_t::N163_CH5: case chan_id_t::N163_CH6: case chan_id_t::N163_CH7: case chan_id_t::N163_CH8:
			return m_iNoteLookupTableN163;
		case chan_id_t::S5B_CH1: case chan_id_t::S5B_CH2: case chan_id_t::S5B_CH3:
			return m_iNoteLookupTableS5B;
		case chan_id_t::SN76489_CH1: case chan_id_t::SN76489_CH2: case chan_id_t::SN76489_CH3: case chan_id_t::SN76489_NOISE:
			return m_iNoteLookupTableSN76489;
		}
		return nullptr;
	};

	ForeachTrack([&] (CChannelHandler &ch, CTrackerChannel &, chan_id_t id) {
		if (auto Table = GetNoteTable(id))
			ch.SetNoteTable(Table);
	});
}

void CSoundDriver::HandleGlobalEffects(stChanNote &note) {
	for (int i = 0; i < MAX_EFFECT_COLUMNS; ++i) {
		unsigned char EffParam = note.EffParam[i];
		switch (note.EffNumber[i]) {
			// Fxx: Sets speed to xx
			case EF_SPEED:
				m_pTempoCounter->DoFxx(EffParam ? EffParam : 1);		// // //
				break;

			// Oxx: Sets groove to xx
			case EF_GROOVE:		// // //
				m_pTempoCounter->DoOxx(EffParam % MAX_GROOVE);		// // //
				break;

			// Bxx: Jump to pattern xx
			case EF_JUMP:
				m_iJumpToPattern = EffParam;
				break;

			// Dxx: Skip to next track and start at row xx
			case EF_SKIP:
				m_iSkipToRow = EffParam;
				break;

			// Cxx: Halt playback
			case EF_HALT:
				m_bDoHalt = true;		// // //
				m_pPlayerCursor->DoCxx();		// // //
				break;

			// // // NCx: SN76489 channel swap
			case EF_SN_CONTROL:
				if (EffParam >= 0xC1 && EffParam <= 0xC3)
					CChannelHandlerSN7::SwapChannels(MakeChannelIndex(sound_chip_t::SN76489, EffParam - 0xC1));
				break;

			default: continue;		// // //
		}

		note.EffNumber[i] = EF_NONE;
	}
}
