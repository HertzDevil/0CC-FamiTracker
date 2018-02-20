/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015  Jonathan Liss
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

// // // port of sn76489 emulation code

#include "APU/SoundChip.h"
#include "APU/Channel.h"
#include "APU/Types.h"

class CSN76489Channel : public CChannel
{
public:
	using CChannel::CChannel;

	virtual void	Reset();
	virtual void	Process(uint32_t Time) = 0;

//	virtual void	SetVGMWriter(const CVGMWriterBase *pWrite) final;

	void	SetStereo(bool Left, bool Right);

	void	SetAttenuation(uint8_t Value);
	int32_t	GetVolume() const;

protected:
	uint8_t m_iAttenuation = 0xFu;
	bool m_bLeft = true;
	bool m_bRight = true;
};



class CSNSquare final : public CSN76489Channel
{
public:
	CSNSquare(CMixer &Mixer, chan_id_t ID);
	~CSNSquare();

	void	Reset() override final;
	void	Process(uint32_t Time) override final;

	void	SetPeriodLo(uint8_t Value);
	void	SetPeriodHi(uint8_t Value);
	void	FlushPeriod() const;

	double	GetFrequency() const override;

	uint16_t	GetPeriod() const;

	static const uint16_t CUTOFF_PERIOD;

private:
	uint32_t	m_iSquareCounter;
	uint8_t	m_iSquarePeriodLo, m_iSquarePeriodHi;
	uint16_t	m_iPrevPeriod;
	bool	m_bSqaureActive;
};



enum class SN_NOISE_CFG
{
	Sega,
	BBCMicro,
	SN76494,
};

enum SN_noise_div_t
{
	SN_NOI_DIV_512  = 0x0,
	SN_NOI_DIV_1024 = 0x1,
	SN_NOI_DIV_2048 = 0x2,
	SN_NOI_DIV_CH3  = 0x3,
};

enum SN_noise_fb_t
{
	SN_NOI_FB_SHORT = 0x0,
	SN_NOI_FB_LONG  = 0x4,
};

class CSNNoise final : public CSN76489Channel
{
public:
	CSNNoise(CMixer &Mixer);
	~CSNNoise();

	void	Reset() override final;
	void	Process(uint32_t Time) override final;

	double	GetFrequency() const override;

	void	SetControlMode(uint8_t Value);
	void	Configure(SN_NOISE_CFG Mode);

	void	CachePeriod(uint16_t Period);

private:
	uint32_t	m_iSquareCounter;
	uint16_t	m_iLFSRState;
	uint16_t	m_iCH3Period;
	uint16_t	m_iFeedbackPattern;

	bool	m_bSqaureActive;

	SN_noise_div_t	m_iNoiseFreq;
	SN_noise_fb_t	m_iNoiseFeedback;

	static const uint16_t LFSR_INIT;
};



class CSN76489 : public CSoundChip
{
public:
	CSN76489(CMixer &Mixer);

	void	Reset() override;
	void	Process(uint32_t Time) override;
	void	EndFrame() override;

	void	Write(uint16_t Address, uint8_t Value) override;
	uint8_t	Read(uint16_t Address, bool &Mapped) override;

	void	ConfigureNoise(SN_NOISE_CFG Mode) const;

//	void	SetVGMWriter(const CVGMWriterBase *pWrite);

	static const uint16_t STEREO_PORT;
	static const uint16_t VOLUME_TABLE[16];

private:
	CSNSquare *GetSquare(unsigned ID) const;
	CSNNoise *GetNoise() const;

	void	UpdateNoisePeriod() const;

private:
//	const CVGMWriterBase *m_pVGMWriter = nullptr;
	std::unique_ptr<CSN76489Channel> m_pChannels[MAX_CHANNELS_SN76489];
	uint8_t	m_iAddressLatch;
};
