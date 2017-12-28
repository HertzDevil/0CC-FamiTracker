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
