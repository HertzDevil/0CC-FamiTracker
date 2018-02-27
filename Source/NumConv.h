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
#include <cstdint>
#include <optional>
#include <limits>
#include "to_sv.h"

// // // locale-independent number conversion routines

namespace conv {

using digit_type = unsigned;
inline constexpr digit_type unknown_digit = (unsigned)-1;

template <typename CharT>
constexpr unsigned from_digit(CharT ch) noexcept {
	if (ch >= CharT('0') && ch <= CharT('9'))
		return ch - '0';
	if (ch >= CharT('A') && ch <= CharT('Z'))
		return ch - 'A' + 10;
	if (ch >= CharT('a') && ch <= CharT('z'))
		return ch - CharT('a') + 10;
	return (unsigned)-1;
}

namespace details {

template <typename T, typename CharT>
constexpr std::optional<T>
to_unsigned(std::basic_string_view<CharT> sv, unsigned radix) noexcept {
	if (!sv.empty()) {
		uintmax_t x = 0uLL;
		for (CharT ch : sv) {
			digit_type digit = from_digit(ch);
			if (/*digit == unknown_digit ||*/ digit >= radix)
				return std::nullopt;
			if (x > (std::numeric_limits<T>::max() - digit) / radix)
				return std::nullopt;
			x = x * radix + digit;
		}
		return static_cast<T>(x);
	}
	return std::nullopt;
}

template <typename T, typename U, typename CharT>
constexpr std::optional<T>
to_signed(std::basic_string_view<CharT> sv, unsigned radix) noexcept {
	bool negative = false;
	if (!sv.empty()) {
		if (sv.front() == CharT('-')) {
			negative = true;
			sv.remove_prefix(1);
		}
		else if (sv.front() == CharT('+'))
			sv.remove_prefix(1);
	}

	if (auto px = to_unsigned<U>(sv, radix)) {
		auto x = static_cast<T>(*px);
		if (negative) {
			if (*px <= static_cast<U>(std::numeric_limits<T>::min()))
				return -x;
		}
		else {
			if (*px <= static_cast<U>(std::numeric_limits<T>::max()))
				return x;
		}
	}
	return std::nullopt;
}

} // namespace details

// converts a complete string to an unsigned 8-bit integer
// returns empty value if parse failed or value is out of range
template <typename T>
constexpr auto to_uint8(T&& str, unsigned radix = 10) noexcept {
	return details::to_unsigned<uint8_t>(to_sv(str), radix);
}

// converts a complete string to an unsigned 16-bit integer
// returns empty value if parse failed or value is out of range
template <typename T>
constexpr auto to_uint16(T&& str, unsigned radix = 10) noexcept {
	return details::to_unsigned<uint16_t>(to_sv(str), radix);
}

// converts a complete string to an unsigned 32-bit integer
// returns empty value if parse failed or value is out of range
template <typename T>
constexpr auto to_uint32(T&& str, unsigned radix = 10) noexcept {
	return details::to_unsigned<uint32_t>(to_sv(str), radix);
}

// converts a complete string to an unsigned integer
// returns empty value if parse failed or value is out of range
template <typename T>
constexpr auto to_uint(T&& str, unsigned radix = 10) noexcept {
	return details::to_unsigned<unsigned int>(to_sv(str), radix);
}

// converts a complete string to a signed 8-bit integer
// returns empty value if parse failed or value is out of range
template <typename T>
constexpr auto to_int8(T&& str, unsigned radix = 10) noexcept {
	return details::to_signed<int8_t, uint8_t>(to_sv(str), radix);
}

// converts a complete string to a signed 16-bit integer
// returns empty value if parse failed or value is out of range
template <typename T>
constexpr auto to_int16(T&& str, unsigned radix = 10) noexcept {
	return details::to_signed<int16_t, uint16_t>(to_sv(str), radix);
}

// converts a complete string to a signed 32-bit integer
// returns empty value if parse failed or value is out of range
template <typename T>
constexpr auto to_int32(T&& str, unsigned radix = 10) noexcept {
	return details::to_signed<int32_t, uint32_t>(to_sv(str), radix);
}

// converts a complete string to a signed integer
// returns empty value if parse failed or value is out of range
template <typename T>
constexpr auto to_int(T&& str, unsigned radix = 10) noexcept {
	return details::to_signed<int, unsigned int>(to_sv(str), radix);
}



namespace details {

template <typename T, unsigned Radix>
constexpr std::size_t max_places() noexcept {
	std::size_t e = 1; // for minus sign
	auto x = std::numeric_limits<T>::max();
	while (x) {
		x /= Radix;
		++e;
	}
	return e;
}

} // namespace details

template <typename CharT, typename T>
constexpr CharT to_digit(T x) noexcept {
	if (x >= T(0)) {
		if (x < T(10))
			return static_cast<CharT>(CharT('0') + x);
		if (x < T(36))
			return static_cast<CharT>(CharT('A') + x - 10);
	}
	return CharT('\0');
}

namespace details {

template <typename CharT, unsigned Radix>
struct str_buf_ {
	static constexpr std::size_t maxlen = details::max_places<uintmax_t, Radix>();
	static thread_local CharT data[maxlen + 1];
};

template <typename CharT, unsigned Radix>
thread_local CharT str_buf_<CharT, Radix>::data[maxlen + 1] = { };

template <unsigned Radix, unsigned BufLen, typename CharT>
CharT *from_uint_impl(CharT (&data)[BufLen], uintmax_t x, unsigned places) noexcept {
	CharT *it = std::end(data) - 1;
	*it = '\0';

	if (places == (unsigned)-1) {
		if (!x) {
			*--it = to_digit<CharT>(0);
			return it;
		}
		while (x) {
			*--it = to_digit<CharT>(x % Radix);
			x /= Radix;
		}
		return it;
	}

	if (places > BufLen - 2)
		places = BufLen - 2;
	while (places--) {
		*--it = to_digit<CharT>(x % Radix);
		x /= Radix;
	}
	return it;
}

template <unsigned Radix, unsigned BufLen, typename CharT>
std::basic_string_view<CharT> from_uint(CharT (&data)[BufLen], uintmax_t x, unsigned places) noexcept {
	return from_uint_impl<Radix>(data, x, places);
}

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4146) // unary minus operator applied to unsigned type, result still unsigned
#endif
template <unsigned Radix, unsigned BufLen, typename CharT>
std::string_view from_int(CharT (&data)[BufLen], intmax_t x, unsigned places) noexcept {
	if (x >= 0)
		return from_uint_impl<Radix>(data, x, places);
	CharT *it = from_uint_impl<Radix>(data, -static_cast<uintmax_t>(x), places);
	*--it = '-';
	return it;
}
#ifdef _MSC_VER
#pragma warning (pop)
#endif

} // namespace details

// converts an unsigned integer to a decimal string
// every sv_* function shares the same buffer in each thread
inline std::string_view sv_from_uint(uintmax_t x, unsigned places = (unsigned)-1) noexcept {
	return details::from_uint<10>(details::str_buf_<char, 10>::data, x, places);
}

// converts an unsigned integer to a binary string
// every sv_* function shares the same buffer in each thread
inline std::string_view sv_from_uint_bin(uintmax_t x, unsigned places = (unsigned)-1) noexcept {
	return details::from_uint<2>(details::str_buf_<char, 2>::data, x, places);
}

// converts an unsigned integer to an octal string
// every sv_* function shares the same buffer in each thread
inline std::string_view sv_from_uint_oct(uintmax_t x, unsigned places = (unsigned)-1) noexcept {
	return details::from_uint<8>(details::str_buf_<char, 8>::data, x, places);
}

// converts an unsigned integer to a hexadecimal string
// every sv_* function shares the same buffer in each thread
inline std::string_view sv_from_uint_hex(uintmax_t x, unsigned places = (unsigned)-1) noexcept {
	return details::from_uint<16>(details::str_buf_<char, 16>::data, x, places);
}

// converts a signed integer to a decimal string
// every sv_* function shares the same buffer in each thread
inline std::string_view sv_from_int(intmax_t x, unsigned places = (unsigned)-1) noexcept {
	return details::from_int<10>(details::str_buf_<char, 10>::data, x, places);
}

// converts a signed integer to a binary string
// every sv_* function shares the same buffer in each thread
inline std::string_view sv_from_int_bin(intmax_t x, unsigned places = (unsigned)-1) noexcept {
	return details::from_int<2>(details::str_buf_<char, 2>::data, x, places);
}

// converts a signed integer to an octal string
// every sv_* function shares the same buffer in each thread
inline std::string_view sv_from_int_oct(intmax_t x, unsigned places = (unsigned)-1) noexcept {
	return details::from_int<8>(details::str_buf_<char, 8>::data, x, places);
}

// converts a signed integer to a hexadecimal string
// every sv_* function shares the same buffer in each thread
inline std::string_view sv_from_int_hex(intmax_t x, unsigned places = (unsigned)-1) noexcept {
	return details::from_int<16>(details::str_buf_<char, 16>::data, x, places);
}

// converts an unsigned integer to a decimal string
// constructs a std::string immediately
inline std::string from_uint(uintmax_t x, unsigned places = (unsigned)-1) {
	char buf[details::max_places<uintmax_t, 10>() + 1] = { };
	return std::string {details::from_uint<10>(buf, x, places)};
}

// converts an unsigned integer to a binary string
// constructs a std::string immediately
inline std::string from_uint_bin(uintmax_t x, unsigned places = (unsigned)-1) {
	char buf[details::max_places<uintmax_t, 2>() + 1] = { };
	return std::string {details::from_uint<2>(buf, x, places)};
}

// converts an unsigned integer to an octal string
// constructs a std::string immediately
inline std::string from_uint_oct(uintmax_t x, unsigned places = (unsigned)-1) {
	char buf[details::max_places<uintmax_t, 8>() + 1] = { };
	return std::string {details::from_uint<8>(buf, x, places)};
}

// converts an unsigned integer to a hexadecimal string
// constructs a std::string immediately
inline std::string from_uint_hex(uintmax_t x, unsigned places = (unsigned)-1) {
	char buf[details::max_places<uintmax_t, 16>() + 1] = { };
	return std::string {details::from_uint<16>(buf, x, places)};
}

// converts a signed integer to a decimal string
// constructs a std::string immediately
inline std::string from_int(intmax_t x, unsigned places = (unsigned)-1) {
	char buf[details::max_places<uintmax_t, 10>() + 1] = { };
	return std::string {details::from_int<10>(buf, x, places)};
}

// converts a signed integer to a binary string
// constructs a std::string immediately
inline std::string from_int_bin(intmax_t x, unsigned places = (unsigned)-1) {
	char buf[details::max_places<uintmax_t, 2>() + 1] = { };
	return std::string {details::from_int<2>(buf, x, places)};
}

// converts a signed integer to an octal string
// constructs a std::string immediately
inline std::string from_int_oct(intmax_t x, unsigned places = (unsigned)-1) {
	char buf[details::max_places<uintmax_t, 8>() + 1] = { };
	return std::string {details::from_int<8>(buf, x, places)};
}

// converts a signed integer to a hexadecimal string
// constructs a std::string immediately
inline std::string from_int_hex(intmax_t x, unsigned places = (unsigned)-1) {
	char buf[details::max_places<uintmax_t, 16>() + 1] = { };
	return std::string {details::from_int<16>(buf, x, places)};
}

// converts an unsigned number into a time string
inline std::string time_from_uint(uintmax_t seconds) {
	return from_uint(seconds / 60u) + ':' + from_uint(seconds % 60u, 2);
}

// converts a signed number into a time string
inline std::string time_from_int(intmax_t seconds) {
	return seconds >= 0 ? time_from_uint(seconds) : ('-' + time_from_uint(-seconds));
}

} // namespace conv
