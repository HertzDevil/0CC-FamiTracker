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

#include "TrackerChannel.h"
#include "Instrument.h"		// // //
#include "APU/Types.h"		// // //

/*
 * This class serves as the interface between the UI and the sound player for each channel
 * Thread synchronization should be done here
 *
 */

void CTrackerChannel::SetNote(const stChanNote &Note, note_prio_t Priority)		// // //
{
	std::lock_guard<std::mutex> lock {m_csNoteLock};

	if (Priority >= m_iNotePriority) {
		m_Note = Note;
		m_bNewNote = true;
		m_iNotePriority = Priority;
	}
}

stChanNote CTrackerChannel::GetNote()
{
	std::lock_guard<std::mutex> lock {m_csNoteLock};

	stChanNote Note = m_Note;		// // //
	m_Note = stChanNote { };
	m_bNewNote = false;
	m_iNotePriority = NOTE_PRIO_0;

	return Note;
}

bool CTrackerChannel::NewNoteData() const
{
	return m_bNewNote;
}

void CTrackerChannel::Reset()
{
	std::lock_guard<std::mutex> lock {m_csNoteLock};

	m_Note = stChanNote { };		// // //
	m_bNewNote = false;
	m_iPitch = 0;		// // //
	m_iVolumeMeter = 0;
	m_iNotePriority = NOTE_PRIO_0;
}

void CTrackerChannel::SetVolumeMeter(int Value)
{
	std::lock_guard<std::mutex> lock {m_csNoteLock};		// // //

	m_iVolumeMeter = Value;
}

int CTrackerChannel::GetVolumeMeter() const
{
	std::lock_guard<std::mutex> lock {m_csNoteLock};		// // //

	return m_iVolumeMeter;
}

void CTrackerChannel::SetPitch(int Pitch)
{
	std::lock_guard<std::mutex> lock {m_csNoteLock};		// // //

	m_iPitch = Pitch;
}

int CTrackerChannel::GetPitch() const
{
	std::lock_guard<std::mutex> lock {m_csNoteLock};		// // //

	return m_iPitch;
}

bool IsInstrumentCompatible(sound_chip_t Chip, inst_type_t Type) {		// // //
	switch (Chip) {
	case sound_chip_t::APU:
	case sound_chip_t::MMC5:
	case sound_chip_t::N163:		// // //
	case sound_chip_t::S5B:
	case sound_chip_t::VRC6:
	case sound_chip_t::FDS:
		switch (Type) {
		case INST_2A03:
		case INST_VRC6:
		case INST_N163:
		case INST_S5B:
		case INST_FDS:
			return true;
		default: return false;
		}
	case sound_chip_t::VRC7:
		return Type == INST_VRC7;
	}

	return false;
}

bool IsEffectCompatible(stChannelID ch, stEffectCommand cmd) {		// // //
	switch (cmd.fx) {
		case effect_t::none:
		case effect_t::SPEED: case effect_t::JUMP: case effect_t::SKIP: case effect_t::HALT:
		case effect_t::DELAY:
			return true;
		case effect_t::NOTE_CUT: case effect_t::NOTE_RELEASE:
			return cmd.param <= 0x7F || IsAPUTriangle(ch);
		case effect_t::GROOVE:
			return cmd.param < MAX_GROOVE;
		case effect_t::VOLUME:
			return ((ch.Chip == sound_chip_t::APU && !IsDPCM(ch)) || ch.Chip == sound_chip_t::MMC5) &&
				(cmd.param <= 0x1F || (cmd.param >= 0xE0 && cmd.param <= 0xE3));
		case effect_t::PORTAMENTO: case effect_t::ARPEGGIO: case effect_t::VIBRATO: case effect_t::TREMOLO:
		case effect_t::PITCH: case effect_t::PORTA_UP: case effect_t::PORTA_DOWN: case effect_t::SLIDE_UP: case effect_t::SLIDE_DOWN:
		case effect_t::VOLUME_SLIDE: case effect_t::DELAYED_VOLUME: case effect_t::TRANSPOSE:
			return !IsDPCM(ch);
		case effect_t::PORTAOFF:
			return false;
		case effect_t::SWEEPUP: case effect_t::SWEEPDOWN:
			return IsAPUPulse(ch);
		case effect_t::DAC: case effect_t::SAMPLE_OFFSET: case effect_t::RETRIGGER: case effect_t::DPCM_PITCH:
			return IsDPCM(ch);
		case effect_t::DUTY_CYCLE:
			return !IsDPCM(ch);		// // // 050B
		case effect_t::FDS_MOD_DEPTH:
			return ch.Chip == sound_chip_t::FDS && (cmd.param <= 0x3F || cmd.param >= 0x80);
		case effect_t::FDS_MOD_SPEED_HI: case effect_t::FDS_MOD_SPEED_LO: case effect_t::FDS_MOD_BIAS:
			return ch.Chip == sound_chip_t::FDS;
		case effect_t::SUNSOFT_ENV_LO: case effect_t::SUNSOFT_ENV_HI: case effect_t::SUNSOFT_ENV_TYPE:
		case effect_t::SUNSOFT_NOISE:		// // // 050B
			return ch.Chip == sound_chip_t::S5B;
		case effect_t::N163_WAVE_BUFFER:
			return ch.Chip == sound_chip_t::N163 && cmd.param <= 0x7F;
		case effect_t::FDS_VOLUME:
			return ch.Chip == sound_chip_t::FDS && (cmd.param <= 0x7F || cmd.param == 0xE0);
		case effect_t::VRC7_PORT: case effect_t::VRC7_WRITE:		// // // 050B
			return ch.Chip == sound_chip_t::VRC7;
	}

	return false;
}

/*
int CTrackerChannel::GetEffect(int Letter) const
{
	if (m_iChip == sound_chip_t::FDS) {
	}

	return
}
*/
