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

// MMC5 file

#include "ChannelsMMC5.h"
#include "APU/Types.h"		// // //
#include "APU/APUInterface.h"		// // //
#include "Instrument.h"		// // //
#include "InstHandler.h"		// // //
#include "SeqInstHandler.h"		// // //
#include "SongState.h"		// // //

CChannelHandlerMMC5::CChannelHandlerMMC5(stChannelID ch) : CChannelHandler(ch, 0x7FF, 0x0F)		// // //
{
	m_bHardwareEnvelope = false;		// // //
	m_bEnvelopeLoop = true;
	m_bResetEnvelope = false;
	m_iLengthCounter = 1;
}

void CChannelHandlerMMC5::HandleNoteData(stChanNote &NoteData)		// // //
{
	// // //
	CChannelHandler::HandleNoteData(NoteData);

	if (ft0cc::doc::is_note(NoteData.Note) || NoteData.Note == note_t::echo) {
		if (!m_bEnvelopeLoop || m_bHardwareEnvelope)		// // //
			m_bResetEnvelope = true;
	}
}

bool CChannelHandlerMMC5::HandleEffect(stEffectCommand cmd)
{
	switch (cmd.fx) {
	case effect_t::VOLUME:
		if (cmd.param < 0x20) {		// // //
			m_iLengthCounter = cmd.param;
			m_bEnvelopeLoop = false;
			m_bResetEnvelope = true;
		}
		else if (cmd.param >= 0xE0 && cmd.param < 0xE4) {
			if (!m_bEnvelopeLoop || !m_bHardwareEnvelope)
				m_bResetEnvelope = true;
			m_bHardwareEnvelope = ((cmd.param & 0x01) == 0x01);
			m_bEnvelopeLoop = ((cmd.param & 0x02) != 0x02);
		}
		break;
	case effect_t::DUTY_CYCLE:
		m_iDefaultDuty = m_iDutyPeriod = cmd.param;
		break;
	default: return CChannelHandler::HandleEffect(cmd);
	}

	return true;
}

void CChannelHandlerMMC5::HandleEmptyNote()
{
}

void CChannelHandlerMMC5::HandleCut()
{
	CutNote();
}

void CChannelHandlerMMC5::HandleRelease()
{
	if (!m_bRelease)
		ReleaseNote();
}

bool CChannelHandlerMMC5::CreateInstHandler(inst_type_t Type)
{
	switch (Type) {
	case INST_2A03: case INST_VRC6: case INST_N163: case INST_S5B: case INST_FDS:
		switch (m_iInstTypeCurrent) {
		case INST_2A03: case INST_VRC6: case INST_N163: case INST_S5B: case INST_FDS: break;
		default:
			m_pInstHandler = std::make_unique<CSeqInstHandler>(this, 0x0F, Type == INST_S5B ? 0x40 : 0);
			return true;
		}
	}
	return false;
}

void CChannelHandlerMMC5::ResetChannel()
{
	CChannelHandler::ResetChannel();
	m_bEnvelopeLoop = true;		// // //
	m_bHardwareEnvelope = false;
	m_iLengthCounter = 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// // // MMC5 Channels
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CChannelHandlerMMC5::RefreshChannel()		// // //
{
	int Period = CalculatePeriod();
	int Volume = CalculateVolume();
	char DutyCycle = (m_iDutyPeriod & 0x03);

	unsigned char HiFreq		= (Period & 0xFF);
	unsigned char LoFreq		= (Period >> 8);
	unsigned int  Offs			= 0x5000 + 4 * GetChannelID().Subindex;

	m_pAPU->Write(0x5015, 0x03);

	if (m_bGate)		// // //
		m_pAPU->Write(Offs, (DutyCycle << 6) | (m_bEnvelopeLoop << 5) | (!m_bHardwareEnvelope << 4) | Volume);
	else {
		m_pAPU->Write(Offs, 0x30);
		m_iLastPeriod = 0xFFFF;
		return;
	}
	m_pAPU->Write(Offs + 2, HiFreq);
	if (LoFreq != (m_iLastPeriod >> 8) || m_bResetEnvelope)		// // //
		m_pAPU->Write(Offs + 3, LoFreq + (m_iLengthCounter << 3));

	m_iLastPeriod = Period;		// // //
	m_bResetEnvelope = false;
}

int CChannelHandlerMMC5::ConvertDuty(int Duty) const		// // //
{
	switch (m_iInstTypeCurrent) {
	case INST_VRC6:	return DUTY_2A03_FROM_VRC6[Duty & 0x07];
	case INST_S5B:	return 0x02;
	default:		return Duty;
	}
}

void CChannelHandlerMMC5::ClearRegisters()
{
	unsigned Offs = 0x5000 + 4 * GetChannelID().Subindex;		// // //
	m_pAPU->Write(Offs, 0x30);
	m_pAPU->Write(Offs + 2, 0);
	m_pAPU->Write(Offs + 3, 0);
	m_iLastPeriod = 0xFFFF;		// // //
}

std::string CChannelHandlerMMC5::GetCustomEffectString() const		// // //
{
	std::string str;

	if (!m_bEnvelopeLoop)
		str += MakeCommandString({effect_t::VOLUME, static_cast<uint8_t>(m_iLengthCounter)});
	if (!m_bEnvelopeLoop || m_bHardwareEnvelope)
		str += MakeCommandString({effect_t::VOLUME, static_cast<uint8_t>(0xE0 + !m_bEnvelopeLoop * 2 + m_bHardwareEnvelope)});

	return str;
}
