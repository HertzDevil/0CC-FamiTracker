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

#include "WaveformGenerator.h"
#include <cmath>

Waves::CPulse::CPulse(double duty) : t_(1. - duty) {
}

double Waves::CPulse::operator()(double phase) const {
	return phase >= t_ ? 1. : -1.;
}



double Waves::CTriangle::operator()(double phase) const {
	return phase >= .5 ? (3. - phase * 4.) : (-1. + phase * 4.);
}



double Waves::CSawtooth::operator()(double phase) const {
	return -1. + phase * 2.;
}



double Waves::CSine::operator()(double phase) const {
	const double PI2 = 3.14159265358979323846 * 2.;
	return std::sin(PI2 * phase);
}
