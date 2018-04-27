/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * 0CC-FamiTracker is (C) 2014-2018 HertzDevil
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public License Version 2, as described below:
 *
 * This file is free software: you may copy, redistribute and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/. */

#include "ft0cc/doc/groove.hpp"
#include <algorithm>
#include <numeric>

using namespace ft0cc::doc;

ft0cc::doc::groove::groove(std::initializer_list<entry_type> entries) {
	std::size_t i = 0;
	len_ = std::min(entries.size(), max_size);
	for (entry_type x : entries) {
		if (i >= len_)
			break;
		entries_[i++] = x;
	}
}

groove::entry_type groove::entry(std::size_t index) const {
	return len_ ? entries_[index % len_] : default_speed;
}

void groove::set_entry(std::size_t index, entry_type value) {
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

int groove::compare(const groove &other) const {
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
//	return std::lexicographical_compare_3way(begin(), begin() + len_, other.begin(), other.begin() + len_);
}
