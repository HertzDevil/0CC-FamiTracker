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

#include "SeqInstrument.h"
#include "InstrumentManagerInterface.h"
#include "Sequence.h"

/*
 * Base class for instruments using sequences
 */

CSeqInstrument::CSeqInstrument(inst_type_t type) : CInstrument(type),
	seq_indices_ {
		{sequence_t::Volume, { }},
		{sequence_t::Arpeggio, { }},
		{sequence_t::Pitch, { }},
		{sequence_t::HiPitch, { }},
		{sequence_t::DutyCycle, { }},
	}
{
}

std::unique_ptr<CInstrument> CSeqInstrument::Clone() const
{
	auto inst = std::make_unique<std::decay_t<decltype(*this)>>(m_iType);		// // //
	inst->CloneFrom(this);
	return inst;
}

void CSeqInstrument::CloneFrom(const CInstrument *pInst)
{
	CInstrument::CloneFrom(pInst);

	if (auto pNew = dynamic_cast<const CSeqInstrument *>(pInst))
		seq_indices_ = pNew->seq_indices_;
}

void CSeqInstrument::OnBlankInstrument()		// // //
{
	CInstrument::OnBlankInstrument();

	for (auto &[seqType, v] : seq_indices_) {
		auto &[enable, index] = v;
		enable = false;
		int newIndex = m_pInstManager->AddSequence(m_iType, seqType, nullptr, this);
		if (newIndex != -1)
			index = newIndex;
	}
}

int CSeqInstrument::GetSeqCount() const {		// // //
	return seq_indices_.size();
}

bool CSeqInstrument::GetSeqEnable(sequence_t SeqType) const
{
	auto it = seq_indices_.find(SeqType);
	return it != seq_indices_.end() && it->second.first;
}

void CSeqInstrument::SetSeqEnable(sequence_t SeqType, bool Enable)
{
	if (auto it = seq_indices_.find(SeqType); it != seq_indices_.end())
		it->second.first = Enable;
}

int	CSeqInstrument::GetSeqIndex(sequence_t SeqType) const
{
	auto it = seq_indices_.find(SeqType);
	return it != seq_indices_.end() ? it->second.second : -1;
}

void CSeqInstrument::SetSeqIndex(sequence_t SeqType, int Value)
{
	if (auto it = seq_indices_.find(SeqType); it != seq_indices_.end())
		it->second.second = Value;
}

std::shared_ptr<CSequence> CSeqInstrument::GetSequence(sequence_t SeqType) const		// // //
{
	auto it = seq_indices_.find(SeqType);
	return it == seq_indices_.end() ? nullptr :
		m_pInstManager->GetSequence(m_iType, SeqType, it->second.second);
}

void CSeqInstrument::SetSequence(sequence_t SeqType, std::shared_ptr<CSequence> pSeq)		// // //
{
	if (auto it = seq_indices_.find(SeqType); it != seq_indices_.end())
		m_pInstManager->SetSequence(m_iType, SeqType, it->second.second, std::move(pSeq));
}

bool CSeqInstrument::CanRelease() const
{
	return GetSeqEnable(sequence_t::Volume) && GetSequence(sequence_t::Volume)->GetReleasePoint() != -1;
}
