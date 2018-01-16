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

#include <cstdint>
#include <stdexcept>

enum sound_chip_t : std::uint8_t;
enum class chan_id_t : unsigned;
enum apu_machine_t : unsigned char;

// // // TODO: use enum_traits (MSVC broke it)
constexpr auto value_cast(sound_chip_t chip) noexcept {
	using T = std::underlying_type_t<sound_chip_t>;
	return static_cast<T>(static_cast<T>(chip) & static_cast<T>(/*SNDCHIP_ALL*/0x3F));
}

struct sound_chip_flag_t {
	using value_type = std::underlying_type_t<sound_chip_t>;

	constexpr sound_chip_flag_t() noexcept = default;
	constexpr sound_chip_flag_t(sound_chip_t chips) noexcept : chips_(chips) { }
	explicit constexpr sound_chip_flag_t(int chips) noexcept : chips_(chips & 0x3Fu) { }

	constexpr bool ContainsChip(sound_chip_t chip) const noexcept {
		return (chips_ & value_cast(chip)) == value_cast(chip);
	}

	constexpr bool IsMultiChip() const noexcept {
		return (chips_ & (chips_ - 1)) != 0;
	}

	constexpr sound_chip_t GetSoundChip() const {
		return !IsMultiChip() ? (sound_chip_t)chips_ :
			throw std::runtime_error {"Sound chip flag contains more than one sound chip"};
	}

	constexpr sound_chip_flag_t WithChip(sound_chip_t chip) const noexcept {
		return sound_chip_flag_t {chips_ | value_cast(chip)};
	}

	constexpr sound_chip_flag_t WithoutChip(sound_chip_t chip) const noexcept {
		return sound_chip_flag_t {chips_ & ~value_cast(chip)};
	}

	constexpr sound_chip_flag_t EnableChip(sound_chip_t chip, bool enable) const noexcept {
		return enable ? WithChip(chip) : WithoutChip(chip);
	}

	constexpr sound_chip_flag_t MergedWith(sound_chip_flag_t other) const noexcept {
		return sound_chip_flag_t {chips_ | other.chips_};
	}

	constexpr bool operator==(const sound_chip_flag_t &other) const noexcept {
		return chips_ == other.chips_;
	}

	constexpr bool operator!=(const sound_chip_flag_t &other) const noexcept {
		return chips_ != other.chips_;
	}

	friend constexpr auto value_cast(sound_chip_flag_t flag) noexcept { // custom
		return flag.chips_;
	}

private:
	value_type chips_ = 0;
};

constexpr bool operator==(const sound_chip_flag_t &lhs, const sound_chip_t &rhs) noexcept {
	return lhs == (sound_chip_flag_t)rhs;
}

constexpr bool operator!=(const sound_chip_flag_t &lhs, const sound_chip_t &rhs) noexcept {
	return lhs != (sound_chip_flag_t)rhs;
}

constexpr bool operator==(const sound_chip_t &lhs, const sound_chip_flag_t &rhs) noexcept {
	return (sound_chip_flag_t)lhs == rhs;
}

constexpr bool operator!=(const sound_chip_t &lhs, const sound_chip_flag_t &rhs) noexcept {
	return (sound_chip_flag_t)lhs != rhs;
}
