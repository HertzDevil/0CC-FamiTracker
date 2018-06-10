/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * 0CC-FamiTracker is (C) 2014-2018 HertzDevil
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public License Version 2, as described below:
 *
 * This file is free software: you may copy, redistribute and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/. */


#pragma once

#include <cstdint>
#include "ft0cc/cpputil/enum_traits.hpp"

namespace ft0cc::doc {

enum class effect_type : std::uint8_t {
	SPEED = 1,
	JUMP,
	SKIP,
	HALT,
	VOLUME,
	PORTAMENTO,
	PORTAOFF, // unused!!
	SWEEPUP,
	SWEEPDOWN,
	ARPEGGIO,
	VIBRATO,
	TREMOLO,
	PITCH,
	DELAY,
	DAC,
	PORTA_UP,
	PORTA_DOWN,
	DUTY_CYCLE,
	SAMPLE_OFFSET,
	SLIDE_UP,
	SLIDE_DOWN,
	VOLUME_SLIDE,
	NOTE_CUT,
	RETRIGGER,
	DELAYED_VOLUME,
	FDS_MOD_DEPTH,
	FDS_MOD_SPEED_HI,
	FDS_MOD_SPEED_LO,
	DPCM_PITCH,
	SUNSOFT_ENV_TYPE,
	SUNSOFT_ENV_HI,
	SUNSOFT_ENV_LO,
	SUNSOFT_NOISE,
	VRC7_PORT,
	VRC7_WRITE,
	NOTE_RELEASE,
	GROOVE,
	TRANSPOSE,
	N163_WAVE_BUFFER,
	FDS_VOLUME,
	FDS_MOD_BIAS,
	SN76489_CONTROL,
//	TARGET_VOLUME_SLIDE,
/*
	VRC7_MODULATOR,
	VRC7_CARRIER,
	VRC7_LEVELS,
*/
	min = SPEED, max = FDS_MOD_BIAS, none = 0,
};

} // namespace ft0cc::doc

ENABLE_ENUM_CATEGORY(ft0cc::doc::effect_type, enum_standard);
