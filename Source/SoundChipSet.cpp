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

#include "SoundChipSet.h"

bool CSoundChipSet::HasChips() const {
	return chips_.any();
}

CSoundChipSet::value_type CSoundChipSet::GetFlag() const {
	return chips_.to_ulong();
}

CSoundChipSet::value_type CSoundChipSet::GetNSFFlag() const {
	return GetFlag() >> 1;
}

std::size_t CSoundChipSet::GetChipCount() const {
	return chips_.count();
}

bool CSoundChipSet::ContainsChip(sound_chip_t chip) const {
	return chips_.test(value_cast(chip));
}

bool CSoundChipSet::IsMultiChip() const noexcept {
	return WithoutChip(sound_chip_t::APU).chips_.count() > 1;
}

sound_chip_t CSoundChipSet::GetSoundChip() const {
	if (IsMultiChip())
		throw std::runtime_error {"Sound chip flag contains more than one sound chip"};
	for (std::size_t i = 0; i < chips_.size(); ++i)
		if (chips_.test(i))
			return (sound_chip_t)i;
	return sound_chip_t::NONE;
}

CSoundChipSet CSoundChipSet::EnableChip(sound_chip_t chip, bool enable) const {
	auto flag = *this;
	flag.chips_.set(value_cast(chip), enable);
	return flag;
}

CSoundChipSet CSoundChipSet::WithChip(sound_chip_t chip) const {
	return EnableChip(chip, true);
}

CSoundChipSet CSoundChipSet::WithoutChip(sound_chip_t chip) const {
	return EnableChip(chip, false);
}

CSoundChipSet CSoundChipSet::MergedWith(CSoundChipSet other) const {
	return CSoundChipSet {(chips_ | other.chips_).to_ulong()};
}

bool CSoundChipSet::operator==(const CSoundChipSet &other) const {
	return chips_ == other.chips_;
}

bool CSoundChipSet::operator!=(const CSoundChipSet &other) const {
	return chips_ != other.chips_;
}

bool operator==(const sound_chip_t &lhs, const CSoundChipSet &rhs) {
	return rhs == lhs;
}

bool operator!=(const sound_chip_t &lhs, const CSoundChipSet &rhs) {
	return rhs != lhs;
}
