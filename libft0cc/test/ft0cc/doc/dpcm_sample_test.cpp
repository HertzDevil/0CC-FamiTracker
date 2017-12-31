/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.  To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include "ft0cc/doc/dpcm_sample.hpp"
#include "gtest/gtest.h"

using dpcm_sample = ft0cc::doc::dpcm_sample;

TEST(DpcmSample, Ctor) {
	auto samp1 = dpcm_sample { };
	EXPECT_EQ(samp1.size(), 0u);

	std::vector<uint8_t> samples {5u};
	auto samp2 = dpcm_sample {samples, "name"};
	EXPECT_EQ(samp2.size(), 0x1u);
	EXPECT_EQ(samp2.sample_at(0u), 5u);
	EXPECT_EQ(samp2.name(), "name");
}

TEST(DpcmSample, Accessors) {
	auto samp = dpcm_sample { };
	EXPECT_EQ(samp.sample_at(0u), 0xAAu);

	EXPECT_EQ(samp.name(), "");
	samp.rename("abc");
	EXPECT_EQ(samp.name(), "abc");

	samp.resize(0x11u);
	ASSERT_EQ(samp.size(), 0x11u);
	EXPECT_EQ(samp.sample_at(0u), 0xAAu);

	samp.set_sample_at(0x3, 0x00u);
	EXPECT_EQ(samp.sample_at(0u), 0xAAu);
	EXPECT_EQ(samp.sample_at(0x3u), 0x00u);

	samp.resize(0x1u);
	EXPECT_EQ(samp.sample_at(0u), 0xAAu);
	EXPECT_EQ(samp.sample_at(0x3u), 0xAAu);
}

TEST(DpcmSample, RelOps) {
	auto samp1 = dpcm_sample { };
	auto samp2 = samp1;
	EXPECT_EQ(samp1, samp2);

	samp1.resize(1u);
	EXPECT_NE(samp1, samp2);
	samp2.resize(1u);
	EXPECT_EQ(samp1, samp2);

	samp1.set_sample_at(0u, 0x00u);
	EXPECT_NE(samp1, samp2);
	samp2.set_sample_at(0u, 0x00u);
	EXPECT_EQ(samp1, samp2);

	samp1.rename("g");
	EXPECT_NE(samp1, samp2);
	samp2.rename("g");
	EXPECT_EQ(samp1, samp2);
}

TEST(DpcmSample, Cut) {
	std::vector<uint8_t> samples {7u, 6u, 5u, 4u};
	auto samp = dpcm_sample {samples, ""};
	samp.cut_samples(1, 1);
}
