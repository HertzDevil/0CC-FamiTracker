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
#include "DocumentFile.h" // stdafx.h
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
#include "PatternNote.h"

#include "DSampleManager.h"

#include "ft0cc/doc/groove.hpp"
#include "ft0cc/doc/dpcm_sample.hpp"

#include "BookmarkCollection.h"
#include "Bookmark.h"

namespace {

using namespace std::string_view_literals;

const unsigned MAX_CHANNELS = 28;		// // // vanilla ft channel count

const auto FILE_BLOCK_PARAMS			= "PARAMS"sv;
const auto FILE_BLOCK_INFO				= "INFO"sv;
const auto FILE_BLOCK_INSTRUMENTS		= "INSTRUMENTS"sv;
const auto FILE_BLOCK_SEQUENCES			= "SEQUENCES"sv;
const auto FILE_BLOCK_FRAMES			= "FRAMES"sv;
const auto FILE_BLOCK_PATTERNS			= "PATTERNS"sv;
const auto FILE_BLOCK_DSAMPLES			= "DPCM SAMPLES"sv;
const auto FILE_BLOCK_HEADER			= "HEADER"sv;
const auto FILE_BLOCK_COMMENTS			= "COMMENTS"sv;

// VRC6
const auto FILE_BLOCK_SEQUENCES_VRC6	= "SEQUENCES_VRC6"sv;

// N163
const auto FILE_BLOCK_SEQUENCES_N163	= "SEQUENCES_N163"sv;
const auto FILE_BLOCK_SEQUENCES_N106	= "SEQUENCES_N106"sv;

// Sunsoft
const auto FILE_BLOCK_SEQUENCES_S5B		= "SEQUENCES_S5B"sv;

// // // 0CC-FamiTracker specific
const auto FILE_BLOCK_DETUNETABLES		= "DETUNETABLES"sv;
const auto FILE_BLOCK_GROOVES			= "GROOVES"sv;
const auto FILE_BLOCK_BOOKMARKS			= "BOOKMARKS"sv;
const auto FILE_BLOCK_PARAMS_EXTRA		= "PARAMS_EXTRA"sv;

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

CFamiTrackerDocIO::CFamiTrackerDocIO(CDocumentFile &file, module_error_level_t err_lv) :
	file_(file), err_lv_(err_lv)
{
}

bool CFamiTrackerDocIO::Load(CFamiTrackerModule &modfile) {
	using map_t = std::unordered_map<std::string_view, void (CFamiTrackerDocIO::*)(CFamiTrackerModule &, int)>;
	const auto FTM_READ_FUNC = map_t {
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
	int msgs_ = 0;
#endif

	// This has to be done for older files
	if (file_.GetFileVersion() < 0x0210)
		(void)modfile.GetSong(0);

	// Read all blocks
	bool ErrorFlag = false;
	while (!file_.Finished() && !ErrorFlag) {
		ErrorFlag = file_.ReadBlock();
		std::string_view BlockID = file_.GetBlockHeaderID();		// // //
		if (BlockID == "END")
			break;

		try {
			(this->*FTM_READ_FUNC.at(BlockID))(modfile, file_.GetBlockVersion());		// // //
		}
		catch (std::out_of_range) {
#ifdef _DEBUG
		// This shouldn't show up in release (debug only)
//			if (++msgs_ < 5)
//				AfxMessageBox(L"Unknown file block!");
#endif
			if (file_.IsFileIncomplete())
				ErrorFlag = true;
		}
	}

	if (ErrorFlag)
		return false;

	PostLoad(modfile);
	return true;
}

bool CFamiTrackerDocIO::Save(const CFamiTrackerModule &modfile) {
	using block_info_t = std::tuple<void (CFamiTrackerDocIO::*)(const CFamiTrackerModule &, int), int, std::string_view>;
	const block_info_t MODULE_WRITE_FUNC[] = {		// // //
		{&CFamiTrackerDocIO::SaveParams,		6, FILE_BLOCK_PARAMS},
		{&CFamiTrackerDocIO::SaveSongInfo,		1, FILE_BLOCK_INFO},
		{&CFamiTrackerDocIO::SaveHeader,		3, FILE_BLOCK_HEADER},
		{&CFamiTrackerDocIO::SaveInstruments,	6, FILE_BLOCK_INSTRUMENTS},
		{&CFamiTrackerDocIO::SaveSequences,		6, FILE_BLOCK_SEQUENCES},
		{&CFamiTrackerDocIO::SaveFrames,		3, FILE_BLOCK_FRAMES},
		{&CFamiTrackerDocIO::SavePatterns,		5, FILE_BLOCK_PATTERNS},
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
		file_.CreateBlock(name.data(), ver);
		(this->*fn)(modfile, ver);
		if (!file_.FlushBlock())
			return false;
	}
	file_.EndDocument();
	return true;
}

void CFamiTrackerDocIO::PostLoad(CFamiTrackerModule &modfile) {
	if (file_.GetFileVersion() <= 0x0201)
		compat::ReorderSequences(modfile, std::move(m_vTmpSequences));

	if (fds_adjust_arps_) {
		if (modfile.HasExpansionChip(sound_chip_t::FDS)) {
			modfile.VisitSongs([&] (CSongData &song) {
				for (int p = 0; p < MAX_PATTERN; ++p)
					for (int r = 0; r < MAX_PATTERN_LENGTH; ++r) {
						stChanNote &Note = song.GetPatternData(fds_subindex_t::wave, p, r);		// // //
						if (is_note(Note.Note)) {
							int Trsp = Note.ToMidiNote() + NOTE_RANGE * 2;
							Trsp = Trsp >= NOTE_COUNT ? NOTE_COUNT - 1 : Trsp;
							Note.Note = ft0cc::doc::pitch_from_midi(Trsp);
							Note.Octave = ft0cc::doc::oct_from_midi(Trsp);
						}
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

void CFamiTrackerDocIO::LoadParams(CFamiTrackerModule &modfile, int ver) {
	auto &Song = *modfile.GetSong(0);

	CSoundChipSet Expansion = sound_chip_t::APU;		// // //

	// Get first track for module versions that require that
	if (ver == 1)
		Song.SetSongSpeed(file_.GetBlockInt());
	else {
		auto flag = AssertRange<MODULE_ERROR_STRICT>((unsigned char)file_.GetBlockChar(), 0u, CSoundChipSet::NSF_MAX_FLAG, "Expansion chip flag");;
		Expansion = CSoundChipSet::FromNSFFlag(flag);
	}

	int channels = AssertRange(file_.GetBlockInt(), 1, (int)CHANID_COUNT, "Track count");		// // //
	AssertRange<MODULE_ERROR_OFFICIAL>(channels, 1, (int)MAX_CHANNELS - 1, "Track count");

	auto machine = static_cast<machine_t>(file_.GetBlockInt());
	AssertFileData(machine == machine_t::NTSC || machine == machine_t::PAL, "Unknown machine");
	modfile.SetMachine(machine);

	if (ver >= 7) {		// // // 050B
		switch (file_.GetBlockInt()) {
		case 1:
			modfile.SetEngineSpeed(static_cast<int>(1000000. / file_.GetBlockInt() + .5));
			break;
		case 0: case 2:
		default:
			file_.GetBlockInt();
			modfile.SetEngineSpeed(0);
		}
	}
	else
		modfile.SetEngineSpeed(file_.GetBlockInt());

	if (ver > 2)
		modfile.SetVibratoStyle(file_.GetBlockInt() ? vibrato_t::Bidir : vibrato_t::Up);		// // //
	else
		modfile.SetVibratoStyle(vibrato_t::Up);

	// TODO read m_bLinearPitch
	if (ver >= 9) {		// // // 050B
		(void)(file_.GetBlockInt() != 0);
	}

	modfile.SetHighlight(CSongData::DEFAULT_HIGHLIGHT);		// // //

	if (ver > 3 && ver <= 6) {		// // // 050B
		stHighlight hl;
		hl.First = file_.GetBlockInt();
		hl.Second = file_.GetBlockInt();
		modfile.SetHighlight(hl);
	}

	// This is strange. Sometimes expansion chip is set to 0xFF in files
	if (channels == 5)
		Expansion = sound_chip_t::APU;

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
			Song.SetSongTempo(machine == machine_t::NTSC ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL);
		}
	}

	// Read namco channel count
	unsigned n163chans = 0;
	if (ver >= 5 && (Expansion.ContainsChip(sound_chip_t::N163)))
		n163chans = AssertRange((unsigned)file_.GetBlockInt(), 1u, MAX_CHANNELS_N163, "N163 track count");

	// Determine if new or old split point is preferred
	int Split = AssertRange<MODULE_ERROR_STRICT>(ver >= 6 ? file_.GetBlockInt() : (int)CFamiTrackerModule::OLD_SPEED_SPLIT_POINT,
		0, 255, "Speed / tempo split point");
	modfile.SetSpeedSplitPoint(Split);

	if (ver >= 8) {		// // // 050B
		int semitones = file_.GetBlockChar();
		modfile.SetTuning(semitones, file_.GetBlockChar());
	}

	modfile.SetChannelMap(Env.GetSoundChipService()->MakeChannelMap(Expansion, n163chans));		// // //
	auto &order = modfile.GetChannelOrder();
	order = order.BuiltinOrder();
	AssertFileData<MODULE_ERROR_STRICT>(order.GetChannelCount() == channels, "Track count mismatch");
}

void CFamiTrackerDocIO::SaveParams(const CFamiTrackerModule &modfile, int ver) {
	if (ver >= 2)
		file_.WriteBlockChar(modfile.GetSoundChipSet().GetNSFFlag());		// // //
	else
		file_.WriteBlockInt(modfile.GetSong(0)->GetSongSpeed());

	file_.WriteBlockInt(modfile.GetChannelOrder().GetChannelCount());
	file_.WriteBlockInt(value_cast(modfile.GetMachine()));
	file_.WriteBlockInt(modfile.GetEngineSpeed());

	if (ver >= 3) {
		file_.WriteBlockInt(value_cast(modfile.GetVibratoStyle()));

		if (ver >= 4) {
			const stHighlight &hl = modfile.GetHighlight(0);
			file_.WriteBlockInt(hl.First);
			file_.WriteBlockInt(hl.Second);

			if (ver >= 5) {
				if (modfile.HasExpansionChip(sound_chip_t::N163))
					file_.WriteBlockInt(modfile.GetNamcoChannels());

				if (ver >= 6)
					file_.WriteBlockInt(modfile.GetSpeedSplitPoint());

				if (ver >= 8) {		// // // 050B
					file_.WriteBlockChar(modfile.GetTuningSemitone());
					file_.WriteBlockChar(modfile.GetTuningCent());
				}
			}
		}
	}
}

void CFamiTrackerDocIO::LoadSongInfo(CFamiTrackerModule &modfile, int ver) {
	std::array<char, CFamiTrackerModule::METADATA_FIELD_LENGTH> buf = { };
	file_.GetBlock(buf.data(), std::size(buf));
	buf.back() = '\0';
	modfile.SetModuleName(buf.data());
	file_.GetBlock(buf.data(), std::size(buf));
	buf.back() = '\0';
	modfile.SetModuleArtist(buf.data());
	file_.GetBlock(buf.data(), std::size(buf));
	buf.back() = '\0';
	modfile.SetModuleCopyright(buf.data());
}

void CFamiTrackerDocIO::SaveSongInfo(const CFamiTrackerModule &modfile, int ver) {
	file_.WriteStringPadded(modfile.GetModuleName(), CFamiTrackerModule::METADATA_FIELD_LENGTH);
	file_.WriteStringPadded(modfile.GetModuleArtist(), CFamiTrackerModule::METADATA_FIELD_LENGTH);
	file_.WriteStringPadded(modfile.GetModuleCopyright(), CFamiTrackerModule::METADATA_FIELD_LENGTH);
}

void CFamiTrackerDocIO::LoadHeader(CFamiTrackerModule &modfile, int ver) {
	if (ver == 1) {
		// Single track
		auto &Song = *modfile.GetSong(0);
		modfile.GetChannelOrder().ForeachChannel([&] (stChannelID i) {
			try {
				// Channel type (unused)
				AssertRange<MODULE_ERROR_STRICT>(file_.GetBlockChar(), 0, (int)CHANID_COUNT - 1, "Channel type index");
				// Effect columns
				Song.SetEffectColumnCount(i, AssertRange<MODULE_ERROR_STRICT>(
					file_.GetBlockChar(), 0, MAX_EFFECT_COLUMNS - 1, "Effect column count") + 1);
			}
			catch (CModuleException &e) {
				e.AppendError("At track + " + std::string {Env.GetSoundChipService()->GetChannelFullName(i)});
				throw e;
			}
		});
	}
	else if (ver >= 2) {
		// Multiple tracks
		unsigned Tracks = AssertRange(file_.GetBlockChar() + 1, 1, static_cast<int>(MAX_TRACKS), "Song count");	// 0 means one song
		if (!modfile.GetSong(Tracks - 1)) // allocate
			throw CModuleException::WithMessage("Unable to allocate song");

		// Song names
		if (ver >= 3)
			modfile.VisitSongs([&] (CSongData &song) { song.SetTitle(file_.ReadString()); });

		modfile.GetChannelOrder().ForeachChannel([&] (stChannelID i) {
			try {
				AssertRange<MODULE_ERROR_STRICT>(file_.GetBlockChar(), 0, (int)CHANID_COUNT - 1, "Channel type index"); // Channel type (unused)
				modfile.VisitSongs([&] (CSongData &song, unsigned index) {
					try {
						song.SetEffectColumnCount(i, AssertRange<MODULE_ERROR_STRICT>(
							file_.GetBlockChar(), 0, MAX_EFFECT_COLUMNS - 1, "Effect column count") + 1);
					}
					catch (CModuleException &e) {
						e.AppendError("At song " + conv::from_int(index + 1) + ',');
						throw e;
					}
				});
			}
			catch (CModuleException &e) {
				e.AppendError("At track " + std::string {Env.GetSoundChipService()->GetChannelFullName(i)} + ',');
				throw e;
			}
		});

		if (ver >= 4)		// // // 050B
			for (unsigned int i = 0; i < Tracks; ++i) {
				int First = static_cast<unsigned char>(file_.GetBlockChar());
				int Second = static_cast<unsigned char>(file_.GetBlockChar());
				if (i == 0)
					modfile.SetHighlight({First, Second});
			}
	}
}

void CFamiTrackerDocIO::SaveHeader(const CFamiTrackerModule &modfile, int ver) {
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
	if (ver >= 2)
		file_.WriteBlockChar((unsigned char)modfile.GetSongCount() - 1);

	// Ver 3, store track names
	if (ver >= 3)
		modfile.VisitSongs([&] (const CSongData &song) { file_.WriteString(song.GetTitle()); });

	modfile.GetChannelOrder().ForeachChannel([&] (stChannelID i) {
		// Channel type
		std::uint8_t id = ChannelIDToChar(i);		// // //
		Assert(id != static_cast<std::uint8_t>(-1));
		file_.WriteBlockChar(id);

		// Effect columns
		if (ver <= 1)
			file_.WriteBlockChar(modfile.GetSong(0)->GetEffectColumnCount(i) - 1);
		else
			modfile.VisitSongs([&] (const CSongData &song) {
				file_.WriteBlockChar(song.GetEffectColumnCount(i) - 1);
			});
	});
}

void CFamiTrackerDocIO::LoadInstruments(CFamiTrackerModule &modfile, int ver) {
	/*
	 * Version changes
	 *
	 *  2 - Extended DPCM octave range
	 *  3 - Added settings to the arpeggio sequence
	 *
	 */

	// Number of instruments
	const int Count = AssertRange(file_.GetBlockInt(), 0, CInstrumentManager::MAX_INSTRUMENTS, "Instrument count");
	auto &Manager = *modfile.GetInstrumentManager();

	for (int i = 0; i < Count; ++i) {
		// Instrument index
		int index = AssertRange(file_.GetBlockInt(), 0, CInstrumentManager::MAX_INSTRUMENTS - 1, "Instrument index");

		// Read instrument type and create an instrument
		inst_type_t Type = (inst_type_t)file_.GetBlockChar();
		auto pInstrument = Manager.CreateNew(Type);		// // //

		try {
			// Load the instrument
			AssertFileData(pInstrument.get() != nullptr, "Failed to create instrument");
			Env.GetInstrumentService()->GetInstrumentIO(Type, err_lv_)->ReadFromModule(*pInstrument, file_);		// // //
			// Read name
			int size = AssertRange(file_.GetBlockInt(), 0, CInstrument::INST_NAME_MAX, "Instrument name length");
			char Name[CInstrument::INST_NAME_MAX + 1] = { };
			file_.GetBlock(Name, size);
			pInstrument->SetName(Name);
			Manager.InsertInstrument(index, std::move(pInstrument));		// // // this registers the instrument content provider
		}
		catch (CModuleException &e) {
			file_.SetDefaultFooter(e);
			e.AppendError("At instrument " + conv::from_int_hex(index, 2) + ',');
			Manager.RemoveInstrument(index);
			throw e;
		}
	}
}

void CFamiTrackerDocIO::SaveInstruments(const CFamiTrackerModule &modfile, int ver) {
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
	file_.WriteBlockInt(Count);

	// Only write instrument if it's used
	for (int i = 0; i < MAX_INSTRUMENTS; ++i)
		if (auto pInst = Manager.GetInstrument(i))
			Env.GetInstrumentService()->GetInstrumentIO(pInst->GetType(), err_lv_)->WriteToModule(*pInst, file_, i);		// // //
}

void CFamiTrackerDocIO::LoadSequences(CFamiTrackerModule &modfile, int ver) {
	unsigned int Count = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES * (int)SEQ_COUNT, "2A03 sequence count");
	AssertRange<MODULE_ERROR_OFFICIAL>(Count, 0U, static_cast<unsigned>(MAX_SEQUENCES * SEQ_COUNT - 1), "2A03 sequence count");		// // //

	auto &Manager = *modfile.GetInstrumentManager();

	if (ver == 1) {
		for (unsigned int i = 0; i < Count; ++i) {
			COldSequence Seq;
			(void)AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
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
			auto Type = static_cast<sequence_t>((unsigned)AssertRange(file_.GetBlockInt(), 0, (int)SEQ_COUNT - 1, "Sequence type"));
			unsigned int SeqCount = static_cast<unsigned char>(file_.GetBlockChar());
			AssertRange(SeqCount, 0U, static_cast<unsigned>(MAX_SEQUENCE_ITEMS - 1), "Sequence item count");
			for (unsigned int j = 0; j < SeqCount; ++j) {
				char Value = file_.GetBlockChar();
				Seq.AddItem(file_.GetBlockChar(), Value);
			}
			Manager.SetSequence(INST_2A03, Type, Index, Seq.Convert(Type));		// // //
		}
	}
	else if (ver >= 3) {
		CSequenceManager *pManager = Manager.GetSequenceManager(INST_2A03);		// // //
		int Indices[MAX_SEQUENCES * SEQ_COUNT];
		sequence_t Types[MAX_SEQUENCES * SEQ_COUNT];

		for (unsigned int i = 0; i < Count; ++i) {
			unsigned int Index = Indices[i] = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
			auto Type = Types[i] = static_cast<sequence_t>((unsigned)AssertRange(file_.GetBlockInt(), 0, (int)SEQ_COUNT - 1, "Sequence type"));
			try {
				unsigned char SeqCount = file_.GetBlockChar();
				// AssertRange(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count");
				auto pSeq = std::make_unique<CSequence>(Type);
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
					int ReleasePoint = file_.GetBlockInt();
					int Settings = file_.GetBlockInt();
					auto pSeq = pManager->GetCollection(j)->GetSequence(i);
					int Length = pSeq->GetItemCount();
					if (Length > 0) {
						pSeq->SetReleasePoint(AssertRange<MODULE_ERROR_STRICT>(
							ReleasePoint, -1, Length - 1, "Sequence release point"));
						pSeq->SetSetting(static_cast<seq_setting_t>(Settings));		// // //
					}
				}
				catch (CModuleException &e) {
					e.AppendError("At 2A03 " + std::string {CInstrument2A03::SEQUENCE_NAME[value_cast(j)]} +" sequence " + conv::from_int(i) + ',');
					throw e;
				}
			}
		}
		else if (ver >= 6) {
			// Read release points correctly stored
			for (unsigned int i = 0; i < Count; ++i) try {
				auto pSeq = pManager->GetCollection(Types[i])->GetSequence(Indices[i]);
				pSeq->SetReleasePoint(AssertRange<MODULE_ERROR_STRICT>(
					file_.GetBlockInt(), -1, static_cast<int>(pSeq->GetItemCount()) - 1, "Sequence release point"));
				pSeq->SetSetting(static_cast<seq_setting_t>(file_.GetBlockInt()));		// // //
			}
			catch (CModuleException &e) {
				e.AppendError("At 2A03 " + std::string {CInstrument2A03::SEQUENCE_NAME[value_cast(Types[i])]} + " sequence " + conv::from_int(Indices[i]) + ',');
				throw e;
			}
		}
	}
}

void CFamiTrackerDocIO::SaveSequences(const CFamiTrackerModule &modfile, int ver) {
	int Count = modfile.GetInstrumentManager()->GetTotalSequenceCount(INST_2A03);
	if (!Count)
		return;		// // //
	file_.WriteBlockInt(Count);

	auto *Manager = modfile.GetInstrumentManager()->GetSequenceManager(INST_2A03);

	VisitSequences(Manager, [&] (const CSequence &seq, int index, sequence_t seqType) {
		file_.WriteBlockInt(index);
		file_.WriteBlockInt(value_cast(seqType));
		file_.WriteBlockChar(seq.GetItemCount());
		file_.WriteBlockInt(seq.GetLoopPoint());
		for (int k = 0, Count = seq.GetItemCount(); k < Count; ++k)
			file_.WriteBlockChar(seq.GetItem(k));
	});

	// v6
	VisitSequences(Manager, [&] (const CSequence &seq, int, sequence_t) {
		file_.WriteBlockInt(seq.GetReleasePoint());
		file_.WriteBlockInt(seq.GetSetting());
	});
}

void CFamiTrackerDocIO::LoadFrames(CFamiTrackerModule &modfile, int ver) {
	if (ver == 1) {
		unsigned int FrameCount = AssertRange(file_.GetBlockInt(), 1, MAX_FRAMES, "Song frame count");
		/*m_iChannelsAvailable =*/ AssertRange<MODULE_ERROR_OFFICIAL>(AssertRange(file_.GetBlockInt(),
			0, (int)CHANID_COUNT, "Track count"), 0, (int)MAX_CHANNELS, "Track count");
		auto &Song = *modfile.GetSong(0);
		Song.SetFrameCount(FrameCount);
		for (unsigned i = 0; i < FrameCount; ++i) {
			modfile.GetChannelOrder().ForeachChannel([&] (stChannelID j) {
				unsigned Pattern = static_cast<unsigned char>(file_.GetBlockChar());
				AssertRange(Pattern, 0U, static_cast<unsigned>(MAX_PATTERN - 1), "Pattern index");
				Song.SetFramePattern(i, j, Pattern);
			});
		}
	}
	else if (ver > 1) {
		modfile.VisitSongs([&] (CSongData &song) {
			unsigned int FrameCount = AssertRange(file_.GetBlockInt(), 1, MAX_FRAMES, "Song frame count");
			unsigned int Speed = AssertRange<MODULE_ERROR_STRICT>(file_.GetBlockInt(), 0, MAX_TEMPO, "Song default speed");
			song.SetFrameCount(FrameCount);

			if (ver >= 3) {
				unsigned int Tempo = AssertRange<MODULE_ERROR_STRICT>(file_.GetBlockInt(), 0, MAX_TEMPO, "Song default tempo");
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

			unsigned PatternLength = AssertRange(file_.GetBlockInt(), 1, MAX_PATTERN_LENGTH, "Song default row count");
			song.SetPatternLength(PatternLength);

			for (unsigned i = 0; i < FrameCount; ++i) {
				modfile.GetChannelOrder().ForeachChannel([&] (stChannelID j) {
					// Read pattern index
					int Pattern = static_cast<unsigned char>(file_.GetBlockChar());
					song.SetFramePattern(i, j, AssertRange(Pattern, 0, MAX_PATTERN - 1, "Pattern index"));
				});
			}
		});
	}
}

void CFamiTrackerDocIO::SaveFrames(const CFamiTrackerModule &modfile, int ver) {
	modfile.VisitSongs([&] (const CSongData &Song) {
		file_.WriteBlockInt(Song.GetFrameCount());
		file_.WriteBlockInt(Song.GetSongSpeed());
		file_.WriteBlockInt(Song.GetSongTempo());
		file_.WriteBlockInt(Song.GetPatternLength());

		for (unsigned int j = 0; j < Song.GetFrameCount(); ++j)
			modfile.GetChannelOrder().ForeachChannel([&] (stChannelID k) {
				file_.WriteBlockChar((unsigned char)Song.GetFramePattern(j, k));
			});
	});
}

void CFamiTrackerDocIO::LoadPatterns(CFamiTrackerModule &modfile, int ver) {
	fds_adjust_arps_ = ver < 5;		// // //
	bool compat200 = (file_.GetFileVersion() == 0x0200);		// // //

	if (ver == 1) {
		int PatternLen = AssertRange(file_.GetBlockInt(), 0, MAX_PATTERN_LENGTH, "Pattern data count");
		modfile.GetSong(0)->SetPatternLength(PatternLen);
	}

	const CChannelOrder &order = modfile.GetChannelOrder();		// // //

	while (!file_.BlockDone()) {
		unsigned Track = 0;
		if (ver > 1)
			Track = AssertRange(file_.GetBlockInt(), 0, static_cast<int>(MAX_TRACKS) - 1, "Pattern song index");

		unsigned Channel = AssertRange((unsigned)file_.GetBlockInt(), 0u, CHANID_COUNT - 1, "Pattern track index");
		AssertRange<MODULE_ERROR_OFFICIAL>(Channel, 0u, MAX_CHANNELS - 1, "Pattern track index");
		unsigned Pattern = AssertRange(file_.GetBlockInt(), 0, MAX_PATTERN - 1, "Pattern index");
		unsigned Items	= AssertRange(file_.GetBlockInt(), 0, MAX_PATTERN_LENGTH, "Pattern data count");
		stChannelID ch = order.TranslateChannel(Channel);

		auto *pSong = modfile.GetSong(Track);

		for (unsigned i = 0; i < Items; ++i) try {
			unsigned Row;
			if (compat200 || ver >= 6)
				Row = static_cast<unsigned char>(file_.GetBlockChar());
			else
				Row = AssertRange(file_.GetBlockInt(), 0, 0xFF, "Row index");		// // //

			try {
				stChanNote Note;		// // //

				Note.Note = enum_cast<note_t>(AssertRange<MODULE_ERROR_STRICT>(		// // //
					file_.GetBlockChar(), value_cast(note_t::none), value_cast(note_t::echo), "Note value"));
				Note.Octave = AssertRange<MODULE_ERROR_STRICT>(
					file_.GetBlockChar(), 0, OCTAVE_RANGE - 1, "Octave value");
				int Inst = static_cast<unsigned char>(file_.GetBlockChar());
				if (Inst != HOLD_INSTRUMENT)		// // // 050B
					AssertRange<MODULE_ERROR_STRICT>(Inst, 0, CInstrumentManager::MAX_INSTRUMENTS, "Instrument index");
				Note.Instrument = Inst;
				Note.Vol = AssertRange<MODULE_ERROR_STRICT>(
					file_.GetBlockChar(), 0, MAX_VOLUME, "Channel volume");

				int FX = compat200 ? 1 : ver >= 6 ? MAX_EFFECT_COLUMNS :
					pSong->GetEffectColumnCount(order.TranslateChannel(Channel));		// // // 050B
				for (int n = 0; n < FX; ++n) try {
					auto EffectNumber = (effect_t)file_.GetBlockChar();
					if (Note.Effects[n].fx = static_cast<effect_t>(EffectNumber); Note.Effects[n].fx != effect_t::NONE) {
						AssertRange<MODULE_ERROR_STRICT>(value_cast(EffectNumber), value_cast(effect_t::NONE), EFFECT_COUNT - 1, "Effect index");
						unsigned char EffectParam = file_.GetBlockChar();
						if (ver < 3) {
							if (EffectNumber == effect_t::PORTAOFF) {
								EffectNumber = effect_t::PORTAMENTO;
								EffectParam = 0;
							}
							else if (EffectNumber == effect_t::PORTAMENTO) {
								if (EffectParam < 0xFF)
									++EffectParam;
							}
						}
						Note.Effects[n].param = EffectParam; // skip on no effect
					}
					else if (ver < 6)
						file_.GetBlockChar(); // unused blank parameter
				}
				catch (CModuleException &e) {
					e.AppendError("At effect column fx" + conv::from_int(n + 1) + ',');
					throw e;
				}

	//			if (Note.Vol > MAX_VOLUME)
	//				Note.Vol &= 0x0F;

				if (compat200) {		// // //
					if (Note.Effects[0].fx == effect_t::SPEED && Note.Effects[0].param < 20)
						++Note.Effects[0].param;

					if (Note.Vol == 0)
						Note.Vol = MAX_VOLUME;
					else {
						--Note.Vol;
						Note.Vol &= 0x0F;
					}

					if (Note.Note == note_t::none)
						Note.Instrument = MAX_INSTRUMENTS;
				}

				if (modfile.GetSoundChipSet().ContainsChip(sound_chip_t::N163) && ch.Chip == sound_chip_t::N163) {		// // //
					for (auto &cmd : Note.Effects)
						if (cmd.fx == effect_t::SAMPLE_OFFSET)
							cmd.fx = effect_t::N163_WAVE_BUFFER;
				}

				if (ver == 3) {
					// Fix for VRC7 portamento
					if (ch.Chip == sound_chip_t::VRC7) {		// // //
						for (auto &cmd : Note.Effects) {
							switch (cmd.fx) {
							case effect_t::PORTA_DOWN:
								cmd.fx = effect_t::PORTA_UP;
								break;
							case effect_t::PORTA_UP:
								cmd.fx = effect_t::PORTA_DOWN;
								break;
							}
						}
					}
					// FDS pitch effect fix
					else if (ch.Chip == sound_chip_t::FDS) {
						for (auto &[fx, param] : Note.Effects)
							if (fx == effect_t::PITCH && param != 0x80)
								param = (0x100 - param) & 0xFF;
					}
				}

				if (file_.GetFileVersion() < 0x450) {		// // // 050B
					for (auto &cmd : Note.Effects)
						if (cmd.fx < effect_t::COUNT)
							cmd.fx = compat::EFF_CONVERSION_050.first[value_cast(cmd.fx)];
				}
				/*
				if (ver < 6) {
					// Noise pitch slide fix
					if (IsAPUNoise(Channel)) {
						for (int n = 0; n < MAX_EFFECT_COLUMNS; ++n) {
							switch (Note.Effects[n].fx) {
								case effect_t::PORTA_DOWN:
									Note.Effects[n].fx = effect_t::PORTA_UP;
									Note.Effects[n].param = Note.Effects[n].param << 4;
									break;
								case effect_t::PORTA_UP:
									Note.Effects[n].fx = effect_t::PORTA_DOWN;
									Note.Effects[n].param = Note.Effects[n].param << 4;
									break;
								case effect_t::PORTAMENTO:
									Note.Effects[n].param = Note.Effects[n].param << 4;
									break;
								case effect_t::SLIDE_UP:
									Note.Effects[n].param = Note.Effects[n].param + 0x70;
									break;
								case effect_t::SLIDE_DOWN:
									Note.Effects[n].param = Note.Effects[n].param + 0x70;
									break;
							}
						}
					}
				}
				*/

				pSong->SetPatternData(order.TranslateChannel(Channel), Pattern, Row, Note);		// // //
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

void CFamiTrackerDocIO::SavePatterns(const CFamiTrackerModule &modfile, int ver) {
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
			file_.WriteBlockInt(song);		// Write track
			file_.WriteBlockInt(modfile.GetChannelOrder().GetChannelIndex(ch));		// Write channel
			file_.WriteBlockInt(index);		// Write pattern
			file_.WriteBlockInt(Items);		// Number of items

			pattern.VisitRows(PatternLen, [&] (const stChanNote &note, unsigned row) {
				if (note == stChanNote { })
					return;
				file_.WriteBlockInt(row);
				file_.WriteBlockChar(value_cast(note.Note));
				file_.WriteBlockChar(note.Octave);
				file_.WriteBlockChar(note.Instrument);
				file_.WriteBlockChar(note.Vol);
				for (int i = 0, EffColumns = x.GetEffectColumnCount(ch); i < EffColumns; ++i) {
					file_.WriteBlockChar(value_cast(compat::EFF_CONVERSION_050.second[value_cast(note.Effects[i].fx)]));		// // // 050B
					file_.WriteBlockChar(note.Effects[i].param);
				}
			});
		});
	});
}

void CFamiTrackerDocIO::LoadDSamples(CFamiTrackerModule &modfile, int ver) {
	unsigned int Count = AssertRange(
		static_cast<unsigned char>(file_.GetBlockChar()), 0U, CDSampleManager::MAX_DSAMPLES, "DPCM sample count");

	auto &manager = *modfile.GetInstrumentManager()->GetDSampleManager();

	for (unsigned int i = 0; i < Count; ++i) {
		unsigned int Index = AssertRange(
			static_cast<unsigned char>(file_.GetBlockChar()), 0U, CDSampleManager::MAX_DSAMPLES - 1, "DPCM sample index");
		try {
			unsigned int Len = AssertRange(file_.GetBlockInt(), 0, (int)ft0cc::doc::dpcm_sample::max_name_length, "DPCM sample name length");
			char Name[ft0cc::doc::dpcm_sample::max_name_length + 1] = { };
			file_.GetBlock(Name, Len);
			int Size = AssertRange(file_.GetBlockInt(), 0, 0x7FFF, "DPCM sample size");
			AssertFileData<MODULE_ERROR_STRICT>(Size <= 0xFF1 && Size % 0x10 == 1, "Bad DPCM sample size");
			int TrueSize = Size + ((1 - Size) & 0x0F);		// // //
			std::vector<uint8_t> samples(TrueSize);
			file_.GetBlock(samples.data(), Size);

			manager.SetDSample(Index, std::make_unique<ft0cc::doc::dpcm_sample>(samples, Name));		// // //
		}
		catch (CModuleException &e) {
			e.AppendError("At DPCM sample " + conv::from_int(Index) + ',');
			throw e;
		}
	}
}

void CFamiTrackerDocIO::SaveDSamples(const CFamiTrackerModule &modfile, int ver) {
	const auto &manager = *modfile.GetInstrumentManager()->GetDSampleManager();
	if (int Count = manager.GetDSampleCount()) {		// // //
		// Write sample count
		file_.WriteBlockChar(Count);

		for (unsigned int i = 0; i < CDSampleManager::MAX_DSAMPLES; ++i) {
			if (auto pSamp = manager.GetDSample(i)) {
				// Write sample
				file_.WriteBlockChar(i);
				file_.WriteStringCounted(pSamp->name());
				file_.WriteBlockInt(pSamp->size());
				file_.WriteBlock(*pSamp);
			}
		}
	}
}

void CFamiTrackerDocIO::LoadComments(CFamiTrackerModule &modfile, int ver) {
	bool disp = file_.GetBlockInt() == 1;
	modfile.SetComment(file_.ReadString(), disp);
}

void CFamiTrackerDocIO::SaveComments(const CFamiTrackerModule &modfile, int ver) {
	if (auto str = modfile.GetComment(); !str.empty()) {
		file_.WriteBlockInt(modfile.ShowsCommentOnOpen() ? 1 : 0);
		file_.WriteString(str);
	}
}

void CFamiTrackerDocIO::LoadSequencesVRC6(CFamiTrackerModule &modfile, int ver) {
	unsigned int Count = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES * (int)SEQ_COUNT, "VRC6 sequence count");
	AssertRange<MODULE_ERROR_OFFICIAL>(Count, 0U, static_cast<unsigned>(MAX_SEQUENCES), "VRC6 sequence count");		// // //

	CSequenceManager *pManager = modfile.GetInstrumentManager()->GetSequenceManager(INST_VRC6);		// // //

	int Indices[MAX_SEQUENCES * SEQ_COUNT];
	sequence_t Types[MAX_SEQUENCES * SEQ_COUNT];
	for (unsigned int i = 0; i < Count; ++i) {
		unsigned int Index = Indices[i] = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
		sequence_t Type = Types[i] = static_cast<sequence_t>((unsigned)AssertRange(file_.GetBlockInt(), 0, (int)SEQ_COUNT - 1, "Sequence type"));
		try {
			unsigned char SeqCount = file_.GetBlockChar();
			auto pSeq = pManager->GetCollection(Type)->GetSequence(Index);
			pSeq->Clear();
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
		catch (CModuleException &e) {
			e.AppendError("At VRC6 " + std::string {CInstrumentVRC6::SEQUENCE_NAME[value_cast(Type)]} + " sequence " + conv::from_int(Index) + ',');
			throw e;
		}
	}

	if (ver == 5) {
		// Version 5 saved the release points incorrectly, this is fixed in ver 6
		for (int i = 0; i < MAX_SEQUENCES; ++i) {
			for (auto j : enum_values<sequence_t>()) try {
				int ReleasePoint = file_.GetBlockInt();
				int Settings = file_.GetBlockInt();
				auto pSeq = pManager->GetCollection(j)->GetSequence(i);
				int Length = pSeq->GetItemCount();
				if (Length > 0) {
					pSeq->SetReleasePoint(AssertRange<MODULE_ERROR_STRICT>(
						ReleasePoint, -1, Length - 1, "Sequence release point"));
					pSeq->SetSetting(static_cast<seq_setting_t>(Settings));		// // //
				}
			}
			catch (CModuleException &e) {
				e.AppendError("At VRC6 " + std::string {CInstrumentVRC6::SEQUENCE_NAME[value_cast(j)]} + " sequence " + conv::from_int(i) + ',');
				throw e;
			}
		}
	}
	else if (ver >= 6) {
		for (unsigned int i = 0; i < Count; ++i) try {
			auto pSeq = pManager->GetCollection(Types[i])->GetSequence(Indices[i]);
			pSeq->SetReleasePoint(AssertRange<MODULE_ERROR_STRICT>(
				file_.GetBlockInt(), -1, static_cast<int>(pSeq->GetItemCount()) - 1, "Sequence release point"));
			pSeq->SetSetting(static_cast<seq_setting_t>(file_.GetBlockInt()));		// // //
		}
		catch (CModuleException &e) {
			e.AppendError("At VRC6 " + std::string {CInstrumentVRC6::SEQUENCE_NAME[value_cast(Types[i])]} + " sequence " + conv::from_int(Indices[i]) + ',');
			throw e;
		}
	}
}

void CFamiTrackerDocIO::SaveSequencesVRC6(const CFamiTrackerModule &modfile, int ver) {
	int Count = modfile.GetInstrumentManager()->GetTotalSequenceCount(INST_VRC6);
	if (!Count)
		return;		// // //
	file_.WriteBlockInt(Count);

	auto *pManager = modfile.GetInstrumentManager()->GetSequenceManager(INST_VRC6);

	VisitSequences(pManager, [&] (const CSequence &seq, int index, sequence_t seqType) {
		file_.WriteBlockInt(index);
		file_.WriteBlockInt(value_cast(seqType));
		file_.WriteBlockChar(seq.GetItemCount());
		file_.WriteBlockInt(seq.GetLoopPoint());
		for (int k = 0, Count = seq.GetItemCount(); k < Count; ++k)
			file_.WriteBlockChar(seq.GetItem(k));
	});

	// v6
	VisitSequences(pManager, [&] (const CSequence &seq, int, sequence_t) {
		file_.WriteBlockInt(seq.GetReleasePoint());
		file_.WriteBlockInt(seq.GetSetting());
	});
}

void CFamiTrackerDocIO::LoadSequencesN163(CFamiTrackerModule &modfile, int ver) {
	unsigned int Count = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES * (int)SEQ_COUNT, "N163 sequence count");
	AssertRange<MODULE_ERROR_OFFICIAL>(Count, 0U, static_cast<unsigned>(MAX_SEQUENCES * SEQ_COUNT - 1), "N163 sequence count");		// // //

	CSequenceManager *pManager = modfile.GetInstrumentManager()->GetSequenceManager(INST_N163);		// // //

	for (unsigned int i = 0; i < Count; ++i) {
		unsigned int  Index		   = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
		unsigned int  Type		   = AssertRange(file_.GetBlockInt(), 0, (int)SEQ_COUNT - 1, "Sequence type");
		try {
			unsigned char SeqCount = file_.GetBlockChar();
			auto pSeq = pManager->GetCollection(static_cast<sequence_t>(Type))->GetSequence(Index);
			pSeq->Clear();
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
		catch (CModuleException &e) {
			e.AppendError("At N163 " + std::string {CInstrumentN163::SEQUENCE_NAME[Type]} + " sequence " + conv::from_int(Index) + ',');
			throw e;
		}
	}
}

void CFamiTrackerDocIO::SaveSequencesN163(const CFamiTrackerModule &modfile, int ver) {
	/*
	 * Store N163 sequences
	 */

	auto *pManager = modfile.GetInstrumentManager();
	int Count = pManager->GetTotalSequenceCount(INST_N163);
	if (!Count)
		return;		// // //
	file_.WriteBlockInt(Count);

	VisitSequences(pManager->GetSequenceManager(INST_N163), [&] (const CSequence &seq, int index, sequence_t seqType) {
		file_.WriteBlockInt(index);
		file_.WriteBlockInt(value_cast(seqType));
		file_.WriteBlockChar(seq.GetItemCount());
		file_.WriteBlockInt(seq.GetLoopPoint());
		file_.WriteBlockInt(seq.GetReleasePoint());
		file_.WriteBlockInt(seq.GetSetting());
		for (int k = 0, Count = seq.GetItemCount(); k < Count; ++k)
			file_.WriteBlockChar(seq.GetItem(k));
	});
}

void CFamiTrackerDocIO::LoadSequencesS5B(CFamiTrackerModule &modfile, int ver) {
	unsigned int Count = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES * (int)SEQ_COUNT, "5B sequence count");
	AssertRange<MODULE_ERROR_OFFICIAL>(Count, 0U, static_cast<unsigned>(MAX_SEQUENCES * SEQ_COUNT - 1), "5B sequence count");		// // //

	CSequenceManager *pManager = modfile.GetInstrumentManager()->GetSequenceManager(INST_S5B);		// // //

	for (unsigned int i = 0; i < Count; ++i) {
		unsigned int  Index		   = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
		unsigned int  Type		   = AssertRange(file_.GetBlockInt(), 0, (int)SEQ_COUNT - 1, "Sequence type");
		try {
			unsigned char SeqCount = file_.GetBlockChar();
			auto pSeq = pManager->GetCollection(static_cast<sequence_t>(Type))->GetSequence(Index);
			pSeq->Clear();
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
		catch (CModuleException &e) {
			e.AppendError("At 5B " + std::string {CInstrumentS5B::SEQUENCE_NAME[Type]} + " sequence " + conv::from_int(Index) + ',');
			throw e;
		}
	}
}

void CFamiTrackerDocIO::SaveSequencesS5B(const CFamiTrackerModule &modfile, int ver) {
	auto *pManager = modfile.GetInstrumentManager();
	int Count = pManager->GetTotalSequenceCount(INST_S5B);
	if (!Count)
		return;		// // //
	file_.WriteBlockInt(Count);

	VisitSequences(pManager->GetSequenceManager(INST_S5B), [&] (const CSequence &seq, int index, sequence_t seqType) {
		file_.WriteBlockInt(index);
		file_.WriteBlockInt(value_cast(seqType));
		file_.WriteBlockChar(seq.GetItemCount());
		file_.WriteBlockInt(seq.GetLoopPoint());
		file_.WriteBlockInt(seq.GetReleasePoint());
		file_.WriteBlockInt(seq.GetSetting());
		for (int k = 0, Count = seq.GetItemCount(); k < Count; ++k)
			file_.WriteBlockChar(seq.GetItem(k));
	});
}

void CFamiTrackerDocIO::LoadParamsExtra(CFamiTrackerModule &modfile, int ver) {
	modfile.SetLinearPitch(file_.GetBlockInt() != 0);
	if (ver >= 2) {
		int semitone = AssertRange(file_.GetBlockChar(), -12, 12, "Global semitone tuning");
		int cent = AssertRange(file_.GetBlockChar(), -100, 100, "Global cent tuning");
		modfile.SetTuning(semitone, cent);
	}
}

void CFamiTrackerDocIO::SaveParamsExtra(const CFamiTrackerModule &modfile, int ver) {
	bool linear = modfile.GetLinearPitch();
	char semitone = modfile.GetTuningSemitone();
	char cent = modfile.GetTuningCent();
	if (linear || semitone || cent) {
		file_.WriteBlockInt(linear);
		if (ver >= 2) {
			file_.WriteBlockChar(semitone);
			file_.WriteBlockChar(cent);
		}
	}
}

// #include "DetuneDlg.h" // TODO: bad, encapsulate detune tables

void CFamiTrackerDocIO::LoadDetuneTables(CFamiTrackerModule &modfile, int ver) {
	int Count = AssertRange(file_.GetBlockChar(), 0, 6, "Detune table count");
	for (int i = 0; i < Count; ++i) {
		int Chip = AssertRange(file_.GetBlockChar(), 0, 5, "Detune table index");
		try {
			int Item = AssertRange(file_.GetBlockChar(), 0, NOTE_COUNT, "Detune table note count");
			for (int j = 0; j < Item; ++j) {
				int Note = AssertRange(file_.GetBlockChar(), 0, NOTE_COUNT - 1, "Detune table note index");
				int Offset = file_.GetBlockInt();
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

void CFamiTrackerDocIO::SaveDetuneTables(const CFamiTrackerModule &modfile, int ver) {
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

	file_.WriteBlockChar(ChipCount);
	for (int i = 0; i < 6; ++i) {
		if (!NoteUsed[i])
			continue;
		file_.WriteBlockChar(i);
		file_.WriteBlockChar(NoteUsed[i]);
		for (int j = 0; j < NOTE_COUNT; ++j)
			if (int detune = modfile.GetDetuneOffset(i, j)) {
				file_.WriteBlockChar(j);
				file_.WriteBlockInt(detune);
			}
	}
}

void CFamiTrackerDocIO::LoadGrooves(CFamiTrackerModule &modfile, int ver) {
	const int Count = AssertRange(file_.GetBlockChar(), 0, MAX_GROOVE, "Groove count");

	for (int i = 0; i < Count; ++i) {
		int Index = AssertRange(file_.GetBlockChar(), 0, MAX_GROOVE - 1, "Groove index");
		try {
			std::size_t Size = AssertRange((uint8_t)file_.GetBlockChar(), 1u, ft0cc::doc::groove::max_size, "Groove size");
			auto pGroove = std::make_unique<ft0cc::doc::groove>();
			pGroove->resize(Size);
			for (std::size_t j = 0; j < Size; ++j) try {
				pGroove->set_entry(j, AssertRange(
					static_cast<unsigned char>(file_.GetBlockChar()), 1U, 0xFFU, "Groove item"));
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

	unsigned int Tracks = file_.GetBlockChar();
	AssertFileData<MODULE_ERROR_STRICT>(Tracks == modfile.GetSongCount(), "Use-groove flag count does not match song count");
	for (unsigned i = 0; i < Tracks; ++i) try {
		bool Use = file_.GetBlockChar() == 1;
		if (i >= modfile.GetSongCount())
			continue;
		int Speed = modfile.GetSong(i)->GetSongSpeed();
		modfile.GetSong(i)->SetSongGroove(Use);
		if (Use)
			AssertRange(Speed, 0, MAX_GROOVE - 1, "Song default groove index");
		else
			AssertRange(Speed, 1, MAX_TEMPO, "Song default speed");
	}
	catch (CModuleException &e) {
		e.AppendError("At song " + conv::from_int(i + 1) + ',');
		throw e;
	}
}

void CFamiTrackerDocIO::SaveGrooves(const CFamiTrackerModule &modfile, int ver) {
	int Count = 0;
	for (int i = 0; i < MAX_GROOVE; ++i)
		if (modfile.HasGroove(i))
			++Count;
	if (!Count)
		return;

	file_.WriteBlockChar(Count);
	for (int i = 0; i < MAX_GROOVE; ++i)
		if (const auto pGroove = modfile.GetGroove(i)) {
			int Size = pGroove->size();
			file_.WriteBlockChar(i);
			file_.WriteBlockChar(Size);
			for (int j = 0; j < Size; ++j)
				file_.WriteBlockChar(pGroove->entry(j));
		}

	file_.WriteBlockChar((unsigned char)modfile.GetSongCount());
	modfile.VisitSongs([&] (const CSongData &song) { file_.WriteBlockChar(song.GetSongGroove()); });
}

void CFamiTrackerDocIO::LoadBookmarks(CFamiTrackerModule &modfile, int ver) {
	for (int i = 0, n = file_.GetBlockInt(); i < n; ++i) {
		auto pMark = std::make_unique<CBookmark>();
		unsigned int Track = AssertRange(
			static_cast<unsigned char>(file_.GetBlockChar()), 0, modfile.GetSongCount() - 1, "Bookmark song index");
		int Frame = static_cast<unsigned char>(file_.GetBlockChar());
		int Row = static_cast<unsigned char>(file_.GetBlockChar());

		auto *pSong = modfile.GetSong(Track);
		pMark->m_iFrame = AssertRange(Frame, 0, static_cast<int>(pSong->GetFrameCount()) - 1, "Bookmark frame index");
		pMark->m_iRow = AssertRange(Row, 0, static_cast<int>(pSong->GetPatternLength()) - 1, "Bookmark row index");
		pMark->m_Highlight.First = file_.GetBlockInt();
		pMark->m_Highlight.Second = file_.GetBlockInt();
		pMark->m_bPersist = file_.GetBlockChar() != 0;
		pMark->m_sName = file_.ReadString();
		pSong->GetBookmarks().AddBookmark(std::move(pMark));
	}
}

void CFamiTrackerDocIO::SaveBookmarks(const CFamiTrackerModule &modfile, int ver) {
	int Count = 0;
	modfile.VisitSongs([&] (const CSongData &song) {
		Count += song.GetBookmarks().GetCount();
	});
	if (!Count)
		return;
	file_.WriteBlockInt(Count);

	modfile.VisitSongs([&] (const CSongData &song, unsigned index) {
		for (const auto &pMark : song.GetBookmarks()) {
			file_.WriteBlockChar(index);
			file_.WriteBlockChar(pMark->m_iFrame);
			file_.WriteBlockChar(pMark->m_iRow);
			file_.WriteBlockInt(pMark->m_Highlight.First);
			file_.WriteBlockInt(pMark->m_Highlight.Second);
			file_.WriteBlockChar(pMark->m_bPersist);
			file_.WriteString(pMark->m_sName);
		}
	});
}

template <module_error_level_t l>
void CFamiTrackerDocIO::AssertFileData(bool Cond, const std::string &Msg) const {
	if (l <= err_lv_ && !Cond) {
		CModuleException e = file_.GetException();
		e.AppendError(Msg);
		throw e;
	}
}

template<module_error_level_t l, typename T, typename U, typename V>
T CFamiTrackerDocIO::AssertRange(T Value, U Min, V Max, const std::string &Desc) const {
	if (l <= err_lv_ && !(Value >= Min && Value <= Max)) {
		std::string msg = Desc + " out of range: expected ["
			+ std::to_string(Min) + ','
			+ std::to_string(Max) + "], got "
			+ std::to_string(Value);
		auto e = CModuleException::WithMessage(msg);
		file_.SetDefaultFooter(e);
		throw e;
	}
	return Value;
}
