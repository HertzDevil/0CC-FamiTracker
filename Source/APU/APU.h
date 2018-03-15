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

#include "Common.h"
#include <memory>		// // //
#include <vector>		// // //
#include "SoundChipSet.h"		// // //
#include "APUInterface.h"		// // //

namespace ft0cc::doc {
class dpcm_sample;
} // namespace ft0cc::doc
class CMixer;		// // //
class CSoundChip;		// // //
class CRegisterState;		// // //
enum chip_level_t : unsigned char;		// // //

#ifdef LOGGING
class CFile;
#endif

class CAPU : public CAPUInterface {
public:
	explicit CAPU(IAudioCallback *pCallback = nullptr);		// // //
	~CAPU();

	void	Reset();
	void	Process();
	void	AddTime(int32_t Cycles);
	void	EndFrame();		// // // public

	void	SetExternalSound(CSoundChipSet Chips);
	void	Write(uint16_t Address, uint8_t Value) override;		// // //
	uint8_t	Read(uint16_t Address);

	void	ChangeMachineRate(int Machine, int Rate);		// // //
	bool	SetupSound(int SampleRate, int NrChannels, int Speed);
	void	SetupMixer(int LowCut, int HighCut, int HighDamp, int Volume) const;
	void	SetCallback(IAudioCallback &pCallback);		// // //

	int32_t	GetVol(chan_id_t Chan) const;		// // //
	uint8_t	GetReg(sound_chip_t Chip, int Reg) const;
	double	GetFreq(sound_chip_t Chip, int Chan) const;		// // //
	CRegisterState *GetRegState(sound_chip_t Chip, int Reg) const;		// // //

	void	SetChipLevel(chip_level_t Chip, float Level);

	void	SetNamcoMixing(bool bLinear);		// // //

	void	SetMeterDecayRate(decay_rate_t Type) const;		// // // 050B
	decay_rate_t GetMeterDecayRate() const;		// // // 050B

	CSoundChip *GetSoundChip(sound_chip_t Chip) const override;		// // //

#ifdef LOGGING
	void	Log();
#endif

private:
	void StepSequence();		// // //

	void LogWrite(uint16_t Address, uint8_t Value);

private:
	std::unique_ptr<CMixer> m_pMixer;		// // //
	IAudioCallback *m_pParent;

	// Expansion chips
	std::vector<std::unique_ptr<CSoundChip>> m_pSoundChips;		// // //
	std::vector<CSoundChip *> m_pActiveChips;		// // //

	CSoundChipSet m_iExternalSoundChip;				// // // External sound chip, if used

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
