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


#pragma once

#include <array>
#include <cstdint>
#include <initializer_list>

namespace ft0cc::doc {

class groove {
public:
	using entry_type = std::uint8_t;

	static constexpr std::size_t max_size = 128u;
	static constexpr entry_type default_speed = 6u;

	constexpr groove() = default;
	groove(std::initializer_list<entry_type> entries);

	entry_type entry(std::size_t index) const;
	void set_entry(std::size_t index, entry_type value);

	std::size_t size() const;
	std::size_t compiled_size() const;
	void resize(std::size_t size);

	double average() const;

	int compare(const groove &other) const;
	bool operator==(const groove &other) const { return compare(other) == 0; }
	bool operator!=(const groove &other) const { return compare(other) != 0; }
	bool operator< (const groove &other) const { return compare(other) <  0; }
	bool operator<=(const groove &other) const { return compare(other) <= 0; }
	bool operator> (const groove &other) const { return compare(other) >  0; }
	bool operator>=(const groove &other) const { return compare(other) >= 0; }

	auto begin() { return entries_.begin(); }
	auto end() { return entries_.begin() + len_; }
	auto begin() const { return entries_.cbegin(); }
	auto end() const { return entries_.cbegin() + len_; }

private:
	std::size_t len_ = 0;
	std::array<entry_type, max_size> entries_ = { };
};

} // namespace ft0cc::doc
