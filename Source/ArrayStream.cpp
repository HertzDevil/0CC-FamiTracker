/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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

#define _SCL_SECURE_NO_WARNINGS
#include "ArrayStream.h"
#include <algorithm>

CConstArrayStream::CConstArrayStream(array_view<const std::byte> input) :
	input_(input), orig_input_(input)
{
}

std::size_t CConstArrayStream::ReadBytes(array_view<std::byte> buf) {
	std::size_t toread = buf.size();
	if (input_.size() >= toread) {
		std::copy_n(input_.begin(), toread, buf.begin());
		input_.remove_front(toread);
		return toread;
	}
	return 0u;
}

void CConstArrayStream::SeekReader(std::size_t pos) {
	if (pos > orig_input_.size())
		throw std::runtime_error {"Cannot seek beyond EOF"};
	input_ = orig_input_.subview(pos);
}

std::size_t CConstArrayStream::GetReaderPos() {
	return orig_input_.size() - input_.size();
}



CArrayStream::CArrayStream(array_view<std::byte> output) :
	output_(output), orig_output_(output)
{
}

std::size_t CArrayStream::WriteBytes(array_view<const std::byte> buf) {
	std::size_t towrite = buf.size();
	if (output_.size() >= towrite) {
		std::copy_n(buf.begin(), towrite, output_.begin());
		output_.remove_front(towrite);
		return towrite;
	}
	return 0u;
}

void CArrayStream::SeekWriter(std::size_t pos) {
	if (pos > orig_output_.size())
		throw std::runtime_error {"Cannot seek beyond EOF"};
	output_ = orig_output_.subview(pos);
}

std::size_t CArrayStream::GetWriterPos() {
	return orig_output_.size() - output_.size();
}
