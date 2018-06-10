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

/*

 This will mix and synthesize the APU audio using blargg's blip-buffer

 Mixing of internal audio relies on Blargg's findings

 Mixing of external channles are based on my own research:

 VRC6 (Madara):
	Pulse channels has the same amplitude as internal-
	pulse channels on equal volume levels.

 FDS:
	Square wave @ v = $1F: 2.4V
				  v = $0F: 1.25V
	(internal square wave: 1.0V)

 MMC5 (just breed):
	2A03 square @ v = $0F: 760mV (the cart attenuates internal channels a little)
	MMC5 square @ v = $0F: 900mV

 VRC7:
	2A03 Square  @ v = $0F: 300mV (the cart attenuates internal channels a lot)
	VRC7 Patch 5 @ v = $0F: 900mV
	Did some more tests and found patch 14 @ v=15 to be 13.77dB stronger than a 50% square @ v=15

 ---

 N163 & 5B are still unknown

*/

#include "APU/Mixer.h"
#include <algorithm>		// // //
#include <memory>
#include <cmath>
#include "ext/emu/emu2413.h"		// // //
#include "APU/SN76489.h"		// // //

namespace {

const float LEVEL_FALL_OFF_RATE = 0.6f;
const int   LEVEL_FALL_OFF_DELAY = 3;

constexpr chip_level_t GetMixerFromChannel(stChannelID ch) noexcept {		// // //
	switch (ch.Chip) {
	case sound_chip_t::APU:
		return IsAPUPulse(ch) ? CHIP_LEVEL_APU1 : CHIP_LEVEL_APU2;
	case sound_chip_t::VRC6:
		return CHIP_LEVEL_VRC6;
	case sound_chip_t::VRC7:
		return CHIP_LEVEL_VRC7;
	case sound_chip_t::FDS:
		return CHIP_LEVEL_FDS;
	case sound_chip_t::MMC5:
		return CHIP_LEVEL_MMC5;
	case sound_chip_t::N163:
		return CHIP_LEVEL_N163;
	case sound_chip_t::S5B:
		return CHIP_LEVEL_S5B;
	case sound_chip_t::SN76489:
		return CHIP_LEVEL_SN76489;
	default:
		return CHIP_LEVEL_NONE;
	}
}

} // namespace

template <typename F>
void CMixer::WithMixer(chip_level_t Mixer, F f) {		// // //
	switch (Mixer) {
	case CHIP_LEVEL_APU1: f(levels2A03SS_); break;
	case CHIP_LEVEL_APU2: f(levels2A03TND_); break;
	case CHIP_LEVEL_VRC6: f(levelsVRC6_); break;
	case CHIP_LEVEL_MMC5: f(levelsMMC5_); break;
	case CHIP_LEVEL_FDS:  f(levelsFDS_); break;
	case CHIP_LEVEL_N163: f(levelsN163_); break;
	case CHIP_LEVEL_S5B:  f(levelsS5B_); break;		// // // 050B
	case CHIP_LEVEL_SN76489: f(levelsSN76489_); break;
	case CHIP_LEVEL_VRC7:
	case CHIP_LEVEL_NONE:
		break;
	}
}

template <typename F>
void CMixer::VisitMixers(F f) {
	WithMixer(CHIP_LEVEL_APU1, f);
	WithMixer(CHIP_LEVEL_APU2, f);
	WithMixer(CHIP_LEVEL_VRC6, f);
	WithMixer(CHIP_LEVEL_MMC5, f);
	WithMixer(CHIP_LEVEL_FDS, f);
	WithMixer(CHIP_LEVEL_N163, f);
	WithMixer(CHIP_LEVEL_S5B, f);
	WithMixer(CHIP_LEVEL_SN76489, f);
}

void CMixer::ExternalSound(CSoundChipSet Chip) {		// // //
	m_iExternalChip = Chip;
	UpdateSettings(m_iLowCut, m_iHighCut, m_iHighDamp, m_fOverallVol);
}

void CMixer::SetNamcoMixing(bool bLinear)		// // //
{
	m_bNamcoMixing = bLinear;
}

void CMixer::SetChipLevel(chip_level_t Chip, float Level)
{
	WithMixer(Chip, [&] (auto &mixer) {
		mixer.SetMixerLevel(Level);
	});
}

float CMixer::GetAttenuation() const
{
	const float ATTENUATION_2A03 = 1.00f;		// // //
	const float ATTENUATION_VRC6 = 0.80f;
	const float ATTENUATION_VRC7 = 0.64f;
	const float ATTENUATION_MMC5 = 0.83f;
	const float ATTENUATION_FDS  = 0.90f;
	const float ATTENUATION_N163 = 0.70f;
	const float ATTENUATION_S5B  = 0.50f;		// // // 050B
	const float ATTENUATION_SN76489 = 0.75f;

	float Attenuation = 1.0f;

	// Increase headroom if some expansion chips are enabled

	if (m_iExternalChip.ContainsChip(sound_chip_t::APU))
		Attenuation *= ATTENUATION_2A03;
	if (m_iExternalChip.ContainsChip(sound_chip_t::VRC6))
		Attenuation *= ATTENUATION_VRC6;
	if (m_iExternalChip.ContainsChip(sound_chip_t::VRC7))
		Attenuation *= ATTENUATION_VRC7;
	if (m_iExternalChip.ContainsChip(sound_chip_t::MMC5))
		Attenuation *= ATTENUATION_MMC5;
	if (m_iExternalChip.ContainsChip(sound_chip_t::FDS))
		Attenuation *= ATTENUATION_FDS;
	if (m_iExternalChip.ContainsChip(sound_chip_t::N163))
		Attenuation *= ATTENUATION_N163;
	if (m_iExternalChip.ContainsChip(sound_chip_t::S5B))		// // // 050B
		Attenuation *= ATTENUATION_S5B;
	if (m_iExternalChip.ContainsChip(sound_chip_t::SN76489))
		Attenuation *= ATTENUATION_SN76489;

	return Attenuation;
}

void CMixer::UpdateSettings(int LowCut,	int HighCut, int HighDamp, float OverallVol)
{
	m_iLowCut = LowCut;
	m_iHighCut = HighCut;
	m_iHighDamp = HighDamp;
	m_fOverallVol = OverallVol;

	// Blip-buffer filtering
	BlipBuffer.bass_freq(m_iLowCut);

	blip_eq_t eq(-m_iHighDamp, m_iHighCut, m_iSampleRate);

	levels2A03SS_.SetLowPass(eq);
	levels2A03TND_.SetLowPass(eq);
	levelsVRC6_.SetLowPass(eq);
	levelsMMC5_.SetLowPass(eq);
	levelsS5B_.SetLowPass(eq);
	levelsSN76489_.SetLowPass(eq);

	// // // N163 special filtering
	levelsN163_.SetLowPass({-(double)std::max(24, m_iHighDamp), std::min(m_iHighCut, 12000), (long)m_iSampleRate});

	// FDS special filtering (TODO fix this for high sample rates)
	levelsFDS_.SetLowPass({-48, 1000, (long)m_iSampleRate});

	float Volume = m_fOverallVol * GetAttenuation();
	VisitMixers([&] (auto &levels) {
		levels.SetVolume(Volume);
	});
}

void CMixer::SetNamcoVolume(float fVol)
{
	float fVolume = fVol * m_fOverallVol * GetAttenuation();

	levelsN163_.SetVolume(fVolume);
}

void CMixer::UpdateMeters() {		// // //
	for (auto &x : m_ChannelLevels) {
		auto &lv = x.second;
		lv.LastLevel = lv.Level;		// // //
		if (m_iMeterDecayRate == decay_rate_t::Fast)		// // // 050B
			lv.Level = 0;
		else if (lv.FallOff > 0)
			--lv.FallOff;
		else {
			lv.Level -= LEVEL_FALL_OFF_RATE;
			if (lv.Level < 0.f)
				lv.Level = 0.f;
		}
	}

}

decay_rate_t CMixer::GetMeterDecayRate() const		// // // 050B
{
	return m_iMeterDecayRate;
}

void CMixer::SetMeterDecayRate(decay_rate_t Rate)		// // // 050B
{
	m_iMeterDecayRate = Rate;
}

void CMixer::MixSamples(blip_sample_t *pBuffer, uint32_t Count)
{
	// For VRC7
	BlipBuffer.mix_samples(pBuffer, Count);
}

uint32_t CMixer::GetMixSampleCount(int t) const
{
	return BlipBuffer.count_samples(t);
}

bool CMixer::AllocateBuffer(unsigned int BufferLength, uint32_t SampleRate, uint8_t NrChannels)
{
	m_iSampleRate = SampleRate;
	return !BlipBuffer.set_sample_rate(SampleRate, (BufferLength * 1000 * 4) / SampleRate);		// // //
}

void CMixer::SetClockRate(uint32_t Rate)
{
	// Change the clockrate
	BlipBuffer.clock_rate(Rate);
}

void CMixer::ClearBuffer()
{
	BlipBuffer.clear();
	VisitMixers([] (auto &levels) {
		levels.ResetDelta();
	});
}

int CMixer::SamplesAvail() const
{
	return (int)BlipBuffer.samples_avail();
}

int CMixer::FinishBuffer(int t)
{
	BlipBuffer.end_frame(t);

	// Get channel levels for VRC7
	for (std::size_t i = 0; i < MAX_CHANNELS_VRC7; ++i)
		StoreChannelLevel(stChannelID {sound_chip_t::VRC7, static_cast<std::uint8_t>(i)}, OPLL_getchanvol(i));

	UpdateMeters();		// // //

	// Return number of samples available
	return BlipBuffer.samples_avail();
}

//
// Mixing
//

void CMixer::AddValue(stChannelID ChanID, int Value, int FrameCycles) {		// // //
	WithMixer(GetMixerFromChannel(ChanID), [&] (auto &mixer) {
		StoreChannelLevel(ChanID, mixer.AddValue(ChanID, Value, FrameCycles, BlipBuffer));
	});
}

int CMixer::ReadBuffer(int Size, blip_sample_t *Buffer, bool Stereo) {		// // //
	return BlipBuffer.read_samples(Buffer, Size);
}

int32_t CMixer::GetChanOutput(stChannelID Chan) const		// // //
{
	auto it = m_ChannelLevels.find(Chan);
	return it != m_ChannelLevels.end() ? it->second.LastLevel : 0;
}

void CMixer::StoreChannelLevel(stChannelID Channel, int Level)		// // //
{
	double AbsVol = std::abs(Level);

	// Adjust channel levels for some channels
	if (IsDPCM(Channel))
		AbsVol /= 8.;

	if (IsVRC6Sawtooth(Channel))
		AbsVol = AbsVol * .75;

	if (Channel.Chip == sound_chip_t::FDS)
		AbsVol /= 188.;

	if (Channel.Chip == sound_chip_t::N163) {		// // //
		AbsVol /= 15.;
		Channel.Subindex = static_cast<uint8_t>(enum_count<n163_subindex_t>() - 1 - Channel.Subindex);
	}

	if (Channel.Chip == sound_chip_t::VRC7)		// // //
		AbsVol = std::log(AbsVol) * 3.;

	if (Channel.Chip == sound_chip_t::S5B)		// // //
		AbsVol = std::log(AbsVol) * 2.8;

	if (Channel.Chip == sound_chip_t::SN76489) {
		auto Lv = static_cast<int>(AbsVol);
		AbsVol = 0;
		while (AbsVol < 15 && Lv >= CSN76489::VOLUME_TABLE[14 - static_cast<int>(AbsVol)])
			++AbsVol;
	}

	auto &lv = m_ChannelLevels[Channel];
	if (AbsVol >= lv.Level) {
		lv.Level = (float)AbsVol;
		lv.FallOff = LEVEL_FALL_OFF_DELAY;
	}
}

uint32_t CMixer::ResampleDuration(uint32_t Time) const
{
	return (uint32_t)BlipBuffer.resampled_duration((blip_time_t)Time);
}
