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
struct iter_range {
	template <typename U>
	explicit iter_range(U&& x) noexcept : x_(std::forward<U>(x)) { }

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
iter_range<T> values(T&& x) noexcept {
	return iter_range<T> {std::forward<T>(x)};
}
