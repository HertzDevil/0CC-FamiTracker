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

#include "SeqInstHandler.h"
#include "FamiTrackerEnv.h"
#include "APU/Types.h"
#include "FamiTrackerTypes.h"
#include "SoundGen.h"

#include "SeqInstrument.h"
#include "ChannelHandlerInterface.h"

/*
 * Class CSeqInstHandler
 */

CSeqInstHandler::CSeqInstHandler(CChannelHandlerInterface *pInterface, int Vol, int Duty) :
	m_SequenceInfo {
		{sequence_t::Volume, { }},
		{sequence_t::Arpeggio, { }},
		{sequence_t::Pitch, { }},
		{sequence_t::HiPitch, { }},
		{sequence_t::DutyCycle, { }},
	},
	CInstHandler(pInterface, Vol),
	m_iDefaultDuty(Duty)
{
}

void CSeqInstHandler::LoadInstrument(std::shared_ptr<CInstrument> pInst)
{
	m_pInstrument = pInst;
	auto pSeqInst = std::dynamic_pointer_cast<CSeqInstrument>(pInst);
	if (!pSeqInst)
		return;

	for (auto &[seqType, info] : m_SequenceInfo)
		if (!pSeqInst->GetSeqEnable(seqType))
			ClearSequence(seqType);
		else {
			const auto pSequence = pSeqInst->GetSequence(seqType);
			if (pSequence != info.m_pSequence || info.m_iSeqState == seq_state_t::Disabled)
				SetupSequence(seqType, std::move(pSequence));
		}
}

void CSeqInstHandler::TriggerInstrument()
{
	for (auto &x : m_SequenceInfo)
		x.second.Trigger();

	m_iVolume = m_iDefaultVolume;
	m_iNoteOffset = 0;
	m_iPitchOffset = 0;
	m_iDutyParam = m_iDefaultDuty;

	if (m_pInterface->IsActive()) {
		m_pInterface->SetVolume(m_iDefaultVolume);
//		m_pInterface->SetDutyPeriod(m_iDefaultDuty); // not same
	}
}

void CSeqInstHandler::ReleaseInstrument()
{
	if (!m_pInterface->IsReleasing())
		for (auto &x : m_SequenceInfo)
			x.second.Release();
}

void CSeqInstHandler::UpdateInstrument()
{
	if (!m_pInterface->IsActive())
		return;
	for (auto &[_, info] : m_SequenceInfo) {
		(void)_;
		const auto &pSeq = info.m_pSequence;
		if (!pSeq || pSeq->GetItemCount() == 0)
			continue;
		switch (info.m_iSeqState) {
		case seq_state_t::Running:
			ProcessSequence(*pSeq, info.m_iSeqPointer);
			info.Step(m_pInterface->IsReleasing());
			break;

		case seq_state_t::End:
			switch (pSeq->GetSequenceType()) {
			case sequence_t::Arpeggio:
				if (pSeq->GetSetting() == SETTING_ARP_FIXED)
					m_pInterface->SetPeriod(m_pInterface->TriggerNote(m_pInterface->GetNote()));
				break;
			}
			info.m_iSeqState = seq_state_t::Halt;
			Env.GetSoundGenerator()->SetSequencePlayPos(pSeq, -1);
			break;

		case seq_state_t::Halt:
		case seq_state_t::Disabled:
			break;
		}
	}
}

bool CSeqInstHandler::ProcessSequence(const CSequence &Seq, int Pos)
{
	int Value = Seq.GetItem(Pos);
	seq_setting_t Setting = Seq.GetSetting();

	switch (Seq.GetSequenceType()) {
	// Volume modifier
	case sequence_t::Volume:
		m_pInterface->SetVolume(Value);
		return true;
	// Arpeggiator
	case sequence_t::Arpeggio:
		switch (Setting) {
		case SETTING_ARP_ABSOLUTE:
			m_pInterface->SetPeriod(m_pInterface->TriggerNote(m_pInterface->GetNote() + Value));
			return true;
		case SETTING_ARP_FIXED:
			m_pInterface->SetPeriod(m_pInterface->TriggerNote(Value));
			return true;
		case SETTING_ARP_RELATIVE:
			m_pInterface->SetNote(m_pInterface->GetNote() + Value);
			m_pInterface->SetPeriod(m_pInterface->TriggerNote(m_pInterface->GetNote()));
			return true;
		case SETTING_ARP_SCHEME:		// // //
			if (Value < 0) Value += 256;
			int lim = Value % 0x40, scheme = Value / 0x40;
			if (lim > ARPSCHEME_MAX)
				lim -= 64;
			{
				unsigned char Param = m_pInterface->GetArpParam();
				switch (scheme) {
				case 0: break;
				case 1: lim += Param >> 4;   break;
				case 2: lim += Param & 0x0F; break;
				case 3: lim -= Param & 0x0F; break;
				}
			}
			m_pInterface->SetPeriod(m_pInterface->TriggerNote(m_pInterface->GetNote() + lim));
			return true;
		}
		return false;
	// Pitch
	case sequence_t::Pitch:
		switch (Setting) {		// // //
		case SETTING_PITCH_RELATIVE:
			m_pInterface->SetPeriod(m_pInterface->GetPeriod() + Value);
			return true;
		case SETTING_PITCH_ABSOLUTE:		// // // 050B
			m_pInterface->SetPeriod(m_pInterface->TriggerNote(m_pInterface->GetNote()) + Value);
			return true;
		}
		return false;
	// Hi-pitch
	case sequence_t::HiPitch:
		m_pInterface->SetPeriod(m_pInterface->GetPeriod() + (Value << 4));
		return true;
	// Duty cycling
	case sequence_t::DutyCycle:
		m_pInterface->SetDutyPeriod(Value);
		return true;
	}
	return false;
}

void CSeqInstHandler::SetupSequence(sequence_t Index, std::shared_ptr<const CSequence> pSequence)		// // //
{
	m_SequenceInfo[Index] = {std::move(pSequence), seq_state_t::Running, 0};
}

void CSeqInstHandler::ClearSequence(sequence_t Index)
{
	m_SequenceInfo[Index] = seq_info_t { };		// // //
}

void CSeqInstHandler::seq_info_t::Trigger() {
	if (m_pSequence) {
		m_iSeqState = seq_state_t::Running;
		m_iSeqPointer = 0;
	}
}

void CSeqInstHandler::seq_info_t::Release() {
	if (m_pSequence && (m_iSeqState == seq_state_t::Running || m_iSeqState == seq_state_t::End)) {
		int ReleasePoint = m_pSequence->GetReleasePoint();
		if (ReleasePoint != -1) {
			m_iSeqPointer = ReleasePoint;
			m_iSeqState = seq_state_t::Running;
		}
	}
}

void CSeqInstHandler::seq_info_t::Step(bool isReleasing) {
	++m_iSeqPointer;
	int Release = m_pSequence->GetReleasePoint();
	int Items = m_pSequence->GetItemCount();
	int Loop = m_pSequence->GetLoopPoint();
	if (m_iSeqPointer == (Release + 1) || m_iSeqPointer >= Items) {
		// End point reached
		if (Loop != -1 && !(isReleasing && Release != -1) && Loop < Release) {		// // //
			m_iSeqPointer = Loop;
		}
		else if (m_iSeqPointer >= Items) {
			// End of sequence
			if (Loop >= Release && Loop != -1)		// // //
				m_iSeqPointer = Loop;
			else
				m_iSeqState = seq_state_t::End;
		}
		else if (!isReleasing) {
			// Waiting for release
			--m_iSeqPointer;
		}
	}
	Env.GetSoundGenerator()->SetSequencePlayPos(m_pSequence, m_iSeqPointer);
}
