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

#include "APU/SN76489.h"
#include "APU/Types.h"
//#include "../VGM/Writer/Base.h"
#include "Assertion.h"
#include "RegisterState.h"
#include "APU/APU.h"

const uint16_t CSN76489::LATCH_PORT = 0x4E; // temp
const uint16_t CSN76489::STEREO_PORT = 0x4F;
const uint16_t CSN76489::VOLUME_TABLE[] = {
	1516, 1205, 957, 760, 603, 479, 381, 303, 240, 191, 152, 120, 96, 76, 60, 0
};
const uint16_t CSNSquare::CUTOFF_PERIOD = 0 /* 0x07*/;



CSN76489Channel::CSN76489Channel(CMixer &Mixer, std::uint8_t nInstance, sn76489_subindex_t subindex) :
	CChannel(Mixer, {nInstance, sound_chip_t::SN76489, value_cast(subindex)})
{
}

void CSN76489Channel::Reset()
{
	SetStereo(true, true);
	SetAttenuation(0xF);
}
/*
void CSN76489Channel::SetVGMWriter(const CVGMWriterBase *pWrite)
{
	m_pVGMWriter = pWrite;
}
*/
void CSN76489Channel::SetStereo(bool Left, bool Right)
{
	m_bLeft = Left;
	m_bRight = Right;
}

void CSN76489Channel::SetAttenuation(uint8_t Value)
{
	Value &= 0xF;
	if (Value != m_iAttenuation) {
//		if (m_pVGMWriter != nullptr)
//			m_pVGMWriter->WriteReg(0, 0x90 | (m_iChanId << 5) | Value);
		m_iAttenuation = Value;
	}
}

int32_t CSN76489Channel::GetVolume() const
{
	return CSN76489::VOLUME_TABLE[m_iAttenuation];
}



CSNSquare::CSNSquare(CMixer &Mixer, std::uint8_t nInstance, sn76489_subindex_t subindex) :
	CSN76489Channel(Mixer, nInstance, subindex)
{
}

CSNSquare::~CSNSquare()
{
}

void CSNSquare::Reset()
{
	CSN76489Channel::Reset();

	SetPeriodLo(0);
	SetPeriodHi(0);

	m_iSquareCounter = 0;
	m_bSqaureActive = true;
}

void CSNSquare::Process(uint32_t Time)
{
	uint16_t Period = GetPeriod();
	if (!Period)
		Period = 1; // or 0x400 according to one of the vgm flags

	while (Time >= m_iSquareCounter) {
		Time -= m_iSquareCounter;
		m_iTime += m_iSquareCounter;
		m_iSquareCounter = Period << 4;
		m_bSqaureActive = Period >= CUTOFF_PERIOD ? !m_bSqaureActive : 0;
		int32_t Vol = m_bSqaureActive ? GetVolume() : 0;
		Mix(((m_bLeft ? Vol : 0) + (m_bRight ? Vol : 0)) / 2);
//		Mix(m_bLeft ? Vol : 0, m_bRight ? Vol : 0);
	}

	m_iSquareCounter -= Time;
	m_iTime += Time;
}

void CSNSquare::SetPeriodLo(uint8_t Value)
{
	m_iPrevPeriod = GetPeriod();
	m_iSquarePeriodLo = Value & 0xF;
}

void CSNSquare::SetPeriodHi(uint8_t Value)
{
	m_iSquarePeriodHi = Value & 0x3F;
	FlushPeriod();
}

void CSNSquare::FlushPeriod() const
{
	/*
	if (m_pVGMWriter == nullptr)
		return;
	uint16_t NewPeriod = GetPeriod();
	if (NewPeriod == m_iPrevPeriod)
		return;
	m_pVGMWriter->WriteReg(0, 0x80 | (m_iChanId << 5) | m_iSquarePeriodLo);
	if (m_iSquarePeriodHi != (m_iPrevPeriod >> 4))
		m_pVGMWriter->WriteReg(0, m_iSquarePeriodHi);
		*/
}

uint16_t CSNSquare::GetPeriod() const
{
	return m_iSquarePeriodLo | (m_iSquarePeriodHi << 4);
}

double CSNSquare::GetFrequency() const {
	return (double)MASTER_CLOCK_NTSC / GetPeriod() / 32;
}



const uint16_t CSNNoise::LFSR_INIT = 0x8000;

CSNNoise::CSNNoise(CMixer &Mixer, std::uint8_t nInstance) :
	CSN76489Channel(Mixer, nInstance, sn76489_subindex_t::noise),
	m_iCH3Period(0)
{
	Configure(SN_NOISE_CFG::Sega);
}

CSNNoise::~CSNNoise()
{
}

void CSNNoise::Reset()
{
	CSN76489Channel::Reset();

	SetControlMode(SN_NOI_DIV_512 | (SN_NOI_FB_LONG << 2));

	m_iSquareCounter = 0;
	m_bSqaureActive = true;
}

void CSNNoise::Process(uint32_t Time)
{
	uint16_t Period = 1;
	switch (m_iNoiseFreq) {
	case SN_NOI_DIV_512:  Period = 0x10; break;
	case SN_NOI_DIV_1024: Period = 0x20; break;
	case SN_NOI_DIV_2048: Period = 0x40; break;
	case SN_NOI_DIV_CH3:  Period = m_iCH3Period ? m_iCH3Period : 1; break;
	default: DEBUG_BREAK();
	}

	while (Time >= m_iSquareCounter) {
		Time -= m_iSquareCounter;
		m_iTime += m_iSquareCounter;
		m_iSquareCounter = Period << 4;
		m_bSqaureActive = !m_bSqaureActive;
		if (m_bSqaureActive) {
			int Feedback = m_iLFSRState;
			switch (m_iNoiseFeedback) {
			case SN_NOI_FB_SHORT:
				Feedback &= 1; break;
			case SN_NOI_FB_LONG:
				Feedback &= m_iFeedbackPattern;
				Feedback = Feedback && (Feedback ^ m_iFeedbackPattern); break;
			default: DEBUG_BREAK();
			}
			m_iLFSRState = (m_iLFSRState >> 1) | (Feedback << (16 - 1));
			int32_t Vol = (m_iLFSRState & 1) ? GetVolume() : 0;
			Mix(((m_bLeft ? Vol : 0) + (m_bRight ? Vol : 0)) / 2);
	//		Mix(m_bLeft ? Vol : 0, m_bRight ? Vol : 0);
		}
	}

	m_iSquareCounter -= Time;
	m_iTime += Time;
}

void CSNNoise::SetControlMode(uint8_t Value)
{
	Value &= 0x7;
	m_iNoiseFreq = static_cast<SN_noise_div_t>(Value & 0x03);
	m_iNoiseFeedback = static_cast<SN_noise_fb_t>(Value & 0x04);
	m_iLFSRState = LFSR_INIT;
//	if (m_pVGMWriter != nullptr)
//		m_pVGMWriter->WriteReg(0, 0xE0 | Value);
}

void CSNNoise::Configure(SN_NOISE_CFG Mode)
{
	switch (Mode) {
	case SN_NOISE_CFG::Sega: m_iFeedbackPattern = 0x0009; break;
	// TODO
	}
}

void CSNNoise::CachePeriod(uint16_t Period)
{
	m_iCH3Period = Period;
}

double CSNNoise::GetFrequency() const {
	uint16_t Period = 1;
	switch (m_iNoiseFreq) {
	case SN_NOI_DIV_512:  Period = 0x10; break;
	case SN_NOI_DIV_1024: Period = 0x20; break;
	case SN_NOI_DIV_2048: Period = 0x40; break;
	case SN_NOI_DIV_CH3:  Period = m_iCH3Period ? m_iCH3Period : 1; break;
	default: DEBUG_BREAK();
	}

	return (double)MASTER_CLOCK_NTSC / Period / 32;
}



CSN76489::CSN76489(CMixer &Mixer, std::uint8_t nInstance) : CSoundChip(Mixer, nInstance),
	m_pChannels {
		std::make_unique<CSNSquare>(Mixer, nInstance, sn76489_subindex_t::square1),
		std::make_unique<CSNSquare>(Mixer, nInstance, sn76489_subindex_t::square2),
		std::make_unique<CSNSquare>(Mixer, nInstance, sn76489_subindex_t::square3),
		std::make_unique<CSNNoise>(Mixer, nInstance),
	}
{
}

sound_chip_t CSN76489::GetID() const {
	return sound_chip_t::SN76489;
}

void CSN76489::Reset()
{
	for (auto &x : m_pChannels)
		x->Reset();

	m_iAddressLatch = 0;
}

void CSN76489::Process(uint32_t Time)
{
	for (auto &x : m_pChannels)
		x->Process(Time);
}

void CSN76489::EndFrame()
{
	for (int i = 0; i < 3; ++i)
		GetSquare(i)->FlushPeriod();
	for (auto &x : m_pChannels)
		x->EndFrame();
}

void CSN76489::Write(uint16_t Address, uint8_t Value)
{
	switch (Address) {
	case 0: case 2: case 4:
		m_iAddressLatch = (uint8_t)Address;
		GetSquare(Address / 2)->SetPeriodLo(Value);
		break;
	case 1: case 3: case 5: case 7:
		GetSquare(m_iAddressLatch / 2)->FlushPeriod();
		m_pChannels[Address / 2]->SetAttenuation(Value);
		break;
	case 6:
		GetSquare(m_iAddressLatch / 2)->FlushPeriod();
		GetNoise()->SetControlMode(Value);
		break;
	case STEREO_PORT:
//		if (m_pVGMWriter != nullptr)
//			m_pVGMWriter->WriteReg(0, Value, 0x06);
		for (const auto &x : m_pChannels) {
			x->SetStereo((Value & 0x10) != 0, (Value & 0x01) != 0);
			Value >>= 1;
		}
		break;
	case LATCH_PORT:
//	default:
		GetSquare(m_iAddressLatch / 2)->SetPeriodHi(Value);
		if (m_iAddressLatch / 2 == value_cast(sn76489_subindex_t::square3))
			UpdateNoisePeriod();
	default:
		return;
	}

	if (Address / 2 == value_cast(sn76489_subindex_t::square3))
		UpdateNoisePeriod();
}

uint8_t CSN76489::Read(uint16_t Address, bool &Mapped)
{
	return 0;
}

void CSN76489::ConfigureNoise(SN_NOISE_CFG Mode) const
{
	GetNoise()->Configure(Mode);
}
/*
void CSN76489::SetVGMWriter(const CVGMWriterBase *pWrite)
{
	for (const auto &x : m_pChannels)
		x->SetVGMWriter(pWrite);
	m_pVGMWriter = pWrite;
}
*/
CSNSquare *CSN76489::GetSquare(unsigned ID) const
{
	Assert(ID <= 2u && m_pChannels[ID] != nullptr);
	return static_cast<CSNSquare *>(m_pChannels[ID].get());
}

CSNNoise *CSN76489::GetNoise() const
{
	Assert(m_pChannels[value_cast(sn76489_subindex_t::noise)] != nullptr);
	return static_cast<CSNNoise *>(m_pChannels[value_cast(sn76489_subindex_t::noise)].get());
}

void CSN76489::UpdateNoisePeriod() const
{
	GetNoise()->CachePeriod(GetSquare(value_cast(sn76489_subindex_t::square3))->GetPeriod());
}
