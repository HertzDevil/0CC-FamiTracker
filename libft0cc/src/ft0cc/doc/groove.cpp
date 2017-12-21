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

ft0cc::doc::groove::groove(std::initializer_list<uint8_t> entries) {
	std::size_t i = 0;
	len_ = std::min(entries.size(), max_size);
	for (uint8_t x : entries) {
		if (i >= len_)
			break;
		entries_[i++] = x;
	}
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

std::size_t ft0cc::doc::groove::compiled_size() const {
	return size() + 2;
}

void groove::resize(std::size_t size) {
	if (size <= max_size) {
		if (size > len_)
			std::fill(entries_.begin() + len_, entries_.begin() + size, (uint8_t)0);
		len_ = size;
	}
}

double groove::average() const {
	return !len_ ? default_speed :
		std::accumulate(entries_.begin(), entries_.begin() + len_, 0.) / len_;
}

int ft0cc::doc::groove::compare(const groove &other) const {
	auto b1 = begin();
	auto e1 = end();
	auto b2 = other.begin();
	auto e2 = other.end();

	while (b1 != e1 && b2 != e2) {
		if (*b1 < *b2)
			return -1;
		if (*b1 > *b2)
			return 1;
		++b1;
		++b2;
	}
	return b2 != e2 ? -1 : (b1 != e1 ? 1 : 0);
//	return std::lexicographical_compare_3way(begin(), end(), other.begin(), other.end());
}
