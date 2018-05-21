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
#include "FamiTrackerDefines.h"		// // //

/*
 * This class serves as the interface between the UI and the sound player for each channel
 * Thread synchronization should be done here
 *
 */

void CTrackerChannel::SetNote(const ft0cc::doc::pattern_note &Note, note_prio_t Priority)		// // //
{
	std::lock_guard<std::mutex> lock {m_csNoteLock};

	if (Priority >= m_iNotePriority) {
		m_Note = Note;
		m_bNewNote = true;
		m_iNotePriority = Priority;
	}
}

ft0cc::doc::pattern_note CTrackerChannel::GetNote()
{
	std::lock_guard<std::mutex> lock {m_csNoteLock};

	ft0cc::doc::pattern_note Note = m_Note;		// // //
	m_Note = ft0cc::doc::pattern_note { };
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

	m_Note = ft0cc::doc::pattern_note { };		// // //
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

bool IsEffectCompatible(stChannelID ch, ft0cc::doc::effect_command cmd) {		// // //
	switch (cmd.fx) {
		case ft0cc::doc::effect_type::none:
		case ft0cc::doc::effect_type::SPEED: case ft0cc::doc::effect_type::JUMP: case ft0cc::doc::effect_type::SKIP: case ft0cc::doc::effect_type::HALT:
		case ft0cc::doc::effect_type::DELAY:
			return true;
		case ft0cc::doc::effect_type::NOTE_CUT: case ft0cc::doc::effect_type::NOTE_RELEASE:
			return cmd.param <= 0x7F || IsAPUTriangle(ch);
		case ft0cc::doc::effect_type::GROOVE:
			return cmd.param < MAX_GROOVE;
		case ft0cc::doc::effect_type::VOLUME:
			return ((ch.Chip == sound_chip_t::APU && !IsDPCM(ch)) || ch.Chip == sound_chip_t::MMC5) &&
				(cmd.param <= 0x1F || (cmd.param >= 0xE0 && cmd.param <= 0xE3));
		case ft0cc::doc::effect_type::PORTAMENTO: case ft0cc::doc::effect_type::ARPEGGIO: case ft0cc::doc::effect_type::VIBRATO: case ft0cc::doc::effect_type::TREMOLO:
		case ft0cc::doc::effect_type::PITCH: case ft0cc::doc::effect_type::PORTA_UP: case ft0cc::doc::effect_type::PORTA_DOWN: case ft0cc::doc::effect_type::SLIDE_UP: case ft0cc::doc::effect_type::SLIDE_DOWN:
		case ft0cc::doc::effect_type::VOLUME_SLIDE: case ft0cc::doc::effect_type::DELAYED_VOLUME: case ft0cc::doc::effect_type::TRANSPOSE:
			return !IsDPCM(ch);
		case ft0cc::doc::effect_type::PORTAOFF:
			return false;
		case ft0cc::doc::effect_type::SWEEPUP: case ft0cc::doc::effect_type::SWEEPDOWN:
			return IsAPUPulse(ch);
		case ft0cc::doc::effect_type::DAC: case ft0cc::doc::effect_type::SAMPLE_OFFSET: case ft0cc::doc::effect_type::RETRIGGER: case ft0cc::doc::effect_type::DPCM_PITCH:
			return IsDPCM(ch);
		case ft0cc::doc::effect_type::DUTY_CYCLE:
			return !IsDPCM(ch);		// // // 050B
		case ft0cc::doc::effect_type::FDS_MOD_DEPTH:
			return ch.Chip == sound_chip_t::FDS && (cmd.param <= 0x3F || cmd.param >= 0x80);
		case ft0cc::doc::effect_type::FDS_MOD_SPEED_HI: case ft0cc::doc::effect_type::FDS_MOD_SPEED_LO: case ft0cc::doc::effect_type::FDS_MOD_BIAS:
			return ch.Chip == sound_chip_t::FDS;
		case ft0cc::doc::effect_type::SUNSOFT_ENV_LO: case ft0cc::doc::effect_type::SUNSOFT_ENV_HI: case ft0cc::doc::effect_type::SUNSOFT_ENV_TYPE:
		case ft0cc::doc::effect_type::SUNSOFT_NOISE:		// // // 050B
			return ch.Chip == sound_chip_t::S5B;
		case ft0cc::doc::effect_type::N163_WAVE_BUFFER:
			return ch.Chip == sound_chip_t::N163 && cmd.param <= 0x7F;
		case ft0cc::doc::effect_type::FDS_VOLUME:
			return ch.Chip == sound_chip_t::FDS && (cmd.param <= 0x7F || cmd.param == 0xE0);
		case ft0cc::doc::effect_type::VRC7_PORT: case ft0cc::doc::effect_type::VRC7_WRITE:		// // // 050B
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
