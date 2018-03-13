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
	for (int j = EffColumns - 1; j >= 0; --j) {
		const int Param = Note.EffParam[j] & 0x0F;
		switch (Note.EffNumber[j]) {
		case effect_t::SLIDE_UP:
			Value += Param; return;
		case effect_t::SLIDE_DOWN:
			Value -= Param; return;
		case effect_t::TRANSPOSE: // Sometimes there are not enough ticks for the transpose to take place
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
	return {' ', EFF_CHAR[value_cast(Effect)], conv::to_digit<char>(Param >> 4), conv::to_digit<char>(Param & 0x0Fu), '\0'};
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
		effStr += MakeCommandString(x, p);
	}

	if ((ChannelID >= chan_id_t::SQUARE1 && ChannelID <= chan_id_t::SQUARE2) ||
		ChannelID == chan_id_t::NOISE ||
		(ChannelID >= chan_id_t::MMC5_SQUARE1 && ChannelID <= chan_id_t::MMC5_SQUARE2))
		for (const auto &x : {effect_t::VOLUME}) {
			int p = Effect[value_cast(x)];
			if (p < 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (ChannelID == chan_id_t::TRIANGLE)
		for (const auto &x : {effect_t::VOLUME, effect_t::NOTE_CUT}) {
			int p = Effect[value_cast(x)];
			if (p < 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (ChannelID == chan_id_t::DPCM)
		for (const auto &x : {effect_t::SAMPLE_OFFSET, /*effect_t::DPCM_PITCH*/}) {
			int p = Effect[value_cast(x)];
			if (p <= 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (GetChipFromChannel(ChannelID) == sound_chip_t::VRC7)
		for (const auto &x : VRC7_EFFECTS) {
			int p = Effect[value_cast(x)];
			if (p < 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (GetChipFromChannel(ChannelID) == sound_chip_t::FDS)
		for (const auto &x : FDS_EFFECTS) {
			int p = Effect[value_cast(x)];
			if (p < 0 || (x == effect_t::FDS_MOD_BIAS && p == 0x80)) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (GetChipFromChannel(ChannelID) == sound_chip_t::S5B)
		for (const auto &x : S5B_EFFECTS) {
			int p = Effect[value_cast(x)];
			if (p < 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (GetChipFromChannel(ChannelID) == sound_chip_t::N163)
		for (const auto &x : N163_EFFECTS) {
			int p = Effect[value_cast(x)];
			if (p < 0 || (x == effect_t::N163_WAVE_BUFFER && p == 0x7F)) continue;
			effStr += MakeCommandString(x, p);
		}
	if (Effect_LengthCounter >= 0)
		effStr += MakeCommandString(effect_t::VOLUME, Effect_LengthCounter);
	if (Effect_AutoFMMult >= 0)
		effStr += MakeCommandString(effect_t::FDS_MOD_DEPTH, Effect_AutoFMMult);

	if (!effStr.size()) effStr = " None";
	log += effStr;
	return log;
}

void stChannelState::HandleNote(const stChanNote &Note, unsigned EffColumns) {
	if (Note.Note != note_t::NONE && Note.Note != note_t::RELEASE) {
		for (int i = 0; i < std::min(BufferPos, (int)ECHO_BUFFER_LENGTH); ++i) {
			if (Echo[i] == ECHO_BUFFER_ECHO) {
				UpdateEchoTranspose(Note, Transpose[i], EffColumns);
				switch (Note.Note) {
				case note_t::HALT: Echo[i] = ECHO_BUFFER_HALT; break;
				case note_t::ECHO: Echo[i] = ECHO_BUFFER_ECHO + Note.Octave; break;
				default:
					int NewNote = MIDI_NOTE(Note.Octave, Note.Note) + Transpose[i];
					NewNote = std::clamp(NewNote, 0, NOTE_COUNT - 1);
					Echo[i] = NewNote;
				}
			}
			else if (Echo[i] > ECHO_BUFFER_ECHO && Echo[i] < ECHO_BUFFER_ECHO + ECHO_BUFFER_LENGTH)
				--Echo[i];
		}
		if (BufferPos >= 0 && BufferPos < ECHO_BUFFER_LENGTH) {
			int Value;
			switch (Note.Note) {
			case note_t::HALT: Value = ECHO_BUFFER_HALT; break;
			case note_t::ECHO: Value = ECHO_BUFFER_ECHO + Note.Octave; break;
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

void stChannelState::HandleNormalCommand(effect_t fx, unsigned char param) {
	if (Effect[value_cast(fx)] == -1)
		Effect[value_cast(fx)] = param;
}

void stChannelState::HandleSlideCommand(effect_t fx, unsigned char param) {
	if (Effect[value_cast(effect_t::PORTAMENTO)] == -1) { // anything else within can be used here
		Effect[value_cast(effect_t::PORTAMENTO)] = fx == effect_t::PORTAMENTO ? param : -2;
		Effect[value_cast(effect_t::ARPEGGIO)] = fx == effect_t::ARPEGGIO ? param : -2;
		Effect[value_cast(effect_t::PORTA_UP)] = fx == effect_t::PORTA_UP ? param : -2;
		Effect[value_cast(effect_t::PORTA_DOWN)] = fx == effect_t::PORTA_DOWN ? param : -2;
	}
}

void stChannelState::HandleExxCommand2A03(unsigned char param) {
	if (Effect_LengthCounter == -1 && param >= 0xE0 && param <= 0xE3)
		Effect_LengthCounter = param;
	else if (Effect[value_cast(effect_t::VOLUME)] == -1 && param <= 0x1F) {
		Effect[value_cast(effect_t::VOLUME)] = param;
		if (Effect_LengthCounter == -1)
			Effect_LengthCounter = ChannelID == chan_id_t::TRIANGLE ? 0xE1 : 0xE2;
	}
}

void stChannelState::HandleSxxCommand(unsigned char xy) {
	if (ChannelID != chan_id_t::TRIANGLE)
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



CSongState::CSongState() {
	for (chan_id_t i = (chan_id_t)0; i < chan_id_t::COUNT; i = (chan_id_t)(value_cast(i) + 1))
		State[value_cast(i)].ChannelID = i;
}

void CSongState::Retrieve(const CFamiTrackerModule &modfile, unsigned Track, unsigned Frame, unsigned Row) {
	CConstSongView SongView {modfile.GetChannelOrder().Canonicalize(), *modfile.GetSong(Track)};
	const auto &song = SongView.GetSong();

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

		SongView.ForeachTrack([&] (const CTrackData &track, chan_id_t c) {
			stChannelState &chState = State[value_cast(c)];
			int EffColumns = track.GetEffectColumnCount();
			const auto &Note = track.GetPatternOnFrame(Frame).GetNoteOn(Row);		// // //

			chState.HandleNote(Note, EffColumns);

			for (int k = EffColumns - 1; k >= 0; --k) {
				effect_t fx = Note.EffNumber[k];
				unsigned char xy = Note.EffParam[k];
				if (!IsEffectCompatible(c, fx, xy))
					continue;
				switch (fx) {
				// ignore effects that cannot have memory
				case effect_t::NONE: case effect_t::PORTAOFF:
				case effect_t::DAC: case effect_t::DPCM_PITCH: case effect_t::RETRIGGER:
				case effect_t::DELAY: case effect_t::DELAYED_VOLUME: case effect_t::NOTE_RELEASE: case effect_t::TRANSPOSE:
				case effect_t::JUMP: case effect_t::SKIP: // no true backward iterator
					break;
				case effect_t::HALT:
					doHalt = true;
					break;
				case effect_t::SPEED:
					if (Speed == -1 && (xy < modfile.GetSpeedSplitPoint() || song.GetSongTempo() == 0)) {
						Speed = std::max((unsigned char)1u, xy);
						GroovePos = -2;
					}
					else if (Tempo == -1 && xy >= modfile.GetSpeedSplitPoint())
						Tempo = xy;
					break;
				case effect_t::GROOVE:
					if (GroovePos == -1 && xy < MAX_GROOVE && modfile.HasGroove(xy)) {
						GroovePos = totalRows + 1;
						Speed = xy;
					}
					break;
				case effect_t::VOLUME:
					chState.HandleExxCommand2A03(xy);
					break;
				case effect_t::NOTE_CUT:
					chState.HandleSxxCommand(xy);
					break;
				case effect_t::FDS_MOD_DEPTH:
					if (chState.Effect_AutoFMMult == -1 && xy >= 0x80)
						chState.Effect_AutoFMMult = xy;
					break;
				case effect_t::FDS_MOD_SPEED_HI:
					if (xy <= 0x0F)
						maskFDS = true;
					else if (!maskFDS && chState.Effect[value_cast(fx)] == -1) {
						chState.Effect[value_cast(fx)] = xy;
						if (chState.Effect_AutoFMMult == -1)
							chState.Effect_AutoFMMult = -2;
					}
					break;
				case effect_t::FDS_MOD_SPEED_LO:
					maskFDS = true;
					break;
				case effect_t::DUTY_CYCLE:
					if (GetChipFromChannel(c) == sound_chip_t::VRC7)		// // // 050B
						break;
					[[fallthrough]];
				case effect_t::SAMPLE_OFFSET:
				case effect_t::FDS_VOLUME: case effect_t::FDS_MOD_BIAS:
				case effect_t::SUNSOFT_ENV_LO: case effect_t::SUNSOFT_ENV_HI: case effect_t::SUNSOFT_ENV_TYPE:
				case effect_t::N163_WAVE_BUFFER:
				case effect_t::VRC7_PORT:
				case effect_t::VIBRATO: case effect_t::TREMOLO: case effect_t::PITCH: case effect_t::VOLUME_SLIDE:
					chState.HandleNormalCommand(fx, xy);
					break;
				case effect_t::SWEEPUP: case effect_t::SWEEPDOWN: case effect_t::SLIDE_UP: case effect_t::SLIDE_DOWN:
				case effect_t::PORTAMENTO: case effect_t::ARPEGGIO: case effect_t::PORTA_UP: case effect_t::PORTA_DOWN:
					chState.HandleSlideCommand(fx, xy);
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

std::string CSongState::GetChannelStateString(const CFamiTrackerModule &modfile, chan_id_t chan) const {
	std::string str = State[modfile.GetChannelOrder().GetChannelIndex(chan)].GetStateString();
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
