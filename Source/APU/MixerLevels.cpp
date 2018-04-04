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

#include "APU/MixerLevels.h"
#include "APU/Types.h"

namespace {

const double AMP_2A03 = 400.0;

} // namespace

int stLevels2A03SS::Offset(apu_subindex_t subindex, int val) {
	switch (subindex) {
	case apu_subindex_t::pulse1: return sq1_ += val;
	case apu_subindex_t::pulse2: return sq2_ += val;
	}
	return 0;
}

double stLevels2A03SS::CalcPin() const {
#ifdef LINEAR_MIXING
	double SumL = (sq1_.Left  + sq2_.Left ) * 0.00752 * InternalVol;
	double SumR = (sq1_.Right + sq2_.Right) * 0.00752 * InternalVol;
#endif
	if ((sq1_ + sq2_) > 0)
		return AMP_2A03 * 95.88 / (100.0 + 8128.0 / (sq1_ + sq2_));
	return 0.;
}



int stLevels2A03TND::Offset(apu_subindex_t subindex, int val) {
	switch (subindex) {
	case apu_subindex_t::triangle: return tri_ += val;
	case apu_subindex_t::noise:    return noi_ += val;
	case apu_subindex_t::dpcm:     return dmc_ += val;
	}
	return 0;
}

double stLevels2A03TND::CalcPin() const {
#ifdef LINEAR_MIXING
	double SumL = (0.00851 * tri_.Left  + 0.00494 * noi_.Left  + 0.00335 * dmc_.Left ) * InternalVol;
	double SumR = (0.00851 * tri_.Right + 0.00494 * noi_.Right + 0.00335 * dmc_.Right) * InternalVol;
#endif
	if ((tri_ + noi_ + dmc_) > 0)
		return AMP_2A03 * 159.79 / (100.0 + 1.0 / (tri_ / 8227.0 + noi_ / 12241.0 + dmc_ / 22638.0));
	return 0.;
}
