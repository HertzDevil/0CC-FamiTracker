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

CModuleImporter::CModuleImporter(CFamiTrackerModule &modfile, CFamiTrackerModule &imported) :
	modfile_(modfile), imported_(imported)
{
	for (unsigned i = 0; i < std::size(inst_index_); ++i)
		inst_index_[i] = i;
	for (unsigned i = 0; i < std::size(groove_index_); ++i)
		groove_index_[i] = i;
}

bool CModuleImporter::Validate() const {
	if (modfile_.GetSongCount() + imported_.GetSongCount() > MAX_TRACKS) {
		AfxMessageBox(IDS_IMPORT_FAILED, MB_ICONEXCLAMATION);
		return false;
	}

	if (modfile_.GetGrooveCount() + imported_.GetGrooveCount() > MAX_GROOVE) {
		AfxMessageBox(IDS_IMPORT_GROOVE_SLOTS, MB_ICONEXCLAMATION);
		return false;
	}

	const inst_type_t INST[] = {INST_2A03, INST_VRC6, INST_N163, INST_S5B};		// // //

	auto *pManager = modfile_.GetInstrumentManager();
	auto *pDSampleManager = modfile_.GetDSampleManager();
	auto *pImportedInst = imported_.GetInstrumentManager();
	auto *pImportedSamps = imported_.GetDSampleManager();		// // //

	// Check instrument count
	if (pManager->GetInstrumentCount() + pImportedInst->GetInstrumentCount() > MAX_INSTRUMENTS) {
		AfxMessageBox(IDS_IMPORT_INSTRUMENT_COUNT, MB_ICONERROR);
		return false;
	}

	// // // Check DPCM sample count
	if (pDSampleManager->GetDSampleCount() + pImportedSamps->GetDSampleCount() > MAX_DSAMPLES) {
		AfxMessageBox(IDS_IMPORT_SAMPLE_SLOTS, MB_ICONEXCLAMATION);
		return false;
	}

	// Check sequence count
	bool bOutOfSeq = false;
	for (inst_type_t i : INST)
		foreachSeq([&] (sequence_t t) {
			if (pManager->GetSequenceCount(i, t) + pImportedInst->GetSequenceCount(i, t) > MAX_SEQUENCES)		// // //
				bOutOfSeq = true;
		});
	if (bOutOfSeq) {
		AfxMessageBox(IDS_IMPORT_SEQUENCE_COUNT, MB_ICONERROR);
		return false;
	}

	return true;
}

void CModuleImporter::DoImport(bool doInst, bool doGroove, bool doDetune) {
	if (doInst)
		ImportInstruments();
	if (doGroove)
		ImportGrooves();
	if (doDetune)
		ImportDetune();
	ImportSongs();
}

void CModuleImporter::ImportInstruments() {
	const inst_type_t INST[] = {INST_2A03, INST_VRC6, INST_N163, INST_S5B};		// // //

	auto *pManager = modfile_.GetInstrumentManager();
	auto *pDSampleManager = modfile_.GetDSampleManager();
	auto *pImportedInst = imported_.GetInstrumentManager();
	auto *pImportedSamps = imported_.GetDSampleManager();		// // //

	int seqTable[std::size(INST)][MAX_SEQUENCES][SEQ_COUNT] = { };
	int SamplesTable[MAX_DSAMPLES] = { };

	for (size_t i = 0; i < std::size(INST); i++) foreachSeq([&] (sequence_t t) {
		for (unsigned int s = 0; s < MAX_SEQUENCES; ++s)
			if (auto pImportSeq = pImportedInst->GetSequence(INST[i], t, s); pImportSeq && pImportSeq->GetItemCount() > 0) {
				for (unsigned j = 0; j < MAX_SEQUENCES; ++j) {
					auto pSeq = pManager->GetSequence(INST[i], t, j);
					if (pSeq && pSeq->GetItemCount() > 0)
						continue;
					// TODO: continue if blank sequence is used by some instrument
					*pSeq = *pImportSeq;		// // //
					// Save a reference to this sequence
					seqTable[i][s][value_cast(t)] = j;
					break;
				}
			}
	});

	// Copy DPCM samples
	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (auto pImportDSample = pImportedSamps->ReleaseDSample(i)) {		// // //
			// Save a reference to this DPCM sample
			int Index = pDSampleManager->GetFirstFree();
			pDSampleManager->SetDSample(Index, pImportDSample);
			SamplesTable[i] = Index;
		}
	}

	// Copy instruments
	pImportedInst->VisitInstruments([&] (const CInstrument &inst, std::size_t i) {
		auto pInst = inst.Clone();		// // //

		// Update references
		if (auto pSeq = dynamic_cast<CSeqInstrument *>(pInst.get())) {
			foreachSeq([&] (sequence_t t) {
				if (pSeq->GetSeqEnable(t)) {
					for (size_t j = 0; j < std::size(INST); j++)
						if (INST[j] == pInst->GetType()) {
							pSeq->SetSeqIndex(t, seqTable[j][pSeq->GetSeqIndex(t)][value_cast(t)]);
							break;
						}
				}
			});
			// Update DPCM samples
			if (auto p2A03 = dynamic_cast<CInstrument2A03 *>(pSeq))
				for (int n = 0; n < NOTE_COUNT; ++n)
					if (int Sample = p2A03->GetSampleIndex(n); Sample != 0)
						p2A03->SetSampleIndex(n, SamplesTable[Sample - 1] + 1);
		}

		int Index = pManager->GetFirstUnused();
		pManager->InsertInstrument(Index, std::move(pInst));		// // //
		// Save a reference to this instrument
		inst_index_[i] = Index;
	});
}

void CModuleImporter::ImportGrooves() {
	int Index = 0;
	for (int i = 0; i < MAX_GROOVE; ++i) {
		if (auto pGroove = imported_.GetGroove(i)) {
			while (modfile_.HasGroove(Index))
				++Index;
			groove_index_[i] = Index;
			modfile_.SetGroove(Index, std::move(pGroove));
		}
	}
}

void CModuleImporter::ImportDetune() {
	for (int i = 0; i < 6; i++) for (int j = 0; j < NOTE_COUNT; j++)
		modfile_.SetDetuneOffset(i, j, imported_.GetDetuneOffset(i, j));
}

void CModuleImporter::ImportSongs() {
	imported_.VisitSongs([&] (const CSongData &, unsigned i) {
		auto pSong = imported_.ReleaseSong(i);		// // //
		auto &song = *pSong;
		modfile_.InsertSong(modfile_.GetSongCount(), std::move(pSong));

		// // // translate instruments and grooves outside modules
		if (song.GetSongGroove())
			song.SetSongSpeed(groove_index_[song.GetSongSpeed()]);
		song.VisitPatterns([this] (CPatternData &pat) {
			pat.VisitRows([this] (stChanNote &note) {
				// Translate instrument number
				if (note.Instrument < MAX_INSTRUMENTS)
					note.Instrument = inst_index_[note.Instrument];
				// // // Translate groove commands
				for (int i = 0; i < MAX_EFFECT_COLUMNS; ++i)
					if (note.EffNumber[i] == effect_t::GROOVE && note.EffParam[i] < MAX_GROOVE)
						note.EffParam[i] = groove_index_[note.EffParam[i]];
			});
		});
	});}
