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

#include "ft0cc/cpputil/utf8_conv.hpp"
#include "gtest/gtest.h"
#include <array>

// all uses of U+FFFD are intentional
// tests may fail on MSVC if this file isn't saved in UTF-8 with BOM

using namespace conv;

#define TEST_STR_IMPL(N) test_str##N
#define TEST_STR(N) TEST_STR_IMPL(N)
#define TEST_RET_IMPL(N) test_ret##N
#define TEST_RET(N) TEST_RET_IMPL(N)



#define UTF8_TO_CODEPOINT_TESTCASE_IMPL(STR, RCOUNT, CHAR, I, O) \
	constexpr char I[] = STR; \
	constexpr auto O = utf8_to_codepoint(std::begin(I), std::end(I) - 1); \
	static_assert(O.first == std::begin(I) + RCOUNT, "utf8_to_codepoint: iterator mismatch"); \
	static_assert(O.second == CHAR, "utf8_to_codepoint: code point mismatch")
#define UTF8_TO_CODEPOINT_TESTCASE(STR, RCOUNT, CHAR) \
	UTF8_TO_CODEPOINT_TESTCASE_IMPL(STR, RCOUNT, CHAR, TEST_STR(__LINE__), TEST_RET(__LINE__))

#define UTF8_TO_CODEPOINT_TESTCASE2_IMPL(RCOUNT, CHAR, I, O, ...) \
	constexpr unsigned char I[] = {__VA_ARGS__, 0x00u}; \
	constexpr auto O = utf8_to_codepoint(std::begin(I), std::end(I) - 1); \
	static_assert(O.first == std::begin(I) + RCOUNT, "utf8_to_codepoint: iterator mismatch"); \
	static_assert(O.second == CHAR, "utf8_to_codepoint: code point mismatch")
#define UTF8_TO_CODEPOINT_TESTCASE2(RCOUNT, CHAR, ...) \
	UTF8_TO_CODEPOINT_TESTCASE2_IMPL(RCOUNT, CHAR, TEST_STR(__LINE__), TEST_RET(__LINE__), __VA_ARGS__)

// basic encoding
UTF8_TO_CODEPOINT_TESTCASE(u8"!", 1, U'!');
UTF8_TO_CODEPOINT_TESTCASE(u8"×", 2, U'×');
UTF8_TO_CODEPOINT_TESTCASE(u8"☭", 3, U'☭');
UTF8_TO_CODEPOINT_TESTCASE(u8"😂", 4, U'😂');

// null/empty string
UTF8_TO_CODEPOINT_TESTCASE(u8"", 0, U'�');
UTF8_TO_CODEPOINT_TESTCASE(u8"\0", 1, U'\0');

// invalid encoding
UTF8_TO_CODEPOINT_TESTCASE2(1, U'�', 0x81u);
UTF8_TO_CODEPOINT_TESTCASE2(1, U'�', 0xC0u);
UTF8_TO_CODEPOINT_TESTCASE2(1, U'�', 0xC0u, 0xC0u);
UTF8_TO_CODEPOINT_TESTCASE2(1, U'�', 0xE0u);
UTF8_TO_CODEPOINT_TESTCASE2(1, U'�', 0xE0u, 0x81u);
UTF8_TO_CODEPOINT_TESTCASE2(1, U'�', 0xF0u);
UTF8_TO_CODEPOINT_TESTCASE2(1, U'�', 0xF0u, 0x81u);
UTF8_TO_CODEPOINT_TESTCASE2(1, U'�', 0xF0u, 0x81u, 0x81u);
UTF8_TO_CODEPOINT_TESTCASE2(1, U'�', 0xF8u, 0x81u, 0x81u, 0x81u, 0x81u);
UTF8_TO_CODEPOINT_TESTCASE2(1, U'�', 0xFCu, 0x81u, 0x81u, 0x81u, 0x81u, 0x81u);
UTF8_TO_CODEPOINT_TESTCASE2(1, U'�', 0xFEu);
UTF8_TO_CODEPOINT_TESTCASE2(1, U'�', 0xFFu);

// surrogate pair
//UTF8_TO_CODEPOINT_TESTCASE2(3, U'\uD7FF', 0xEDu, 0x9Fu, 0xBFu);
//UTF8_TO_CODEPOINT_TESTCASE2(3, U'�', 0xEDu, 0xA0u, 0x80u);
//UTF8_TO_CODEPOINT_TESTCASE2(3, U'�', 0xEDu, 0xAFu, 0xBFu);
//UTF8_TO_CODEPOINT_TESTCASE2(3, U'�', 0xEDu, 0xB0u, 0x80u);
//UTF8_TO_CODEPOINT_TESTCASE2(3, U'�', 0xEDu, 0xBFu, 0xBFu);
//UTF8_TO_CODEPOINT_TESTCASE2(3, U'\uE000', 0xEEu, 0x80u, 0x80u);



constexpr auto make_utf8_array(char32_t c) noexcept {
	std::array<char, 8> I = { };
	auto O = codepoint_to_utf8(std::begin(I), c);
	(void)O;
	return I; //std::make_pair(I, O - std::begin(I));
}

template <typename T, typename U, std::size_t... Is>
constexpr bool eql_impl(const T &str, const U &arr, std::index_sequence<Is...>) noexcept {
	return ((str[Is] == arr[Is]) && ...);
}

template <typename T, std::size_t N, typename U>
constexpr bool compare_str_array(const T (&str)[N], const U &arr) noexcept {
	return eql_impl(str, arr, std::make_index_sequence<N>());
}

#define CODEPOINT_TO_UTF8_TESTCASE_IMPL(STR, CHAR, I, O) \
	constexpr char I[] = STR; \
	constexpr auto O = make_utf8_array(CHAR); \
	static_assert(compare_str_array(I, O), "codepoint_to_utf8: string mismatch")
#define CODEPOINT_TO_UTF8_TESTCASE(STR, CHAR) \
	CODEPOINT_TO_UTF8_TESTCASE_IMPL(STR, CHAR, TEST_STR(__LINE__), TEST_RET(__LINE__))

// basic encoding
CODEPOINT_TO_UTF8_TESTCASE(u8"\0", U'\0');
CODEPOINT_TO_UTF8_TESTCASE(u8"!", U'!');
CODEPOINT_TO_UTF8_TESTCASE(u8"×", U'×');
CODEPOINT_TO_UTF8_TESTCASE(u8"☭", U'☭');
CODEPOINT_TO_UTF8_TESTCASE(u8"😂", U'😂');
CODEPOINT_TO_UTF8_TESTCASE(u8"�", U'�');

// non-characters
CODEPOINT_TO_UTF8_TESTCASE(u8"", 0xFFFEu);
CODEPOINT_TO_UTF8_TESTCASE(u8"", 0xFFFFu);
CODEPOINT_TO_UTF8_TESTCASE(u8"", 0x1FFFEu);
CODEPOINT_TO_UTF8_TESTCASE(u8"", 0x1FFFFu);
CODEPOINT_TO_UTF8_TESTCASE(u8"", 0x10FFFFu);
CODEPOINT_TO_UTF8_TESTCASE(u8"", 0x110000u);

// surrogate pair
CODEPOINT_TO_UTF8_TESTCASE(u8"\uD7FF", U'\uD7FF');
CODEPOINT_TO_UTF8_TESTCASE(u8"�", 0xD800);
CODEPOINT_TO_UTF8_TESTCASE(u8"�", 0xDBFF);
CODEPOINT_TO_UTF8_TESTCASE(u8"�", 0xDC00);
CODEPOINT_TO_UTF8_TESTCASE(u8"�", 0xDFFF);
CODEPOINT_TO_UTF8_TESTCASE(u8"\uE000", U'\uE000');



#define UTF16_TO_CODEPOINT_TESTCASE_IMPL(STR, RCOUNT, CHAR, I, O) \
	constexpr char16_t I[] = STR; \
	constexpr auto O = utf16_to_codepoint(std::begin(I), std::end(I) - 1); \
	static_assert(O.first == std::begin(I) + RCOUNT, "utf16_to_codepoint: iterator mismatch"); \
	static_assert(O.second == CHAR, "utf16_to_codepoint: code point mismatch")
#define UTF16_TO_CODEPOINT_TESTCASE(STR, RCOUNT, CHAR) \
	UTF16_TO_CODEPOINT_TESTCASE_IMPL(STR, RCOUNT, CHAR, TEST_STR(__LINE__), TEST_RET(__LINE__))

#define UTF16_TO_CODEPOINT_TESTCASE2_IMPL(RCOUNT, CHAR, I, O, ...) \
	constexpr char16_t I[] = {__VA_ARGS__, 0x0000u}; \
	constexpr auto O = utf16_to_codepoint(std::begin(I), std::end(I) - 1); \
	static_assert(O.first == std::begin(I) + RCOUNT, "utf16_to_codepoint: iterator mismatch"); \
	static_assert(O.second == CHAR, "utf16_to_codepoint: code point mismatch")
#define UTF16_TO_CODEPOINT_TESTCASE2(RCOUNT, CHAR, ...) \
	UTF16_TO_CODEPOINT_TESTCASE2_IMPL(RCOUNT, CHAR, TEST_STR(__LINE__), TEST_RET(__LINE__), __VA_ARGS__)

// basic encoding
UTF16_TO_CODEPOINT_TESTCASE(u"!", 1, U'!');
UTF16_TO_CODEPOINT_TESTCASE(u"×", 1, U'×');
UTF16_TO_CODEPOINT_TESTCASE(u"☭", 1, U'☭');
UTF16_TO_CODEPOINT_TESTCASE(u"😂", 2, U'😂');

// null/empty string
UTF16_TO_CODEPOINT_TESTCASE(u"", 0, U'�');
UTF16_TO_CODEPOINT_TESTCASE(u"\0", 1, U'\0');

// invalid encoding
UTF16_TO_CODEPOINT_TESTCASE2(1, U'�', 0xD800u);
UTF16_TO_CODEPOINT_TESTCASE2(1, U'�', 0xDBFFu);
UTF16_TO_CODEPOINT_TESTCASE2(1, U'�', 0xDC00u);
UTF16_TO_CODEPOINT_TESTCASE2(1, U'�', 0xDFFFu);
UTF16_TO_CODEPOINT_TESTCASE2(1, U'�', 0xD800u, 0x1234u);
UTF16_TO_CODEPOINT_TESTCASE2(1, U'�', 0xD800u, 0xD800u);
UTF16_TO_CODEPOINT_TESTCASE2(1, U'�', 0xDC00u, 0x1234u);
UTF16_TO_CODEPOINT_TESTCASE2(1, U'�', 0xDC00u, 0xD800u);
UTF16_TO_CODEPOINT_TESTCASE2(1, U'�', 0xDC00u, 0xDC00u);



constexpr auto make_utf16_array(char32_t c) noexcept {
	std::array<char16_t, 4> I = { };
	auto O = codepoint_to_utf16(std::begin(I), c);
	(void)O;
	return I; //std::make_pair(I, O - std::begin(I));
}

#define CODEPOINT_TO_UTF16_TESTCASE_IMPL(STR, CHAR, I, O) \
	constexpr char16_t I[] = STR; \
	constexpr auto O = make_utf16_array(CHAR); \
	static_assert(compare_str_array(I, O), "codepoint_to_utf16: string mismatch")
#define CODEPOINT_TO_UTF16_TESTCASE(STR, CHAR) \
	CODEPOINT_TO_UTF16_TESTCASE_IMPL(STR, CHAR, TEST_STR(__LINE__), TEST_RET(__LINE__))

// basic encoding
CODEPOINT_TO_UTF16_TESTCASE(u"\0", U'\0');
CODEPOINT_TO_UTF16_TESTCASE(u"!", U'!');
CODEPOINT_TO_UTF16_TESTCASE(u"×", U'×');
CODEPOINT_TO_UTF16_TESTCASE(u"☭", U'☭');
CODEPOINT_TO_UTF16_TESTCASE(u"😂", U'😂');
CODEPOINT_TO_UTF16_TESTCASE(u"�", U'�');

// non-characters
CODEPOINT_TO_UTF16_TESTCASE(u"", 0xFFFEu);
CODEPOINT_TO_UTF16_TESTCASE(u"", 0xFFFFu);
CODEPOINT_TO_UTF16_TESTCASE(u"", 0x1FFFEu);
CODEPOINT_TO_UTF16_TESTCASE(u"", 0x1FFFFu);
CODEPOINT_TO_UTF16_TESTCASE(u"", 0x10FFFFu);
CODEPOINT_TO_UTF16_TESTCASE(u"", 0x110000u);

// surrogate pair
CODEPOINT_TO_UTF16_TESTCASE(u"\uD7FF", U'\uD7FF');
CODEPOINT_TO_UTF16_TESTCASE(u"�", 0xD800u);
CODEPOINT_TO_UTF16_TESTCASE(u"�", 0xDBFFu);
CODEPOINT_TO_UTF16_TESTCASE(u"�", 0xDC00u);
CODEPOINT_TO_UTF16_TESTCASE(u"�", 0xDFFFu);
CODEPOINT_TO_UTF16_TESTCASE(u"\uE000", U'\uE000');



using namespace std::string_view_literals;

#define UTF8_SUBSTR_TESTCASE_IMPL(STR, COUNT, LEN, I, O) \
	constexpr std::string_view I = STR##sv; \
	constexpr auto O = utf8_trim(I.substr(0, COUNT)); \
	static_assert(O.data() == I.data()); \
	static_assert(O.size() == LEN, "utf8_substr: length mismatch")
#define UTF8_SUBSTR_TESTCASE(STR, COUNT, LEN) \
	UTF8_SUBSTR_TESTCASE_IMPL(STR, COUNT, LEN, TEST_STR(__LINE__), TEST_RET(__LINE__))

#define UTF8_SUBSTR_TESTCASE2_IMPL(LEN, I, O, ...) \
	constexpr char I[] = {__VA_ARGS__, '\0'}; \
	constexpr auto O = utf8_trim(std::string_view {I, std::size(I) - 1u}); \
	static_assert(O.data() == I); \
	static_assert(O.size() == LEN, "utf8_substr: length mismatch")
#define UTF8_SUBSTR_TESTCASE2(LEN, ...) \
	UTF8_SUBSTR_TESTCASE2_IMPL(LEN, TEST_STR(__LINE__), TEST_RET(__LINE__), __VA_ARGS__)

UTF8_SUBSTR_TESTCASE(u8"!", 1u, 1u);
UTF8_SUBSTR_TESTCASE(u8"×", 2u, 2u);
UTF8_SUBSTR_TESTCASE(u8"×", 1u, 0u);
UTF8_SUBSTR_TESTCASE(u8"☭", 3u, 3u);
UTF8_SUBSTR_TESTCASE(u8"☭", 2u, 0u);
UTF8_SUBSTR_TESTCASE(u8"☭", 1u, 0u);
UTF8_SUBSTR_TESTCASE(u8"😂", 4u, 4u);
UTF8_SUBSTR_TESTCASE(u8"😂", 3u, 0u);
UTF8_SUBSTR_TESTCASE(u8"😂", 2u, 0u);
UTF8_SUBSTR_TESTCASE(u8"😂", 1u, 0u);

UTF8_SUBSTR_TESTCASE2(1u, '\x80');
UTF8_SUBSTR_TESTCASE2(2u, '\x80', '\x80');
UTF8_SUBSTR_TESTCASE2(3u, '\x80', '\x80', '\x80');
