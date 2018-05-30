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

#include "InstrumentIO.h"
#include "Instrument2A03.h"
#include "InstrumentVRC7.h"
#include "InstrumentFDS.h"
#include "InstrumentN163.h"

#include "NumConv.h"

#include "InstrumentManagerInterface.h"
#include "Sequence.h"
#include "OldSequence.h"
#include "ft0cc/doc/dpcm_sample.hpp"

#include "DocumentFile.h"



namespace {

// FTI instruments files
const std::string_view FTI_INST_HEADER = "FTI";
const std::string_view FTI_INST_VERSION = "2.4";

} // namespace

CInstrumentIO::CInstrumentIO(module_error_level_t err_lv) : err_lv_(err_lv) {
}

void CInstrumentIO::WriteToModule(const CInstrument &inst, CDocumentOutputBlock &block, unsigned inst_index) const {
	// Write index and type
	block.WriteInt<std::int32_t>(inst_index);
	block.WriteInt<std::int8_t>(static_cast<char>(inst.GetType()));

	// Store the instrument
	DoWriteToModule(inst, block);

	// Store the name
	block.WriteString(inst.GetName());		// // //
}

void CInstrumentIO::WriteToFTI(const CInstrument &inst, CBinaryWriter &output) const {
	// Write header
	output.WriteBytes(byte_view(FTI_INST_HEADER));
	output.WriteBytes(byte_view(FTI_INST_VERSION));

	// Write type
	output.WriteInt<std::int8_t>(value_cast(inst.GetType()));

	// Write name
	output.WriteString(inst.GetName());

	// Write instrument data
	DoWriteToFTI(inst, output);
}

void CInstrumentIO::ReadFromFTI(CInstrument &inst, CBinaryReader &file, int fti_ver) const {
	inst.SetName(file.ReadString<char>());		// // //
	DoReadFromFTI(inst, file, fti_ver);
}

template <module_error_level_t l, typename T, typename U, typename V>
T CInstrumentIO::AssertRange(T Value, U Min, V Max, const std::string &Desc) const {
	if (l <= err_lv_ && !(Value >= Min && Value <= Max)) {
		std::string msg = Desc + " out of range: expected ["
			+ std::to_string(Min) + ','
			+ std::to_string(Max) + "], got "
			+ std::to_string(Value);
		throw CModuleException::WithMessage(msg);
	}
	return Value;
}



void CInstrumentIONull::DoWriteToModule(const CInstrument &inst_, CDocumentOutputBlock &block) const {
}

void CInstrumentIONull::ReadFromModule(CInstrument &inst_, CDocumentInputBlock &block) const {
}

void CInstrumentIONull::DoWriteToFTI(const CInstrument &inst_, CBinaryWriter &output) const {
}

void CInstrumentIONull::DoReadFromFTI(CInstrument &inst_, CBinaryReader &input, int fti_ver) const {
}



void CInstrumentIOSeq::DoWriteToModule(const CInstrument &inst_, CDocumentOutputBlock &block) const {
	auto &inst = dynamic_cast<const CSeqInstrument &>(inst_);

	int seqCount = inst.GetSeqCount();
	block.WriteInt<std::int32_t>(seqCount);

	for (int i = 0; i < seqCount; ++i) {
		block.WriteInt<std::int8_t>(inst.GetSeqEnable((sequence_t)i) ? 1 : 0);
		block.WriteInt<std::int8_t>(inst.GetSeqIndex((sequence_t)i));
	}
}

void CInstrumentIOSeq::ReadFromModule(CInstrument &inst_, CDocumentInputBlock &block) const {
	auto &inst = dynamic_cast<CSeqInstrument &>(inst_);

	AssertRange(block.ReadInt<std::int32_t>(), 0, (int)SEQ_COUNT, "Instrument sequence count"); // unused right now

	for (auto i : enum_values<sequence_t>()) {
		inst.SetSeqEnable(i, 0 != AssertRange<MODULE_ERROR_STRICT>(
			block.ReadInt<std::int8_t>(), 0, 1, "Instrument sequence enable flag"));
		int Index = block.ReadInt<std::uint8_t>();
		inst.SetSeqIndex(i, AssertRange(Index, 0, MAX_SEQUENCES - 1, "Instrument sequence index"));
	}
}

void CInstrumentIOSeq::DoWriteToFTI(const CInstrument &inst_, CBinaryWriter &output) const {
	auto &inst = dynamic_cast<const CSeqInstrument &>(inst_);

	int seqCount = inst.GetSeqCount();
	output.WriteInt<std::int8_t>(seqCount);

	for (auto i : enum_values<sequence_t>()) {
		if (inst.GetSeqEnable(i)) {
			auto pSeq = inst.GetSequence(i);
			output.WriteInt<std::int8_t>(1);
			output.WriteInt<std::int32_t>(pSeq->GetItemCount());
			output.WriteInt<std::int32_t>(pSeq->GetLoopPoint());
			output.WriteInt<std::int32_t>(pSeq->GetReleasePoint());
			output.WriteInt<std::int32_t>(pSeq->GetSetting());
			for (unsigned j = 0; j < pSeq->GetItemCount(); ++j) {
				output.WriteInt<std::int8_t>(pSeq->GetItem(j));
			}
		}
		else
			output.WriteInt<std::int8_t>(0);
	}
}

void CInstrumentIOSeq::DoReadFromFTI(CInstrument &inst_, CBinaryReader &input, int fti_ver) const {
	auto &inst = dynamic_cast<CSeqInstrument &>(inst_);

	// Sequences
	std::shared_ptr<CSequence> pSeq;

	AssertRange(input.ReadInt<std::int8_t>(), 0, (int)SEQ_COUNT, "Sequence count"); // unused right now

	// Loop through all instrument effects
	for (auto i : enum_values<sequence_t>()) {
		try {
			if (input.ReadInt<std::int8_t>() != 1) {
				inst.SetSeqEnable(i, false);
				inst.SetSeqIndex(i, 0);
				continue;
			}
			inst.SetSeqEnable(i, true);

			// Read the sequence
			int Count = AssertRange(input.ReadInt<std::int32_t>(), 0, 0xFF, "Sequence item count");

			if (fti_ver < 20) {
				COldSequence OldSeq;
				for (int j = 0; j < Count; ++j) {
					char Length = input.ReadInt<std::int8_t>();
					OldSeq.AddItem(Length, input.ReadInt<std::int8_t>());
				}
				pSeq = OldSeq.Convert(i);
			}
			else {
				pSeq = std::make_shared<CSequence>(i);
				int Count2 = Count > MAX_SEQUENCE_ITEMS ? MAX_SEQUENCE_ITEMS : Count;
				pSeq->SetItemCount(Count2);
				pSeq->SetLoopPoint(AssertRange(
					input.ReadInt<std::int32_t>(), -1, Count2 - 1, "Sequence loop point"));
				if (fti_ver > 20) {
					pSeq->SetReleasePoint(AssertRange(
						input.ReadInt<std::int32_t>(), -1, Count2 - 1, "Sequence release point"));
					if (fti_ver >= 22)
						pSeq->SetSetting(static_cast<seq_setting_t>(input.ReadInt<std::int32_t>()));
				}
				for (int j = 0; j < Count; ++j) {
					int8_t item = input.ReadInt<std::int8_t>();
					if (j < Count2)
						pSeq->SetItem(j, item);
				}
			}
			if (inst.GetSequence(i) && inst.GetSequence(i)->GetItemCount() > 0)
				throw CModuleException::WithMessage("Document has no free sequence slot");
			inst.GetInstrumentManager()->SetSequence(inst.GetType(), i, inst.GetSeqIndex(i), pSeq);
		}
		catch (CModuleException &e) {
			e.AppendError("At " + std::string {inst.GetSequenceName(value_cast(i))} + " sequence,");
			throw e;
		}
	}
}



void CInstrumentIO2A03::DoWriteToModule(const CInstrument &inst_, CDocumentOutputBlock &block) const {
	auto &inst = dynamic_cast<const CInstrument2A03 &>(inst_);
	CInstrumentIOSeq::DoWriteToModule(inst_, block);

	int Version = block.GetBlockVersion();
	int Octaves = Version >= 2 ? OCTAVE_RANGE : 6;

	if (Version >= 7)		// // // 050B
		block.WriteInt<std::int32_t>(inst.GetSampleCount());
	for (int n = 0; n < NOTE_RANGE * Octaves; ++n) {
		if (Version >= 7) {		// // // 050B
			if (inst.GetSampleIndex(n) == CInstrument2A03::NO_DPCM)
				continue;
			block.WriteInt<std::int8_t>(n);
		}
		block.WriteInt<std::int8_t>(inst.GetSampleIndex(n) + 1);
		block.WriteInt<std::int8_t>(inst.GetSamplePitch(n));
		if (Version >= 6)
			block.WriteInt<std::int8_t>(inst.GetSampleDeltaValue(n));
	}
}

void CInstrumentIO2A03::ReadFromModule(CInstrument &inst_, CDocumentInputBlock &block) const {
	auto &inst = dynamic_cast<CInstrument2A03 &>(inst_);
	CInstrumentIOSeq::ReadFromModule(inst_, block);

	const int Version = block.GetBlockVersion();
	const int Octaves = (Version == 1) ? 6 : OCTAVE_RANGE;

	const auto ReadAssignment = [&] (int MidiNote) {
		try {
			int Index = AssertRange<MODULE_ERROR_STRICT>(
				block.ReadInt<std::int8_t>(), 0, MAX_DSAMPLES, "DPCM sample assignment index");
			if (Index > MAX_DSAMPLES)
				Index = 0;
			inst.SetSampleIndex(MidiNote, Index - 1);
			char Pitch = block.ReadInt<std::int8_t>();
			AssertRange<MODULE_ERROR_STRICT>(Pitch & 0x7F, 0, 0xF, "DPCM sample pitch");
			inst.SetSamplePitch(MidiNote, Pitch & 0x8F);
			if (Version > 5) {
				char Value = block.ReadInt<std::int8_t>();
				if (Value < -1) // not validated
					Value = -1;
				inst.SetSampleDeltaValue(MidiNote, Value);
			}
		}
		catch (CModuleException &e) {
			auto n = value_cast(ft0cc::doc::pitch_from_midi(MidiNote));
			auto o = ft0cc::doc::oct_from_midi(MidiNote);
			e.AppendError("At note " + conv::from_int(n) + ", octave " + conv::from_int(o) + ',');
			throw e;
		}
	};

	if (Version >= 7) {		// // // 050B
		const int Count = AssertRange<MODULE_ERROR_STRICT>(
			block.ReadInt<std::int32_t>(), 0, NOTE_COUNT, "DPCM sample assignment count");
		for (int i = 0; i < Count; ++i) {
			int Note = AssertRange<MODULE_ERROR_STRICT>(
				block.ReadInt<std::int8_t>(), 0, NOTE_COUNT - 1, "DPCM sample assignment note index");
			ReadAssignment(Note);
		}
	}
	else
		for (int n = 0; n < NOTE_RANGE * Octaves; ++n)
			ReadAssignment(n);
}

void CInstrumentIO2A03::DoWriteToFTI(const CInstrument &inst_, CBinaryWriter &output) const {
	auto &inst = dynamic_cast<const CInstrument2A03 &>(inst_);
	CInstrumentIOSeq::DoWriteToFTI(inst_, output);

	// Saves an 2A03 instrument
	// Current version 2.4

	// DPCM
	const auto *pManager = inst.GetInstrumentManager();
	if (!pManager) {
		output.WriteInt<std::int32_t>(0);
		output.WriteInt<std::int32_t>(0);
		return;
	}

	unsigned int Count = inst.GetSampleCount();		// // // 050B
	output.WriteInt<std::int32_t>(Count);

	bool UsedSamples[MAX_DSAMPLES] = { };

	int UsedCount = 0;
	for (int n = 0; n < NOTE_COUNT; ++n) {
		if (unsigned Sample = inst.GetSampleIndex(n); Sample != CInstrument2A03::NO_DPCM) {
			output.WriteInt<std::uint8_t>(n);
			output.WriteInt<std::uint8_t>(Sample + 1);
			output.WriteInt<std::uint8_t>(inst.GetSamplePitch(n));
			output.WriteInt<std::int8_t>(inst.GetSampleDeltaValue(n));
			if (!UsedSamples[Sample])
				++UsedCount;
			UsedSamples[Sample] = true;
		}
	}

	// Write the number
	output.WriteInt<std::int32_t>(UsedCount);

	// List of sample names
	for (int i = 0; i < MAX_DSAMPLES; ++i) if (UsedSamples[i]) {
		if (auto pSample = pManager->GetDSample(i)) {
			output.WriteInt<std::int32_t>(i);
			output.WriteString(pSample->name());
			output.WriteString(std::string_view(reinterpret_cast<const char *>(pSample->data()), pSample->size()));
		}
	}
}

void CInstrumentIO2A03::DoReadFromFTI(CInstrument &inst_, CBinaryReader &input, int fti_ver) const {
	auto &inst = dynamic_cast<CInstrument2A03 &>(inst_);
	CInstrumentIOSeq::DoReadFromFTI(inst_, input, fti_ver);

	auto *pManager = inst.GetInstrumentManager();

	char SampleNames[MAX_DSAMPLES][ft0cc::doc::dpcm_sample::max_name_length + 1] = { };

	unsigned int Count = input.ReadInt<std::int32_t>();
	AssertRange(Count, 0U, static_cast<unsigned>(NOTE_COUNT), "DPCM assignment count");

	// DPCM instruments
	for (unsigned int i = 0; i < Count; ++i) {
		unsigned char InstNote = input.ReadInt<std::uint8_t>();
		try {
			unsigned char Sample = AssertRange(input.ReadInt<std::uint8_t>(), 0, 0x7F, "DPCM sample assignment index");
			if (Sample > MAX_DSAMPLES)
				Sample = 0;
			unsigned char Pitch = input.ReadInt<std::uint8_t>();
			AssertRange(Pitch & 0x7FU, 0U, 0xFU, "DPCM sample pitch");
			inst.SetSamplePitch(InstNote, Pitch);
			inst.SetSampleIndex(InstNote, Sample - 1);
			inst.SetSampleDeltaValue(InstNote, AssertRange(
				fti_ver >= 24 ? input.ReadInt<std::int8_t>() : static_cast<std::int8_t>(-1), -1, 0x7F, "DPCM sample delta value"));
		}
		catch (CModuleException &e) {
			auto n = value_cast(ft0cc::doc::pitch_from_midi(InstNote));
			auto o = ft0cc::doc::oct_from_midi(InstNote);
			e.AppendError("At note " + conv::from_int(n) + ", octave " + conv::from_int(o) + ',');
			throw e;
		}
	}

	// DPCM samples list
	bool bAssigned[NOTE_COUNT] = {};
	int TotalSize = 0;		// // // ???
	for (int i = 0; i < MAX_DSAMPLES; ++i)
		if (auto pSamp = pManager->GetDSample(i))
			TotalSize += pSamp->size();

	unsigned int SampleCount = input.ReadInt<std::int32_t>();
	for (unsigned int i = 0; i < SampleCount; ++i) {
		int Index = AssertRange(input.ReadInt<std::int32_t>(), 0, MAX_DSAMPLES - 1, "DPCM sample index");
		std::size_t Len = AssertRange(input.ReadInt<std::int32_t>(), 0, (int)ft0cc::doc::dpcm_sample::max_name_length, "DPCM sample name length");
		input.ReadBytes(as_writeable_bytes(array_view<char> {SampleNames[Index], Len}));
		SampleNames[Index][Len] = '\0';
		int Size = input.ReadInt<std::int32_t>();
		std::vector<uint8_t> SampleData(Size);
		input.ReadBytes(byte_view(SampleData));
		auto pSample = std::make_shared<ft0cc::doc::dpcm_sample>(std::move(SampleData), SampleNames[Index]);

		bool Found = false;
		for (int j = 0; j < MAX_DSAMPLES; ++j) if (auto s = pManager->GetDSample(j)) {
			// Compare size and name to see if identical sample exists
			if (*pSample == *s) {
				Found = true;
				// Assign sample
				for (int n = 0; n < NOTE_COUNT; ++n)
					if (inst.GetSampleIndex(n) == Index && !bAssigned[n]) {
						inst.SetSampleIndex(n, j);
						bAssigned[n] = true;
					}
				break;
			}
		}
		if (Found)
			continue;

		// Load sample

		if (TotalSize + Size > MAX_SAMPLE_SPACE)
			throw CModuleException::WithMessage("Insufficient DPCM sample space (maximum " + conv::from_int(MAX_SAMPLE_SPACE / 1024) + " KB)");
		int FreeSample = pManager->AddDSample(pSample);
		if (FreeSample == -1)
			throw CModuleException::WithMessage("Document has no free DPCM sample slot");
		TotalSize += Size;
		// Assign it
		for (int n = 0; n < NOTE_COUNT; ++n)
			if (inst.GetSampleIndex(n) == Index && !bAssigned[n]) {
				inst.SetSampleIndex(n, FreeSample);
				bAssigned[n] = true;
			}
	}
}



void CInstrumentIOVRC7::DoWriteToModule(const CInstrument &inst_, CDocumentOutputBlock &block) const {
	auto &inst = dynamic_cast<const CInstrumentVRC7 &>(inst_);

	block.WriteInt<std::int32_t>(inst.GetPatch());

	for (int i = 0; i < 8; ++i)
		block.WriteInt<std::int8_t>(inst.GetCustomReg(i));
}

void CInstrumentIOVRC7::ReadFromModule(CInstrument &inst_, CDocumentInputBlock &block) const {
	auto &inst = dynamic_cast<CInstrumentVRC7 &>(inst_);

	inst.SetPatch(AssertRange(block.ReadInt<std::int32_t>(), 0, 0xF, "VRC7 patch number"));

	for (int i = 0; i < 8; ++i)
		inst.SetCustomReg(i, block.ReadInt<std::int8_t>());
}

void CInstrumentIOVRC7::DoWriteToFTI(const CInstrument &inst_, CBinaryWriter &output) const {
	auto &inst = dynamic_cast<const CInstrumentVRC7 &>(inst_);

	output.WriteInt<std::int32_t>(inst.GetPatch());

	for (int i = 0; i < 8; ++i)
		output.WriteInt<std::uint8_t>(inst.GetCustomReg(i));
}

void CInstrumentIOVRC7::DoReadFromFTI(CInstrument &inst_, CBinaryReader &input, int fti_ver) const {
	auto &inst = dynamic_cast<CInstrumentVRC7 &>(inst_);

	inst.SetPatch(input.ReadInt<std::int32_t>());

	for (int i = 0; i < 8; ++i)
		inst.SetCustomReg(i, input.ReadInt<std::uint8_t>());
}



void CInstrumentIOFDS::DoWriteToModule(const CInstrument &inst_, CDocumentOutputBlock &block) const {
	auto &inst = dynamic_cast<const CInstrumentFDS &>(inst_);

	// Write wave
	for (auto x : inst.GetSamples())
		block.WriteInt<std::int8_t>(x);
	for (auto x : inst.GetModTable())
		block.WriteInt<std::int8_t>(x);

	// Modulation parameters
	block.WriteInt<std::int32_t>(inst.GetModulationSpeed());
	block.WriteInt<std::int32_t>(inst.GetModulationDepth());
	block.WriteInt<std::int32_t>(inst.GetModulationDelay());

	// Sequences
	const auto StoreSequence = [&] (const CSequence &Seq) {
		// Store number of items in this sequence
		block.WriteInt<std::int8_t>(Seq.GetItemCount());
		// Store loop point
		block.WriteInt<std::int32_t>(Seq.GetLoopPoint());
		// Store release point (v4)
		block.WriteInt<std::int32_t>(Seq.GetReleasePoint());
		// Store setting (v4)
		block.WriteInt<std::int32_t>(Seq.GetSetting());
		// Store items
		for (unsigned int j = 0; j < Seq.GetItemCount(); ++j) {
			block.WriteInt<std::int8_t>(Seq.GetItem(j));
		}
	};
	StoreSequence(*inst.GetSequence(sequence_t::Volume));
	StoreSequence(*inst.GetSequence(sequence_t::Arpeggio));
	StoreSequence(*inst.GetSequence(sequence_t::Pitch));
}

void CInstrumentIOFDS::ReadFromModule(CInstrument &inst_, CDocumentInputBlock &block) const {
	auto &inst = dynamic_cast<CInstrumentFDS &>(inst_);

	unsigned char samples[64] = { };
	for (auto &x : samples)
		x = block.ReadInt<std::int8_t>();
	inst.SetSamples(samples);

	unsigned char modtable[32] = { };
	for (auto &x : modtable)
		x = block.ReadInt<std::int8_t>();
	inst.SetModTable(modtable);

	inst.SetModulationSpeed(block.ReadInt<std::int32_t>());
	inst.SetModulationDepth(block.ReadInt<std::int32_t>());
	inst.SetModulationDelay(block.ReadInt<std::int32_t>());

	// hack to fix earlier saved files (remove this eventually)
/*
	if (block.GetBlockVersion() > 2) {
		LoadSequence(pDocFile, GetSequence(sequence_t::Volume));
		LoadSequence(pDocFile, GetSequence(sequence_t::Arpeggio));
		if (block.GetBlockVersion() > 2)
			LoadSequence(pDocFile, GetSequence(sequence_t::Pitch));
	}
	else {
*/
	unsigned int a = block.ReadInt<std::int32_t>();
	unsigned int b = block.ReadInt<std::int32_t>();
	block.AdvancePointer(-8);

	const auto LoadSequence = [&] (sequence_t SeqType) {
		int SeqCount = block.ReadInt<std::uint8_t>();
		unsigned int LoopPoint = AssertRange(block.ReadInt<std::int32_t>(), -1, SeqCount - 1, "Sequence loop point");
		unsigned int ReleasePoint = AssertRange(block.ReadInt<std::int32_t>(), -1, SeqCount - 1, "Sequence release point");

		// AssertRange(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count", "%i");

		auto pSeq = std::make_shared<CSequence>(SeqType);
		pSeq->SetItemCount(SeqCount > MAX_SEQUENCE_ITEMS ? MAX_SEQUENCE_ITEMS : SeqCount);
		pSeq->SetLoopPoint(LoopPoint);
		pSeq->SetReleasePoint(ReleasePoint);
		pSeq->SetSetting(static_cast<seq_setting_t>(block.ReadInt<std::int32_t>()));

		for (int x = 0; x < SeqCount; ++x) {
			char Value = block.ReadInt<std::int8_t>();
			pSeq->SetItem(x, Value);
		}

		return pSeq;
	};

	if (a < 256 && (b & 0xFF) != 0x00) {
	}
	else {
		inst.SetSequence(sequence_t::Volume, LoadSequence(sequence_t::Volume));
		inst.SetSequence(sequence_t::Arpeggio, LoadSequence(sequence_t::Arpeggio));
		//
		// Note: Remove this line when files are unable to load
		// (if a module contains FDS instruments but FDS is disabled)
		// this was a problem in an earlier version.
		//
		if (block.GetBlockVersion() > 2)
			inst.SetSequence(sequence_t::Pitch, LoadSequence(sequence_t::Pitch));
	}

//	}

	// Older files was 0-15, new is 0-31
	if (block.GetBlockVersion() <= 3)
		DoubleVolume(*inst.GetSequence(sequence_t::Volume));
}

void CInstrumentIOFDS::DoWriteToFTI(const CInstrument &inst_, CBinaryWriter &output) const {
	auto &inst = dynamic_cast<const CInstrumentFDS &>(inst_);

	// Write wave
	for (auto x : inst.GetSamples())
		output.WriteInt<std::uint8_t>(x);
	for (auto x : inst.GetModTable())
		output.WriteInt<std::uint8_t>(x);

	// Modulation parameters
	output.WriteInt<std::int32_t>(inst.GetModulationSpeed());
	output.WriteInt<std::int32_t>(inst.GetModulationDepth());
	output.WriteInt<std::int32_t>(inst.GetModulationDelay());

	// Sequences
	const auto StoreInstSequence = [&] (const CSequence &Seq) {
		// Store number of items in this sequence
		output.WriteInt<std::int32_t>(Seq.GetItemCount());
		// Store loop point
		output.WriteInt<std::int32_t>(Seq.GetLoopPoint());
		// Store release point (v4)
		output.WriteInt<std::int32_t>(Seq.GetReleasePoint());
		// Store setting (v4)
		output.WriteEnum(Seq.GetSetting());
		// Store items
		for (unsigned i = 0; i < Seq.GetItemCount(); ++i)
			output.WriteInt<std::int8_t>(Seq.GetItem(i));
	};
	StoreInstSequence(*inst.GetSequence(sequence_t::Volume));
	StoreInstSequence(*inst.GetSequence(sequence_t::Arpeggio));
	StoreInstSequence(*inst.GetSequence(sequence_t::Pitch));
}

void CInstrumentIOFDS::DoReadFromFTI(CInstrument &inst_, CBinaryReader &input, int fti_ver) const {
	auto &inst = dynamic_cast<CInstrumentFDS &>(inst_);

	// Read wave
	unsigned char samples[64] = { };
	for (auto &x : samples)
		x = input.ReadInt<std::uint8_t>();
	inst.SetSamples(samples);

	unsigned char modtable[32] = { };
	for (auto &x : modtable)
		x = input.ReadInt<std::uint8_t>();
	inst.SetModTable(modtable);

	// Modulation parameters
	inst.SetModulationSpeed(input.ReadInt<std::int32_t>());
	inst.SetModulationDepth(input.ReadInt<std::int32_t>());
	inst.SetModulationDelay(input.ReadInt<std::int32_t>());

	// Sequences
	const auto LoadInstSequence = [&] (sequence_t SeqType) {
		int SeqCount = AssertRange(input.ReadInt<std::int32_t>(), 0, 0xFF, "Sequence item count");
		int Loop = AssertRange(input.ReadInt<std::int32_t>(), -1, SeqCount - 1, "Sequence loop point");
		int Release = AssertRange(input.ReadInt<std::int32_t>(), -1, SeqCount - 1, "Sequence release point");

		auto pSeq = std::make_shared<CSequence>(SeqType);
		pSeq->SetItemCount(SeqCount > MAX_SEQUENCE_ITEMS ? MAX_SEQUENCE_ITEMS : SeqCount);
		pSeq->SetLoopPoint(Loop);
		pSeq->SetReleasePoint(Release);
		pSeq->SetSetting(static_cast<seq_setting_t>(input.ReadInt<std::int32_t>()));

		for (int i = 0; i < SeqCount; ++i)
			pSeq->SetItem(i, input.ReadInt<std::int8_t>());

		return pSeq;
	};
	inst.SetSequence(sequence_t::Volume, LoadInstSequence(sequence_t::Volume));
	inst.SetSequence(sequence_t::Arpeggio, LoadInstSequence(sequence_t::Arpeggio));
	inst.SetSequence(sequence_t::Pitch, LoadInstSequence(sequence_t::Pitch));

	if (fti_ver <= 22)
		DoubleVolume(*inst.GetSequence(sequence_t::Volume));
}

void CInstrumentIOFDS::DoubleVolume(CSequence &seq) {
	for (unsigned int i = 0; i < seq.GetItemCount(); ++i)
		seq.SetItem(i, seq.GetItem(i) * 2);
}



void CInstrumentION163::DoWriteToModule(const CInstrument &inst_, CDocumentOutputBlock &block) const {
	auto &inst = dynamic_cast<const CInstrumentN163 &>(inst_);
	CInstrumentIOSeq::DoWriteToModule(inst_, block);

	// Store wave
	block.WriteInt<std::int32_t>(inst.GetWaveSize());
	block.WriteInt<std::int32_t>(inst.GetWavePos());
	//block.WriteInt<std::int32_t>(m_bAutoWavePos ? 1 : 0);
	block.WriteInt<std::int32_t>(inst.GetWaveCount());

	for (int i = 0; i < inst.GetWaveCount(); ++i)
		for (auto x : inst.GetSamples(i))
			block.WriteInt<std::int8_t>(x);
}

void CInstrumentION163::ReadFromModule(CInstrument &inst_, CDocumentInputBlock &block) const {
	auto &inst = dynamic_cast<CInstrumentN163 &>(inst_);
	CInstrumentIOSeq::ReadFromModule(inst_, block);

	inst.SetWaveSize(AssertRange(block.ReadInt<std::int32_t>(), 4, CInstrumentN163::MAX_WAVE_SIZE, "N163 wave size"));
	inst.SetWavePos(AssertRange(block.ReadInt<std::int32_t>(), 0, CInstrumentN163::MAX_WAVE_SIZE - 1, "N163 wave position"));
	AssertRange<MODULE_ERROR_OFFICIAL>(inst.GetWavePos(), 0, 0x7F, "N163 wave position");
	if (block.GetBlockVersion() >= 8) {		// // // 050B
		(void)(block.ReadInt<std::int32_t>() != 0);
	}
	inst.SetWaveCount(AssertRange(block.ReadInt<std::int32_t>(), 1, CInstrumentN163::MAX_WAVE_COUNT, "N163 wave count"));
	AssertRange<MODULE_ERROR_OFFICIAL>(inst.GetWaveCount(), 1, 0x10, "N163 wave count");

	for (int i = 0; i < inst.GetWaveCount(); ++i) {
		for (unsigned j = 0; j < inst.GetWaveSize(); ++j) try {
			inst.SetSample(i, j, AssertRange(block.ReadInt<std::int8_t>(), 0, 15, "N163 wave sample"));
		}
		catch (CModuleException &e) {
			e.AppendError("At wave " + conv::from_int(i) + ", sample " + conv::from_int(j) + ',');
			throw e;
		}
	}
}

void CInstrumentION163::DoWriteToFTI(const CInstrument &inst_, CBinaryWriter &output) const {
	auto &inst = dynamic_cast<const CInstrumentN163 &>(inst_);
	CInstrumentIOSeq::DoWriteToFTI(inst_, output);

	// Write wave config
	int WaveCount = inst.GetWaveCount();
	int WaveSize = inst.GetWaveSize();

	output.WriteInt<std::int32_t>(WaveSize);
	output.WriteInt<std::int32_t>(inst.GetWavePos());
//	block.WriteInt<std::int32_t>(m_bAutoWavePos);		// // // 050B
	output.WriteInt<std::int32_t>(WaveCount);

	for (int i = 0; i < WaveCount; ++i)
		for (auto x : inst.GetSamples(i))
			output.WriteInt<std::uint8_t>(x);
}

void CInstrumentION163::DoReadFromFTI(CInstrument &inst_, CBinaryReader &input, int fti_ver) const {
	auto &inst = dynamic_cast<CInstrumentN163 &>(inst_);
	CInstrumentIOSeq::DoReadFromFTI(inst_, input, fti_ver);

	// Read wave config
	int WaveSize = AssertRange(input.ReadInt<std::int32_t>(), 4, CInstrumentN163::MAX_WAVE_SIZE, "N163 wave size");
	int WavePos = AssertRange(input.ReadInt<std::int32_t>(), 0, CInstrumentN163::MAX_WAVE_SIZE - 1, "N163 wave position");
	if (fti_ver >= 25) {		// // // 050B
		bool AutoWavePos = input.ReadInt<std::int32_t>() != 0;
		(void)AutoWavePos;
	}
	int WaveCount = AssertRange(input.ReadInt<std::int32_t>(), 1, CInstrumentN163::MAX_WAVE_COUNT, "N163 wave count");

	inst.SetWaveSize(WaveSize);
	inst.SetWavePos(WavePos);
	inst.SetWaveCount(WaveCount);

	for (int i = 0; i < WaveCount; ++i)
		for (int j = 0; j < WaveSize; ++j) try {
			inst.SetSample(i, j, AssertRange(input.ReadInt<std::int8_t>(), 0, 15, "N163 wave sample"));
		}
	catch (CModuleException &e) {
		e.AppendError("At wave " + conv::from_int(i) + ", sample " + conv::from_int(j) + ',');
		throw e;
	}
}
