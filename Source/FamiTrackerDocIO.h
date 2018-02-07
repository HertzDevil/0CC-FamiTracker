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

#include <string>
#include <vector>
#include "OldSequence.h"

class CFamiTrackerModule;
class CDocumentFile;

enum module_error_level_t : int; // Settings.h

class CFamiTrackerDocIO {
public:
	explicit CFamiTrackerDocIO(CDocumentFile &file);

	bool Load(CFamiTrackerModule &modfile);
	bool Save(const CFamiTrackerModule &modfile);

private:
	void PostLoad(CFamiTrackerModule &modfile);

	void LoadParams(CFamiTrackerModule &modfile, int ver);
	void SaveParams(const CFamiTrackerModule &modfile, int ver);

	void LoadSongInfo(CFamiTrackerModule &modfile, int ver);
	void SaveSongInfo(const CFamiTrackerModule &modfile, int ver);

	void LoadHeader(CFamiTrackerModule &modfile, int ver);
	void SaveHeader(const CFamiTrackerModule &modfile, int ver);

	void LoadInstruments(CFamiTrackerModule &modfile, int ver);
	void SaveInstruments(const CFamiTrackerModule &modfile, int ver);

	void LoadSequences(CFamiTrackerModule &modfile, int ver);
	void SaveSequences(const CFamiTrackerModule &modfile, int ver);

	void LoadFrames(CFamiTrackerModule &modfile, int ver);
	void SaveFrames(const CFamiTrackerModule &modfile, int ver);

	void LoadPatterns(CFamiTrackerModule &modfile, int ver);
	void SavePatterns(const CFamiTrackerModule &modfile, int ver);

	void LoadDSamples(CFamiTrackerModule &modfile, int ver);
	void SaveDSamples(const CFamiTrackerModule &modfile, int ver);

	void LoadComments(CFamiTrackerModule &modfile, int ver);
	void SaveComments(const CFamiTrackerModule &modfile, int ver);

	void LoadSequencesVRC6(CFamiTrackerModule &modfile, int ver);
	void SaveSequencesVRC6(const CFamiTrackerModule &modfile, int ver);

	void LoadSequencesN163(CFamiTrackerModule &modfile, int ver);
	void SaveSequencesN163(const CFamiTrackerModule &modfile, int ver);

	void LoadSequencesS5B(CFamiTrackerModule &modfile, int ver);
	void SaveSequencesS5B(const CFamiTrackerModule &modfile, int ver);

	void LoadParamsExtra(CFamiTrackerModule &modfile, int ver);
	void SaveParamsExtra(const CFamiTrackerModule &modfile, int ver);

	void LoadDetuneTables(CFamiTrackerModule &modfile, int ver);
	void SaveDetuneTables(const CFamiTrackerModule &modfile, int ver);

	void LoadGrooves(CFamiTrackerModule &modfile, int ver);
	void SaveGrooves(const CFamiTrackerModule &modfile, int ver);

	void LoadBookmarks(CFamiTrackerModule &modfile, int ver);
	void SaveBookmarks(const CFamiTrackerModule &modfile, int ver);

private:
	template <module_error_level_t l = MODULE_ERROR_DEFAULT>
	void AssertFileData(bool Cond, const std::string &Msg) const;		// // //

	template <module_error_level_t l = MODULE_ERROR_DEFAULT, typename T, typename U, typename V>
	T AssertRange(T Value, U Min, V Max, const std::string &Desc) const;

	CDocumentFile &file_;

	std::vector<COldSequence> m_vTmpSequences;		// // //
	bool fds_adjust_arps_ = false;
};
