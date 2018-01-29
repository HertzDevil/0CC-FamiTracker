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

#include "SoundChip.h"
#include "Channel.h"
#include "Types.h"		// // //

class CMixer;

class CN163;		// // //

class CN163Chan : public CChannel {
public:
	CN163Chan(CMixer &Mixer, CN163 &parent, chan_id_t ID, uint8_t *pWaveData);		// // //

	void Reset();
	void Write(uint16_t Address, uint8_t Value);

	void Process(uint32_t Time, uint8_t ChannelsActive);		// // //
	void ProcessClean(uint32_t Time, uint8_t ChannelsActive);		// // //

	uint8_t ReadMem(uint8_t Reg);
	void ResetCounter();
	double GetFrequency() const;		// // //

private:
	uint32_t	m_iCounter, m_iFrequency;
	uint32_t	m_iPhase;
	uint32_t	m_iWaveLength;

	uint8_t		m_iVolume;
	uint8_t		m_iWaveOffset;
	uint8_t		*m_pWaveData;

	uint8_t		m_iLastSample;

	CN163		&parent_;
};

class CN163 : public CSoundChip {
public:
	explicit CN163(CMixer &Mixer);

	void Reset() override;
	void Process(uint32_t Time) override;
	void EndFrame() override;
	void Write(uint16_t Address, uint8_t Value) override;
	void Log(uint16_t Address, uint8_t Value) override;		// // //
	double GetFreq(int Channel) const override;		// // //

	uint8_t Read(uint16_t Address, bool &Mapped);
	uint8_t ReadMem(uint8_t Reg);
	void Mix(int32_t Value, uint32_t Time, chan_id_t ChanID);		// // //
	void SetMixingMethod(bool bLinear);		// // //

protected:
	void ProcessOld(uint32_t Time);		// // //

private:
	CN163Chan	m_Channels[MAX_CHANNELS_N163];		// // //

	uint8_t		m_iWaveData[0x80] = { };		// // //
	uint8_t		m_iExpandAddr = 0;
	uint8_t		m_iChansInUse = 0;

	int32_t		m_iLastValue = 0;

	uint32_t	m_iGlobalTime = 0;

	uint32_t	m_iChannelCntr = 0;
	uint32_t	m_iActiveChan = 0;
	uint32_t	m_iLastChan = 0;		// // //
	uint32_t	m_iCycle = 0;

	bool		m_bOldMixing = false;		// // //
};
