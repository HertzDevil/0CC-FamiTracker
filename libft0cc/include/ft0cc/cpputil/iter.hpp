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
#include <utility>

template <typename T>
struct values_range {
	template <typename U>
	explicit values_range(U&& x) noexcept : x_(std::forward<U>(x)) { }

	auto begin() const {
		return std::begin(x_);
	}
	auto end() const {
		return std::end(x_);
	}

private:
	T x_;
};

template <typename T>
values_range<T> values(T&& x) noexcept {
	return values_range<T> {std::forward<T>(x)};
}



template <typename B, typename E>
struct iter_range {
	iter_range(B b, E e) noexcept : b_(std::move(b)), e_(std::move(e)) { }

	B begin() const {
		return b_;
	}

	E end() const {
		return e_;
	}

private:
	B b_;
	E e_;
};

template <typename B, typename E>
iter_range<B, E> iter(B b, E e) noexcept {
	return {std::move(b), std::move(e)};
}



template <typename B>
struct with_index_iterator {
	explicit with_index_iterator(B b) : b_(std::move(b)) { }

	auto operator*() const {
		return std::pair<decltype(*b_), decltype(i_)> {*b_, i_};
	}

	with_index_iterator &operator++() {
		++b_;
		++i_;
		return *this;
	}

	template <typename E>
	bool operator!=(const E &e) const {
		return b_ != e;
	}

private:
	B b_;
	std::size_t i_ = 0;
};

template <typename T>
auto with_index(T&& x) {
	return iter(with_index_iterator {std::begin(x)}, std::end(x));
}
