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
#include "Sequence.h"
#include "ft0cc/doc/dpcm_sample.hpp"
#include "DocumentFile.h"
#include "SimpleFile.h"
#include "ModuleException.h"

void CInstrumentIONull::WriteToModule(const CInstrument &inst_, CDocumentFile &file) const {
}

void CInstrumentIONull::ReadFromModule(CInstrument &inst_, CDocumentFile &file) const {
}

void CInstrumentIONull::WriteToFTI(const CInstrument &inst_, CSimpleFile &file) const {
}

void CInstrumentIONull::ReadFromFTI(CInstrument &inst_, CSimpleFile &file) const {
}



void CInstrumentIOSeq::WriteToModule(const CInstrument &inst_, CDocumentFile &file) const {
	auto &inst = dynamic_cast<const CSeqInstrument &>(inst_);

	int seqCount = inst.GetSeqCount();
	file.WriteBlockInt(seqCount);

	for (int i = 0; i < seqCount; ++i) {
		file.WriteBlockChar(inst.GetSeqEnable((sequence_t)i) ? 1 : 0);
		file.WriteBlockChar(inst.GetSeqIndex((sequence_t)i));
	}
}

void CInstrumentIOSeq::ReadFromModule(CInstrument &inst_, CDocumentFile &file) const {
	auto &inst = dynamic_cast<CSeqInstrument &>(inst_);

	CModuleException::AssertRangeFmt(file.GetBlockInt(), 0, (int)SEQ_COUNT, "Instrument sequence count"); // unused right now

	foreachSeq([&] (sequence_t i) {
		inst.SetSeqEnable(i, 0 != CModuleException::AssertRangeFmt<MODULE_ERROR_STRICT>(
			file.GetBlockChar(), 0, 1, "Instrument sequence enable flag"));
		int Index = static_cast<unsigned char>(file.GetBlockChar());		// // //
		inst.SetSeqIndex(i, CModuleException::AssertRangeFmt(Index, 0, MAX_SEQUENCES - 1, "Instrument sequence index"));
	});
}

void CInstrumentIOSeq::WriteToFTI(const CInstrument &inst_, CSimpleFile &file) const {
	auto &inst = dynamic_cast<const CSeqInstrument &>(inst_);

}

void CInstrumentIOSeq::ReadFromFTI(CInstrument &inst_, CSimpleFile &file) const {
	auto &inst = dynamic_cast<CSeqInstrument &>(inst_);

}



void CInstrumentIO2A03::WriteToModule(const CInstrument &inst_, CDocumentFile &file) const {
	auto &inst = dynamic_cast<const CInstrument2A03 &>(inst_);
	CInstrumentIOSeq::WriteToModule(inst_, file);

	int Version = 6;
	int Octaves = Version >= 2 ? OCTAVE_RANGE : 6;

	if (Version >= 7)		// // // 050B
		file.WriteBlockInt(inst.GetSampleCount());
	for (int n = 0; n < NOTE_RANGE * Octaves; ++n) {
		if (Version >= 7) {		// // // 050B
			if (inst.GetSampleIndex(n) == CInstrument2A03::NO_DPCM)
				continue;
			file.WriteBlockChar(n);
		}
		file.WriteBlockChar(inst.GetSampleIndex(n) + 1);
		file.WriteBlockChar(inst.GetSamplePitch(n));
		if (Version >= 6)
			file.WriteBlockChar(inst.GetSampleDeltaValue(n));
	}
}

void CInstrumentIO2A03::ReadFromModule(CInstrument &inst_, CDocumentFile &file) const {
	auto &inst = dynamic_cast<CInstrument2A03 &>(inst_);
	CInstrumentIOSeq::ReadFromModule(inst_, file);

	const int Version = file.GetBlockVersion();
	const int Octaves = (Version == 1) ? 6 : OCTAVE_RANGE;

	const auto ReadAssignment = [&] (int MidiNote) {
		try {
			int Index = CModuleException::AssertRangeFmt<MODULE_ERROR_STRICT>(
				file.GetBlockChar(), 0, MAX_DSAMPLES, "DPCM sample assignment index");
			if (Index > MAX_DSAMPLES)
				Index = 0;
			inst.SetSampleIndex(MidiNote, Index - 1);
			char Pitch = file.GetBlockChar();
			CModuleException::AssertRangeFmt<MODULE_ERROR_STRICT>(Pitch & 0x7F, 0, 0xF, "DPCM sample pitch");
			inst.SetSamplePitch(MidiNote, Pitch & 0x8F);
			if (Version > 5) {
				char Value = file.GetBlockChar();
				if (Value < -1) // not validated
					Value = -1;
				inst.SetSampleDeltaValue(MidiNote, Value);
			}
		}
		catch (CModuleException e) {
			e.AppendError("At note %i, octave %i,", value_cast(GET_NOTE(MidiNote)), GET_OCTAVE(MidiNote));
			throw e;
		}
	};

	if (Version >= 7) {		// // // 050B
		const int Count = CModuleException::AssertRangeFmt<MODULE_ERROR_STRICT>(
			file.GetBlockInt(), 0, NOTE_COUNT, "DPCM sample assignment count");
		for (int i = 0; i < Count; ++i) {
			int Note = CModuleException::AssertRangeFmt<MODULE_ERROR_STRICT>(
				file.GetBlockChar(), 0, NOTE_COUNT - 1, "DPCM sample assignment note index");
			ReadAssignment(Note);
		}
	}
	else
		for (int n = 0; n < NOTE_COUNT; ++n)
			ReadAssignment(n);
}

void CInstrumentIO2A03::WriteToFTI(const CInstrument &inst_, CSimpleFile &file) const {
	auto &inst = dynamic_cast<const CInstrument2A03 &>(inst_);


}

void CInstrumentIO2A03::ReadFromFTI(CInstrument &inst_, CSimpleFile &file) const {
	auto &inst = dynamic_cast<CInstrument2A03 &>(inst_);


}



void CInstrumentIOVRC7::WriteToModule(const CInstrument &inst_, CDocumentFile &file) const {
	auto &inst = dynamic_cast<const CInstrumentVRC7 &>(inst_);

	file.WriteBlockInt(inst.GetPatch());

	for (int i = 0; i < 8; ++i)
		file.WriteBlockChar(inst.GetCustomReg(i));
}

void CInstrumentIOVRC7::ReadFromModule(CInstrument &inst_, CDocumentFile &file) const {
	auto &inst = dynamic_cast<CInstrumentVRC7 &>(inst_);

	inst.SetPatch(CModuleException::AssertRangeFmt(file.GetBlockInt(), 0, 0xF, "VRC7 patch number"));

	for (int i = 0; i < 8; ++i)
		inst.SetCustomReg(i, file.GetBlockChar());
}

void CInstrumentIOVRC7::WriteToFTI(const CInstrument &inst_, CSimpleFile &file) const {
	auto &inst = dynamic_cast<const CInstrumentVRC7 &>(inst_);


}

void CInstrumentIOVRC7::ReadFromFTI(CInstrument &inst_, CSimpleFile &file) const {
	auto &inst = dynamic_cast<CInstrumentVRC7 &>(inst_);


}



void CInstrumentIOFDS::WriteToModule(const CInstrument &inst_, CDocumentFile &file) const {
	auto &inst = dynamic_cast<const CInstrumentFDS &>(inst_);

	// Write wave
	for (auto x : inst.GetSamples())		// // //
		file.WriteBlockChar(x);
	for (auto x : inst.GetModTable())
		file.WriteBlockChar(x);

	// Modulation parameters
	file.WriteBlockInt(inst.GetModulationSpeed());
	file.WriteBlockInt(inst.GetModulationDepth());
	file.WriteBlockInt(inst.GetModulationDelay());

	// Sequences
	const auto StoreSequence = [&] (const CSequence &Seq) {
		// Store number of items in this sequence
		file.WriteBlockChar(Seq.GetItemCount());
		// Store loop point
		file.WriteBlockInt(Seq.GetLoopPoint());
		// Store release point (v4)
		file.WriteBlockInt(Seq.GetReleasePoint());
		// Store setting (v4)
		file.WriteBlockInt(Seq.GetSetting());
		// Store items
		for (unsigned int j = 0; j < Seq.GetItemCount(); j++) {
			file.WriteBlockChar(Seq.GetItem(j));
		}
	};
	StoreSequence(*inst.GetSequence(sequence_t::Volume));		// // //
	StoreSequence(*inst.GetSequence(sequence_t::Arpeggio));
	StoreSequence(*inst.GetSequence(sequence_t::Pitch));
}

void CInstrumentIOFDS::ReadFromModule(CInstrument &inst_, CDocumentFile &file) const {
	auto &inst = dynamic_cast<CInstrumentFDS &>(inst_);

	unsigned char samples[64] = { };
	for (auto &x : samples)		// // //
		x = file.GetBlockChar();
	inst.SetSamples(samples);

	unsigned char modtable[32] = { };
	for (auto &x : modtable)
		x = file.GetBlockChar();
	inst.SetModTable(modtable);

	inst.SetModulationSpeed(file.GetBlockInt());
	inst.SetModulationDepth(file.GetBlockInt());
	inst.SetModulationDelay(file.GetBlockInt());

	// hack to fix earlier saved files (remove this eventually)
/*
	if (file.GetBlockVersion() > 2) {
		LoadSequence(pDocFile, GetSequence(sequence_t::Volume));
		LoadSequence(pDocFile, GetSequence(sequence_t::Arpeggio));
		if (file.GetBlockVersion() > 2)
			LoadSequence(pDocFile, GetSequence(sequence_t::Pitch));
	}
	else {
*/
	unsigned int a = file.GetBlockInt();
	unsigned int b = file.GetBlockInt();
	file.RollbackPointer(8);

	const auto LoadSequence = [&] (sequence_t SeqType) {
		int SeqCount = static_cast<unsigned char>(file.GetBlockChar());
		unsigned int LoopPoint = CModuleException::AssertRangeFmt(file.GetBlockInt(), -1, SeqCount - 1, "Sequence loop point");
		unsigned int ReleasePoint = CModuleException::AssertRangeFmt(file.GetBlockInt(), -1, SeqCount - 1, "Sequence release point");

		// CModuleException::AssertRangeFmt(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count", "%i");

		auto pSeq = std::make_shared<CSequence>(SeqType);
		pSeq->SetItemCount(SeqCount > MAX_SEQUENCE_ITEMS ? MAX_SEQUENCE_ITEMS : SeqCount);
		pSeq->SetLoopPoint(LoopPoint);
		pSeq->SetReleasePoint(ReleasePoint);
		pSeq->SetSetting(static_cast<seq_setting_t>(file.GetBlockInt()));		// // //

		for (int x = 0; x < SeqCount; ++x) {
			char Value = file.GetBlockChar();
			pSeq->SetItem(x, Value);
		}

		return pSeq;
	};

	if (a < 256 && (b & 0xFF) != 0x00) {
	}
	else {
		inst.SetSequence(sequence_t::Volume, LoadSequence(sequence_t::Volume));		// // //
		inst.SetSequence(sequence_t::Arpeggio, LoadSequence(sequence_t::Arpeggio));
		//
		// Note: Remove this line when files are unable to load
		// (if a file contains FDS instruments but FDS is disabled)
		// this was a problem in an earlier version.
		//
		if (file.GetBlockVersion() > 2)
			inst.SetSequence(sequence_t::Pitch, LoadSequence(sequence_t::Pitch));
	}

//	}

	// Older files was 0-15, new is 0-31
	if (file.GetBlockVersion() <= 3)
		DoubleVolume(*inst.GetSequence(sequence_t::Volume));
}

void CInstrumentIOFDS::WriteToFTI(const CInstrument &inst_, CSimpleFile &file) const {
	auto &inst = dynamic_cast<const CInstrumentFDS &>(inst_);


}

void CInstrumentIOFDS::ReadFromFTI(CInstrument &inst_, CSimpleFile &file) const {
	auto &inst = dynamic_cast<CInstrumentFDS &>(inst_);


}

void CInstrumentIOFDS::DoubleVolume(CSequence &seq) {
	for (unsigned int i = 0; i < seq.GetItemCount(); ++i)
		seq.SetItem(i, seq.GetItem(i) * 2);
}



void CInstrumentION163::WriteToModule(const CInstrument &inst_, CDocumentFile &file) const {
	auto &inst = dynamic_cast<const CInstrumentN163 &>(inst_);
	CInstrumentIOSeq::WriteToModule(inst_, file);

	// Store wave
	file.WriteBlockInt(inst.GetWaveSize());
	file.WriteBlockInt(inst.GetWavePos());
	//file.WriteBlockInt(m_bAutoWavePos ? 1 : 0);
	file.WriteBlockInt(inst.GetWaveCount());

	for (int i = 0; i < inst.GetWaveCount(); ++i)
		for (auto x : inst.GetSamples(i))
			file.WriteBlockChar(x);
}

void CInstrumentION163::ReadFromModule(CInstrument &inst_, CDocumentFile &file) const {
	auto &inst = dynamic_cast<CInstrumentN163 &>(inst_);
	CInstrumentIOSeq::ReadFromModule(inst_, file);

	inst.SetWaveSize(CModuleException::AssertRangeFmt(file.GetBlockInt(), 4, CInstrumentN163::MAX_WAVE_SIZE, "N163 wave size"));
	inst.SetWavePos(CModuleException::AssertRangeFmt(file.GetBlockInt(), 0, CInstrumentN163::MAX_WAVE_SIZE - 1, "N163 wave position"));
	CModuleException::AssertRangeFmt<MODULE_ERROR_OFFICIAL>(inst.GetWavePos(), 0, 0x7F, "N163 wave position");
	if (file.GetBlockVersion() >= 8) {		// // // 050B
		(void)(file.GetBlockInt() != 0);
	}
	inst.SetWaveCount(CModuleException::AssertRangeFmt(file.GetBlockInt(), 1, CInstrumentN163::MAX_WAVE_COUNT, "N163 wave count"));
	CModuleException::AssertRangeFmt<MODULE_ERROR_OFFICIAL>(inst.GetWaveCount(), 1, 0x10, "N163 wave count");

	for (int i = 0; i < inst.GetWaveCount(); ++i) {
		for (unsigned j = 0; j < inst.GetWaveSize(); ++j) try {
			inst.SetSample(i, j, CModuleException::AssertRangeFmt(file.GetBlockChar(), 0, 15, "N163 wave sample"));
		}
		catch (CModuleException e) {
			e.AppendError("At wave %i, sample %i,", i, j);
			throw e;
		}
	}
}

void CInstrumentION163::WriteToFTI(const CInstrument &inst_, CSimpleFile &file) const {
	auto &inst = dynamic_cast<const CInstrumentN163 &>(inst_);


}

void CInstrumentION163::ReadFromFTI(CInstrument &inst_, CSimpleFile &file) const {
	auto &inst = dynamic_cast<CInstrumentN163 &>(inst_);


}
