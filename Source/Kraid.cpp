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

#include "Kraid.h"
#include "FamiTrackerDoc.h"
#include "InstrumentManager.h"
#include "Instrument2A03.h"
#include "SongData.h"
#include "Sequence.h"

void Kraid::operator()(CFamiTrackerDoc &doc) {
	doc.CreateEmpty();
	buildDoc(doc);
	buildSong(*doc.GetSong(0));
}

void Kraid::buildDoc(CFamiTrackerDoc &doc) {
	// Instruments and sequences
	makeInst(doc, 0, 6, "Lead ");
	makeInst(doc, 1, 2, "Echo");
	makeInst(doc, 2, 15, "Triangle");
}

void Kraid::buildSong(CSongData &song) {
	const unsigned FRAMES = 14;
	const unsigned ROWS = 24;
	const unsigned PATTERNS[][FRAMES] = {
		{0, 0, 0, 0, 1, 1, 2, 3, 3, 3, 4, 5, 6, 6},
		{0, 0, 0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4},
		{0, 0, 0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	};

	song.SetFrameCount(FRAMES);
	song.SetPatternLength(ROWS);
	song.SetSongSpeed(8);
	song.SetEffectColumnCount(0, 1);

	for (int ch = 0; ch < FRAMES; ++ch)
		for (int f = 0; f < ROWS; ++f)
			song.SetFramePattern(f, ch, PATTERNS[ch][f]);

	makePattern(song, 2, 0, "<e.>e...<e.>e...<e.>e...<e.>e...");
	makePattern(song, 2, 1, "<c.>c...<c.>c...<d.>d...<d.>d...");
	makePattern(song, 2, 2, "<e.>e.>e.<<F.>F.>F.<<f.>f.>f.<<<b.>b.>b.");
	makePattern(song, 2, 3, "<e...b.>c...<b.c...g.a...b.");
	makePattern(song, 2, 4, "<<e");

	makePattern(song, 1, 0, "@e...<b.>a... c. F...d.<b...A.");
	makePattern(song, 1, 1, "@g... d. e...<b.>F...d. a...e.");
	makePattern(song, 1, 2, "@g<b>g<b>g<b>AeAeAeacacacaDFDbD");
	makePattern(song, 1, 3, "Fgab>d<b>Fd<agFb>aFd<agFega>de-");
	makePattern(song, 1, 4, ">a-g-F-e-F-g-a-g-F-e-F-g-");

	int f = 0;
	int r = 0;
	do { // TODO: use CSongIterator
		auto note = song.GetPatternOnFrame(1, f).GetNoteOn(r);
		if (++r >= ROWS) {
			r = 0;
			if (++f >= FRAMES)
				f = 0;
		}
		if (note != stChanNote { }) {
			note.Instrument = 1;
			note.EffNumber[1] = EF_DELAY;
			note.EffParam[1] = 3;
			song.GetPatternOnFrame(0, f).SetNoteOn(r, note);
		}
	} while (f || r);
}

void Kraid::makeInst(CFamiTrackerDoc &doc, unsigned index, char vol, const char * name) {
	doc.AddInstrument(doc.GetInstrumentManager()->CreateNew(INST_2A03), index);
	auto leadInst = std::dynamic_pointer_cast<CInstrument2A03>(doc.GetInstrument(index));
	leadInst->SetSeqEnable(sequence_t::Volume, true);
	leadInst->SetSeqIndex(sequence_t::Volume, index);
	leadInst->SetName(name);

	CSequence &leadEnv = *leadInst->GetSequence(sequence_t::Volume);
	leadEnv.SetItemCount(1);
	leadEnv.SetItem(0, vol);
	leadEnv.SetLoopPoint(-1);
	leadEnv.SetReleasePoint(-1);
}

void Kraid::makePattern(CSongData &song, unsigned ch, unsigned pat, std::string_view mml) {
	const uint8_t INST = ch == 1 ? 0 : 2;
	uint8_t octave = 3;
	int row = 0;
	auto &pattern = song.GetPattern(ch, pat);

	for (auto c : mml) {
		auto &note = pattern.GetNoteOn(row);
		switch (c) {
		case '<': --octave; break;
		case '>': ++octave; break;
		case '.': ++row; break;
		case '-': ++row; note.Note = HALT; break;
		case '=': ++row; note.Note = RELEASE; break;
		case 'c': ++row; note.Note = NOTE_C; note.Octave = octave, note.Instrument = INST; break;
		case 'C': ++row; note.Note = NOTE_Cs; note.Octave = octave, note.Instrument = INST; break;
		case 'd': ++row; note.Note = NOTE_D; note.Octave = octave, note.Instrument = INST; break;
		case 'D': ++row; note.Note = NOTE_Ds; note.Octave = octave, note.Instrument = INST; break;
		case 'e': ++row; note.Note = NOTE_E; note.Octave = octave, note.Instrument = INST; break;
		case 'f': ++row; note.Note = NOTE_F; note.Octave = octave, note.Instrument = INST; break;
		case 'F': ++row; note.Note = NOTE_Fs; note.Octave = octave, note.Instrument = INST; break;
		case 'g': ++row; note.Note = NOTE_G; note.Octave = octave, note.Instrument = INST; break;
		case 'G': ++row; note.Note = NOTE_Gs; note.Octave = octave, note.Instrument = INST; break;
		case 'a': ++row; note.Note = NOTE_A; note.Octave = octave, note.Instrument = INST; break;
		case 'A': ++row; note.Note = NOTE_As; note.Octave = octave, note.Instrument = INST; break;
		case 'b': ++row; note.Note = NOTE_B; note.Octave = octave, note.Instrument = INST; break;
		case '@': note.EffNumber[0] = EF_DUTY_CYCLE; note.EffParam[0] = 2; break;
		}
	}
}
