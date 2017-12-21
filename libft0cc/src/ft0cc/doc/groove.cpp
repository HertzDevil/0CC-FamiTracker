/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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

#include "ft0cc/doc/groove.hpp"
#include <algorithm>
#include <numeric>

using namespace ft0cc::doc;

groove::groove(uint8_t Speed) {
	clear(Speed);
}

void groove::copy(const groove *Source) { // TODO: remove
	*this = *Source;
}

void groove::clear(uint8_t Speed) {
	resize(Speed > 0 ? 1 : 0);
	set_entry(0, Speed);
}

uint8_t groove::entry(std::size_t index) const {
	return len_ ? entries_[index % len_] : default_speed;
}

void groove::set_entry(std::size_t index, uint8_t value) {
	if (len_)
		entries_[index % len_] = value;
}

std::size_t groove::size() const {
	return len_;
}

void groove::resize(std::size_t size) {
	if (size <= max_size) {
		if (size > len_)
			std::fill(begin(entries_) + len_, begin(entries_) + size, (uint8_t)0);
		len_ = size;
	}
}

double groove::average() const {
	return !len_ ? default_speed :
		std::accumulate(begin(entries_), begin(entries_) + len_, 0.) / len_;
}
