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

#include "FamiTrackerTypes.h"
#include <array>

namespace compat {

// // // helper function for effect conversion

template <effect_t From, effect_t To>
struct conv_pair { };

using EffTable = std::array<effect_t, enum_count<effect_t>() + 1>;

template <effect_t... Froms, effect_t... Tos>
constexpr std::pair<EffTable, EffTable>
MakeEffectConversion(conv_pair<Froms, Tos>...) noexcept {
	EffTable forward = { };
	forward[0] = effect_t::none;
	for (auto fx : enum_values<effect_t>())
		forward[value_cast(fx)] = fx;

	EffTable backward = forward;
	((forward[value_cast(Froms)] = Tos), ...);
	((backward[value_cast(Tos)] = Froms), ...);
	return std::make_pair(forward, backward);
}

constexpr auto EFF_CONVERSION_050 = MakeEffectConversion(
//	conv_pair<effect_t::SUNSOFT_ENV_LO,		effect_t::SUNSOFT_ENV_TYPE>(),
//	conv_pair<effect_t::SUNSOFT_ENV_TYPE,	effect_t::SUNSOFT_ENV_LO>(),
	conv_pair<effect_t::SUNSOFT_NOISE,		effect_t::NOTE_RELEASE>(),
	conv_pair<effect_t::VRC7_PORT,			effect_t::GROOVE>(),
	conv_pair<effect_t::VRC7_WRITE,			effect_t::TRANSPOSE>(),
	conv_pair<effect_t::NOTE_RELEASE,		effect_t::N163_WAVE_BUFFER>(),
	conv_pair<effect_t::GROOVE,				effect_t::FDS_VOLUME>(),
	conv_pair<effect_t::TRANSPOSE,			effect_t::FDS_MOD_BIAS>(),
	conv_pair<effect_t::N163_WAVE_BUFFER,	effect_t::SUNSOFT_NOISE>(),
	conv_pair<effect_t::FDS_VOLUME,			effect_t::VRC7_PORT>(),
	conv_pair<effect_t::FDS_MOD_BIAS,		effect_t::VRC7_WRITE>()
);

} // namespace compat
