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

#include "APU/Types_fwd.h"		// // //

/*
 * Here are the constants that defines the limits in the tracker
 * change if needed (some might cause side effects)
 *
 */

// Maximum number of instruments to use
const int MAX_INSTRUMENTS = 64;

// Hold instrument index
const int HOLD_INSTRUMENT = 0xFF;		// // // 050B
// TODO: check if this conflicts with INVALID_INSTRUMENT

// Maximum number of sequence lists
const int MAX_SEQUENCES	= 128;

// Maximum number of items in each sequence
const int MAX_SEQUENCE_ITEMS = /*128*/ 252;		// TODO: need to check if this exports correctly

// Maximum number of patterns per channel
const int MAX_PATTERN = 256;		// // //

// Maximum number of frames
const int MAX_FRAMES = 256;		// // //

// Maximum length of patterns (in rows). 256 is max in NSF
const int MAX_PATTERN_LENGTH = 256;

// Maximum number of DPCM samples, cannot be increased unless the NSF driver is modified.
const int MAX_DSAMPLES = 64;

// Sample space available (from $C000-$FFFF), may now switch banks
const int MAX_SAMPLE_SPACE = 0x40000;	// 256kB

// Number of effect columns allowed
const int MAX_EFFECT_COLUMNS = 4;

// Maximum numbers of tracks allowed (NSF limit is 256, but dunno if the bankswitcher can handle that)
const unsigned int MAX_TRACKS = 64;

// Max tempo
const int MAX_TEMPO	= 255;

// Min tempo
//const int MIN_TEMPO	= 21;

// // // Default tempo
const unsigned int DEFAULT_TEMPO_NTSC = 150;
const unsigned int DEFAULT_TEMPO_PAL  = 125;

// Max speed
//const int MAX_SPEED = 20;

// Min speed
const int MIN_SPEED = 1;

// // // Default speed
const unsigned int DEFAULT_SPEED = 6;

// // // Maximum number of grooves
const int MAX_GROOVE = 32;

// // // Maximum number of entries in the echo buffer
const int ECHO_BUFFER_LENGTH = 3;

const int OCTAVE_RANGE = 8;
const int DEFAULT_OCTAVE = 3;		// // //
const int NOTE_RANGE   = 12;
const int NOTE_COUNT   = OCTAVE_RANGE * NOTE_RANGE;		// // // moved from SoundGen.h

const int INVALID_INSTRUMENT = -1;

// Max allowed value in volume column. The actual meaning is no specific volume information, rather than max volume.
const int MAX_VOLUME = 0x10;

// Channel effects
enum effect_t : unsigned char {
	EF_NONE = 0,
	EF_SPEED,
	EF_JUMP,
	EF_SKIP,
	EF_HALT,
	EF_VOLUME,
	EF_PORTAMENTO,
	EF_PORTAOFF,				// unused!!
	EF_SWEEPUP,
	EF_SWEEPDOWN,
	EF_ARPEGGIO,
	EF_VIBRATO,
	EF_TREMOLO,
	EF_PITCH,
	EF_DELAY,
	EF_DAC,
	EF_PORTA_UP,
	EF_PORTA_DOWN,
	EF_DUTY_CYCLE,
	EF_SAMPLE_OFFSET,
	EF_SLIDE_UP,
	EF_SLIDE_DOWN,
	EF_VOLUME_SLIDE,
	EF_NOTE_CUT,
	EF_RETRIGGER,
	EF_DELAYED_VOLUME,			// // //
	EF_FDS_MOD_DEPTH,
	EF_FDS_MOD_SPEED_HI,
	EF_FDS_MOD_SPEED_LO,
	EF_DPCM_PITCH,
	EF_SUNSOFT_ENV_TYPE,
	EF_SUNSOFT_ENV_HI,
	EF_SUNSOFT_ENV_LO,
	EF_SUNSOFT_NOISE,		// // // 050B
	EF_VRC7_PORT,		// // // 050B
	EF_VRC7_WRITE,		// // // 050B
	EF_NOTE_RELEASE,			// // //
	EF_GROOVE,					// // //
	EF_TRANSPOSE,				// // //
	EF_N163_WAVE_BUFFER,		// // //
	EF_FDS_VOLUME,				// // //
	EF_FDS_MOD_BIAS,				// // //
//	EF_TARGET_VOLUME_SLIDE,
/*
	EF_VRC7_MODULATOR,
	EF_VRC7_CARRIER,
	EF_VRC7_LEVELS,
*/
	EF_COUNT,
};

// DPCM  effects
//const int EF_DPCM_PITCH = EF_SWEEPUP;		// DPCM pitch, 'H'

//const int EF_VRC7_PATCH = EF_DUTY_CYCLE;	// VRC7 patch setting, 'V'

// FDS effects
//const int EF_FDS_MOD_DEPTH = EF_SWEEPUP;	// FDS modulation depth, 'H'

//const int EF_RETRIGGER = EF_SWEEPDOWN;

// const effect_t VRC6_EFFECTS[] = {};
const effect_t VRC7_EFFECTS[] = {EF_VRC7_PORT, EF_VRC7_WRITE};
const effect_t FDS_EFFECTS[] = {EF_FDS_MOD_DEPTH, EF_FDS_MOD_SPEED_HI, EF_FDS_MOD_SPEED_LO, EF_FDS_VOLUME, EF_FDS_MOD_BIAS};
// const effect_t MMC5_EFFECTS[] = {};
const effect_t N163_EFFECTS[] = {EF_N163_WAVE_BUFFER};
const effect_t S5B_EFFECTS[] = {EF_SUNSOFT_ENV_TYPE, EF_SUNSOFT_ENV_HI, EF_SUNSOFT_ENV_LO, EF_SUNSOFT_NOISE};

// Channel effect letters
const char EFF_CHAR[] = {
	'F',	// Speed
	'B',	// Jump
	'D',	// Skip
	'C',	// Halt
	'E',	// Volume
	'3',	// Porta on
	 0,		// Porta off		// unused
	'H',	// Sweep up
	'I',	// Sweep down
	'0',	// Arpeggio
	'4',	// Vibrato
	'7',	// Tremolo
	'P',	// Pitch
	'G',	// Note delay
	'Z',	// DAC setting
	'1',	// Portamento up
	'2',	// Portamento down
	'V',	// Duty cycle
	'Y',	// Sample offset
	'Q',	// Slide up
	'R',	// Slide down
	'A',	// Volume slide
	'S',	// Note cut
	'X',	// DPCM retrigger
	'M',	// // // Delayed channel volume
	'H',	// FDS modulation depth
	'I',	// FDS modulation speed hi
	'J',	// FDS modulation speed lo
	'W',	// DPCM Pitch
	'H',	// Sunsoft envelope type
	'I',	// Sunsoft envelope high
	'J',	// Sunsoft envelope low
	'W',	// // // 050B Sunsoft noise period
	'H',	// // // 050B VRC7 custom patch port
	'I',	// // // 050B VRC7 custom patch write
	'L',	// // // Delayed release
	'O',	// // // Groove
	'T',	// // // Delayed transpose
	'Z',	// // // N163 wave buffer
	'E',	// // // FDS volume envelope
	'Z',	// // // FDS auto-FM bias
	//'9',	// Targeted volume slide
	/*
	'H',	// VRC7 modulator
	'I',	// VRC7 carrier
	'J',	// VRC7 modulator/feedback level
	*/
};

effect_t GetEffectFromChar(char ch, sound_chip_t Chip, bool *bValid = nullptr);		// // //

enum class note_t : unsigned char {
	NONE = 0,					// No note
	C,  Cs, D,  Ds, E,  F,		// // // renamed
	Fs, G,  Gs, A,  As, B,
	RELEASE,					// Release, begin note release sequence
	HALT,						// Halt, stops note
	ECHO,						// // // Echo buffer access, octave determines position
};

constexpr auto value_cast(note_t n) noexcept { // TODO: use enum_traits
	return static_cast<unsigned char>(n);
}

constexpr bool IsNote(note_t n) noexcept {
	return n >= note_t::C && n <= note_t::B;
}

// // // special echo buffer constants
const char ECHO_BUFFER_NONE = '\xFF';
const char ECHO_BUFFER_HALT = '\x7F';
const char ECHO_BUFFER_ECHO = '\x80';

enum machine_t : unsigned char {
	NTSC,
	PAL,
};

inline constexpr machine_t DEFAULT_MACHINE_TYPE = machine_t::NTSC;		// // //
inline constexpr int DEFAULT_TEMPO = DEFAULT_MACHINE_TYPE == machine_t::PAL ? DEFAULT_TEMPO_PAL : DEFAULT_TEMPO_NTSC;		// // //

enum vibrato_t : unsigned char {
	VIBRATO_OLD = 0,
	VIBRATO_NEW,
};

constexpr int MIDI_NOTE(int octave, note_t note) noexcept {		// // //
	if (IsNote(note))
		return octave * NOTE_RANGE + static_cast<unsigned>(note) - 1;
	return -1;
}

constexpr int GET_OCTAVE(int midi_note) noexcept {
	int x = midi_note / NOTE_RANGE;
	if (midi_note < 0 && !(midi_note % NOTE_RANGE))
		--x;
	return x;
}

constexpr note_t GET_NOTE(int midi_note) noexcept {
	int x = midi_note % NOTE_RANGE;
	if (x < 0)
		x += NOTE_RANGE;
	return static_cast<note_t>(++x);
}
