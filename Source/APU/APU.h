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

//#define LOGGING

#include "../Common.h"
#include <memory>		// // //
#include "Types_fwd.h"		// // //

// External classes
class C2A03;		// // //
class CVRC6;
class CVRC7;
class CFDS;
class CMMC5;
class CN163;
class CS5B;

namespace ft0cc::doc {
class dpcm_sample;
} // namespace ft0cc::doc
class CMixer;		// // //
class CSoundChip;		// // //
class CRegisterState;		// // //
enum chip_level_t : int;		// // //

#ifdef LOGGING
class CFile;
#endif

class CAPU {
public:
	explicit CAPU(IAudioCallback *pCallback = nullptr);		// // //
	~CAPU();

	void	Reset();
	void	Process();
	void	AddTime(int32_t Cycles);
	void	EndFrame();		// // // public

	void	SetExternalSound(sound_chip_t Chip);
	void	Write(uint16_t Address, uint8_t Value);		// // //
	uint8_t	Read(uint16_t Address);

	void	ChangeMachineRate(int Machine, int Rate);		// // //
	bool	SetupSound(int SampleRate, int NrChannels, int Speed);
	void	SetupMixer(int LowCut, int HighCut, int HighDamp, int Volume) const;
	void	SetCallback(IAudioCallback &pCallback);		// // //

	int32_t	GetVol(chan_id_t Chan) const;		// // //
	uint8_t	GetReg(sound_chip_t Chip, int Reg) const;
	double	GetFreq(sound_chip_t Chip, int Chan) const;		// // //
	CRegisterState *GetRegState(sound_chip_t Chip, int Reg) const;		// // //

	uint8_t	GetSamplePos() const;
	uint8_t	GetDeltaCounter() const;
	bool	DPCMPlaying() const;
	void	WriteSample(std::shared_ptr<const ft0cc::doc::dpcm_sample> pSample);		// // //
	void	ClearSample();		// // //

	void	SetChipLevel(chip_level_t Chip, float Level);

	void	SetNamcoMixing(bool bLinear);		// // //

	void	SetMeterDecayRate(decay_rate_t Type) const;		// // // 050B
	decay_rate_t GetMeterDecayRate() const;		// // // 050B

#ifdef LOGGING
	void	Log();
#endif

public:
	static const uint8_t	LENGTH_TABLE[];
	static const uint32_t	BASE_FREQ_NTSC;
	static const uint32_t	BASE_FREQ_PAL;
	static const uint8_t	FRAME_RATE_NTSC;
	static const uint8_t	FRAME_RATE_PAL;

private:
	static const int SEQUENCER_FREQUENCY;		// // //

	void StepSequence();		// // //

	void LogWrite(uint16_t Address, uint8_t Value);

private:
	std::unique_ptr<CMixer> m_pMixer;		// // //
	IAudioCallback *m_pParent;

	// Expansion chips
	std::unique_ptr<C2A03> m_p2A03;		// // //
	std::unique_ptr<CVRC6> m_pVRC6;
	std::unique_ptr<CMMC5> m_pMMC5;
	std::unique_ptr<CFDS> m_pFDS;
	std::unique_ptr<CN163> m_pN163;
	std::unique_ptr<CVRC7> m_pVRC7;
	std::unique_ptr<CS5B> m_pS5B;

	uint8_t		m_iExternalSoundChip;				// External sound chip, if used

	uint32_t	m_iSampleRate;						// // //
	uint32_t	m_iFrameClock;
	uint32_t	m_iCyclesToRun;						// Number of cycles to process

	uint32_t	m_iSoundBufferSamples;				// Size of buffer, in samples
	bool		m_bStereoEnabled;					// If stereo is enabled

	uint32_t	m_iSampleSizeShift;					// To convert samples to bytes
	uint32_t	m_iSoundBufferSize;					// Size of buffer, in samples
	uint32_t	m_iBufferPointer;					// Fill pos in buffer
	std::unique_ptr<int16_t[]> m_pSoundBuffer;			// // // Sound transfer buffer

	uint32_t	m_iFrameCycles;						// Cycles emulated from start of frame
	uint32_t	m_iSequencerClock;					// Clock for frame sequencer
	uint32_t	m_iSequencerNext;					// // // Next value for sequencer
	uint8_t		m_iSequencerCount;					// // // Step count for sequencer

	float		m_fLevelVRC7;
	// // // 050B removed

#ifdef LOGGING
	std::unique_ptr<CFile> m_pLog;		// // //
	int			  m_iFrame;
#endif
};
