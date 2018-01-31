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

#include "APU/SoundChip.h"
#include "APU/ext/emu2413.h"		// // //
#include <vector>		// // //

struct OPLL_deleter {
	void operator()(void *ptr) {
		OPLL_delete(static_cast<OPLL *>(ptr));
	}
};

class CVRC7 : public CSoundChip {
public:
	explicit CVRC7(CMixer &Mixer);

	void SetSampleSpeed(uint32_t SampleRate, double ClockRate, uint32_t FrameRate);
	void SetVolume(float Volume);

	void Reset() override;
	void Write(uint16_t Address, uint8_t Value) override;
	void Log(uint16_t Address, uint8_t Value) override;		// // //
	uint8_t Read(uint16_t Address, bool &Mapped) override;
	void EndFrame() override;
	void Process(uint32_t Time) override;

	double GetFreq(int Channel) const override;		// // //

protected:
	static const float  AMPLIFY;
	static const uint32_t OPL_CLOCK;

private:
	std::unique_ptr<OPLL, OPLL_deleter> m_pOPLLInt;		// // //
	uint32_t	m_iTime;

	uint32_t	m_iMaxSamples = 0;
	std::vector<int16_t> m_iBuffer;		// // //
	uint32_t	m_iBufferPtr;

	float		m_fVolume = 1.f;

	uint8_t		m_iSoundReg = 0;
};
