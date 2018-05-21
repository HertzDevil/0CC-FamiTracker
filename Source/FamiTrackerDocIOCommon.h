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

#include "ft0cc/doc/effect_type.hpp"
#include <array>

namespace compat {

// // // helper function for effect conversion

template <ft0cc::doc::effect_type From, ft0cc::doc::effect_type To>
struct conv_pair { };

using EffTable = std::array<ft0cc::doc::effect_type, enum_count<ft0cc::doc::effect_type>() + 1>;

template <ft0cc::doc::effect_type... Froms, ft0cc::doc::effect_type... Tos>
constexpr std::pair<EffTable, EffTable>
MakeEffectConversion(conv_pair<Froms, Tos>...) noexcept {
	EffTable forward = { };
	forward[value_cast(ft0cc::doc::effect_type::none)] = ft0cc::doc::effect_type::none;
	for (auto fx : enum_values<ft0cc::doc::effect_type>())
		forward[value_cast(fx)] = fx;

	EffTable backward = forward;
	((forward[value_cast(Froms)] = Tos), ...);
	((backward[value_cast(Tos)] = Froms), ...);
	return std::make_pair(forward, backward);
}

constexpr auto EFF_CONVERSION_050 = MakeEffectConversion(
//	conv_pair<ft0cc::doc::effect_type::SUNSOFT_ENV_LO,		ft0cc::doc::effect_type::SUNSOFT_ENV_TYPE>(),
//	conv_pair<ft0cc::doc::effect_type::SUNSOFT_ENV_TYPE,	ft0cc::doc::effect_type::SUNSOFT_ENV_LO>(),
	conv_pair<ft0cc::doc::effect_type::SUNSOFT_NOISE,		ft0cc::doc::effect_type::NOTE_RELEASE>(),
	conv_pair<ft0cc::doc::effect_type::VRC7_PORT,			ft0cc::doc::effect_type::GROOVE>(),
	conv_pair<ft0cc::doc::effect_type::VRC7_WRITE,			ft0cc::doc::effect_type::TRANSPOSE>(),
	conv_pair<ft0cc::doc::effect_type::NOTE_RELEASE,		ft0cc::doc::effect_type::N163_WAVE_BUFFER>(),
	conv_pair<ft0cc::doc::effect_type::GROOVE,				ft0cc::doc::effect_type::FDS_VOLUME>(),
	conv_pair<ft0cc::doc::effect_type::TRANSPOSE,			ft0cc::doc::effect_type::FDS_MOD_BIAS>(),
	conv_pair<ft0cc::doc::effect_type::N163_WAVE_BUFFER,	ft0cc::doc::effect_type::SUNSOFT_NOISE>(),
	conv_pair<ft0cc::doc::effect_type::FDS_VOLUME,			ft0cc::doc::effect_type::VRC7_PORT>(),
	conv_pair<ft0cc::doc::effect_type::FDS_MOD_BIAS,		ft0cc::doc::effect_type::VRC7_WRITE>()
);

} // namespace compat
