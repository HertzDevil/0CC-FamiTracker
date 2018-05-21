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

// Channel effect letters
constexpr char EFF_CHAR[] = {
	 0,		// // // blank
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
