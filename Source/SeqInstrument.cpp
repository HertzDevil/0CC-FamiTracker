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
#include "ModuleException.h"
#include "InstrumentManagerInterface.h"
#include "Sequence.h"
#include "OldSequence.h"		// // //
#include "SimpleFile.h"

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

void CSeqInstrument::DoSaveFTI(CSimpleFile &File) const
{
	File.WriteInt8(static_cast<char>(seq_indices_.size()));

	foreachSeq([&] (sequence_t i) {
		if (GetSeqEnable(i)) {
			const auto pSeq = GetSequence(i);
			File.WriteInt8(1);
			File.WriteInt32(pSeq->GetItemCount());
			File.WriteInt32(pSeq->GetLoopPoint());
			File.WriteInt32(pSeq->GetReleasePoint());
			File.WriteInt32(pSeq->GetSetting());
			for (unsigned j = 0; j < pSeq->GetItemCount(); j++) {
				File.WriteInt8(pSeq->GetItem(j));
			}
		}
		else {
			File.WriteInt8(0);
		}
	});
}

void CSeqInstrument::DoLoadFTI(CSimpleFile &File, int iVersion)
{
	// Sequences
	std::shared_ptr<CSequence> pSeq;		// // //

	CModuleException::AssertRangeFmt(File.ReadInt8(), 0, (int)SEQ_COUNT, "Sequence count"); // unused right now

	// Loop through all instrument effects
	foreachSeq([&] (sequence_t i) {
		try {
			if (File.ReadInt8() != 1) {
				SetSeqEnable(i, false);
				SetSeqIndex(i, 0);
				return;
			}
			SetSeqEnable(i, true);

			// Read the sequence
			int Count = CModuleException::AssertRangeFmt(File.ReadInt32(), 0, 0xFF, "Sequence item count");

			if (iVersion < 20) {
				COldSequence OldSeq;
				for (int j = 0; j < Count; ++j) {
					char Length = File.ReadInt8();
					OldSeq.AddItem(Length, File.ReadInt8());
				}
				pSeq = OldSeq.Convert(i);
			}
			else {
				pSeq = std::make_shared<CSequence>(i);		// // //
				int Count2 = Count > MAX_SEQUENCE_ITEMS ? MAX_SEQUENCE_ITEMS : Count;
				pSeq->SetItemCount(Count2);
				pSeq->SetLoopPoint(CModuleException::AssertRangeFmt(
					static_cast<int>(File.ReadInt32()), -1, Count2 - 1, "Sequence loop point"));
				if (iVersion > 20) {
					pSeq->SetReleasePoint(CModuleException::AssertRangeFmt(
						static_cast<int>(File.ReadInt32()), -1, Count2 - 1, "Sequence release point"));
					if (iVersion >= 22)
						pSeq->SetSetting(static_cast<seq_setting_t>(File.ReadInt32()));
				}
				for (int j = 0; j < Count; ++j) {
					int8_t item = File.ReadInt8();
					if (j < Count2)
						pSeq->SetItem(j, item);
				}
			}
			if (GetSequence(i) && GetSequence(i)->GetItemCount() > 0)
				throw CModuleException::WithMessage("Document has no free sequence slot");
			m_pInstManager->SetSequence(m_iType, i, GetSeqIndex(i), pSeq);
		}
		catch (CModuleException e) {
			e.AppendError("At %s sequence,", GetSequenceName(value_cast(i)));
			throw e;
		}
	});
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
