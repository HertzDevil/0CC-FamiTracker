/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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

#include <utility>		// // //
#include "Types.h"
#include "../Common.h"
#include "../Blip_Buffer/blip_buffer.h"

enum chip_level_t {
	CHIP_LEVEL_APU1,
	CHIP_LEVEL_APU2,
	CHIP_LEVEL_VRC6,
	CHIP_LEVEL_VRC7,
	CHIP_LEVEL_MMC5,
	CHIP_LEVEL_FDS,
	CHIP_LEVEL_N163,
	CHIP_LEVEL_S5B
};

class CMixer
{
public:
	CMixer();
	~CMixer();

	void	AddValue(int ChanID, int Value, int FrameCycles);		// // //

	void	ExternalSound(int Chip);
	void	UpdateSettings(int LowCut,	int HighCut, int HighDamp, float OverallVol);

	bool	AllocateBuffer(unsigned int Size, uint32_t SampleRate, uint8_t NrChannels);
	void	SetClockRate(uint32_t Rate);
	void	ClearBuffer();
	int		FinishBuffer(int t);
	int		SamplesAvail() const;
	void	MixSamples(blip_sample_t *pBuffer, uint32_t Count);
	uint32_t	GetMixSampleCount(int t) const;

	void	AddSample(int ChanID, int Value);
	int		ReadBuffer(int Size, void *Buffer, bool Stereo);

	int32_t	GetChanOutput(uint8_t Chan) const;
	void	SetChipLevel(chip_level_t Chip, float Level);
	uint32_t	ResampleDuration(uint32_t Time) const;
	void	SetNamcoMixing(bool bLinear);		// // //
	void	SetNamcoVolume(float fVol);

	int		GetMeterDecayRate() const;		// // // 050B
	void	SetMeterDecayRate(int Rate);		// // // 050B

private:
	void StoreChannelLevel(int Channel, int Value);
	void ClearChannelLevels();

	float GetAttenuation() const;

private:
	// Blip buffer synths
	Blip_Synth<blip_good_quality, -500>		Synth2A03SS;
	Blip_Synth<blip_good_quality, -500>		Synth2A03TND;
	Blip_Synth<blip_good_quality, -500>		SynthVRC6;
	Blip_Synth<blip_good_quality, -130>		SynthMMC5;	
	Blip_Synth<blip_good_quality, -1600>	SynthN163;
	Blip_Synth<blip_good_quality, -3500>	SynthFDS;
	Blip_Synth<blip_good_quality, -1200>	SynthS5B;		// // // 050B
	
	// Blip buffer object
	Blip_Buffer	BlipBuffer;

	struct stLevels2A03SS {
		inline int GetDelta(int ChanID, int Value);
		inline void ResetDelta();
	private:
		int32_t sq1_ = 0;
		int32_t sq2_ = 0;
		double lastSum_ = 0.;
		inline double CalcPin() const;
	} levels2A03SS_;

	struct stLevels2A03TND {
		inline int GetDelta(int ChanID, int Value);
		inline void ResetDelta();
	private:
		int32_t tri_ = 0;
		int32_t noi_ = 0;
		int32_t dmc_ = 0;
		double lastSum_ = 0.;
		inline double CalcPin() const;
	} levels2A03TND_;

	struct stLevelsMono {
		inline int GetDelta(int /*ChanID*/, int Value) {
			lvl_ = Value;
			double Sum = CalcPin();
			return static_cast<int>(Sum - std::exchange(lastSum_, Sum));
		}
		inline void ResetDelta() {
			lvl_ = 0;
			lastSum_ = 0.;
		}
	private:
		int32_t lvl_ = 0;
		double lastSum_ = 0.;
		inline double CalcPin() {
			return lvl_;
		}
	};

	stLevelsMono levelsVRC6_;
	stLevelsMono levelsMMC5_;
	stLevelsMono levelsFDS_;
	stLevelsMono levelsN163_;
	stLevelsMono levelsS5B_;

	uint8_t		m_iExternalChip;
	uint32_t	m_iSampleRate;

	float		m_fChannelLevels[CHANNELS] = { };
	uint32_t	m_iChanLevelFallOff[CHANNELS] = { };

	int			m_iMeterDecayRate;		// // // 050B
	int			m_iLowCut;
	int			m_iHighCut;
	int			m_iHighDamp;
	float		m_fOverallVol;

	float		m_fLevelAPU1;
	float		m_fLevelAPU2;
	float		m_fLevelVRC6;
	float		m_fLevelMMC5;
	float		m_fLevelFDS;
	float		m_fLevelN163;
	float		m_fLevelS5B;		// // // 050B

	bool		m_bNamcoMixing;		// // //
};
