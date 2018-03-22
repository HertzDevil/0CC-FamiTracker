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

#include <type_traits>
#include <limits>
#include <algorithm>

namespace details {

template <typename From, typename To>
struct int_contains : std::bool_constant<
	std::is_integral_v<From> && std::is_integral_v<To> &&
	((std::is_signed_v<From> == std::is_signed_v<To> && sizeof(From) >= sizeof(To)) || (
		std::is_signed_v<From> && std::is_unsigned_v<To> && sizeof(From) > sizeof(To)))> { };
template <typename From>
struct int_contains<From, bool> : std::bool_constant<std::is_integral_v<From>> { };
template <typename To>
struct int_contains<bool, To> : std::false_type { };
template <>
struct int_contains<bool, bool> : std::true_type { };
template <typename From, typename To>
inline constexpr bool int_contains_v = int_contains<From, To>::value;

template <typename To, typename From,
	std::enable_if_t<std::is_integral_v<To>, int> = 0,
	std::enable_if_t<std::is_integral_v<From>, int> = 0>
constexpr To clip(From x) noexcept {
	if constexpr (details::int_contains_v<To, From>)
		return static_cast<To>(x);
	else if constexpr (details::int_contains_v<From, To>)
		return static_cast<To>(std::clamp(x,
			static_cast<From>(std::numeric_limits<To>::min()),
			static_cast<From>(std::numeric_limits<To>::max())));
	else if constexpr (std::is_signed_v<To> && std::is_unsigned_v<From>)
		return static_cast<To>(std::min(x,
			static_cast<From>(std::numeric_limits<To>::max())));
	else if constexpr (std::is_unsigned_v<To> && std::is_signed_v<From>)
		return static_cast<To>(std::max(x,
			static_cast<From>(std::numeric_limits<To>::min())));
	else
		static_assert(!sizeof(To), "Something happened");
}

} // namespace details

template <typename To, typename From>
constexpr To clip(From x) noexcept {
	return details::clip<To>(x);
}
