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


#pragma once

#include "InstrumentManagerInterface.h"
#include <vector>
#include <memory>
#include <mutex>

class CInstrument;
class CSequenceManager;
class CDSampleManager;

enum inst_type_t : unsigned;
enum class sequence_t : unsigned;

/*!
	\brief A container of FamiTracker instruments.
	\details This class implements common facilities for manipulating a fixed-length array of
	instrument objects.
*/
class CInstrumentManager : public CInstrumentManagerInterface
{
public:
	CInstrumentManager();
	virtual ~CInstrumentManager();

	void ClearAll();

	std::unique_ptr<CInstrument> CreateNew(inst_type_t InstType);
	std::shared_ptr<CInstrument> GetInstrument(unsigned int Index) const;
	std::shared_ptr<CInstrument> ReleaseInstrument(unsigned int Index);
	bool InsertInstrument(unsigned int Index, std::shared_ptr<CInstrument> pInst);
	bool RemoveInstrument(unsigned int Index);
	int CloneInstrument(unsigned OldIndex, unsigned NewIndex);		// // //
	bool DeepCloneInstrument(unsigned OldIndex, unsigned NewIndex);		// // //
	void SwapInstruments(unsigned int IndexA, unsigned int IndexB);

	bool IsInstrumentUsed(unsigned int Index) const;
	unsigned int GetInstrumentCount() const;
	unsigned int GetFirstUnused() const;
	int GetFreeSequenceIndex(inst_type_t InstType, sequence_t Type, const CSeqInstrument *pInst = nullptr) const;

	int GetSequenceCount(inst_type_t InstType, sequence_t Type) const;
	int GetTotalSequenceCount(inst_type_t InstType) const;

	inst_type_t GetInstrumentType(unsigned int Index) const;

	CSequenceManager *GetSequenceManager(int InstType) const;
	CDSampleManager *GetDSampleManager() const;

	// from interface
	std::shared_ptr<CSequence> GetSequence(inst_type_t InstType, sequence_t SeqType, int Index) const override; // TODO: use SetSequence and provide const getter
	void SetSequence(inst_type_t InstType, sequence_t SeqType, int Index, std::shared_ptr<CSequence> pSeq) override;
	int AddSequence(inst_type_t InstType, sequence_t SeqType, std::shared_ptr<CSequence> pSeq, CSeqInstrument *pInst = nullptr) override;
	std::shared_ptr<ft0cc::doc::dpcm_sample> GetDSample(int Index) override;
	std::shared_ptr<const ft0cc::doc::dpcm_sample> GetDSample(int Index) const override;
	void SetDSample(int Index, std::shared_ptr<ft0cc::doc::dpcm_sample> pSamp) override;
	int AddDSample(std::shared_ptr<ft0cc::doc::dpcm_sample> pSamp) override;

	// void (*F)(CInstrument &inst [, std::size_t index])
	template <typename F>
	void VisitInstruments(F f) {
		if constexpr (std::is_invocable_v<F, CInstrument &>) {
			for (auto &x : m_pInstruments)
				if (x)
					f(*x);
		}
		else if constexpr (std::is_invocable_v<F, CInstrument &, std::size_t>) {
			std::size_t index = 0;
			for (auto &x : m_pInstruments) {
				if (x)
					f(*x, index);
				++index;
			}
		}
		else
			static_assert(sizeof(F) == 0, "Invalid function signature");
	}

	// void (*F)(const CInstrument &inst [, std::size_t index])
	template <typename F>
	void VisitInstruments(F f) const {
		if constexpr (std::is_invocable_v<F, const CInstrument &>) {
			for (auto &x : m_pInstruments)
				if (x)
					f(*x);
		}
		else if constexpr (std::is_invocable_v<F, const CInstrument &, std::size_t>) {
			std::size_t index = 0;
			for (auto &x : m_pInstruments) {
				if (x)
					f(*x, index);
				++index;
			}
		}
		else
			static_assert(sizeof(F) == 0, "Invalid function signature");
	}

public:
	static const int MAX_INSTRUMENTS;

private:
	std::vector<std::shared_ptr<CInstrument>> m_pInstruments;
	std::vector<std::unique_ptr<CSequenceManager>> m_pSequenceManager;
	std::unique_ptr<CDSampleManager> m_pDSampleManager;

	mutable std::mutex m_InstrumentLock;		// // //

private:
	static const int SEQ_MANAGER_COUNT;
};
