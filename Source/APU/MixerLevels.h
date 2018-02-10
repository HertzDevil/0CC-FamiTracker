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
	int Offset(chan_id_t ChanID, int Value);
	double CalcPin() const;

private:
	int sq1_ = 0;
	int sq2_ = 0;
};



struct stLevels2A03TND {
	int Offset(chan_id_t ChanID, int Value);
	double CalcPin() const;

private:
	int tri_ = 0;
	int noi_ = 0;
	int dmc_ = 0;
};



struct stLevelsMono {
	int Offset(chan_id_t ChanID, int Value);
	double CalcPin() const;

private:
	int lvl_ = 0;
};



template <chan_id_t... ChanIDs>
struct stLevelsLinear {
	int Offset(chan_id_t ChanID, int Value) {
		return Offset(ChanID, Value,
			std::integer_sequence<unsigned, value_cast(ChanIDs)...> { },
			std::make_index_sequence<sizeof...(ChanIDs)> { });
	}

	double CalcPin() const {
		return tot_;
	}

private:
	int Offset(chan_id_t ChanID, int Value, std::integer_sequence<unsigned>, std::index_sequence<>) {
		return 0;
	}

	template <unsigned I, unsigned... Is, std::size_t J, std::size_t... Js>
	int Offset(chan_id_t ChanID, int Value, std::integer_sequence<unsigned, I, Is...>, std::index_sequence<J, Js...>) {
		if (ChanID == (chan_id_t)I) {
			tot_ += Value;
			return lvl_[J] += Value;
		}
		return Offset(ChanID, Value,
			std::integer_sequence<unsigned, Is...> { },
			std::index_sequence<Js...> { });
	}

private:
	int lvl_[sizeof...(ChanIDs)] = { };
	int tot_ = 0;
};

using stLevelsVRC6 = stLevelsLinear<chan_id_t::VRC6_PULSE1, chan_id_t::VRC6_PULSE2, chan_id_t::VRC6_SAWTOOTH>;
using stLevelsFDS = stLevelsLinear<chan_id_t::FDS>;
using stLevelsMMC5 = stLevelsLinear<chan_id_t::MMC5_SQUARE1, chan_id_t::MMC5_SQUARE2, chan_id_t::MMC5_VOICE>;
using stLevelsN163 = stLevelsLinear<chan_id_t::N163_CH1, chan_id_t::N163_CH2, chan_id_t::N163_CH3, chan_id_t::N163_CH4,
	chan_id_t::N163_CH5, chan_id_t::N163_CH6, chan_id_t::N163_CH7, chan_id_t::N163_CH8>;
using stLevelsS5B = stLevelsLinear<chan_id_t::S5B_CH1, chan_id_t::S5B_CH2, chan_id_t::S5B_CH3>;
