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
#include "ft0cc/cpputil/iter.hpp"
#include "ft0cc/doc/constants.hpp"
#include "ft0cc/doc/effect_command.hpp"
#include "ft0cc/doc/pitch.hpp"

namespace ft0cc::doc {

class pattern_note {
public:
	static_assert(static_cast<std::uint8_t>(max_volumes) == max_volumes,
		"ft0cc::doc::max_volumes cannot fit into pattern note");
	static_assert(
		static_cast<std::uint8_t>(max_instruments) == max_instruments,
		"ft0cc::doc::max_instruments cannot fit into pattern note");

	constexpr pattern_note() noexcept = default;

	constexpr bool operator==(const pattern_note &other) const noexcept {
		return note_ == other.note_ &&
			vol_ == other.vol_ &&
			inst_ == other.inst_ &&
			(note_ == pitch::none || oct_ == other.oct_ ||
				note_ == pitch::halt || note_ == pitch::release) &&
			fx_ == other.fx_;
	}
	constexpr bool operator!=(const pattern_note &other) const noexcept {
		return !operator==(other);
	}

	constexpr pitch note() const noexcept {
		return note_;
	}
	constexpr unsigned char oct() const noexcept {
		return oct_;
	}
	constexpr unsigned char vol() const noexcept {
		return vol_;
	}
	constexpr unsigned char inst() const noexcept {
		return inst_;
	}
	constexpr effect_command fx_cmd(std::size_t n) const noexcept {
		return n < std::size(fx_) ? fx_[n] : effect_command { };
	}
	constexpr effect_type fx_name(std::size_t n) const noexcept {
		return fx_cmd(n).fx;
	}
	constexpr std::uint8_t fx_param(std::size_t n) const noexcept {
		return fx_cmd(n).param;
	}

	constexpr void set_note(pitch n) noexcept {
		note_ = n;
	}
	constexpr void set_oct(unsigned char o) noexcept {
		oct_ = o;
	}
	constexpr void set_vol(unsigned char v) noexcept {
		vol_ = v;
	}
	constexpr void set_inst(unsigned char i) noexcept {
		inst_ = i;
	}
	constexpr void set_fx_cmd(std::size_t n, effect_command cmd) {
		fx_.at(n) = cmd;
	}
	constexpr void set_fx_name(std::size_t n, effect_type name) {
		fx_.at(n).fx = name;
	}
	constexpr void set_fx_param(std::size_t n, std::uint8_t param) {
		fx_.at(n).param = param;
	}

	constexpr int midi_note() const noexcept {
		return ft0cc::doc::midi_note(oct_, note_);
	}

	auto fx_cmds() noexcept {
		return values(fx_);
	}
	auto fx_cmds() const noexcept {
		return values(fx_);
	}

private:
	pitch note_ {pitch::none};
	unsigned char oct_ {0u};
	std::uint8_t vol_ {static_cast<std::uint8_t>(max_volumes)};
	std::uint8_t inst_ {static_cast<std::uint8_t>(max_instruments)};
	std::array<effect_command, max_effect_columns> fx_ { };
};

} // namespace ft0cc::doc
