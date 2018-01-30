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



namespace {

void UpdateEchoTranspose(const stChanNote &Note, int &Value, unsigned int EffColumns) {
	for (int j = EffColumns; j >= 0; --j) {
		const int Param = Note.EffParam[j] & 0x0F;
		switch (Note.EffNumber[j]) {
		case EF_SLIDE_UP:
			Value += Param; return;
		case EF_SLIDE_DOWN:
			Value -= Param; return;
		case EF_TRANSPOSE: // Sometimes there are not enough ticks for the transpose to take place
			if (Note.EffParam[j] & 0x80)
				Value -= Param;
			else
				Value += Param;
			return;
		}
	}
}

} // namespace



std::string MakeCommandString(effect_t Effect, unsigned char Param) {		// // //
	return {' ', EFF_CHAR[Effect - 1], conv::to_digit(Param >> 4), conv::to_digit(Param)};
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
	log += conv::to_digit(Volume >= MAX_VOLUME ? 0xF : Volume);
	log += "        Active effects:";

	std::string effStr;

	const effect_t SLIDE_EFFECT = Effect[EF_ARPEGGIO] >= 0 ? EF_ARPEGGIO :
		Effect[EF_PORTA_UP] >= 0 ? EF_PORTA_UP :
		Effect[EF_PORTA_DOWN] >= 0 ? EF_PORTA_DOWN :
		EF_PORTAMENTO;
	for (const auto &x : {SLIDE_EFFECT, EF_VIBRATO, EF_TREMOLO, EF_VOLUME_SLIDE, EF_PITCH, EF_DUTY_CYCLE}) {
		int p = Effect[x];
		if (p < 0) continue;
		if (p == 0 && x != EF_PITCH) continue;
		if (p == 0x80 && x == EF_PITCH) continue;
		effStr += MakeCommandString(x, p);
	}

	if ((ChannelID >= chan_id_t::SQUARE1 && ChannelID <= chan_id_t::SQUARE2) ||
		ChannelID == chan_id_t::NOISE ||
		(ChannelID >= chan_id_t::MMC5_SQUARE1 && ChannelID <= chan_id_t::MMC5_SQUARE2))
		for (const auto &x : {EF_VOLUME}) {
			int p = Effect[x];
			if (p < 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (ChannelID == chan_id_t::TRIANGLE)
		for (const auto &x : {EF_VOLUME, EF_NOTE_CUT}) {
			int p = Effect[x];
			if (p < 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (ChannelID == chan_id_t::DPCM)
		for (const auto &x : {EF_SAMPLE_OFFSET, /*EF_DPCM_PITCH*/}) {
			int p = Effect[x];
			if (p <= 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (GetChipFromChannel(ChannelID) == sound_chip_t::VRC7)
		for (const auto &x : VRC7_EFFECTS) {
			int p = Effect[x];
			if (p < 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (GetChipFromChannel(ChannelID) == sound_chip_t::FDS)
		for (const auto &x : FDS_EFFECTS) {
			int p = Effect[x];
			if (p < 0 || (x == EF_FDS_MOD_BIAS && p == 0x80)) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (GetChipFromChannel(ChannelID) == sound_chip_t::S5B)
		for (const auto &x : S5B_EFFECTS) {
			int p = Effect[x];
			if (p < 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (GetChipFromChannel(ChannelID) == sound_chip_t::N163)
		for (const auto &x : N163_EFFECTS) {
			int p = Effect[x];
			if (p < 0 || (x == EF_N163_WAVE_BUFFER && p == 0x7F)) continue;
			effStr += MakeCommandString(x, p);
		}
	if (Effect_LengthCounter >= 0)
		effStr += MakeCommandString(EF_VOLUME, Effect_LengthCounter);
	if (Effect_AutoFMMult >= 0)
		effStr += MakeCommandString(EF_FDS_MOD_DEPTH, Effect_AutoFMMult);

	if (!effStr.size()) effStr = " None";
	log += effStr;
	return log;
}

void stChannelState::HandleNote(const stChanNote &Note, unsigned EffColumns) {
	if (Note.Note != NONE && Note.Note != RELEASE) {
		for (int i = 0; i < std::min(BufferPos, ECHO_BUFFER_LENGTH + 1); i++) {
			if (Echo[i] == ECHO_BUFFER_ECHO) {
				UpdateEchoTranspose(Note, Transpose[i], EffColumns);
				switch (Note.Note) {
				case HALT: Echo[i] = ECHO_BUFFER_HALT; break;
				case ECHO: Echo[i] = ECHO_BUFFER_ECHO + Note.Octave; break;
				default:
					int NewNote = MIDI_NOTE(Note.Octave, Note.Note) + Transpose[i];
					NewNote = std::clamp(NewNote, 0, NOTE_COUNT - 1);
					Echo[i] = NewNote;
				}
			}
			else if (Echo[i] > ECHO_BUFFER_ECHO && Echo[i] <= ECHO_BUFFER_ECHO + ECHO_BUFFER_LENGTH)
				Echo[i]--;
		}
		if (BufferPos >= 0 && BufferPos <= ECHO_BUFFER_LENGTH) {
			// WriteEchoBuffer(&Note, BufferPos, EffColumns);
			int Value;
			switch (Note.Note) {
			case HALT: Value = ECHO_BUFFER_HALT; break;
			case ECHO: Value = ECHO_BUFFER_ECHO + Note.Octave; break;
			default:
				Value = MIDI_NOTE(Note.Octave, Note.Note);
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

void stChannelState::HandleNormalCommand(unsigned char fx, unsigned char param) {
	if (Effect[fx] == -1)
		Effect[fx] = param;
}

void stChannelState::HandleSlideCommand(unsigned char fx, unsigned char param) {
	if (Effect[EF_PORTAMENTO] == -1) { // anything else within can be used here
		Effect[EF_PORTAMENTO] = fx == EF_PORTAMENTO ? param : -2;
		Effect[EF_ARPEGGIO] = fx == EF_ARPEGGIO ? param : -2;
		Effect[EF_PORTA_UP] = fx == EF_PORTA_UP ? param : -2;
		Effect[EF_PORTA_DOWN] = fx == EF_PORTA_DOWN ? param : -2;
	}
}

void stChannelState::HandleExxCommand2A03(unsigned char param) {
	if (Effect_LengthCounter == -1 && param >= 0xE0 && param <= 0xE3)
		Effect_LengthCounter = param;
	else if (Effect[EF_VOLUME] == -1 && param <= 0x1F) {
		Effect[EF_VOLUME] = param;
		if (Effect_LengthCounter == -1)
			Effect_LengthCounter = ChannelID == chan_id_t::TRIANGLE ? 0xE1 : 0xE2;
	}
}

void stChannelState::HandleSxxCommand(unsigned char xy) {
	if (ChannelID != chan_id_t::TRIANGLE)
		return;
	if (Effect[EF_NOTE_CUT] == -1) {
		if (xy <= 0x7F) {
			if (Effect_LengthCounter == -1)
				Effect_LengthCounter = 0xE0;
			return;
		}
		if (Effect_LengthCounter != 0xE0) {
			Effect[EF_NOTE_CUT] = xy;
			if (Effect_LengthCounter == -1)
				Effect_LengthCounter = 0xE1;
		}
	}
}



CSongState::CSongState() {
	for (chan_id_t i = (chan_id_t)0; i < chan_id_t::COUNT; i = (chan_id_t)(value_cast(i) + 1))
		State[value_cast(i)].ChannelID = i;
}

void CSongState::Retrieve(const CFamiTrackerModule &modfile, unsigned Track, unsigned Frame, unsigned Row) {
	CConstSongView SongView {modfile.GetChannelOrder().Canonicalize(), *modfile.GetSong(Track)};
	const auto &order = SongView.GetChannelOrder();
	const auto &song = SongView.GetSong();

	int totalRows = 0;

	bool maskFDS = false;
	bool doHalt = false;

	while (true) {
		SongView.ForeachTrack([&] (const CTrackData &track, chan_id_t c) {
			stChannelState &chState = State[value_cast(c)];
			int EffColumns = track.GetEffectColumnCount();
			const auto &Note = track.GetPatternOnFrame(Frame).GetNoteOn(Row);		// // //

			chState.HandleNote(Note, EffColumns);

			for (int k = EffColumns; k >= 0; k--) {
				unsigned char fx = Note.EffNumber[k], xy = Note.EffParam[k];
				if (!IsEffectCompatible(c, fx, xy))
					continue;
				switch (fx) {
				// ignore effects that cannot have memory
				case EF_NONE: case EF_PORTAOFF:
				case EF_DAC: case EF_DPCM_PITCH: case EF_RETRIGGER:
				case EF_DELAY: case EF_DELAYED_VOLUME: case EF_NOTE_RELEASE: case EF_TRANSPOSE:
				case EF_JUMP: case EF_SKIP: // no true backward iterator
					break;
				case EF_HALT:
					doHalt = true;
					break;
				case EF_SPEED:
					if (Speed == -1 && (xy < modfile.GetSpeedSplitPoint() || song.GetSongTempo() == 0)) {
						Speed = std::max((unsigned char)1u, xy);
						GroovePos = -2;
					}
					else if (Tempo == -1 && xy >= modfile.GetSpeedSplitPoint())
						Tempo = xy;
					break;
				case EF_GROOVE:
					if (GroovePos == -1 && xy < MAX_GROOVE && modfile.HasGroove(xy)) {
						GroovePos = totalRows;
						Speed = xy;
					}
					break;
				case EF_VOLUME:
					chState.HandleExxCommand2A03(xy);
					break;
				case EF_NOTE_CUT:
					chState.HandleSxxCommand(xy);
					break;
				case EF_FDS_MOD_DEPTH:
					if (chState.Effect_AutoFMMult == -1 && xy >= 0x80)
						chState.Effect_AutoFMMult = xy;
					break;
				case EF_FDS_MOD_SPEED_HI:
					if (xy <= 0x0F)
						maskFDS = true;
					else if (!maskFDS && chState.Effect[fx] == -1) {
						chState.Effect[fx] = xy;
						if (chState.Effect_AutoFMMult == -1)
							chState.Effect_AutoFMMult = -2;
					}
					break;
				case EF_FDS_MOD_SPEED_LO:
					maskFDS = true;
					break;
				case EF_DUTY_CYCLE:
					if (GetChipFromChannel(c) == sound_chip_t::VRC7)		// // // 050B
						break;
					[[fallthrough]];
				case EF_SAMPLE_OFFSET:
				case EF_FDS_VOLUME: case EF_FDS_MOD_BIAS:
				case EF_SUNSOFT_ENV_LO: case EF_SUNSOFT_ENV_HI: case EF_SUNSOFT_ENV_TYPE:
				case EF_N163_WAVE_BUFFER:
				case EF_VRC7_PORT:
				case EF_VIBRATO: case EF_TREMOLO: case EF_PITCH: case EF_VOLUME_SLIDE:
					chState.HandleNormalCommand(fx, xy);
					break;
				case EF_SWEEPUP: case EF_SWEEPDOWN: case EF_SLIDE_UP: case EF_SLIDE_DOWN:
				case EF_PORTAMENTO: case EF_ARPEGGIO: case EF_PORTA_UP: case EF_PORTA_DOWN:
					chState.HandleSlideCommand(fx, xy);
					break;
				}
			}
		});
		if (doHalt)
			break;
		if (Row)
			Row--;
		else if (Frame)
			Row = SongView.GetFrameLength(--Frame) - 1;
		else
			break;
		totalRows++;
	}
	if (GroovePos == -1 && song.GetSongGroove()) {
		unsigned Index = song.GetSongSpeed();
		if (Index < MAX_GROOVE && modfile.HasGroove(Index)) {
			GroovePos = totalRows;
			Speed = Index;
		}
	}
}

std::string CSongState::GetChannelStateString(const CFamiTrackerModule &modfile, chan_id_t chan) const {
	std::string str = State[modfile.GetChannelOrder().GetChannelIndex(chan)].GetStateString();
	if (Tempo >= 0)
		str += "        Tempo: " + std::to_string(Tempo);
	if (Speed >= 0) {
		if (const auto pGroove = modfile.GetGroove(Speed); pGroove && GroovePos >= 0) {
			str += "        Groove: ";
			str += {conv::to_digit(Speed >> 4), conv::to_digit(Speed), ' ', '<', '-'};
			unsigned Size = pGroove->size();
			for (unsigned i = 0; i < Size; ++i)
				str += ' ' + std::to_string(pGroove->entry((i + GroovePos) % Size));
		}
		else
			str += "        Speed: " + std::to_string(Speed);
	}

	return str;
}
