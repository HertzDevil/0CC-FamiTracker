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

/*!
	\brief Simple class for integer range generator. A single iteration object is stateless, so
	every use in a range-for loop always iterates through all values.
*/
template <class T>
struct CIntRange {
private:
	struct it;
public:
	constexpr CIntRange() = default;
	constexpr CIntRange(T b, T e) : b_(b), e_(e) { }
	constexpr T operator[](size_t x) const {
		return b_ + x;
	}
	template <class T_>
	friend constexpr bool operator==(const CIntRange<T_> &lhs, const CIntRange<T_> &rhs) {
		return lhs.b_ == rhs.b_ && lhs.e_ == rhs.e_;
	}
	template <class T_>
	friend constexpr bool operator!=(const CIntRange<T_> &lhs, const CIntRange<T_> &rhs) {
		return lhs.b_ != rhs.b_ || lhs.e_ != rhs.e_;
	}
	template <class T_> // subset of
	friend constexpr bool operator<=(const CIntRange<T_> &lhs, const CIntRange<T_> &rhs) {
		return lhs.b_ >= rhs.b_ && lhs.e_ <= rhs.e_;
	}
	template <class T_> // superset of
	friend constexpr bool operator>=(const CIntRange<T_> &lhs, const CIntRange<T_> &rhs) {
		return lhs.b_ <= rhs.b_ && lhs.e_ >= rhs.e_;
	}
	template <class T_> // strict subset of
	friend constexpr bool operator<(const CIntRange<T_> &lhs, const CIntRange<T_> &rhs) {
		return lhs <= rhs && lhs != rhs;
	}
	template <class T_> // strict superset of
	friend constexpr bool operator>(const CIntRange<T_> &lhs, const CIntRange<T_> &rhs) {
		return lhs >= rhs && lhs != rhs;
	}
	template <class T_> // intersection
	friend constexpr CIntRange<T_> operator&(const CIntRange<T_> &lhs, const CIntRange<T_> &rhs) {
		return CIntRange<T_>(lhs.b_ > rhs.b_ ? lhs.b_ : rhs.b_,
							 lhs.e_ < rhs.e_ ? lhs.e_ : rhs.e_);
	}
	constexpr CIntRange &operator+=(T rhs) {
		b_ += rhs;
		e_ += rhs;
		return *this;
	}
	constexpr CIntRange &operator-=(T rhs) {
		b_ -= rhs;
		e_ -= rhs;
		return *this;
	}
	template <class T_> // shift
	friend constexpr CIntRange<T_> operator+(const CIntRange<T_> &lhs, T rhs) {
		CIntRange<T_> r {lhs};
		r += rhs;
		return r;
	}
	template <class T_> // shift
	friend constexpr CIntRange<T_> operator-(const CIntRange<T_> &lhs, T rhs) {
		CIntRange<T_> r {lhs};
		r -= rhs;
		return r;
	}
	template <class T_> // shift
	friend constexpr CIntRange<T_> operator+(T lhs, const CIntRange<T_> &rhs) {
		return rhs + lhs;
	}
	template <class T_> // shift
	friend constexpr CIntRange<T_> operator-(T lhs, const CIntRange<T_> &rhs) {
		return rhs - lhs;
	}
	constexpr CIntRange &operator++() {
		++b_;
		++e_;
		return *this;
	}
	constexpr CIntRange &operator--() {
		--b_;
		--e_;
		return *this;
	}
	CIntRange operator++(int) {
		return CIntRange<T>(b_++, e_++);
	}
	CIntRange operator--(int) {
		return CIntRange<T>(b_--, e_--);
	}
	constexpr it begin() const { return it(b_); }
	constexpr it end() const { return it(e_); }
	T b_, e_;
private:
	struct it {
		explicit constexpr it(T x) : _x(x) { }
		constexpr T operator *() const { return _x; }
		it &operator++() { ++_x; return *this; }
		constexpr bool operator!=(const it &other) const { return _x != other._x; }
	private:
		T _x;
	};
};

template <class T, class U>
constexpr CIntRange<std::common_type_t<T, U>>
make_int_range(T b, U e) {
	return CIntRange<std::common_type_t<T, U>>(b, e);
}
