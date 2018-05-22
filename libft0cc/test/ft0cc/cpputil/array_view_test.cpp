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


#include "ft0cc/cpputil/array_view.hpp"
#include "gtest/gtest.h"

#include <array>
#include <string_view>

// force instantiation
template struct array_view<int>;
template struct array_view<const int>;

TEST(ArrayView, Ctor) {
	// default ctor
	auto a1 = array_view<int> { };
	EXPECT_EQ(a1.size(), 0u);
	EXPECT_EQ(a1.length(), a1.size());
	EXPECT_TRUE(a1.empty());
	EXPECT_LT(a1.size(), a1.max_size());

	// c array ctor
	int a2_i[] = {1, 2, 3};
	auto a2 = array_view {a2_i};
	::testing::StaticAssertTypeEq<decltype(a2), array_view<int>>();
	EXPECT_EQ(a2.data(), &a2_i[0]);
	EXPECT_EQ(a2.size(), std::size(a2_i));
	EXPECT_EQ(a2.length(), a2.size());
	EXPECT_FALSE(a2.empty());

	const int a3_i[] = {4, 5, 6, 7};
	auto a3 = array_view {a3_i};
	::testing::StaticAssertTypeEq<decltype(a3), array_view<const int>>();
	EXPECT_EQ(a3.data(), &a3_i[0]);
	EXPECT_EQ(a3.size(), std::size(a3_i));

	// pointer + size
	auto a4 = array_view<const int> {&a3_i[1], 2u};
	EXPECT_EQ(a4.data(), &a3_i[1]);
	EXPECT_EQ(a4.size(), 2u);

	// view-like objects
	std::array<int, 6> a5_i = { };
	auto a5 = array_view {a5_i};
	::testing::StaticAssertTypeEq<decltype(a5), array_view<int>>();
	EXPECT_EQ(a5.data(), a5_i.data());
	EXPECT_EQ(a5.size(), a5_i.size());

	const std::array<int, 7> a6_i = { };
	auto a6 = array_view {a6_i};
	::testing::StaticAssertTypeEq<decltype(a6), array_view<const int>>();
	EXPECT_EQ(a6.data(), a6_i.data());
	EXPECT_EQ(a6.size(), a6_i.size());

	// copy ctor
	auto a1c = a1;
	EXPECT_EQ(a1c.size(), a1.size());

	auto a2c = a2;
	EXPECT_EQ(a2c.data(), a2.data());
	EXPECT_EQ(a2c.size(), a2.size());

	auto a3c = a3;
	EXPECT_EQ(a3c.data(), a3.data());
	EXPECT_EQ(a3c.size(), a3.size());

	auto a5c = std::move(a5);
	EXPECT_EQ(a5c.data(), a5.data());
	EXPECT_EQ(a5c.size(), a5.size());

	auto a6c = std::move(a6);
	EXPECT_EQ(a6c.data(), a6.data());
	EXPECT_EQ(a6c.size(), a6.size());

	// non-const -> const
	auto a2_const = array_view<const int> {a2};
	EXPECT_EQ(a2_const.data(), a2.data());
	EXPECT_EQ(a2_const.size(), a2.size());

	// this should fail
	// auto a3_const = array_view<int> {a3};
	// EXPECT_EQ(a3_const.data(), a3.data());
	// EXPECT_EQ(a3_const.size(), a3.size());
}

TEST(ArrayView, Swap) {
	using std::swap;

	int x[3] = {};

	auto a1 = array_view<int> { };
	auto a2 = array_view<int> {x};

	a1.swap(a2);
	EXPECT_EQ(a1.data(), &x[0]);
	EXPECT_EQ(a1.size(), std::size(x));
	EXPECT_TRUE(a2.empty());

	swap(a1, a2);
	EXPECT_TRUE(a1.empty());
	EXPECT_EQ(a2.data(), &x[0]);
	EXPECT_EQ(a2.size(), std::size(x));

	a1.swap(a1);
	EXPECT_TRUE(a1.empty());
	a2.swap(a2);
	EXPECT_EQ(a2.data(), &x[0]);
	EXPECT_EQ(a2.size(), std::size(x));
}

TEST(ArrayView, Addressing) {
	int x[3] = {111, 222, 333};
	auto a1 = array_view {x};
	auto a2 = array_view {std::as_const(x)};

	EXPECT_EQ(a1[0], 111);
	EXPECT_EQ(a1[1], 222);
	EXPECT_EQ(a1[2], 333);
	EXPECT_EQ(a1.at(0), 111);
	EXPECT_EQ(a1.at(1), 222);
	EXPECT_EQ(a1.at(2), 333);
	EXPECT_EQ(a2[0], 111);
	EXPECT_EQ(a2[1], 222);
	EXPECT_EQ(a2[2], 333);
	EXPECT_EQ(a2.at(0), 111);
	EXPECT_EQ(a2.at(1), 222);
	EXPECT_EQ(a2.at(2), 333);

	a1[0] = 444;
	EXPECT_EQ(x[0], 444);
	EXPECT_EQ(a1[0], 444);
	EXPECT_EQ(a2[0], 444);

	a1.at(1) = 555;
	EXPECT_EQ(x[1], 555);
	EXPECT_EQ(a1[1], 555);
	EXPECT_EQ(a2[1], 555);

	EXPECT_EQ(&a1.front(), &x[0]);
	EXPECT_EQ(&a2.front(), &x[0]);
	EXPECT_EQ(&a1.back(), &x[2]);
	EXPECT_EQ(&a2.back(), &x[2]);

	EXPECT_NO_THROW({ static_cast<void>(a1[3]); });
	ASSERT_THROW({ static_cast<void>(a1.at(3)); }, std::out_of_range);
}

TEST(ArrayView, Sizing) {
	const int x[10] = { };
	auto a1 = array_view {x};
	auto a2 = array_view {x};

	a1.pop_front();
	EXPECT_EQ(a1.data(), &x[1]);
	EXPECT_EQ(a1.size(), 9u);
	EXPECT_FALSE(a1.empty());
	a1.pop_back();
	EXPECT_EQ(a1.data(), &x[1]);
	EXPECT_EQ(a1.size(), 8u);
	a1.remove_front(3);
	EXPECT_EQ(a1.data(), &x[4]);
	EXPECT_EQ(a1.size(), 5u);
	EXPECT_FALSE(a1.empty());
	a1.remove_front(100);
	EXPECT_TRUE(a1.empty());
	a1.pop_front();
	EXPECT_TRUE(a1.empty());

	EXPECT_EQ(&a2.back(), &x[9]);
	a2.pop_back();
	EXPECT_EQ(a2.size(), 9u);
	EXPECT_EQ(&a2.back(), &x[8]);
	a2.remove_back(3);
	EXPECT_EQ(a2.size(), 6u);
	EXPECT_EQ(&a2.back(), &x[5]);
	a2.remove_back(100);
	EXPECT_TRUE(a2.empty());
	a2.pop_back();
	EXPECT_TRUE(a2.empty());

	auto a3 = array_view {x};
	a3.clear();
	EXPECT_TRUE(a3.empty());
}

TEST(ArrayView, Subview) {
	const int x[10] = { };
	auto a1 = array_view {x};

	auto a2 = a1.subview(0);
	EXPECT_EQ(a2.data(), &x[0]);
	EXPECT_EQ(a2.size(), 10u);

	auto a3 = a1.subview(1, 2u);
	EXPECT_EQ(a3.data(), &x[1]);
	EXPECT_EQ(a3.size(), 2u);

	auto a4 = a1.subview(10, 1u);
	EXPECT_TRUE(a4.empty());

	ASSERT_THROW({ static_cast<void>(a1.subview(11, 0u)); }, std::out_of_range);
}

TEST(ArrayView, Iteration) {
	int x[4] = {5, 8, 3, 6};
	auto a = array_view {x};

	// const_iterator
	auto b1 = a.cbegin();
	auto e1 = a.cend();
	ASSERT_NE(b1, e1);
	EXPECT_EQ(*b1, 5);
	++b1;
	ASSERT_NE(b1, e1);
	EXPECT_EQ(*b1, 8);
	++b1;
	ASSERT_NE(b1, e1);
	EXPECT_EQ(*b1, 3);
	++b1;
	ASSERT_NE(b1, e1);
	EXPECT_EQ(*b1, 6);
	++b1;
	EXPECT_EQ(b1, e1);

	// iterator
	auto b2 = a.begin();
	auto e2 = a.end();
	ASSERT_NE(b2, e2);
	EXPECT_EQ(*b2, 5);
	++b2;
	ASSERT_NE(b2, e2);
	EXPECT_EQ(*b2, 8);
	++b2;
	ASSERT_NE(b2, e2);
	EXPECT_EQ(*b2, 3);
	++b2;
	ASSERT_NE(b2, e2);
	*b2 = 10;
	EXPECT_EQ(x[3], 10);
	EXPECT_EQ(*b2, 10);
	++b2;
	EXPECT_EQ(b2, e2);

	// reverse_const_iterator
	auto b3 = a.crbegin();
	auto e3 = a.crend();
	ASSERT_NE(b3, e3);
	EXPECT_EQ(*b3, 10);
	++b3;
	ASSERT_NE(b3, e3);
	EXPECT_EQ(*b3, 3);
	++b3;
	ASSERT_NE(b3, e3);
	EXPECT_EQ(*b3, 8);
	++b3;
	ASSERT_NE(b3, e3);
	EXPECT_EQ(*b3, 5);
	++b3;
	EXPECT_EQ(b3, e3);

	// reverse_iterator
	auto b4 = a.rbegin();
	auto e4 = a.rend();
	ASSERT_NE(b4, e4);
	*b4 = 6;
	EXPECT_EQ(x[3], 6);
	EXPECT_EQ(*b4, 6);
	++b4;
	ASSERT_NE(b4, e4);
	EXPECT_EQ(*b4, 3);
	++b4;
	ASSERT_NE(b4, e4);
	EXPECT_EQ(*b4, 8);
	++b4;
	ASSERT_NE(b4, e4);
	EXPECT_EQ(*b4, 5);
	++b4;
	EXPECT_EQ(b4, e4);
}

TEST(ArrayView, CopyOut) {
	int x1[2] = {-1, -2};
	auto a1 = array_view {x1};

	int y[2] = { };
	EXPECT_EQ(a1.copy(y, 2u), 2u);
	EXPECT_EQ(y[0], -1);
	EXPECT_EQ(y[1], -2);
	EXPECT_EQ(a1.copy(y, 1u, 1u), 1u);
	EXPECT_EQ(y[0], -2);
	EXPECT_EQ(y[1], -2);
	y[1] = -3;
	EXPECT_EQ(a1.copy(y, 2u, 1u), 1u);
	EXPECT_EQ(y[1], -3);
	EXPECT_EQ(a1.copy(y, 2u, 2u), 0u);
	EXPECT_EQ(a1.copy(y, 0u), 0u);
	ASSERT_THROW({ static_cast<void>(a1.copy(y, 0u, 4u)); }, std::out_of_range);

	const int x2[3] = {1, 2, 3};
	auto a2 = array_view {x2};
	EXPECT_EQ(a2.copy(a1), 2u);
	EXPECT_EQ(x1[0], 1);
	EXPECT_EQ(x1[1], 2);
	EXPECT_EQ(a2.copy(a1, 1u), 2u);
	EXPECT_EQ(x1[0], 2);
	EXPECT_EQ(x1[1], 3);
	EXPECT_EQ(a2.copy(a1, 2u), 1u);
	EXPECT_EQ(x1[0], 3);
	EXPECT_EQ(a2.copy(a1, 3u), 0u);
	ASSERT_THROW({ static_cast<void>(a2.copy(a1, 100u)); }, std::out_of_range);
}

TEST(ArrayView, RelOps) {
	const int x1[] = {1};
	int x2[] = {1, 0};
	int x3[] = {2};
	auto a0 = array_view<int> { };
	auto a1 = array_view<const int> {x1};
	auto a2 = array_view<int> {x2};
	auto a3 = array_view<int> {x3};

	// 3-way comparison
	EXPECT_EQ(a0.compare(a0), 0);
	EXPECT_LT(a0.compare(a1), 0);
	EXPECT_LT(a0.compare(a2), 0);
	EXPECT_LT(a0.compare(a3), 0);
	EXPECT_GT(a1.compare(a0), 0);
	EXPECT_EQ(a1.compare(a1), 0);
	EXPECT_LT(a1.compare(a2), 0);
	EXPECT_LT(a1.compare(a3), 0);
	EXPECT_GT(a2.compare(a0), 0);
	EXPECT_GT(a2.compare(a1), 0);
	EXPECT_EQ(a2.compare(a2), 0);
	EXPECT_LT(a2.compare(a3), 0);
	EXPECT_GT(a3.compare(a0), 0);
	EXPECT_GT(a3.compare(a1), 0);
	EXPECT_GT(a3.compare(a2), 0);
	EXPECT_EQ(a3.compare(a3), 0);

	// relational operators
	EXPECT_FALSE(a0 == a1);
	EXPECT_TRUE (a0 != a1);
	EXPECT_TRUE (a0 <  a1);
	EXPECT_TRUE (a0 <= a1);
	EXPECT_FALSE(a0 >  a1);
	EXPECT_FALSE(a0 >= a1);

	EXPECT_TRUE (a1 == a1);
	EXPECT_FALSE(a1 != a1);
	EXPECT_FALSE(a1 <  a1);
	EXPECT_TRUE (a1 <= a1);
	EXPECT_FALSE(a1 >  a1);
	EXPECT_TRUE (a1 >= a1);

	EXPECT_FALSE(a2 == a1);
	EXPECT_TRUE (a2 != a1);
	EXPECT_FALSE(a2 <  a1);
	EXPECT_FALSE(a2 <= a1);
	EXPECT_TRUE (a2 >  a1);
	EXPECT_TRUE (a2 >= a1);

	EXPECT_FALSE(a1 == a2);
	EXPECT_TRUE (a1 != a2);
	EXPECT_TRUE (a1 <  a2);
	EXPECT_TRUE (a1 <= a2);
	EXPECT_FALSE(a1 >  a2);
	EXPECT_FALSE(a1 >= a2);

	EXPECT_TRUE (a2 == a2);
	EXPECT_FALSE(a2 != a2);
	EXPECT_FALSE(a2 <  a2);
	EXPECT_TRUE (a2 <= a2);
	EXPECT_FALSE(a2 >  a2);
	EXPECT_TRUE (a2 >= a2);

	// empty views are all equal
	auto a0_2 = array_view<const int> {x1, 0u};
	auto a0_3 = array_view<const int> {x2, 0u};
	EXPECT_EQ(a0, a0_2);
	EXPECT_EQ(a0, a0_3);
	EXPECT_EQ(a0_2, a0_3);

	// array view <=> c array
	EXPECT_EQ(a1, x1);
	EXPECT_NE(a2, x1);
	EXPECT_NE(a1, x2);
	EXPECT_EQ(a2, x2);
	EXPECT_EQ(x1, a1);
	EXPECT_NE(x2, a1);
	EXPECT_NE(x1, a2);
	EXPECT_EQ(x2, a2);

	// array view <=> view-like object
	const std::array<int, 1> x1_2 = {1};
	std::array<int, 2> x2_2 = {1, 0};

	EXPECT_EQ(a1, x1_2);
	EXPECT_NE(a2, x1_2);
	EXPECT_NE(a1, x2_2);
	EXPECT_EQ(a2, x2_2);
	EXPECT_EQ(x1_2, a1);
	EXPECT_NE(x2_2, a1);
	EXPECT_NE(x1_2, a2);
	EXPECT_EQ(x2_2, a2);

	// equality-comparable types
	struct Eq {
		constexpr bool operator==(Eq) const noexcept { return true; }
	} e1, e2;
	Eq e3[2] = { };
	auto ae1 = array_view {&e1, 1u};
	auto ae2 = array_view {&e2, 1u};
	auto ae3 = array_view {e3};
	EXPECT_EQ(ae1, ae1);
	EXPECT_EQ(ae1, ae2);
	EXPECT_NE(ae1, ae3);
	EXPECT_EQ(ae2, ae1);
	EXPECT_EQ(ae2, ae2);
	EXPECT_NE(ae2, ae3);
	EXPECT_NE(ae3, ae1);
	EXPECT_NE(ae3, ae2);
	EXPECT_EQ(ae3, ae3);
	EXPECT_EQ(array_view<Eq> { }, array_view<Eq> { });

	// non-comparable types
	struct Non { } n1, n2;
	Non n3[2] = { };
	auto an1 = array_view {&n1, 1u};
	auto an2 = array_view {&n2, 1u};
	auto an3 = array_view {n3};
	EXPECT_EQ(an1, an1);
	EXPECT_NE(an1, an2);
	EXPECT_NE(an1, an3);
	EXPECT_NE(an2, an1);
	EXPECT_EQ(an2, an2);
	EXPECT_NE(an2, an3);
	EXPECT_NE(an3, an1);
	EXPECT_NE(an3, an2);
	EXPECT_EQ(an3, an3);
	EXPECT_EQ(array_view<Non> { }, array_view<Non> { });
}

TEST(ArrayView, ViewOf) {
	int x1[] = {1, 2};
	const int x2[] = {1, 2};
	std::array<int, 2> x3 = {1, 2};
	const std::array<int, 2> x4 = {1, 2};
	int y1 = 5;
	const int y2 = 6;

	auto ax1 = view_of(x1);
	::testing::StaticAssertTypeEq<decltype(ax1), array_view<int>>();
	EXPECT_EQ(ax1, array_view {x1});
	auto ax2 = view_of(x2);
	::testing::StaticAssertTypeEq<decltype(ax2), array_view<const int>>();
	EXPECT_EQ(ax2, array_view {x2});
	auto ax3 = view_of(x3);
	::testing::StaticAssertTypeEq<decltype(ax3), array_view<int>>();
	EXPECT_EQ(ax3, array_view {x3});
	auto ax4 = view_of(x4);
	::testing::StaticAssertTypeEq<decltype(ax4), array_view<const int>>();
	EXPECT_EQ(ax4, array_view {x4});
	auto ay1 = view_of(y1);
	::testing::StaticAssertTypeEq<decltype(ay1), array_view<int>>();
	EXPECT_EQ(ay1, (array_view {&y1, 1u}));
	auto ay2 = view_of(y2);
	::testing::StaticAssertTypeEq<decltype(ay2), array_view<const int>>();
	EXPECT_EQ(ay2, (array_view {&y2, 1u}));

	std::string_view sv = "abc";
	auto asv = view_of(sv);
	::testing::StaticAssertTypeEq<decltype(asv), array_view<const char>>();
	EXPECT_EQ(asv.data(), sv.data());
	EXPECT_EQ(asv.size(), sv.size());

	struct TAG { } tag;
	auto tagview = view_of(tag);
	auto tagview2 = array_view {&tag, 1};
	EXPECT_EQ(tagview, tagview2);
}

TEST(ArrayView, AsBytes) {
	unsigned x[2] = {5u, 6u};
	auto b = as_bytes(array_view {x});
	::testing::StaticAssertTypeEq<decltype(b), array_view<const std::byte>>();
	EXPECT_EQ(b.data(), reinterpret_cast<const std::byte *>(x));
	EXPECT_EQ(b.size(), 2 * sizeof(unsigned));
	auto cb = as_bytes(array_view {std::as_const(x)});
	::testing::StaticAssertTypeEq<decltype(cb), array_view<const std::byte>>();
	EXPECT_EQ(b, cb);
	auto wb = as_writeable_bytes(array_view {x});
	::testing::StaticAssertTypeEq<decltype(wb), array_view<std::byte>>();
	EXPECT_EQ(b, wb);

#if __cplusplus > 201703L
	static_assert(std::endian::native == std::endian::little);
#elif !defined(_WIN32)
	static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__);
#endif
	wb.at(sizeof(unsigned) - 1) = std::byte {1};
	wb.at(sizeof(unsigned)) = std::byte {2};
	EXPECT_EQ(x[0], 0x01000005u);
	EXPECT_EQ(x[1], 0x00000002u);

	auto bv = byte_view(std::as_const(x));
	::testing::StaticAssertTypeEq<decltype(bv), array_view<const std::byte>>();
	EXPECT_EQ(b, bv);
	auto wbv = byte_view(x);
	::testing::StaticAssertTypeEq<decltype(wbv), array_view<std::byte>>();
	EXPECT_EQ(wb, wbv);
}
