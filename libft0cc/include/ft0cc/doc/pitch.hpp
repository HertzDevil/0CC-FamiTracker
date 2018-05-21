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

#include "ft0cc/cpputil/enum_traits.hpp"
#include <cstdint>

namespace ft0cc::doc {

enum class pitch : std::uint8_t {
	none,					// No note
	C, Cs, D, Ds, E, F, Fs, G, Gs, A, As, B,
	release,				// Release, begin note release sequence
	halt,					// Halt, stops note
	echo,					// Echo buffer access, octave determines position
	min = C, max = echo,
};

} // namespace ft0cc::doc

ENABLE_ENUM_CATEGORY(ft0cc::doc::pitch, enum_standard);

namespace ft0cc::doc {

inline constexpr auto note_range = static_cast<int>(
	value_cast(pitch::B) - value_cast(pitch::C) + 1);

constexpr bool is_note(pitch n) noexcept {
	return n >= pitch::C && n <= pitch::B;
}

constexpr int midi_note(int octave, pitch note) noexcept {
	if (is_note(note) && octave >= 0)
		return static_cast<int>(octave * note_range + value_cast(note) - 1);
	return -1;
}

constexpr pitch pitch_from_midi(int midi_note) noexcept {
	return midi_note >= 0 ?
		enum_cast<pitch>(midi_note % note_range + 1) : pitch::none;
}

constexpr int oct_from_midi(int midi_note) noexcept {
	return midi_note >= 0 ? midi_note / note_range : 0;
}

} // namespace ft0cc::doc
