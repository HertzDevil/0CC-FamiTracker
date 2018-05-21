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

#include "ft0cc/doc/pattern_note.hpp"
#include "gtest/gtest.h"

using pattern_note = ft0cc::doc::pattern_note;

TEST(PatternNote, Ctor) {
	auto note = pattern_note { };
	EXPECT_EQ(note.note(), ft0cc::doc::pitch::none);
	EXPECT_EQ(note.oct(), 0);
	EXPECT_EQ(note.vol(), ft0cc::doc::max_volumes);
	EXPECT_EQ(note.inst(), ft0cc::doc::max_instruments);
	for (const auto &fx : note.fx_cmds())
		EXPECT_EQ(fx, ft0cc::doc::effect_command { });
}

TEST(PatternNote, Accessors) {
	auto note = pattern_note { };
	auto fx = ft0cc::doc::effect_command {ft0cc::doc::effect_type::JUMP, 0xFFu};

	note.set_note(ft0cc::doc::pitch::halt);
	EXPECT_EQ(note.note(), ft0cc::doc::pitch::halt);

	note.set_oct(3);
	EXPECT_EQ(note.oct(), 3);

	note.set_vol(15);
	EXPECT_EQ(note.vol(), 15);

	note.set_inst(1);
	EXPECT_EQ(note.inst(), 1);

	constexpr auto FXS = ft0cc::doc::max_effect_columns;
	static_assert(FXS >= 4);
	note.set_fx_cmd(2, {ft0cc::doc::effect_type::SPEED, 0x03u});
	note.set_fx_name(3, ft0cc::doc::effect_type::JUMP);
	note.set_fx_param(3, 0xFFu);
	EXPECT_EQ(note.fx_name(2), ft0cc::doc::effect_type::SPEED);
	EXPECT_EQ(note.fx_param(2), 3);
	EXPECT_EQ(note.fx_cmd(3), fx);

	ASSERT_ANY_THROW({ note.set_fx_name(FXS, ft0cc::doc::effect_type::SPEED); });
	ASSERT_ANY_THROW({ note.set_fx_param(FXS, 0x01u); });
	ASSERT_ANY_THROW({ note.set_fx_cmd(FXS, fx); });

	EXPECT_EQ(note.fx_name(4), ft0cc::doc::effect_type::none);
	EXPECT_EQ(note.fx_param(4), 0x00u);
	EXPECT_EQ(note.fx_cmd(4), ft0cc::doc::effect_command { });
}

TEST(PatternNote, FxCmds) {
	auto note = pattern_note { };
	for (std::size_t i = 0; i < ft0cc::doc::max_effect_columns; ++i)
		note.set_fx_param(i, static_cast<ft0cc::doc::effect_param_t>(i + 1));

	auto rng = note.fx_cmds();
	auto b = rng.begin();
	auto e = rng.end();

	ASSERT_NE(b, e);
	EXPECT_EQ(b->param, 1);
	++b;
	ASSERT_NE(b, e);
	EXPECT_EQ(b->param, 2);
	++b;
	ASSERT_NE(b, e);
	EXPECT_EQ(b->param, 3);
	++b;
	ASSERT_NE(b, e);
	EXPECT_EQ(b->param, 4);
	++b;
	EXPECT_EQ(b, e);

	for (auto &cmd : note.fx_cmds())
		cmd.param = 0x00u;
	for (const auto &cmd : note.fx_cmds())
		EXPECT_EQ(cmd.param, 0x00u);
}
