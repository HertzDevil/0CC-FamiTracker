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

#include "APU/Types.h"
#include <string_view>

constexpr std::string_view GetChannelFullName(chan_id_t ch) noexcept {
	using namespace std::string_view_literals;

	constexpr std::string_view NAMES[] = {
		"Pulse 1"sv, "Pulse 2"sv, "Triangle"sv, "Noise"sv, "DPCM"sv,
		"VRC6 Pulse 1"sv, "VRC6 Pulse 2"sv, "Sawtooth"sv,
		"MMC5 Pulse 1"sv, "MMC5 PUlse 2"sv, "MMC5 PCM"sv,
		"Namco 1"sv, "Namco 2"sv, "Namco 3"sv, "Namco 4"sv, "Namco 5"sv, "Namco 6"sv, "Namco 7"sv, "Namco 8"sv,
		"FDS"sv,
		"FM Channel 1"sv, "FM Channel 2"sv, "FM Channel 3"sv, "FM Channel 4"sv, "FM Channel 5"sv, "FM Channel 6"sv,
		"5B Square 1"sv, "5B Square 2"sv, "5B Square 3"sv,
	};

	return ch < chan_id_t::COUNT ? NAMES[value_cast(ch)] : "(N/A)";
}

constexpr std::string_view GetChannelShortName(chan_id_t ch) noexcept {
	using namespace std::string_view_literals;

	constexpr std::string_view NAMES[] = {
		"PU1"sv, "PU2"sv, "TRI"sv, "NOI"sv, "DMC"sv,
		 "V1"sv,  "V2"sv, "SAW"sv,
		"PU3"sv, "PU4"sv, "PCM"sv,
		 "N1"sv,  "N2"sv,  "N3"sv,  "N4"sv,  "N5"sv,  "N6"sv,  "N7"sv,  "N8"sv,
		"FDS"sv,
		"FM1"sv, "FM2"sv, "FM3"sv, "FM4"sv, "FM5"sv, "FM6"sv,
		"5B1"sv, "5B2"sv, "5B3"sv,
	};

	return ch < chan_id_t::COUNT ? NAMES[value_cast(ch)] : "(N/A)";
}
