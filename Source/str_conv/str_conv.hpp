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

#include <string>
#include <string_view>
#include "stdafx.h"

namespace conv {

namespace std = ::std;

std::string to_utf8(std::string_view sv);
std::string to_utf8(std::wstring_view sv);
std::string to_utf8(std::u16string_view sv);
std::string to_utf8(std::u32string_view sv);

std::u16string to_utf16(std::string_view sv);
std::u16string to_utf16(std::wstring_view sv);
std::u16string to_utf16(std::u16string_view sv);
std::u16string to_utf16(std::u32string_view sv);

std::u32string to_utf32(std::string_view sv);
std::u32string to_utf32(std::wstring_view sv);
std::u32string to_utf32(std::u16string_view sv);
std::u32string to_utf32(std::u32string_view sv);

std::wstring to_wide(std::string_view sv);
std::wstring to_wide(std::wstring_view sv);
std::wstring to_wide(std::u16string_view sv);
std::wstring to_wide(std::u32string_view sv);

// MFC-specific

template <typename CharT>
auto to_utf8(const CharT *str) {
	return to_utf8(std::basic_string_view<CharT> {str});
}

template <typename CharT>
auto to_utf16(const CharT *str) {
	return to_utf16(std::basic_string_view<CharT> {str});
}

template <typename CharT>
auto to_utf32(const CharT *str) {
	return to_utf32(std::basic_string_view<CharT> {str});
}

template <typename CharT>
auto to_wide(const CharT *str) {
	return to_wide(std::basic_string_view<CharT> {str});
}

std::string to_utf8(const CStringA &str);
std::string to_utf8(const CStringW &str);
std::wstring to_wide(const CStringA &str);
std::wstring to_wide(const CStringW &str);

} // namespace conv
