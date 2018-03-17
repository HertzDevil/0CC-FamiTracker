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
#include <cmath>
#include <algorithm>		// // //
#include "APU/Mixer.h"		// // //
#include "APU/2A03.h"		// // //
#include "APU/MMC5.h"
#include "APU/N163.h"
#include "APU/VRC7.h"
#include "FamiTrackerEnv.h"		// // //
#include "SoundChipService.h"		// // //
#include "RegisterState.h"		// // //
#include "Assertion.h"		// // //

CAPU::CAPU(IAudioCallback *pCallback) :		// // //
	m_pMixer(std::make_unique<CMixer>()),		// // //
	m_pParent(pCallback),
	m_iSampleRate(44100),		// // //
	m_iCyclesToRun(0),
	m_iFrameCycles(0),
	m_fLevelVRC7(1.f)
{
	for (sound_chip_t c : SOUND_CHIPS)
		m_pSoundChips.push_back(Env.GetSoundChipService()->MakeSoundChipDriver(c, *m_pMixer));

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

		for (auto *Chip : m_pActiveChips)		// // //
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
	if (++m_iSequencerCount == C2A03Chan::SEQUENCER_FREQUENCY)
		m_iSequencerClock = m_iSequencerCount = 0;
	m_iSequencerNext = (uint64_t)MASTER_CLOCK_NTSC * (m_iSequencerCount + 1) / C2A03Chan::SEQUENCER_FREQUENCY;

	for (auto &c : m_pSoundChips)		// // //
		if (auto *p2A03 = dynamic_cast<C2A03 *>(c.get()))
			p2A03->ClockSequence();
		else if (auto *pMMC5 = dynamic_cast<CMMC5 *>(c.get()))
			pMMC5->ClockSequence();
}

// End of audio frame, flush the buffer if enough samples has been produced, and start a new frame
void CAPU::EndFrame()
{
	// The APU will always output audio in 32 bit signed format

	for (auto *Chip : m_pActiveChips)		// // //
		Chip->EndFrame();

	int SamplesAvail = m_pMixer->FinishBuffer(m_iFrameCycles);
	int ReadSamples	= m_pMixer->ReadBuffer(SamplesAvail, m_pSoundBuffer.get(), m_bStereoEnabled);
	if (m_pParent)		// // //
		m_pParent->FlushBuffer({m_pSoundBuffer.get(), (unsigned)ReadSamples});

	m_iFrameCycles = 0;

	for (auto *r : m_pActiveChips)		// // //
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
	m_iSequencerNext	= MASTER_CLOCK_NTSC / C2A03Chan::SEQUENCER_FREQUENCY;

	m_iCyclesToRun		= 0;
	m_iFrameCycles		= 0;

	for (auto &c : m_pSoundChips)		// // //
		if (auto *p2A03 = dynamic_cast<C2A03 *>(c.get()))
			p2A03->ClearSample();

	for (auto *Chip : m_pActiveChips) {		// // //
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
	for (auto &c : m_pSoundChips)		// // //
		if (auto *pVRC7 = dynamic_cast<CVRC7 *>(c.get()))
			pVRC7->SetVolume((float(Volume) / 100.0f) * m_fLevelVRC7);
}

// // //
void CAPU::SetCallback(IAudioCallback &pCallback) {
	m_pParent = &pCallback;
}

void CAPU::SetExternalSound(CSoundChipSet Chip) {
	// Set expansion chip
	m_iExternalSoundChip = Chip;
	m_pMixer->ExternalSound(Chip);

	m_pActiveChips.clear();

	for (auto &c : m_pSoundChips)		// // //
		if (Chip.ContainsChip(c->GetID()))
			m_pActiveChips.push_back(c.get());

	Reset();
}

void CAPU::ChangeMachineRate(int Machine, int Rate)		// // //
{
	// Allow to change speed on the fly
	//

	uint32_t BaseFreq = (Machine == MACHINE_NTSC) ? MASTER_CLOCK_NTSC : MASTER_CLOCK_PAL;
	for (auto &c : m_pSoundChips)		// // //
		if (auto *p2A03 = dynamic_cast<C2A03 *>(c.get()))
			p2A03->ChangeMachine(Machine);
		else if (auto *pVRC7 = dynamic_cast<CVRC7 *>(c.get()))
			pVRC7->SetSampleSpeed(m_iSampleRate, BaseFreq, Rate);
}

bool CAPU::SetupSound(int SampleRate, int NrChannels, int Machine)		// // //
{
	// Allocate a sound buffer
	//
	// Returns false if a buffer couldn't be allocated
	//

	uint32_t BaseFreq = (Machine == MACHINE_NTSC) ? MASTER_CLOCK_NTSC : MASTER_CLOCK_PAL;
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

	for (auto *Chip : m_pActiveChips)		// // //
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

	for (auto *Chip : m_pActiveChips)		// // //
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
	for (auto &c : m_pSoundChips)
		if (c->GetID() == Chip)
			return c.get();
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
	float fLevel = std::pow(10, Level / 20.0f);		// Convert dB to linear

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
	for (auto &c : m_pSoundChips)		// // //
		if (auto *pN163 = dynamic_cast<CN163 *>(c.get()))
			pN163->SetMixingMethod(bLinear);
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
	for (auto *r : m_pActiveChips)		// // //
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
