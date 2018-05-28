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

#include "ft0cc/cpputil/array_view.hpp"
#include <string>

namespace details {

template <typename T, std::size_t N, std::size_t... Is>
static constexpr T
MakeIntegerLE(const std::byte (&buf)[N], std::index_sequence<Is...>) noexcept {
	return static_cast<T>((... | (std::to_integer<std::uintmax_t>(buf[Is]) << (Is * 8))));
}

} // namespace details

class CBinaryReader {
public:
	template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
	T ReadInt() {
		std::byte buf[sizeof(T)] = { };
		std::size_t readcount = ReadBytes(buf);
		if (readcount != sizeof(T))
			throw std::runtime_error {"Unexpected EOF reached"};
		return details::MakeIntegerLE<T>(buf, std::make_index_sequence<sizeof(T)> { });
	}

	template <typename CharT>
	std::basic_string<CharT> ReadStringNull() {
		std::basic_string<CharT> str;
		while (true)
			if (CharT ch = ReadInt<CharT>())
				str += ch;
			else
				break;
		return str;
	}

	template <typename CharT>
	std::basic_string<CharT> ReadString() {
		std::basic_string<CharT> str;
		auto sz = ReadInt<std::uint32_t>();
		for (std::uint32_t i = 0; i < sz; ++i)
			str += ReadInt<CharT>();
		return str;
	}

	template <typename CharT>
	std::basic_string<CharT> ReadStringN(std::size_t count) {
		std::basic_string<CharT> str(count, CharT { });
		std::size_t readcount = ReadBytes(byte_view(str));
		str.resize(readcount / sizeof(CharT));
		return str;
	}

private:
	virtual std::size_t ReadBytes(array_view<std::byte> buf) = 0;
};
