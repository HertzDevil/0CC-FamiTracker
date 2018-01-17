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

#include <bitset>
#include <stdexcept>
#include "APU/Types.h"

class CSoundChipSet {
public:
	using value_type = std::uint32_t;
	static constexpr value_type NSF_MAX_FLAG = 0x3Fu;

private:
	explicit constexpr CSoundChipSet(value_type chips) noexcept : chips_(chips) { }

public:
	explicit constexpr CSoundChipSet() noexcept = default; // no chips enabled at all
	constexpr CSoundChipSet(sound_chip_t chip) noexcept : chips_(1ull << static_cast<unsigned long long>(chip)) { }

	static constexpr CSoundChipSet FromFlag(value_type flag) noexcept {
		return CSoundChipSet {flag};
	}

	static const CSoundChipSet FromNSFFlag(value_type flag) {
		return FromFlag(flag << 1).WithChip(sound_chip_t::APU);
	}

	static constexpr CSoundChipSet All() noexcept {
		return CSoundChipSet {(unsigned)-1};
	}

	bool HasChips() const;

	value_type GetFlag() const;
	value_type GetNSFFlag() const;

	bool ContainsChip(sound_chip_t chip) const;
	bool IsMultiChip() const noexcept;
	sound_chip_t GetSoundChip() const;

	CSoundChipSet EnableChip(sound_chip_t chip, bool enable) const;
	CSoundChipSet WithChip(sound_chip_t chip) const;
	CSoundChipSet WithoutChip(sound_chip_t chip) const;

	CSoundChipSet MergedWith(CSoundChipSet other) const;

	bool operator==(const CSoundChipSet &other) const;
	bool operator!=(const CSoundChipSet &other) const;
	friend bool operator==(const sound_chip_t &lhs, const CSoundChipSet &rhs);
	friend bool operator!=(const sound_chip_t &lhs, const CSoundChipSet &rhs);

private:
	std::bitset<SOUND_CHIP_COUNT> chips_ = { };
};
