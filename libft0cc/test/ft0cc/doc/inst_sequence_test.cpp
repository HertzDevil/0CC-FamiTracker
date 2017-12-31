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

#include "ft0cc/doc/inst_sequence.hpp"
#include "gtest/gtest.h"

using inst_sequence = ft0cc::doc::inst_sequence;

TEST(InstSequence, DefaultCtor) {
	auto seq = inst_sequence { };
	EXPECT_EQ(seq.size(), 0u);
	EXPECT_EQ(seq.compiled_size(), 0u);
	EXPECT_EQ(seq.loop_point(), inst_sequence::none);
	EXPECT_EQ(seq.release_point(), inst_sequence::none);
	EXPECT_EQ(seq.sequence_setting(), inst_sequence::setting::def);
}

TEST(InstSequence, Entries) {
	auto seq = inst_sequence { };
	seq.resize(2u);
	seq.set_entry(0, 5);
	seq.set_entry(1, 6);
	EXPECT_EQ(seq.entry(0), 5);
	EXPECT_EQ(seq.entry(1), 6);

	seq.resize(1u);
	EXPECT_EQ(seq.entry(1), 6);
}

TEST(InstSequence, Resizing) {
	auto seq = inst_sequence { };
	seq.resize(5u);
	EXPECT_EQ(seq.size(), 5u);
	EXPECT_EQ(seq.compiled_size(), 8u);
	EXPECT_EQ(seq.loop_point(), inst_sequence::none);
	EXPECT_EQ(seq.release_point(), inst_sequence::none);

	seq.set_loop_point(3);
	seq.set_release_point(2);
	seq.resize(4u);
	EXPECT_EQ(seq.loop_point(), 3u);
	EXPECT_EQ(seq.release_point(), 2u);

	seq.resize(1u);
	EXPECT_EQ(seq.loop_point(), inst_sequence::none);
	EXPECT_EQ(seq.release_point(), inst_sequence::none);
}

TEST(InstSequence, SeqSetting) {
	auto seq = inst_sequence { };
	seq.set_sequence_setting(inst_sequence::setting::vol_64_steps);
	EXPECT_EQ(seq.sequence_setting(), inst_sequence::setting::vol_64_steps);
}

TEST(InstSequence, RangeFor) {
	auto seq = inst_sequence { };
	seq.resize(3u);
	seq.set_entry(0, 8);
	seq.set_entry(1, 11);
	seq.set_entry(2, 13);
	auto b = seq.begin();
	auto e = seq.end();

	ASSERT_NE(b, e);
	EXPECT_EQ(*b, 8);
	++b;
	ASSERT_NE(b, e);
	EXPECT_EQ(*b, 11);
	++b;
	ASSERT_NE(b, e);
	EXPECT_EQ(*b, 13);
	++b;
	EXPECT_EQ(b, e);
}

TEST(InstSequence, RelOps) {
	auto seq1 = inst_sequence { };
	auto seq2 = inst_sequence { };
	EXPECT_EQ(seq1, seq2);

	seq1.resize(1);
	EXPECT_NE(seq1, seq2);
	seq2.resize(1);
	EXPECT_EQ(seq1, seq2);

	seq1.set_entry(0, 4);
	EXPECT_NE(seq1, seq2);
	seq2.set_entry(0, 3);
	EXPECT_NE(seq1, seq2);
	seq2.set_entry(0, 4);
	EXPECT_EQ(seq1, seq2);

	seq1.set_loop_point(0);
	EXPECT_NE(seq1, seq2);
	seq2.set_loop_point(0);
	EXPECT_EQ(seq1, seq2);

	seq1.set_release_point(0);
	EXPECT_NE(seq1, seq2);
	seq2.set_release_point(0);
	EXPECT_EQ(seq1, seq2);

	seq1.set_sequence_setting(inst_sequence::setting::vol_64_steps);
	EXPECT_NE(seq1, seq2);
	seq2.set_sequence_setting(inst_sequence::setting::vol_64_steps);
	EXPECT_EQ(seq1, seq2);
}
