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

#include "WaveRendererFactory.h"
#include "WaveRenderer.h"
#include "FamiTrackerModule.h"
#include "SongLengthScanner.h"
#include "SongView.h"

std::unique_ptr<CWaveRenderer> CWaveRendererFactory::Make(const CFamiTrackerModule &modfile, unsigned track, render_type_t renderType, unsigned param) {
	switch (renderType) {
	case render_type_t::Loops: {
		auto pSongView = modfile.MakeSongView(track);
		CSongLengthScanner scanner {modfile, *pSongView};
		auto [FirstLoop, SecondLoop] = scanner.GetRowCount();
		return std::make_unique<CWaveRendererRow>(FirstLoop + SecondLoop * (param - 1));
	}
	case render_type_t::Seconds:
		return std::make_unique<CWaveRendererTick>(param * modfile.GetFrameRate(), modfile.GetFrameRate());
	}
	return nullptr;
}
