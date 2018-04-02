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

#include "SoundChipTypeImpl.h"
#include "APU/Types.h"

#include "APU/2A03.h"
#include "APU/VRC6.h"
#include "APU/VRC7.h"
#include "APU/FDS.h"
#include "APU/MMC5.h"
#include "APU/N163.h"
#include "APU/S5B.h"

#include "Channels2A03.h"
#include "ChannelsVRC6.h"
#include "ChannelsVRC7.h"
#include "ChannelsFDS.h"
#include "ChannelsMMC5.h"
#include "ChannelsN163.h"
#include "ChannelsS5B.h"

#include "ChipHandler.h"
#include "ChipHandlerVRC7.h"
#include "ChipHandlerS5B.h"

// implementations of built-in sound chip types

sound_chip_t CSoundChipType2A03::GetID() const {
	return sound_chip_t::APU;
}

std::size_t CSoundChipType2A03::GetSupportedChannelCount() const {
	return 5;
}

chan_id_t CSoundChipType2A03::GetFirstChannelID() const {
	return chan_id_t::SQUARE1;
}

std::string_view CSoundChipType2A03::GetShortName() const {
	return "2A03";
}

std::string_view CSoundChipType2A03::GetFullName() const {
	return "Nintendo 2A03";
}

std::string_view CSoundChipType2A03::GetShortChannelName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {"PU1"sv, "PU2"sv, "TRI"sv, "NOI"sv, "DMC"sv};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::string_view CSoundChipType2A03::GetFullChannelName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {
		"Pulse 1"sv,
		"Pulse 2"sv,
		"Triangle"sv,
		"Noise"sv,
		"DPCM"sv,
	};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::unique_ptr<CSoundChip> CSoundChipType2A03::MakeSoundDriver(CMixer &mixer) const {
	return std::make_unique<C2A03>(mixer);
}

std::unique_ptr<CChipHandler> CSoundChipType2A03::MakeChipHandler() const {
	auto chip = std::make_unique<CChipHandler>();
	chip->AddChannelHandler(std::make_unique<C2A03Square>(chan_id_t::SQUARE1));
	chip->AddChannelHandler(std::make_unique<C2A03Square>(chan_id_t::SQUARE2));
	chip->AddChannelHandler(std::make_unique<CTriangleChan>(chan_id_t::TRIANGLE));
	chip->AddChannelHandler(std::make_unique<CNoiseChan>(chan_id_t::NOISE));
	chip->AddChannelHandler(std::make_unique<CDPCMChan>(chan_id_t::DPCM));
	return chip;
}



sound_chip_t CSoundChipTypeVRC6::GetID() const {
	return sound_chip_t::VRC6;
}

std::size_t CSoundChipTypeVRC6::GetSupportedChannelCount() const {
	return 3;
}

chan_id_t CSoundChipTypeVRC6::GetFirstChannelID() const {
	return chan_id_t::VRC6_PULSE1;
}

std::string_view CSoundChipTypeVRC6::GetShortName() const {
	return "VRC6";
}

std::string_view CSoundChipTypeVRC6::GetFullName() const {
	return "Konami VRC6";
}

std::string_view CSoundChipTypeVRC6::GetShortChannelName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {"V1"sv, "V2"sv, "SAW"sv};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::string_view CSoundChipTypeVRC6::GetFullChannelName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {
		"VRC6 Pulse 1"sv,
		"VRC6 Pulse 2"sv,
		"Sawtooth"sv,
	};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::unique_ptr<CSoundChip> CSoundChipTypeVRC6::MakeSoundDriver(CMixer &mixer) const {
	return std::make_unique<CVRC6>(mixer);
}

std::unique_ptr<CChipHandler> CSoundChipTypeVRC6::MakeChipHandler() const {
	auto chip = std::make_unique<CChipHandler>();
	chip->AddChannelHandler(std::make_unique<CVRC6Square>(chan_id_t::VRC6_PULSE1));
	chip->AddChannelHandler(std::make_unique<CVRC6Square>(chan_id_t::VRC6_PULSE2));
	chip->AddChannelHandler(std::make_unique<CVRC6Sawtooth>(chan_id_t::VRC6_SAWTOOTH));
	return chip;
}



sound_chip_t CSoundChipTypeVRC7::GetID() const {
	return sound_chip_t::VRC7;
}

std::size_t CSoundChipTypeVRC7::GetSupportedChannelCount() const {
	return 6;
}

chan_id_t CSoundChipTypeVRC7::GetFirstChannelID() const {
	return chan_id_t::VRC7_CH1;
}

std::string_view CSoundChipTypeVRC7::GetShortName() const {
	return "VRC7";
}

std::string_view CSoundChipTypeVRC7::GetFullName() const {
	return "Konami VRC7";
}

std::string_view CSoundChipTypeVRC7::GetShortChannelName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {"FM1"sv, "FM2"sv, "FM3"sv, "FM4"sv, "FM5"sv, "FM6"sv};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::string_view CSoundChipTypeVRC7::GetFullChannelName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {
		"FM Channel 1"sv,
		"FM Channel 2"sv,
		"FM Channel 3"sv,
		"FM Channel 4"sv,
		"FM Channel 5"sv,
		"FM Channel 6"sv,
	};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::unique_ptr<CSoundChip> CSoundChipTypeVRC7::MakeSoundDriver(CMixer &mixer) const {
	return std::make_unique<CVRC7>(mixer);
}

std::unique_ptr<CChipHandler> CSoundChipTypeVRC7::MakeChipHandler() const {
	auto chip = std::make_unique<CChipHandlerVRC7>();
	for (unsigned i = 0; i < MAX_CHANNELS_VRC7; ++i)
		chip->AddChannelHandler(std::make_unique<CChannelHandlerVRC7>(MakeChannelIndex(GetID(), i), *chip));
	return chip;
}



sound_chip_t CSoundChipTypeFDS::GetID() const {
	return sound_chip_t::FDS;
}

std::size_t CSoundChipTypeFDS::GetSupportedChannelCount() const {
	return 1;
}

chan_id_t CSoundChipTypeFDS::GetFirstChannelID() const {
	return chan_id_t::FDS;
}

std::string_view CSoundChipTypeFDS::GetShortName() const {
	return "FDS";
}

std::string_view CSoundChipTypeFDS::GetFullName() const {
	return "Nintendo FDS";
}

std::string_view CSoundChipTypeFDS::GetShortChannelName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {"FDS"sv};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::string_view CSoundChipTypeFDS::GetFullChannelName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {
		"FDS"sv,
	};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::unique_ptr<CSoundChip> CSoundChipTypeFDS::MakeSoundDriver(CMixer &mixer) const {
	return std::make_unique<CFDS>(mixer);
}

std::unique_ptr<CChipHandler> CSoundChipTypeFDS::MakeChipHandler() const {
	auto chip = std::make_unique<CChipHandler>();
	chip->AddChannelHandler(std::make_unique<CChannelHandlerFDS>(chan_id_t::FDS));
	return chip;
}



sound_chip_t CSoundChipTypeMMC5::GetID() const {
	return sound_chip_t::MMC5;
}

std::size_t CSoundChipTypeMMC5::GetSupportedChannelCount() const {
	return 3; // 2
}

chan_id_t CSoundChipTypeMMC5::GetFirstChannelID() const {
	return chan_id_t::MMC5_SQUARE1;
}

std::string_view CSoundChipTypeMMC5::GetShortName() const {
	return "MMC5";
}

std::string_view CSoundChipTypeMMC5::GetFullName() const {
	return "Nintendo MMC5";
}

std::string_view CSoundChipTypeMMC5::GetShortChannelName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {"PU3"sv, "PU4"sv, "PCM"sv};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::string_view CSoundChipTypeMMC5::GetFullChannelName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {
		"MMC5 Pulse 1"sv,
		"MMC5 Pulse 2"sv,
		"MMC5 PCM"sv,
	};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::unique_ptr<CSoundChip> CSoundChipTypeMMC5::MakeSoundDriver(CMixer &mixer) const {
	return std::make_unique<CMMC5>(mixer);
}

std::unique_ptr<CChipHandler> CSoundChipTypeMMC5::MakeChipHandler() const {
	auto chip = std::make_unique<CChipHandler>();
	chip->AddChannelHandler(std::make_unique<CChannelHandlerMMC5>(chan_id_t::MMC5_SQUARE1));
	chip->AddChannelHandler(std::make_unique<CChannelHandlerMMC5>(chan_id_t::MMC5_SQUARE2));
	return chip;
}



sound_chip_t CSoundChipTypeN163::GetID() const {
	return sound_chip_t::N163;
}

std::size_t CSoundChipTypeN163::GetSupportedChannelCount() const {
	return 8;
}

chan_id_t CSoundChipTypeN163::GetFirstChannelID() const {
	return chan_id_t::N163_CH1;
}

std::string_view CSoundChipTypeN163::GetShortName() const {
	return "N163";
}

std::string_view CSoundChipTypeN163::GetFullName() const {
	return "Namco 163";
}

std::string_view CSoundChipTypeN163::GetShortChannelName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {"N1"sv, "N2"sv, "N3"sv, "N4"sv, "N5"sv, "N6"sv, "N7"sv, "N8"sv};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::string_view CSoundChipTypeN163::GetFullChannelName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {
		"Namco 1"sv,
		"Namco 2"sv,
		"Namco 3"sv,
		"Namco 4"sv,
		"Namco 5"sv,
		"Namco 6"sv,
		"Namco 7"sv,
		"Namco 8"sv,
	};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::unique_ptr<CSoundChip> CSoundChipTypeN163::MakeSoundDriver(CMixer &mixer) const {
	return std::make_unique<CN163>(mixer);
}

std::unique_ptr<CChipHandler> CSoundChipTypeN163::MakeChipHandler() const {
	auto chip = std::make_unique<CChipHandler>();
	for (unsigned i = 0; i < MAX_CHANNELS_N163; ++i)
		chip->AddChannelHandler(std::make_unique<CChannelHandlerN163>(MakeChannelIndex(GetID(), i)));
	return chip;
}



sound_chip_t CSoundChipTypeS5B::GetID() const {
	return sound_chip_t::S5B;
}

std::size_t CSoundChipTypeS5B::GetSupportedChannelCount() const {
	return 3;
}

chan_id_t CSoundChipTypeS5B::GetFirstChannelID() const {
	return chan_id_t::S5B_CH1;
}

std::string_view CSoundChipTypeS5B::GetShortName() const {
	return "5B";
}

std::string_view CSoundChipTypeS5B::GetFullName() const {
	return "Sunsoft 5B";
}

std::string_view CSoundChipTypeS5B::GetShortChannelName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {"5B1"sv, "5B2"sv, "5B3"sv};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::string_view CSoundChipTypeS5B::GetFullChannelName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {
		"5B Square 1"sv,
		"5B Square 2"sv,
		"5B Square 3"sv,
	};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::unique_ptr<CSoundChip> CSoundChipTypeS5B::MakeSoundDriver(CMixer &mixer) const {
	return std::make_unique<CS5B>(mixer);
}

std::unique_ptr<CChipHandler> CSoundChipTypeS5B::MakeChipHandler() const {
	auto chip = std::make_unique<CChipHandlerS5B>();
	for (unsigned i = 0; i < MAX_CHANNELS_S5B; ++i)
		chip->AddChannelHandler(std::make_unique<CChannelHandlerS5B>(MakeChannelIndex(GetID(), i), *chip));
	return chip;
}
