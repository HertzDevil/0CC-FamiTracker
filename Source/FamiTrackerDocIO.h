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
#include <memory>
#include "OldSequence.h"
#include "ModuleException.h"

class CFamiTrackerModule;
class CDocumentFile;
class CBinaryWriter;
class CDocumentInputBlock;
class CDocumentOutputBlock;

class CFamiTrackerDocReader {
public:
	CFamiTrackerDocReader(CDocumentFile &file, module_error_level_t err_lv);

	std::unique_ptr<CFamiTrackerModule> Load();

private:
	void PostLoad(CFamiTrackerModule &modfile);

	void LoadParams(CFamiTrackerModule &modfile, CDocumentInputBlock &block);
	void LoadSongInfo(CFamiTrackerModule &modfile, CDocumentInputBlock &block);
	void LoadHeader(CFamiTrackerModule &modfile, CDocumentInputBlock &block);
	void LoadInstruments(CFamiTrackerModule &modfile, CDocumentInputBlock &block);
	void LoadSequences(CFamiTrackerModule &modfile, CDocumentInputBlock &block);
	void LoadFrames(CFamiTrackerModule &modfile, CDocumentInputBlock &block);
	void LoadPatterns(CFamiTrackerModule &modfile, CDocumentInputBlock &block);
	void LoadDSamples(CFamiTrackerModule &modfile, CDocumentInputBlock &block);
	void LoadComments(CFamiTrackerModule &modfile, CDocumentInputBlock &block);
	void LoadSequencesVRC6(CFamiTrackerModule &modfile, CDocumentInputBlock &block);
	void LoadSequencesN163(CFamiTrackerModule &modfile, CDocumentInputBlock &block);
	void LoadSequencesS5B(CFamiTrackerModule &modfile, CDocumentInputBlock &block);
	void LoadSequencesSN7(CFamiTrackerModule &modfile, CDocumentInputBlock &block);
	void LoadParamsExtra(CFamiTrackerModule &modfile, CDocumentInputBlock &block);
	void LoadDetuneTables(CFamiTrackerModule &modfile, CDocumentInputBlock &block);
	void LoadGrooves(CFamiTrackerModule &modfile, CDocumentInputBlock &block);
	void LoadBookmarks(CFamiTrackerModule &modfile, CDocumentInputBlock &block);

private:
	CDocumentFile &file_;
	module_error_level_t err_lv_;

	std::vector<COldSequence> m_vTmpSequences;		// // //
	bool fds_adjust_arps_ = false;
};



class CFamiTrackerDocWriter {
public:
	CFamiTrackerDocWriter(CBinaryWriter &file, module_error_level_t err_lv);

	bool Save(const CFamiTrackerModule &modfile);

private:
	void SaveParams(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block);
	void SaveSongInfo(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block);
	void SaveHeader(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block);
	void SaveInstruments(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block);
	void SaveSequences(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block);
	void SaveFrames(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block);
	void SavePatterns(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block);
	void SaveDSamples(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block);
	void SaveComments(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block);
	void SaveSequencesVRC6(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block);
	void SaveSequencesN163(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block);
	void SaveSequencesS5B(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block);
	void SaveSequencesSN7(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block);
	void SaveParamsExtra(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block);
	void SaveDetuneTables(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block);
	void SaveGrooves(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block);
	void SaveBookmarks(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block);

private:
	CBinaryWriter &file_;
	module_error_level_t err_lv_;
};
