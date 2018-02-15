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

#include <string_view>
#include <utility>

namespace conv {

template <typename ForwardIt>
constexpr std::pair<ForwardIt, char32_t> utf8_to_codepoint(ForwardIt first, ForwardIt last) noexcept {
	if (first != last) {
		char32_t c = static_cast<unsigned char>(*first);
		++first;
		if (c <= 0x7Fu) // 0xxxxxxx
			return {first, c};
		if (c >= 0xC0u && first != last) {
			ForwardIt it = first;
			if (c <= 0xDFu) { // 110xxxxx 10xxxxxx
				if (auto c2 = static_cast<unsigned char>(*it); c2 >= 0x80u && c2 <= 0xBFu)
					return {++it, static_cast<char32_t>(((c & 0x1Fu) << 6) | (c2 & 0x3Fu))};
			}
			else if (c <= 0xEFu) { // 1110xxxx 10xxxxxx 10xxxxxx
				if (auto c2 = static_cast<unsigned char>(*it); c2 >= 0x80u && c2 <= 0xBFu && ++it != last)
					if (auto c3 = static_cast<unsigned char>(*it); c3 >= 0x80u && c3 <= 0xBFu)
						if (char32_t codepoint = ((c & 0x0Fu) << 12) | ((c2 & 0x3Fu) << 6) | (c3 & 0x3Fu); codepoint < 0xD800u || codepoint > 0xDFFFu)
							return {++it, codepoint};
			}
			else if (c <= 0xF7u) { // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
				if (auto c2 = static_cast<unsigned char>(*it); c2 >= 0x80u && c2 <= 0xBFu && ++it != last)
					if (auto c3 = static_cast<unsigned char>(*it); c3 >= 0x80u && c3 <= 0xBFu && ++it != last)
						if (auto c4 = static_cast<unsigned char>(*it); c4 >= 0x80u && c4 <= 0xBFu)
							return {++it, static_cast<char32_t>(((c & 0x07u) << 18) | ((c2 & 0x3Fu) << 12) | ((c3 & 0x3Fu) << 6) | (c4 & 0x3Fu))};
			}
		}
	}
	return {first, 0xFFFDu};
}

template <typename ForwardIt>
constexpr std::pair<ForwardIt, char32_t> utf16_to_codepoint(ForwardIt first, ForwardIt last) noexcept {
	if (first != last) {
		char32_t c = static_cast<char16_t>(*first);
		++first;
		if (c < 0xD800u || c > 0xDFFFu) // xxxxxxxx xxxxxxxx
			return {first, c};
		else if (c <= 0xDBFFu && first != last) { // high surrogate
			ForwardIt it = first;
			if (auto c2 = static_cast<char16_t>(*it); c2 >= 0xDC00u && c2 <= 0xDFFFu) // low surrogate
				return {++it, static_cast<char32_t>((((c & 0x3FFu) << 10) | (c2 & 0x3FFu)) + 0x10000u)};
		}
	}
	return {first, 0xFFFDu};
}

template <typename ForwardIt>
constexpr std::pair<ForwardIt, char32_t> utf32_to_codepoint(ForwardIt first, ForwardIt last) noexcept {
	if (first != last) {
		char32_t c = static_cast<char32_t>(*first);
		++first;
		if (c < 0xD800u || c > 0xDFFFu) // xxxxxxxx xxxxxxxx
			return {first, c};
	}
	return {first, 0xFFFDu};
}



template <typename OutputIt>
constexpr OutputIt codepoint_to_utf8(OutputIt it, char32_t c) noexcept {
	if (c <= 0x7Fu) // 0xxxxxxx
		*it++ = static_cast<char>(c);
	else if (c <= 0x7FFu) { // 110xxxxx 10xxxxxx
		*it++ = static_cast<char>(((c >> 6) & 0x1Fu) | 0xC0u);
		*it++ = static_cast<char>((c & 0x3Fu) | 0x80u);
	}
	else if (c < 0xFFFEu) { // 1110xxxx 10xxxxxx 10xxxxxx
		if (c >= 0xD800u && c <= 0xDFFFu)
			return codepoint_to_utf8(it, 0xFFFDu);
		*it++ = static_cast<char>(((c >> 12) & 0x0Fu) | 0xE0u);
		*it++ = static_cast<char>(((c >> 6) & 0x3Fu) | 0x80u);
		*it++ = static_cast<char>((c & 0x3Fu) | 0x80u);
	}
	else if ((c >> 16) <= 0x10u && (c & 0xFFFFu) < 0xFFFEu) { // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		*it++ = static_cast<char>(((c >> 18) & 0x07u) | 0xF0u);
		*it++ = static_cast<char>(((c >> 12) & 0x3Fu) | 0x80u);
		*it++ = static_cast<char>(((c >> 6) & 0x3Fu) | 0x80u);
		*it++ = static_cast<char>((c & 0x3Fu) | 0x80u);
	}
//	else
//		return codepoint_to_utf8(it, 0xFFFDu);
	return it;
}

template <typename OutputIt>
constexpr OutputIt codepoint_to_utf16(OutputIt it, char32_t c) noexcept {
	if (c < 0xFFFEu) { // xxxxxxxx xxxxxxxx
		if (c >= 0xD800u && c <= 0xDFFFu)
			return codepoint_to_utf16(it, 0xFFFDu);
		*it++ = static_cast<char16_t>(c & 0xFFFFu);
	}
	else if ((c >> 16) <= 0x10u && (c & 0xFFFFu) < 0xFFFEu) { // 110110xx xxxxxxxx, 110111xx xxxxxxxx
		c -= 0x10000u;
		*it++ = static_cast<char16_t>(((c >> 10) & 0x3FFu) | 0xD800u);
		*it++ = static_cast<char16_t>((c & 0x3FFu) | 0xDC00u);
	}
//	else
//		return codepoint_to_utf16(it, 0xFFFDu);
	return it;
}

template <typename OutputIt>
constexpr OutputIt codepoint_to_utf32(OutputIt it, char32_t c) noexcept {
	if ((c >> 16) <= 0x10u && (c & 0xFFFFu) < 0xFFFEu) {
		if (c >= 0xD800u && c <= 0xDFFFu)
			return codepoint_to_utf32(it, 0xFFFDu);
		*it++ = c;
	}
//	else
//		return codepoint_to_utf32(it, 0xFFFDu);
	return it;
}

} // namespace conv
