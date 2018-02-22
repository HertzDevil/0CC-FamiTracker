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

#include "APU/MixerChannel.h"		// // //
#include "APU/MixerLevels.h"		// // //
#include "Common.h"
#include "Blip_Buffer/Blip_Buffer.h"
#include <array>		// // //
#include "SoundChipSet.h"		// // //

enum chip_level_t : unsigned char {
	CHIP_LEVEL_APU1,
	CHIP_LEVEL_APU2,
	CHIP_LEVEL_VRC6,
	CHIP_LEVEL_VRC7,
	CHIP_LEVEL_MMC5,
	CHIP_LEVEL_FDS,
	CHIP_LEVEL_N163,
	CHIP_LEVEL_S5B,
};

class CMixer
{
public:
	void	AddValue(chan_id_t ChanID, int Value, int FrameCycles);		// // //

	void	ExternalSound(CSoundChipSet Chip);		// // //
	void	UpdateSettings(int LowCut, int HighCut, int HighDamp, float OverallVol);

	bool	AllocateBuffer(unsigned int Size, uint32_t SampleRate, uint8_t NrChannels);
	void	SetClockRate(uint32_t Rate);
	void	ClearBuffer();
	int		FinishBuffer(int t);
	int		SamplesAvail() const;
	void	MixSamples(blip_sample_t *pBuffer, uint32_t Count);
	uint32_t	GetMixSampleCount(int t) const;

	void	AddSample(int ChanID, int Value);
	int		ReadBuffer(int Size, void *Buffer, bool Stereo);

	int32_t	GetChanOutput(chan_id_t Chan) const;		// // //
	void	SetChipLevel(chip_level_t Chip, float Level);
	uint32_t	ResampleDuration(uint32_t Time) const;
	void	SetNamcoMixing(bool bLinear);		// // //
	void	SetNamcoVolume(float fVol);

	decay_rate_t GetMeterDecayRate() const;		// // // 050B
	void	SetMeterDecayRate(decay_rate_t Rate);		// // // 050B

private:
	void UpdateMeters();		// // //
	void StoreChannelLevel(chan_id_t Channel, int Level);		// // //
	void ClearChannelLevels();

	float GetAttenuation() const;

private:
	// Blip buffer object
	Blip_Buffer	BlipBuffer;

	CMixerChannel<stLevels2A03SS>  levels2A03SS_  {500};		// // //
	CMixerChannel<stLevels2A03TND> levels2A03TND_ {500};
	CMixerChannel<stLevelsVRC6>    levelsVRC6_    {500};
	CMixerChannel<stLevelsFDS>     levelsFDS_     {3500};
	CMixerChannel<stLevelsMMC5>    levelsMMC5_    {130};
	CMixerChannel<stLevelsN163>    levelsN163_    {1600};
	CMixerChannel<stLevelsS5B>     levelsS5B_     {1200};

	CSoundChipSet m_iExternalChip;
	uint32_t	m_iSampleRate = 0;

	std::array<float, CHANID_COUNT>		m_fChannelLevels = { };
	std::array<float, CHANID_COUNT>		m_fChannelLevelsLast = { };		// // //
	std::array<uint32_t, CHANID_COUNT>	m_iChanLevelFallOff = { };

	decay_rate_t m_iMeterDecayRate = DECAY_SLOW;		// // // 050B
	int			m_iLowCut = 0;
	int			m_iHighCut = 0;
	int			m_iHighDamp = 0;
	float		m_fOverallVol = 1.f;

	bool		m_bNamcoMixing = false;		// // //
};
