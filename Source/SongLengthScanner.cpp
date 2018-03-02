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

#include "SongLengthScanner.h"
#include "FamiTrackerModule.h"
#include "SongView.h"
#include "SongData.h"
#include "ft0cc/doc/groove.hpp"
#include "ChannelOrder.h"
#include "FamiTrackerTypes.h"
#include <bitset>
#include <type_traits>
#include <algorithm>



class loop_visitor {
public:
	explicit loop_visitor(const CConstSongView &view) :
		song_view_(view.GetChannelOrder().Canonicalize(), view.GetSong()) { }

	template <typename F, typename G>
	void Visit(F cb, G fx) {
		unsigned FrameCount = song_view_.GetSong().GetFrameCount();
		unsigned Rows = song_view_.GetSong().GetPatternLength();

		std::bitset<MAX_PATTERN_LENGTH> RowVisited[MAX_FRAMES] = { };		// // //
		while (!RowVisited[f_][r_]) {
			RowVisited[f_][r_] = true;

			int Bxx = -1;
			int Dxx = -1;
			bool Cxx = false;

			song_view_.ForeachChannel([&] (std::size_t index) {
				const auto &Note = song_view_.GetPatternOnFrame(index, f_).GetNoteOn(r_);		// // //
				for (int l = 0, m = song_view_.GetEffectColumnCount(index); l < m; ++l) {
					switch (Note.EffNumber[l]) {
					case effect_t::JUMP:
						Bxx = Note.EffParam[l];
						break;
					case effect_t::SKIP:
						Dxx = Note.EffParam[l];
						break;
					case effect_t::HALT:
						Cxx = true;
						break;
					default:
						fx(/* index, */ Note.EffNumber[l], Note.EffParam[l]);
					}
				}
			});

			if (Cxx && !first_)
				break;
			cb();
			if (Cxx)
				break;

			if (Bxx != -1) {
				f_ = std::min(static_cast<unsigned int>(Bxx), FrameCount - 1);
				r_ = 0;
			}
			else if (Dxx != -1) {
				if (++f_ >= FrameCount)
					f_ = 0;
				r_ = std::min(static_cast<unsigned int>(Dxx), Rows - 1);
			}
			else if (++r_ >= Rows) {		// // //
				r_ = 0;
				if (++f_ >= FrameCount)
					f_ = 0;
			}
		}

		first_ = false;
	}

private:
	CConstSongView song_view_;
	unsigned f_ = 0;
	unsigned r_ = 0;
	bool first_ = true;
};



CSongLengthScanner::CSongLengthScanner(const CFamiTrackerModule &modfile, const CConstSongView &view) :
	modfile_(modfile), song_view_(view)
{
}

std::pair<unsigned, unsigned> CSongLengthScanner::GetRowCount() {
	Compute();
	return {rows1_, rows2_};
}

std::pair<double, double> CSongLengthScanner::GetSecondsCount() {
	Compute();
	return {sec1_, sec2_};
}

void CSongLengthScanner::Compute() {
	if (std::exchange(scanned_, true))
		return;

	const auto &song = song_view_.GetSong();

	double Tempo = song.GetSongTempo();
	if (!Tempo)
		Tempo = 2.5 * modfile_.GetFrameRate();
	const bool AllowTempo = song.GetSongTempo() != 0;
	const int Split = modfile_.GetSpeedSplitPoint();
	int Speed = song.GetSongSpeed();
	int GrooveIndex = song.GetSongGroove() ? Speed : -1;
	int GroovePointer = 0;

	if (GrooveIndex != -1 && !modfile_.HasGroove(Speed)) {
		GrooveIndex = -1;
		Speed = DEFAULT_SPEED;
	}

	const auto fxhandler = [&] (effect_t fx, uint8_t param) {
		switch (fx) {
		case effect_t::SPEED:
			if (AllowTempo && param >= Split)
				Tempo = param;
			else {
				GrooveIndex = -1;
				Speed = param;
			}
			break;
		case effect_t::GROOVE:
			if (modfile_.HasGroove(param)) {
				GrooveIndex = param;
				GroovePointer = 0;
			}
			break;
		}
	};

	loop_visitor visitor(song_view_);
	visitor.Visit([&] {
		if (auto pGroove = modfile_.GetGroove(GrooveIndex))
			Speed = pGroove->entry(GroovePointer++);
		sec1_ += Speed / Tempo;
		++rows1_;
	}, fxhandler);
	visitor.Visit([&] {
		if (auto pGroove = modfile_.GetGroove(GrooveIndex))
			Speed = pGroove->entry(GroovePointer++);
		sec2_ += Speed / Tempo;
		++rows2_;
	}, fxhandler);

	rows1_ -= rows2_;
	sec1_ -= sec2_;
	sec1_ *= 2.5;
	sec2_ *= 2.5;
}
