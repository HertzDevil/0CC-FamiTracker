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

enum class apu_subindex_t : std::size_t {
	pulse1, pulse2, triangle, noise, dpcm,
	count,
};
enum class vrc6_subindex_t : std::size_t {
	pulse1, pulse2, sawtooth,
	count,
};
enum class mmc5_subindex_t : std::size_t {
	pulse1, pulse2, pcm,
	count,
};
enum class n163_subindex_t : std::size_t {
	ch1, ch2, ch3, ch4, ch5, ch6, ch7, ch8,
	count,
};
enum class fds_subindex_t : std::size_t {
	wave,
	count,
};
enum class vrc7_subindex_t : std::size_t {
	ch1, ch2, ch3, ch4, ch5, ch6,
	count,
};
enum class s5b_subindex_t : std::size_t {
	square1, square2, square3,
	count,
};

// // // moved from FamiTrackerTypes.h
inline constexpr std::size_t MAX_CHANNELS_2A03 = value_cast(apu_subindex_t ::count);
inline constexpr std::size_t MAX_CHANNELS_VRC6 = value_cast(vrc6_subindex_t::count);
inline constexpr std::size_t MAX_CHANNELS_VRC7 = value_cast(vrc7_subindex_t::count);
inline constexpr std::size_t MAX_CHANNELS_FDS  = value_cast(fds_subindex_t ::count);
inline constexpr std::size_t MAX_CHANNELS_MMC5 = value_cast(mmc5_subindex_t::count); // includes pcm
inline constexpr std::size_t MAX_CHANNELS_N163 = value_cast(n163_subindex_t::count);
inline constexpr std::size_t MAX_CHANNELS_S5B  = value_cast(s5b_subindex_t ::count);

inline constexpr std::size_t CHANID_COUNT = (unsigned)chan_id_t::COUNT;

enum apu_machine_t : unsigned char {
	MACHINE_NTSC,
	MACHINE_PAL,
};



struct stChannelID {		// // //
	sound_chip_t Chip = sound_chip_t::NONE;
	std::size_t Subindex = 0u;
	std::size_t Ident = 0u;

	constexpr stChannelID() noexcept = default;
	constexpr stChannelID(sound_chip_t chip, std::size_t subindex) noexcept : Chip(chip), Subindex(subindex) { }
	constexpr stChannelID(std::size_t id, sound_chip_t chip, std::size_t subindex) noexcept : Chip(chip), Subindex(subindex), Ident(id) { }
	constexpr stChannelID(chan_id_t id) noexcept : Chip(GetChipFromChannel(id)), Subindex(GetChannelSubIndex(id)) { }

	constexpr stChannelID(apu_subindex_t  subindex) noexcept : stChannelID(sound_chip_t::APU , value_cast(subindex)) { }
	constexpr stChannelID(vrc6_subindex_t subindex) noexcept : stChannelID(sound_chip_t::VRC6, value_cast(subindex)) { }
	constexpr stChannelID(vrc7_subindex_t subindex) noexcept : stChannelID(sound_chip_t::VRC7, value_cast(subindex)) { }
	constexpr stChannelID(fds_subindex_t  subindex) noexcept : stChannelID(sound_chip_t::FDS , value_cast(subindex)) { }
	constexpr stChannelID(mmc5_subindex_t subindex) noexcept : stChannelID(sound_chip_t::MMC5, value_cast(subindex)) { }
	constexpr stChannelID(n163_subindex_t subindex) noexcept : stChannelID(sound_chip_t::N163, value_cast(subindex)) { }
	constexpr stChannelID(s5b_subindex_t  subindex) noexcept : stChannelID(sound_chip_t::S5B , value_cast(subindex)) { }

	explicit constexpr operator chan_id_t() const noexcept {
		switch (Chip) {
		case sound_chip_t::APU:
			if (Subindex < MAX_CHANNELS_2A03)
				return enum_cast<chan_id_t>(value_cast(chan_id_t::SQUARE1) + Subindex);
			break;
		case sound_chip_t::VRC6:
			if (Subindex < MAX_CHANNELS_VRC6)
				return enum_cast<chan_id_t>(value_cast(chan_id_t::VRC6_PULSE1) + Subindex);
			break;
		case sound_chip_t::MMC5:
			if (Subindex < MAX_CHANNELS_MMC5)
				return enum_cast<chan_id_t>(value_cast(chan_id_t::MMC5_SQUARE1) + Subindex);
			break;
		case sound_chip_t::N163:
			if (Subindex < MAX_CHANNELS_N163)
				return enum_cast<chan_id_t>(value_cast(chan_id_t::N163_CH1) + Subindex);
			break;
		case sound_chip_t::FDS:
			if (Subindex < MAX_CHANNELS_FDS)
				return enum_cast<chan_id_t>(value_cast(chan_id_t::FDS) + Subindex);
			break;
		case sound_chip_t::VRC7:
			if (Subindex < MAX_CHANNELS_VRC7)
				return enum_cast<chan_id_t>(value_cast(chan_id_t::VRC7_CH1) + Subindex);
			break;
		case sound_chip_t::S5B:
			if (Subindex < MAX_CHANNELS_S5B)
				return enum_cast<chan_id_t>(value_cast(chan_id_t::S5B_CH1) + Subindex);
			break;
		}
		return chan_id_t::NONE;
	}

	constexpr int compare(const stChannelID &other) const noexcept {
		if (Chip > other.Chip)
			return 1;
		if (Chip < other.Chip)
			return -1;
		if (Chip == sound_chip_t::NONE)
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

	constexpr bool operator==(const stChannelID &other) const noexcept { return compare(other) == 0; }
	constexpr bool operator!=(const stChannelID &other) const noexcept { return compare(other) != 0; }
	constexpr bool operator< (const stChannelID &other) const noexcept { return compare(other) <  0; }
	constexpr bool operator<=(const stChannelID &other) const noexcept { return compare(other) <= 0; }
	constexpr bool operator> (const stChannelID &other) const noexcept { return compare(other) >  0; }
	constexpr bool operator>=(const stChannelID &other) const noexcept { return compare(other) >= 0; }
	// constexpr std::strong_ordering operator<=>(const stChannelID &other) const noexcept = default;

private:
	static constexpr sound_chip_t GetChipFromChannel(chan_id_t ch) noexcept {
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

	static constexpr std::size_t GetChannelSubIndex(chan_id_t ch) noexcept {
		if (ch <= chan_id_t::DPCM)
			return value_cast(ch) - value_cast(chan_id_t::SQUARE1);
		if (ch <= chan_id_t::VRC6_SAWTOOTH)
			return value_cast(ch) - value_cast(chan_id_t::VRC6_PULSE1);
		if (ch <= chan_id_t::MMC5_VOICE)
			return value_cast(ch) - value_cast(chan_id_t::MMC5_SQUARE1);
		if (ch <= chan_id_t::N163_CH8)
			return value_cast(ch) - value_cast(chan_id_t::N163_CH1);
		if (ch <= chan_id_t::FDS)
			return value_cast(ch) - value_cast(chan_id_t::FDS);
		if (ch <= chan_id_t::VRC7_CH6)
			return value_cast(ch) - value_cast(chan_id_t::VRC7_CH1);
		if (ch <= chan_id_t::S5B_CH3)
			return value_cast(ch) - value_cast(chan_id_t::S5B_CH1);
		return value_cast(chan_id_t::NONE);
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

constexpr sound_chip_t GetChipFromChannel(stChannelID id) noexcept {
	return id.Chip;
}

constexpr std::size_t GetChannelSubIndex(stChannelID id) noexcept {
	return id.Subindex;
}
