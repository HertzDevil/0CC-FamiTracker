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
#include "ModuleException.h"		// // //
#include "InstrumentManagerInterface.h"		// // //
#include "ft0cc/doc/dpcm_sample.hpp"		// // //
#include "DocumentFile.h"
#include "SimpleFile.h"

// 2A03 instruments

const char *const CInstrument2A03::SEQUENCE_NAME[] = {"Volume", "Arpeggio", "Pitch", "Hi-pitch", "Duty / Noise"};

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

void CInstrument2A03::Store(CDocumentFile *pDocFile) const
{
	CSeqInstrument::Store(pDocFile);		// // //

	int Version = 6;
	int Octaves = Version >= 2 ? OCTAVE_RANGE : 6;

	if (Version >= 7)		// // // 050B
		pDocFile->WriteBlockInt(GetSampleCount());
	for (int n = 0; n < NOTE_RANGE * Octaves; ++n) {
		if (Version >= 7) {		// // // 050B
			if (GetSampleIndex(n) == NO_DPCM)
				continue;
			pDocFile->WriteBlockChar(n);
		}
		pDocFile->WriteBlockChar(GetSampleIndex(n) + 1);
		pDocFile->WriteBlockChar(GetSamplePitch(n));
		if (Version >= 6)
			pDocFile->WriteBlockChar(GetSampleDeltaValue(n));
	}
}

bool CInstrument2A03::Load(CDocumentFile *pDocFile)
{
	if (!CSeqInstrument::Load(pDocFile)) return false;		// // //

	const int Version = pDocFile->GetBlockVersion();
	const int Octaves = (Version == 1) ? 6 : OCTAVE_RANGE;

	const auto ReadAssignment = [&] (int MidiNote) {
		try {
			int Index = CModuleException::AssertRangeFmt<MODULE_ERROR_STRICT>(
				pDocFile->GetBlockChar(), 0, MAX_DSAMPLES, "DPCM sample assignment index");
			if (Index > MAX_DSAMPLES)
				Index = 0;
			SetSampleIndex(MidiNote, Index - 1);
			char Pitch = pDocFile->GetBlockChar();
			CModuleException::AssertRangeFmt<MODULE_ERROR_STRICT>(Pitch & 0x7F, 0, 0xF, "DPCM sample pitch");
			SetSamplePitch(MidiNote, Pitch & 0x8F);
			if (Version > 5) {
				char Value = pDocFile->GetBlockChar();
				if (Value < -1) // not validated
					Value = -1;
				SetSampleDeltaValue(MidiNote, Value);
			}
		}
		catch (CModuleException e) {
			e.AppendError("At note %i, octave %i,", value_cast(GET_NOTE(MidiNote)), GET_OCTAVE(MidiNote));
			throw e;
		}
	};

	if (Version >= 7) {		// // // 050B
		const int Count = CModuleException::AssertRangeFmt<MODULE_ERROR_STRICT>(
			pDocFile->GetBlockInt(), 0, NOTE_COUNT, "DPCM sample assignment count");
		for (int i = 0; i < Count; ++i) {
			int Note = CModuleException::AssertRangeFmt<MODULE_ERROR_STRICT>(
				pDocFile->GetBlockChar(), 0, NOTE_COUNT - 1, "DPCM sample assignment note index");
			ReadAssignment(Note);
		}
	}
	else
		for (int n = 0; n < NOTE_COUNT; ++n)
			ReadAssignment(n);

	return true;
}

void CInstrument2A03::DoSaveFTI(CSimpleFile &File) const
{
	// Saves an 2A03 instrument
	// Current version 2.4

	// Sequences
	CSeqInstrument::DoSaveFTI(File);		// // //

	// DPCM
	if (!m_pInstManager) {		// // //
		File.WriteInt32(0);
		File.WriteInt32(0);
		return;
	}

	unsigned int Count = GetSampleCount();		// // // 050B
	File.WriteInt32(Count);

	bool UsedSamples[MAX_DSAMPLES] = { };

	int UsedCount = 0;
	for (int n = 0; n < NOTE_COUNT; ++n) {
		if (unsigned Sample = GetSampleIndex(n); Sample != NO_DPCM) {
			File.WriteInt8(n);
			File.WriteInt8(Sample + 1);
			File.WriteInt8(GetSamplePitch(n));
			File.WriteInt8(GetSampleDeltaValue(n));
			if (!UsedSamples[Sample])
				++UsedCount;
			UsedSamples[Sample] = true;
		}
	}

	// Write the number
	File.WriteInt32(UsedCount);

	// List of sample names
	for (int i = 0; i < MAX_DSAMPLES; ++i) if (UsedSamples[i]) {
		if (auto pSample = m_pInstManager->GetDSample(i)) {		// // //
			File.WriteInt32(i);
			File.WriteString(pSample->name());
			File.WriteString(std::string_view((const char *)pSample->data(), pSample->size()));
		}
	}
}

void CInstrument2A03::DoLoadFTI(CSimpleFile &File, int iVersion)
{
	char SampleNames[MAX_DSAMPLES][ft0cc::doc::dpcm_sample::max_name_length + 1];

	CSeqInstrument::DoLoadFTI(File, iVersion);		// // //

	unsigned int Count = File.ReadInt32();
	CModuleException::AssertRangeFmt(Count, 0U, static_cast<unsigned>(NOTE_COUNT), "DPCM assignment count");

	// DPCM instruments
	for (unsigned int i = 0; i < Count; ++i) {
		unsigned char InstNote = File.ReadInt8();
		try {
			unsigned char Sample = CModuleException::AssertRangeFmt(File.ReadInt8(), 0, 0x7F, "DPCM sample assignment index");
			if (Sample > MAX_DSAMPLES)
				Sample = 0;
			unsigned char Pitch = File.ReadInt8();
			CModuleException::AssertRangeFmt(Pitch & 0x7FU, 0U, 0xFU, "DPCM sample pitch");
			SetSamplePitch(InstNote, Pitch);
			SetSampleIndex(InstNote, Sample - 1);
			SetSampleDeltaValue(InstNote, CModuleException::AssertRangeFmt(
				static_cast<char>(iVersion >= 24 ? File.ReadInt8() : -1), -1, 0x7F, "DPCM sample delta value"));
		}
		catch (CModuleException e) {
			e.AppendError("At note %i, octave %i,", value_cast(GET_NOTE(InstNote)), GET_OCTAVE(InstNote));
			throw e;
		}
	}

	// DPCM samples list
	bool bAssigned[NOTE_COUNT] = {};
	int TotalSize = 0;		// // // ???
	for (int i = 0; i < MAX_DSAMPLES; ++i)
		if (auto pSamp = m_pInstManager->GetDSample(i))
			TotalSize += pSamp->size();

	unsigned int SampleCount = File.ReadInt32();
	for (unsigned int i = 0; i < SampleCount; ++i) {
		int Index = CModuleException::AssertRangeFmt(
			File.ReadInt32(), 0, MAX_DSAMPLES - 1, "DPCM sample index");
		int Len = CModuleException::AssertRangeFmt(
			File.ReadInt32(), 0, (int)ft0cc::doc::dpcm_sample::max_name_length, "DPCM sample name length");
		File.ReadBytes(SampleNames[Index], Len);
		SampleNames[Index][Len] = '\0';
		int Size = File.ReadInt32();
		std::vector<uint8_t> SampleData(Size);		// // //
		File.ReadBytes(SampleData.data(), Size);
		auto pSample = std::make_shared<ft0cc::doc::dpcm_sample>(SampleData, SampleNames[Index]);		// // //

		bool Found = false;
		for (int j = 0; j < MAX_DSAMPLES; ++j) if (auto s = m_pInstManager->GetDSample(j)) {		// // //
			// Compare size and name to see if identical sample exists
			if (*pSample == *s) {
				Found = true;
				// Assign sample
				for (int n = 0; n < NOTE_COUNT; ++n)
					if (GetSampleIndex(n) == Index && !bAssigned[n]) {
						SetSampleIndex(n, j);
						bAssigned[n] = true;
					}
				break;
			}
		}
		if (Found)
			continue;

		// Load sample

		if (TotalSize + Size > MAX_SAMPLE_SPACE)
			throw CModuleException::WithMessage("Insufficient DPCM sample space (maximum %d KB)", MAX_SAMPLE_SPACE / 1024);
		int FreeSample = m_pInstManager->AddDSample(pSample);
		if (FreeSample == -1)
			throw CModuleException::WithMessage("Document has no free DPCM sample slot");
		TotalSize += Size;
		// Assign it
		for (int n = 0; n < NOTE_COUNT; ++n)
			if (GetSampleIndex(n) == Index && !bAssigned[n]) {
				SetSampleIndex(n, FreeSample);
				bAssigned[n] = true;
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
