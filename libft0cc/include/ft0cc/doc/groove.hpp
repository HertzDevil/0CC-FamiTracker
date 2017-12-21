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

namespace ft0cc::doc {

class groove {
public:
	static constexpr std::size_t max_size = 128u;
	static constexpr uint8_t default_speed = 6u;

	explicit groove(uint8_t speed = 0u);

	void copy(const groove *source);
	void clear(uint8_t speed);
	uint8_t entry(std::size_t index) const;
	void set_entry(std::size_t index, uint8_t value);
	std::size_t size() const;
	void resize(std::size_t size);
	double average() const;

private:
	std::size_t len_ = 0;
	std::array<uint8_t, max_size> entries_;
};

} // namespace ft0cc::doc
