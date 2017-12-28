/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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
#include <algorithm>

using namespace ft0cc::doc;

inst_sequence::entry_type inst_sequence::entry(std::size_t index) const {
	return entries_[index];
}

void inst_sequence::set_entry(std::size_t index, entry_type value) {
	entries_[index] = value;
}

std::size_t inst_sequence::size() const {
	return len_;
}

std::size_t inst_sequence::compiled_size() const {
	return size() ? size() + 3 : 0;
}

void inst_sequence::resize(std::size_t sz) {
	if (sz <= max_items) {
		len_ = sz;
		if (loop_point() >= size())
			set_loop_point(none);
		if (release_point() >= size())
			set_release_point(none);
	}
}

std::size_t inst_sequence::loop_point() const {
	return loop_;
}

void inst_sequence::set_loop_point(std::size_t index) {
	loop_ = index >= len_ ? none : index;
}

std::size_t inst_sequence::release_point() const {
	return release_;
}

void inst_sequence::set_release_point(std::size_t index) {
	release_ = index >= len_ ? none : index;
}

inst_sequence::setting inst_sequence::sequence_setting() const {
	return setting_;
}

void inst_sequence::set_sequence_setting(setting s) {
	setting_ = s;
}

bool inst_sequence::operator==(const inst_sequence &other) const {
	return size() == other.size() &&
		loop_point() == other.loop_point() &&
		release_point() == other.release_point() &&
		sequence_setting() == other.sequence_setting() &&
		std::equal(begin(), begin() + len_, other.begin(), other.begin() + len_);
}
