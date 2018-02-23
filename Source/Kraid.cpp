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
#include "FamiTrackerModule.h"
#include "InstrumentManager.h"
#include "Instrument2A03.h"
#include "SongData.h"
#include "Sequence.h"

void Kraid::operator()(CFamiTrackerModule &modfile) {
	buildDoc(modfile);
	buildSong(*modfile.GetSong(0));
}

void Kraid::buildDoc(CFamiTrackerModule &modfile) {
	// Instruments and sequences
	makeInst(modfile, 0, 6, "Lead ");
	makeInst(modfile, 1, 2, "Echo");
	makeInst(modfile, 2, 15, "Triangle");
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
	song.SetEffectColumnCount(chan_id_t::SQUARE1, 1);

	for (int ch = 0; ch < FRAMES; ++ch)
		for (int f = 0; f < ROWS; ++f)
			song.SetFramePattern(f, (chan_id_t)ch, PATTERNS[ch][f]);

	makePattern(song, chan_id_t::TRIANGLE, 0, "<e.>e...<e.>e...<e.>e...<e.>e...");
	makePattern(song, chan_id_t::TRIANGLE, 1, "<c.>c...<c.>c...<d.>d...<d.>d...");
	makePattern(song, chan_id_t::TRIANGLE, 2, "<e.>e.>e.<<F.>F.>F.<<f.>f.>f.<<<b.>b.>b.");
	makePattern(song, chan_id_t::TRIANGLE, 3, "<e...b.>c...<b.c...g.a...b.");
	makePattern(song, chan_id_t::TRIANGLE, 4, "<<e");

	makePattern(song, chan_id_t::SQUARE2, 0, "@e...<b.>a... c. F...d.<b...A.");
	makePattern(song, chan_id_t::SQUARE2, 1, "@g... d. e...<b.>F...d. a...e.");
	makePattern(song, chan_id_t::SQUARE2, 2, "@g<b>g<b>g<b>AeAeAeacacacaDFDbD");
	makePattern(song, chan_id_t::SQUARE2, 3, "Fgab>d<b>Fd<agFb>aFd<agFega>de-");
	makePattern(song, chan_id_t::SQUARE2, 4, ">a-g-F-e-F-g-a-g-F-e-F-g-");

	int f = 0;
	int r = 0;
	do { // TODO: use CSongIterator
		auto note = song.GetPatternOnFrame(chan_id_t::SQUARE2, f).GetNoteOn(r);
		if (++r >= ROWS) {
			r = 0;
			if (++f >= FRAMES)
				f = 0;
		}
		if (note != stChanNote { }) {
			note.Instrument = 1;
			note.EffNumber[1] = effect_t::DELAY;
			note.EffParam[1] = 3;
			song.GetPatternOnFrame(chan_id_t::SQUARE1, f).SetNoteOn(r, note);
		}
	} while (f || r);
}

void Kraid::makeInst(CFamiTrackerModule &modfile, unsigned index, char vol, std::string_view name) {
	auto *pManager = modfile.GetInstrumentManager();
	pManager->InsertInstrument(index, pManager->CreateNew(INST_2A03));
	auto leadInst = std::dynamic_pointer_cast<CInstrument2A03>(pManager->GetInstrument(index));
	leadInst->SetSeqEnable(sequence_t::Volume, true);
	leadInst->SetSeqIndex(sequence_t::Volume, index);
	leadInst->SetName(name);

	CSequence &leadEnv = *leadInst->GetSequence(sequence_t::Volume);
	leadEnv.SetItemCount(1);
	leadEnv.SetItem(0, vol);
	leadEnv.SetLoopPoint(-1);
	leadEnv.SetReleasePoint(-1);
}

void Kraid::makePattern(CSongData &song, chan_id_t ch, unsigned pat, std::string_view mml) {
	const uint8_t INST = ch == chan_id_t::SQUARE2 ? 0 : 2;
	uint8_t octave = 3;
	int row = 0;
	auto &pattern = song.GetPattern(ch, pat);

	for (auto c : mml) {
		auto &note = pattern.GetNoteOn(row);
		switch (c) {
		case '<': --octave; break;
		case '>': ++octave; break;
		case '.': ++row; break;
		case '-': ++row; note.Note = note_t::HALT; break;
		case '=': ++row; note.Note = note_t::RELEASE; break;
		case 'c': ++row; note.Note = note_t::C;  note.Octave = octave, note.Instrument = INST; break;
		case 'C': ++row; note.Note = note_t::Cs; note.Octave = octave, note.Instrument = INST; break;
		case 'd': ++row; note.Note = note_t::D;  note.Octave = octave, note.Instrument = INST; break;
		case 'D': ++row; note.Note = note_t::Ds; note.Octave = octave, note.Instrument = INST; break;
		case 'e': ++row; note.Note = note_t::E;  note.Octave = octave, note.Instrument = INST; break;
		case 'f': ++row; note.Note = note_t::F;  note.Octave = octave, note.Instrument = INST; break;
		case 'F': ++row; note.Note = note_t::Fs; note.Octave = octave, note.Instrument = INST; break;
		case 'g': ++row; note.Note = note_t::G;  note.Octave = octave, note.Instrument = INST; break;
		case 'G': ++row; note.Note = note_t::Gs; note.Octave = octave, note.Instrument = INST; break;
		case 'a': ++row; note.Note = note_t::A;  note.Octave = octave, note.Instrument = INST; break;
		case 'A': ++row; note.Note = note_t::As; note.Octave = octave, note.Instrument = INST; break;
		case 'b': ++row; note.Note = note_t::B;  note.Octave = octave, note.Instrument = INST; break;
		case '@': note.EffNumber[0] = effect_t::DUTY_CYCLE; note.EffParam[0] = 2; break;
		}
	}
}
