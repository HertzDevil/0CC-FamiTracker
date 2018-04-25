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

// // // placeholder for std::strong_ordering
// // // TODO: replace with operator<=>

namespace details {

template <typename T>
using compare_type = decltype(std::declval<T>().compare(std::declval<T>()));

} // namespace details

template <typename T>
struct IStrongOrderable : std::false_type { };

#define ENABLE_STRONG_ORDERING(T) \
	template <> \
	struct IStrongOrderable<T> : std::true_type { }

template <typename T,
	typename = std::enable_if_t<IStrongOrderable<T>::value>,
	typename = details::compare_type<T>>
constexpr bool operator==(const T &lhs, const T &rhs)
	noexcept(noexcept(lhs.compare(rhs)))
{
	return lhs.compare(rhs) == 0;
}

template <typename T,
	typename = std::enable_if_t<IStrongOrderable<T>::value>,
	typename = details::compare_type<T>>
constexpr bool operator!=(const T &lhs, const T &rhs)
	noexcept(noexcept(lhs.compare(rhs)))
{
	return lhs.compare(rhs) != 0;
}

template <typename T,
	typename = std::enable_if_t<IStrongOrderable<T>::value>,
	typename = details::compare_type<T>>
constexpr bool operator<(const T &lhs, const T &rhs)
	noexcept(noexcept(lhs.compare(rhs)))
{
	return lhs.compare(rhs) < 0;
}

template <typename T,
	typename = std::enable_if_t<IStrongOrderable<T>::value>,
	typename = details::compare_type<T>>
constexpr bool operator<=(const T &lhs, const T &rhs)
	noexcept(noexcept(lhs.compare(rhs)))
{
	return lhs.compare(rhs) <= 0;
}

template <typename T,
	typename = std::enable_if_t<IStrongOrderable<T>::value>,
	typename = details::compare_type<T>>
constexpr bool operator>(const T &lhs, const T &rhs)
	noexcept(noexcept(lhs.compare(rhs)))
{
	return lhs.compare(rhs) > 0;
}

template <typename T,
	typename = std::enable_if_t<IStrongOrderable<T>::value>,
	typename = details::compare_type<T>>
constexpr bool operator>=(const T &lhs, const T &rhs)
	noexcept(noexcept(lhs.compare(rhs)))
{
	return lhs.compare(rhs) >= 0;
}
