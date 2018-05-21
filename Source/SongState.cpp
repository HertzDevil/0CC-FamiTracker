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

#include "SongState.h"
#include "FamiTrackerModule.h"
#include "SongView.h"
#include "SongData.h"
#include "TrackerChannel.h"
#include "ft0cc/doc/groove.hpp"
#include "EffectName.h"
#include "NumConv.h"
#include "FamiTrackerEnv.h"
#include "SoundChipService.h"
#include <algorithm>



namespace {

void UpdateEchoTranspose(const ft0cc::doc::pattern_note &Note, int &Value, unsigned int EffColumns) {
	for (int j = EffColumns - 1; j >= 0; --j) {
		const int Param = Note.fx_param(j) & 0x0F;
		switch (Note.fx_name(j)) {
		case ft0cc::doc::effect_type::SLIDE_UP:
			Value += Param; return;
		case ft0cc::doc::effect_type::SLIDE_DOWN:
			Value -= Param; return;
		case ft0cc::doc::effect_type::TRANSPOSE: // Sometimes there are not enough ticks for the transpose to take place
			if (Note.fx_param(j) & 0x80)
				Value -= Param;
			else
				Value += Param;
			return;
		}
	}
}

} // namespace



std::string MakeCommandString(ft0cc::doc::effect_command cmd) {		// // //
	return {
		' ',
		EFF_CHAR[value_cast(cmd.fx)],
		conv::to_digit<char>(cmd.param >> 4),
		conv::to_digit<char>(cmd.param & 0x0Fu),
		'\0',
	};
}



stChannelState::stChannelState()
{
	Effect.fill(-1);
	Echo.fill(-1);
}

std::string stChannelState::GetStateString() const {
	std::string log("Inst.: ");
	if (Instrument == MAX_INSTRUMENTS)
		log += "None";
	else
		log += conv::sv_from_int_hex(Instrument, 2);
	log += "        Vol.: ";
	log += conv::to_digit<char>(Volume >= MAX_VOLUME ? 0xF : Volume);
	log += "        Active effects:";

	std::string effStr;

	const ft0cc::doc::effect_type SLIDE_EFFECT = Effect[value_cast(ft0cc::doc::effect_type::ARPEGGIO)] >= 0 ? ft0cc::doc::effect_type::ARPEGGIO :
		Effect[value_cast(ft0cc::doc::effect_type::PORTA_UP)] >= 0 ? ft0cc::doc::effect_type::PORTA_UP :
		Effect[value_cast(ft0cc::doc::effect_type::PORTA_DOWN)] >= 0 ? ft0cc::doc::effect_type::PORTA_DOWN :
		ft0cc::doc::effect_type::PORTAMENTO;
	for (const auto &x : {SLIDE_EFFECT, ft0cc::doc::effect_type::VIBRATO, ft0cc::doc::effect_type::TREMOLO, ft0cc::doc::effect_type::VOLUME_SLIDE, ft0cc::doc::effect_type::PITCH, ft0cc::doc::effect_type::DUTY_CYCLE}) {
		int p = Effect[value_cast(x)];
		if (p < 0) continue;
		if (p == 0 && x != ft0cc::doc::effect_type::PITCH) continue;
		if (p == 0x80 && x == ft0cc::doc::effect_type::PITCH) continue;
		effStr += MakeCommandString({x, static_cast<uint8_t>(p)});
	}

	if (IsAPUPulse(ChannelID) || IsAPUNoise(ChannelID) || ChannelID.Chip == sound_chip_t::MMC5)
		for (const auto &x : {ft0cc::doc::effect_type::VOLUME}) {
			if (int p = Effect[value_cast(x)]; p >= 0)
				effStr += MakeCommandString({x, static_cast<uint8_t>(p)});
		}
	else if (IsAPUTriangle(ChannelID))
		for (const auto &x : {ft0cc::doc::effect_type::VOLUME, ft0cc::doc::effect_type::NOTE_CUT}) {
			if (int p = Effect[value_cast(x)]; p >= 0)
				effStr += MakeCommandString({x, static_cast<uint8_t>(p)});
		}
	else if (IsDPCM(ChannelID))
		for (const auto &x : {ft0cc::doc::effect_type::SAMPLE_OFFSET, /*ft0cc::doc::effect_type::DPCM_PITCH*/}) {
			if (int p = Effect[value_cast(x)]; p > 0)
				effStr += MakeCommandString({x, static_cast<uint8_t>(p)});
		}
	else if (ChannelID.Chip == sound_chip_t::VRC7)
		for (const auto &x : {ft0cc::doc::effect_type::VRC7_PORT, ft0cc::doc::effect_type::VRC7_WRITE}) {
			if (int p = Effect[value_cast(x)]; p >= 0)
				effStr += MakeCommandString({x, static_cast<uint8_t>(p)});
		}
	else if (ChannelID.Chip == sound_chip_t::FDS) {
		for (const auto &x : {ft0cc::doc::effect_type::FDS_MOD_DEPTH, ft0cc::doc::effect_type::FDS_MOD_SPEED_HI, ft0cc::doc::effect_type::FDS_MOD_SPEED_LO, ft0cc::doc::effect_type::FDS_VOLUME}) {
			if (int p = Effect[value_cast(x)]; p >= 0)
				effStr += MakeCommandString({x, static_cast<uint8_t>(p)});
		}
		if (int p = Effect[value_cast(ft0cc::doc::effect_type::FDS_MOD_BIAS)]; p >= 0 && p != 0x80)
			effStr += MakeCommandString({ft0cc::doc::effect_type::FDS_MOD_BIAS, static_cast<uint8_t>(p)});
	}
	else if (ChannelID.Chip == sound_chip_t::S5B)
		for (const auto &x : {ft0cc::doc::effect_type::SUNSOFT_ENV_TYPE, ft0cc::doc::effect_type::SUNSOFT_ENV_HI, ft0cc::doc::effect_type::SUNSOFT_ENV_LO, ft0cc::doc::effect_type::SUNSOFT_NOISE}) {
			if (int p = Effect[value_cast(x)]; p > 0)
				effStr += MakeCommandString({x, static_cast<uint8_t>(p)});
		}
	else if (ChannelID.Chip == sound_chip_t::N163) {
		if (int p = Effect[value_cast(ft0cc::doc::effect_type::N163_WAVE_BUFFER)]; p >= 0 && p != 0x7F)
			effStr += MakeCommandString({ft0cc::doc::effect_type::N163_WAVE_BUFFER, static_cast<uint8_t>(p)});
	}
	if (Effect_LengthCounter >= 0)
		effStr += MakeCommandString({ft0cc::doc::effect_type::VOLUME, static_cast<uint8_t>(Effect_LengthCounter)});
	if (Effect_AutoFMMult >= 0)
		effStr += MakeCommandString({ft0cc::doc::effect_type::FDS_MOD_DEPTH, static_cast<uint8_t>(Effect_AutoFMMult)});

	if (!effStr.size()) effStr = " None";
	log += effStr;
	return log;
}

void stChannelState::HandleNote(const ft0cc::doc::pattern_note &Note, unsigned EffColumns) {
	if (Note.note() != ft0cc::doc::pitch::none && Note.note() != ft0cc::doc::pitch::release) {
		for (int i = 0; i < std::min(BufferPos, (int)std::size(Echo)); ++i) {
			if (Echo[i] == ECHO_BUFFER_ECHO) {
				UpdateEchoTranspose(Note, Transpose[i], EffColumns);
				switch (Note.note()) {
				case ft0cc::doc::pitch::halt: Echo[i] = ECHO_BUFFER_HALT; break;
				case ft0cc::doc::pitch::echo: Echo[i] = ECHO_BUFFER_ECHO + Note.oct(); break;
				default:
					int NewNote = Note.midi_note() + Transpose[i];
					NewNote = std::clamp(NewNote, 0, NOTE_COUNT - 1);
					Echo[i] = NewNote;
				}
			}
			else if (Echo[i] > ECHO_BUFFER_ECHO && Echo[i] < ECHO_BUFFER_ECHO + (int)ECHO_BUFFER_LENGTH)
				--Echo[i];
		}
		if (BufferPos >= 0 && BufferPos < (int)ECHO_BUFFER_LENGTH) {
			int Value;
			switch (Note.note()) {
			case ft0cc::doc::pitch::halt: Value = ECHO_BUFFER_HALT; break;
			case ft0cc::doc::pitch::echo: Value = ECHO_BUFFER_ECHO + Note.oct(); break;
			default:
				Value = Note.midi_note();
				UpdateEchoTranspose(Note, Value, EffColumns);
				Value = std::clamp(Value, 0, NOTE_COUNT - 1);
			}
			Echo[BufferPos] = Value;
			UpdateEchoTranspose(Note, Transpose[BufferPos], EffColumns);
		}
		++BufferPos;
	}
	if (BufferPos < 0)
		BufferPos = 0;

	if (Instrument == MAX_INSTRUMENTS)
		if (Note.inst() != MAX_INSTRUMENTS && Note.inst() != HOLD_INSTRUMENT)		// // // 050B
			Instrument = Note.inst();

	if (Volume == MAX_VOLUME)
		if (Note.vol() != MAX_VOLUME)
			Volume = Note.vol();
}

void stChannelState::HandleNormalCommand(ft0cc::doc::effect_command cmd) {
	if (Effect[value_cast(cmd.fx)] == -1)
		Effect[value_cast(cmd.fx)] = cmd.param;
}

void stChannelState::HandleSlideCommand(ft0cc::doc::effect_command cmd) {
	if (Effect[value_cast(ft0cc::doc::effect_type::PORTAMENTO)] == -1) { // anything else within can be used here
		Effect[value_cast(ft0cc::doc::effect_type::PORTAMENTO)] = cmd.fx == ft0cc::doc::effect_type::PORTAMENTO ? cmd.param : -2;
		Effect[value_cast(ft0cc::doc::effect_type::ARPEGGIO)] = cmd.fx == ft0cc::doc::effect_type::ARPEGGIO ? cmd.param : -2;
		Effect[value_cast(ft0cc::doc::effect_type::PORTA_UP)] = cmd.fx == ft0cc::doc::effect_type::PORTA_UP ? cmd.param : -2;
		Effect[value_cast(ft0cc::doc::effect_type::PORTA_DOWN)] = cmd.fx == ft0cc::doc::effect_type::PORTA_DOWN ? cmd.param : -2;
	}
}

void stChannelState::HandleExxCommand2A03(unsigned char param) {
	if (Effect_LengthCounter == -1 && param >= 0xE0 && param <= 0xE3)
		Effect_LengthCounter = param;
	else if (Effect[value_cast(ft0cc::doc::effect_type::VOLUME)] == -1 && param <= 0x1F) {
		Effect[value_cast(ft0cc::doc::effect_type::VOLUME)] = param;
		if (Effect_LengthCounter == -1)
			Effect_LengthCounter = IsAPUTriangle(ChannelID) ? 0xE1 : 0xE2;
	}
}

void stChannelState::HandleSxxCommand(unsigned char xy) {
	if (!IsAPUTriangle(ChannelID))
		return;
	if (Effect[value_cast(ft0cc::doc::effect_type::NOTE_CUT)] == -1) {
		if (xy <= 0x7F) {
			if (Effect_LengthCounter == -1)
				Effect_LengthCounter = 0xE0;
			return;
		}
		if (Effect_LengthCounter != 0xE0) {
			Effect[value_cast(ft0cc::doc::effect_type::NOTE_CUT)] = xy;
			if (Effect_LengthCounter == -1)
				Effect_LengthCounter = 0xE1;
		}
	}
}



void CSongState::Retrieve(const CFamiTrackerModule &modfile, unsigned Track, unsigned Frame, unsigned Row) {
	CConstSongView SongView {modfile.GetChannelOrder().Canonicalize(), *modfile.GetSong(Track), false};
	const auto &song = SongView.GetSong();

	State.clear();
	SongView.GetChannelOrder().ForeachChannel([&] (stChannelID id) {
		State.try_emplace(id, stChannelState { });
	});

	int totalRows = 0;

	bool maskFDS = false;
	bool doHalt = false;

	while (true) {
		if (Row)
			--Row;
		else if (Frame)
			Row = SongView.GetFrameLength(--Frame) - 1;
		else
			break;

		SongView.ForeachTrack([&] (const CTrackData &track, stChannelID c) {
			stChannelState &chState = State.find(c)->second;
			int EffColumns = track.GetEffectColumnCount();
			const auto &Note = track.GetPatternOnFrame(Frame).GetNoteOn(Row);		// // //

			chState.HandleNote(Note, EffColumns);

			for (int k = EffColumns - 1; k >= 0; --k) {
				ft0cc::doc::effect_command cmd = Note.fx_cmd(k);
				if (!IsEffectCompatible(c, cmd))
					continue;
				switch (cmd.fx) {
				// ignore effects that cannot have memory
				case ft0cc::doc::effect_type::none: case ft0cc::doc::effect_type::PORTAOFF:
				case ft0cc::doc::effect_type::DAC: case ft0cc::doc::effect_type::DPCM_PITCH: case ft0cc::doc::effect_type::RETRIGGER:
				case ft0cc::doc::effect_type::DELAY: case ft0cc::doc::effect_type::DELAYED_VOLUME: case ft0cc::doc::effect_type::NOTE_RELEASE: case ft0cc::doc::effect_type::TRANSPOSE:
				case ft0cc::doc::effect_type::JUMP: case ft0cc::doc::effect_type::SKIP: // no true backward iterator
					break;
				case ft0cc::doc::effect_type::HALT:
					doHalt = true;
					break;
				case ft0cc::doc::effect_type::SPEED:
					if (Speed == -1 && (cmd.param < modfile.GetSpeedSplitPoint() || song.GetSongTempo() == 0)) {
						Speed = std::max((unsigned char)1u, cmd.param);
						GroovePos = -2;
					}
					else if (Tempo == -1 && cmd.param >= modfile.GetSpeedSplitPoint())
						Tempo = cmd.param;
					break;
				case ft0cc::doc::effect_type::GROOVE:
					if (GroovePos == -1 && cmd.param < MAX_GROOVE && modfile.HasGroove(cmd.param)) {
						GroovePos = totalRows + 1;
						Speed = cmd.param;
					}
					break;
				case ft0cc::doc::effect_type::VOLUME:
					chState.HandleExxCommand2A03(cmd.param);
					break;
				case ft0cc::doc::effect_type::NOTE_CUT:
					chState.HandleSxxCommand(cmd.param);
					break;
				case ft0cc::doc::effect_type::FDS_MOD_DEPTH:
					if (chState.Effect_AutoFMMult == -1 && cmd.param >= 0x80)
						chState.Effect_AutoFMMult = cmd.param;
					break;
				case ft0cc::doc::effect_type::FDS_MOD_SPEED_HI:
					if (cmd.param <= 0x0F)
						maskFDS = true;
					else if (!maskFDS && chState.Effect[value_cast(cmd.fx)] == -1) {
						chState.Effect[value_cast(cmd.fx)] = cmd.param;
						if (chState.Effect_AutoFMMult == -1)
							chState.Effect_AutoFMMult = -2;
					}
					break;
				case ft0cc::doc::effect_type::FDS_MOD_SPEED_LO:
					maskFDS = true;
					break;
				case ft0cc::doc::effect_type::DUTY_CYCLE:
					if (c.Chip == sound_chip_t::VRC7)		// // // 050B
						break;
					[[fallthrough]];
				case ft0cc::doc::effect_type::SAMPLE_OFFSET:
				case ft0cc::doc::effect_type::FDS_VOLUME: case ft0cc::doc::effect_type::FDS_MOD_BIAS:
				case ft0cc::doc::effect_type::SUNSOFT_ENV_LO: case ft0cc::doc::effect_type::SUNSOFT_ENV_HI: case ft0cc::doc::effect_type::SUNSOFT_ENV_TYPE:
				case ft0cc::doc::effect_type::N163_WAVE_BUFFER:
				case ft0cc::doc::effect_type::VRC7_PORT:
				case ft0cc::doc::effect_type::VIBRATO: case ft0cc::doc::effect_type::TREMOLO: case ft0cc::doc::effect_type::PITCH: case ft0cc::doc::effect_type::VOLUME_SLIDE:
					chState.HandleNormalCommand(cmd);
					break;
				case ft0cc::doc::effect_type::SWEEPUP: case ft0cc::doc::effect_type::SWEEPDOWN: case ft0cc::doc::effect_type::SLIDE_UP: case ft0cc::doc::effect_type::SLIDE_DOWN:
				case ft0cc::doc::effect_type::PORTAMENTO: case ft0cc::doc::effect_type::ARPEGGIO: case ft0cc::doc::effect_type::PORTA_UP: case ft0cc::doc::effect_type::PORTA_DOWN:
					chState.HandleSlideCommand(cmd);
					break;
				}
			}
		});
		if (doHalt)
			break;
		++totalRows;
	}
	if (GroovePos == -1 && song.GetSongGroove()) {
		unsigned Index = song.GetSongSpeed();
		if (Index < MAX_GROOVE && modfile.HasGroove(Index)) {
			GroovePos = totalRows;
			Speed = Index;
		}
	}
}

std::string CSongState::GetChannelStateString(const CFamiTrackerModule &modfile, stChannelID chan) const {
	if (State.empty())
		return "State is not ready";

	std::string str = State.find(chan)->second.GetStateString();
	if (Tempo >= 0)
		str += "        Tempo: " + std::to_string(Tempo);
	if (Speed >= 0) {
		if (const auto pGroove = modfile.GetGroove(Speed); pGroove && GroovePos >= 0) {
			str += "        Groove: ";
			str += {conv::to_digit<char>(Speed >> 4), conv::to_digit<char>(Speed), ' ', '<', '-'};
			unsigned Size = pGroove->size();
			for (unsigned i = 0; i < Size; ++i)
				str += ' ' + std::to_string(pGroove->entry((i + GroovePos) % Size));
		}
		else
			str += "        Speed: " + std::to_string(Speed);
	}

	return str;
}
