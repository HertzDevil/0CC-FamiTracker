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

#include "ChipHandler.h"
#include <cstdint>
#include <string>

class CChipHandlerS5B : public CChipHandler {
public:
	void SetChannelOutput(unsigned Subindex, int Square, int Noise);

	void SetNoisePeriod(uint8_t period);
	void SetDefaultNoisePeriod(uint8_t period);
	void RestoreNoisePeriod();

	uint16_t GetEnvelopePeriod() const;
	void SetEnvelopePeriod(uint16_t period);
	void SetEnvelopeShape(uint8_t shape);
	void TriggerEnvelope();

	std::string GetCustomEffectString() const;

private:
	void ResetChip(CAPUInterface &apu) override;
	void RefreshAfter(CAPUInterface &apu) override;

	void WriteReg(CAPUInterface &apu, uint8_t adr, uint8_t val) const;

	int			m_iNoiseFreq	= 0;
	int			m_iNoisePrev	= -1;
	int			m_iDefaultNoise	= 0;		// // //
	uint16_t	m_iEnvFreq		= 0u;		// // //
	uint8_t		m_iModes		= 0x3Fu;
	bool		m_bEnvTrigger	= false;		// // // 050B
	int			m_iEnvType		= 0;
	int			m_i5808B4		= 0;		// // // 050B
};
