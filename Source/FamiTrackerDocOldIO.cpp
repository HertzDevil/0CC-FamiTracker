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
	FB_SONGCOPYRIGHT
};

} // namespace

bool compat::OpenDocumentOld(CFamiTrackerModule &modfile, CFile *pOpenFile) {
	unsigned int i, c, ReadCount, FileBlock;

	FileBlock = 0;

	// Only single track files
	auto &Song = *modfile.GetSong(0);

	modfile.SetChannelMap(Env.GetSoundGenerator()->MakeChannelMap(sound_chip_t::APU, 0));		// // //
	modfile.SetMachine(NTSC);		// // //
	modfile.SetVibratoStyle(VIBRATO_OLD);
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

	const auto ReadInt = [] (CFile *pFile) {
		int x;
		pFile->Read(&x, sizeof(x));
		return x;
	};

	std::vector<COldSequence> TmpSequences;		// // //

	while (FileBlock != FB_EOF) {
		if (pOpenFile->Read(&FileBlock, sizeof(int)) == 0)
			FileBlock = FB_EOF;

		char pBuf[CFamiTrackerModule::METADATA_FIELD_LENGTH] = { };		// // //

		switch (FileBlock) {
		case FB_CHANNELS:
			break;		// // //

		case FB_SPEED:
			Song.SetSongSpeed(ReadInt(pOpenFile) + 1);
			break;

		case FB_MACHINE:
			modfile.SetMachine(ReadInt(pOpenFile) ? PAL : NTSC);
			break;

		case FB_ENGINESPEED:
			modfile.SetEngineSpeed(ReadInt(pOpenFile));
			break;

		case FB_INSTRUMENTS:
			ReadCount = ReadInt(pOpenFile);
			if (ReadCount > MAX_INSTRUMENTS)
				ReadCount = MAX_INSTRUMENTS - 1;
			for (i = 0; i < ReadCount; i++) {
				pOpenFile->Read(&ImportedInstruments, sizeof(ImportedInstruments));
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
							pInst->SetSampleIndex(n, ImportedInstruments.AssignedSample);
							pInst->SetSamplePitch(n, Pitch);
							Pitch = (Pitch + 1) % 16;
						}
					}

					modfile.GetInstrumentManager()->InsertInstrument(i, std::move(pInst));		// // //
				}
			}
			break;

		case FB_SEQUENCES:
			pOpenFile->Read(&ReadCount, sizeof(int));
			for (i = 0; i < ReadCount; i++) {
				COldSequence Seq;
				pOpenFile->Read(&ImportedSequence, sizeof(ImportedSequence));
				if (ImportedSequence.Count > 0 && ImportedSequence.Count < MAX_SEQUENCE_ITEMS)
					for (unsigned int i = 0; i < ImportedSequence.Count; ++i)		// // //
						Seq.AddItem(ImportedSequence.Length[i], ImportedSequence.Value[i]);
				TmpSequences.push_back(Seq);		// // //
			}
			break;

		case FB_PATTERN_ROWS: {
			unsigned FrameCount = ReadInt(pOpenFile);
			Song.SetFrameCount(FrameCount);
			for (c = 0; c < FrameCount; c++)
				modfile.GetChannelOrder().ForeachChannel([&] (chan_id_t i) {
					Song.SetFramePattern(c, i, ReadInt(pOpenFile));
				});
			break;
		}
		case FB_PATTERNS: {
			ReadCount = ReadInt(pOpenFile);
			unsigned PatternLength = ReadInt(pOpenFile);
			Song.SetPatternLength(PatternLength);
			modfile.GetChannelOrder().ForeachChannel([&] (chan_id_t x) {
				for (c = 0; c < ReadCount; c++) {
					for (i = 0; i < PatternLength; i++) {
						pOpenFile->Read(&ImportedNote, sizeof(ImportedNote));
						if (ImportedNote.ExtraStuff1 == EF_PORTAOFF) {
							ImportedNote.ExtraStuff1 = EF_PORTAMENTO;
							ImportedNote.ExtraStuff2 = 0;
						}
						else if (ImportedNote.ExtraStuff1 == EF_PORTAMENTO) {
							if (ImportedNote.ExtraStuff2 < 0xFF)
								ImportedNote.ExtraStuff2++;
						}
						stChanNote Note;		// // //
						Note.EffNumber[0] = static_cast<effect_t>(ImportedNote.ExtraStuff1);
						Note.EffParam[0] = ImportedNote.ExtraStuff2;
						Note.Instrument = ImportedNote.Instrument;
						Note.Note = static_cast<note_t>(ImportedNote.Note);
						Note.Octave = ImportedNote.Octave;
						Note.Vol = 0;
						if (Note.Note == note_t::NONE)
							Note.Instrument = MAX_INSTRUMENTS;
						if (Note.Vol == 0)
							Note.Vol = MAX_VOLUME;
						if (Note.EffNumber[0] < EF_COUNT)		// // //
							Note.EffNumber[0] = EFF_CONVERSION_050.first[Note.EffNumber[0]];
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

			pOpenFile->Read(&ReadCount, sizeof(int));
			for (i = 0; i < ReadCount; i++) {
				std::vector<uint8_t> pBuf;		// // //
				pOpenFile->Read(&ImportedDSample, sizeof(ImportedDSample));
				if (ImportedDSample.SampleSize != 0 && ImportedDSample.SampleSize < 0x4000) {
					pBuf.resize(ImportedDSample.SampleSize);		// // //
					pOpenFile->Read(pBuf.data(), ImportedDSample.SampleSize);
				}

				modfile.GetDSampleManager()->SetDSample(i, std::make_shared<ft0cc::doc::dpcm_sample>(pBuf, ImportedDSample.Name));
			}
			break;
		}
		case FB_SONGNAME:
			pOpenFile->Read(pBuf, std::size(pBuf));		// // //
			modfile.SetModuleName(pBuf);
			break;

		case FB_SONGARTIST:
			pOpenFile->Read(pBuf, std::size(pBuf));
			modfile.SetModuleArtist(pBuf);
			break;

		case FB_SONGCOPYRIGHT:
			pOpenFile->Read(pBuf, std::size(pBuf));
			modfile.SetModuleCopyright(pBuf);
			break;

		default:
			FileBlock = FB_EOF;
		}
	}

	ReorderSequences(modfile, std::move(TmpSequences));		// // //

	pOpenFile->Close();

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
