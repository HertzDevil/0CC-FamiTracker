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


// CTrackerChannel

#include <mutex>		// // //
#include "PatternNote.h"		// // //
#include "APU/Types_fwd.h"		// // //

enum inst_type_t : unsigned;

enum note_prio_t : unsigned {
	NOTE_PRIO_0,
	NOTE_PRIO_1,
	NOTE_PRIO_2,
};

bool IsInstrumentCompatible(sound_chip_t chip, inst_type_t Type);		// // //
bool IsEffectCompatible(stChannelID ch, stEffectCommand cmd);		// // //

class CTrackerChannel
{
public:
	stChanNote GetNote();
	void SetNote(const stChanNote &Note, note_prio_t Priority);		// // //
	bool NewNoteData() const;
	void Reset();

	void SetVolumeMeter(int Value);
	int GetVolumeMeter() const;

	void SetPitch(int Pitch);
	int GetPitch() const;

private:
	stChanNote m_Note;
	note_prio_t m_iNotePriority = NOTE_PRIO_0;

	int m_iVolumeMeter = 0;
	int m_iPitch = 0;

	bool m_bNewNote = false;

private:
	mutable std::mutex m_csNoteLock;		// // //
};
