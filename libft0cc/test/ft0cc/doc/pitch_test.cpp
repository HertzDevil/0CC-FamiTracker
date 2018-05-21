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

#include "ft0cc/doc/pitch.hpp"
#include "gtest/gtest.h"

using pitch = ft0cc::doc::pitch;

TEST(Pitch, Classify) {
	EXPECT_TRUE(is_note(pitch::C));
	EXPECT_TRUE(is_note(pitch::Cs));
	EXPECT_TRUE(is_note(pitch::As));
	EXPECT_TRUE(is_note(pitch::B));

	EXPECT_FALSE(is_note(pitch::none));
	EXPECT_FALSE(is_note(pitch::release));
	EXPECT_FALSE(is_note(pitch::halt));
	EXPECT_FALSE(is_note(pitch::echo));
	static_assert(!enum_valid<pitch>(0x80));
	EXPECT_FALSE(is_note(static_cast<pitch>(0x80)));
}

TEST(Pitch, MidiNote) {
	EXPECT_EQ(ft0cc::doc::midi_note(0, pitch::C), 0);
	EXPECT_EQ(ft0cc::doc::midi_note(0, pitch::B), 11);
	EXPECT_EQ(ft0cc::doc::midi_note(10, pitch::G), 127);
	EXPECT_EQ(ft0cc::doc::midi_note(-1, pitch::C), -1);
	EXPECT_EQ(ft0cc::doc::midi_note(-1, pitch::B), -1);

	EXPECT_EQ(ft0cc::doc::midi_note(1, pitch::none), -1);
	EXPECT_EQ(ft0cc::doc::midi_note(2, pitch::release), -1);
	EXPECT_EQ(ft0cc::doc::midi_note(3, pitch::halt), -1);
	EXPECT_EQ(ft0cc::doc::midi_note(4, pitch::echo), -1);

	EXPECT_EQ(ft0cc::doc::pitch_from_midi(0), pitch::C);
	EXPECT_EQ(ft0cc::doc::pitch_from_midi(1), pitch::Cs);
	EXPECT_EQ(ft0cc::doc::pitch_from_midi(11), pitch::B);
	EXPECT_EQ(ft0cc::doc::pitch_from_midi(12), pitch::C);
	EXPECT_EQ(ft0cc::doc::pitch_from_midi(-1), pitch::none);
	EXPECT_EQ(ft0cc::doc::pitch_from_midi(-2), pitch::none);

	EXPECT_EQ(ft0cc::doc::oct_from_midi(0), 0);
	EXPECT_EQ(ft0cc::doc::oct_from_midi(1), 0);
	EXPECT_EQ(ft0cc::doc::oct_from_midi(11), 0);
	EXPECT_EQ(ft0cc::doc::oct_from_midi(12), 1);
	EXPECT_EQ(ft0cc::doc::oct_from_midi(127), 10);
	EXPECT_EQ(ft0cc::doc::oct_from_midi(-1), 0);
	EXPECT_EQ(ft0cc::doc::oct_from_midi(-2), 0);
}
