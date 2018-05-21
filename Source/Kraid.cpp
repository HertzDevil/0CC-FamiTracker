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
	(void)modfile.ReplaceSong(0, makeSong(modfile));
}

void Kraid::buildDoc(CFamiTrackerModule &modfile) {
	// Instruments and sequences
	makeInst(modfile, 0, 6, "Lead ");
	makeInst(modfile, 1, 2, "Echo");
	makeInst(modfile, 2, 15, "Triangle");
}

std::unique_ptr<CSongData> Kraid::makeSong(CFamiTrackerModule &modfile) {
	auto pSong = modfile.MakeNewSong();

	const unsigned FRAMES = 14;
	const unsigned ROWS = 24;
	const unsigned PATTERNS[][FRAMES] = {
		{0, 0, 0, 0, 1, 1, 2, 3, 3, 3, 4, 5, 6, 6},
		{0, 0, 0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4},
		{0, 0, 0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	};

	pSong->SetFrameCount(FRAMES);
	pSong->SetPatternLength(ROWS);
	pSong->SetSongSpeed(8);
	pSong->SetEffectColumnCount(apu_subindex_t::pulse1, 2);

	for (std::size_t subindex = 0; subindex < std::size(PATTERNS); ++subindex)
		for (int f = 0; f < FRAMES; ++f)
			pSong->SetFramePattern(f, {sound_chip_t::APU, static_cast<std::uint8_t>(subindex)}, PATTERNS[subindex][f]);

	makePattern(*pSong, apu_subindex_t::triangle, 0, "<e.>e...<e.>e...<e.>e...<e.>e...");
	makePattern(*pSong, apu_subindex_t::triangle, 1, "<c.>c...<c.>c...<d.>d...<d.>d...");
	makePattern(*pSong, apu_subindex_t::triangle, 2, "<e.>e.>e.<<F.>F.>F.<<f.>f.>f.<<<b.>b.>b.");
	makePattern(*pSong, apu_subindex_t::triangle, 3, "<e...b.>c...<b.c...g.a...b.");
	makePattern(*pSong, apu_subindex_t::triangle, 4, "<<e");

	makePattern(*pSong, apu_subindex_t::pulse2, 0, "@e...<b.>a... c. F...d.<b...A.");
	makePattern(*pSong, apu_subindex_t::pulse2, 1, "@g... d. e...<b.>F...d. a...e.");
	makePattern(*pSong, apu_subindex_t::pulse2, 2, "@g<b>g<b>g<b>AeAeAeacacacaDFDbD");
	makePattern(*pSong, apu_subindex_t::pulse2, 3, "Fgab>d<b>Fd<agFb>aFd<agFega>de-");
	makePattern(*pSong, apu_subindex_t::pulse2, 4, ">a-g-F-e-F-g-a-g-F-e-F-g-");

	int f = 0;
	int r = 0;
	do { // TODO: use CSongIterator
		auto note = pSong->GetPatternOnFrame(apu_subindex_t::pulse2, f).GetNoteOn(r);
		if (++r >= ROWS) {
			r = 0;
			if (++f >= FRAMES)
				f = 0;
		}
		if (note != ft0cc::doc::pattern_note { }) {
			note.set_inst(1);
			note.set_fx_cmd(1, {ft0cc::doc::effect_type::DELAY, 3u});
			pSong->GetPatternOnFrame(apu_subindex_t::pulse1, f).SetNoteOn(r, note);
		}
	} while (f || r);

	return pSong;
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

void Kraid::makePattern(CSongData &song, stChannelID ch, unsigned pat, std::string_view mml) {
	const uint8_t INST = ch.Chip == sound_chip_t::APU && ch.Subindex == value_cast(apu_subindex_t::pulse2) ? 0 : 2;
	uint8_t octave = 3;
	int row = 0;
	auto &pattern = song.GetPattern(ch, pat);

	for (auto c : mml) {
		auto &note = pattern.GetNoteOn(row);
		switch (c) {
		case '<': --octave; break;
		case '>': ++octave; break;
		case '.': ++row; break;
		case '-': ++row; note.set_note(ft0cc::doc::pitch::halt); break;
		case '=': ++row; note.set_note(ft0cc::doc::pitch::release); break;
		case 'c': ++row; note.set_note(ft0cc::doc::pitch::C ); note.set_oct(octave); note.set_inst(INST); break;
		case 'C': ++row; note.set_note(ft0cc::doc::pitch::Cs); note.set_oct(octave); note.set_inst(INST); break;
		case 'd': ++row; note.set_note(ft0cc::doc::pitch::D ); note.set_oct(octave); note.set_inst(INST); break;
		case 'D': ++row; note.set_note(ft0cc::doc::pitch::Ds); note.set_oct(octave); note.set_inst(INST); break;
		case 'e': ++row; note.set_note(ft0cc::doc::pitch::E ); note.set_oct(octave); note.set_inst(INST); break;
		case 'f': ++row; note.set_note(ft0cc::doc::pitch::F ); note.set_oct(octave); note.set_inst(INST); break;
		case 'F': ++row; note.set_note(ft0cc::doc::pitch::Fs); note.set_oct(octave); note.set_inst(INST); break;
		case 'g': ++row; note.set_note(ft0cc::doc::pitch::G ); note.set_oct(octave); note.set_inst(INST); break;
		case 'G': ++row; note.set_note(ft0cc::doc::pitch::Gs); note.set_oct(octave); note.set_inst(INST); break;
		case 'a': ++row; note.set_note(ft0cc::doc::pitch::A ); note.set_oct(octave); note.set_inst(INST); break;
		case 'A': ++row; note.set_note(ft0cc::doc::pitch::As); note.set_oct(octave); note.set_inst(INST); break;
		case 'b': ++row; note.set_note(ft0cc::doc::pitch::B ); note.set_oct(octave); note.set_inst(INST); break;
		case '@': note.set_fx_cmd(0, {ft0cc::doc::effect_type::DUTY_CYCLE, 2u}); break;
		}
	}
}
