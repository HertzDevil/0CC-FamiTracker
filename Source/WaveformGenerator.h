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

#include <cstddef>
#include <algorithm>

namespace Waves {

struct CPulse {
	explicit CPulse(double duty);

	double operator()(double phase) const;

private:
	double t_;
};

struct CTriangle {
	double operator()(double phase) const;
};

struct CSawtooth {
	double operator()(double phase) const;
};

struct CSine {
	double operator()(double phase) const;
};

} // namespace Waves

template <typename T, typename F>
auto GenerateWaveform(T wave, std::size_t len, std::size_t steps, F f) {
	for (std::size_t i = 0; i < len; ++i) {
		double phase = (static_cast<double>(i) + .5) / len;
		double val = (wave(phase) + 1.) / 2. * steps;
		f(static_cast<unsigned>(std::clamp(val, 0., static_cast<double>(steps) - 1.)), i);
	}
}

