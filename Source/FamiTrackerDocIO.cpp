/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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

#include "FamiTrackerDocIO.h"
#include "FamiTrackerDocIOCommon.h"
#include "FamiTrackerDocOldIO.h"
#include "DocumentFile.h" // stdafx.h
#include "ModuleException.h"
#include "FamiTrackerDoc.h"
#include "Settings.h"

#include "InstrumentManager.h"
#include "Instrument2A03.h"
#include "InstrumentVRC6.h" // error message
#include "InstrumentN163.h" // error message
#include "InstrumentS5B.h" // error message

#include "SequenceManager.h"
#include "SequenceCollection.h"

#include "SongData.h"
#include "PatternNote.h"

#include "DSampleManager.h"
#include "DSample.h"

#include "ft0cc/doc/groove.hpp"

#include "BookmarkCollection.h"

namespace {

const char *FILE_BLOCK_PARAMS			= "PARAMS";
const char *FILE_BLOCK_INFO				= "INFO";
const char *FILE_BLOCK_INSTRUMENTS		= "INSTRUMENTS";
const char *FILE_BLOCK_SEQUENCES		= "SEQUENCES";
const char *FILE_BLOCK_FRAMES			= "FRAMES";
const char *FILE_BLOCK_PATTERNS			= "PATTERNS";
const char *FILE_BLOCK_DSAMPLES			= "DPCM SAMPLES";
const char *FILE_BLOCK_HEADER			= "HEADER";
const char *FILE_BLOCK_COMMENTS			= "COMMENTS";

// VRC6
const char *FILE_BLOCK_SEQUENCES_VRC6	= "SEQUENCES_VRC6";

// N163
const char *FILE_BLOCK_SEQUENCES_N163	= "SEQUENCES_N163";
const char *FILE_BLOCK_SEQUENCES_N106	= "SEQUENCES_N106";

// Sunsoft
const char *FILE_BLOCK_SEQUENCES_S5B	= "SEQUENCES_S5B";

// // // 0CC-FamiTracker specific
const char *FILE_BLOCK_DETUNETABLES		= "DETUNETABLES";
const char *FILE_BLOCK_GROOVES			= "GROOVES";
const char *FILE_BLOCK_BOOKMARKS		= "BOOKMARKS";
const char *FILE_BLOCK_PARAMS_EXTRA		= "PARAMS_EXTRA";

template <typename F> // (const CSequence &seq, int index, int seqType)
void VisitSequences(const CSequenceManager *manager, F&& f) {
	if (!manager)
		return;
	for (int j = 0, n = manager->GetCount(); j < n; ++j) {
		const auto &col = *manager->GetCollection(j);
		for (int i = 0; i < MAX_SEQUENCES; ++i) {
			if (const CSequence *pSeq = col.GetSequence(i); pSeq && pSeq->GetItemCount())
				f(*pSeq, i, j);
		}
	}
}

} // namespace

// // // save/load functionality

CFamiTrackerDocIO::CFamiTrackerDocIO(CDocumentFile &file) :
	file_(file)
{
}

bool CFamiTrackerDocIO::Load(CFamiTrackerDoc &doc) {
	using map_t = std::unordered_map<std::string, void (CFamiTrackerDocIO::*)(CFamiTrackerDoc &, int)>;
	static const auto FTM_READ_FUNC = map_t {
		{FILE_BLOCK_PARAMS,			&CFamiTrackerDocIO::LoadParams},
		{FILE_BLOCK_INFO,			&CFamiTrackerDocIO::LoadSongInfo},
		{FILE_BLOCK_HEADER,			&CFamiTrackerDocIO::LoadHeader},
		{FILE_BLOCK_INSTRUMENTS,	&CFamiTrackerDocIO::LoadInstruments},
		{FILE_BLOCK_SEQUENCES,		&CFamiTrackerDocIO::LoadSequences},
		{FILE_BLOCK_FRAMES,			&CFamiTrackerDocIO::LoadFrames},
		{FILE_BLOCK_PATTERNS,		&CFamiTrackerDocIO::LoadPatterns},
		{FILE_BLOCK_DSAMPLES,		&CFamiTrackerDocIO::LoadDSamples},
		{FILE_BLOCK_COMMENTS,		&CFamiTrackerDocIO::LoadComments},
		{FILE_BLOCK_SEQUENCES_VRC6,	&CFamiTrackerDocIO::LoadSequencesVRC6},
		{FILE_BLOCK_SEQUENCES_N163,	&CFamiTrackerDocIO::LoadSequencesN163},
		{FILE_BLOCK_SEQUENCES_N106,	&CFamiTrackerDocIO::LoadSequencesN163},	// Backward compatibility
		{FILE_BLOCK_SEQUENCES_S5B,	&CFamiTrackerDocIO::LoadSequencesS5B},	// // //
		{FILE_BLOCK_PARAMS_EXTRA,	&CFamiTrackerDocIO::LoadParamsExtra},	// // //
		{FILE_BLOCK_DETUNETABLES,	&CFamiTrackerDocIO::LoadDetuneTables},	// // //
		{FILE_BLOCK_GROOVES,		&CFamiTrackerDocIO::LoadGrooves},		// // //
		{FILE_BLOCK_BOOKMARKS,		&CFamiTrackerDocIO::LoadBookmarks},		// // //
	};

#ifdef _DEBUG
	int _msgs_ = 0;
#endif

	// This has to be done for older files
	if (file_.GetFileVersion() < 0x0210)
		doc.GetSongData(0);

	// Read all blocks
	bool ErrorFlag = false;
	while (!file_.Finished() && !ErrorFlag) {
		ErrorFlag = file_.ReadBlock();
		const char *BlockID = file_.GetBlockHeaderID();
		if (!strcmp(BlockID, "END"))
			break;

		try {
			(this->*FTM_READ_FUNC.at(BlockID))(doc, file_.GetBlockVersion());		// // //
		}
		catch (std::out_of_range) {
#ifdef _DEBUG
		// This shouldn't show up in release (debug only)
//			if (++_msgs_ < 5)
//				AfxMessageBox(_T("Unknown file block!"));
#endif
			if (file_.IsFileIncomplete())
				ErrorFlag = true;
		}
	}

	if (ErrorFlag) {
		doc.DeleteContents();
		return false;
	}

	PostLoad(doc);
	return true;
}

bool CFamiTrackerDocIO::Save(const CFamiTrackerDoc &doc) {
	using block_info_t = std::tuple<void (CFamiTrackerDocIO::*)(const CFamiTrackerDoc &, int), int, const char *>;
	static const block_info_t MODULE_WRITE_FUNC[] = {		// // //
		{&CFamiTrackerDocIO::SaveParams,		6, FILE_BLOCK_PARAMS},
		{&CFamiTrackerDocIO::SaveSongInfo,		1, FILE_BLOCK_INFO},
		{&CFamiTrackerDocIO::SaveHeader,		3, FILE_BLOCK_HEADER},
		{&CFamiTrackerDocIO::SaveInstruments,	6, FILE_BLOCK_INSTRUMENTS},
		{&CFamiTrackerDocIO::SaveSequences,		6, FILE_BLOCK_SEQUENCES},
		{&CFamiTrackerDocIO::SaveFrames,		3, FILE_BLOCK_FRAMES},
#ifdef TRANSPOSE_FDS
		{&CFamiTrackerDocIO::SavePatterns,		5, FILE_BLOCK_PATTERNS},
#else
		{&CFamiTrackerDocIO::SavePatterns,		4, FILE_BLOCK_PATTERNS},
#endif
		{&CFamiTrackerDocIO::SaveDSamples,		1, FILE_BLOCK_DSAMPLES},
		{&CFamiTrackerDocIO::SaveComments,		1, FILE_BLOCK_COMMENTS},
		{&CFamiTrackerDocIO::SaveSequencesVRC6,	6, FILE_BLOCK_SEQUENCES_VRC6},		// // //
		{&CFamiTrackerDocIO::SaveSequencesN163,	1, FILE_BLOCK_SEQUENCES_N163},
		{&CFamiTrackerDocIO::SaveSequencesS5B,	1, FILE_BLOCK_SEQUENCES_S5B},
		{&CFamiTrackerDocIO::SaveParamsExtra,	2, FILE_BLOCK_PARAMS_EXTRA},		// // //
		{&CFamiTrackerDocIO::SaveDetuneTables,	1, FILE_BLOCK_DETUNETABLES},		// // //
		{&CFamiTrackerDocIO::SaveGrooves,		1, FILE_BLOCK_GROOVES},				// // //
		{&CFamiTrackerDocIO::SaveBookmarks,		1, FILE_BLOCK_BOOKMARKS},			// // //
	};

	file_.BeginDocument();
	for (auto [fn, ver, name] : MODULE_WRITE_FUNC) {
		file_.CreateBlock(name, ver);
		(this->*fn)(doc, ver);
		if (!file_.FlushBlock())
			return false;
	}
	file_.EndDocument();
	return true;
}

void CFamiTrackerDocIO::PostLoad(CFamiTrackerDoc &doc) {
	if (file_.GetFileVersion() <= 0x0201)
		compat::ReorderSequences(doc, std::move(m_vTmpSequences));

	if (fds_adjust_arps_) {
		int Channel = doc.GetChannelIndex(CHANID_FDS);
		if (Channel != -1) {
			for (unsigned int t = 0; t < doc.GetTrackCount(); ++t) for (int p = 0; p < MAX_PATTERN; ++p) for (int r = 0; r < MAX_PATTERN_LENGTH; ++r) {
				stChanNote Note = doc.GetDataAtPattern(t, p, Channel, r);		// // //
				if (Note.Note >= NOTE_C && Note.Note <= NOTE_B) {
					int Trsp = MIDI_NOTE(Note.Octave, Note.Note) + NOTE_RANGE * 2;
					Trsp = Trsp >= NOTE_COUNT ? NOTE_COUNT - 1 : Trsp;
					Note.Note = GET_NOTE(Trsp);
					Note.Octave = GET_OCTAVE(Trsp);
					doc.SetDataAtPattern(t, p, Channel, r, Note);		// // //
				}
			}
		}
		for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
			if (doc.GetInstrumentType(i) == INST_FDS) {
				CSequence *pSeq = std::static_pointer_cast<CSeqInstrument>(doc.GetInstrument(i))->GetSequence(SEQ_ARPEGGIO);
				if (pSeq != nullptr && pSeq->GetItemCount() > 0 && pSeq->GetSetting() == SETTING_ARP_FIXED)
					for (unsigned int j = 0; j < pSeq->GetItemCount(); ++j) {
						int Trsp = pSeq->GetItem(j) + NOTE_RANGE * 2;
						pSeq->SetItem(j, Trsp >= NOTE_COUNT ? NOTE_COUNT - 1 : Trsp);
					}
			}
		}
	}
}

void CFamiTrackerDocIO::LoadParams(CFamiTrackerDoc &doc, int ver) {
	// Get first track for module versions that require that
	auto &Song = doc.GetSongData(0);

	unsigned Expansion = SNDCHIP_NONE;		// // //

	if (ver == 1)
		Song.SetSongSpeed(file_.GetBlockInt());
	else
		Expansion = file_.GetBlockChar();

	int channels = AssertRange(file_.GetBlockInt(), 1, MAX_CHANNELS, "Channel count");		// // //
	AssertRange<MODULE_ERROR_OFFICIAL>(channels, 1, MAX_CHANNELS - 1, "Channel count");
	doc.SetAvailableChannels(channels);		// // //

	auto machine = static_cast<machine_t>(file_.GetBlockInt());
	AssertFileData(machine == NTSC || machine == PAL, "Unknown machine");
	doc.SetMachine(machine);

	if (ver >= 7) {		// // // 050B
		switch (file_.GetBlockInt()) {
		case 1:
			doc.SetEngineSpeed(static_cast<int>(1000000. / file_.GetBlockInt() + .5));
			break;
		case 0: case 2:
		default:
			file_.GetBlockInt();
			doc.SetEngineSpeed(0);
		}
	}
	else
		doc.SetEngineSpeed(file_.GetBlockInt());

	if (ver > 2)
		doc.SetVibratoStyle(file_.GetBlockInt() ? VIBRATO_NEW : VIBRATO_OLD);		// // //
	else
		doc.SetVibratoStyle(VIBRATO_OLD);

	// TODO read m_bLinearPitch
	if (ver >= 9) {		// // // 050B
		bool SweepReset = file_.GetBlockInt() != 0;
	}

	doc.SetHighlight(CSongData::DEFAULT_HIGHLIGHT);		// // //

	if (ver > 3 && ver <= 6) {		// // // 050B
		stHighlight hl;
		hl.First = file_.GetBlockInt();
		hl.Second = file_.GetBlockInt();
		doc.SetHighlight(hl);
	}

	// This is strange. Sometimes expansion chip is set to 0xFF in files
	if (channels == 5)
		Expansion = SNDCHIP_NONE;

	if (file_.GetFileVersion() == 0x0200) {
		int Speed = Song.GetSongSpeed();
		if (Speed < 20)
			Song.SetSongSpeed(Speed + 1);
	}

	if (ver == 1) {
		if (Song.GetSongSpeed() > 19) {
			Song.SetSongTempo(Song.GetSongSpeed());
			Song.SetSongSpeed(6);
		}
		else {
			Song.SetSongTempo(machine == NTSC ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL);
		}
	}

	// Read namco channel count
	int n163chans = 0;
	if (ver >= 5 && (Expansion & SNDCHIP_N163))
		n163chans = AssertRange(file_.GetBlockInt(), 1, 8, "N163 channel count");

	// Determine if new or old split point is preferred
	doc.SetSpeedSplitPoint(ver >= 6 ? file_.GetBlockInt() : CFamiTrackerDoc::OLD_SPEED_SPLIT_POINT);

	AssertRange<MODULE_ERROR_STRICT>(Expansion, 0u, 0x3Fu, "Expansion chip flag");

	if (ver >= 8) {		// // // 050B
		int semitones = file_.GetBlockChar();
		doc.SetTuning(semitones, file_.GetBlockChar());
	}

	doc.SelectExpansionChip(Expansion, n163chans, false);		// // //
}

void CFamiTrackerDocIO::SaveParams(const CFamiTrackerDoc &doc, int ver) {
	if (ver >= 2)
		file_.WriteBlockChar(doc.GetExpansionChip());		// ver 2 change
	else
		file_.WriteBlockInt(doc.GetSongData(0).GetSongSpeed());

	file_.WriteBlockInt(doc.GetChannelCount());
	file_.WriteBlockInt(doc.GetMachine());
	file_.WriteBlockInt(doc.GetEngineSpeed());
	
	if (ver >= 3) {
		file_.WriteBlockInt(doc.GetVibratoStyle());

		if (ver >= 4) {
			auto hl = doc.GetHighlight();
			file_.WriteBlockInt(hl.First);
			file_.WriteBlockInt(hl.Second);

			if (ver >= 5) {
				if (doc.ExpansionEnabled(SNDCHIP_N163))
					file_.WriteBlockInt(doc.GetNamcoChannels());

				if (ver >= 6)
					file_.WriteBlockInt(doc.GetSpeedSplitPoint());

				if (ver >= 8) {		// // // 050B
					file_.WriteBlockChar(doc.GetTuningSemitone());
					file_.WriteBlockChar(doc.GetTuningCent());
				}
			}
		}
	}
}

void CFamiTrackerDocIO::LoadSongInfo(CFamiTrackerDoc &doc, int ver) {
	char buf[CFamiTrackerDoc::METADATA_FIELD_LENGTH];
	file_.GetBlock(buf, std::size(buf));
	doc.SetModuleName(buf);
	file_.GetBlock(buf, std::size(buf));
	doc.SetModuleArtist(buf);
	file_.GetBlock(buf, std::size(buf));
	doc.SetModuleCopyright(buf);
}

void CFamiTrackerDocIO::SaveSongInfo(const CFamiTrackerDoc &doc, int ver) {
	const auto write_sv = [&] (std::string_view sv, size_t len) {
		sv = sv.substr(0, CFamiTrackerDoc::METADATA_FIELD_LENGTH - 1);
		file_.WriteBlock(sv.data(), sv.size());
		for (size_t i = sv.size(); i < len; ++i)
			file_.WriteBlockChar(0);
	};
	write_sv(doc.GetModuleName(), CFamiTrackerDoc::METADATA_FIELD_LENGTH);
	write_sv(doc.GetModuleArtist(), CFamiTrackerDoc::METADATA_FIELD_LENGTH);
	write_sv(doc.GetModuleCopyright(), CFamiTrackerDoc::METADATA_FIELD_LENGTH);
}

void CFamiTrackerDocIO::LoadHeader(CFamiTrackerDoc &doc, int ver) {
	if (ver == 1) {
		// Single track
		auto &Song = doc.GetSongData(0);
		for (int i = 0; i < doc.GetChannelCount(); ++i) try {
			// Channel type (unused)
			AssertRange<MODULE_ERROR_STRICT>(file_.GetBlockChar(), 0, CHANNELS - 1, "Channel type index");
			// Effect columns
			Song.SetEffectColumnCount(i, AssertRange<MODULE_ERROR_STRICT>(
				file_.GetBlockChar(), 0, MAX_EFFECT_COLUMNS - 1, "Effect column count"));
		}
		catch (CModuleException e) {
			e.AppendError("At channel %d", i + 1);
			throw e;
		}
	}
	else if (ver >= 2) {
		// Multiple tracks
		unsigned Tracks = AssertRange(file_.GetBlockChar() + 1, 1, static_cast<int>(MAX_TRACKS), "Track count");	// 0 means one track
		doc.GetSongData(Tracks - 1); // allocate

		// Track names
		if (ver >= 3)
			doc.VisitSongs([&] (CSongData &song) { song.SetTitle((LPCTSTR)file_.ReadString()); });

		for (int i = 0; i < doc.GetChannelCount(); ++i) try {
			AssertRange<MODULE_ERROR_STRICT>(file_.GetBlockChar(), 0, CHANNELS - 1, "Channel type index"); // Channel type (unused)
			doc.VisitSongs([&] (CSongData &song, unsigned index) {
				try {
					song.SetEffectColumnCount(i, AssertRange<MODULE_ERROR_STRICT>(
						file_.GetBlockChar(), 0, MAX_EFFECT_COLUMNS - 1, "Effect column count"));
				}
				catch (CModuleException e) {
					e.AppendError("At song %d,", index + 1);
					throw e;
				}
			});
		}
		catch (CModuleException e) {
			e.AppendError("At channel %d,", i + 1);
			throw e;
		}

		if (ver >= 4)		// // // 050B
			for (unsigned int i = 0; i < Tracks; ++i) {
				int First = static_cast<unsigned char>(file_.GetBlockChar());
				int Second = static_cast<unsigned char>(file_.GetBlockChar());
				if (!i)
					doc.SetHighlight({First, Second});
			}
		doc.VisitSongs([&] (CSongData &song) { song.SetHighlight(doc.GetHighlight()); });		// // //
	}
}

void CFamiTrackerDocIO::SaveHeader(const CFamiTrackerDoc &doc, int ver) {
	// Write number of tracks
	if (ver >= 2)
		file_.WriteBlockChar(doc.GetTrackCount() - 1);

	// Ver 3, store track names
	if (ver >= 3)
		doc.VisitSongs([&] (const CSongData &song) { file_.WriteString(song.GetTitle().c_str()); });

	for (int i = 0; i < doc.GetChannelCount(); ++i) {
		// Channel type
		file_.WriteBlockChar(doc.GetChannelType(i));		// // //
		for (unsigned int j = 0; j < doc.GetTrackCount(); ++j) {
			// Effect columns
			file_.WriteBlockChar(doc.GetSongData(j).GetEffectColumnCount(i));
			if (ver <= 1)
				break;
		}
	}
}

void CFamiTrackerDocIO::LoadInstruments(CFamiTrackerDoc &doc, int ver) {
	/*
	 * Version changes
	 *
	 *  2 - Extended DPCM octave range
	 *  3 - Added settings to the arpeggio sequence
	 *
	 */

	// Number of instruments
	const int Count = AssertRange(file_.GetBlockInt(), 0, CInstrumentManager::MAX_INSTRUMENTS, "Instrument count");
	auto &Manager = *doc.GetInstrumentManager();

	for (int i = 0; i < Count; ++i) {
		// Instrument index
		int index = AssertRange(file_.GetBlockInt(), 0, CInstrumentManager::MAX_INSTRUMENTS - 1, "Instrument index");

		// Read instrument type and create an instrument
		inst_type_t Type = (inst_type_t)file_.GetBlockChar();
		auto pInstrument = Manager.CreateNew(Type);		// // //

		try {
			// Load the instrument
			AssertFileData(pInstrument.get() != nullptr, "Failed to create instrument");
			pInstrument->Load(&file_);
			// Read name
			int size = AssertRange(file_.GetBlockInt(), 0, CInstrument::INST_NAME_MAX, "Instrument name length");
			char Name[CInstrument::INST_NAME_MAX + 1];
			file_.GetBlock(Name, size);
			Name[size] = 0;
			pInstrument->SetName(Name);
			Manager.InsertInstrument(index, std::move(pInstrument));		// // // this registers the instrument content provider
		}
		catch (CModuleException e) {
			file_.SetDefaultFooter(e);
			e.AppendError("At instrument %02X,", index);
			Manager.RemoveInstrument(index);
			throw e;
		}
	}
}

void CFamiTrackerDocIO::SaveInstruments(const CFamiTrackerDoc &doc, int ver) {
	// A bug in v0.3.0 causes a crash if this is not 2, so change only when that ver is obsolete!
	//
	// Log:
	// - v6: adds DPCM delta settings
	//

	// If FDS is used then version must be at least 4 or recent files won't load

	// Fix for FDS instruments
/*	if (m_iExpansionChip & SNDCHIP_FDS)
		ver = 4;

	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (m_pInstrumentManager->GetInstrument(i)->GetType() == INST_FDS)
			ver = 4;
	}
*/
	const auto &Manager = *doc.GetInstrumentManager();

	// Instruments block
	const int Count = Manager.GetInstrumentCount();
	if (!Count)
		return;		// // //
	file_.WriteBlockInt(Count);

	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		// Only write instrument if it's used
		if (auto pInst = Manager.GetInstrument(i)) {
			// Write index and type
			file_.WriteBlockInt(i);
			file_.WriteBlockChar(static_cast<char>(pInst->GetType()));

			// Store the instrument
			pInst->Store(&file_);

			// Store the name
			file_.WriteBlockInt(pInst->GetName().size());
			file_.WriteBlock(pInst->GetName().data(), pInst->GetName().size());
		}
	}
}

void CFamiTrackerDocIO::LoadSequences(CFamiTrackerDoc &doc, int ver) {
	unsigned int Count = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES * SEQ_COUNT, "2A03 sequence count");
	AssertRange<MODULE_ERROR_OFFICIAL>(Count, 0U, static_cast<unsigned>(MAX_SEQUENCES * SEQ_COUNT - 1), "2A03 sequence count");		// // //

	auto &Manager = *doc.GetInstrumentManager();

	if (ver == 1) {
		for (unsigned int i = 0; i < Count; ++i) {
			COldSequence Seq;
			unsigned int Index = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
			unsigned int SeqCount = static_cast<unsigned char>(file_.GetBlockChar());
			AssertRange(SeqCount, 0U, static_cast<unsigned>(MAX_SEQUENCE_ITEMS - 1), "Sequence item count");
			for (unsigned int j = 0; j < SeqCount; ++j) {
				char Value = file_.GetBlockChar();
				Seq.AddItem(file_.GetBlockChar(), Value);
			}
			m_vTmpSequences.push_back(Seq);		// // //
		}
	}
	else if (ver == 2) {
		for (unsigned int i = 0; i < Count; ++i) {
			COldSequence Seq;		// // //
			unsigned int Index = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
			unsigned int Type = AssertRange(file_.GetBlockInt(), 0, SEQ_COUNT - 1, "Sequence type");
			unsigned int SeqCount = static_cast<unsigned char>(file_.GetBlockChar());
			AssertRange(SeqCount, 0U, static_cast<unsigned>(MAX_SEQUENCE_ITEMS - 1), "Sequence item count");
			for (unsigned int j = 0; j < SeqCount; ++j) {
				char Value = file_.GetBlockChar();
				Seq.AddItem(file_.GetBlockChar(), Value);
			}
			Manager.SetSequence(INST_2A03, Type, Index, Seq.Convert(Type).release());		// // //
		}
	}
	else if (ver >= 3) {
		CSequenceManager *pManager = doc.GetSequenceManager(INST_2A03);		// // //
		int Indices[MAX_SEQUENCES * SEQ_COUNT];
		int Types[MAX_SEQUENCES * SEQ_COUNT];

		for (unsigned int i = 0; i < Count; ++i) {
			unsigned int Index = Indices[i] = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
			unsigned int Type = Types[i] = AssertRange(file_.GetBlockInt(), 0, SEQ_COUNT - 1, "Sequence type");
			try {
				unsigned char SeqCount = file_.GetBlockChar();
				// AssertRange(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count");
				auto pSeq = std::make_unique<CSequence>();
				pSeq->SetItemCount(SeqCount < MAX_SEQUENCE_ITEMS ? SeqCount : MAX_SEQUENCE_ITEMS);

				unsigned int LoopPoint = AssertRange<MODULE_ERROR_STRICT>(
					file_.GetBlockInt(), -1, static_cast<int>(SeqCount), "Sequence loop point");
				// Work-around for some older files
				if (LoopPoint != SeqCount)
					pSeq->SetLoopPoint(LoopPoint);

				if (ver == 4) {
					int ReleasePoint = file_.GetBlockInt();
					int Settings = file_.GetBlockInt();
					pSeq->SetReleasePoint(AssertRange<MODULE_ERROR_STRICT>(
						ReleasePoint, -1, static_cast<int>(SeqCount) - 1, "Sequence release point"));
					pSeq->SetSetting(static_cast<seq_setting_t>(Settings));		// // //
				}

				for (int j = 0; j < SeqCount; ++j) {
					char Value = file_.GetBlockChar();
					if (j < MAX_SEQUENCE_ITEMS)		// // //
						pSeq->SetItem(j, Value);
				}
				pManager->GetCollection(Type)->SetSequence(Index, pSeq.release());
			}
			catch (CModuleException e) {
				e.AppendError("At 2A03 %s sequence %d,", CInstrument2A03::SEQUENCE_NAME[Type], Index);
				throw e;
			}
		}

		if (ver == 5) {
			// ver 5 saved the release points incorrectly, this is fixed in ver 6
			for (unsigned int i = 0; i < MAX_SEQUENCES; ++i) {
				for (int j = 0; j < SEQ_COUNT; ++j) try {
					int ReleasePoint = file_.GetBlockInt();
					int Settings = file_.GetBlockInt();
					CSequence *pSeq = pManager->GetCollection(j)->GetSequence(i);
					int Length = pSeq->GetItemCount();
					if (Length > 0) {
						pSeq->SetReleasePoint(AssertRange<MODULE_ERROR_STRICT>(
							ReleasePoint, -1, Length - 1, "Sequence release point"));
						pSeq->SetSetting(static_cast<seq_setting_t>(Settings));		// // //
					}
				}
				catch (CModuleException e) {
					e.AppendError("At 2A03 %s sequence %d,", CInstrument2A03::SEQUENCE_NAME[j], i);
					throw e;
				}
			}
		}
		else if (ver >= 6) {
			// Read release points correctly stored
			for (unsigned int i = 0; i < Count; ++i) try {
				CSequence *pSeq = pManager->GetCollection(Types[i])->GetSequence(Indices[i]);
				pSeq->SetReleasePoint(AssertRange<MODULE_ERROR_STRICT>(
					file_.GetBlockInt(), -1, static_cast<int>(pSeq->GetItemCount()) - 1, "Sequence release point"));
				pSeq->SetSetting(static_cast<seq_setting_t>(file_.GetBlockInt()));		// // //
			}
			catch (CModuleException e) {
				e.AppendError("At 2A03 %s sequence %d,", CInstrument2A03::SEQUENCE_NAME[Types[i]], Indices[i]);
				throw e;
			}
		}
	}
}

void CFamiTrackerDocIO::SaveSequences(const CFamiTrackerDoc &doc, int ver) {
	int Count = doc.GetTotalSequenceCount(INST_2A03);
	if (!Count)
		return;		// // //
	file_.WriteBlockInt(Count);

	VisitSequences(doc.GetSequenceManager(INST_2A03), [&] (const CSequence &seq, int index, int seqType) {
		file_.WriteBlockInt(index);
		file_.WriteBlockInt(seqType);
		file_.WriteBlockChar(seq.GetItemCount());
		file_.WriteBlockInt(seq.GetLoopPoint());
		for (int k = 0, Count = seq.GetItemCount(); k < Count; ++k)
			file_.WriteBlockChar(seq.GetItem(k));
	});

	// v6
	VisitSequences(doc.GetSequenceManager(INST_2A03), [&] (const CSequence &seq, int index, int seqType) {
		file_.WriteBlockInt(seq.GetReleasePoint());
		file_.WriteBlockInt(seq.GetSetting());
	});
}

void CFamiTrackerDocIO::LoadFrames(CFamiTrackerDoc &doc, int ver) {
	if (ver == 1) {
		unsigned int FrameCount = AssertRange(file_.GetBlockInt(), 1, MAX_FRAMES, "Track frame count");
		/*m_iChannelsAvailable =*/ AssertRange(file_.GetBlockInt(), 0, MAX_CHANNELS, "Channel count");
		auto &Song = doc.GetSongData(0);
		Song.SetFrameCount(FrameCount);
		for (unsigned i = 0; i < FrameCount; ++i) {
			for (int j = 0; j < doc.GetChannelCount(); ++j) {
				unsigned Pattern = static_cast<unsigned char>(file_.GetBlockChar());
				AssertRange(Pattern, 0U, static_cast<unsigned>(MAX_PATTERN - 1), "Pattern index");
				Song.SetFramePattern(i, j, Pattern);
			}
		}
	}
	else if (ver > 1) {
		doc.VisitSongs([&] (CSongData &song) {
			unsigned int FrameCount = AssertRange(file_.GetBlockInt(), 1, MAX_FRAMES, "Track frame count");
			unsigned int Speed = AssertRange<MODULE_ERROR_STRICT>(file_.GetBlockInt(), 0, MAX_TEMPO, "Track default speed");
			song.SetFrameCount(FrameCount);

			if (ver >= 3) {
				unsigned int Tempo = AssertRange<MODULE_ERROR_STRICT>(file_.GetBlockInt(), 0, MAX_TEMPO, "Track default tempo");
				song.SetSongTempo(Tempo);
				song.SetSongSpeed(Speed);
			}
			else {
				if (Speed < 20) {
					song.SetSongTempo(doc.GetMachine() == NTSC ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL);
					song.SetSongSpeed(Speed);
				}
				else {
					song.SetSongTempo(Speed);
					song.SetSongSpeed(DEFAULT_SPEED);
				}
			}

			unsigned PatternLength = AssertRange(file_.GetBlockInt(), 1, MAX_PATTERN_LENGTH, "Track default row count");
			song.SetPatternLength(PatternLength);
			
			for (unsigned i = 0; i < FrameCount; ++i) {
				for (int j = 0; j < doc.GetChannelCount(); ++j) {
					// Read pattern index
					int Pattern = static_cast<unsigned char>(file_.GetBlockChar());
					song.SetFramePattern(i, j, AssertRange(Pattern, 0, MAX_PATTERN - 1, "Pattern index"));
				}
			}
		});
	}
}

void CFamiTrackerDocIO::SaveFrames(const CFamiTrackerDoc &doc, int ver) {
	doc.VisitSongs([&] (const CSongData &Song) {
		file_.WriteBlockInt(Song.GetFrameCount());
		file_.WriteBlockInt(Song.GetSongSpeed());
		file_.WriteBlockInt(Song.GetSongTempo());
		file_.WriteBlockInt(Song.GetPatternLength());

		for (unsigned int j = 0; j < Song.GetFrameCount(); ++j)
			for (int k = 0; k < doc.GetChannelCount(); ++k)
				file_.WriteBlockChar((unsigned char)Song.GetFramePattern(j, k));
	});
}

void CFamiTrackerDocIO::LoadPatterns(CFamiTrackerDoc &doc, int ver) {
	fds_adjust_arps_ = ver < 5;		// // //

	bool compat200 = (file_.GetFileVersion() == 0x0200);		// // //

	if (ver == 1) {
		int PatternLen = AssertRange(file_.GetBlockInt(), 0, MAX_PATTERN_LENGTH, "Pattern data count");
		auto &Song = doc.GetSongData(0);
		Song.SetPatternLength(PatternLen);
	}

	while (!file_.BlockDone()) {
		unsigned Track;
		if (ver > 1)
			Track = AssertRange(file_.GetBlockInt(), 0, static_cast<int>(MAX_TRACKS) - 1, "Pattern track index");
		else if (ver == 1)
			Track = 0;

		unsigned Channel = AssertRange(file_.GetBlockInt(), 0, MAX_CHANNELS - 1, "Pattern channel index");
		unsigned Pattern = AssertRange(file_.GetBlockInt(), 0, MAX_PATTERN - 1, "Pattern index");
		unsigned Items	= AssertRange(file_.GetBlockInt(), 0, MAX_PATTERN_LENGTH, "Pattern data count");

		auto &Song = doc.GetSongData(Track);

		for (unsigned i = 0; i < Items; ++i) try {
			unsigned Row;
			if (compat200 || ver >= 6)
				Row = static_cast<unsigned char>(file_.GetBlockChar());
			else
				Row = AssertRange(file_.GetBlockInt(), 0, 0xFF, "Row index");		// // //

			try {
				stChanNote Note;		// // //

				Note.Note = AssertRange<MODULE_ERROR_STRICT>(		// // //
					file_.GetBlockChar(), NONE, ECHO, "Note value");
				Note.Octave = AssertRange<MODULE_ERROR_STRICT>(
					file_.GetBlockChar(), 0, OCTAVE_RANGE - 1, "Octave value");
				int Inst = static_cast<unsigned char>(file_.GetBlockChar());
				if (Inst != HOLD_INSTRUMENT)		// // // 050B
					AssertRange<MODULE_ERROR_STRICT>(Inst, 0, CInstrumentManager::MAX_INSTRUMENTS, "Instrument index");
				Note.Instrument = Inst;
				Note.Vol = AssertRange<MODULE_ERROR_STRICT>(
					file_.GetBlockChar(), 0, MAX_VOLUME, "Channel volume");

				int FX = compat200 ? 1 : ver >= 6 ? MAX_EFFECT_COLUMNS :
					(Song.GetEffectColumnCount(Channel) + 1);		// // // 050B
				for (int n = 0; n < FX; ++n) try {
					unsigned char EffectNumber = file_.GetBlockChar();
					if (Note.EffNumber[n] = static_cast<effect_t>(EffectNumber)) {
						AssertRange<MODULE_ERROR_STRICT>(EffectNumber, EF_NONE, EF_COUNT - 1, "Effect index");
						unsigned char EffectParam = file_.GetBlockChar();
						if (ver < 3) {
							if (EffectNumber == EF_PORTAOFF) {
								EffectNumber = EF_PORTAMENTO;
								EffectParam = 0;
							}
							else if (EffectNumber == EF_PORTAMENTO) {
								if (EffectParam < 0xFF)
									EffectParam++;
							}
						}
						Note.EffParam[n] = EffectParam; // skip on no effect
					}
					else if (ver < 6)
						file_.GetBlockChar(); // unused blank parameter
				}
				catch (CModuleException e) {
					e.AppendError("At effect column fx%d,", n + 1);
					throw e;
				}

	//			if (Note.Vol > MAX_VOLUME)
	//				Note.Vol &= 0x0F;

				if (compat200) {		// // //
					if (Note.EffNumber[0] == EF_SPEED && Note.EffParam[0] < 20)
						Note.EffParam[0]++;

					if (Note.Vol == 0)
						Note.Vol = MAX_VOLUME;
					else {
						Note.Vol--;
						Note.Vol &= 0x0F;
					}

					if (Note.Note == 0)
						Note.Instrument = MAX_INSTRUMENTS;
				}

				if (doc.ExpansionEnabled(SNDCHIP_N163) && doc.GetChipType(Channel) == SNDCHIP_N163) {		// // //
					for (int n = 0; n < MAX_EFFECT_COLUMNS; ++n)
						if (Note.EffNumber[n] == EF_SAMPLE_OFFSET)
							Note.EffNumber[n] = EF_N163_WAVE_BUFFER;
				}

				if (ver == 3) {
					// Fix for VRC7 portamento
					if (doc.ExpansionEnabled(SNDCHIP_VRC7) && Channel > 4) {
						for (int n = 0; n < MAX_EFFECT_COLUMNS; ++n) {
							switch (Note.EffNumber[n]) {
							case EF_PORTA_DOWN:
								Note.EffNumber[n] = EF_PORTA_UP;
								break;
							case EF_PORTA_UP:
								Note.EffNumber[n] = EF_PORTA_DOWN;
								break;
							}
						}
					}
					// FDS pitch effect fix
					else if (doc.ExpansionEnabled(SNDCHIP_FDS) && doc.GetChannelType(Channel) == CHANID_FDS) {
						for (int n = 0; n < MAX_EFFECT_COLUMNS; ++n) {
							switch (Note.EffNumber[n]) {
							case EF_PITCH:
								if (Note.EffParam[n] != 0x80)
									Note.EffParam[n] = (0x100 - Note.EffParam[n]) & 0xFF;
								break;
							}
						}
					}
				}

				if (file_.GetFileVersion() < 0x450) {		// // // 050B
					for (auto &x : Note.EffNumber)
						if (x < EF_COUNT)
							x = compat::EFF_CONVERSION_050.first[x];
				}
				/*
				if (ver < 6) {
					// Noise pitch slide fix
					if (GetChannelType(Channel) == CHANID_NOISE) {
						for (int n = 0; n < MAX_EFFECT_COLUMNS; ++n) {
							switch (Note.EffNumber[n]) {
								case EF_PORTA_DOWN:
									Note.EffNumber[n] = EF_PORTA_UP;
									Note.EffParam[n] = Note.EffParam[n] << 4;
									break;
								case EF_PORTA_UP:
									Note.EffNumber[n] = EF_PORTA_DOWN;
									Note.EffParam[n] = Note.EffParam[n] << 4;
									break;
								case EF_PORTAMENTO:
									Note.EffParam[n] = Note.EffParam[n] << 4;
									break;
								case EF_SLIDE_UP:
									Note.EffParam[n] = Note.EffParam[n] + 0x70;
									break;
								case EF_SLIDE_DOWN:
									Note.EffParam[n] = Note.EffParam[n] + 0x70;
									break;
							}
						}
					}
				}
				*/

				Song.SetPatternData(Channel, Pattern, Row, Note);		// // //
			}
			catch (CModuleException e) {
				e.AppendError("At row %02X,", Row);
				throw e;
			}
		}
		catch (CModuleException e) {
			e.AppendError("At pattern %02X, channel %d, track %d,", Pattern, Channel, Track + 1);
			throw e;
		}
	}
}

void CFamiTrackerDocIO::SavePatterns(const CFamiTrackerDoc &doc, int ver) {
	/*
	 * Version changes: 
	 *
	 *  2: Support multiple tracks
	 *  3: Changed portamento effect
	 *  4: Switched portamento effects for VRC7 (1xx & 2xx), adjusted Pxx for FDS
	 *  5: Adjusted FDS octave
	 *  (6: Noise pitch slide effects fix)
	 *
	 */ 

	doc.VisitSongs([&] (const CSongData &x, unsigned song) {
		x.VisitPatterns([&] (const CPatternData &pattern, unsigned ch, unsigned index) {
			// Save all rows
			unsigned int PatternLen = MAX_PATTERN_LENGTH;
			//unsigned int PatternLen = Song.GetPatternLength();

			unsigned Items = pattern.GetNoteCount(PatternLen);
			if (!Items)
				return;
			file_.WriteBlockInt(song);		// Write track
			file_.WriteBlockInt(ch);		// Write channel
			file_.WriteBlockInt(index);		// Write pattern
			file_.WriteBlockInt(Items);		// Number of items

			pattern.VisitRows(PatternLen, [&] (const stChanNote &note, unsigned row) {
				if (note == stChanNote { })
					return;
				file_.WriteBlockInt(row);
				file_.WriteBlockChar(note.Note);
				file_.WriteBlockChar(note.Octave);
				file_.WriteBlockChar(note.Instrument);
				file_.WriteBlockChar(note.Vol);
				for (int n = 0, EffColumns = doc.GetEffColumns(song, ch) + 1; n < EffColumns; ++n) {
					file_.WriteBlockChar(compat::EFF_CONVERSION_050.second[note.EffNumber[n]]);		// // // 050B
					file_.WriteBlockChar(note.EffParam[n]);
				}
			});
		});
	});
}

void CFamiTrackerDocIO::LoadDSamples(CFamiTrackerDoc &doc, int ver) {
	unsigned int Count = AssertRange(
		static_cast<unsigned char>(file_.GetBlockChar()), 0U, CDSampleManager::MAX_DSAMPLES, "DPCM sample count");

	for (unsigned int i = 0; i < Count; ++i) {
		unsigned int Index = AssertRange(
			static_cast<unsigned char>(file_.GetBlockChar()), 0U, CDSampleManager::MAX_DSAMPLES - 1, "DPCM sample index");
		try {
			auto pSample = std::make_unique<CDSample>();		// // //
			unsigned int Len = AssertRange(file_.GetBlockInt(), 0, CDSample::MAX_NAME_SIZE - 1, "DPCM sample name length");
			char Name[CDSample::MAX_NAME_SIZE] = { };
			file_.GetBlock(Name, Len);
			pSample->SetName(Name);
			int Size = AssertRange(file_.GetBlockInt(), 0, 0x7FFF, "DPCM sample size");
			AssertFileData<MODULE_ERROR_STRICT>(Size <= 0xFF1 && Size % 0x10 == 1, "Bad DPCM sample size");
			int TrueSize = Size + ((1 - Size) & 0x0F);		// // //
			auto pData = std::make_unique<char[]>(TrueSize);
			file_.GetBlock(pData.get(), Size);
			memset(pData.get() + Size, 0xAA, TrueSize - Size);
			pSample->SetData(TrueSize, pData.release());
			doc.SetSample(Index, pSample.release());
		}
		catch (CModuleException e) {
			e.AppendError("At DPCM sample %d,", Index);
			throw e;
		}
	}
}

void CFamiTrackerDocIO::SaveDSamples(const CFamiTrackerDoc &doc, int ver) {
	const auto &manager = *doc.GetDSampleManager();
	if (int Count = manager.GetSampleCount()) {		// // //
		// Write sample count
		file_.WriteBlockChar(Count);

		for (unsigned int i = 0; i < CDSampleManager::MAX_DSAMPLES; ++i) {
			if (const CDSample *pSamp = manager.GetDSample(i)) {
				// Write sample
				file_.WriteBlockChar(i);
				int Length = strlen(pSamp->GetName());
				file_.WriteBlockInt(Length);
				file_.WriteBlock(pSamp->GetName(), Length);
				file_.WriteBlockInt(pSamp->GetSize());
				file_.WriteBlock(pSamp->GetData(), pSamp->GetSize());
			}
		}
	}
}

void CFamiTrackerDocIO::LoadComments(CFamiTrackerDoc &doc, int ver) {
	bool disp = file_.GetBlockInt() == 1;
	doc.SetComment(file_.ReadString().GetString(), disp);
}

void CFamiTrackerDocIO::SaveComments(const CFamiTrackerDoc &doc, int ver) {
	if (const auto &str = doc.GetComment(); !str.empty()) {
		file_.WriteBlockInt(doc.ShowCommentOnOpen() ? 1 : 0);
		file_.WriteString(str.c_str());
	}
}

void CFamiTrackerDocIO::LoadSequencesVRC6(CFamiTrackerDoc &doc, int ver) {
	unsigned int Count = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES * SEQ_COUNT, "VRC6 sequence count");
	AssertRange<MODULE_ERROR_OFFICIAL>(Count, 0U, static_cast<unsigned>(MAX_SEQUENCES), "VRC6 sequence count");		// // //

	CSequenceManager *pManager = doc.GetSequenceManager(INST_VRC6);		// // //

	int Indices[MAX_SEQUENCES * SEQ_COUNT];
	int Types[MAX_SEQUENCES * SEQ_COUNT];
	for (unsigned int i = 0; i < Count; ++i) {
		unsigned int Index = Indices[i] = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
		unsigned int Type = Types[i] = AssertRange(file_.GetBlockInt(), 0, SEQ_COUNT - 1, "Sequence type");
		try {
			unsigned char SeqCount = file_.GetBlockChar();
			CSequence *pSeq = pManager->GetCollection(Type)->GetSequence(Index);
			*pSeq = CSequence { };
			pSeq->SetItemCount(SeqCount < MAX_SEQUENCE_ITEMS ? SeqCount : MAX_SEQUENCE_ITEMS);

			pSeq->SetLoopPoint(AssertRange<MODULE_ERROR_STRICT>(
				file_.GetBlockInt(), -1, static_cast<int>(SeqCount) - 1, "Sequence loop point"));

			if (ver == 4) {
				pSeq->SetReleasePoint(AssertRange<MODULE_ERROR_STRICT>(
					file_.GetBlockInt(), -1, static_cast<int>(SeqCount) - 1, "Sequence release point"));
				pSeq->SetSetting(static_cast<seq_setting_t>(file_.GetBlockInt()));		// // //
			}

			// AssertRange(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count");
			for (unsigned int j = 0; j < SeqCount; ++j) {
				char Value = file_.GetBlockChar();
				if (j < MAX_SEQUENCE_ITEMS)		// // //
					pSeq->SetItem(j, Value);
			}
		}
		catch (CModuleException e) {
			e.AppendError("At VRC6 %s sequence %d,", CInstrumentVRC6::SEQUENCE_NAME[Type], Index);
			throw e;
		}
	}

	if (ver == 5) {
		// Version 5 saved the release points incorrectly, this is fixed in ver 6
		for (int i = 0; i < MAX_SEQUENCES; ++i) {
			for (int j = 0; j < SEQ_COUNT; ++j) try {
				int ReleasePoint = file_.GetBlockInt();
				int Settings = file_.GetBlockInt();
				CSequence *pSeq = pManager->GetCollection(j)->GetSequence(i);
				int Length = pSeq->GetItemCount();
				if (Length > 0) {
					pSeq->SetReleasePoint(AssertRange<MODULE_ERROR_STRICT>(
						ReleasePoint, -1, Length - 1, "Sequence release point"));
					pSeq->SetSetting(static_cast<seq_setting_t>(Settings));		// // //
				}
			}
			catch (CModuleException e) {
				e.AppendError("At VRC6 %s sequence %d,", CInstrumentVRC6::SEQUENCE_NAME[j], i);
				throw e;
			}
		}
	}
	else if (ver >= 6) {
		for (unsigned int i = 0; i < Count; ++i) try {
			CSequence *pSeq = pManager->GetCollection(Types[i])->GetSequence(Indices[i]);
			pSeq->SetReleasePoint(AssertRange<MODULE_ERROR_STRICT>(
				file_.GetBlockInt(), -1, static_cast<int>(pSeq->GetItemCount()) - 1, "Sequence release point"));
			pSeq->SetSetting(static_cast<seq_setting_t>(file_.GetBlockInt()));		// // //
		}
		catch (CModuleException e) {
			e.AppendError("At VRC6 %s sequence %d,", CInstrumentVRC6::SEQUENCE_NAME[Types[i]], Indices[i]);
			throw e;
		}
	}
}

void CFamiTrackerDocIO::SaveSequencesVRC6(const CFamiTrackerDoc &doc, int ver) {
	int Count = doc.GetTotalSequenceCount(INST_VRC6);
	if (!Count)
		return;		// // //
	file_.WriteBlockInt(Count);

	VisitSequences(doc.GetSequenceManager(INST_VRC6), [&] (const CSequence &seq, int index, int seqType) {
		file_.WriteBlockInt(index);
		file_.WriteBlockInt(seqType);
		file_.WriteBlockChar(seq.GetItemCount());
		file_.WriteBlockInt(seq.GetLoopPoint());
		for (int k = 0, Count = seq.GetItemCount(); k < Count; ++k)
			file_.WriteBlockChar(seq.GetItem(k));
	});

	// v6
	VisitSequences(doc.GetSequenceManager(INST_VRC6), [&] (const CSequence &seq, int index, int seqType) {
		file_.WriteBlockInt(seq.GetReleasePoint());
		file_.WriteBlockInt(seq.GetSetting());
	});
}

void CFamiTrackerDocIO::LoadSequencesN163(CFamiTrackerDoc &doc, int ver) {
	unsigned int Count = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES * SEQ_COUNT, "N163 sequence count");
	AssertRange<MODULE_ERROR_OFFICIAL>(Count, 0U, static_cast<unsigned>(MAX_SEQUENCES * SEQ_COUNT - 1), "N163 sequence count");		// // //

	CSequenceManager *pManager = doc.GetSequenceManager(INST_N163);		// // //

	for (unsigned int i = 0; i < Count; i++) {
		unsigned int  Index		   = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
		unsigned int  Type		   = AssertRange(file_.GetBlockInt(), 0, SEQ_COUNT - 1, "Sequence type");
		try {
			unsigned char SeqCount = file_.GetBlockChar();
			CSequence *pSeq = pManager->GetCollection(Type)->GetSequence(Index);
			*pSeq = CSequence { };
			pSeq->SetItemCount(SeqCount < MAX_SEQUENCE_ITEMS ? SeqCount : MAX_SEQUENCE_ITEMS);

			pSeq->SetLoopPoint(AssertRange<MODULE_ERROR_STRICT>(
				file_.GetBlockInt(), -1, static_cast<int>(SeqCount) - 1, "Sequence loop point"));
			pSeq->SetReleasePoint(AssertRange<MODULE_ERROR_STRICT>(
				file_.GetBlockInt(), -1, static_cast<int>(SeqCount) - 1, "Sequence release point"));
			pSeq->SetSetting(static_cast<seq_setting_t>(file_.GetBlockInt()));		// // //

			// AssertRange(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count");
			for (int j = 0; j < SeqCount; ++j) {
				char Value = file_.GetBlockChar();
				if (j < MAX_SEQUENCE_ITEMS)		// // //
					pSeq->SetItem(j, Value);
			}
		}
		catch (CModuleException e) {
			e.AppendError("At N163 %s sequence %d,", CInstrumentN163::SEQUENCE_NAME[Type], Index);
			throw e;
		}
	}
}

void CFamiTrackerDocIO::SaveSequencesN163(const CFamiTrackerDoc &doc, int ver) {
	/* 
	 * Store N163 sequences
	 */ 

	int Count = doc.GetTotalSequenceCount(INST_N163);
	if (!Count)
		return;		// // //
	file_.WriteBlockInt(Count);

	VisitSequences(doc.GetSequenceManager(INST_N163), [&] (const CSequence &seq, int index, int seqType) {
		file_.WriteBlockInt(index);
		file_.WriteBlockInt(seqType);
		file_.WriteBlockChar(seq.GetItemCount());
		file_.WriteBlockInt(seq.GetLoopPoint());
		file_.WriteBlockInt(seq.GetReleasePoint());
		file_.WriteBlockInt(seq.GetSetting());
		for (int k = 0, Count = seq.GetItemCount(); k < Count; ++k)
			file_.WriteBlockChar(seq.GetItem(k));
	});
}

void CFamiTrackerDocIO::LoadSequencesS5B(CFamiTrackerDoc &doc, int ver) {
	unsigned int Count = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES * SEQ_COUNT, "5B sequence count");
	AssertRange<MODULE_ERROR_OFFICIAL>(Count, 0U, static_cast<unsigned>(MAX_SEQUENCES * SEQ_COUNT - 1), "5B sequence count");		// // //

	CSequenceManager *pManager = doc.GetSequenceManager(INST_S5B);		// // //

	for (unsigned int i = 0; i < Count; i++) {
		unsigned int  Index		   = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
		unsigned int  Type		   = AssertRange(file_.GetBlockInt(), 0, SEQ_COUNT - 1, "Sequence type");
		try {
			unsigned char SeqCount = file_.GetBlockChar();
			CSequence *pSeq = pManager->GetCollection(Type)->GetSequence(Index);
			*pSeq = CSequence { };
			pSeq->SetItemCount(SeqCount < MAX_SEQUENCE_ITEMS ? SeqCount : MAX_SEQUENCE_ITEMS);

			pSeq->SetLoopPoint(AssertRange<MODULE_ERROR_STRICT>(
				file_.GetBlockInt(), -1, static_cast<int>(SeqCount) - 1, "Sequence loop point"));
			pSeq->SetReleasePoint(AssertRange<MODULE_ERROR_STRICT>(
				file_.GetBlockInt(), -1, static_cast<int>(SeqCount) - 1, "Sequence release point"));
			pSeq->SetSetting(static_cast<seq_setting_t>(file_.GetBlockInt()));		// // //

			// AssertRange(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count");
			for (int j = 0; j < SeqCount; ++j) {
				char Value = file_.GetBlockChar();
				if (j < MAX_SEQUENCE_ITEMS)		// // //
					pSeq->SetItem(j, Value);
			}
		}
		catch (CModuleException e) {
			e.AppendError("At 5B %s sequence %d,", CInstrumentS5B::SEQUENCE_NAME[Type], Index);
			throw e;
		}
	}
}

void CFamiTrackerDocIO::SaveSequencesS5B(const CFamiTrackerDoc &doc, int ver) {
	int Count = doc.GetTotalSequenceCount(INST_S5B);
	if (!Count)
		return;		// // //
	file_.WriteBlockInt(Count);

	VisitSequences(doc.GetSequenceManager(INST_S5B), [&] (const CSequence &seq, int index, int seqType) {
		file_.WriteBlockInt(index);
		file_.WriteBlockInt(seqType);
		file_.WriteBlockChar(seq.GetItemCount());
		file_.WriteBlockInt(seq.GetLoopPoint());
		file_.WriteBlockInt(seq.GetReleasePoint());
		file_.WriteBlockInt(seq.GetSetting());
		for (int k = 0, Count = seq.GetItemCount(); k < Count; ++k)
			file_.WriteBlockChar(seq.GetItem(k));
	});
}

void CFamiTrackerDocIO::LoadParamsExtra(CFamiTrackerDoc &doc, int ver) {
	doc.SetLinearPitch(file_.GetBlockInt() != 0);
	if (ver >= 2) {
		int semitone = AssertRange(file_.GetBlockChar(), -12, 12, "Global semitone tuning");
		int cent = AssertRange(file_.GetBlockChar(), -100, 100, "Global cent tuning");
		doc.SetTuning(semitone, cent);
	}
}

void CFamiTrackerDocIO::SaveParamsExtra(const CFamiTrackerDoc &doc, int ver) {
	bool linear = doc.GetLinearPitch();
	char semitone = doc.GetTuningSemitone();
	char cent = doc.GetTuningCent();
	if (linear || semitone || cent) {
		file_.WriteBlockInt(linear);
		if (ver >= 2) {
			file_.WriteBlockChar(semitone);
			file_.WriteBlockChar(cent);
		}
	}
}

#include "DetuneDlg.h" // TODO: bad, encapsulate detune tables

void CFamiTrackerDocIO::LoadDetuneTables(CFamiTrackerDoc &doc, int ver) {
	int Count = AssertRange(file_.GetBlockChar(), 0, 6, "Detune table count");
	for (int i = 0; i < Count; ++i) {
		int Chip = AssertRange(file_.GetBlockChar(), 0, 5, "Detune table index");
		try {
			int Item = AssertRange(file_.GetBlockChar(), 0, NOTE_COUNT, "Detune table note count");
			for (int j = 0; j < Item; ++j) {
				int Note = AssertRange(file_.GetBlockChar(), 0, NOTE_COUNT - 1, "Detune table note index");
				int Offset = file_.GetBlockInt();
				doc.SetDetuneOffset(Chip, Note, Offset);
			}
		}
		catch (CModuleException e) {
			e.AppendError("At %s detune table,", CDetuneDlg::CHIP_STR[Chip]);
			throw e;
		}
	}
}

void CFamiTrackerDocIO::SaveDetuneTables(const CFamiTrackerDoc &doc, int ver) {
	int NoteUsed[6] = { };
	int ChipCount = 0;
	for (size_t i = 0; i < std::size(NoteUsed); ++i) {
		for (int j = 0; j < NOTE_COUNT; j++)
			if (doc.GetDetuneOffset(i, j) != 0)
				++NoteUsed[i];
		if (NoteUsed[i])
			++ChipCount;
	}
	if (!ChipCount)
		return;

	file_.WriteBlockChar(ChipCount);
	for (int i = 0; i < 6; i++) {
		if (!NoteUsed[i])
			continue;
		file_.WriteBlockChar(i);
		file_.WriteBlockChar(NoteUsed[i]);
		for (int j = 0; j < NOTE_COUNT; j++)
			if (int detune = doc.GetDetuneOffset(i, j)) {
				file_.WriteBlockChar(j);
				file_.WriteBlockInt(detune);
			}
	}
}

void CFamiTrackerDocIO::LoadGrooves(CFamiTrackerDoc &doc, int ver) {
	const int Count = AssertRange(file_.GetBlockChar(), 0, MAX_GROOVE, "Groove count");

	for (int i = 0; i < Count; i++) {
		int Index = AssertRange(file_.GetBlockChar(), 0, MAX_GROOVE - 1, "Groove index");
		try {
			std::size_t Size = AssertRange((uint8_t)file_.GetBlockChar(), 1u, ft0cc::doc::groove::max_size, "Groove size");
			auto pGroove = std::make_unique<ft0cc::doc::groove>();
			pGroove->resize(Size);
			for (std::size_t j = 0; j < Size; ++j) try {
				pGroove->set_entry(j, AssertRange(
					static_cast<unsigned char>(file_.GetBlockChar()), 1U, 0xFFU, "Groove item"));
			}
			catch (CModuleException e) {
				e.AppendError("At position %i,", j);
				throw e;
			}
			doc.SetGroove(Index, std::move(pGroove));
		}
		catch (CModuleException e) {
			e.AppendError("At groove %i,", Index);
			throw e;
		}
	}

	unsigned int Tracks = file_.GetBlockChar();
	AssertFileData<MODULE_ERROR_STRICT>(Tracks == doc.GetTrackCount(), "Use-groove flag count does not match track count");
	for (unsigned i = 0; i < Tracks; ++i) try {
		bool Use = file_.GetBlockChar() == 1;
		if (i >= doc.GetTrackCount())
			continue;
		int Speed = doc.GetSongSpeed(i);
		doc.SetSongGroove(i, Use);
		if (Use)
			AssertRange(Speed, 0, MAX_GROOVE - 1, "Track default groove index");
		else
			AssertRange(Speed, 1, MAX_TEMPO, "Track default speed");
	}
	catch (CModuleException e) {
		e.AppendError("At track %d,", i + 1);
		throw e;
	}
}

void CFamiTrackerDocIO::SaveGrooves(const CFamiTrackerDoc &doc, int ver) {
	int Count = 0;
	for (int i = 0; i < MAX_GROOVE; ++i)
		if (doc.GetGroove(i))
			++Count;
	if (!Count)
		return;

	file_.WriteBlockChar(Count);
	for (int i = 0; i < MAX_GROOVE; ++i)
		if (const auto pGroove = doc.GetGroove(i)) {
			int Size = pGroove->size();
			file_.WriteBlockChar(i);
			file_.WriteBlockChar(Size);
			for (int j = 0; j < Size; ++j)
				file_.WriteBlockChar(pGroove->entry(j));
		}

	file_.WriteBlockChar(doc.GetTrackCount());
	doc.VisitSongs([&] (const CSongData &song) { file_.WriteBlockChar(song.GetSongGroove()); });
}

void CFamiTrackerDocIO::LoadBookmarks(CFamiTrackerDoc &doc, int ver) {
	for (int i = 0, n = file_.GetBlockInt(); i < n; ++i) {
		auto pMark = std::make_unique<CBookmark>();
		unsigned int Track = AssertRange(
			static_cast<unsigned char>(file_.GetBlockChar()), 0, doc.GetTrackCount() - 1, "Bookmark track index");
		int Frame = static_cast<unsigned char>(file_.GetBlockChar());
		int Row = static_cast<unsigned char>(file_.GetBlockChar());
		pMark->m_iFrame = AssertRange(Frame, 0, static_cast<int>(doc.GetFrameCount(Track)) - 1, "Bookmark frame index");
		pMark->m_iRow = AssertRange(Row, 0, static_cast<int>(doc.GetPatternLength(Track)) - 1, "Bookmark row index");
		pMark->m_Highlight.First = file_.GetBlockInt();
		pMark->m_Highlight.Second = file_.GetBlockInt();
		pMark->m_bPersist = file_.GetBlockChar() != 0;
		pMark->m_sName = std::string(file_.ReadString());
		doc.GetBookmarkCollection(Track)->AddBookmark(pMark.release());
	}
}

void CFamiTrackerDocIO::SaveBookmarks(const CFamiTrackerDoc &doc, int ver) {
	int Count = doc.GetTotalBookmarkCount();
	if (!Count)
		return;
	file_.WriteBlockInt(Count);

	for (unsigned int i = 0, n = doc.GetTrackCount(); i < n; ++i) {
		for (const auto &pMark : *doc.GetBookmarkCollection(i)) {
			file_.WriteBlockChar(i);
			file_.WriteBlockChar(pMark->m_iFrame);
			file_.WriteBlockChar(pMark->m_iRow);
			file_.WriteBlockInt(pMark->m_Highlight.First);
			file_.WriteBlockInt(pMark->m_Highlight.Second);
			file_.WriteBlockChar(pMark->m_bPersist);
			//file_.WriteBlockInt(pMark->m_sName.size());
			//file_.WriteBlock(pMark->m_sName, (int)strlen(Name));	
			file_.WriteString(pMark->m_sName.c_str());
		}
	}
}

template <module_error_level_t l>
void CFamiTrackerDocIO::AssertFileData(bool Cond, const std::string &Msg) const {
	if (l <= theApp.GetSettings()->Version.iErrorLevel && !Cond) {
		CModuleException e = file_.GetException();
		e.AppendError(Msg);
		throw e;
	}
}

template<module_error_level_t l, typename T, typename U, typename V>
T CFamiTrackerDocIO::AssertRange(T Value, U Min, V Max, const std::string &Desc) const try {
	return CModuleException::AssertRangeFmt<l>(Value, Min, Max, Desc);
}
catch (CModuleException e) {
	file_.SetDefaultFooter(e);
//	if (m_pCurrentDocument)
//		m_pCurrentDocument->SetDefaultFooter(e);
	throw e;
}
