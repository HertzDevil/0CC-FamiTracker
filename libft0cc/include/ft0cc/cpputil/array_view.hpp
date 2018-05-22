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


#pragma once

#include <iterator>
#include <limits>
#include <type_traits>
#include <cstddef>

namespace details {

template <typename T>
using data_func_t = decltype(std::declval<T>().data());
template <typename T>
using size_func_t = decltype(std::declval<T>().size());
template <typename T, typename ValT, typename = void>
struct is_view_like : std::false_type { };
template <typename T, typename ValT>
struct is_view_like<T, ValT, std::void_t<data_func_t<T>, size_func_t<T>>> :
	std::bool_constant<std::is_convertible_v<data_func_t<T>, ValT *> &&
		std::is_convertible_v<size_func_t<T>, std::size_t>> { };
/*
template <typename T, typename ValT>
concept ViewLike = requires (T x) {
	{ x.data() } -> const ValT *;
	{ x.size() } -> std::size_t;
};
*/

} // namespace details

#define REQUIRES_ViewLike(T, ValT) \
	, std::enable_if_t<details::is_view_like<T, const ValT>::value, int> = 0

// substitute for std::span (a.k.a. gsl::span)
template <typename T>
class array_view {
public:
	using value_type = T;
	using pointer = T *;
	using const_pointer = std::add_const_t<T> *;
	using reference = T &;
	using const_reference = std::add_const_t<T> &;
	using iterator = pointer;
	using const_iterator = const_pointer;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	static constexpr auto npos = (size_type)-1;

	constexpr array_view() noexcept = default;
	constexpr array_view(pointer data, std::size_t sz) noexcept :
		data_(data), size_(sz) { }
	template <typename ValT, std::size_t N,
		std::enable_if_t<std::is_convertible_v<ValT *, pointer>, int> = 0>
	constexpr array_view(ValT (&data)[N]) noexcept :
		data_(data), size_(N) { }
	template <typename U REQUIRES_ViewLike(U, value_type)>
	constexpr array_view(U &arr) noexcept :
		data_(arr.data()), size_(arr.size()) { }
	template <typename U REQUIRES_ViewLike(U, value_type)>
	constexpr array_view(const U &arr) noexcept :
		data_(arr.data()), size_(arr.size()) { }

	constexpr array_view(const array_view &) noexcept = default;
	constexpr array_view(array_view &&) noexcept = default;
	constexpr array_view &operator=(const array_view &) noexcept = default;
	constexpr array_view &operator=(array_view &&) noexcept = default;
	~array_view() noexcept = default;

	constexpr void swap(array_view &other) noexcept {
		std::swap(data_, other.data_);
		std::swap(size_, other.size_);
	}

	constexpr reference operator[](size_type pos) const {
		return data_[pos];
	}
	constexpr reference at(size_type pos) const {
		if (pos >= size())
			throw std::out_of_range {"array_view::at() out of range"};
		return operator[](pos);
	}
	constexpr reference front() const {
		return operator[](0);
	}
	constexpr reference back() const {
		return operator[](size() - 1);
	}
	constexpr pointer data() const noexcept {
		return data_;
	}

	constexpr size_type size() const noexcept {
		return size_;
	}
	constexpr size_type length() const noexcept {
		return size_;
	}
	constexpr size_type max_size() const noexcept {
		return std::numeric_limits<size_type>::max() / sizeof(value_type);
	}
	constexpr bool empty() const noexcept {
		return size_ == 0u;
	}

	constexpr void clear() noexcept {
		*this = array_view { };
	}
	constexpr void remove_front(size_type count) noexcept {
		difference_type offs = (count < size()) ? count : size();
		data_ += offs;
		size_ -= offs;
	}
	constexpr void pop_front() noexcept {
		remove_front(1u);
	}
	constexpr void remove_back(size_type count) noexcept {
		size_ -= (count < size()) ? count : size();
	}
	constexpr void pop_back() noexcept {
		remove_back(1u);
	}
	constexpr array_view subview(size_type pos, size_type count = npos) const {
		if (pos > size())
			throw std::out_of_range {"pos cannot be larger than size()"};
		if (count > size() - pos)
			count = size() - pos;
		return {data_ + pos, count};
	}

	constexpr iterator begin() const noexcept {
		return iterator {data_};
	}
	constexpr const_iterator cbegin() const noexcept {
		return const_iterator {data_};
	}
	constexpr iterator end() const noexcept {
		return iterator {data_ + size_};
	}
	constexpr const_iterator cend() const noexcept {
		return const_iterator {data_ + size_};
	}

	reverse_iterator rbegin() const noexcept {
		return reverse_iterator {end()};
	}
	const_reverse_iterator crbegin() const noexcept {
		return const_reverse_iterator {cend()};
	}
	reverse_iterator rend() const noexcept {
		return reverse_iterator {begin()};
	}
	const_reverse_iterator crend() const noexcept {
		return const_reverse_iterator {cbegin()};
	}

	size_type copy(std::remove_const_t<value_type> *dest,
	               size_type count, size_type pos = 0) const {
		array_view trimmed = subview(pos, count);
		for (value_type x : trimmed)
			*dest++ = x;
		return trimmed.size();
	}
	template <typename U REQUIRES_ViewLike(U, std::remove_const_t<value_type>)>
	size_type copy(U &dest, size_type pos = 0) const {
		return copy(dest.data(), dest.size(), pos);
	}

	constexpr int
	compare(const array_view<std::add_const_t<value_type>> &other) const {
		auto b1 = begin();
		auto b2 = other.begin();
		auto e1 = end();
		auto e2 = other.end();

		if (b1 == b2 && e1 == e2)
			return 0;

		while ((b1 != e1) && (b2 != e2)) {
			if (*b1 < *b2)
				return -1;
			if (*b2 < *b1)
				return 1;
			++b1;
			++b2;
		}

		if (b2 != e2)
			return -1;
		if (b1 != e1)
			return 1;
		return 0;
	}

private:
	pointer data_ = nullptr;
	size_type size_ = 0u;
};

template <typename ValT>
constexpr void swap(array_view<ValT> &lhs, array_view<ValT> &rhs) noexcept {
	lhs.swap(rhs);
}



template <typename ValT, std::size_t N>
array_view(ValT (&)[N]) -> array_view<ValT>;

template <typename T
	REQUIRES_ViewLike(T, typename T::value_type)>
array_view(T &) -> array_view<typename T::value_type>;

template <typename T
	REQUIRES_ViewLike(T, std::add_const_t<typename T::value_type>)>
array_view(const T &) -> array_view<std::add_const_t<typename T::value_type>>;



namespace details {

template <typename T>
using less_than_t = decltype(std::declval<T>() < std::declval<T>());
template <typename T, typename = void>
struct less_than_comparable : std::false_type { };
template <typename T>
struct less_than_comparable<T, std::void_t<less_than_t<T>>> :
	std::bool_constant<std::is_convertible_v<less_than_t<T>, bool>> { };
/*
template <typename T, typename ValT>
concept LessThanComparable = requires (T x, T y) {
	{ x < y } -> bool;
};
*/

template <typename T>
using equality_t = decltype(std::declval<T>() == std::declval<T>());
template <typename T, typename = void>
struct equality_comparable : std::false_type { };
template <typename T>
struct equality_comparable<T, std::void_t<equality_t<T>>> :
	std::bool_constant<std::is_convertible_v<equality_t<T>, bool>> { };
/*
template <typename T, typename ValT>
concept EqualityComparable = requires (T x, T y) {
	{ x == y } -> bool;
};
*/

} // namespace details

template <typename ValT>
constexpr bool
operator==(const array_view<ValT> &lhs, const array_view<ValT> &rhs) {
	if constexpr (details::less_than_comparable<ValT>::value)
		return lhs.compare(rhs) == 0;
	else if constexpr (details::equality_comparable<ValT>::value) {
		auto b1 = lhs.begin();
		auto b2 = rhs.begin();
		auto e1 = lhs.end();
		auto e2 = rhs.end();

		if (b1 == b2 && e1 == e2)
			return true;

		while ((b1 != e1) && (b2 != e2)) {
			if (!(*b1 == *b2))
				return false;
			++b1;
			++b2;
		}

		return b1 == e1 && b2 == e2;
	}
	else
		return (lhs.empty() && rhs.empty()) || (
			lhs.data() == rhs.data() && lhs.size() == rhs.size());
}

template <typename ValT>
constexpr bool
operator!=(const array_view<ValT> &lhs, const array_view<ValT> &rhs) {
	return !(lhs == rhs);
}

template <typename ValT>
constexpr bool
operator<(const array_view<ValT> &lhs, const array_view<ValT> &rhs) {
	return lhs.compare(rhs) < 0;
}

template <typename ValT>
constexpr bool
operator<=(const array_view<ValT> &lhs, const array_view<ValT> &rhs) {
	return lhs.compare(rhs) <= 0;
}

template <typename ValT>
constexpr bool
operator>(const array_view<ValT> &lhs, const array_view<ValT> &rhs) {
	return lhs.compare(rhs) > 0;
}

template <typename ValT>
constexpr bool
operator>=(const array_view<ValT> &lhs, const array_view<ValT> &rhs) {
	return lhs.compare(rhs) >= 0;
}

namespace details {

template <typename T>
struct is_array_view : std::false_type { };
template <typename ValT>
struct is_array_view<array_view<ValT>> : std::true_type { };

} // namespace details

#define REQUIRES_NotArrayView(T) \
	, std::enable_if_t<!details::is_array_view<T>::value, int> = 0

template <typename T, typename ValT>
constexpr bool operator==(const array_view<ValT> &lhs, const T &rhs) {
	return array_view<const ValT> {lhs} == array_view<const ValT> {rhs};
}
template <typename T, typename ValT>
constexpr bool operator!=(const array_view<ValT> &lhs, const T &rhs) {
	return array_view<const ValT> {lhs} != array_view<const ValT> {rhs};
}
template <typename T, typename ValT>
constexpr bool operator<(const array_view<ValT> &lhs, const T &rhs) {
	return array_view<const ValT> {lhs} < array_view<const ValT> {rhs};
}
template <typename T, typename ValT>
constexpr bool operator>(const array_view<ValT> &lhs, const T &rhs) {
	return array_view<const ValT> {lhs} > array_view<const ValT> {rhs};
}
template <typename T, typename ValT>
constexpr bool operator<=(const array_view<ValT> &lhs, const T &rhs) {
	return array_view<const ValT> {lhs} <= array_view<const ValT> {rhs};
}
template <typename T, typename ValT>
constexpr bool operator>=(const array_view<ValT> &lhs, const T &rhs) {
	return array_view<const ValT> {lhs} >= array_view<const ValT> {rhs};
}

template <typename T REQUIRES_NotArrayView(T), typename ValT>
constexpr bool operator==(const T &lhs, const array_view<ValT> &rhs) {
	return rhs == lhs;
}
template <typename T REQUIRES_NotArrayView(T), typename ValT>
constexpr bool operator!=(const T &lhs, const array_view<ValT> &rhs) {
	return rhs != lhs;
}
template <typename T REQUIRES_NotArrayView(T), typename ValT>
constexpr bool operator<(const T &lhs, const array_view<ValT> &rhs) {
	return rhs > lhs;
}
template <typename T REQUIRES_NotArrayView(T), typename ValT>
constexpr bool operator>(const T &lhs, const array_view<ValT> &rhs) {
	return rhs < lhs;
}
template <typename T REQUIRES_NotArrayView(T), typename ValT>
constexpr bool operator<=(const T &lhs, const array_view<ValT> &rhs) {
	return rhs >= lhs;
}
template <typename T REQUIRES_NotArrayView(T), typename ValT>
constexpr bool operator>=(const T &lhs, const array_view<ValT> &rhs) {
	return rhs <= lhs;
}



namespace details {

template <typename T>
using value_type_t = typename T::value_type;
template <typename T, typename = void>
struct has_value_type : std::false_type { };
template <typename T>
struct has_value_type<T, std::void_t<value_type_t<T>>> : std::true_type { };

template <typename T>
constexpr auto view_of(T &x) noexcept {
	using ValT = typename T::value_type;
	using CValT = std::add_const_t<ValT>;
	if constexpr (details::is_view_like<T, ValT>::value)
		return array_view<ValT> {x};
	else if constexpr (details::is_view_like<T, CValT>::value)
		return array_view<CValT> {x};
	else
		return array_view<T> {&x, 1};
}

} // namespace details

template <typename T>
constexpr auto view_of(T &x) noexcept {
	if constexpr (details::has_value_type<T>::value)
		return details::view_of(x);
	else if constexpr (std::is_array_v<T>)
		return array_view<std::remove_extent_t<T>> {x};
	else
		return array_view<T> {&x, 1};
}

template <typename ValT>
constexpr array_view<const std::byte>
as_bytes(array_view<ValT> view) noexcept {
	return {
		reinterpret_cast<const std::byte *>(view.data()),
		sizeof(ValT) * view.size(),
	};
}
template <typename ValT,
	std::enable_if_t<!std::is_const_v<ValT>, int> = 0>
constexpr array_view<std::byte>
as_writeable_bytes(array_view<ValT> view) noexcept {
	return {
		reinterpret_cast<std::byte *>(view.data()),
		sizeof(ValT) * view.size(),
	};
}

template <typename T>
constexpr auto byte_view(T &x) noexcept {
	auto view = view_of(x);
	if constexpr (std::is_const_v<typename decltype(view)::value_type>)
		return as_bytes(view);
	else
		return as_writeable_bytes(view);
}



template <>
class array_view<void>;
template <>
class array_view<const void>;
template <>
class array_view<volatile void>;
template <>
class array_view<const volatile void>;
