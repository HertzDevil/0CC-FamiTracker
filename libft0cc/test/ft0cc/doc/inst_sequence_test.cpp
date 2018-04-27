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
