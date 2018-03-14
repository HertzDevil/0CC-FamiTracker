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

class CMixer;
namespace ft0cc::doc {
class dpcm_sample;
} // namespace ft0cc::doc

#include <memory>		// // //
#include "APU/SoundChip.h"
#include "APU/Channel.h"

#include "APU/Square.h"		// // //
#include "APU/Triangle.h"
#include "APU/Noise.h"
#include "APU/DPCM.h"

class C2A03 : public CSoundChip
{
public:
	explicit C2A03(CMixer &Mixer);

	void Reset() override;
	void Process(uint32_t Time) override;
	void EndFrame() override;

	void Write(uint16_t Address, uint8_t Value) override;
	uint8_t Read(uint16_t Address, bool &Mapped) override;

	double GetFreq(int Channel) const override;		// // //

public:
	void	ClockSequence();		// // //

	void	ChangeMachine(int Machine);

	void	WriteSample(std::shared_ptr<const ft0cc::doc::dpcm_sample> pSample);		// // //
	void	ClearSample();		// // //
	uint8_t	GetSamplePos() const;
	uint8_t	GetDeltaCounter() const;
	bool	DPCMPlaying() const;

private:
	inline void Clock_240Hz();		// // //
	inline void Clock_120Hz();		// // //
	inline void Clock_60Hz();		// // //

	inline void RunAPU1(uint32_t Time);
	inline void RunAPU2(uint32_t Time);

private:
	CSquare		m_Square1;		// // //
	CSquare		m_Square2;
	CTriangle	m_Triangle;
	CNoise		m_Noise;
	CDPCM		m_DPCM;

	uint8_t		m_iFrameSequence = 0;		// Frame sequence
	uint8_t		m_iFrameMode = 0;			// 4 or 5-steps frame sequence

	std::shared_ptr<const ft0cc::doc::dpcm_sample> preview_sample_;		// // //
};
