/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
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

// This file handles playing of VRC6 channels

#include "stdafx.h"
#include "FamiTrackerTypes.h"		// // //
#include "APU/Types.h"		// // //
#include "Instrument.h"		// // //
#include "ChannelHandler.h"
#include "ChannelsVRC6.h"
#include "InstHandler.h"		// // //
#include "SeqInstHandler.h"		// // //

CChannelHandlerVRC6::CChannelHandlerVRC6() : CChannelHandler(0xFFF, 0x0F)
{
}

bool CChannelHandlerVRC6::HandleEffect(effect_t EffNum, unsigned char EffParam)
{
	switch (EffNum) {
	case EF_DUTY_CYCLE:
		m_iDefaultDuty = m_iDutyPeriod = EffParam;
		break;
	default: return CChannelHandler::HandleEffect(EffNum, EffParam);
	}

	return true;
}

void CChannelHandlerVRC6::HandleEmptyNote()
{
}

void CChannelHandlerVRC6::HandleCut()
{
	CutNote();
}

void CChannelHandlerVRC6::HandleRelease()
{
	if (!m_bRelease)
		ReleaseNote();
}

void CChannelHandlerVRC6::HandleNote(int Note, int Octave)
{
	m_iInstVolume  = 0x0F;
	m_iDutyPeriod = m_iDefaultDuty;
}

bool CChannelHandlerVRC6::CreateInstHandler(inst_type_t Type)
{
	switch (Type) {
	case INST_2A03: case INST_VRC6: case INST_N163: case INST_S5B: case INST_FDS:
		m_pInstHandler.reset(new CSeqInstHandler(this, 0x0F, Type == INST_S5B ? 0x40 : 0));
		return true;
	}
	return false;
}

void CChannelHandlerVRC6::ClearRegisters()		// // //
{
	uint16_t Address = ((m_iChannelID - CHANID_VRC6_PULSE1) << 12) + 0x9000;
	WriteRegister(Address, 0);
	WriteRegister(Address + 1, 0);
	WriteRegister(Address + 2, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// // // VRC6 Squares
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CVRC6Square::RefreshChannel()
{
	uint16_t Address = ((m_iChannelID - CHANID_VRC6_PULSE1) << 12) + 0x9000;

	unsigned int Period = CalculatePeriod();
	unsigned int Volume = CalculateVolume();
	unsigned char DutyCycle = m_iDutyPeriod << 4;

	unsigned char HiFreq = (Period & 0xFF);
	unsigned char LoFreq = (Period >> 8);
	
	if (!m_bGate) {		// // //
		WriteRegister(Address, DutyCycle);
		return;
	}

	WriteRegister(Address, DutyCycle | Volume);
	WriteRegister(Address + 1, HiFreq);
	WriteRegister(Address + 2, 0x80 | LoFreq);
}

int CVRC6Square::ConvertDuty(int Duty) const		// // //
{
	switch (m_iInstTypeCurrent) {
	case INST_2A03:	return DUTY_VRC6_FROM_2A03[Duty & 0x03];
	case INST_N163:	return Duty;
	case INST_S5B:	return 0x07;
	default:		return Duty;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// VRC6 Sawtooth
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CVRC6Sawtooth::RefreshChannel()
{
	unsigned int Period = CalculatePeriod();

	unsigned char HiFreq = (Period & 0xFF);
	unsigned char LoFreq = (Period >> 8);

	int Volume = (CalculateVolume() << 1) | ((m_iDutyPeriod & 1) << 5);		// // //

	if (Volume < 0)
		Volume = 0;
	if (Volume > 63)
		Volume = 63;
	
	if (!m_bGate) {		// // //
		WriteRegister(0xB000, 0);
		return;
	}

	WriteRegister(0xB000, Volume);
	WriteRegister(0xB001, HiFreq);
	WriteRegister(0xB002, 0x80 | LoFreq);
}
