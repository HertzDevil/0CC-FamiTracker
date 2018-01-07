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
using EffTable = std::array<effect_t, EF_COUNT>;
constexpr std::pair<EffTable, EffTable>
MakeEffectConversion(std::initializer_list<std::pair<effect_t, effect_t>> List) {
	EffTable forward = { }, backward = { };
	for (int i = 0; i < EF_COUNT; ++i)
		forward[i] = backward[i] = static_cast<effect_t>(i);
	for (const auto &p : List) {
		forward[p.first] = p.second;
		backward[p.second] = p.first;
	}
	return std::make_pair(forward, backward);
}

const auto EFF_CONVERSION_050 = MakeEffectConversion({
//	{EF_SUNSOFT_ENV_LO,		EF_SUNSOFT_ENV_TYPE},
//	{EF_SUNSOFT_ENV_TYPE,	EF_SUNSOFT_ENV_LO},
	{EF_SUNSOFT_NOISE,		EF_NOTE_RELEASE},
	{EF_VRC7_PORT,			EF_GROOVE},
	{EF_VRC7_WRITE,			EF_TRANSPOSE},
	{EF_NOTE_RELEASE,		EF_N163_WAVE_BUFFER},
	{EF_GROOVE,				EF_FDS_VOLUME},
	{EF_TRANSPOSE,			EF_FDS_MOD_BIAS},
	{EF_N163_WAVE_BUFFER,	EF_SUNSOFT_NOISE},
	{EF_FDS_VOLUME,			EF_VRC7_PORT},
	{EF_FDS_MOD_BIAS,		EF_VRC7_WRITE},
});

} // namespace compat
