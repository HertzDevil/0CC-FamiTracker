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

#include <cstdint>
#include <array>

namespace ft0cc::doc {

class inst_sequence {
public:
	using entry_type = std::int8_t;

	enum class setting {
		def = 0,

		vol_16_steps = 0,
		vol_64_steps,		// // // 050B
		vol_count,

		arp_absolute = 0,
		arp_fixed,
		arp_relative,
		arp_scheme,
		arp_count,

		pitch_relative = 0,
		pitch_absolute,
		pitch_sweep,		// // // 050B
#ifdef NDEBUG
		pitch_count = 2,
#else
		pitch_count,
#endif
		hipitch_default = 0,
		hipitch_count,

		duty_default = 0,
		duty_count,
	};

	static constexpr std::size_t max_items = 252u;
	static constexpr std::size_t none = (std::size_t)-1;

	constexpr inst_sequence() = default;

	entry_type entry(std::size_t index) const;
	void set_entry(std::size_t index, entry_type value);

	std::size_t size() const;
	std::size_t compiled_size() const;
	void resize(std::size_t sz);

	std::size_t loop_point() const;
	void set_loop_point(std::size_t index);

	std::size_t release_point() const;
	void set_release_point(std::size_t index);

	setting sequence_setting() const;
	void set_sequence_setting(setting s);

	bool operator==(const inst_sequence &other) const;
	bool operator!=(const inst_sequence &other) const { return !(*this == other); }

	auto begin() { return entries_.begin(); }
	auto end() { return entries_.begin() + len_; }
	auto begin() const { return entries_.cbegin(); }
	auto end() const { return entries_.cbegin() + len_; }

private:
	std::size_t len_ = 0;
	std::size_t loop_ = none;
	std::size_t release_ = none;
	setting setting_ = setting::def;
	std::array<int8_t, max_items> entries_ = { };
};

} // namespace ft0cc::doc
