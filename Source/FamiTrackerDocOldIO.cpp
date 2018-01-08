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
#include "FamiTrackerDoc.h"
#include "SongData.h"
#include "InstrumentManager.h"
#include <memory>
#include <array>
#include "Instrument2A03.h"
#include "ft0cc/doc/dpcm_sample.hpp"
#include "Sequence.h"

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

bool compat::OpenDocumentOld(CFamiTrackerDoc &doc, CFile *pOpenFile) {
	unsigned int i, c, ReadCount, FileBlock;

	FileBlock = 0;

	// Only single track files
	auto &Song = *doc.GetSong(0);

	doc.SelectExpansionChip(SNDCHIP_NONE, 0, false);		// // //
	doc.SetMachine(NTSC);		// // //
	doc.SetVibratoStyle(VIBRATO_OLD);
	doc.SetLinearPitch(false);

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

		char pBuf[CFamiTrackerDoc::METADATA_FIELD_LENGTH] = { };		// // //

		switch (FileBlock) {
		case FB_CHANNELS:
			doc.SetAvailableChannels(ReadInt(pOpenFile));
			break;

		case FB_SPEED:
			Song.SetSongSpeed(ReadInt(pOpenFile) + 1);
			break;

		case FB_MACHINE:
			doc.SetMachine(ReadInt(pOpenFile) ? PAL : NTSC);
			break;

		case FB_ENGINESPEED:
			doc.SetEngineSpeed(ReadInt(pOpenFile));
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
						pInst->SetSeqEnable(j, ImportedInstruments.ModEnable[j]);
						pInst->SetSeqIndex(j, ImportedInstruments.ModIndex[j]);
						});
					pInst->SetName(ImportedInstruments.Name);

					if (ImportedInstruments.AssignedSample > 0) {
						int Pitch = 0;
						for (int y = 0; y < 6; y++) {
							for (int x = 0; x < 12; x++) {
								pInst->SetSampleIndex(y, x, ImportedInstruments.AssignedSample);
								pInst->SetSamplePitch(y, x, Pitch);
								Pitch = (Pitch + 1) % 16;
							}
						}
					}

					doc.AddInstrument(std::move(pInst), i);		// // //
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
				for (i = 0; i < doc.GetAvailableChannels(); i++)
					Song.SetFramePattern(c, i, ReadInt(pOpenFile));
			break;
		}
		case FB_PATTERNS: {
			ReadCount = ReadInt(pOpenFile);
			unsigned PatternLength = ReadInt(pOpenFile);
			Song.SetPatternLength(PatternLength);
			for (unsigned int x = 0; x < doc.GetAvailableChannels(); x++) {
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
						auto &Note = Song.GetPatternData(x, c, i);		// // //
						Note.EffNumber[0] = static_cast<effect_t>(ImportedNote.ExtraStuff1);
						Note.EffParam[0] = ImportedNote.ExtraStuff2;
						Note.Instrument = ImportedNote.Instrument;
						Note.Note = ImportedNote.Note;
						Note.Octave = ImportedNote.Octave;
						Note.Vol = 0;
						if (Note.Note == 0)
							Note.Instrument = MAX_INSTRUMENTS;
						if (Note.Vol == 0)
							Note.Vol = MAX_VOLUME;
						if (Note.EffNumber[0] < EF_COUNT)		// // //
							Note.EffNumber[0] = EFF_CONVERSION_050.first[Note.EffNumber[0]];
					}
				}
			}
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

				doc.SetSample(i, std::make_shared<ft0cc::doc::dpcm_sample>(pBuf, ImportedDSample.Name));
			}
			break;
		}
		case FB_SONGNAME:
			pOpenFile->Read(pBuf, std::size(pBuf));		// // //
			doc.SetModuleName(pBuf);
			break;

		case FB_SONGARTIST:
			pOpenFile->Read(pBuf, std::size(pBuf));
			doc.SetModuleArtist(pBuf);
			break;

		case FB_SONGCOPYRIGHT:
			pOpenFile->Read(pBuf, std::size(pBuf));
			doc.SetModuleCopyright(pBuf);
			break;

		default:
			FileBlock = FB_EOF;
		}
	}

	ReorderSequences(doc, std::move(TmpSequences));		// // //

	pOpenFile->Close();

	return TRUE;
}

void compat::ReorderSequences(CFamiTrackerDoc &doc, std::vector<COldSequence> seqs)		// // //
{
	int Slots[SEQ_COUNT] = {0, 0, 0, 0, 0};
	std::array<std::array<int, SEQ_COUNT>, MAX_SEQUENCES> Indices;
	for (auto &x : Indices)
		x.fill(-1);

	auto &Manager = *doc.GetInstrumentManager();

	// Organize sequences
	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (auto pInst = std::dynamic_pointer_cast<CInstrument2A03>(Manager.GetInstrument(i))) {		// // //
			foreachSeq([&] (sequence_t j) {
				if (pInst->GetSeqEnable(j)) {
					int Index = pInst->GetSeqIndex(j);
					if (Indices[Index][j] >= 0 && Indices[Index][j] != -1) {
						pInst->SetSeqIndex(j, Indices[Index][j]);
					}
					else {
						COldSequence &Seq = seqs[Index];		// // //
						if (j == SEQ_VOLUME)
							for (unsigned int k = 0; k < Seq.GetLength(); ++k)
								Seq.Value[k] = std::clamp(Seq.Value[k], (char)0, (char)15);
						else if (j == SEQ_DUTYCYCLE)
							for (unsigned int k = 0; k < Seq.GetLength(); ++k)
								Seq.Value[k] = std::clamp(Seq.Value[k], (char)0, (char)3);
						Indices[Index][j] = Slots[j];
						pInst->SetSeqIndex(j, Slots[j]);
						Manager.SetSequence(INST_2A03, j, Slots[j]++, Seq.Convert(j));
					}
				}
				else
					pInst->SetSeqIndex(j, 0);
			});
		}
	}
}
