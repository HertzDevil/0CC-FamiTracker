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

#include <cstdint>
#include <tuple>

// // //  Color macros

constexpr std::uint8_t GetR(unsigned x) noexcept {
	return x & 0xFF;
}

constexpr std::uint8_t GetG(unsigned x) noexcept {
	return (x >> 8) & 0xFF;
}

constexpr std::uint8_t GetB(unsigned x) noexcept {
	return (x >> 16) & 0xFF;
}

template <typename T>
constexpr std::uint8_t ClipColorValue(T x) noexcept {
	return static_cast<std::uint8_t>(x < T(0) ? T(0) : (x > T(255) ? T(255) : x));
}

constexpr auto GetRGB(unsigned x) noexcept {
	return std::make_tuple(GetR(x), GetG(x), GetB(x));
}

constexpr unsigned MakeRGB(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept {
	return r | (g << 8) | (b << 16);
}

inline constexpr unsigned BLACK = MakeRGB(0, 0, 0);

inline constexpr unsigned WHITE = MakeRGB(255, 255, 255);

constexpr unsigned GREY(std::uint8_t v) noexcept {
	return MakeRGB(v, v, v);
}

constexpr unsigned Invert(unsigned c) noexcept {
	return MakeRGB(0xFF - GetR(c), 0xFF - GetG(c), 0xFF - GetB(c));
}

constexpr unsigned BlendColors(unsigned c1, double w1, unsigned c2, double w2) noexcept {
	auto [r1, g1, b1] = GetRGB(c1);
	auto [r2, g2, b2] = GetRGB(c2);
	return MakeRGB(
		ClipColorValue((r1 * w1 + r2 * w2) / (w1 + w2)),
		ClipColorValue((g1 * w1 + g2 * w2) / (w1 + w2)),
		ClipColorValue((b1 * w1 + b2 * w2) / (w1 + w2)));
}

constexpr unsigned BLEND(unsigned c1, unsigned c2, double level) noexcept {
	return BlendColors(c1, level, c2, 1. - level);
}

constexpr unsigned DIM(unsigned c, double l) noexcept {
	return BLEND(c, BLACK, l);
}

constexpr unsigned INTENSITY(unsigned c) noexcept {
	auto [r, g, b] = GetRGB(c);
	return (r + g + b) / 3;
}
