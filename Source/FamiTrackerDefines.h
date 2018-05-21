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

#include "ft0cc/doc/constants.hpp"		// // //
#include "ft0cc/doc/pitch.hpp"		// // // TODO: remove

/*
 * Here are the constants that define the limits in the tracker
 * Change if needed (some might cause side effects)
 */

// Maximum number of instruments to use
const int MAX_INSTRUMENTS = ft0cc::doc::max_instruments;

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
const int MAX_EFFECT_COLUMNS = ft0cc::doc::max_effect_columns;

// Maximum numbers of tracks allowed (NSF limit is 256, but dunno if the bankswitcher can handle that)
const unsigned int MAX_TRACKS = 64;

// Max tempo
const int MAX_TEMPO	= 255;

// // // Default tempo
const unsigned int DEFAULT_TEMPO_NTSC = 150;
const unsigned int DEFAULT_TEMPO_PAL  = 125;

// Min speed
const int MIN_SPEED = 1;

// // // Default speed
const unsigned int DEFAULT_SPEED = 6;

// // // Maximum number of grooves
const int MAX_GROOVE = 32;

// // // Maximum number of entries in the echo buffer
const std::size_t ECHO_BUFFER_LENGTH = 4u;

const int OCTAVE_RANGE = 8;
const int DEFAULT_OCTAVE = 3;		// // //
const int NOTE_RANGE   = ft0cc::doc::note_range;
const int NOTE_COUNT   = OCTAVE_RANGE * NOTE_RANGE;		// // // moved from SoundGen.h

const int INVALID_INSTRUMENT = -1;

// Max allowed value in volume column. The actual meaning is no specific volume information, rather than max volume.
const int MAX_VOLUME = ft0cc::doc::max_volumes;
