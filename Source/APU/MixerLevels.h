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
#include <utility>
#include "APU/Types.h"

//#define LINEAR_MIXING

struct stLevels2A03SS {
	using subindex_t = apu_subindex_t;
	int Offset(apu_subindex_t subindex, int val);
	double CalcPin() const;

private:
	int sq1_ = 0;
	int sq2_ = 0;
};



struct stLevels2A03TND {
	using subindex_t = apu_subindex_t;
	int Offset(apu_subindex_t subindex, int val);
	double CalcPin() const;

private:
	int tri_ = 0;
	int noi_ = 0;
	int dmc_ = 0;
};



template <typename EnumT, EnumT... Subindices>
struct stLevelsLinear {
	using subindex_t = EnumT;

private:
	using T2 = std::underlying_type_t<EnumT>;

public:
	int Offset(EnumT ChanID, int val) {
		return Offset(ChanID, val,
			std::integer_sequence<T2, value_cast(Subindices)...> { },
			std::make_index_sequence<sizeof...(Subindices)> { });
	}

	double CalcPin() const {
		return tot_;
	}

private:
	int Offset(EnumT ChanID, int val, std::integer_sequence<T2>, std::index_sequence<>) {
		return 0;
	}

	template <T2 I, T2... Is, std::size_t J, std::size_t... Js>
	int Offset(EnumT ChanID, int val, std::integer_sequence<T2, I, Is...>, std::index_sequence<J, Js...>) {
		if (value_cast(ChanID) == I) {
			tot_ += val;
			return lvl_[J] += val;
		}
		return Offset(ChanID, val,
			std::integer_sequence<T2, Is...> { },
			std::index_sequence<Js...> { });
	}

private:
	int lvl_[sizeof...(Subindices)] = { };
	int tot_ = 0;
};

using stLevelsVRC6 = stLevelsLinear<vrc6_subindex_t,
	vrc6_subindex_t::pulse1, vrc6_subindex_t::pulse2, vrc6_subindex_t::sawtooth>;
using stLevelsFDS = stLevelsLinear<fds_subindex_t,
	fds_subindex_t::wave>;
using stLevelsMMC5 = stLevelsLinear<mmc5_subindex_t,
	mmc5_subindex_t::pulse1, mmc5_subindex_t::pulse2, mmc5_subindex_t::pcm>;
using stLevelsN163 = stLevelsLinear<n163_subindex_t,
	n163_subindex_t::ch1, n163_subindex_t::ch2, n163_subindex_t::ch3, n163_subindex_t::ch4,
	n163_subindex_t::ch5, n163_subindex_t::ch6, n163_subindex_t::ch7, n163_subindex_t::ch8>;
using stLevelsS5B = stLevelsLinear<s5b_subindex_t,
	s5b_subindex_t::square1, s5b_subindex_t::square2, s5b_subindex_t::square3>;
