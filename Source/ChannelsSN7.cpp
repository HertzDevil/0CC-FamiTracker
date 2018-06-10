/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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

#include "ChannelsSN7.h"
#include "SeqInstHandler.h"
#include "Instrument.h"
#include "APU/APUInterface.h"
#include "APU/SN76489.h"

unsigned CChannelHandlerSN7::m_iRegisterPos[] = {		// // //
	value_cast(sn76489_subindex_t::square1),
	value_cast(sn76489_subindex_t::square2),
	value_cast(sn76489_subindex_t::square3),
};
uint8_t CChannelHandlerSN7::m_cStereoFlag = 0xFFu;		// // //
uint8_t CChannelHandlerSN7::m_cStereoFlagLast = 0xFFu;		// // //

CChannelHandlerSN7::CChannelHandlerSN7(stChannelID ch) :
	CChannelHandler(ch, 0x3FF, 0x0F),		// // //
	m_bManualVolume(0),
	m_iInitVolume(0),
	// // //
	m_iPostEffect(0),
	m_iPostEffectParam(0)
{
}

bool CChannelHandlerSN7::HandleEffect(ft0cc::doc::effect_command cmd)
{
	switch (cmd.fx) {
	case ft0cc::doc::effect_type::SN76489_CONTROL:
		if (cmd.param >= 0x00 && cmd.param <= 0x1F)
			SetStereo(cmd.param && cmd.param <= 0x10, cmd.param >= 0x10);
		break;
	default: return CChannelHandler::HandleEffect(cmd);
	}

	return true;
}

bool CChannelHandlerSN7::CreateInstHandler(inst_type_t Type)
{
	switch (Type) {
	case INST_2A03: case INST_VRC6: case INST_N163: case INST_S5B: case INST_FDS: case INST_SN76489:
		switch (m_iInstTypeCurrent) {
		case INST_2A03: case INST_VRC6: case INST_N163: case INST_S5B: case INST_FDS: case INST_SN76489: break;
		default:
			m_pInstHandler = std::make_unique<CSeqInstHandler>(this, 0x0F, Type == INST_S5B ? 0x40 : 0);
			return true;
		}
	}
	return false;
}

void CChannelHandlerSN7::HandleEmptyNote()
{
}

void CChannelHandlerSN7::HandleCut()
{
	CutNote();
}

void CChannelHandlerSN7::HandleRelease()
{
	if (!m_bRelease)
		ReleaseNote();
}

int CChannelHandlerSN7::CalculateVolume() const		// // //
{
	return LimitVolume((m_iVolume >> VOL_COLUMN_SHIFT) + m_iInstVolume - 15 - GetTremolo());
}

void CChannelHandlerSN7::SetStereo(bool Left, bool Right) const
{
	auto index = m_iChannelID.Subindex;
	m_cStereoFlag &= ~(0x11 << index);
	if (Left)
		m_cStereoFlag |= ((uint8_t)0x10) << index;
	if (Right)
		m_cStereoFlag |= ((uint8_t)0x01) << index;
}

void CChannelHandlerSN7::SwapChannels(stChannelID ID)		// // //
{
	if (ID.Chip == sound_chip_t::SN76489) {
		m_iRegisterPos[value_cast(sn76489_subindex_t::square1)] = value_cast(sn76489_subindex_t::square1);
		m_iRegisterPos[value_cast(sn76489_subindex_t::square2)] = value_cast(sn76489_subindex_t::square2);
		m_iRegisterPos[value_cast(sn76489_subindex_t::square3)] = ID.Subindex;
		m_iRegisterPos[ID.Subindex] = value_cast(sn76489_subindex_t::square3);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Square
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CSN7SquareChan::RefreshChannel()
{
	// TODO: move this into an appropriate chip handler class
	if (m_iChannelID.Subindex == value_cast(sn76489_subindex_t::square1) && m_cStereoFlagLast != m_cStereoFlag)
		m_pAPU->Write(/* CSN76489::STEREO_PORT */ 0x4F, m_cStereoFlagLast = m_cStereoFlag);

	int Period = CalculatePeriod();
	int Volume = CalculateVolume();
	// // //
	unsigned char HiFreq = (Period >> 4) & 0x3F;
	unsigned char LoFreq = (Period & 0xF);

	const uint16_t Base = m_iRegisterPos[m_iChannelID.Subindex] * 2;		// // //
	m_pAPU->Write(Base, LoFreq);
	m_pAPU->Write(CSN76489::LATCH_PORT, HiFreq); // double-byte
	m_pAPU->Write(Base + 1, 0xF ^ Volume);
}

void CSN7SquareChan::ClearRegisters()
{
	const uint16_t Base = m_iRegisterPos[m_iChannelID.Subindex] * 2;		// // //
	m_pAPU->Write(Base, 0x00);
	m_pAPU->Write(CSN76489::LATCH_PORT, 0x00); // double-byte
	m_pAPU->Write(Base + 1, 0xF);
	m_iRegisterPos[m_iChannelID.Subindex] = m_iChannelID.Subindex;
	SetStereo(true, true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Noise
///////////////////////////////////////////////////////////////////////////////////////////////////////////

CSN7NoiseChan::CSN7NoiseChan(stChannelID ch) : CChannelHandlerSN7(ch)
{
	m_iDefaultDuty = 0;
}

void CSN7NoiseChan::HandleNote(int MidiNote)
{
	CChannelHandler::HandleNote(MidiNote);

	int NewNote = MidiNote;
	int NesFreq = TriggerNote(NewNote);

//	NewNote &= 0x0F;

	if (m_iPortaSpeed > 0 && m_iEffect == ft0cc::doc::effect_type::PORTAMENTO) {
		if (m_iPeriod == 0)
			m_iPeriod = NesFreq;
		m_iPortaTo = NesFreq;
	}
	else
		m_iPeriod = NesFreq;

	m_bGate = true;
	m_bTrigger = true;		// // //

	m_iNote = NewNote;
}

void CSN7NoiseChan::SetupSlide()
{
	static const auto GetSlideSpeed = [] (unsigned char param) {
		return ((param & 0xF0) >> 3) + 1;
	};

	switch (m_iEffect) {
	case ft0cc::doc::effect_type::PORTAMENTO:
		m_iPortaSpeed = m_iEffectParam;
		break;
	case ft0cc::doc::effect_type::SLIDE_UP:
		m_iNote += m_iEffectParam & 0xF;
		m_iPortaSpeed = GetSlideSpeed(m_iEffectParam);
		break;
	case ft0cc::doc::effect_type::SLIDE_DOWN:
		m_iNote -= m_iEffectParam & 0xF;
		m_iPortaSpeed = GetSlideSpeed(m_iEffectParam);
		break;
	}

	m_iPortaTo = m_iNote;
}

/*
int CSN7NoiseChan::CalculatePeriod() const
{
	return LimitPeriod(m_iPeriod - GetVibrato() + GetFinePitch() + GetPitch());
}
*/

void CSN7NoiseChan::RefreshChannel()
{
	int Period = CalculatePeriod();
	int Volume = CalculateVolume();
	char NoiseMode = !(m_iDutyPeriod & 0x01);		// // //

	int newCtrl = (NoiseMode << 2) | Period;		// // //
	if ((m_bTrigger && m_bNoiseReset) || newCtrl != m_iLastCtrl) {
		m_pAPU->Write(0x06, newCtrl);
		m_iLastCtrl = newCtrl;
	}
	m_pAPU->Write(0x07, 0xF ^ Volume);

	m_bTrigger = false;
}

bool CSN7NoiseChan::HandleEffect(ft0cc::doc::effect_command cmd)
{
	switch (cmd.fx) {
	case ft0cc::doc::effect_type::SN76489_CONTROL:
		switch (cmd.param) {
		case 0xE0: m_bNoiseReset = false; return true;
		case 0xE1: m_bNoiseReset = m_bTrigger = true; return true;
		}
	default: return CChannelHandlerSN7::HandleEffect(cmd);
	}
}

void CSN7NoiseChan::ClearRegisters()
{
	m_iLastCtrl = -1;		// // //
	m_pAPU->Write(0x06, 0);
	m_pAPU->Write(0x07, 0xF);
	SetStereo(true, true);
	m_bNoiseReset = false;
}

int CSN7NoiseChan::TriggerNote(int Note)
{
	// Clip range to 0-15
	/*
	if (Note > 0x0F)
		Note = 0x0F;
	if (Note < 0)
		Note = 0;
		*/

	RegisterKeyState(Note);

//	Note &= 0x0F;

	return (2 - Note) & 0x03;		// // //
}

// // //
