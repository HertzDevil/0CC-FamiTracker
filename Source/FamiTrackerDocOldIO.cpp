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

#include "FamiTrackerDocOldIO.h"
#include "FamiTrackerDocIOCommon.h"
#include "FamiTrackerEnv.h"
#include "SoundChipService.h"
#include "ChannelMap.h"
#include "ChannelOrder.h"
#include "SongData.h"
#include "InstrumentManager.h"
#include "DSampleManager.h"
#include <memory>
#include <array>
#include <algorithm>
#include "Instrument2A03.h"
#include "ft0cc/doc/dpcm_sample.hpp"
#include "Sequence.h"
#include "FamiTrackerModule.h"
#include "APU/Types.h"
#include "SoundChipSet.h"
#include "SimpleFile.h"

namespace {

enum {
	FB_INSTRUMENTS,
	FB_SEQUENCES,
	FB_PATTERN_ROWS,
	FB_PATTERNS,
	FB_SPEED,
	FB_CHANNELS,
	FB_DSAMPLES,
	FB_EOF,
	FB_MACHINE,
	FB_ENGINESPEED,
	FB_SONGNAME,
	FB_SONGARTIST,
	FB_SONGCOPYRIGHT,
};

} // namespace

bool compat::OpenDocumentOld(CFamiTrackerModule &modfile, CSimpleFile &OpenFile) {
	unsigned int i, c, ReadCount, FileBlock;

	FileBlock = 0;

	// Only single track files
	auto &Song = *modfile.GetSong(0);

	modfile.SetChannelMap(FTEnv.GetSoundChipService()->MakeChannelMap(sound_chip_t::APU, 0));		// // //
	modfile.SetMachine(machine_t::NTSC);		// // //
	modfile.SetVibratoStyle(vibrato_t::Up);
	modfile.SetLinearPitch(false);

	// // // local structs
	struct {
		char Name[256];
		bool Free;
		int	 ModEnable[SEQ_COUNT];
		int	 ModIndex[SEQ_COUNT];
		int	 AssignedSample;				// For DPCM
	} ImportedInstruments;
	struct {
		char Length[64];
		char Value[64];
		unsigned int Count;
	} ImportedSequence;
	struct {
		int	Note;
		int	Octave;
		int	Vol;
		int	Instrument;
		int	ExtraStuff1;
		int	ExtraStuff2;
	} ImportedNote;

	std::vector<COldSequence> TmpSequences;		// // //

	while (FileBlock != FB_EOF) {
		if (OpenFile.ReadBytes(byte_view(FileBlock)) == 0)
			FileBlock = FB_EOF;

		char pBuf[CFamiTrackerModule::METADATA_FIELD_LENGTH] = { };		// // //

		switch (FileBlock) {
		case FB_CHANNELS:
			break;		// // //

		case FB_SPEED:
			Song.SetSongSpeed(OpenFile.ReadInt32() + 1);
			break;

		case FB_MACHINE:
			modfile.SetMachine(OpenFile.ReadInt32() ? machine_t::PAL : machine_t::NTSC);
			break;

		case FB_ENGINESPEED:
			modfile.SetEngineSpeed(OpenFile.ReadInt32());
			break;

		case FB_INSTRUMENTS:
			ReadCount = OpenFile.ReadInt32();
			if (ReadCount > MAX_INSTRUMENTS)
				ReadCount = MAX_INSTRUMENTS - 1;
			for (i = 0; i < ReadCount; ++i) {
				OpenFile.ReadBytes(byte_view(ImportedInstruments));
				if (ImportedInstruments.Free == false) {
					auto pInst = std::make_unique<CInstrument2A03>();
					for (auto j : enum_values<sequence_t>()) {
						pInst->SetSeqEnable(j, ImportedInstruments.ModEnable[value_cast(j)]);
						pInst->SetSeqIndex(j, ImportedInstruments.ModIndex[value_cast(j)]);
					}
					pInst->SetName(ImportedInstruments.Name);

					if (ImportedInstruments.AssignedSample > 0) {
						int Pitch = 0;
						for (int n = 0; n < 72; ++n) {		// // //
							pInst->SetSampleIndex(n, ImportedInstruments.AssignedSample - 1);
							pInst->SetSamplePitch(n, Pitch);
							Pitch = (Pitch + 1) % 16;
						}
					}

					modfile.GetInstrumentManager()->InsertInstrument(i, std::move(pInst));		// // //
				}
			}
			break;

		case FB_SEQUENCES:
			ReadCount = OpenFile.ReadInt32();
			for (i = 0; i < ReadCount; ++i) {
				COldSequence Seq;
				OpenFile.ReadBytes(byte_view(ImportedSequence));
				if (ImportedSequence.Count > 0 && ImportedSequence.Count < MAX_SEQUENCE_ITEMS)
					for (unsigned int j = 0; j < ImportedSequence.Count; ++j)		// // //
						Seq.AddItem(ImportedSequence.Length[j], ImportedSequence.Value[j]);
				TmpSequences.push_back(Seq);		// // //
			}
			break;

		case FB_PATTERN_ROWS: {
			unsigned FrameCount = OpenFile.ReadInt32();
			Song.SetFrameCount(FrameCount);
			for (c = 0; c < FrameCount; ++c)
				modfile.GetChannelOrder().ForeachChannel([&] (stChannelID i) {
					Song.SetFramePattern(c, i, OpenFile.ReadInt32());
				});
			break;
		}
		case FB_PATTERNS: {
			ReadCount = OpenFile.ReadInt32();
			unsigned PatternLength = OpenFile.ReadInt32();
			Song.SetPatternLength(PatternLength);
			modfile.GetChannelOrder().ForeachChannel([&] (stChannelID x) {
				for (c = 0; c < ReadCount; ++c) {
					for (i = 0; i < PatternLength; ++i) {
						OpenFile.ReadBytes(byte_view(ImportedNote));
						if (ImportedNote.ExtraStuff1 == (int)ft0cc::doc::effect_type::PORTAOFF) {
							ImportedNote.ExtraStuff1 = (int)ft0cc::doc::effect_type::PORTAMENTO;
							ImportedNote.ExtraStuff2 = 0;
						}
						else if (ImportedNote.ExtraStuff1 == (int)ft0cc::doc::effect_type::PORTAMENTO) {
							if (ImportedNote.ExtraStuff2 < 0xFF)
								++ImportedNote.ExtraStuff2;
						}
						ft0cc::doc::pattern_note Note;		// // //
						Note.set_fx_cmd(0, {static_cast<ft0cc::doc::effect_type>(ImportedNote.ExtraStuff1), static_cast<uint8_t>(ImportedNote.ExtraStuff2)});
						Note.set_inst(ImportedNote.Instrument);
						Note.set_note(enum_cast<ft0cc::doc::pitch>(ImportedNote.Note));
						Note.set_oct(ImportedNote.Octave);
						Note.set_vol(0);
						if (Note.note() == ft0cc::doc::pitch::none)
							Note.set_inst(MAX_INSTRUMENTS);
						if (Note.vol() == 0)
							Note.set_vol(MAX_VOLUME);
						if (Note.fx_name(0) <= ft0cc::doc::effect_type::max)		// // //
							Note.set_fx_name(0, EFF_CONVERSION_050.first[value_cast(Note.fx_name(0))]);
						Song.GetPattern(x, c).SetNoteOn(i, Note);
					}
				}
			});
			break;
		}
		case FB_DSAMPLES: {
			struct {
				char *SampleData; // TODO: verify
				int	 SampleSize;
				char Name[256];
			} ImportedDSample;

			ReadCount = OpenFile.ReadInt32();
			for (i = 0; i < ReadCount; ++i) {
				std::vector<uint8_t> Sample;		// // //
				OpenFile.ReadBytes(byte_view(ImportedDSample));
				if (ImportedDSample.SampleSize != 0 && ImportedDSample.SampleSize < 0x4000) {
					Sample.resize(ImportedDSample.SampleSize);		// // //
					OpenFile.ReadBytes(byte_view(Sample));
				}

				modfile.GetDSampleManager()->SetDSample(i, std::make_shared<ft0cc::doc::dpcm_sample>(std::move(Sample), ImportedDSample.Name));
			}
			break;
		}
		case FB_SONGNAME:
			OpenFile.ReadBytes(byte_view(pBuf));		// // //
			modfile.SetModuleName(pBuf);
			break;

		case FB_SONGARTIST:
			OpenFile.ReadBytes(byte_view(pBuf));		// // //
			modfile.SetModuleArtist(pBuf);
			break;

		case FB_SONGCOPYRIGHT:
			OpenFile.ReadBytes(byte_view(pBuf));		// // //
			modfile.SetModuleCopyright(pBuf);
			break;

		default:
			FileBlock = FB_EOF;
		}
	}

	ReorderSequences(modfile, std::move(TmpSequences));		// // //

	return true;
}

void compat::ReorderSequences(CFamiTrackerModule &modfile, std::vector<COldSequence> seqs)		// // //
{
	int Slots[SEQ_COUNT] = {0, 0, 0, 0, 0};
	std::array<std::array<int, SEQ_COUNT>, MAX_SEQUENCES> Indices;
	for (auto &x : Indices)
		x.fill(-1);

	auto &Manager = *modfile.GetInstrumentManager();

	// Organize sequences
	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (auto pInst = std::dynamic_pointer_cast<CInstrument2A03>(Manager.GetInstrument(i))) {		// // //
			for (auto j : enum_values<sequence_t>()) {
				auto s = value_cast(j);
				if (pInst->GetSeqEnable(j)) {
					int Index = pInst->GetSeqIndex(j);
					if (Indices[Index][s] >= 0 && Indices[Index][s] != -1) {
						pInst->SetSeqIndex(j, Indices[Index][s]);
					}
					else {
						COldSequence &Seq = seqs[Index];		// // //
						if (j == sequence_t::Volume)
							for (unsigned int k = 0; k < Seq.GetLength(); ++k)
								Seq.Value[k] = std::clamp(Seq.Value[k], (char)0, (char)15);
						else if (j == sequence_t::DutyCycle)
							for (unsigned int k = 0; k < Seq.GetLength(); ++k)
								Seq.Value[k] = std::clamp(Seq.Value[k], (char)0, (char)3);
						Indices[Index][s] = Slots[s];
						pInst->SetSeqIndex(j, Slots[s]);
						Manager.SetSequence(INST_2A03, j, Slots[s]++, Seq.Convert(j));
					}
				}
				else
					pInst->SetSeqIndex(j, 0);
			}
		}
	}
}
