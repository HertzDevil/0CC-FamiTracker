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

#include "str_conv/str_conv.hpp"
#include "str_conv/utf8_conv.hpp"

namespace conv {

namespace details {

template <typename CharT, typename ForwardIt>
constexpr auto codepoint_from_utf(ForwardIt first, ForwardIt last) noexcept {
	if constexpr (sizeof(CharT) == sizeof(char))
		return utf8_to_codepoint(std::move(first), std::move(last));
	else if constexpr (sizeof(CharT) == sizeof(char16_t))
		return utf16_to_codepoint(std::move(first), std::move(last));
	else if constexpr (sizeof(CharT) == sizeof(char32_t))
		return utf32_to_codepoint(std::move(first), std::move(last));
	else
		static_assert(!sizeof(ForwardIt), "Invalid character size");
}

template <typename CharT, typename OutputIt>
constexpr auto utf_from_codepoint(OutputIt it, char32_t c) noexcept {
	if constexpr (sizeof(CharT) == sizeof(char))
		return codepoint_to_utf8(std::move(it), c);
	else if constexpr (sizeof(CharT) == sizeof(char16_t))
		return codepoint_to_utf16(std::move(it), c);
	else if constexpr (sizeof(CharT) == sizeof(char32_t))
		return codepoint_to_utf32(std::move(it), c);
	else
		static_assert(!sizeof(OutputIt), "Invalid character size");
}

template <typename To, typename From>
std::basic_string<To> utf_convert(std::basic_string_view<From> sv) {
	std::basic_string<To> str;
	auto b = sv.begin();
	auto e = sv.end();
	while (b != e) {
		auto [it, ch] = codepoint_from_utf<From>(b, e);
		b = it;
		utf_from_codepoint<To>(std::back_inserter(str), ch);
	}
	return str;
}

} // namespace details



std::string to_utf8(std::string_view sv) {
    return std::string {sv};
}

std::string to_utf8(std::wstring_view sv) {
	return details::utf_convert<char>(sv);
}

std::string to_utf8(std::u16string_view sv) {
	return details::utf_convert<char>(sv);
}

std::string to_utf8(std::u32string_view sv) {
	return details::utf_convert<char>(sv);
}

std::wstring to_wide(std::string_view sv) {
	return details::utf_convert<wchar_t>(sv);
}

std::wstring to_wide(std::wstring_view sv) {
	return std::wstring {sv};
}

std::wstring to_wide(std::u16string_view sv) {
	return details::utf_convert<wchar_t>(sv);
}

std::wstring to_wide(std::u32string_view sv) {
	return details::utf_convert<wchar_t>(sv);
}

std::u16string to_utf16(std::string_view sv) {
	return details::utf_convert<char16_t>(sv);
}

std::u16string to_utf16(std::wstring_view sv) {
	return details::utf_convert<char16_t>(sv);
}

std::u16string to_utf16(std::u16string_view sv) {
    return std::u16string {sv};
}

std::u16string to_utf16(std::u32string_view sv) {
	return details::utf_convert<char16_t>(sv);
}

std::u32string to_utf32(std::string_view sv) {
	return details::utf_convert<char32_t>(sv);
}

std::u32string to_utf32(std::wstring_view sv) {
	return details::utf_convert<char32_t>(sv);
}

std::u32string to_utf32(std::u16string_view sv) {
	return details::utf_convert<char32_t>(sv);
}

std::u32string to_utf32(std::u32string_view sv) {
    return std::u32string {sv};
}

// MFC-specific

std::string to_utf8(const CStringA &str) {
	return to_utf8(std::string_view {str.GetString(), (std::size_t)str.GetLength()});
}

std::string to_utf8(const CStringW &str) {
	return to_utf8(std::wstring_view {str.GetString(), (std::size_t)str.GetLength()});
}

std::wstring to_wide(const CStringA &str) {
	return to_wide(std::string_view {str.GetString(), (std::size_t)str.GetLength()});
}

std::wstring to_wide(const CStringW &str) {
	return to_wide(std::wstring_view {str.GetString(), (std::size_t)str.GetLength()});
}

} // namespace conv
