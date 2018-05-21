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

#include "ft0cc/doc/effect_command.hpp"
#include "gtest/gtest.h"

using effect_command = ft0cc::doc::effect_command;
using effect_type = ft0cc::doc::effect_type;

TEST(EffectCommand, RelOps) {
	auto fx1 = effect_command { };
	auto fx2 = effect_command {effect_type::none, 2};
	auto fx3 = effect_command {effect_type::SPEED, 1};
	auto fx4 = effect_command {effect_type::SPEED, 2};

	EXPECT_EQ(fx1.compare(fx1), 0);
	EXPECT_EQ(fx1.compare(fx2), 0);
	EXPECT_LT(fx1.compare(fx3), 0);
	EXPECT_LT(fx1.compare(fx4), 0);
	EXPECT_EQ(fx2.compare(fx1), 0);
	EXPECT_EQ(fx2.compare(fx2), 0);
	EXPECT_LT(fx2.compare(fx3), 0);
	EXPECT_LT(fx2.compare(fx4), 0);
	EXPECT_GT(fx3.compare(fx1), 0);
	EXPECT_GT(fx3.compare(fx2), 0);
	EXPECT_EQ(fx3.compare(fx3), 0);
	EXPECT_LT(fx3.compare(fx4), 0);
	EXPECT_GT(fx4.compare(fx1), 0);
	EXPECT_GT(fx4.compare(fx2), 0);
	EXPECT_GT(fx4.compare(fx3), 0);
	EXPECT_EQ(fx4.compare(fx4), 0);
}
