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
#include "StrongOrdering.h"		// // //

// // // moved from FamiTrackerTypes.h
ENUM_CLASS_STANDARD(machine_t, std::uint8_t) {
	NTSC, PAL,
	min = NTSC, max = PAL, none = static_cast<std::uint8_t>(-1),
};

inline constexpr machine_t DEFAULT_MACHINE_TYPE = machine_t::NTSC;		// // //

inline constexpr unsigned MASTER_CLOCK_NTSC = 1789773;		// // // moved from CAPU
inline constexpr unsigned MASTER_CLOCK_PAL  = 1662607;
inline constexpr unsigned FRAME_RATE_NTSC   = 60;
inline constexpr unsigned FRAME_RATE_PAL    = 50;

ENUM_CLASS_STANDARD(sound_chip_t, std::uint8_t) {		// // //
	APU, VRC6, VRC7, FDS, MMC5, N163, S5B,
	min = APU, max = S5B, none = static_cast<std::uint8_t>(-1),
};

inline constexpr std::size_t SOUND_CHIP_COUNT = enum_count<sound_chip_t>();

ENUM_CLASS_STANDARD(apu_subindex_t, std::uint8_t) {
	pulse1, pulse2, triangle, noise, dpcm,
	min = pulse1, max = dpcm, none = static_cast<std::uint8_t>(-1),
};
ENUM_CLASS_STANDARD(vrc6_subindex_t, std::uint8_t) {
	pulse1, pulse2, sawtooth,
	min = pulse1, max = sawtooth, none = static_cast<std::uint8_t>(-1),
};
ENUM_CLASS_STANDARD(mmc5_subindex_t, std::uint8_t) {
	pulse1, pulse2, pcm,
	min = pulse1, max = pcm, none = static_cast<std::uint8_t>(-1),
};
ENUM_CLASS_STANDARD(n163_subindex_t, std::uint8_t) {
	ch1, ch2, ch3, ch4, ch5, ch6, ch7, ch8,
	min = ch1, max = ch8, none = static_cast<std::uint8_t>(-1),
};
ENUM_CLASS_STANDARD(fds_subindex_t, std::uint8_t) {
	wave,
	min = wave, max = wave, none = static_cast<std::uint8_t>(-1),
};
ENUM_CLASS_STANDARD(vrc7_subindex_t, std::uint8_t) {
	ch1, ch2, ch3, ch4, ch5, ch6,
	min = ch1, max = ch6, none = static_cast<std::uint8_t>(-1),
};
ENUM_CLASS_STANDARD(s5b_subindex_t, std::uint8_t) {
	square1, square2, square3,
	min = square1, max = square3, none = static_cast<std::uint8_t>(-1),
};

// // // moved from FamiTrackerTypes.h
inline constexpr std::size_t MAX_CHANNELS_2A03 = enum_count<apu_subindex_t>();
inline constexpr std::size_t MAX_CHANNELS_VRC6 = enum_count<vrc6_subindex_t>();
inline constexpr std::size_t MAX_CHANNELS_VRC7 = enum_count<vrc7_subindex_t>();
inline constexpr std::size_t MAX_CHANNELS_FDS  = enum_count<fds_subindex_t>();
inline constexpr std::size_t MAX_CHANNELS_MMC5 = enum_count<mmc5_subindex_t>(); // includes pcm
inline constexpr std::size_t MAX_CHANNELS_N163 = enum_count<n163_subindex_t>();
inline constexpr std::size_t MAX_CHANNELS_S5B  = enum_count<s5b_subindex_t>();

inline constexpr std::size_t CHANID_COUNT =
	MAX_CHANNELS_2A03 +
	MAX_CHANNELS_VRC6 +
	MAX_CHANNELS_VRC7 +
	MAX_CHANNELS_FDS  +
	MAX_CHANNELS_MMC5 +
	MAX_CHANNELS_N163 +
	MAX_CHANNELS_S5B;



struct stChannelID {		// // //
	sound_chip_t Chip = sound_chip_t::none;
	std::uint8_t Subindex = 0u;
	std::uint8_t Ident = 0u;

	constexpr stChannelID() noexcept = default;
	constexpr stChannelID(sound_chip_t chip, std::uint8_t subindex) noexcept : Chip(chip), Subindex(subindex) { }
	constexpr stChannelID(std::uint8_t id, sound_chip_t chip, std::uint8_t subindex) noexcept : Chip(chip), Subindex(subindex), Ident(id) { }

	static constexpr stChannelID FromInteger(std::uint32_t x) noexcept {
		return {
			static_cast<std::uint8_t>(x & 0xFF),
			enum_cast<sound_chip_t>(x >> 16),
			static_cast<std::uint8_t>((x >> 8) & 0xFF),
		};
	}

	constexpr std::uint32_t ToInteger() const noexcept {
		return Ident | (Subindex << 8) | (value_cast(Chip) << 16);
	}

	constexpr stChannelID(apu_subindex_t  subindex) noexcept : stChannelID(sound_chip_t::APU , value_cast(subindex)) { }
	constexpr stChannelID(vrc6_subindex_t subindex) noexcept : stChannelID(sound_chip_t::VRC6, value_cast(subindex)) { }
	constexpr stChannelID(vrc7_subindex_t subindex) noexcept : stChannelID(sound_chip_t::VRC7, value_cast(subindex)) { }
	constexpr stChannelID(fds_subindex_t  subindex) noexcept : stChannelID(sound_chip_t::FDS , value_cast(subindex)) { }
	constexpr stChannelID(mmc5_subindex_t subindex) noexcept : stChannelID(sound_chip_t::MMC5, value_cast(subindex)) { }
	constexpr stChannelID(n163_subindex_t subindex) noexcept : stChannelID(sound_chip_t::N163, value_cast(subindex)) { }
	constexpr stChannelID(s5b_subindex_t  subindex) noexcept : stChannelID(sound_chip_t::S5B , value_cast(subindex)) { }

	constexpr int compare(const stChannelID &other) const noexcept {
		if (Chip > other.Chip)
			return 1;
		if (Chip < other.Chip)
			return -1;
		if (Chip == sound_chip_t::none)
			return 0;
		if (Subindex > other.Subindex)
			return 1;
		if (Subindex < other.Subindex)
			return -1;
		if (Ident > other.Ident)
			return 1;
		if (Ident < other.Ident)
			return -1;
		return 0;
	}

	// constexpr std::strong_ordering operator<=>(const stChannelID &other) const noexcept = default;
};

ENABLE_STRONG_ORDERING(stChannelID);

struct stChannelID_ident_less {
	static constexpr int compare(const stChannelID &lhs, const stChannelID &rhs) noexcept {
		if (lhs.Chip == sound_chip_t::none && rhs.Chip == sound_chip_t::none)
			return 0;
		if (lhs.Ident > rhs.Ident)
			return 1;
		if (lhs.Ident < rhs.Ident)
			return -1;
		if (lhs.Chip > rhs.Chip)
			return 1;
		if (lhs.Chip < rhs.Chip)
			return -1;
		if (lhs.Subindex > rhs.Subindex)
			return 1;
		if (lhs.Subindex < rhs.Subindex)
			return -1;
		return 0;
	}
	constexpr bool operator()(const stChannelID &lhs, const stChannelID &rhs) const noexcept {
		return compare(lhs, rhs) < 0;
	}
};

constexpr bool IsAPUPulse(stChannelID id) noexcept {
	return id.Chip == sound_chip_t::APU && (
		id.Subindex == value_cast(apu_subindex_t::pulse1) || id.Subindex == value_cast(apu_subindex_t::pulse2));
}

constexpr bool IsAPUTriangle(stChannelID id) noexcept {
	return id.Chip == sound_chip_t::APU && id.Subindex == value_cast(apu_subindex_t::triangle);
}

constexpr bool IsAPUNoise(stChannelID id) noexcept {
	return id.Chip == sound_chip_t::APU && id.Subindex == value_cast(apu_subindex_t::noise);
}

constexpr bool IsDPCM(stChannelID id) noexcept {
	return id.Chip == sound_chip_t::APU && id.Subindex == value_cast(apu_subindex_t::dpcm);
}

constexpr bool IsVRC6Sawtooth(stChannelID id) noexcept {
	return id.Chip == sound_chip_t::VRC6 && id.Subindex == value_cast(vrc6_subindex_t::sawtooth);
}
