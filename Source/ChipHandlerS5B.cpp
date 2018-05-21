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

#include "ChipHandlerS5B.h"
#include "APU/APUInterface.h"
#include "SongState.h"
#include "PatternNote.h" // ft0cc::doc::effect_command

void CChipHandlerS5B::SetChannelOutput(unsigned Subindex, int Square, int Noise) {
	switch (Subindex) {
	case 0:
		m_iModes &= 0x36;
		break;
	case 1:
		m_iModes &= 0x2D;
		break;
	case 2:
		m_iModes &= 0x1B;
		break;
	}

	m_iModes |= (Noise << (3 + Subindex)) | (Square << Subindex);
}

void CChipHandlerS5B::SetNoisePeriod(uint8_t period) {
	m_iNoiseFreq = period & 0x1Fu;
}

void CChipHandlerS5B::SetDefaultNoisePeriod(uint8_t period) {
	SetNoisePeriod(period);
	m_iDefaultNoise = m_iNoiseFreq;
}

void CChipHandlerS5B::RestoreNoisePeriod() {
	m_iNoiseFreq = m_iDefaultNoise;
}

uint16_t CChipHandlerS5B::GetEnvelopePeriod() const {
	return m_iEnvFreq;
}

void CChipHandlerS5B::SetEnvelopePeriod(uint16_t period) {
	m_iEnvFreq = period;
}

void CChipHandlerS5B::SetEnvelopeShape(uint8_t shape) {
	m_bEnvTrigger = true;		// // // 050B
	m_iEnvType = shape & 0x0F;
}

void CChipHandlerS5B::TriggerEnvelope() {
	m_bEnvTrigger = true;
}

std::string CChipHandlerS5B::GetCustomEffectString() const {
	std::string str;

	if (auto lo = static_cast<uint8_t>(m_iEnvFreq & 0xFFu))
		str += MakeCommandString({ft0cc::doc::effect_type::SUNSOFT_ENV_LO, lo});
	if (auto hi = static_cast<uint8_t>(m_iEnvFreq >> 8))
		str += MakeCommandString({ft0cc::doc::effect_type::SUNSOFT_ENV_HI, hi});
	if (m_iEnvType)
		str += MakeCommandString({ft0cc::doc::effect_type::SUNSOFT_ENV_TYPE, static_cast<uint8_t>(m_iEnvType)});
	if (m_iDefaultNoise)
		str += MakeCommandString({ft0cc::doc::effect_type::SUNSOFT_NOISE, static_cast<uint8_t>(m_iDefaultNoise)});

	return str;
}

void CChipHandlerS5B::ResetChip(CAPUInterface &apu) {
	m_iModes = 0x3Fu;
	m_iDefaultNoise = 0;		// // //
	m_iNoiseFreq = 0;		// // //
	m_iNoisePrev = -1;		// // //
	m_iEnvFreq = 0u;
	m_iEnvType = 0;
	m_i5808B4 = 0;		// // // 050B
	m_bEnvTrigger = false;
}

void CChipHandlerS5B::RefreshAfter(CAPUInterface &apu) {
	if (m_iNoiseFreq != m_iNoisePrev)		// // //
		WriteReg(apu, 0x06, (m_iNoisePrev = m_iNoiseFreq) ^ 0x1F);
	WriteReg(apu, 0x07, m_iModes);
	WriteReg(apu, 0x0B, m_iEnvFreq & 0xFF);
	WriteReg(apu, 0x0C, m_iEnvFreq >> 8);
	if (m_bEnvTrigger)		// // // 050B
		WriteReg(apu, 0x0D, m_iEnvType);
	m_bEnvTrigger = false;
}

void CChipHandlerS5B::WriteReg(CAPUInterface &apu, uint8_t adr, uint8_t val) const {
	apu.Write(0xC000, adr);
	apu.Write(0xE000, val);
}
