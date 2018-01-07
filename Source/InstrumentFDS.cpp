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

#include "InstrumentFDS.h"		// // //
#include "Sequence.h"		// // //
#include "DocumentFile.h"
#include "SimpleFile.h"
#include "ModuleException.h"		// // //

const char TEST_WAVE[] = {
	00, 01, 12, 22, 32, 36, 39, 39, 42, 47, 47, 50, 48, 51, 54, 58,
	54, 55, 49, 50, 52, 61, 63, 63, 59, 56, 53, 51, 48, 47, 41, 35,
	35, 35, 41, 47, 48, 51, 53, 56, 59, 63, 63, 61, 52, 50, 49, 55,
	54, 58, 54, 51, 48, 50, 47, 47, 42, 39, 39, 36, 32, 22, 12, 01
};

const char *const CInstrumentFDS::SEQUENCE_NAME[] = {"Volume", "Arpeggio", "Pitch", "Hi-pitch", "(N/A)"};

CInstrumentFDS::CInstrumentFDS() : CSeqInstrument(INST_FDS),		// // //
	m_iModulationSpeed(0),
	m_iModulationDepth(0),
	m_iModulationDelay(0),
	m_bModulationEnable(true),
	m_iModulation()
{
	memcpy(m_iSamples, TEST_WAVE, WAVE_SIZE);
	for (int i = 0; i < SEQ_COUNT; ++i)
		m_pSequence.push_back(std::make_shared<CSequence>(i));
}

std::unique_ptr<CInstrument> CInstrumentFDS::Clone() const
{
	auto inst = std::make_unique<std::decay_t<decltype(*this)>>();		// // //
	inst->CloneFrom(this);
	return inst;
}

void CInstrumentFDS::CloneFrom(const CInstrument *pInst)
{
	CInstrument::CloneFrom(pInst);

	if (auto pNew = dynamic_cast<const CInstrumentFDS*>(pInst)) {
	// Copy parameters
		for (int i = 0; i < WAVE_SIZE; ++i)
			SetSample(i, pNew->GetSample(i));
		for (int i = 0; i < MOD_SIZE; ++i)
			SetModulation(i, pNew->GetModulation(i));
		SetModulationDelay(pNew->GetModulationDelay());
		SetModulationDepth(pNew->GetModulationDepth());
		SetModulationSpeed(pNew->GetModulationSpeed());

		// Copy sequences
		SetSequence(SEQ_VOLUME, pNew->GetSequence(SEQ_VOLUME));
		SetSequence(SEQ_ARPEGGIO, pNew->GetSequence(SEQ_ARPEGGIO));
		SetSequence(SEQ_PITCH, pNew->GetSequence(SEQ_PITCH));
	}
}

void CInstrumentFDS::StoreInstSequence(CSimpleFile &File, const CSequence &Seq) const		// // //
{
	// Store number of items in this sequence
	File.WriteInt(Seq.GetItemCount());
	// Store loop point
	File.WriteInt(Seq.GetLoopPoint());
	// Store release point (v4)
	File.WriteInt(Seq.GetReleasePoint());
	// Store setting (v4)
	File.WriteInt(Seq.GetSetting());
	// Store items
	for (unsigned i = 0; i < Seq.GetItemCount(); ++i)
		File.WriteChar(Seq.GetItem(i));
}

std::shared_ptr<CSequence> CInstrumentFDS::LoadInstSequence(CSimpleFile &File, unsigned SeqType) const		// // //
{
	int SeqCount = CModuleException::AssertRangeFmt(File.ReadInt(), 0, 0xFF, "Sequence item count");
	int Loop = CModuleException::AssertRangeFmt(static_cast<int>(File.ReadInt()), -1, SeqCount - 1, "Sequence loop point");
	int Release = CModuleException::AssertRangeFmt(static_cast<int>(File.ReadInt()), -1, SeqCount - 1, "Sequence release point");

	auto pSeq = std::make_shared<CSequence>(SeqType);
	pSeq->SetItemCount(SeqCount > MAX_SEQUENCE_ITEMS ? MAX_SEQUENCE_ITEMS : SeqCount);
	pSeq->SetLoopPoint(Loop);
	pSeq->SetReleasePoint(Release);
	pSeq->SetSetting(static_cast<seq_setting_t>(File.ReadInt()));		// // //

	for (int i = 0; i < SeqCount; ++i)
		pSeq->SetItem(i, File.ReadChar());

	return pSeq;
}

void CInstrumentFDS::StoreSequence(CDocumentFile &DocFile, const CSequence &Seq) const
{
	// Store number of items in this sequence
	DocFile.WriteBlockChar(Seq.GetItemCount());
	// Store loop point
	DocFile.WriteBlockInt(Seq.GetLoopPoint());
	// Store release point (v4)
	DocFile.WriteBlockInt(Seq.GetReleasePoint());
	// Store setting (v4)
	DocFile.WriteBlockInt(Seq.GetSetting());
	// Store items
	for (unsigned int j = 0; j < Seq.GetItemCount(); j++) {
		DocFile.WriteBlockChar(Seq.GetItem(j));
	}
}

std::shared_ptr<CSequence> CInstrumentFDS::LoadSequence(CDocumentFile &DocFile, unsigned SeqType) const
{
	int SeqCount = static_cast<unsigned char>(DocFile.GetBlockChar());
	unsigned int LoopPoint = CModuleException::AssertRangeFmt(DocFile.GetBlockInt(), -1, SeqCount - 1, "Sequence loop point");
	unsigned int ReleasePoint = CModuleException::AssertRangeFmt(DocFile.GetBlockInt(), -1, SeqCount - 1, "Sequence release point");

	// CModuleException::AssertRangeFmt(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count", "%i");

	auto pSeq = std::make_shared<CSequence>(SeqType);
	pSeq->SetItemCount(SeqCount > MAX_SEQUENCE_ITEMS ? MAX_SEQUENCE_ITEMS : SeqCount);
	pSeq->SetLoopPoint(LoopPoint);
	pSeq->SetReleasePoint(ReleasePoint);
	pSeq->SetSetting(static_cast<seq_setting_t>(DocFile.GetBlockInt()));		// // //

	for (int x = 0; x < SeqCount; ++x) {
		char Value = DocFile.GetBlockChar();
		pSeq->SetItem(x, Value);
	}

	return pSeq;
}

void CInstrumentFDS::DoubleVolume() const
{
	auto &Vol = *m_pSequence[SEQ_VOLUME];
	for (unsigned int i = 0; i < Vol.GetItemCount(); ++i)
		Vol.SetItem(i, Vol.GetItem(i) * 2);
}

void CInstrumentFDS::Store(CDocumentFile *pDocFile) const
{
	// Write wave
	for (int i = 0; i < WAVE_SIZE; ++i) {
		pDocFile->WriteBlockChar(GetSample(i));
	}

	// Write modulation table
	for (int i = 0; i < MOD_SIZE; ++i) {
		pDocFile->WriteBlockChar(GetModulation(i));
	}

	// Modulation parameters
	pDocFile->WriteBlockInt(GetModulationSpeed());
	pDocFile->WriteBlockInt(GetModulationDepth());
	pDocFile->WriteBlockInt(GetModulationDelay());

	// Sequences
	StoreSequence(*pDocFile, *GetSequence(SEQ_VOLUME));		// // //
	StoreSequence(*pDocFile, *GetSequence(SEQ_ARPEGGIO));
	StoreSequence(*pDocFile, *GetSequence(SEQ_PITCH));
}

bool CInstrumentFDS::Load(CDocumentFile *pDocFile)
{
	for (int i = 0; i < WAVE_SIZE; ++i) {
		SetSample(i, pDocFile->GetBlockChar());
	}

	for (int i = 0; i < MOD_SIZE; ++i) {
		SetModulation(i, pDocFile->GetBlockChar());
	}

	SetModulationSpeed(pDocFile->GetBlockInt());
	SetModulationDepth(pDocFile->GetBlockInt());
	SetModulationDelay(pDocFile->GetBlockInt());

	// hack to fix earlier saved files (remove this eventually)
/*
	if (pDocFile->GetBlockVersion() > 2) {
		LoadSequence(pDocFile, GetSequence(SEQ_VOLUME));
		LoadSequence(pDocFile, GetSequence(SEQ_ARPEGGIO));
		if (pDocFile->GetBlockVersion() > 2)
			LoadSequence(pDocFile, GetSequence(SEQ_PITCH));
	}
	else {
*/
	unsigned int a = pDocFile->GetBlockInt();
	unsigned int b = pDocFile->GetBlockInt();
	pDocFile->RollbackPointer(8);

	if (a < 256 && (b & 0xFF) != 0x00) {
	}
	else {
		SetSequence(SEQ_VOLUME, LoadSequence(*pDocFile, SEQ_VOLUME));		// // //
		SetSequence(SEQ_ARPEGGIO, LoadSequence(*pDocFile, SEQ_ARPEGGIO));
		//
		// Note: Remove this line when files are unable to load
		// (if a file contains FDS instruments but FDS is disabled)
		// this was a problem in an earlier version.
		//
		if (pDocFile->GetBlockVersion() > 2)
			SetSequence(SEQ_PITCH, LoadSequence(*pDocFile, SEQ_PITCH));
	}

//	}

	// Older files was 0-15, new is 0-31
	if (pDocFile->GetBlockVersion() <= 3)
		DoubleVolume();

	return true;
}

void CInstrumentFDS::OnBlankInstrument() {		// // //
	CInstrument::OnBlankInstrument(); // skip CSeqInstrument
}

void CInstrumentFDS::DoSaveFTI(CSimpleFile &File) const
{
	// Write wave
	for (int i = 0; i < WAVE_SIZE; ++i) {
		File.WriteChar(GetSample(i));
	}

	// Write modulation table
	for (int i = 0; i < MOD_SIZE; ++i) {
		File.WriteChar(GetModulation(i));
	}

	// Modulation parameters
	File.WriteInt(GetModulationSpeed());
	File.WriteInt(GetModulationDepth());
	File.WriteInt(GetModulationDelay());

	// Sequences
	StoreInstSequence(File, *GetSequence(SEQ_VOLUME));
	StoreInstSequence(File, *GetSequence(SEQ_ARPEGGIO));
	StoreInstSequence(File, *GetSequence(SEQ_PITCH));
}

void CInstrumentFDS::DoLoadFTI(CSimpleFile &File, int iVersion)
{
	// Read wave
	for (int i = 0; i < WAVE_SIZE; ++i) {
		SetSample(i, File.ReadChar());
	}

	// Read modulation table
	for (int i = 0; i < MOD_SIZE; ++i) {
		SetModulation(i, File.ReadChar());
	}

	// Modulation parameters
	SetModulationSpeed(File.ReadInt());
	SetModulationDepth(File.ReadInt());
	SetModulationDelay(File.ReadInt());

	// Sequences
	SetSequence(SEQ_VOLUME, LoadInstSequence(File, SEQ_VOLUME));
	SetSequence(SEQ_ARPEGGIO, LoadInstSequence(File, SEQ_ARPEGGIO));
	SetSequence(SEQ_PITCH, LoadInstSequence(File, SEQ_PITCH));

	if (iVersion <= 22)
		DoubleVolume();
}

bool CInstrumentFDS::CanRelease() const
{
	const auto &pVol = m_pSequence[SEQ_VOLUME];
	return pVol && pVol->GetItemCount() && pVol->GetReleasePoint() != -1;
}

unsigned char CInstrumentFDS::GetSample(int Index) const
{
	ASSERT(Index < WAVE_SIZE);
	return m_iSamples[Index];
}

void CInstrumentFDS::SetSample(int Index, int Sample)
{
	ASSERT(Index < WAVE_SIZE);
	if (m_iSamples[Index] != Sample)		// // //
		InstrumentChanged();
	m_iSamples[Index] = Sample;
}

int CInstrumentFDS::GetModulation(int Index) const
{
	return m_iModulation[Index];
}

void CInstrumentFDS::SetModulation(int Index, int Value)
{
	if (m_iModulation[Index] != Value)		// // //
		InstrumentChanged();
	m_iModulation[Index] = Value;
}

int CInstrumentFDS::GetModulationSpeed() const
{
	return m_iModulationSpeed;
}

void CInstrumentFDS::SetModulationSpeed(int Speed)
{
	if (m_iModulationSpeed != Speed)		// // //
		InstrumentChanged();
	m_iModulationSpeed = Speed;
}

int CInstrumentFDS::GetModulationDepth() const
{
	return m_iModulationDepth;
}

void CInstrumentFDS::SetModulationDepth(int Depth)
{
	if (m_iModulationDepth != Depth)		// // //
		InstrumentChanged();
	m_iModulationDepth = Depth;
}

int CInstrumentFDS::GetModulationDelay() const
{
	return m_iModulationDelay;
}

void CInstrumentFDS::SetModulationDelay(int Delay)
{
	if (m_iModulationDelay != Delay)		// // //
		InstrumentChanged();
	m_iModulationDelay = Delay;
}

bool CInstrumentFDS::GetModulationEnable() const
{
	return m_bModulationEnable;
}

void CInstrumentFDS::SetModulationEnable(bool Enable)
{
	if (m_bModulationEnable != Enable)			// // //
		InstrumentChanged();
	m_bModulationEnable = Enable;
}

bool CInstrumentFDS::GetSeqEnable(sequence_t SeqType) const
{
	switch (SeqType) {
	case SEQ_VOLUME: case SEQ_ARPEGGIO: case SEQ_PITCH:
		return true;
	}
	return false;
}

void CInstrumentFDS::SetSeqEnable(sequence_t, bool) {
}

int	CInstrumentFDS::GetSeqIndex(sequence_t SeqType) const
{
	__debugbreak();
	return -1;
}

void CInstrumentFDS::SetSeqIndex(sequence_t SeqType, int Value)
{
	__debugbreak();
}

std::shared_ptr<CSequence> CInstrumentFDS::GetSequence(sequence_t SeqType) const		// // //
{
	return GetSeqEnable(SeqType) ? m_pSequence[SeqType] : nullptr;
}

void CInstrumentFDS::SetSequence(sequence_t SeqType, std::shared_ptr<CSequence> pSeq)
{
	if (GetSeqEnable(SeqType))
		m_pSequence[SeqType] = std::move(pSeq);
}
