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

#include "APU/Types_fwd.h"
#include "ft0cc/enum_traits.h"		// // //

inline constexpr unsigned MASTER_CLOCK_NTSC = 1789773;		// // // moved from CAPU
inline constexpr unsigned MASTER_CLOCK_PAL  = 1662607;
inline constexpr unsigned FRAME_RATE_NTSC   = 60;
inline constexpr unsigned FRAME_RATE_PAL    = 50;

enum class sound_chip_t : std::uint8_t {		// // //
	APU,
	VRC6,
	VRC7,
	FDS,
	MMC5,
	N163,
	S5B,
	NONE = (std::uint8_t)-1,
	none = NONE,
};

inline constexpr std::size_t SOUND_CHIP_COUNT = 7;

constexpr sound_chip_t SOUND_CHIPS[] = {
	sound_chip_t::APU, sound_chip_t::VRC6, sound_chip_t::VRC7, sound_chip_t::FDS, sound_chip_t::MMC5, sound_chip_t::N163, sound_chip_t::S5B,
};
constexpr sound_chip_t EXPANSION_CHIPS[] = {
	sound_chip_t::VRC6, sound_chip_t::VRC7, sound_chip_t::FDS, sound_chip_t::MMC5, sound_chip_t::N163, sound_chip_t::S5B,
};

enum class chan_id_t : unsigned {
	SQUARE1,
	SQUARE2,
	TRIANGLE,
	NOISE,
	DPCM,

	VRC6_PULSE1,
	VRC6_PULSE2,
	VRC6_SAWTOOTH,

	MMC5_SQUARE1,
	MMC5_SQUARE2,
	MMC5_VOICE,

	N163_CH1,		// // //
	N163_CH2,
	N163_CH3,
	N163_CH4,
	N163_CH5,
	N163_CH6,
	N163_CH7,
	N163_CH8,

	FDS,

	VRC7_CH1,
	VRC7_CH2,
	VRC7_CH3,
	VRC7_CH4,
	VRC7_CH5,
	VRC7_CH6,

	S5B_CH1,
	S5B_CH2,
	S5B_CH3,

	COUNT,		/* Total number of channels */
	NONE = (unsigned)-1,		// // //
	none = NONE,
};

// // // moved from FamiTrackerTypes.h
inline constexpr std::size_t MAX_CHANNELS_2A03 = 5;
inline constexpr std::size_t MAX_CHANNELS_VRC6 = 3;
inline constexpr std::size_t MAX_CHANNELS_VRC7 = 6;
inline constexpr std::size_t MAX_CHANNELS_FDS = 1;
inline constexpr std::size_t MAX_CHANNELS_MMC5 = 3; // includes pcm
inline constexpr std::size_t MAX_CHANNELS_N163 = 8;
inline constexpr std::size_t MAX_CHANNELS_S5B = 3;

inline constexpr std::size_t CHANID_COUNT = (unsigned)chan_id_t::COUNT;

constexpr sound_chip_t GetChipFromChannel(chan_id_t ch) noexcept {
	if (ch <= chan_id_t::DPCM)
		return sound_chip_t::APU;
	if (ch <= chan_id_t::VRC6_SAWTOOTH)
		return sound_chip_t::VRC6;
	if (ch <= chan_id_t::MMC5_VOICE)
		return sound_chip_t::MMC5;
	if (ch <= chan_id_t::N163_CH8)
		return sound_chip_t::N163;
	if (ch <= chan_id_t::FDS)
		return sound_chip_t::FDS;
	if (ch <= chan_id_t::VRC7_CH6)
		return sound_chip_t::VRC7;
	if (ch <= chan_id_t::S5B_CH3)
		return sound_chip_t::S5B;
	return sound_chip_t::NONE;
}

constexpr std::size_t GetChannelSubIndex(chan_id_t ch) noexcept {
	if (ch <= chan_id_t::DPCM)
		return (std::size_t)ch - (std::size_t)chan_id_t::SQUARE1;
	if (ch <= chan_id_t::VRC6_SAWTOOTH)
		return (std::size_t)ch - (std::size_t)chan_id_t::VRC6_PULSE1;
	if (ch <= chan_id_t::MMC5_VOICE)
		return (std::size_t)ch - (std::size_t)chan_id_t::MMC5_SQUARE1;
	if (ch <= chan_id_t::N163_CH8)
		return (std::size_t)ch - (std::size_t)chan_id_t::N163_CH1;
	if (ch <= chan_id_t::FDS)
		return (std::size_t)ch - (std::size_t)chan_id_t::FDS;
	if (ch <= chan_id_t::VRC7_CH6)
		return (std::size_t)ch - (std::size_t)chan_id_t::VRC7_CH1;
	if (ch <= chan_id_t::S5B_CH3)
		return (std::size_t)ch - (std::size_t)chan_id_t::S5B_CH1;
	return (std::size_t)chan_id_t::NONE;
}

constexpr chan_id_t MakeChannelIndex(sound_chip_t chip, unsigned subindex) noexcept {
	switch (chip) {
	case sound_chip_t::APU:
		if (subindex < MAX_CHANNELS_2A03)
			return (chan_id_t)((unsigned)chan_id_t::SQUARE1 + subindex);
		break;
	case sound_chip_t::VRC6:
		if (subindex < MAX_CHANNELS_VRC6)
			return (chan_id_t)((unsigned)chan_id_t::VRC6_PULSE1 + subindex);
		break;
	case sound_chip_t::MMC5:
		if (subindex < MAX_CHANNELS_MMC5)
			return (chan_id_t)((unsigned)chan_id_t::MMC5_SQUARE1 + subindex);
		break;
	case sound_chip_t::N163:
		if (subindex < MAX_CHANNELS_N163)
			return (chan_id_t)((unsigned)chan_id_t::N163_CH1 + subindex);
		break;
	case sound_chip_t::FDS:
		if (subindex < MAX_CHANNELS_FDS)
			return (chan_id_t)((unsigned)chan_id_t::FDS + subindex);
		break;
	case sound_chip_t::VRC7:
		if (subindex < MAX_CHANNELS_VRC7)
			return (chan_id_t)((unsigned)chan_id_t::VRC7_CH1 + subindex);
		break;
	case sound_chip_t::S5B:
		if (subindex < MAX_CHANNELS_S5B)
			return (chan_id_t)((unsigned)chan_id_t::S5B_CH1 + subindex);
		break;
	}
	return chan_id_t::NONE;
}

enum apu_machine_t : unsigned char {
	MACHINE_NTSC,
	MACHINE_PAL,
};
