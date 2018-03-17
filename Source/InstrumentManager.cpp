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

#include "InstrumentManager.h"
#include "Instrument.h"
#include "SeqInstrument.h"
#include "InstrumentService.h"
#include "Sequence.h"
#include "SequenceCollection.h"
#include "SequenceManager.h"
#include "DSampleManager.h"
#include "FamiTrackerEnv.h"

const int CInstrumentManager::MAX_INSTRUMENTS = 64;
const int CInstrumentManager::SEQ_MANAGER_COUNT = 5;

CInstrumentManager::CInstrumentManager() :
	m_pInstruments(MAX_INSTRUMENTS),
	m_pDSampleManager(std::make_unique<CDSampleManager>())
{
	for (int i = 0; i < SEQ_MANAGER_COUNT; ++i)
		m_pSequenceManager.push_back(std::make_unique<CSequenceManager>(i == 2 ? 3 : SEQ_COUNT));
}

CInstrumentManager::~CInstrumentManager()
{
	for (auto &ptr : m_pInstruments)
		if (ptr)
			ptr->RegisterManager(nullptr);
}

//
// Instrument methods
//

std::shared_ptr<CInstrument> CInstrumentManager::GetInstrument(unsigned int Index) const
{
	std::lock_guard<std::mutex> lock(m_InstrumentLock);
	return Index < m_pInstruments.size() ? m_pInstruments[Index] : nullptr;
}

std::shared_ptr<CInstrument> CInstrumentManager::ReleaseInstrument(unsigned int Index) {
	std::lock_guard<std::mutex> lock(m_InstrumentLock);
	if (Index >= m_pInstruments.size())
		return nullptr;
	if (m_pInstruments[Index])
		m_pInstruments[Index]->RegisterManager(nullptr);
	return std::move(m_pInstruments[Index]);
}

std::unique_ptr<CInstrument> CInstrumentManager::CreateNew(inst_type_t InstType)
{
	auto pInst = Env.GetInstrumentService()->Make(InstType);
	pInst->RegisterManager(this);
	return pInst;
}

bool CInstrumentManager::InsertInstrument(unsigned int Index, std::shared_ptr<CInstrument> pInst)
{
	if (!pInst)
		return false;
	m_InstrumentLock.lock();
	if (Index < m_pInstruments.size()) {
		if (m_pInstruments[Index] != pInst) {
			if (m_pInstruments[Index])
				m_pInstruments[Index]->RegisterManager(nullptr);
			m_pInstruments[Index] = pInst;
			m_InstrumentLock.unlock();
			pInst->RegisterManager(this);
		}
		else
			m_InstrumentLock.unlock();
		return true;
	}
	m_InstrumentLock.unlock();
	return false;
}

bool CInstrumentManager::RemoveInstrument(unsigned int Index)
{
	std::lock_guard<std::mutex> lock(m_InstrumentLock);
	if (Index >= m_pInstruments.size())
		return false;
	if (m_pInstruments[Index]) {
		m_pInstruments[Index]->RegisterManager(nullptr);
		m_pInstruments[Index].reset();
		return true;
	}
	return false;
}

int CInstrumentManager::CloneInstrument(unsigned OldIndex, unsigned NewIndex) {		// // //
	return IsInstrumentUsed(OldIndex) && NewIndex != INVALID_INSTRUMENT && !IsInstrumentUsed(NewIndex) &&
		InsertInstrument(NewIndex, GetInstrument(OldIndex)->Clone());		// // //
}

bool CInstrumentManager::DeepCloneInstrument(unsigned OldIndex, unsigned NewIndex) {		// // //
	if (CloneInstrument(OldIndex, NewIndex)) {
		if (auto pInstrument = std::dynamic_pointer_cast<CSeqInstrument>(GetInstrument(NewIndex))) {
			const inst_type_t it = pInstrument->GetType();
			foreachSeq([&] (sequence_t i) {
				int freeSeq = GetFreeSequenceIndex(it, i, pInstrument.get());
				if (freeSeq != -1) {
					if (pInstrument->GetSeqEnable(i))
						*GetSequence(it, i, unsigned(freeSeq)) = *pInstrument->GetSequence(i);		// // //
					pInstrument->SetSeqIndex(i, freeSeq);
				}
			});
		}
		return true;
	}

	return false;
}

void CInstrumentManager::SwapInstruments(unsigned int IndexA, unsigned int IndexB) {
	std::lock_guard<std::mutex> lock(m_InstrumentLock);
	m_pInstruments[IndexA].swap(m_pInstruments[IndexB]);
}

void CInstrumentManager::ClearAll()
{
	std::lock_guard<std::mutex> lock(m_InstrumentLock);
	for (auto &ptr : m_pInstruments) {
		if (ptr)
			ptr->RegisterManager(nullptr);
		ptr.reset();
	}
	for (int i = 0; i < SEQ_MANAGER_COUNT; ++i)
		m_pSequenceManager[i] = std::make_unique<CSequenceManager>(i == 2 ? 3 : SEQ_COUNT);
	m_pDSampleManager = std::make_unique<CDSampleManager>();
}

bool CInstrumentManager::IsInstrumentUsed(unsigned int Index) const
{
	return Index < MAX_INSTRUMENTS && m_pInstruments[Index];
}

unsigned int CInstrumentManager::GetInstrumentCount() const
{
	unsigned x = 0;
	for (int i = 0; i < MAX_INSTRUMENTS; ++i)
		if (m_pInstruments[i])
			++x;
	return x;
}

unsigned int CInstrumentManager::GetFirstUnused() const
{
	for (int i = 0; i < MAX_INSTRUMENTS; ++i)
		if (!m_pInstruments[i])
			return i;
	return INVALID_INSTRUMENT;
}

int CInstrumentManager::GetFreeSequenceIndex(inst_type_t InstType, sequence_t Type, const CSeqInstrument *pInst) const
{
	// moved from CFamiTrackerDoc
	std::vector<bool> Used(CSequenceCollection::MAX_SEQUENCES, false);
	for (int i = 0; i < MAX_INSTRUMENTS; ++i) if (GetInstrumentType(i) == InstType) {		// // //
		auto pInstrument = std::static_pointer_cast<CSeqInstrument>(GetInstrument(i));
		if (pInstrument->GetSeqEnable(Type) && ((pInst && pInst->GetSequence(Type)->GetItemCount()) || pInst != pInstrument.get()))
			Used[pInstrument->GetSeqIndex(Type)] = true;
	}
	for (int i = 0; i < CSequenceCollection::MAX_SEQUENCES; ++i)
		if (!Used[i])
			if (auto pSeq = GetSequence(InstType, Type, i); !pSeq || !pSeq->GetItemCount())
				return i;
	return -1;
}

int CInstrumentManager::GetSequenceCount(inst_type_t InstType, sequence_t Type) const {
	int Count = 0;
	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		if (auto pSeq = GetSequence(InstType, Type, i); pSeq && pSeq->GetItemCount() > 0)
			++Count;
	}
	return Count;
}

int CInstrumentManager::GetTotalSequenceCount(inst_type_t InstType) const {
	int Count = 0;
	foreachSeq([&] (sequence_t i) {
		Count += GetSequenceCount(InstType, i);
	});
	return Count;
}

inst_type_t CInstrumentManager::GetInstrumentType(unsigned int Index) const
{
	return !IsInstrumentUsed(Index) ? INST_NONE : m_pInstruments[Index]->GetType();
}

//
// Sequence methods
//

CSequenceManager *const CInstrumentManager::GetSequenceManager(int InstType) const
{
	int Index = -1;
	switch (InstType) {
	case INST_2A03: Index = 0; break;
	case INST_VRC6: Index = 1; break;
	case INST_FDS:  Index = 2; break;
	case INST_N163: Index = 3; break;
	case INST_S5B:  Index = 4; break;
	default: return nullptr;
	}
	return m_pSequenceManager[Index].get();
}

CDSampleManager *const CInstrumentManager::GetDSampleManager() const
{
	return m_pDSampleManager.get();
}

//
// from interface
//

std::shared_ptr<CSequence> CInstrumentManager::GetSequence(inst_type_t InstType, sequence_t SeqType, int Index) const
{
	if (auto pManager = GetSequenceManager(InstType))
		if (auto pCol = pManager->GetCollection(SeqType))
			return pCol->GetSequence(Index);
	return nullptr;
}

void CInstrumentManager::SetSequence(inst_type_t InstType, sequence_t SeqType, int Index, std::shared_ptr<CSequence> pSeq)
{
	if (auto pManager = GetSequenceManager(InstType))
		if (auto pCol = pManager->GetCollection(SeqType))
			pCol->SetSequence(Index, std::move(pSeq));
}

int CInstrumentManager::AddSequence(inst_type_t InstType, sequence_t SeqType, std::shared_ptr<CSequence> pSeq, CSeqInstrument *pInst)
{
	int Index = GetFreeSequenceIndex(InstType, SeqType, pInst);
	if (Index != -1)
		SetSequence(InstType, SeqType, Index, std::move(pSeq));
	return Index;
}

std::shared_ptr<ft0cc::doc::dpcm_sample> CInstrumentManager::GetDSample(int Index)
{
	return m_pDSampleManager->GetDSample(Index);
}

std::shared_ptr<const ft0cc::doc::dpcm_sample> CInstrumentManager::GetDSample(int Index) const
{
	return m_pDSampleManager->GetDSample(Index);
}

void CInstrumentManager::SetDSample(int Index, std::shared_ptr<ft0cc::doc::dpcm_sample> pSamp)
{
	m_pDSampleManager->SetDSample(Index, std::move(pSamp));
}

int CInstrumentManager::AddDSample(std::shared_ptr<ft0cc::doc::dpcm_sample> pSamp)
{
	int Index = m_pDSampleManager->GetFirstFree();
	if (Index != -1)
		SetDSample(Index, std::move(pSamp));
	return Index;
}
