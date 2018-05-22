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
#include "Assertion.h"		// // //

namespace {

const std::array<unsigned char, CInstrumentFDS::WAVE_SIZE> TEST_WAVE = {
	00, 01, 12, 22, 32, 36, 39, 39, 42, 47, 47, 50, 48, 51, 54, 58,
	54, 55, 49, 50, 52, 61, 63, 63, 59, 56, 53, 51, 48, 47, 41, 35,
	35, 35, 41, 47, 48, 51, 53, 56, 59, 63, 63, 61, 52, 50, 49, 55,
	54, 58, 54, 51, 48, 50, 47, 47, 42, 39, 39, 36, 32, 22, 12, 01
};

} // namespace

const char *const CInstrumentFDS::SEQUENCE_NAME[] = {"Volume", "Arpeggio", "Pitch", "Hi-pitch", "(N/A)"};

CInstrumentFDS::CInstrumentFDS() : CSeqInstrument(INST_FDS),		// // //
	m_iSamples(TEST_WAVE),		// // //
	m_iModulationSpeed(0),
	m_iModulationDepth(0),
	m_iModulationDelay(0),
	m_bModulationEnable(true)
{
	m_pSequence.emplace(sequence_t::Volume, std::make_shared<CSequence>(sequence_t::Volume));
	m_pSequence.emplace(sequence_t::Arpeggio, std::make_shared<CSequence>(sequence_t::Arpeggio));
	m_pSequence.emplace(sequence_t::Pitch, std::make_shared<CSequence>(sequence_t::Pitch));
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
		SetSamples(pNew->GetSamples());
		SetModTable(pNew->GetModTable());
		SetModulationDelay(pNew->GetModulationDelay());
		SetModulationDepth(pNew->GetModulationDepth());
		SetModulationSpeed(pNew->GetModulationSpeed());

		// Copy sequences
		SetSequence(sequence_t::Volume, pNew->GetSequence(sequence_t::Volume));
		SetSequence(sequence_t::Arpeggio, pNew->GetSequence(sequence_t::Arpeggio));
		SetSequence(sequence_t::Pitch, pNew->GetSequence(sequence_t::Pitch));
	}
}

void CInstrumentFDS::OnBlankInstrument() {		// // //
	CInstrument::OnBlankInstrument(); // skip CSeqInstrument
}

bool CInstrumentFDS::CanRelease() const
{
	const auto &Vol = *GetSequence(sequence_t::Volume);
	return Vol.GetItemCount() && Vol.GetReleasePoint() != -1;
}

unsigned char CInstrumentFDS::GetSample(int Index) const
{
	return m_iSamples[Index];
}

void CInstrumentFDS::SetSample(int Index, int Sample)
{
	m_iSamples[Index] = Sample;
}

array_view<const unsigned char> CInstrumentFDS::GetSamples() const {		// // //
	return m_iSamples;
}

void CInstrumentFDS::SetSamples(array_view<const unsigned char> Wave) {
	if (Wave.size() == m_iSamples.size())
		Wave.copy(m_iSamples);
}

int CInstrumentFDS::GetModulation(int Index) const
{
	return m_iModulation[Index];
}

void CInstrumentFDS::SetModulation(int Index, int Value)
{
	m_iModulation[Index] = Value;
}

array_view<const unsigned char> CInstrumentFDS::GetModTable() const {		// // //
	return m_iModulation;
}

void CInstrumentFDS::SetModTable(array_view<const unsigned char> Mod) {
	if (Mod.size() == m_iModulation.size())
		Mod.copy(m_iModulation);
}

int CInstrumentFDS::GetModulationSpeed() const
{
	return m_iModulationSpeed;
}

void CInstrumentFDS::SetModulationSpeed(int Speed)
{
	m_iModulationSpeed = Speed;
}

int CInstrumentFDS::GetModulationDepth() const
{
	return m_iModulationDepth;
}

void CInstrumentFDS::SetModulationDepth(int Depth)
{
	m_iModulationDepth = Depth;
}

int CInstrumentFDS::GetModulationDelay() const
{
	return m_iModulationDelay;
}

void CInstrumentFDS::SetModulationDelay(int Delay)
{
	m_iModulationDelay = Delay;
}

bool CInstrumentFDS::GetModulationEnable() const
{
	return m_bModulationEnable;
}

void CInstrumentFDS::SetModulationEnable(bool Enable)
{
	m_bModulationEnable = Enable;
}

bool CInstrumentFDS::GetSeqEnable(sequence_t SeqType) const
{
	switch (SeqType) {
	case sequence_t::Volume: case sequence_t::Arpeggio: case sequence_t::Pitch:
		return true;
	}
	return false;
}

void CInstrumentFDS::SetSeqEnable(sequence_t, bool) {
}

int	CInstrumentFDS::GetSeqIndex(sequence_t SeqType) const
{
	DEBUG_BREAK();
	return -1;
}

void CInstrumentFDS::SetSeqIndex(sequence_t SeqType, int Value)
{
	DEBUG_BREAK();
}

std::shared_ptr<CSequence> CInstrumentFDS::GetSequence(sequence_t SeqType) const		// // //
{
	auto it = m_pSequence.find(SeqType);
	return it != m_pSequence.end() ? it->second : nullptr;
}

void CInstrumentFDS::SetSequence(sequence_t SeqType, std::shared_ptr<CSequence> pSeq)
{
	if (GetSeqEnable(SeqType))
		m_pSequence[SeqType] = std::move(pSeq);
}
