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
	dpcm_sample(std::vector<sample_t> samples, std::string_view name);

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
	void tilt(std::size_t b, std::size_t e, double seed);

private:
	std::vector<sample_t> data_;
	std::string name_;
};

} // namespace ft0cc::doc
