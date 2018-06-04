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

#include "FamiTrackerDocIO.h"
#include "FamiTrackerDocIOCommon.h"
#include "FamiTrackerDocOldIO.h"
#include "DocumentFile.h"
#include "BinaryStream.h"
#include "FamiTrackerModule.h"
#include "APU/Types.h"
#include "SoundChipSet.h"
#include "ChannelOrder.h"
#include "str_conv/str_conv.hpp"
#include "NumConv.h"
#include "Assertion.h"

#include "FamiTrackerEnv.h"
#include "SoundChipService.h"
#include "ChannelMap.h"

#include "InstrumentService.h"
#include "InstrumentIO.h"
#include "InstrumentManager.h"
#include "Instrument2A03.h"
#include "InstrumentVRC6.h" // error message
#include "InstrumentN163.h" // error message
#include "InstrumentS5B.h" // error message

#include "SequenceManager.h"
#include "SequenceCollection.h"
#include "Sequence.h"

#include "SongData.h"
#include "ft0cc/doc/pattern_note.hpp"

#include "DSampleManager.h"

#include "ft0cc/doc/groove.hpp"
#include "ft0cc/doc/dpcm_sample.hpp"

#include "BookmarkCollection.h"
#include "Bookmark.h"

namespace {

using namespace std::string_view_literals;

constexpr unsigned MAX_CHANNELS = 28;		// // // vanilla ft channel count

constexpr auto FILE_BLOCK_PARAMS			= "PARAMS"sv;
constexpr auto FILE_BLOCK_INFO				= "INFO"sv;
constexpr auto FILE_BLOCK_INSTRUMENTS		= "INSTRUMENTS"sv;
constexpr auto FILE_BLOCK_SEQUENCES			= "SEQUENCES"sv;
constexpr auto FILE_BLOCK_FRAMES			= "FRAMES"sv;
constexpr auto FILE_BLOCK_PATTERNS			= "PATTERNS"sv;
constexpr auto FILE_BLOCK_DSAMPLES			= "DPCM SAMPLES"sv;
constexpr auto FILE_BLOCK_HEADER			= "HEADER"sv;
constexpr auto FILE_BLOCK_COMMENTS			= "COMMENTS"sv;

// VRC6
constexpr auto FILE_BLOCK_SEQUENCES_VRC6	= "SEQUENCES_VRC6"sv;

// N163
constexpr auto FILE_BLOCK_SEQUENCES_N163	= "SEQUENCES_N163"sv;
constexpr auto FILE_BLOCK_SEQUENCES_N106	= "SEQUENCES_N106"sv;

// Sunsoft
constexpr auto FILE_BLOCK_SEQUENCES_S5B		= "SEQUENCES_S5B"sv;

// // // 0CC-FamiTracker specific
constexpr auto FILE_BLOCK_DETUNETABLES		= "DETUNETABLES"sv;
constexpr auto FILE_BLOCK_GROOVES			= "GROOVES"sv;
constexpr auto FILE_BLOCK_BOOKMARKS			= "BOOKMARKS"sv;
constexpr auto FILE_BLOCK_PARAMS_EXTRA		= "PARAMS_EXTRA"sv;

template <typename F> // (const CSequence &seq, int index, int seqType)
void VisitSequences(const CSequenceManager *manager, F&& f) {
	if (!manager)
		return;
	for (unsigned j = 0, n = manager->GetCount(); j < n; ++j) {
		auto s = static_cast<sequence_t>(j);
		const auto &col = *manager->GetCollection(s);
		for (int i = 0; i < MAX_SEQUENCES; ++i) {
			if (auto pSeq = col.GetSequence(i); pSeq && pSeq->GetItemCount())
				f(*pSeq, i, s);
		}
	}
}

} // namespace

// // // save/load functionality

CFamiTrackerDocReader::CFamiTrackerDocReader(CDocumentFile &file, module_error_level_t err_lv) :
	file_(file), err_lv_(err_lv)
{
}

std::unique_ptr<CFamiTrackerModule> CFamiTrackerDocReader::Load() {
	using map_t = std::unordered_map<std::string_view, void (CFamiTrackerDocReader::*)(CFamiTrackerModule &, CDocumentInputBlock &)>;
	const auto FTM_READ_FUNC = map_t {
		{FILE_BLOCK_PARAMS,			&CFamiTrackerDocReader::LoadParams},
		{FILE_BLOCK_INFO,			&CFamiTrackerDocReader::LoadSongInfo},
		{FILE_BLOCK_HEADER,			&CFamiTrackerDocReader::LoadHeader},
		{FILE_BLOCK_INSTRUMENTS,	&CFamiTrackerDocReader::LoadInstruments},
		{FILE_BLOCK_SEQUENCES,		&CFamiTrackerDocReader::LoadSequences},
		{FILE_BLOCK_FRAMES,			&CFamiTrackerDocReader::LoadFrames},
		{FILE_BLOCK_PATTERNS,		&CFamiTrackerDocReader::LoadPatterns},
		{FILE_BLOCK_DSAMPLES,		&CFamiTrackerDocReader::LoadDSamples},
		{FILE_BLOCK_COMMENTS,		&CFamiTrackerDocReader::LoadComments},
		{FILE_BLOCK_SEQUENCES_VRC6,	&CFamiTrackerDocReader::LoadSequencesVRC6},
		{FILE_BLOCK_SEQUENCES_N163,	&CFamiTrackerDocReader::LoadSequencesN163},
		{FILE_BLOCK_SEQUENCES_N106,	&CFamiTrackerDocReader::LoadSequencesN163},	// Backward compatibility
		{FILE_BLOCK_SEQUENCES_S5B,	&CFamiTrackerDocReader::LoadSequencesS5B},	// // //
		{FILE_BLOCK_PARAMS_EXTRA,	&CFamiTrackerDocReader::LoadParamsExtra},	// // //
		{FILE_BLOCK_DETUNETABLES,	&CFamiTrackerDocReader::LoadDetuneTables},	// // //
		{FILE_BLOCK_GROOVES,		&CFamiTrackerDocReader::LoadGrooves},		// // //
		{FILE_BLOCK_BOOKMARKS,		&CFamiTrackerDocReader::LoadBookmarks},		// // //
	};

	auto modfile = std::make_unique<CFamiTrackerModule>();
	modfile->SetChannelMap(FTEnv.GetSoundChipService()->MakeChannelMap(sound_chip_t::APU, 0));

	// This has to be done for older files
	if (file_.GetFileVersion() < 0x0210)
		(void)modfile->GetSong(0);

	// Read all blocks
	while (true) {
		if (auto block = file_.ReadBlock()) {
			if (auto it = FTM_READ_FUNC.find(block->GetBlockHeaderID()); it != FTM_READ_FUNC.end())
				(this->*(it->second))(*modfile, *block);
			else
				DEBUG_BREAK();
		}
		else if (file_.Finished())
			break;
		else
			return nullptr;
	}

	PostLoad(*modfile);
	return modfile;
}

void CFamiTrackerDocReader::PostLoad(CFamiTrackerModule &modfile) {
	if (file_.GetFileVersion() <= 0x0201)
		compat::ReorderSequences(modfile, std::move(m_vTmpSequences));

	if (fds_adjust_arps_) {
		if (modfile.HasExpansionChip(sound_chip_t::FDS)) {
			modfile.VisitSongs([&] (CSongData &song) {
				if (CTrackData *pFDS = song.GetTrack(fds_subindex_t::wave)) {
					pFDS->VisitPatterns([&] (CPatternData &pattern, std::size_t p) {
						pattern.VisitRows([&] (ft0cc::doc::pattern_note &Note, unsigned r) {
							if (is_note(Note.note())) {
								int Trsp = Note.midi_note() + NOTE_RANGE * 2;
								Trsp = Trsp >= NOTE_COUNT ? NOTE_COUNT - 1 : Trsp;
								Note.set_note(ft0cc::doc::pitch_from_midi(Trsp));
								Note.set_oct(ft0cc::doc::oct_from_midi(Trsp));
							}
						});
					});
				}
			});
		}
		auto *pManager = modfile.GetInstrumentManager();
		for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
			if (pManager->GetInstrumentType(i) == INST_FDS) {
				auto pSeq = std::static_pointer_cast<CSeqInstrument>(pManager->GetInstrument(i))->GetSequence(sequence_t::Arpeggio);
				if (pSeq != nullptr && pSeq->GetItemCount() > 0 && pSeq->GetSetting() == SETTING_ARP_FIXED)
					for (unsigned int j = 0; j < pSeq->GetItemCount(); ++j) {
						int Trsp = pSeq->GetItem(j) + NOTE_RANGE * 2;
						pSeq->SetItem(j, Trsp >= NOTE_COUNT ? NOTE_COUNT - 1 : Trsp);
					}
			}
		}
	}
}

void CFamiTrackerDocReader::LoadParams(CFamiTrackerModule &modfile, CDocumentInputBlock &block) {
	auto &Song = *modfile.GetSong(0);

	CSoundChipSet Expansion = sound_chip_t::APU;		// // //

	// Get first track for module versions that require that
	unsigned ver = block.GetBlockVersion();
	if (ver == 1)
		Song.SetSongSpeed(block.ReadInt<std::uint32_t>());
	else {
		auto flag = block.AssertRange<MODULE_ERROR_STRICT>(block.ReadInt<std::uint8_t>(), 0u, CSoundChipSet::NSF_MAX_FLAG, "Expansion chip flag", err_lv_);
		Expansion = CSoundChipSet::FromNSFFlag(flag);
	}

	int channels = block.AssertRange(block.ReadInt<std::int32_t>(), 1, (int)CHANID_COUNT, "Track count", err_lv_);		// // //
	block.AssertRange<MODULE_ERROR_OFFICIAL>(channels, 1, (int)MAX_CHANNELS - 1, "Track count", err_lv_);

	auto machine = enum_cast<machine_t>(block.ReadInt<std::int32_t>());
	block.AssertFileData(machine != machine_t::none, "Unknown machine", err_lv_);
	modfile.SetMachine(machine);

	if (ver >= 7u) {		// // // 050B
		switch (block.ReadInt<std::uint32_t>()) {
		case 1:
			modfile.SetEngineSpeed(static_cast<int>(1000000. / block.ReadInt<std::uint32_t>() + .5));
			break;
		case 0: case 2:
		default:
			static_cast<void>(block.ReadInt<std::uint32_t>());
			modfile.SetEngineSpeed(0);
		}
	}
	else
		modfile.SetEngineSpeed(block.ReadInt<std::uint32_t>());

	if (ver > 2u)
		modfile.SetVibratoStyle(block.ReadInt<std::uint32_t>() ? vibrato_t::Bidir : vibrato_t::Up);		// // //
	else
		modfile.SetVibratoStyle(vibrato_t::Up);

	// TODO read m_bLinearPitch
	if (ver >= 9u) {		// // // 050B
		static_cast<void>(block.ReadInt<std::uint32_t>() != 0);
	}

	modfile.SetHighlight(CSongData::DEFAULT_HIGHLIGHT);		// // //

	if (ver > 3u && ver <= 6u) {		// // // 050B
		stHighlight hl;
		hl.First = block.ReadInt<std::int32_t>();
		hl.Second = block.ReadInt<std::uint32_t>();
		modfile.SetHighlight(hl);
	}

	// This is strange. Sometimes expansion chip is set to 0xFF in files
	if (channels == 5)
		Expansion = sound_chip_t::APU;

	if (block.GetFileVersion() == 0x0200) {
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
			Song.SetSongTempo(machine == machine_t::NTSC ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL);
		}
	}

	// Read namco channel count
	unsigned n163chans = 0;
	if (ver >= 5u && (Expansion.ContainsChip(sound_chip_t::N163)))
		n163chans = block.AssertRange(block.ReadInt<std::uint32_t>(), 1u, MAX_CHANNELS_N163, "N163 track count", err_lv_);

	// Determine if new or old split point is preferred
	unsigned Split = block.AssertRange<MODULE_ERROR_STRICT>(
		ver >= 6u ? block.ReadInt<std::uint32_t>() : CFamiTrackerModule::OLD_SPEED_SPLIT_POINT, 0u, 255u, "Speed / tempo split point", err_lv_);
	modfile.SetSpeedSplitPoint(Split);

	if (ver >= 8u) {		// // // 050B
		int semitones = block.ReadInt<std::int8_t>();
		modfile.SetTuning(semitones, block.ReadInt<std::int8_t>());
	}

	modfile.SetChannelMap(FTEnv.GetSoundChipService()->MakeChannelMap(Expansion, n163chans));		// // //
	auto &order = modfile.GetChannelOrder();
	order = order.BuiltinOrder();
	block.AssertFileData<MODULE_ERROR_STRICT>(order.GetChannelCount() == channels, "Track count mismatch", err_lv_);
}

void CFamiTrackerDocReader::LoadSongInfo(CFamiTrackerModule &modfile, CDocumentInputBlock &block) {
	modfile.SetModuleName(block.ReadStringN<char>(CFamiTrackerModule::METADATA_FIELD_LENGTH));
	modfile.SetModuleArtist(block.ReadStringN<char>(CFamiTrackerModule::METADATA_FIELD_LENGTH));
	modfile.SetModuleCopyright(block.ReadStringN<char>(CFamiTrackerModule::METADATA_FIELD_LENGTH));
}

void CFamiTrackerDocReader::LoadHeader(CFamiTrackerModule &modfile, CDocumentInputBlock &block) {
	unsigned ver = block.GetBlockVersion();
	if (ver == 1) {
		// Single track
		auto &Song = *modfile.GetSong(0);
		modfile.GetChannelOrder().ForeachChannel([&] (stChannelID i) {
			try {
				// Channel type (unused)
				block.AssertRange<MODULE_ERROR_STRICT>(block.ReadInt<std::int8_t>(), 0, (int)CHANID_COUNT - 1, "Channel type index", err_lv_);
				// Effect columns
				Song.SetEffectColumnCount(i, block.AssertRange<MODULE_ERROR_STRICT>(
					block.ReadInt<std::int8_t>(), 0, MAX_EFFECT_COLUMNS - 1, "Effect column count", err_lv_) + 1);
			}
			catch (CModuleException &e) {
				e.AppendError("At track + " + std::string {FTEnv.GetSoundChipService()->GetChannelFullName(i)});
				throw e;
			}
		});
	}
	else if (ver >= 2u) {
		// Multiple tracks
		unsigned Tracks = block.AssertRange(block.ReadInt<std::int8_t>() + 1, 1, static_cast<int>(MAX_TRACKS), "Song count", err_lv_);	// 0 means one song
		if (!modfile.GetSong(Tracks - 1)) // allocate
			throw CModuleException::WithMessage("Unable to allocate song");

		// Song names
		if (ver >= 3u)
			modfile.VisitSongs([&] (CSongData &song) { song.SetTitle(block.ReadStringNull<char>()); });

		modfile.GetChannelOrder().ForeachChannel([&] (stChannelID i) {
			try {
				block.AssertRange<MODULE_ERROR_STRICT>(block.ReadInt<std::int8_t>(), 0, (int)CHANID_COUNT - 1, "Channel type index", err_lv_); // Channel type (unused)
				modfile.VisitSongs([&] (CSongData &song, unsigned index) {
					try {
						song.SetEffectColumnCount(i, block.AssertRange<MODULE_ERROR_STRICT>(
							block.ReadInt<std::int8_t>(), 0, MAX_EFFECT_COLUMNS - 1, "Effect column count", err_lv_) + 1);
					}
					catch (CModuleException &e) {
						e.AppendError("At song " + conv::from_int(index + 1) + ',');
						throw e;
					}
				});
			}
			catch (CModuleException &e) {
				e.AppendError("At track " + std::string {FTEnv.GetSoundChipService()->GetChannelFullName(i)} + ',');
				throw e;
			}
		});

		if (ver >= 4u)		// // // 050B
			for (unsigned int i = 0; i < Tracks; ++i) {
				int First = block.ReadInt<std::uint8_t>();
				int Second = block.ReadInt<std::uint8_t>();
				if (i == 0)
					modfile.SetHighlight({First, Second});
			}
	}
}

void CFamiTrackerDocReader::LoadInstruments(CFamiTrackerModule &modfile, CDocumentInputBlock &block) {
	/*
	 * Version changes
	 *
	 *  2 - Extended DPCM octave range
	 *  3 - Added settings to the arpeggio sequence
	 *
	 */

	// Number of instruments
	const int Count = block.AssertRange(block.ReadInt<std::int32_t>(), 0, CInstrumentManager::MAX_INSTRUMENTS, "Instrument count", err_lv_);
	auto &Manager = *modfile.GetInstrumentManager();

	for (int i = 0; i < Count; ++i) {
		// Instrument index
		int index = block.AssertRange(block.ReadInt<std::int32_t>(), 0, CInstrumentManager::MAX_INSTRUMENTS - 1, "Instrument index", err_lv_);

		// Read instrument type and create an instrument
		auto Type = static_cast<inst_type_t>(block.ReadInt<std::int8_t>());
		auto pInstrument = Manager.CreateNew(Type);		// // //

		try {
			// Load the instrument
			block.AssertFileData(pInstrument.get() != nullptr, "Failed to create instrument", err_lv_);
			FTEnv.GetInstrumentService()->GetInstrumentIO(Type, err_lv_)->ReadFromModule(*pInstrument, block);		// // //
			// Read name
			auto name = block.ReadString<char>();
			block.AssertFileData<MODULE_ERROR_STRICT>(name.size() <= CInstrument::INST_NAME_MAX, "Instrument name is too long", err_lv_);
			pInstrument->SetName(name);
			Manager.InsertInstrument(index, std::move(pInstrument));		// // // this registers the instrument content provider
		}
		catch (CModuleException &e) {
			file_.SetDefaultFooter(block, e);
			e.AppendError("At instrument " + conv::from_int_hex(index, 2) + ',');
			Manager.RemoveInstrument(index);
			throw e;
		}
	}
}

void CFamiTrackerDocReader::LoadSequences(CFamiTrackerModule &modfile, CDocumentInputBlock &block) {
	unsigned int Count = block.AssertRange(block.ReadInt<std::int32_t>(), 0, MAX_SEQUENCES * (int)SEQ_COUNT, "2A03 sequence count", err_lv_);
	block.AssertRange<MODULE_ERROR_OFFICIAL>(Count, 0U, static_cast<unsigned>(MAX_SEQUENCES * SEQ_COUNT - 1), "2A03 sequence count", err_lv_);		// // //

	auto &Manager = *modfile.GetInstrumentManager();

	unsigned ver = block.GetBlockVersion();
	if (ver == 1) {
		for (unsigned int i = 0; i < Count; ++i) {
			COldSequence Seq;
			(void)block.AssertRange(block.ReadInt<std::int32_t>(), 0, MAX_SEQUENCES - 1, "Sequence index", err_lv_);
			unsigned int SeqCount = block.ReadInt<std::uint8_t>();
			block.AssertRange(SeqCount, 0U, static_cast<unsigned>(MAX_SEQUENCE_ITEMS - 1), "Sequence item count", err_lv_);
			for (unsigned int j = 0; j < SeqCount; ++j) {
				char Value = block.ReadInt<std::int8_t>();
				Seq.AddItem(block.ReadInt<std::int8_t>(), Value);
			}
			m_vTmpSequences.push_back(Seq);		// // //
		}
	}
	else if (ver == 2) {
		for (unsigned int i = 0; i < Count; ++i) {
			COldSequence Seq;		// // //
			unsigned int Index = block.AssertRange(block.ReadInt<std::int32_t>(), 0, MAX_SEQUENCES - 1, "Sequence index", err_lv_);
			auto Type = static_cast<sequence_t>(block.AssertRange(block.ReadInt<std::uint32_t>(), 0u, SEQ_COUNT - 1, "Sequence type", err_lv_));
			unsigned int SeqCount = block.ReadInt<std::uint8_t>();
			block.AssertRange(SeqCount, 0U, static_cast<unsigned>(MAX_SEQUENCE_ITEMS - 1), "Sequence item count", err_lv_);
			for (unsigned int j = 0; j < SeqCount; ++j) {
				char Value = block.ReadInt<std::int8_t>();
				Seq.AddItem(block.ReadInt<std::int8_t>(), Value);
			}
			Manager.SetSequence(INST_2A03, Type, Index, Seq.Convert(Type));		// // //
		}
	}
	else if (ver >= 3u) {
		CSequenceManager *pManager = Manager.GetSequenceManager(INST_2A03);		// // //
		int Indices[MAX_SEQUENCES * SEQ_COUNT];
		sequence_t Types[MAX_SEQUENCES * SEQ_COUNT];

		for (unsigned int i = 0; i < Count; ++i) {
			unsigned int Index = Indices[i] = block.AssertRange(block.ReadInt<std::int32_t>(), 0, MAX_SEQUENCES - 1, "Sequence index", err_lv_);
			auto Type = Types[i] = static_cast<sequence_t>(block.AssertRange(block.ReadInt<std::uint32_t>(), 0u, SEQ_COUNT - 1, "Sequence type", err_lv_));
			try {
				unsigned char SeqCount = block.ReadInt<std::int8_t>();
				// block.AssertRange(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count", err_lv_);
				auto pSeq = std::make_unique<CSequence>(Type);
				pSeq->SetItemCount(SeqCount < MAX_SEQUENCE_ITEMS ? SeqCount : MAX_SEQUENCE_ITEMS);

				unsigned int LoopPoint = block.AssertRange<MODULE_ERROR_STRICT>(
					block.ReadInt<std::int32_t>(), -1, static_cast<int>(SeqCount), "Sequence loop point", err_lv_);
				// Work-around for some older files
				if (LoopPoint != SeqCount)
					pSeq->SetLoopPoint(LoopPoint);

				if (ver == 4) {
					int ReleasePoint = block.ReadInt<std::int32_t>();
					int Settings = block.ReadInt<std::int32_t>();
					pSeq->SetReleasePoint(block.AssertRange<MODULE_ERROR_STRICT>(
						ReleasePoint, -1, static_cast<int>(SeqCount) - 1, "Sequence release point", err_lv_));
					pSeq->SetSetting(static_cast<seq_setting_t>(Settings));		// // //
				}

				for (int j = 0; j < SeqCount; ++j) {
					char Value = block.ReadInt<std::int8_t>();
					if (j < MAX_SEQUENCE_ITEMS)		// // //
						pSeq->SetItem(j, Value);
				}
				pManager->GetCollection(Type)->SetSequence(Index, std::move(pSeq));
			}
			catch (CModuleException &e) {
				e.AppendError("At 2A03 " + std::string {CInstrument2A03::SEQUENCE_NAME[value_cast(Type)]} + " sequence " + conv::from_int(Index) + ',');
				throw e;
			}
		}

		if (ver == 5) {
			// ver 5 saved the release points incorrectly, this is fixed in ver 6
			for (unsigned int i = 0; i < MAX_SEQUENCES; ++i) {
				for (auto j : enum_values<sequence_t>()) try {
					int ReleasePoint = block.ReadInt<std::int32_t>();
					int Settings = block.ReadInt<std::int32_t>();
					auto pSeq = pManager->GetCollection(j)->GetSequence(i);
					int Length = pSeq->GetItemCount();
					if (Length > 0) {
						pSeq->SetReleasePoint(block.AssertRange<MODULE_ERROR_STRICT>(ReleasePoint, -1, Length - 1, "Sequence release point", err_lv_));
						pSeq->SetSetting(static_cast<seq_setting_t>(Settings));		// // //
					}
				}
				catch (CModuleException &e) {
					e.AppendError("At 2A03 " + std::string {CInstrument2A03::SEQUENCE_NAME[value_cast(j)]} + " sequence " + conv::from_int(i) + ',');
					throw e;
				}
			}
		}
		else if (ver >= 6u) {
			// Read release points correctly stored
			for (unsigned int i = 0; i < Count; ++i) try {
				auto pSeq = pManager->GetCollection(Types[i])->GetSequence(Indices[i]);
				pSeq->SetReleasePoint(block.AssertRange<MODULE_ERROR_STRICT>(
					block.ReadInt<std::int32_t>(), -1, static_cast<int>(pSeq->GetItemCount()) - 1, "Sequence release point", err_lv_));
				pSeq->SetSetting(static_cast<seq_setting_t>(block.ReadInt<std::int32_t>()));		// // //
			}
			catch (CModuleException &e) {
				e.AppendError("At 2A03 " + std::string {CInstrument2A03::SEQUENCE_NAME[value_cast(Types[i])]} + " sequence " + conv::from_int(Indices[i]) + ',');
				throw e;
			}
		}
	}
}

void CFamiTrackerDocReader::LoadFrames(CFamiTrackerModule &modfile, CDocumentInputBlock &block) {
	unsigned ver = block.GetBlockVersion();
	if (ver == 1) {
		unsigned int FrameCount = block.AssertRange(block.ReadInt<std::int32_t>(), 1, MAX_FRAMES, "Song frame count", err_lv_);
		/*m_iChannelsAvailable =*/ block.AssertRange<MODULE_ERROR_OFFICIAL>(block.AssertRange(block.ReadInt<std::int32_t>(),
			0, (int)CHANID_COUNT, "Track count", err_lv_), 0, (int)MAX_CHANNELS, "Track count", err_lv_);
		auto &Song = *modfile.GetSong(0);
		Song.SetFrameCount(FrameCount);
		for (unsigned i = 0; i < FrameCount; ++i) {
			modfile.GetChannelOrder().ForeachChannel([&] (stChannelID j) {
				unsigned Pattern = block.ReadInt<std::uint8_t>();
				block.AssertRange(Pattern, 0U, static_cast<unsigned>(MAX_PATTERN - 1), "Pattern index", err_lv_);
				Song.SetFramePattern(i, j, Pattern);
			});
		}
	}
	else if (ver > 1u) {
		modfile.VisitSongs([&] (CSongData &song) {
			unsigned int FrameCount = block.AssertRange(block.ReadInt<std::int32_t>(), 1, MAX_FRAMES, "Song frame count", err_lv_);
			unsigned int Speed = block.AssertRange<MODULE_ERROR_STRICT>(block.ReadInt<std::int32_t>(), 0, MAX_TEMPO, "Song default speed", err_lv_);
			song.SetFrameCount(FrameCount);

			if (ver >= 3u) {
				unsigned int Tempo = block.AssertRange<MODULE_ERROR_STRICT>(block.ReadInt<std::int32_t>(), 0, MAX_TEMPO, "Song default tempo", err_lv_);
				song.SetSongTempo(Tempo);
				song.SetSongSpeed(Speed);
			}
			else {
				if (Speed < 20) {
					song.SetSongTempo(modfile.GetMachine() == machine_t::NTSC ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL);
					song.SetSongSpeed(Speed);
				}
				else {
					song.SetSongTempo(Speed);
					song.SetSongSpeed(DEFAULT_SPEED);
				}
			}

			unsigned PatternLength = block.AssertRange(block.ReadInt<std::int32_t>(), 1, MAX_PATTERN_LENGTH, "Song default row count", err_lv_);
			song.SetPatternLength(PatternLength);

			for (unsigned i = 0; i < FrameCount; ++i) {
				modfile.GetChannelOrder().ForeachChannel([&] (stChannelID j) {
					// Read pattern index
					int Pattern = block.ReadInt<std::uint8_t>();
					song.SetFramePattern(i, j, block.AssertRange(Pattern, 0, MAX_PATTERN - 1, "Pattern index", err_lv_));
				});
			}
		});
	}
}

void CFamiTrackerDocReader::LoadPatterns(CFamiTrackerModule &modfile, CDocumentInputBlock &block) {
	unsigned ver = block.GetBlockVersion();
	fds_adjust_arps_ = ver < 5u;		// // //
	bool compat200 = (block.GetFileVersion() == 0x0200);		// // //

	if (ver == 1) {
		int PatternLen = block.AssertRange(block.ReadInt<std::int32_t>(), 0, MAX_PATTERN_LENGTH, "Pattern data count", err_lv_);
		modfile.GetSong(0)->SetPatternLength(PatternLen);
	}

	const CChannelOrder &order = modfile.GetChannelOrder();		// // //

	while (!block.BlockDone()) {
		unsigned Track = 0;
		if (ver > 1u)
			Track = block.AssertRange(block.ReadInt<std::int32_t>(), 0, static_cast<int>(MAX_TRACKS) - 1, "Pattern song index", err_lv_);

		unsigned Channel = block.AssertRange((unsigned)block.ReadInt<std::int32_t>(), 0u, CHANID_COUNT - 1, "Pattern track index", err_lv_);
		block.AssertRange<MODULE_ERROR_OFFICIAL>(Channel, 0u, MAX_CHANNELS - 1, "Pattern track index", err_lv_);
		unsigned Pattern = block.AssertRange(block.ReadInt<std::int32_t>(), 0, MAX_PATTERN - 1, "Pattern index", err_lv_);
		unsigned Items	= block.AssertRange(block.ReadInt<std::int32_t>(), 0, MAX_PATTERN_LENGTH, "Pattern data count", err_lv_);
		stChannelID ch = order.TranslateChannel(Channel);

		auto *pSong = modfile.GetSong(Track);

		for (unsigned i = 0; i < Items; ++i) try {
			unsigned Row;
			if (compat200 || ver >= 6u)
				Row = block.ReadInt<std::uint8_t>();
			else
				Row = block.AssertRange(block.ReadInt<std::int32_t>(), 0, 0xFF, "Row index", err_lv_);		// // //

			try {
				ft0cc::doc::pattern_note Note;		// // //

				Note.set_note(enum_cast<ft0cc::doc::pitch>(block.AssertRange<MODULE_ERROR_STRICT>(		// // //
					block.ReadInt<std::int8_t>(), value_cast(ft0cc::doc::pitch::none), value_cast(ft0cc::doc::pitch::echo), "Note value", err_lv_)));
				Note.set_oct(block.AssertRange<MODULE_ERROR_STRICT>(block.ReadInt<std::int8_t>(), 0, OCTAVE_RANGE - 1, "Octave value", err_lv_));
				int Inst = block.ReadInt<std::uint8_t>();
				if (Inst != HOLD_INSTRUMENT)		// // // 050B
					block.AssertRange<MODULE_ERROR_STRICT>(Inst, 0, CInstrumentManager::MAX_INSTRUMENTS, "Instrument index", err_lv_);
				Note.set_inst(Inst);
				Note.set_vol(block.AssertRange<MODULE_ERROR_STRICT>(block.ReadInt<std::int8_t>(), 0, MAX_VOLUME, "Channel volume", err_lv_));

				int FX = compat200 ? 1 : ver >= 6u ? MAX_EFFECT_COLUMNS : pSong->GetEffectColumnCount(ch);		// // // 050B
				for (int n = 0; n < FX; ++n) try {
					auto EffectNumber = enum_cast<ft0cc::doc::effect_type>(block.ReadInt<std::int8_t>());
					Note.set_fx_name(n, EffectNumber);
					if (Note.fx_name(n) != ft0cc::doc::effect_type::none) {
						block.AssertRange<MODULE_ERROR_STRICT>(value_cast(EffectNumber), value_cast(ft0cc::doc::effect_type::none), value_cast(ft0cc::doc::effect_type::max), "Effect index", err_lv_);
						unsigned char EffectParam = block.ReadInt<std::int8_t>();
						if (ver < 3u) {
							if (EffectNumber == ft0cc::doc::effect_type::PORTAOFF) {
								EffectNumber = ft0cc::doc::effect_type::PORTAMENTO;
								EffectParam = 0;
							}
							else if (EffectNumber == ft0cc::doc::effect_type::PORTAMENTO) {
								if (EffectParam < 0xFF)
									++EffectParam;
							}
						}
						Note.set_fx_param(n, EffectParam); // skip on no effect
					}
					else if (ver < 6u)
						block.ReadInt<std::int8_t>(); // unused blank parameter
				}
				catch (CModuleException &e) {
					e.AppendError("At effect column fx" + conv::from_int(n + 1) + ',');
					throw e;
				}

	//			if (Note.Vol > MAX_VOLUME)
	//				Note.Vol &= 0x0F;

				if (compat200) {		// // //
					if (Note.fx_name(0) == ft0cc::doc::effect_type::SPEED && Note.fx_param(0) < 20)
						Note.set_fx_param(0, Note.fx_param(0) + 1);

					if (Note.vol() == 0)
						Note.set_vol(MAX_VOLUME);
					else
						Note.set_vol((Note.vol() - 1) & 0x0F);

					if (Note.note() == ft0cc::doc::pitch::none)
						Note.set_inst(MAX_INSTRUMENTS);
				}

				if (modfile.GetSoundChipSet().ContainsChip(sound_chip_t::N163) && ch.Chip == sound_chip_t::N163) {		// // //
					for (auto &cmd : Note.fx_cmds())
						if (cmd.fx == ft0cc::doc::effect_type::SAMPLE_OFFSET)
							cmd.fx = ft0cc::doc::effect_type::N163_WAVE_BUFFER;
				}

				if (ver == 3) {
					// Fix for VRC7 portamento
					if (ch.Chip == sound_chip_t::VRC7) {		// // //
						for (auto &cmd : Note.fx_cmds()) {
							switch (cmd.fx) {
							case ft0cc::doc::effect_type::PORTA_DOWN:
								cmd.fx = ft0cc::doc::effect_type::PORTA_UP;
								break;
							case ft0cc::doc::effect_type::PORTA_UP:
								cmd.fx = ft0cc::doc::effect_type::PORTA_DOWN;
								break;
							}
						}
					}
					// FDS pitch effect fix
					else if (ch.Chip == sound_chip_t::FDS) {
						for (auto &[fx, param] : Note.fx_cmds())
							if (fx == ft0cc::doc::effect_type::PITCH && param != 0x80)
								param = (0x100 - param) & 0xFF;
					}
				}

				if (block.GetFileVersion() < 0x450) {		// // // 050B
					for (auto &cmd : Note.fx_cmds())
						if (cmd.fx <= ft0cc::doc::effect_type::max)
							cmd.fx = compat::EFF_CONVERSION_050.first[value_cast(cmd.fx)];
				}
				/*
				if (ver < 6u) {
					// Noise pitch slide fix
					if (IsAPUNoise(Channel)) {
						for (int n = 0; n < MAX_EFFECT_COLUMNS; ++n) {
							switch (Note.fx_name(n)) {
								case ft0cc::doc::effect_type::PORTA_DOWN:
									Note.set_fx_name(n, ft0cc::doc::effect_type::PORTA_UP);
									Note.set_fx_param(n, Note.fx_param(n) << 4);
									break;
								case ft0cc::doc::effect_type::PORTA_UP:
									Note.set_fx_name(n, ft0cc::doc::effect_type::PORTA_DOWN);
									Note.set_fx_param(n, Note.fx_param(n) << 4);
									break;
								case ft0cc::doc::effect_type::PORTAMENTO:
									Note.set_fx_param(n, Note.fx_param(n) << 4);
									break;
								case ft0cc::doc::effect_type::SLIDE_UP:
									Note.set_fx_param(n, Note.fx_param(n) + 0x70);
									break;
								case ft0cc::doc::effect_type::SLIDE_DOWN:
									Note.set_fx_param(n, Note.fx_param(n) + 0x70);
									break;
							}
						}
					}
				}
				*/

				pSong->GetPattern(ch, Pattern).SetNoteOn(Row, Note);		// // //
			}
			catch (CModuleException &e) {
				e.AppendError("At row " + conv::from_int_hex(Row, 2) + ',');
				throw e;
			}
		}
		catch (CModuleException &e) {
			e.AppendError("At pattern " + conv::from_int_hex(Pattern, 2) + ", channel " + conv::from_int(Channel) + ", song " + conv::from_int(Track + 1) + ',');
			throw e;
		}
	}
}

void CFamiTrackerDocReader::LoadDSamples(CFamiTrackerModule &modfile, CDocumentInputBlock &block) {
	unsigned int Count = block.AssertRange(
		block.ReadInt<std::uint8_t>(), 0U, CDSampleManager::MAX_DSAMPLES, "DPCM sample count", err_lv_);

	auto &manager = *modfile.GetInstrumentManager()->GetDSampleManager();

	for (unsigned int i = 0; i < Count; ++i) {
		unsigned int Index = block.AssertRange(
			block.ReadInt<std::uint8_t>(), 0U, CDSampleManager::MAX_DSAMPLES - 1, "DPCM sample index", err_lv_);
		try {
			auto name = block.ReadString<char>();
			block.AssertFileData<MODULE_ERROR_STRICT>(name.size() <= ft0cc::doc::dpcm_sample::max_name_length, "DPCM sample name too long", err_lv_);
			std::size_t Size = block.AssertRange(block.ReadInt<std::int32_t>(), 0, 0x7FFF, "DPCM sample size", err_lv_);
			block.AssertFileData<MODULE_ERROR_STRICT>(Size <= 0xFF1 && Size % 0x10 == 1, "Bad DPCM sample size", err_lv_);
			std::vector<uint8_t> samples(Size + ((1 - Size) & 0x0F), std::uint8_t {0xAAu});		// // //
			block.ReadBuffer(byte_view(samples).subview(0, Size));
			manager.SetDSample(Index, std::make_unique<ft0cc::doc::dpcm_sample>(samples, name));		// // //
		}
		catch (CModuleException &e) {
			e.AppendError("At DPCM sample " + conv::from_int(Index) + ',');
			throw e;
		}
	}
}

void CFamiTrackerDocReader::LoadComments(CFamiTrackerModule &modfile, CDocumentInputBlock &block) {
	bool disp = block.ReadInt<std::uint32_t>() == 1;
	modfile.SetComment(block.ReadStringNull<char>(), disp);
}

void CFamiTrackerDocReader::LoadSequencesVRC6(CFamiTrackerModule &modfile, CDocumentInputBlock &block) {
	unsigned int Count = block.AssertRange(block.ReadInt<std::int32_t>(), 0, MAX_SEQUENCES * (int)SEQ_COUNT, "VRC6 sequence count", err_lv_);
	block.AssertRange<MODULE_ERROR_OFFICIAL>(Count, 0U, static_cast<unsigned>(MAX_SEQUENCES), "VRC6 sequence count", err_lv_);		// // //

	CSequenceManager *pManager = modfile.GetInstrumentManager()->GetSequenceManager(INST_VRC6);		// // //

	unsigned ver = block.GetBlockVersion();
	int Indices[MAX_SEQUENCES * SEQ_COUNT] = { };
	sequence_t Types[MAX_SEQUENCES * SEQ_COUNT] = { };
	for (unsigned int i = 0; i < Count; ++i) {
		unsigned int Index = Indices[i] = block.AssertRange(block.ReadInt<std::int32_t>(), 0, MAX_SEQUENCES - 1, "Sequence index", err_lv_);
		sequence_t Type = Types[i] = static_cast<sequence_t>(block.AssertRange(block.ReadInt<std::uint32_t>(), 0u, SEQ_COUNT - 1, "Sequence type", err_lv_));
		try {
			unsigned char SeqCount = block.ReadInt<std::int8_t>();
			auto pSeq = pManager->GetCollection(Type)->GetSequence(Index);
			pSeq->Clear();
			pSeq->SetItemCount(SeqCount < MAX_SEQUENCE_ITEMS ? SeqCount : MAX_SEQUENCE_ITEMS);

			pSeq->SetLoopPoint(block.AssertRange<MODULE_ERROR_STRICT>(
				block.ReadInt<std::int32_t>(), -1, static_cast<int>(SeqCount) - 1, "Sequence loop point", err_lv_));

			if (ver == 4) {
				pSeq->SetReleasePoint(block.AssertRange<MODULE_ERROR_STRICT>(
					block.ReadInt<std::int32_t>(), -1, static_cast<int>(SeqCount) - 1, "Sequence release point", err_lv_));
				pSeq->SetSetting(static_cast<seq_setting_t>(block.ReadInt<std::int32_t>()));		// // //
			}

			// block.AssertRange(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count", err_lv_);
			for (unsigned int j = 0; j < SeqCount; ++j) {
				char Value = block.ReadInt<std::int8_t>();
				if (j < MAX_SEQUENCE_ITEMS)		// // //
					pSeq->SetItem(j, Value);
			}
		}
		catch (CModuleException &e) {
			e.AppendError("At VRC6 " + std::string {CInstrumentVRC6::SEQUENCE_NAME[value_cast(Type)]} + " sequence " + conv::from_int(Index) + ',');
			throw e;
		}
	}

	if (ver == 5) {
		// Version 5 saved the release points incorrectly, this is fixed in ver 6
		for (int i = 0; i < MAX_SEQUENCES; ++i) {
			for (auto j : enum_values<sequence_t>()) try {
				int ReleasePoint = block.ReadInt<std::int32_t>();
				int Settings = block.ReadInt<std::int32_t>();
				auto pSeq = pManager->GetCollection(j)->GetSequence(i);
				int Length = pSeq->GetItemCount();
				if (Length > 0) {
					pSeq->SetReleasePoint(block.AssertRange<MODULE_ERROR_STRICT>(ReleasePoint, -1, Length - 1, "Sequence release point", err_lv_));
					pSeq->SetSetting(static_cast<seq_setting_t>(Settings));		// // //
				}
			}
			catch (CModuleException &e) {
				e.AppendError("At VRC6 " + std::string {CInstrumentVRC6::SEQUENCE_NAME[value_cast(j)]} + " sequence " + conv::from_int(i) + ',');
				throw e;
			}
		}
	}
	else if (ver >= 6u) {
		for (unsigned int i = 0; i < Count; ++i) try {
			auto pSeq = pManager->GetCollection(Types[i])->GetSequence(Indices[i]);
			pSeq->SetReleasePoint(block.AssertRange<MODULE_ERROR_STRICT>(
				block.ReadInt<std::int32_t>(), -1, static_cast<int>(pSeq->GetItemCount()) - 1, "Sequence release point", err_lv_));
			pSeq->SetSetting(static_cast<seq_setting_t>(block.ReadInt<std::int32_t>()));		// // //
		}
		catch (CModuleException &e) {
			e.AppendError("At VRC6 " + std::string {CInstrumentVRC6::SEQUENCE_NAME[value_cast(Types[i])]} + " sequence " + conv::from_int(Indices[i]) + ',');
			throw e;
		}
	}
}

void CFamiTrackerDocReader::LoadSequencesN163(CFamiTrackerModule &modfile, CDocumentInputBlock &block) {
	unsigned int Count = block.AssertRange(block.ReadInt<std::int32_t>(), 0, MAX_SEQUENCES * (int)SEQ_COUNT, "N163 sequence count", err_lv_);
	block.AssertRange<MODULE_ERROR_OFFICIAL>(Count, 0U, static_cast<unsigned>(MAX_SEQUENCES * SEQ_COUNT - 1), "N163 sequence count", err_lv_);		// // //

	CSequenceManager *pManager = modfile.GetInstrumentManager()->GetSequenceManager(INST_N163);		// // //

	for (unsigned int i = 0; i < Count; ++i) {
		unsigned Index = block.AssertRange(block.ReadInt<std::int32_t>(), 0, MAX_SEQUENCES - 1, "Sequence index", err_lv_);
		unsigned Type = block.AssertRange(block.ReadInt<std::int32_t>(), 0, (int)SEQ_COUNT - 1, "Sequence type", err_lv_);
		try {
			unsigned char SeqCount = block.ReadInt<std::int8_t>();
			auto pSeq = pManager->GetCollection(static_cast<sequence_t>(Type))->GetSequence(Index);
			pSeq->Clear();
			pSeq->SetItemCount(SeqCount < MAX_SEQUENCE_ITEMS ? SeqCount : MAX_SEQUENCE_ITEMS);

			pSeq->SetLoopPoint(block.AssertRange<MODULE_ERROR_STRICT>(
				block.ReadInt<std::int32_t>(), -1, static_cast<int>(SeqCount) - 1, "Sequence loop point", err_lv_));
			pSeq->SetReleasePoint(block.AssertRange<MODULE_ERROR_STRICT>(
				block.ReadInt<std::int32_t>(), -1, static_cast<int>(SeqCount) - 1, "Sequence release point", err_lv_));
			pSeq->SetSetting(static_cast<seq_setting_t>(block.ReadInt<std::int32_t>()));		// // //

			// block.AssertRange(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count", err_lv_);
			for (int j = 0; j < SeqCount; ++j) {
				char Value = block.ReadInt<std::int8_t>();
				if (j < MAX_SEQUENCE_ITEMS)		// // //
					pSeq->SetItem(j, Value);
			}
		}
		catch (CModuleException &e) {
			e.AppendError("At N163 " + std::string {CInstrumentN163::SEQUENCE_NAME[Type]} + " sequence " + conv::from_int(Index) + ',');
			throw e;
		}
	}
}

void CFamiTrackerDocReader::LoadSequencesS5B(CFamiTrackerModule &modfile, CDocumentInputBlock &block) {
	unsigned int Count = block.AssertRange(block.ReadInt<std::int32_t>(), 0, MAX_SEQUENCES * (int)SEQ_COUNT, "5B sequence count", err_lv_);
	block.AssertRange<MODULE_ERROR_OFFICIAL>(Count, 0U, static_cast<unsigned>(MAX_SEQUENCES * SEQ_COUNT - 1), "5B sequence count", err_lv_);		// // //

	CSequenceManager *pManager = modfile.GetInstrumentManager()->GetSequenceManager(INST_S5B);		// // //

	for (unsigned int i = 0; i < Count; ++i) {
		unsigned Index = block.AssertRange(block.ReadInt<std::int32_t>(), 0, MAX_SEQUENCES - 1, "Sequence index", err_lv_);
		unsigned Type = block.AssertRange(block.ReadInt<std::int32_t>(), 0, (int)SEQ_COUNT - 1, "Sequence type", err_lv_);
		try {
			unsigned char SeqCount = block.ReadInt<std::int8_t>();
			auto pSeq = pManager->GetCollection(static_cast<sequence_t>(Type))->GetSequence(Index);
			pSeq->Clear();
			pSeq->SetItemCount(SeqCount < MAX_SEQUENCE_ITEMS ? SeqCount : MAX_SEQUENCE_ITEMS);

			pSeq->SetLoopPoint(block.AssertRange<MODULE_ERROR_STRICT>(
				block.ReadInt<std::int32_t>(), -1, static_cast<int>(SeqCount) - 1, "Sequence loop point", err_lv_));
			pSeq->SetReleasePoint(block.AssertRange<MODULE_ERROR_STRICT>(
				block.ReadInt<std::int32_t>(), -1, static_cast<int>(SeqCount) - 1, "Sequence release point", err_lv_));
			pSeq->SetSetting(static_cast<seq_setting_t>(block.ReadInt<std::int32_t>()));		// // //

			// block.AssertRange(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count", err_lv_);
			for (int j = 0; j < SeqCount; ++j) {
				char Value = block.ReadInt<std::int8_t>();
				if (j < MAX_SEQUENCE_ITEMS)		// // //
					pSeq->SetItem(j, Value);
			}
		}
		catch (CModuleException &e) {
			e.AppendError("At 5B " + std::string {CInstrumentS5B::SEQUENCE_NAME[Type]} + " sequence " + conv::from_int(Index) + ',');
			throw e;
		}
	}
}

void CFamiTrackerDocReader::LoadParamsExtra(CFamiTrackerModule &modfile, CDocumentInputBlock &block) {
	modfile.SetLinearPitch(block.ReadInt<std::int32_t>() != 0);
	unsigned ver = block.GetBlockVersion();
	if (ver >= 2u) {
		int semitone = block.AssertRange(block.ReadInt<std::int8_t>(), -12, 12, "Global semitone tuning", err_lv_);
		int cent = block.AssertRange(block.ReadInt<std::int8_t>(), -100, 100, "Global cent tuning", err_lv_);
		modfile.SetTuning(semitone, cent);
	}
}

// #include "DetuneDlg.h" // TODO: bad, encapsulate detune tables

void CFamiTrackerDocReader::LoadDetuneTables(CFamiTrackerModule &modfile, CDocumentInputBlock &block) {
	int Count = block.AssertRange(block.ReadInt<std::int8_t>(), 0, 6, "Detune table count", err_lv_);
	for (int i = 0; i < Count; ++i) {
		int Chip = block.AssertRange(block.ReadInt<std::int8_t>(), 0, 5, "Detune table index", err_lv_);
		try {
			int Item = block.AssertRange(block.ReadInt<std::int8_t>(), 0, NOTE_COUNT, "Detune table note count", err_lv_);
			for (int j = 0; j < Item; ++j) {
				int Note = block.AssertRange(block.ReadInt<std::int8_t>(), 0, NOTE_COUNT - 1, "Detune table note index", err_lv_);
				int Offset = block.ReadInt<std::int32_t>();
				modfile.SetDetuneOffset(Chip, Note, Offset);
			}
		}
		catch (CModuleException &e) {
			static const std::string CHIP_STR[] = {"NTSC", "PAL", "Saw", "VRC7", "FDS", "N163"};
			e.AppendError("At " + CHIP_STR[Chip] + " detune table,");
//			e.AppendError("At " + conv::to_utf8(CDetuneDlg::CHIP_STR[Chip]) + " detune table,");
			throw e;
		}
	}
}

void CFamiTrackerDocReader::LoadGrooves(CFamiTrackerModule &modfile, CDocumentInputBlock &block) {
	const int Count = block.AssertRange(block.ReadInt<std::int8_t>(), 0, MAX_GROOVE, "Groove count", err_lv_);

	for (int i = 0; i < Count; ++i) {
		int Index = block.AssertRange(block.ReadInt<std::int8_t>(), 0, MAX_GROOVE - 1, "Groove index", err_lv_);
		try {
			std::size_t Size = block.AssertRange(block.ReadInt<std::uint8_t>(), 1u, ft0cc::doc::groove::max_size, "Groove size", err_lv_);
			auto pGroove = std::make_unique<ft0cc::doc::groove>();
			pGroove->resize(Size);
			for (std::size_t j = 0; j < Size; ++j) try {
				pGroove->set_entry(j, block.AssertRange(
					block.ReadInt<std::uint8_t>(), 1U, 0xFFU, "Groove item", err_lv_));
			}
			catch (CModuleException &e) {
				e.AppendError("At position " + conv::from_int(j) + ',');
				throw e;
			}
			modfile.SetGroove(Index, std::move(pGroove));
		}
		catch (CModuleException &e) {
			e.AppendError("At groove " + conv::from_int(Index) + ',');
			throw e;
		}
	}

	unsigned int Tracks = block.ReadInt<std::int8_t>();
	block.AssertFileData<MODULE_ERROR_STRICT>(Tracks == modfile.GetSongCount(), "Use-groove flag count does not match song count", err_lv_);
	for (unsigned i = 0; i < Tracks; ++i) try {
		bool Use = block.ReadInt<std::int8_t>() == 1;
		if (i >= modfile.GetSongCount())
			continue;
		int Speed = modfile.GetSong(i)->GetSongSpeed();
		modfile.GetSong(i)->SetSongGroove(Use);
		if (Use)
			block.AssertRange(Speed, 0, MAX_GROOVE - 1, "Song default groove index", err_lv_);
		else
			block.AssertRange(Speed, 1, MAX_TEMPO, "Song default speed", err_lv_);
	}
	catch (CModuleException &e) {
		e.AppendError("At song " + conv::from_int(i + 1) + ',');
		throw e;
	}
}

void CFamiTrackerDocReader::LoadBookmarks(CFamiTrackerModule &modfile, CDocumentInputBlock &block) {
	for (int i = 0, n = block.ReadInt<std::int32_t>(); i < n; ++i) {
		auto pMark = std::make_unique<CBookmark>();
		unsigned int Track = block.AssertRange(
			block.ReadInt<std::uint8_t>(), 0, modfile.GetSongCount() - 1, "Bookmark song index", err_lv_);
		int Frame = block.ReadInt<std::uint8_t>();
		int Row = block.ReadInt<std::uint8_t>();

		auto *pSong = modfile.GetSong(Track);
		pMark->m_iFrame = block.AssertRange(Frame, 0, static_cast<int>(pSong->GetFrameCount()) - 1, "Bookmark frame index", err_lv_);
		pMark->m_iRow = block.AssertRange(Row, 0, static_cast<int>(pSong->GetPatternLength()) - 1, "Bookmark row index", err_lv_);
		pMark->m_Highlight.First = block.ReadInt<std::int32_t>();
		pMark->m_Highlight.Second = block.ReadInt<std::int32_t>();
		pMark->m_bPersist = block.ReadInt<std::int8_t>() != 0;
		pMark->m_sName = block.ReadStringNull<char>();
		pSong->GetBookmarks().AddBookmark(std::move(pMark));
	}
}



CFamiTrackerDocWriter::CFamiTrackerDocWriter(CBinaryWriter &file, module_error_level_t err_lv) :
	file_(file), err_lv_(err_lv)
{
}

bool CFamiTrackerDocWriter::Save(const CFamiTrackerModule &modfile) {
	using save_func_t = void (CFamiTrackerDocWriter::*)(const CFamiTrackerModule &, CDocumentOutputBlock &);
	constexpr std::tuple<save_func_t, unsigned, std::string_view> MODULE_WRITE_FUNC[] = {		// // //
		{&CFamiTrackerDocWriter::SaveParams,		6, FILE_BLOCK_PARAMS},
		{&CFamiTrackerDocWriter::SaveSongInfo,		1, FILE_BLOCK_INFO},
		{&CFamiTrackerDocWriter::SaveHeader,		3, FILE_BLOCK_HEADER},
		{&CFamiTrackerDocWriter::SaveInstruments,	6, FILE_BLOCK_INSTRUMENTS},
		{&CFamiTrackerDocWriter::SaveSequences,		6, FILE_BLOCK_SEQUENCES},
		{&CFamiTrackerDocWriter::SaveFrames,		3, FILE_BLOCK_FRAMES},
		{&CFamiTrackerDocWriter::SavePatterns,		5, FILE_BLOCK_PATTERNS},
		{&CFamiTrackerDocWriter::SaveDSamples,		1, FILE_BLOCK_DSAMPLES},
		{&CFamiTrackerDocWriter::SaveComments,		1, FILE_BLOCK_COMMENTS},
		{&CFamiTrackerDocWriter::SaveSequencesVRC6,	6, FILE_BLOCK_SEQUENCES_VRC6},		// // //
		{&CFamiTrackerDocWriter::SaveSequencesN163,	1, FILE_BLOCK_SEQUENCES_N163},
		{&CFamiTrackerDocWriter::SaveSequencesS5B,	1, FILE_BLOCK_SEQUENCES_S5B},
		{&CFamiTrackerDocWriter::SaveParamsExtra,	2, FILE_BLOCK_PARAMS_EXTRA},		// // //
		{&CFamiTrackerDocWriter::SaveDetuneTables,	1, FILE_BLOCK_DETUNETABLES},		// // //
		{&CFamiTrackerDocWriter::SaveGrooves,		1, FILE_BLOCK_GROOVES},				// // //
		{&CFamiTrackerDocWriter::SaveBookmarks,		1, FILE_BLOCK_BOOKMARKS},			// // //
	};

	file_.WriteBytes(byte_view(CDocumentFile::FILE_HEADER_ID));
	file_.WriteBytes(byte_view(CDocumentFile::FILE_VER));
	for (auto [fn, ver, name] : MODULE_WRITE_FUNC) {
		CDocumentOutputBlock block {name, ver};
		(this->*fn)(modfile, block);
		if (!block.FlushToFile(file_))
			return false;
	}
	file_.WriteBytes(byte_view(CDocumentFile::FILE_END_ID));
	return true;
}

void CFamiTrackerDocWriter::SaveParams(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block) {
	unsigned ver = block.GetBlockVersion();
	if (ver >= 2u)
		block.WriteInt<std::int8_t>(modfile.GetSoundChipSet().GetNSFFlag());		// // //
	else
		block.WriteInt<std::int32_t>(modfile.GetSong(0)->GetSongSpeed());

	block.WriteInt<std::int32_t>(modfile.GetChannelOrder().GetChannelCount());
	block.WriteInt<std::int32_t>(value_cast(modfile.GetMachine()));
	block.WriteInt<std::int32_t>(modfile.GetEngineSpeed());

	if (ver >= 3u) {
		block.WriteInt<std::int32_t>(value_cast(modfile.GetVibratoStyle()));

		if (ver >= 4u) {
			const stHighlight &hl = modfile.GetHighlight(0);
			block.WriteInt<std::int32_t>(hl.First);
			block.WriteInt<std::int32_t>(hl.Second);

			if (ver >= 5u) {
				if (modfile.HasExpansionChip(sound_chip_t::N163))
					block.WriteInt<std::int32_t>(modfile.GetNamcoChannels());

				if (ver >= 6u)
					block.WriteInt<std::int32_t>(modfile.GetSpeedSplitPoint());

				if (ver >= 8u) {		// // // 050B
					block.WriteInt<std::int8_t>(modfile.GetTuningSemitone());
					block.WriteInt<std::int8_t>(modfile.GetTuningCent());
				}
			}
		}
	}
}

void CFamiTrackerDocWriter::SaveSongInfo(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block) {
	block.WriteStringN(modfile.GetModuleName(), CFamiTrackerModule::METADATA_FIELD_LENGTH - 1);
	block.WriteInt<std::int8_t>(0);
	block.WriteStringN(modfile.GetModuleArtist(), CFamiTrackerModule::METADATA_FIELD_LENGTH - 1);
	block.WriteInt<std::int8_t>(0);
	block.WriteStringN(modfile.GetModuleCopyright(), CFamiTrackerModule::METADATA_FIELD_LENGTH - 1);
	block.WriteInt<std::int8_t>(0);
}

void CFamiTrackerDocWriter::SaveHeader(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block) {
	const auto ChannelIDToChar = [&] (stChannelID id) {
		enum : unsigned {
			apu_start  = 0u,
			vrc6_start = apu_start + MAX_CHANNELS_2A03,
			mmc5_start = vrc6_start + MAX_CHANNELS_VRC6,
			n163_start = mmc5_start + MAX_CHANNELS_MMC5,
			fds_start  = n163_start + MAX_CHANNELS_N163,
			vrc7_start = fds_start + MAX_CHANNELS_FDS,
			s5b_start  = vrc7_start + MAX_CHANNELS_VRC7,
		};

		switch (id.Chip) {
		case sound_chip_t::APU:
			if (id.Subindex < MAX_CHANNELS_2A03)
				return static_cast<std::uint8_t>(apu_start + id.Subindex);
			break;
		case sound_chip_t::VRC6:
			if (id.Subindex < MAX_CHANNELS_VRC6)
				return static_cast<std::uint8_t>(vrc6_start + id.Subindex);
			break;
		case sound_chip_t::MMC5:
			if (id.Subindex < MAX_CHANNELS_MMC5)
				return static_cast<std::uint8_t>(mmc5_start + id.Subindex);
			break;
		case sound_chip_t::N163:
			if (id.Subindex < MAX_CHANNELS_N163)
				return static_cast<std::uint8_t>(n163_start + id.Subindex);
			break;
		case sound_chip_t::FDS:
			if (id.Subindex < MAX_CHANNELS_FDS)
				return static_cast<std::uint8_t>(fds_start + id.Subindex);
			break;
		case sound_chip_t::VRC7:
			if (id.Subindex < MAX_CHANNELS_VRC7)
				return static_cast<std::uint8_t>(vrc7_start + id.Subindex);
			break;
		case sound_chip_t::S5B:
			if (id.Subindex < MAX_CHANNELS_S5B)
				return static_cast<std::uint8_t>(s5b_start + id.Subindex);
			break;
		}
		return static_cast<std::uint8_t>(-1);
	};

	// Write number of tracks
	unsigned ver = block.GetBlockVersion();
	if (ver >= 2u)
		block.WriteInt<std::int8_t>((unsigned char)modfile.GetSongCount() - 1);

	// Ver 3, store track names
	if (ver >= 3u)
		modfile.VisitSongs([&] (const CSongData &song) { block.WriteStringNull(song.GetTitle()); });

	modfile.GetChannelOrder().ForeachChannel([&] (stChannelID i) {
		// Channel type
		std::uint8_t id = ChannelIDToChar(i);		// // //
		Assert(id != static_cast<std::uint8_t>(-1));
		block.WriteInt<std::int8_t>(id);

		// Effect columns
		if (ver <= 1u)
			block.WriteInt<std::int8_t>(modfile.GetSong(0)->GetEffectColumnCount(i) - 1);
		else
			modfile.VisitSongs([&] (const CSongData &song) {
				block.WriteInt<std::int8_t>(song.GetEffectColumnCount(i) - 1);
			});
	});
}

void CFamiTrackerDocWriter::SaveInstruments(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block) {
	// A bug in v0.3.0 causes a crash if this is not 2, so change only when that ver is obsolete!
	//
	// Log:
	// - v6: adds DPCM delta settings
	//

	// If FDS is used then version must be at least 4 or recent files won't load

	// Fix for FDS instruments
/*	if (m_iExpansionChip & sound_chip_t::FDS)
		ver = 4;

	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (m_pInstrumentManager->GetInstrument(i)->GetType() == INST_FDS)
			ver = 4;
	}
*/
	const auto &Manager = *modfile.GetInstrumentManager();

	// Instruments block
	const int Count = Manager.GetInstrumentCount();
	if (!Count)
		return;		// // //
	block.WriteInt<std::int32_t>(Count);

	// Only write instrument if it's used
	for (int i = 0; i < MAX_INSTRUMENTS; ++i)
		if (auto pInst = Manager.GetInstrument(i))
			FTEnv.GetInstrumentService()->GetInstrumentIO(pInst->GetType(), err_lv_)->WriteToModule(*pInst, block, i);		// // //
}

void CFamiTrackerDocWriter::SaveSequences(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block) {
	int Count = modfile.GetInstrumentManager()->GetTotalSequenceCount(INST_2A03);
	if (!Count)
		return;		// // //
	block.WriteInt<std::int32_t>(Count);

	auto *Manager = modfile.GetInstrumentManager()->GetSequenceManager(INST_2A03);

	VisitSequences(Manager, [&] (const CSequence &seq, int index, sequence_t seqType) {
		block.WriteInt<std::int32_t>(index);
		block.WriteInt<std::int32_t>(value_cast(seqType));
		block.WriteInt<std::int8_t>(seq.GetItemCount());
		block.WriteInt<std::int32_t>(seq.GetLoopPoint());
		for (int k = 0, Count = seq.GetItemCount(); k < Count; ++k)
			block.WriteInt<std::int8_t>(seq.GetItem(k));
	});

	// v6
	VisitSequences(Manager, [&] (const CSequence &seq, int, sequence_t) {
		block.WriteInt<std::int32_t>(seq.GetReleasePoint());
		block.WriteInt<std::int32_t>(value_cast(seq.GetSetting()));
	});
}

void CFamiTrackerDocWriter::SaveFrames(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block) {
	modfile.VisitSongs([&] (const CSongData &Song) {
		block.WriteInt<std::int32_t>(Song.GetFrameCount());
		block.WriteInt<std::int32_t>(Song.GetSongSpeed());
		block.WriteInt<std::int32_t>(Song.GetSongTempo());
		block.WriteInt<std::int32_t>(Song.GetPatternLength());

		for (unsigned int j = 0; j < Song.GetFrameCount(); ++j)
			modfile.GetChannelOrder().ForeachChannel([&] (stChannelID k) {
				block.WriteInt<std::int8_t>((unsigned char)Song.GetFramePattern(j, k));
			});
	});
}

void CFamiTrackerDocWriter::SavePatterns(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block) {
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

	modfile.VisitSongs([&] (const CSongData &x, unsigned song) {
		x.VisitPatterns([&] (const CPatternData &pattern, stChannelID ch, unsigned index) {
			if (!x.IsPatternInUse(ch, index))		// // //
				return;

			// Save all rows
			unsigned int PatternLen = MAX_PATTERN_LENGTH;
			//unsigned int PatternLen = Song.GetPatternLength();

			unsigned Items = pattern.GetNoteCount(PatternLen);
			if (!Items)
				return;
			block.WriteInt<std::int32_t>(song);		// Write track
			block.WriteInt<std::int32_t>(modfile.GetChannelOrder().GetChannelIndex(ch));		// Write channel
			block.WriteInt<std::int32_t>(index);		// Write pattern
			block.WriteInt<std::int32_t>(Items);		// Number of items

			pattern.VisitRows(PatternLen, [&] (const ft0cc::doc::pattern_note &note, unsigned row) {
				if (note == ft0cc::doc::pattern_note { })
					return;
				block.WriteInt<std::int32_t>(row);
				block.WriteInt<std::int8_t>(value_cast(note.note()));
				block.WriteInt<std::int8_t>(note.oct());
				block.WriteInt<std::int8_t>(note.inst());
				block.WriteInt<std::int8_t>(note.vol());
				for (int i = 0, EffColumns = x.GetEffectColumnCount(ch); i < EffColumns; ++i) {
					block.WriteInt<std::int8_t>(value_cast(compat::EFF_CONVERSION_050.second[value_cast(note.fx_name(i))]));		// // // 050B
					block.WriteInt<std::int8_t>(note.fx_param(i));
				}
			});
		});
	});
}

void CFamiTrackerDocWriter::SaveDSamples(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block) {
	const auto &manager = *modfile.GetInstrumentManager()->GetDSampleManager();
	if (int Count = manager.GetDSampleCount()) {		// // //
		// Write sample count
		block.WriteInt<std::int8_t>(Count);

		for (unsigned int i = 0; i < CDSampleManager::MAX_DSAMPLES; ++i) {
			if (auto pSamp = manager.GetDSample(i)) {
				// Write sample
				block.WriteInt<std::int8_t>(i);
				block.WriteString(pSamp->name());
				block.WriteInt<std::int32_t>(pSamp->size());
				block.WriteBytes(byte_view(*pSamp));
			}
		}
	}
}

void CFamiTrackerDocWriter::SaveComments(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block) {
	if (auto str = modfile.GetComment(); !str.empty()) {
		block.WriteInt<std::int32_t>(modfile.ShowsCommentOnOpen() ? 1 : 0);
		block.WriteStringNull(str);
	}
}

void CFamiTrackerDocWriter::SaveSequencesVRC6(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block) {
	int Count = modfile.GetInstrumentManager()->GetTotalSequenceCount(INST_VRC6);
	if (!Count)
		return;		// // //
	block.WriteInt<std::int32_t>(Count);

	auto *pManager = modfile.GetInstrumentManager()->GetSequenceManager(INST_VRC6);

	VisitSequences(pManager, [&] (const CSequence &seq, int index, sequence_t seqType) {
		block.WriteInt<std::int32_t>(index);
		block.WriteInt<std::int32_t>(value_cast(seqType));
		block.WriteInt<std::int8_t>(seq.GetItemCount());
		block.WriteInt<std::int32_t>(seq.GetLoopPoint());
		for (int k = 0, Count = seq.GetItemCount(); k < Count; ++k)
			block.WriteInt<std::int8_t>(seq.GetItem(k));
	});

	// v6
	VisitSequences(pManager, [&] (const CSequence &seq, int, sequence_t) {
		block.WriteInt<std::int32_t>(seq.GetReleasePoint());
		block.WriteInt<std::int32_t>(value_cast(seq.GetSetting()));
	});
}

void CFamiTrackerDocWriter::SaveSequencesN163(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block) {
	/*
	 * Store N163 sequences
	 */

	auto *pManager = modfile.GetInstrumentManager();
	int Count = pManager->GetTotalSequenceCount(INST_N163);
	if (!Count)
		return;		// // //
	block.WriteInt<std::int32_t>(Count);

	VisitSequences(pManager->GetSequenceManager(INST_N163), [&] (const CSequence &seq, int index, sequence_t seqType) {
		block.WriteInt<std::int32_t>(index);
		block.WriteInt<std::int32_t>(value_cast(seqType));
		block.WriteInt<std::int8_t>(seq.GetItemCount());
		block.WriteInt<std::int32_t>(seq.GetLoopPoint());
		block.WriteInt<std::int32_t>(seq.GetReleasePoint());
		block.WriteInt<std::int32_t>(value_cast(seq.GetSetting()));
		for (int k = 0, Count = seq.GetItemCount(); k < Count; ++k)
			block.WriteInt<std::int8_t>(seq.GetItem(k));
	});
}

void CFamiTrackerDocWriter::SaveSequencesS5B(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block) {
	auto *pManager = modfile.GetInstrumentManager();
	int Count = pManager->GetTotalSequenceCount(INST_S5B);
	if (!Count)
		return;		// // //
	block.WriteInt<std::int32_t>(Count);

	VisitSequences(pManager->GetSequenceManager(INST_S5B), [&] (const CSequence &seq, int index, sequence_t seqType) {
		block.WriteInt<std::int32_t>(index);
		block.WriteInt<std::int32_t>(value_cast(seqType));
		block.WriteInt<std::int8_t>(seq.GetItemCount());
		block.WriteInt<std::int32_t>(seq.GetLoopPoint());
		block.WriteInt<std::int32_t>(seq.GetReleasePoint());
		block.WriteInt<std::int32_t>(value_cast(seq.GetSetting()));
		for (int k = 0, Count = seq.GetItemCount(); k < Count; ++k)
			block.WriteInt<std::int8_t>(seq.GetItem(k));
	});
}

void CFamiTrackerDocWriter::SaveParamsExtra(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block) {
	unsigned ver = block.GetBlockVersion();
	bool linear = modfile.GetLinearPitch();
	char semitone = modfile.GetTuningSemitone();
	char cent = modfile.GetTuningCent();
	if (linear || semitone || cent) {
		block.WriteInt<std::int32_t>(linear);
		if (ver >= 2u) {
			block.WriteInt<std::int8_t>(semitone);
			block.WriteInt<std::int8_t>(cent);
		}
	}
}

void CFamiTrackerDocWriter::SaveDetuneTables(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block) {
	int NoteUsed[6] = { };
	int ChipCount = 0;
	for (size_t i = 0; i < std::size(NoteUsed); ++i) {
		for (int j = 0; j < NOTE_COUNT; ++j)
			if (modfile.GetDetuneOffset(i, j) != 0)
				++NoteUsed[i];
		if (NoteUsed[i])
			++ChipCount;
	}
	if (!ChipCount)
		return;

	block.WriteInt<std::int8_t>(ChipCount);
	for (int i = 0; i < 6; ++i) {
		if (!NoteUsed[i])
			continue;
		block.WriteInt<std::int8_t>(i);
		block.WriteInt<std::int8_t>(NoteUsed[i]);
		for (int j = 0; j < NOTE_COUNT; ++j)
			if (int detune = modfile.GetDetuneOffset(i, j)) {
				block.WriteInt<std::int8_t>(j);
				block.WriteInt<std::int32_t>(detune);
			}
	}
}

void CFamiTrackerDocWriter::SaveGrooves(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block) {
	int Count = 0;
	for (int i = 0; i < MAX_GROOVE; ++i)
		if (modfile.HasGroove(i))
			++Count;
	if (!Count)
		return;

	block.WriteInt<std::int8_t>(Count);
	for (int i = 0; i < MAX_GROOVE; ++i)
		if (const auto pGroove = modfile.GetGroove(i)) {
			int Size = pGroove->size();
			block.WriteInt<std::int8_t>(i);
			block.WriteInt<std::int8_t>(Size);
			for (int j = 0; j < Size; ++j)
				block.WriteInt<std::int8_t>(pGroove->entry(j));
		}

	block.WriteInt<std::int8_t>((unsigned char)modfile.GetSongCount());
	modfile.VisitSongs([&] (const CSongData &song) { block.WriteInt<std::int8_t>(song.GetSongGroove()); });
}

void CFamiTrackerDocWriter::SaveBookmarks(const CFamiTrackerModule &modfile, CDocumentOutputBlock &block) {
	int Count = 0;
	modfile.VisitSongs([&] (const CSongData &song) {
		Count += song.GetBookmarks().GetCount();
	});
	if (!Count)
		return;
	block.WriteInt<std::int32_t>(Count);

	modfile.VisitSongs([&] (const CSongData &song, unsigned index) {
		for (const auto &pMark : song.GetBookmarks()) {
			block.WriteInt<std::int8_t>(index);
			block.WriteInt<std::int8_t>(pMark->m_iFrame);
			block.WriteInt<std::int8_t>(pMark->m_iRow);
			block.WriteInt<std::int32_t>(pMark->m_Highlight.First);
			block.WriteInt<std::int32_t>(pMark->m_Highlight.Second);
			block.WriteInt<std::int8_t>(pMark->m_bPersist);
			block.WriteStringNull(std::string_view {pMark->m_sName});
		}
	});
}
