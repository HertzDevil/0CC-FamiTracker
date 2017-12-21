#include "ft0cc/doc/groove.hpp"
#include "gtest/gtest.h"

using groove = ft0cc::doc::groove;

TEST(Groove, DefaultCtor) {
	auto grv = groove { };
	EXPECT_EQ(grv.size(), 0u);
	EXPECT_DOUBLE_EQ(grv.average(), groove::default_speed);
}

TEST(Groove, DefaultCtorWithSpeed) {
	auto grv = groove {2};
	EXPECT_EQ(grv.size(), 1u);
	EXPECT_DOUBLE_EQ(grv.average(), 2.);
}
