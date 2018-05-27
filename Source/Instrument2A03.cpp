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

#include "Instrument2A03.h"
#include "InstrumentManagerInterface.h"		// // //
#include "ft0cc/doc/dpcm_sample.hpp"		// // //

// 2A03 instruments

CInstrument2A03::CInstrument2A03() : CSeqInstrument(INST_2A03)		// // //
{
}

std::unique_ptr<CInstrument> CInstrument2A03::Clone() const
{
	auto inst = std::make_unique<std::decay_t<decltype(*this)>>();		// // //
	inst->CloneFrom(this);
	return inst;
}

void CInstrument2A03::CloneFrom(const CInstrument *pInst)
{
	CSeqInstrument::CloneFrom(pInst);

	if (auto pNew = dynamic_cast<const CInstrument2A03*>(pInst)) {
		for (int n = 0; n < NOTE_COUNT; ++n) {
			SetSampleIndex(n, pNew->GetSampleIndex(n));
			SetSamplePitch(n, pNew->GetSamplePitch(n));
			SetSampleDeltaValue(n, pNew->GetSampleDeltaValue(n));
		}
	}
}

int CInstrument2A03::GetSampleCount() const		// // // 050B
{
	int Count = 0;
	for (int n = 0; n < NOTE_COUNT; ++n)
		if (GetSampleIndex(n) != NO_DPCM)
			++Count;
	return Count;
}

unsigned CInstrument2A03::GetSampleIndex(int MidiNote) const		// // //
{
	return m_Assignments[MidiNote].Index;
}

char CInstrument2A03::GetSamplePitch(int MidiNote) const
{
	return m_Assignments[MidiNote].Pitch;
}

bool CInstrument2A03::GetSampleLoop(int MidiNote) const
{
	return (m_Assignments[MidiNote].Pitch & 0x80u) == 0x80u;
}

char CInstrument2A03::GetSampleLoopOffset(int MidiNote) const
{
	return m_Assignments[MidiNote].LoopOffset;
}

char CInstrument2A03::GetSampleDeltaValue(int MidiNote) const
{
	return m_Assignments[MidiNote].Delta;
}

void CInstrument2A03::SetSampleIndex(int MidiNote, unsigned Sample)		// // //
{
	m_Assignments[MidiNote].Index = Sample;
}

void CInstrument2A03::SetSamplePitch(int MidiNote, char Pitch)
{
	m_Assignments[MidiNote].Pitch = Pitch;
}

void CInstrument2A03::SetSampleLoop(int MidiNote, bool Loop)
{
	m_Assignments[MidiNote].Pitch = (m_Assignments[MidiNote].Pitch & 0x7Fu) | (Loop ? 0x80u : 0x00u);
}

void CInstrument2A03::SetSampleLoopOffset(int MidiNote, char Offset)
{
	m_Assignments[MidiNote].LoopOffset = Offset;
}

void CInstrument2A03::SetSampleDeltaValue(int MidiNote, char Value)
{
	m_Assignments[MidiNote].Delta = Value;
}

bool CInstrument2A03::AssignedSamples() const
{
	// Returns true if there are assigned samples in this instrument

	for (int i = 0; i < NOTE_COUNT; ++i)
		if (GetSampleIndex(i) != NO_DPCM)
			return true;
	return false;
}

std::shared_ptr<ft0cc::doc::dpcm_sample> CInstrument2A03::GetDSample(int MidiNote) const		// // //
{
	if (!m_pInstManager)
		return nullptr;
	unsigned Index = GetSampleIndex(MidiNote);
	return Index != NO_DPCM ? m_pInstManager->GetDSample(Index) : nullptr;
}
