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

#include "InstCompiler.h"
#include "SeqInstrument.h"
#include "InstrumentVRC7.h"
#include "InstrumentFDS.h"
#include "InstrumentN163.h"
#include "Chunk.h"
#include "Sequence.h"

int CInstCompilerNull::CompileChunk(const CInstrument &, CChunk &, unsigned) const {
	return 0;
}

int CInstCompilerSeq::CompileChunk(const CInstrument &inst_, CChunk &chunk, unsigned instIndex) const {
	auto &inst = dynamic_cast<const CSeqInstrument &>(inst_);

	int StoredBytes = 0;
	inst_type_t Type = inst.GetType();

	switch (Type) {		// // //
	case INST_2A03: chunk.StoreByte(0);  break;
	case INST_VRC6: chunk.StoreByte(4);  break;
	case INST_N163: chunk.StoreByte(9);  break;
	case INST_S5B:  chunk.StoreByte(10); break;
	}

	int ModSwitch = 0;
	foreachSeq([&] (sequence_t i) {
		const auto pSequence = inst.GetSequence(i);
		if (inst.GetSeqEnable(i) && pSequence && pSequence->GetItemCount() > 0)
			ModSwitch |= 1 << i;
	});
	chunk.StoreByte(ModSwitch);
	StoredBytes += 2;

	foreachSeq([&] (sequence_t i) {
		if (ModSwitch & (1 << i)) {
			chunk.StorePointer({CHUNK_SEQUENCE, inst.GetSeqIndex(i) * SEQ_COUNT + i, (unsigned)Type});		// // //
			StoredBytes += 2;
		}
	});

	return StoredBytes;
}

int CInstCompilerVRC7::CompileChunk(const CInstrument &inst_, CChunk &chunk, unsigned instIndex) const {
	auto &inst = dynamic_cast<const CInstrumentVRC7 &>(inst_);

	int Patch = inst.GetPatch();

	chunk.StoreByte(6);		// // // CHAN_VRC7
	chunk.StoreByte(Patch << 4);	// Shift up by 4 to make room for volume

	// Write custom patch settings
	if (Patch == 0)
		for (int i = 0; i < 8; ++i)
			chunk.StoreByte(inst.GetCustomReg(i));

	return (Patch == 0) ? 10 : 2;		// // //
}

int CInstCompilerFDS::CompileChunk(const CInstrument &inst_, CChunk &chunk, unsigned instIndex) const {
	auto &inst = dynamic_cast<const CInstrumentFDS &>(inst_);

	// Store wave
//	int Table = pCompiler->AddWavetable(m_iSamples);
//	int Table = 0;
//	chunk.StoreByte(Table);

	chunk.StoreByte(7);		// // // CHAN_FDS

	const int FIXED_FDS_INST_SIZE = 2 + 16 + 4 + 1;		// // //

	// Store sequences
	char Switch = 0;		// // //
	int size = FIXED_FDS_INST_SIZE;
	foreachSeq([&] (sequence_t i) {
		if (inst.GetSequence(i) && inst.GetSequence(i)->GetItemCount() > 0) {
			Switch |= 1 << i;
			size += 2;
		}
	});
	chunk.StoreByte(Switch);

	for (unsigned i = 0; i < CInstrumentFDS::SEQUENCE_COUNT; ++i)
		if (Switch & (1 << i))
			chunk.StorePointer({CHUNK_SEQUENCE, instIndex * 5 + i, INST_FDS});		// // //

	// // // Store modulation table, two entries/byte
	for (int i = 0; i < 16; ++i) {
		char Data = inst.GetModulation(i << 1) | (inst.GetModulation((i << 1) + 1) << 3);
		chunk.StoreByte(Data);
	}

	chunk.StoreByte(inst.GetModulationDelay());
	chunk.StoreByte(inst.GetModulationDepth());
	chunk.StoreWord(inst.GetModulationSpeed());

	return size;
}

int CInstCompilerN163::CompileChunk(const CInstrument &inst_, CChunk &chunk, unsigned instIndex) const {
	auto &inst = dynamic_cast<const CInstrumentN163 &>(inst_);

	int StoredBytes = CInstCompilerSeq::CompileChunk(inst, chunk, instIndex);		// // //

	// Store wave info
	chunk.StoreByte(inst.GetWaveSize() >> 1);
	chunk.StoreByte(/*inst.GetAutoWavePos() ? 0xFF :*/ inst.GetWavePos());
	StoredBytes += 2;

	// Store reference to wave
	chunk.StorePointer({CHUNK_WAVES, instIndex});		// // //
	StoredBytes += 2;

	return StoredBytes;
}

int CInstCompilerN163::StoreWaves(const CInstrumentN163 &inst, CChunk &chunk) const {
	int Count = inst.GetWaveCount();
	int Size = inst.GetWaveSize();

	// Number of waves
	// chunk.StoreByte(Count);

	// Pack samples
	for (int i = 0; i < Count; ++i)
		for (int j = 0; j < Size; j += 2)
			chunk.StoreByte(inst.GetSample(i, j) | (inst.GetSample(i, j + 1) << 4));

	return Count * Size / 2;
}
