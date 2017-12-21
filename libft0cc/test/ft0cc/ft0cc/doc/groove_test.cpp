#include "ft0cc/doc/groove.hpp"
#include "gtest/gtest.h"

using groove = ft0cc::doc::groove;

TEST(Groove, DefaultCtor) {
	auto grv = groove { };
	EXPECT_EQ(grv.size(), 0u);
	EXPECT_DOUBLE_EQ(grv.average(), groove::default_speed);
}

TEST(Groove, ListCtor) {
	auto grv = groove {4u, 7u, 11u};
	EXPECT_EQ(grv.size(), 3u);
	EXPECT_EQ(grv.entry(0), 4u);
	EXPECT_EQ(grv.entry(1), 7u);
	EXPECT_EQ(grv.entry(2), 11u);
}

TEST(Groove, RelOps) {
	auto grv1 = groove { };
	auto grv2 = groove {1u};
	auto grv3 = groove {1u, 1u};
	auto grv4 = groove {2u};

	ASSERT_LT(grv1.compare(grv2), 0);
	EXPECT_LT(grv1.compare(grv3), 0);
	EXPECT_LT(grv1.compare(grv4), 0);
	EXPECT_LT(grv2.compare(grv3), 0);
	EXPECT_LT(grv2.compare(grv4), 0);
	EXPECT_LT(grv3.compare(grv4), 0);

	ASSERT_EQ(grv1.compare(grv1), 0);
	ASSERT_EQ(grv2.compare(grv2), 0);
	EXPECT_EQ(grv3.compare(grv3), 0);
	EXPECT_EQ(grv4.compare(grv4), 0);

	EXPECT_EQ(grv1, grv1);
	EXPECT_NE(grv1, grv2);
	EXPECT_LT(grv1, grv2);
	EXPECT_LE(grv1, grv2);
	EXPECT_GT(grv2, grv1);
	EXPECT_GE(grv2, grv1);
}

TEST(Groove, Resizing) {
	auto grv = groove { };
	grv.resize(3);
	EXPECT_EQ(grv.size(), 3u);
	EXPECT_EQ(grv.entry(0), 0u);
	EXPECT_EQ(grv.entry(1), 0u);
	EXPECT_EQ(grv.entry(2), 0u);
	EXPECT_EQ(grv.compiled_size(), 5u);
}

TEST(Groove, SettingEntries) {
	auto grv = groove {5u, 8u};
	grv.set_entry(1, 3u);
	EXPECT_EQ(grv.entry(0), 5u);
	EXPECT_EQ(grv.entry(1), 3u);
	grv.resize(1);
	grv.resize(2);
	EXPECT_EQ(grv.entry(1), 0u);
}

TEST(Groove, RangeFor) {
	const auto grv1 = groove {4u, 7u, 11u};
	auto b = grv1.begin();
	auto e = grv1.end();

	ASSERT_NE(b, e);
	EXPECT_EQ(*b, 4u);
	++b;
	ASSERT_NE(b, e);
	EXPECT_EQ(*b, 7u);
	++b;
	ASSERT_NE(b, e);
	EXPECT_EQ(*b, 11u);
	++b;
	EXPECT_EQ(b, e);
}

TEST(Groove, AverageSpeed) {
	EXPECT_DOUBLE_EQ(groove { }.average(), groove::default_speed);
	EXPECT_DOUBLE_EQ(groove {5u}.average(), 5.);
	EXPECT_DOUBLE_EQ((groove {5u, 4u}.average()), 4.5);
}
