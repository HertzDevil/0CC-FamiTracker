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

#include "APU/APU.h"
#include <algorithm>		// // //
#include <vector>
#include <memory>
#include <cmath>
#include "APU/Mixer.h"		// // //
#include "APU/2A03.h"		// // //
#include "APU/VRC6.h"
#include "APU/MMC5.h"
#include "APU/FDS.h"
#include "APU/N163.h"
#include "APU/VRC7.h"
#include "APU/S5B.h"
#include "RegisterState.h"		// // //
#include "Assertion.h"		// // //

const int		CAPU::SEQUENCER_FREQUENCY	= 240;		// // //
const uint32_t	CAPU::BASE_FREQ_NTSC		= 1789773;		// 72.667
const uint32_t	CAPU::BASE_FREQ_PAL			= 1662607;
const uint8_t	CAPU::FRAME_RATE_NTSC		= 60;
const uint8_t	CAPU::FRAME_RATE_PAL		= 50;

const uint8_t CAPU::LENGTH_TABLE[] = {
	0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06,
	0xA0, 0x08, 0x3C, 0x0A, 0x0E, 0x0C, 0x1A, 0x0E,
	0x0C, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16,
	0xC0, 0x18, 0x48, 0x1A, 0x10, 0x1C, 0x20, 0x1E,
};

CAPU::CAPU(IAudioCallback *pCallback) :		// // //
	m_pMixer(std::make_unique<CMixer>()),		// // //
	m_pParent(pCallback),
	m_p2A03(std::make_unique<C2A03>(*m_pMixer)),		// // //
	m_pMMC5(std::make_unique<CMMC5>(*m_pMixer)),
	m_pVRC6(std::make_unique<CVRC6>(*m_pMixer)),
	m_pVRC7(std::make_unique<CVRC7>(*m_pMixer)),
	m_pFDS (std::make_unique<CFDS>(*m_pMixer)),
	m_pN163(std::make_unique<CN163>(*m_pMixer)),
	m_pS5B (std::make_unique<CS5B>(*m_pMixer)),
	m_iFrameCycles(0),
	m_pSoundBuffer(NULL),
	m_iCyclesToRun(0),
	m_iSampleRate(44100)		// // //
{

	m_fLevelVRC7 = 1.0f;

#ifdef LOGGING
	m_pLog = std::make_unique<CFile>("apu_log.txt", CFile::modeCreate | CFile::modeWrite);
	m_iFrame = 0;
#endif
}

CAPU::~CAPU()
{
#ifdef LOGGING
	m_pLog->Close();
#endif
}

// The main APU emulation
//
// The amount of cycles that will be emulated is added by CAPU::AddCycles
//
void CAPU::Process()
{
	while (m_iCyclesToRun > 0) {

		uint32_t Time = std::min(m_iCyclesToRun, m_iSequencerNext - m_iSequencerClock);		// // //

		for (auto *Chip : m_pExpansionChips)		// // //
			Chip->Process(Time);

		m_iFrameCycles	  += Time;
		m_iSequencerClock += Time;
		m_iCyclesToRun	  -= Time;

		if (m_iSequencerClock == m_iSequencerNext)
			StepSequence();		// // //
	}
}

void CAPU::StepSequence()		// // //
{
	if (++m_iSequencerCount == SEQUENCER_FREQUENCY)
		m_iSequencerClock = m_iSequencerCount = 0;
	m_iSequencerNext = (uint64_t)BASE_FREQ_NTSC * (m_iSequencerCount + 1) / SEQUENCER_FREQUENCY;
	GetSoundChip<sound_chip_t::APU>()->ClockSequence();
	GetSoundChip<sound_chip_t::MMC5>()->ClockSequence();		// // //
}

// End of audio frame, flush the buffer if enough samples has been produced, and start a new frame
void CAPU::EndFrame()
{
	// The APU will always output audio in 32 bit signed format

	for (auto *Chip : m_pExpansionChips)		// // //
		Chip->EndFrame();

	int SamplesAvail = m_pMixer->FinishBuffer(m_iFrameCycles);
	int ReadSamples	= m_pMixer->ReadBuffer(SamplesAvail, m_pSoundBuffer.get(), m_bStereoEnabled);
	if (m_pParent)		// // //
		m_pParent->FlushBuffer({m_pSoundBuffer.get(), (unsigned)ReadSamples});

	m_iFrameCycles = 0;

	for (auto *r : m_pExpansionChips)		// // //
		r->GetRegisterLogger().Step();

#ifdef LOGGING
	++m_iFrame;
#endif
}

void CAPU::Reset()
{
	// Reset APU
	//

	m_iSequencerCount	= 0;		// // //
	m_iSequencerClock	= 0;		// // //
	m_iSequencerNext	= BASE_FREQ_NTSC / SEQUENCER_FREQUENCY;

	m_iCyclesToRun		= 0;
	m_iFrameCycles		= 0;

	GetSoundChip<sound_chip_t::APU>()->ClearSample();		// // //

	for (auto *Chip : m_pExpansionChips) {		// // //
		Chip->GetRegisterLogger().Reset();
		Chip->Reset();
	}

	m_pMixer->ClearBuffer();

#ifdef LOGGING
	m_iFrame = 0;
#endif
}

void CAPU::SetupMixer(int LowCut, int HighCut, int HighDamp, int Volume) const
{
	// New settings
	m_pMixer->UpdateSettings(LowCut, HighCut, HighDamp, float(Volume) / 100.0f);
	GetSoundChip<sound_chip_t::VRC7>()->SetVolume((float(Volume) / 100.0f) * m_fLevelVRC7);
}

// // //
void CAPU::SetCallback(IAudioCallback &pCallback) {
	m_pParent = &pCallback;
}

void CAPU::SetExternalSound(CSoundChipSet Chip) {
	// Set expansion chip
	m_iExternalSoundChip = Chip;
	m_pMixer->ExternalSound(Chip);

	m_pExpansionChips.clear();

	if (Chip.ContainsChip(sound_chip_t::APU))		// // //
		m_pExpansionChips.push_back(GetSoundChip<sound_chip_t::APU>());
	if (Chip.ContainsChip(sound_chip_t::VRC6))
		m_pExpansionChips.push_back(GetSoundChip<sound_chip_t::VRC6>());
	if (Chip.ContainsChip(sound_chip_t::VRC7))
		m_pExpansionChips.push_back(GetSoundChip<sound_chip_t::VRC7>());
	if (Chip.ContainsChip(sound_chip_t::FDS))
		m_pExpansionChips.push_back(GetSoundChip<sound_chip_t::FDS>());
	if (Chip.ContainsChip(sound_chip_t::MMC5))
		m_pExpansionChips.push_back(GetSoundChip<sound_chip_t::MMC5>());
	if (Chip.ContainsChip(sound_chip_t::N163))
		m_pExpansionChips.push_back(GetSoundChip<sound_chip_t::N163>());
	if (Chip.ContainsChip(sound_chip_t::S5B))
		m_pExpansionChips.push_back(GetSoundChip<sound_chip_t::S5B>());

	Reset();
}

void CAPU::ChangeMachineRate(int Machine, int Rate)		// // //
{
	// Allow to change speed on the fly
	//

	uint32_t BaseFreq = (Machine == MACHINE_NTSC) ? BASE_FREQ_NTSC : BASE_FREQ_PAL;
	GetSoundChip<sound_chip_t::APU>()->ChangeMachine(Machine);
	GetSoundChip<sound_chip_t::VRC7>()->SetSampleSpeed(m_iSampleRate, BaseFreq, Rate);
}

bool CAPU::SetupSound(int SampleRate, int NrChannels, int Machine)		// // //
{
	// Allocate a sound buffer
	//
	// Returns false if a buffer couldn't be allocated
	//

	uint32_t BaseFreq = (Machine == MACHINE_NTSC) ? BASE_FREQ_NTSC : BASE_FREQ_PAL;
	uint8_t FrameRate = (Machine == MACHINE_NTSC) ? FRAME_RATE_NTSC : FRAME_RATE_PAL;
	m_iSampleRate = SampleRate;		// // //

	m_iSoundBufferSamples = uint32_t(m_iSampleRate / FRAME_RATE_PAL);	// Samples / frame. Allocate for PAL, since it's more
	m_bStereoEnabled	  = (NrChannels == 2);
	m_iSoundBufferSize	  = m_iSoundBufferSamples * NrChannels;		// Total amount of samples to allocate
	m_iSampleSizeShift	  = (NrChannels == 2) ? 1 : 0;
	m_iBufferPointer	  = 0;

	if (!m_pMixer->AllocateBuffer(m_iSoundBufferSamples, m_iSampleRate, NrChannels))		// // //
		return false;

	m_pMixer->SetClockRate(BaseFreq);

	m_pSoundBuffer = std::make_unique<int16_t[]>(m_iSoundBufferSize << 1);
	if (!m_pSoundBuffer)
		return false;

	ChangeMachineRate(Machine, FrameRate);		// // //

	return true;
}

void CAPU::AddTime(int32_t Cycles)
{
	if (Cycles < 0)
		return;
	m_iCyclesToRun += Cycles;
}

void CAPU::Write(uint16_t Address, uint8_t Value)
{
	// Data was written to an external sound chip

	Process();

	for (auto *Chip : m_pExpansionChips)		// // //
		Chip->Write(Address, Value);

	LogWrite(Address, Value);
}

uint8_t CAPU::Read(uint16_t Address)
{
	// Data read from an external chip
	//

	uint8_t Value(0);
	bool Mapped(false);

	Process();

	for (auto *Chip : m_pExpansionChips)		// // //
		if (!Mapped)
			Value = Chip->Read(Address, Mapped);

	if (!Mapped)
		Value = Address >> 8;	// open bus

	return Value;
}

// Expansion for famitracker

int32_t CAPU::GetVol(chan_id_t Chan) const		// // //
{
	return m_pMixer->GetChanOutput(Chan);
}

CSoundChip *CAPU::GetSoundChip(sound_chip_t Chip) const {		// // //
	switch (Chip) {
	case sound_chip_t::APU:  return m_p2A03.get();
	case sound_chip_t::VRC6: return m_pVRC6.get();
	case sound_chip_t::VRC7: return m_pVRC7.get();
	case sound_chip_t::FDS:  return m_pFDS.get();
	case sound_chip_t::MMC5: return m_pMMC5.get();
	case sound_chip_t::N163: return m_pN163.get();
	case sound_chip_t::S5B:  return m_pS5B.get();
	}
	DEBUG_BREAK();
	return nullptr;
}

#ifdef LOGGING
void CAPU::Log()
{
	CStringA str = FormattedA("Frame %08i: ", m_iFrame - 1);
	str.Append("2A03 ");
	for (int i = 0; i < 0x14; ++i)
		AppendFormatA(str, "%02X ", GetReg(sound_chip_t::APU, i));
	if (m_iExternalSoundChip.ContainsChip(sound_chip_t::VRC6)) {		// // //
		str.Append("VRC6 ");
		for (int i = 0; i < 0x03; ++i) for (int j = 0; j < 0x03; ++j)
			AppendFormatA(str, "%02X ", GetReg(sound_chip_t::VRC6, 0x9000 + i * 0x1000 + j));
	}
	if (m_iExternalSoundChip.ContainsChip(sound_chip_t::MMC5)) {
		str.Append("MMC5 ");
		for (int i = 0; i < 0x08; ++i)
			AppendFormatA(str, "%02X ", GetReg(sound_chip_t::MMC5, 0x5000 + i));
	}
	if (m_iExternalSoundChip.ContainsChip(sound_chip_t::N163)) {
		str.Append("N163 ");
		for (int i = 0; i < 0x80; ++i)
			AppendFormatA(str, "%02X ", GetReg(sound_chip_t::N163, i));
	}
	if (m_iExternalSoundChip.ContainsChip(sound_chip_t::FDS)) {
		str.Append("FDS ");
		for (int i = 0; i < 0x0B; ++i)
			AppendFormatA(str, "%02X ", GetReg(sound_chip_t::FDS, 0x4080 + i));
	}
	if (m_iExternalSoundChip.ContainsChip(sound_chip_t::VRC7)) {
		str.Append("VRC7 ");
		for (int i = 0; i < 0x40; ++i)
			AppendFormatA(str, "%02X ", GetReg(sound_chip_t::VRC7, i));
	}
	if (m_iExternalSoundChip.ContainsChip(sound_chip_t::S5B)) {
		str.Append("S5B ");
		for (int i = 0; i < 0x10; ++i)
			AppendFormatA(str, "%02X ", GetReg(sound_chip_t::S5B, i));
	}
	str.Append("\r\n");
	m_pLog->Write(str, str.GetLength());
}
#endif

void CAPU::SetChipLevel(chip_level_t Chip, float Level)
{
	float fLevel = powf(10, Level / 20.0f);		// Convert dB to linear

	switch (Chip) {
	case CHIP_LEVEL_VRC7:
		m_fLevelVRC7 = fLevel;
		break;
		// // // 050B
	default:
		m_pMixer->SetChipLevel(Chip, fLevel);
	}
}

void CAPU::SetNamcoMixing(bool bLinear)		// // //
{
	m_pMixer->SetNamcoMixing(bLinear);
	GetSoundChip<sound_chip_t::N163>()->SetMixingMethod(bLinear);
}

void CAPU::SetMeterDecayRate(decay_rate_t Type) const		// // // 050B
{
	m_pMixer->SetMeterDecayRate(Type);
}

decay_rate_t CAPU::GetMeterDecayRate() const		// // // 050B
{
	return m_pMixer->GetMeterDecayRate();
}

void CAPU::LogWrite(uint16_t Address, uint8_t Value)
{
	for (auto *r : m_pExpansionChips)		// // //
		r->Log(Address, Value);
}

uint8_t CAPU::GetReg(sound_chip_t Chip, int Reg) const
{
	if (auto *r = GetRegState(Chip, Reg))		// // //
		return r->GetValue();
	return static_cast<uint8_t>(0);
}

double CAPU::GetFreq(sound_chip_t Chip, int Chan) const
{
	const CSoundChip *pChip = GetSoundChip(Chip);		// // //
	return pChip ? pChip->GetFreq(Chan) : 0.;
}

CRegisterState *CAPU::GetRegState(sound_chip_t Chip, int Reg) const		// // //
{
	const CSoundChip *pChip = GetSoundChip(Chip);
	return pChip ? pChip->GetRegisterLogger().GetRegister(Reg) : nullptr;
}
