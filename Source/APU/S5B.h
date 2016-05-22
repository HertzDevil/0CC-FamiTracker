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


#pragma once

#include "SoundChip.h"
#include "Channel.h"

class CS5B : public CSoundChip {
public:
	CS5B(CMixer *pMixer);
	virtual ~CS5B();
	void	Reset();
	void	Process(uint32_t Time);
	void	EndFrame();
	void	Write(uint16_t Address, uint8_t Value);
	void	Log(uint16_t Address, uint8_t Value);		// // //
	uint8_t 	Read(uint16_t Address, bool &Mapped);
	void	SetSampleSpeed(uint32_t SampleRate, double ClockRate, uint32_t FrameRate);
	void	SetVolume(float fVol);
	double	GetFreq(int Channel) const;		// // //
//	void	SetChannelVolume(int Chan, int LevelL, int LevelR);
protected:
	void	GetMixMono();
private:
	static float AMPLIFY;
private:
	uint8_t	m_iRegister;

	uint32_t	m_iTime;

	float	m_fVolume;

};
