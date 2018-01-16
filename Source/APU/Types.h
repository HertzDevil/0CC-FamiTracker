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

#include "Types_fwd.h"
#include <type_traits>
#include <cstddef>

enum sound_chip_t : std::uint8_t {		// // //
	SNDCHIP_NONE = 0x00u,
	SNDCHIP_2A03 = 0x00u,
	SNDCHIP_VRC6 = 0x01u,
	SNDCHIP_VRC7 = 0x02u,
	SNDCHIP_FDS  = 0x04u,
	SNDCHIP_MMC5 = 0x08u,
	SNDCHIP_N163 = 0x10u,
	SNDCHIP_S5B  = 0x20u,
	SNDCHIP_ALL  = 0x3Fu,
};

enum sound_chip_flag_t : std::uint8_t {
	SNDCHIP_FLAG_NONE = SNDCHIP_NONE,
};

// // // TODO: use enum_traits (MSVC broke it)
constexpr auto value_cast(sound_chip_t chip) noexcept {
	using T = std::underlying_type_t<sound_chip_t>;
	return static_cast<T>(static_cast<T>(chip) & static_cast<T>(SNDCHIP_ALL));
}

constexpr bool ContainsSoundChip(sound_chip_t flag, sound_chip_t chip) noexcept {
	return (value_cast(flag) & value_cast(chip)) == value_cast(chip);
}

constexpr bool IsMultiChip(sound_chip_t flag) noexcept {
	auto v = value_cast(flag);
	return (v & (v - 1)) != 0;
}

constexpr sound_chip_t WithSoundChip(sound_chip_t flag, sound_chip_t chip) noexcept {
	return (sound_chip_t)(value_cast(flag) | value_cast(chip));
}

constexpr sound_chip_t WithoutSoundChip(sound_chip_t flag, sound_chip_t chip) noexcept {
	return (sound_chip_t)(value_cast(flag) & ~value_cast(chip));
}

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
};

inline constexpr std::size_t CHANID_COUNT = (unsigned)chan_id_t::COUNT;

// // // TODO: use enum_traits (MSVC broke it)
constexpr auto value_cast(chan_id_t ch) noexcept {
	auto x = static_cast<std::underlying_type_t<chan_id_t>>(ch);
	if (x < static_cast<std::underlying_type_t<chan_id_t>>(chan_id_t::COUNT))
		return x;
	return static_cast<std::underlying_type_t<chan_id_t>>(chan_id_t::NONE);
}

constexpr sound_chip_t GetChipFromChannel(chan_id_t ch) noexcept {
	if (ch <= chan_id_t::DPCM)
		return SNDCHIP_NONE;
	if (ch <= chan_id_t::VRC6_SAWTOOTH)
		return SNDCHIP_VRC6;
	if (ch <= chan_id_t::MMC5_VOICE)
		return SNDCHIP_MMC5;
	if (ch <= chan_id_t::N163_CH8)
		return SNDCHIP_N163;
	if (ch <= chan_id_t::FDS)
		return SNDCHIP_FDS;
	if (ch <= chan_id_t::VRC7_CH6)
		return SNDCHIP_VRC7;
	if (ch <= chan_id_t::S5B_CH3)
		return SNDCHIP_S5B;
	return SNDCHIP_NONE;
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
	case SNDCHIP_NONE:
		if (subindex < 5)
			return (chan_id_t)((unsigned)chan_id_t::SQUARE1 + subindex);
		break;
	case SNDCHIP_VRC6:
		if (subindex < 3)
			return (chan_id_t)((unsigned)chan_id_t::VRC6_PULSE1 + subindex);
		break;
	case SNDCHIP_MMC5:
		if (subindex < 3)
			return (chan_id_t)((unsigned)chan_id_t::MMC5_SQUARE1 + subindex);
		break;
	case SNDCHIP_N163:
		if (subindex < 8)
			return (chan_id_t)((unsigned)chan_id_t::N163_CH1 + subindex);
		break;
	case SNDCHIP_FDS:
		if (subindex < 1)
			return (chan_id_t)((unsigned)chan_id_t::FDS + subindex);
		break;
	case SNDCHIP_VRC7:
		if (subindex < 6)
			return (chan_id_t)((unsigned)chan_id_t::VRC7_CH1 + subindex);
		break;
	case SNDCHIP_S5B:
		if (subindex < 3)
			return (chan_id_t)((unsigned)chan_id_t::S5B_CH1 + subindex);
		break;
	}
	return chan_id_t::NONE;
}

enum apu_machine_t : unsigned char {
	MACHINE_NTSC,
	MACHINE_PAL
};
