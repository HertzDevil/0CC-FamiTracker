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

#include <vector>
#include <string>
#include <cstdint>

namespace ft0cc::doc {

class dpcm_sample {
public:
	using sample_t = std::uint8_t;

	static constexpr std::size_t max_size = 0xFF1;
	static constexpr std::size_t max_name_length = 255;
	static constexpr sample_t pad_value = 0xAA;

	dpcm_sample() = default;
	explicit dpcm_sample(std::size_t sz);
	dpcm_sample(const std::vector<sample_t> &samples, std::string_view name);

	bool operator==(const dpcm_sample &other) const;
	bool operator!=(const dpcm_sample &other) const { return !(*this == other); }

	std::string_view name() const;
	void rename(std::string_view sv);

	sample_t sample_at(std::size_t pos) const;
	void set_sample_at(std::size_t pos, sample_t sample);
	const sample_t *data() const;

	std::size_t size() const;
	void resize(std::size_t sz);
	void cut_samples(std::size_t b, std::size_t e);
	void tilt(std::size_t b, std::size_t e);

private:
	std::vector<sample_t> data_;
	std::string name_;
};

} // namespace ft0cc::doc
