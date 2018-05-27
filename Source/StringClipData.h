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


#pragma once

#include "BinarySerializable.h"
#include <cstring>
#include <string>
#include <string_view>

template <typename CharT>
class CStringClipData : public CBinarySerializableInterface {
public:
	CStringClipData() = default;
	explicit CStringClipData(std::basic_string<CharT> str) : str_(std::move(str)) { }
	explicit CStringClipData(std::basic_string_view<CharT> str) : str_ {std::move(str)} { }

	const std::basic_string<CharT> &GetStringData() const & {
		return str_;
	}
	std::basic_string<CharT> GetStringData() && {
		return std::move(str_);
	}

private:
	virtual bool ContainsData() const {
		return !str_.empty();
	}

	virtual std::size_t GetAllocSize() const {
		return (str_.size() + 1) * sizeof(CharT);
	}

	virtual bool ToBytes(array_view<std::byte> Buf) const {
		if (Buf.size() >= GetAllocSize()) {
			std::memcpy(Buf.data(), str_.c_str(), GetAllocSize());
			return true;
		}
		return false;
	}

	virtual bool FromBytes(array_view<const std::byte> Buf) {
		auto sv = array_view<const CharT> {reinterpret_cast<const CharT *>(Buf.data()), Buf.size() / sizeof(CharT)};
		str_.clear();
		str_.reserve(sv.size());
		for (CharT c : sv) {
			if (!c)
				break;
			str_ += c;
		}
		return true;
	}

	std::basic_string<CharT> str_;
};
