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

#include "ft0cc/cpputil/iter.hpp"
#include "gtest/gtest.h"
#include <array>

namespace {

struct A {
	int x_[5] = { };
	auto begin() { return std::begin(x_); }
	auto end() { return std::end(x_); }
};

} // namespace

TEST(Iter, ValuesRange) {
	int a1[3] = { };
	auto i1 = values(a1);
	EXPECT_EQ(i1.begin(), a1);
	EXPECT_EQ(i1.end(), a1 + 3);

	std::array<int, 4> a2 = { };
	auto i2 = values(a2);
	EXPECT_EQ(i2.begin(), a2.begin());
	EXPECT_EQ(i2.end(), a2.end());

	auto a3 = A { };
	auto i3 = values(a3);
	EXPECT_EQ(i3.begin(), a3.x_);
	EXPECT_EQ(i3.end(), a3.x_ + 5);
}

TEST(Iter, IterRange) {
	int a1[3] = { };
	auto i1 = iter(a1 + 1, a1 + 2);
	EXPECT_EQ(i1.begin(), a1 + 1);
	EXPECT_EQ(i1.end(), a1 + 2);

	std::array<int, 4> a2 = { };
	auto i2 = iter(a2.begin() + 1, a2.begin() + 2);
	EXPECT_EQ(i2.begin(), a2.begin() + 1);
	EXPECT_EQ(i2.end(), a2.begin() + 2);
}

TEST(Iter, WithIndex) {
	int a[3] = {8, 7, 5};
	auto i = with_index(a);
	auto b = i.begin();
	auto e = i.end();

	ASSERT_NE(b, e);
	auto p0 = *b;
	ASSERT_EQ(p0.first, a[0]);
	ASSERT_EQ(p0.second, 0u);
	++b;
	ASSERT_NE(b, e);
	auto p1 = *b;
	ASSERT_EQ(p1.first, a[1]);
	ASSERT_EQ(p1.second, 1u);
	++b;
	ASSERT_NE(b, e);
	auto p2 = *b;
	ASSERT_EQ(p2.first, a[2]);
	ASSERT_EQ(p2.second, 2u);
	p2.first = 123;
	++b;
	EXPECT_FALSE(b != e);

	EXPECT_EQ(a[2], 123);
}
