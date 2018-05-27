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

#include "InstrumentN163.h"		// // //
#include "Assertion.h"		// // //

// // // Default wave

namespace {

const char TRIANGLE_WAVE[] = {
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
};
const int DEFAULT_WAVE_SIZE = std::size(TRIANGLE_WAVE);

} // namespace

CInstrumentN163::CInstrumentN163() : CSeqInstrument(INST_N163),		// // //
	m_iSamples(),
	m_iWaveSize(DEFAULT_WAVE_SIZE),
	m_iWavePos(0),
	m_bAutoWavePos(false),		// // // 050B
	m_iWaveCount(1)
{
	for (int j = 0; j < DEFAULT_WAVE_SIZE; ++j)
		m_iSamples[0][j] = TRIANGLE_WAVE[j];
}

std::unique_ptr<CInstrument> CInstrumentN163::Clone() const
{
	auto inst = std::make_unique<std::decay_t<decltype(*this)>>();		// // //
	inst->CloneFrom(this);
	return inst;
}

void CInstrumentN163::CloneFrom(const CInstrument *pInst)
{
	CSeqInstrument::CloneFrom(pInst);

	if (auto pNew = dynamic_cast<const CInstrumentN163 *>(pInst)) {
		SetWaveSize(pNew->GetWaveSize());
		SetWavePos(pNew->GetWavePos());
	//	SetAutoWavePos(pInst->GetAutoWavePos());
		SetWaveCount(pNew->GetWaveCount());

		m_iSamples = pNew->m_iSamples;
	}
}

bool CInstrumentN163::IsWaveEqual(const CInstrumentN163 &Instrument) const {		// // //
	int Count = GetWaveCount();
	int Size = GetWaveSize();

	if (Instrument.GetWaveCount() != Count)
		return false;

	if (Instrument.GetWaveSize() != Size)
		return false;

	for (int i = 0; i < Count; ++i)
		if (GetSamples(i) != Instrument.GetSamples(i))		// // //
			return false;

	return true;
}

bool CInstrumentN163::InsertNewWave(int Index)		// // //
{
	if (m_iWaveCount >= CInstrumentN163::MAX_WAVE_COUNT)
		return false;
	if (Index < 0 || Index > m_iWaveCount || Index >= CInstrumentN163::MAX_WAVE_COUNT)
		return false;

	for (int i = m_iWaveCount; i > Index; --i)
		m_iSamples[i] = m_iSamples[i - 1];
	m_iSamples[Index].fill(0);
	++m_iWaveCount;
	return true;
}

bool CInstrumentN163::RemoveWave(int Index)		// // //
{
	if (m_iWaveCount <= 1)
		return false;
	if (Index < 0 || Index >= m_iWaveCount)
		return false;

	for (int i = Index; i < m_iWaveCount - 1; ++i)
		m_iSamples[i] = m_iSamples[i + 1];
	--m_iWaveCount;
	return true;
}

unsigned CInstrumentN163::GetWaveSize() const		// // //
{
	return m_iWaveSize;
}

void CInstrumentN163::SetWaveSize(unsigned size)
{
	m_iWaveSize = size;
}

int CInstrumentN163::GetSample(int wave, int pos) const
{
	return m_iSamples[wave][pos];
}

void CInstrumentN163::SetSample(int wave, int pos, int sample)
{
	m_iSamples[wave][pos] = sample;
}

array_view<const int> CInstrumentN163::GetSamples(int wave) const {		// // //
	return {m_iSamples[wave].data(), GetWaveSize()};
}

void CInstrumentN163::SetSamples(int wave, array_view<const int> buf) {		// // //
	if (buf.size() <= MAX_WAVE_SIZE)
		buf.copy(m_iSamples[wave].data(), buf.size());
}

int CInstrumentN163::GetWavePos() const
{
	return m_iWavePos;
}

void CInstrumentN163::SetWavePos(int pos)
{
	m_iWavePos = std::min(pos, MAX_WAVE_SIZE - (int)m_iWaveSize);		// // // prevent reading non-wave n163 registers
}
/*
void CInstrumentN163::SetAutoWavePos(bool Enable)
{
	m_bAutoWavePos = Enable;
}

bool CInstrumentN106::GetAutoWavePos() const
{
	return m_bAutoWavePos;
}
*/
void CInstrumentN163::SetWaveCount(int count)
{
	Assert(count <= MAX_WAVE_COUNT);
	m_iWaveCount = count;
}

int CInstrumentN163::GetWaveCount() const
{
	return m_iWaveCount;
}
