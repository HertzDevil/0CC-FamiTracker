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
#include "SoundGen.h"
#include "ChannelMap.h"
#include "ChannelOrder.h"
#include "SongData.h"
#include "InstrumentManager.h"
#include "DSampleManager.h"
#include <memory>
#include <array>
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

	modfile.SetChannelMap(Env.GetSoundGenerator()->MakeChannelMap(sound_chip_t::APU, 0));		// // //
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
		if (OpenFile.ReadBytes(&FileBlock, sizeof(int)) == 0)
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
				OpenFile.ReadBytes(&ImportedInstruments, sizeof(ImportedInstruments));
				if (ImportedInstruments.Free == false) {
					auto pInst = std::make_unique<CInstrument2A03>();
					foreachSeq([&] (sequence_t j) {
						pInst->SetSeqEnable(j, ImportedInstruments.ModEnable[value_cast(j)]);
						pInst->SetSeqIndex(j, ImportedInstruments.ModIndex[value_cast(j)]);
					});
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
				OpenFile.ReadBytes(&ImportedSequence, sizeof(ImportedSequence));
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
						OpenFile.ReadBytes(&ImportedNote, sizeof(ImportedNote));
						if (ImportedNote.ExtraStuff1 == (int)effect_t::PORTAOFF) {
							ImportedNote.ExtraStuff1 = (int)effect_t::PORTAMENTO;
							ImportedNote.ExtraStuff2 = 0;
						}
						else if (ImportedNote.ExtraStuff1 == (int)effect_t::PORTAMENTO) {
							if (ImportedNote.ExtraStuff2 < 0xFF)
								++ImportedNote.ExtraStuff2;
						}
						stChanNote Note;		// // //
						Note.Effects[0].fx = static_cast<effect_t>(ImportedNote.ExtraStuff1);
						Note.Effects[0].param = ImportedNote.ExtraStuff2;
						Note.Instrument = ImportedNote.Instrument;
						Note.Note = static_cast<note_t>(ImportedNote.Note);
						Note.Octave = ImportedNote.Octave;
						Note.Vol = 0;
						if (Note.Note == note_t::NONE)
							Note.Instrument = MAX_INSTRUMENTS;
						if (Note.Vol == 0)
							Note.Vol = MAX_VOLUME;
						if (Note.Effects[0].fx < effect_t::COUNT)		// // //
							Note.Effects[0].fx = EFF_CONVERSION_050.first[value_cast(Note.Effects[0].fx)];
						Song.SetPatternData(x, c, i, Note);
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
				OpenFile.ReadBytes(&ImportedDSample, sizeof(ImportedDSample));
				if (ImportedDSample.SampleSize != 0 && ImportedDSample.SampleSize < 0x4000) {
					Sample.resize(ImportedDSample.SampleSize);		// // //
					OpenFile.ReadBytes(Sample.data(), ImportedDSample.SampleSize);
				}

				modfile.GetDSampleManager()->SetDSample(i, std::make_shared<ft0cc::doc::dpcm_sample>(std::move(Sample), ImportedDSample.Name));
			}
			break;
		}
		case FB_SONGNAME:
			OpenFile.ReadBytes(pBuf, std::size(pBuf));		// // //
			modfile.SetModuleName(pBuf);
			break;

		case FB_SONGARTIST:
			OpenFile.ReadBytes(pBuf, std::size(pBuf));
			modfile.SetModuleArtist(pBuf);
			break;

		case FB_SONGCOPYRIGHT:
			OpenFile.ReadBytes(pBuf, std::size(pBuf));
			modfile.SetModuleCopyright(pBuf);
			break;

		default:
			FileBlock = FB_EOF;
		}
	}

	ReorderSequences(modfile, std::move(TmpSequences));		// // //

	return TRUE;
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
			foreachSeq([&] (sequence_t j) {
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
			});
		}
	}
}
