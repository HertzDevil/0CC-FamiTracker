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

#include "ModuleImporter.h"
#include "FamiTrackerModule.h"
#include "stdafx.h" // error messages
#include "../resource.h" // error string IDs
#include "Instrument2A03.h"
#include "InstrumentManager.h"
#include "DSampleManager.h"
#include "Sequence.h"
#include "SongData.h"
#include "ft0cc/doc/groove.hpp"
#include "ft0cc/doc/pattern_note.hpp"

CModuleImporter::CModuleImporter(CFamiTrackerModule &modfile, CFamiTrackerModule &imported,
	import_mode_t instMode, import_mode_t grooveMode) :
	modfile_(modfile), imported_(imported), inst_mode_(instMode), groove_mode_(grooveMode)
{
	auto *pInst = modfile_.GetInstrumentManager();
	auto *pImportedInst = imported_.GetInstrumentManager();
	unsigned ii = 0;
	while (pInst->HasInstrument(ii))
		++ii;
	for (unsigned i = 0; i < MAX_INSTRUMENTS; ++i)
		if (pImportedInst->HasInstrument(i))
			switch (inst_mode_) {
			case import_mode_t::duplicate_all:
				inst_index_.try_emplace(i, ii);
				while (pInst->HasInstrument(++ii))
					;
				break;
			case import_mode_t::overwrite_all:
				inst_index_.try_emplace(i, i);
				break;
			case import_mode_t::import_missing:
				if (!pInst->HasInstrument(i))
					inst_index_.try_emplace(i, i);
				break;
			case import_mode_t::none: default:
				break;
			}

	unsigned gi = 0;
	while (modfile_.HasGroove(gi))
		++gi;
	for (unsigned i = 0; i < MAX_GROOVE; ++i)
		if (imported_.HasGroove(i))
			switch (groove_mode_) {
			case import_mode_t::duplicate_all:
				groove_index_.try_emplace(i, gi);
				while (modfile_.HasGroove(++gi))
					;
				break;
			case import_mode_t::overwrite_all:
				groove_index_.try_emplace(i, i);
				break;
			case import_mode_t::import_missing:
				if (!modfile_.HasGroove(i))
					groove_index_.try_emplace(i, i);
				break;
			case import_mode_t::none: default:
				break;
			}
}

bool CModuleImporter::Validate() const {
	if (modfile_.GetSongCount() + imported_.GetSongCount() > MAX_TRACKS) {
		AfxMessageBox(IDS_IMPORT_FAILED, MB_ICONEXCLAMATION);
		return false;
	}

	for (const auto &it : inst_index_)
		if (it.second >= MAX_INSTRUMENTS) {
			AfxMessageBox(IDS_IMPORT_INSTRUMENT_COUNT, MB_ICONERROR);
			return false;
		}

	for (const auto &it : groove_index_)
		if (it.second >= MAX_GROOVE) {
			AfxMessageBox(IDS_IMPORT_GROOVE_SLOTS, MB_ICONEXCLAMATION);
			return false;
		}

	if (inst_mode_ != import_mode_t::none) {
		// // // Check DPCM sample count
		// TODO: allow importing only used samples
		auto *pSamps = modfile_.GetDSampleManager();
		auto *pImportedSamps = imported_.GetDSampleManager();		// // //
		if (pSamps->GetDSampleCount() + pImportedSamps->GetDSampleCount() > MAX_DSAMPLES) {
			AfxMessageBox(IDS_IMPORT_SAMPLE_SLOTS, MB_ICONEXCLAMATION);
			return false;
		}

		const inst_type_t INST[] = {INST_2A03, INST_VRC6, INST_N163, INST_S5B};		// // //

		// Check sequence count
		// TODO: allow importing only used sequences
		auto *pInsts = modfile_.GetInstrumentManager();
		auto *pImportedInsts = imported_.GetInstrumentManager();
		for (inst_type_t i : INST)
			for (auto t : enum_values<sequence_t>()) {
				int seqCount = pInsts->GetSequenceCount(i, t);
				seqCount += pImportedInsts->GetSequenceCount(i, t);
				if (seqCount > MAX_SEQUENCES) {		// // //
					AfxMessageBox(IDS_IMPORT_SEQUENCE_COUNT, MB_ICONERROR);
					return false;
				}
			}
	}

	return true;
}

void CModuleImporter::DoImport(bool doDetune) {
	ImportInstruments();
	ImportGrooves();
	if (doDetune)
		ImportDetune();
	ImportSongs();
}

void CModuleImporter::ImportInstruments() {
	if (inst_mode_ == import_mode_t::none)
		return;

	const inst_type_t INST[] = {INST_2A03, INST_VRC6, INST_N163, INST_S5B};		// // //

	auto *pInsts = modfile_.GetInstrumentManager();
	auto *pSamps = modfile_.GetDSampleManager();
	auto *pImportedInsts = imported_.GetInstrumentManager();
	auto *pImportedSamps = imported_.GetDSampleManager();		// // //

	int seqTable[std::size(INST)][MAX_SEQUENCES][SEQ_COUNT] = { };
	int SamplesTable[MAX_DSAMPLES] = { };

	for (size_t i = 0; i < std::size(INST); ++i) for (auto t : enum_values<sequence_t>()) {
		for (unsigned int s = 0; s < MAX_SEQUENCES; ++s)
			if (auto pImportSeq = pImportedInsts->GetSequence(INST[i], t, s); pImportSeq && pImportSeq->GetItemCount() > 0) {
				for (unsigned j = 0; j < MAX_SEQUENCES; ++j) {
					auto pSeq = pInsts->GetSequence(INST[i], t, j);
					if (pSeq && pSeq->GetItemCount() > 0)
						continue;
					// TODO: continue if blank sequence is used by some instrument
					*pSeq = *pImportSeq;		// // //
					// Save a reference to this sequence
					seqTable[i][s][value_cast(t)] = j;
					break;
				}
			}
	}

	// Copy DPCM samples
	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (auto pImportDSample = pImportedSamps->ReleaseDSample(i)) {		// // //
			// Save a reference to this DPCM sample
			int Index = pSamps->GetFirstFree();
			pSamps->SetDSample(Index, std::move(pImportDSample));
			SamplesTable[i] = Index;
		}
	}

	// Copy instruments
	pImportedInsts->VisitInstruments([&] (const CInstrument &inst, std::size_t i) {
		if (auto it = inst_index_.find(i); it != inst_index_.end()) {
			auto pNewInst = inst.Clone();		// // //

			// Update references
			if (auto *pSeq = dynamic_cast<CSeqInstrument *>(pNewInst.get())) {
				for (auto t : enum_values<sequence_t>())
					if (pSeq->GetSeqEnable(t)) {
						for (size_t j = 0; j < std::size(INST); ++j)
							if (INST[j] == pNewInst->GetType()) {
								pSeq->SetSeqIndex(t, seqTable[j][pSeq->GetSeqIndex(t)][value_cast(t)]);
								break;
							}
					}
				// Update DPCM samples
				if (auto *p2A03 = dynamic_cast<CInstrument2A03 *>(pSeq))
					for (int n = 0; n < NOTE_COUNT; ++n)
						if (unsigned Sample = p2A03->GetSampleIndex(n); Sample != CInstrument2A03::NO_DPCM)
							p2A03->SetSampleIndex(n, SamplesTable[Sample]);
			}

			pInsts->InsertInstrument(it->second, std::move(pNewInst));		// // //
		}
	});
}

void CModuleImporter::ImportGrooves() {
	if (groove_mode_ != import_mode_t::none)
		for (auto [i, gi] : groove_index_)
			modfile_.SetGroove(gi, imported_.GetGroove(i));
}

void CModuleImporter::ImportDetune() {
	for (int i = 0; i < 6; ++i)
		for (int j = 0; j < NOTE_COUNT; ++j)
			modfile_.SetDetuneOffset(i, j, imported_.GetDetuneOffset(i, j));
}

void CModuleImporter::ImportSongs() {
	imported_.VisitSongs([&] (const CSongData &, unsigned i) {
		auto pSong = imported_.ReleaseSong(i);		// // //
		auto &song = *pSong;
		modfile_.InsertSong(modfile_.GetSongCount(), std::move(pSong));

		// // // translate instruments and grooves outside modules
		if (song.GetSongGroove())
			if (auto it = groove_index_.find(song.GetSongSpeed()); it != groove_index_.end())
				song.SetSongSpeed(it->second);
		song.VisitPatterns([this] (CPatternData &pat) {
			for (ft0cc::doc::pattern_note &note : pat.Rows()) {
				// Translate instrument number
				if (note.inst() < MAX_INSTRUMENTS)
					if (auto it = inst_index_.find(note.inst()); it != inst_index_.end())
						note.set_inst(it->second);
				// // // Translate groove commands
				for (auto [fx, param] : note.fx_cmds())
					if (fx == ft0cc::doc::effect_type::GROOVE && param < MAX_GROOVE)
						if (auto it = groove_index_.find(param); it != groove_index_.end())
							param = it->second;
			}
		});
	});
}
