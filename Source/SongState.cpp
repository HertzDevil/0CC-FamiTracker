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
#include <algorithm>
#include "NumConv.h"
#include "FamiTrackerEnv.h"
#include "SoundChipService.h"



namespace {

void UpdateEchoTranspose(const stChanNote &Note, int &Value, unsigned int EffColumns) {
	for (int j = EffColumns - 1; j >= 0; --j) {
		const int Param = Note.Effects[j].param & 0x0F;
		switch (Note.Effects[j].fx) {
		case effect_t::SLIDE_UP:
			Value += Param; return;
		case effect_t::SLIDE_DOWN:
			Value -= Param; return;
		case effect_t::TRANSPOSE: // Sometimes there are not enough ticks for the transpose to take place
			if (Note.Effects[j].param & 0x80)
				Value -= Param;
			else
				Value += Param;
			return;
		}
	}
}

} // namespace



std::string MakeCommandString(stEffectCommand cmd) {		// // //
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

	const effect_t SLIDE_EFFECT = Effect[value_cast(effect_t::ARPEGGIO)] >= 0 ? effect_t::ARPEGGIO :
		Effect[value_cast(effect_t::PORTA_UP)] >= 0 ? effect_t::PORTA_UP :
		Effect[value_cast(effect_t::PORTA_DOWN)] >= 0 ? effect_t::PORTA_DOWN :
		effect_t::PORTAMENTO;
	for (const auto &x : {SLIDE_EFFECT, effect_t::VIBRATO, effect_t::TREMOLO, effect_t::VOLUME_SLIDE, effect_t::PITCH, effect_t::DUTY_CYCLE}) {
		int p = Effect[value_cast(x)];
		if (p < 0) continue;
		if (p == 0 && x != effect_t::PITCH) continue;
		if (p == 0x80 && x == effect_t::PITCH) continue;
		effStr += MakeCommandString({x, static_cast<uint8_t>(p)});
	}

	if (IsAPUPulse(ChannelID) || IsAPUNoise(ChannelID) || ChannelID.Chip == sound_chip_t::MMC5)
		for (const auto &x : {effect_t::VOLUME}) {
			if (int p = Effect[value_cast(x)]; p >= 0)
				effStr += MakeCommandString({x, static_cast<uint8_t>(p)});
		}
	else if (IsAPUTriangle(ChannelID))
		for (const auto &x : {effect_t::VOLUME, effect_t::NOTE_CUT}) {
			if (int p = Effect[value_cast(x)]; p >= 0)
				effStr += MakeCommandString({x, static_cast<uint8_t>(p)});
		}
	else if (IsDPCM(ChannelID))
		for (const auto &x : {effect_t::SAMPLE_OFFSET, /*effect_t::DPCM_PITCH*/}) {
			if (int p = Effect[value_cast(x)]; p > 0)
				effStr += MakeCommandString({x, static_cast<uint8_t>(p)});
		}
	else if (ChannelID.Chip == sound_chip_t::VRC7)
		for (const auto &x : {effect_t::VRC7_PORT, effect_t::VRC7_WRITE}) {
			if (int p = Effect[value_cast(x)]; p >= 0)
				effStr += MakeCommandString({x, static_cast<uint8_t>(p)});
		}
	else if (ChannelID.Chip == sound_chip_t::FDS) {
		for (const auto &x : {effect_t::FDS_MOD_DEPTH, effect_t::FDS_MOD_SPEED_HI, effect_t::FDS_MOD_SPEED_LO, effect_t::FDS_VOLUME}) {
			if (int p = Effect[value_cast(x)]; p >= 0)
				effStr += MakeCommandString({x, static_cast<uint8_t>(p)});
		}
		if (int p = Effect[value_cast(effect_t::FDS_MOD_BIAS)]; p >= 0 && p != 0x80)
			effStr += MakeCommandString({effect_t::FDS_MOD_BIAS, static_cast<uint8_t>(p)});
	}
	else if (ChannelID.Chip == sound_chip_t::S5B)
		for (const auto &x : {effect_t::SUNSOFT_ENV_TYPE, effect_t::SUNSOFT_ENV_HI, effect_t::SUNSOFT_ENV_LO, effect_t::SUNSOFT_NOISE}) {
			if (int p = Effect[value_cast(x)]; p > 0)
				effStr += MakeCommandString({x, static_cast<uint8_t>(p)});
		}
	else if (ChannelID.Chip == sound_chip_t::N163) {
		if (int p = Effect[value_cast(effect_t::N163_WAVE_BUFFER)]; p >= 0 && p != 0x7F)
			effStr += MakeCommandString({effect_t::N163_WAVE_BUFFER, static_cast<uint8_t>(p)});
	}
	if (Effect_LengthCounter >= 0)
		effStr += MakeCommandString({effect_t::VOLUME, static_cast<uint8_t>(Effect_LengthCounter)});
	if (Effect_AutoFMMult >= 0)
		effStr += MakeCommandString({effect_t::FDS_MOD_DEPTH, static_cast<uint8_t>(Effect_AutoFMMult)});

	if (!effStr.size()) effStr = " None";
	log += effStr;
	return log;
}

void stChannelState::HandleNote(const stChanNote &Note, unsigned EffColumns) {
	if (Note.Note != note_t::none && Note.Note != note_t::release) {
		for (int i = 0; i < std::min(BufferPos, (int)std::size(Echo)); ++i) {
			if (Echo[i] == ECHO_BUFFER_ECHO) {
				UpdateEchoTranspose(Note, Transpose[i], EffColumns);
				switch (Note.Note) {
				case note_t::halt: Echo[i] = ECHO_BUFFER_HALT; break;
				case note_t::echo: Echo[i] = ECHO_BUFFER_ECHO + Note.Octave; break;
				default:
					int NewNote = Note.ToMidiNote() + Transpose[i];
					NewNote = std::clamp(NewNote, 0, NOTE_COUNT - 1);
					Echo[i] = NewNote;
				}
			}
			else if (Echo[i] > ECHO_BUFFER_ECHO && Echo[i] < ECHO_BUFFER_ECHO + (int)ECHO_BUFFER_LENGTH)
				--Echo[i];
		}
		if (BufferPos >= 0 && BufferPos < (int)ECHO_BUFFER_LENGTH) {
			int Value;
			switch (Note.Note) {
			case note_t::halt: Value = ECHO_BUFFER_HALT; break;
			case note_t::echo: Value = ECHO_BUFFER_ECHO + Note.Octave; break;
			default:
				Value = Note.ToMidiNote();
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
		if (Note.Instrument != MAX_INSTRUMENTS && Note.Instrument != HOLD_INSTRUMENT)		// // // 050B
			Instrument = Note.Instrument;

	if (Volume == MAX_VOLUME)
		if (Note.Vol != MAX_VOLUME)
			Volume = Note.Vol;
}

void stChannelState::HandleNormalCommand(stEffectCommand cmd) {
	if (Effect[value_cast(cmd.fx)] == -1)
		Effect[value_cast(cmd.fx)] = cmd.param;
}

void stChannelState::HandleSlideCommand(stEffectCommand cmd) {
	if (Effect[value_cast(effect_t::PORTAMENTO)] == -1) { // anything else within can be used here
		Effect[value_cast(effect_t::PORTAMENTO)] = cmd.fx == effect_t::PORTAMENTO ? cmd.param : -2;
		Effect[value_cast(effect_t::ARPEGGIO)] = cmd.fx == effect_t::ARPEGGIO ? cmd.param : -2;
		Effect[value_cast(effect_t::PORTA_UP)] = cmd.fx == effect_t::PORTA_UP ? cmd.param : -2;
		Effect[value_cast(effect_t::PORTA_DOWN)] = cmd.fx == effect_t::PORTA_DOWN ? cmd.param : -2;
	}
}

void stChannelState::HandleExxCommand2A03(unsigned char param) {
	if (Effect_LengthCounter == -1 && param >= 0xE0 && param <= 0xE3)
		Effect_LengthCounter = param;
	else if (Effect[value_cast(effect_t::VOLUME)] == -1 && param <= 0x1F) {
		Effect[value_cast(effect_t::VOLUME)] = param;
		if (Effect_LengthCounter == -1)
			Effect_LengthCounter = IsAPUTriangle(ChannelID) ? 0xE1 : 0xE2;
	}
}

void stChannelState::HandleSxxCommand(unsigned char xy) {
	if (!IsAPUTriangle(ChannelID))
		return;
	if (Effect[value_cast(effect_t::NOTE_CUT)] == -1) {
		if (xy <= 0x7F) {
			if (Effect_LengthCounter == -1)
				Effect_LengthCounter = 0xE0;
			return;
		}
		if (Effect_LengthCounter != 0xE0) {
			Effect[value_cast(effect_t::NOTE_CUT)] = xy;
			if (Effect_LengthCounter == -1)
				Effect_LengthCounter = 0xE1;
		}
	}
}



void CSongState::Retrieve(const CFamiTrackerModule &modfile, unsigned Track, unsigned Frame, unsigned Row) {
	CConstSongView SongView {modfile.GetChannelOrder().Canonicalize(), *modfile.GetSong(Track)};
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
				const stEffectCommand &cmd = Note.Effects[k];
				if (!IsEffectCompatible(c, cmd))
					continue;
				switch (cmd.fx) {
				// ignore effects that cannot have memory
				case effect_t::none: case effect_t::PORTAOFF:
				case effect_t::DAC: case effect_t::DPCM_PITCH: case effect_t::RETRIGGER:
				case effect_t::DELAY: case effect_t::DELAYED_VOLUME: case effect_t::NOTE_RELEASE: case effect_t::TRANSPOSE:
				case effect_t::JUMP: case effect_t::SKIP: // no true backward iterator
					break;
				case effect_t::HALT:
					doHalt = true;
					break;
				case effect_t::SPEED:
					if (Speed == -1 && (cmd.param < modfile.GetSpeedSplitPoint() || song.GetSongTempo() == 0)) {
						Speed = std::max((unsigned char)1u, cmd.param);
						GroovePos = -2;
					}
					else if (Tempo == -1 && cmd.param >= modfile.GetSpeedSplitPoint())
						Tempo = cmd.param;
					break;
				case effect_t::GROOVE:
					if (GroovePos == -1 && cmd.param < MAX_GROOVE && modfile.HasGroove(cmd.param)) {
						GroovePos = totalRows + 1;
						Speed = cmd.param;
					}
					break;
				case effect_t::VOLUME:
					chState.HandleExxCommand2A03(cmd.param);
					break;
				case effect_t::NOTE_CUT:
					chState.HandleSxxCommand(cmd.param);
					break;
				case effect_t::FDS_MOD_DEPTH:
					if (chState.Effect_AutoFMMult == -1 && cmd.param >= 0x80)
						chState.Effect_AutoFMMult = cmd.param;
					break;
				case effect_t::FDS_MOD_SPEED_HI:
					if (cmd.param <= 0x0F)
						maskFDS = true;
					else if (!maskFDS && chState.Effect[value_cast(cmd.fx)] == -1) {
						chState.Effect[value_cast(cmd.fx)] = cmd.param;
						if (chState.Effect_AutoFMMult == -1)
							chState.Effect_AutoFMMult = -2;
					}
					break;
				case effect_t::FDS_MOD_SPEED_LO:
					maskFDS = true;
					break;
				case effect_t::DUTY_CYCLE:
					if (c.Chip == sound_chip_t::VRC7)		// // // 050B
						break;
					[[fallthrough]];
				case effect_t::SAMPLE_OFFSET:
				case effect_t::FDS_VOLUME: case effect_t::FDS_MOD_BIAS:
				case effect_t::SUNSOFT_ENV_LO: case effect_t::SUNSOFT_ENV_HI: case effect_t::SUNSOFT_ENV_TYPE:
				case effect_t::N163_WAVE_BUFFER:
				case effect_t::VRC7_PORT:
				case effect_t::VIBRATO: case effect_t::TREMOLO: case effect_t::PITCH: case effect_t::VOLUME_SLIDE:
					chState.HandleNormalCommand(cmd);
					break;
				case effect_t::SWEEPUP: case effect_t::SWEEPDOWN: case effect_t::SLIDE_UP: case effect_t::SLIDE_DOWN:
				case effect_t::PORTAMENTO: case effect_t::ARPEGGIO: case effect_t::PORTA_UP: case effect_t::PORTA_DOWN:
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
