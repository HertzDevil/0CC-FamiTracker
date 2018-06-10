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
#include "EffectName.h"

#include "APU/2A03.h"
#include "APU/VRC6.h"
#include "APU/VRC7.h"
#include "APU/FDS.h"
#include "APU/MMC5.h"
#include "APU/N163.h"
#include "APU/S5B.h"
#include "APU/SN76489.h"

#include "Channels2A03.h"
#include "ChannelsVRC6.h"
#include "ChannelsVRC7.h"
#include "ChannelsFDS.h"
#include "ChannelsMMC5.h"
#include "ChannelsN163.h"
#include "ChannelsS5B.h"
#include "ChannelsSN7.h"

#include "ChipHandler.h"
#include "ChipHandlerVRC7.h"
#include "ChipHandlerS5B.h"

namespace {

// constexpr ft0cc::doc::effect_type VRC6_EFFECTS[] = {};
constexpr ft0cc::doc::effect_type VRC7_EFFECTS[] = {ft0cc::doc::effect_type::VRC7_PORT, ft0cc::doc::effect_type::VRC7_WRITE};
constexpr ft0cc::doc::effect_type FDS_EFFECTS[] = {ft0cc::doc::effect_type::FDS_MOD_DEPTH, ft0cc::doc::effect_type::FDS_MOD_SPEED_HI, ft0cc::doc::effect_type::FDS_MOD_SPEED_LO, ft0cc::doc::effect_type::FDS_VOLUME, ft0cc::doc::effect_type::FDS_MOD_BIAS};
// constexpr ft0cc::doc::effect_type MMC5_EFFECTS[] = {};
constexpr ft0cc::doc::effect_type N163_EFFECTS[] = {ft0cc::doc::effect_type::N163_WAVE_BUFFER};
constexpr ft0cc::doc::effect_type S5B_EFFECTS[] = {ft0cc::doc::effect_type::SUNSOFT_ENV_TYPE, ft0cc::doc::effect_type::SUNSOFT_ENV_HI, ft0cc::doc::effect_type::SUNSOFT_ENV_LO, ft0cc::doc::effect_type::SUNSOFT_NOISE};
constexpr ft0cc::doc::effect_type SN76489_EFFECTS[] = {ft0cc::doc::effect_type::SN76489_CONTROL};

template <typename T>
struct CChipHandlerBuilder {
	CChipHandlerBuilder(std::uint8_t nInstance, sound_chip_t chip) :
		CChipHandlerBuilder(std::make_unique<T>(), nInstance, chip)
	{
	}

	template <typename U, typename SubindexT>
	CChipHandlerBuilder With(SubindexT subindex) && {
		auto sub = static_cast<std::uint8_t>(subindex);
		if constexpr (std::is_constructible_v<U, stChannelID, T &>)
			chip_->AddChannelHandler(std::make_unique<U>(stChannelID {instance_, id_, sub}, *chip_));
		else
			chip_->AddChannelHandler(std::make_unique<U>(stChannelID {instance_, id_, sub}));
		return {std::move(chip_), instance_, id_};
	}

	operator std::unique_ptr<CChipHandler>() && noexcept {
		return std::move(chip_);
	}

private:
	CChipHandlerBuilder(std::unique_ptr<T> handler, std::uint8_t nInstance, sound_chip_t chip) :
		chip_(std::move(handler)), instance_(nInstance), id_(chip)
	{
	}

	std::unique_ptr<T> chip_;
	std::uint8_t instance_;
	sound_chip_t id_;
};

} // namespace

// implementations of built-in sound chip types

sound_chip_t CSoundChipType2A03::GetID() const {
	return sound_chip_t::APU;
}

std::size_t CSoundChipType2A03::GetSupportedChannelCount() const {
	return 5;
}

std::string_view CSoundChipType2A03::GetShortName() const {
	return "2A03";
}

std::string_view CSoundChipType2A03::GetFullName() const {
	return "Nintendo 2A03";
}

std::string_view CSoundChipType2A03::GetChannelShortName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {"PU1", "PU2", "TRI", "NOI", "DMC"};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::string_view CSoundChipType2A03::GetChannelFullName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {
		"Pulse 1",
		"Pulse 2",
		"Triangle",
		"Noise",
		"DPCM",
	};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::unique_ptr<CSoundChip> CSoundChipType2A03::MakeSoundDriver(CMixer &mixer, std::uint8_t nInstance) const {
	return std::make_unique<C2A03>(mixer, nInstance);
}

std::unique_ptr<CChipHandler> CSoundChipType2A03::MakeChipHandler(std::uint8_t nInstance) const {
	return CChipHandlerBuilder<CChipHandler> {nInstance, GetID()}
		.With<C2A03Square>(apu_subindex_t::pulse1)
		.With<C2A03Square>(apu_subindex_t::pulse2)
		.With<CTriangleChan>(apu_subindex_t::triangle)
		.With<CNoiseChan>(apu_subindex_t::noise)
		.With<CDPCMChan>(apu_subindex_t::dpcm);
}

ft0cc::doc::effect_type CSoundChipType2A03::TranslateEffectName(char name, sound_chip_t chip) const {
	for (auto fx : enum_values<ft0cc::doc::effect_type>())
		if (name == EFF_CHAR[value_cast(fx)])
			return fx;
	return ft0cc::doc::effect_type::none;
}



sound_chip_t CSoundChipTypeVRC6::GetID() const {
	return sound_chip_t::VRC6;
}

std::size_t CSoundChipTypeVRC6::GetSupportedChannelCount() const {
	return 3;
}

std::string_view CSoundChipTypeVRC6::GetShortName() const {
	return "VRC6";
}

std::string_view CSoundChipTypeVRC6::GetFullName() const {
	return "Konami VRC6";
}

std::string_view CSoundChipTypeVRC6::GetChannelShortName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {"V1", "V2", "SAW"};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::string_view CSoundChipTypeVRC6::GetChannelFullName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {
		"VRC6 Pulse 1",
		"VRC6 Pulse 2",
		"Sawtooth",
	};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::unique_ptr<CSoundChip> CSoundChipTypeVRC6::MakeSoundDriver(CMixer &mixer, std::uint8_t nInstance) const {
	return std::make_unique<CVRC6>(mixer, nInstance);
}

std::unique_ptr<CChipHandler> CSoundChipTypeVRC6::MakeChipHandler(std::uint8_t nInstance) const {
	return CChipHandlerBuilder<CChipHandler> {nInstance, GetID()}
		.With<CVRC6Square>(vrc6_subindex_t::pulse1)
		.With<CVRC6Square>(vrc6_subindex_t::pulse2)
		.With<CVRC6Sawtooth>(vrc6_subindex_t::sawtooth);
}

ft0cc::doc::effect_type CSoundChipTypeVRC6::TranslateEffectName(char name, sound_chip_t chip) const {
	for (auto fx : enum_values<ft0cc::doc::effect_type>())
		if (name == EFF_CHAR[value_cast(fx)])
			return fx;
	return ft0cc::doc::effect_type::none;
}



sound_chip_t CSoundChipTypeVRC7::GetID() const {
	return sound_chip_t::VRC7;
}

std::size_t CSoundChipTypeVRC7::GetSupportedChannelCount() const {
	return 6;
}

std::string_view CSoundChipTypeVRC7::GetShortName() const {
	return "VRC7";
}

std::string_view CSoundChipTypeVRC7::GetFullName() const {
	return "Konami VRC7";
}

std::string_view CSoundChipTypeVRC7::GetChannelShortName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {"FM1", "FM2", "FM3", "FM4", "FM5", "FM6"};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::string_view CSoundChipTypeVRC7::GetChannelFullName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {
		"FM Channel 1",
		"FM Channel 2",
		"FM Channel 3",
		"FM Channel 4",
		"FM Channel 5",
		"FM Channel 6",
	};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::unique_ptr<CSoundChip> CSoundChipTypeVRC7::MakeSoundDriver(CMixer &mixer, std::uint8_t nInstance) const {
	return std::make_unique<CVRC7>(mixer, nInstance);
}

std::unique_ptr<CChipHandler> CSoundChipTypeVRC7::MakeChipHandler(std::uint8_t nInstance) const {
	return CChipHandlerBuilder<CChipHandlerVRC7> {nInstance, GetID()}
		.With<CChannelHandlerVRC7>(vrc7_subindex_t::ch1)
		.With<CChannelHandlerVRC7>(vrc7_subindex_t::ch2)
		.With<CChannelHandlerVRC7>(vrc7_subindex_t::ch3)
		.With<CChannelHandlerVRC7>(vrc7_subindex_t::ch4)
		.With<CChannelHandlerVRC7>(vrc7_subindex_t::ch5)
		.With<CChannelHandlerVRC7>(vrc7_subindex_t::ch6);
}

ft0cc::doc::effect_type CSoundChipTypeVRC7::TranslateEffectName(char name, sound_chip_t chip) const {
	for (ft0cc::doc::effect_type fx : VRC7_EFFECTS)
		if (name == EFF_CHAR[value_cast(fx)])
			return fx;
	for (auto fx : enum_values<ft0cc::doc::effect_type>())
		if (name == EFF_CHAR[value_cast(fx)])
			return fx;
	return ft0cc::doc::effect_type::none;
}



sound_chip_t CSoundChipTypeFDS::GetID() const {
	return sound_chip_t::FDS;
}

std::size_t CSoundChipTypeFDS::GetSupportedChannelCount() const {
	return 1;
}

std::string_view CSoundChipTypeFDS::GetShortName() const {
	return "FDS";
}

std::string_view CSoundChipTypeFDS::GetFullName() const {
	return "Nintendo FDS";
}

std::string_view CSoundChipTypeFDS::GetChannelShortName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {"FDS"};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::string_view CSoundChipTypeFDS::GetChannelFullName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {
		"FDS",
	};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::unique_ptr<CSoundChip> CSoundChipTypeFDS::MakeSoundDriver(CMixer &mixer, std::uint8_t nInstance) const {
	return std::make_unique<CFDS>(mixer, nInstance);
}

std::unique_ptr<CChipHandler> CSoundChipTypeFDS::MakeChipHandler(std::uint8_t nInstance) const {
	return CChipHandlerBuilder<CChipHandler> {nInstance, GetID()}
		.With<CChannelHandlerFDS>(fds_subindex_t::wave);
}

ft0cc::doc::effect_type CSoundChipTypeFDS::TranslateEffectName(char name, sound_chip_t chip) const {
	for (ft0cc::doc::effect_type fx : FDS_EFFECTS)
		if (name == EFF_CHAR[value_cast(fx)])
			return fx;
	for (auto fx : enum_values<ft0cc::doc::effect_type>())
		if (name == EFF_CHAR[value_cast(fx)])
			return fx;
	return ft0cc::doc::effect_type::none;
}



sound_chip_t CSoundChipTypeMMC5::GetID() const {
	return sound_chip_t::MMC5;
}

std::size_t CSoundChipTypeMMC5::GetSupportedChannelCount() const {
	return 3; // 2
}

std::string_view CSoundChipTypeMMC5::GetShortName() const {
	return "MMC5";
}

std::string_view CSoundChipTypeMMC5::GetFullName() const {
	return "Nintendo MMC5";
}

std::string_view CSoundChipTypeMMC5::GetChannelShortName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {"PU3", "PU4", "PCM"};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::string_view CSoundChipTypeMMC5::GetChannelFullName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {
		"MMC5 Pulse 1",
		"MMC5 Pulse 2",
		"MMC5 PCM",
	};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::unique_ptr<CSoundChip> CSoundChipTypeMMC5::MakeSoundDriver(CMixer &mixer, std::uint8_t nInstance) const {
	return std::make_unique<CMMC5>(mixer, nInstance);
}

std::unique_ptr<CChipHandler> CSoundChipTypeMMC5::MakeChipHandler(std::uint8_t nInstance) const {
	return CChipHandlerBuilder<CChipHandler> {nInstance, GetID()}
		.With<CChannelHandlerMMC5>(mmc5_subindex_t::pulse1)
		.With<CChannelHandlerMMC5>(mmc5_subindex_t::pulse2);
}

ft0cc::doc::effect_type CSoundChipTypeMMC5::TranslateEffectName(char name, sound_chip_t chip) const {
	for (auto fx : enum_values<ft0cc::doc::effect_type>())
		if (name == EFF_CHAR[value_cast(fx)])
			return fx;
	return ft0cc::doc::effect_type::none;
}



sound_chip_t CSoundChipTypeN163::GetID() const {
	return sound_chip_t::N163;
}

std::size_t CSoundChipTypeN163::GetSupportedChannelCount() const {
	return 8;
}

std::string_view CSoundChipTypeN163::GetShortName() const {
	return "N163";
}

std::string_view CSoundChipTypeN163::GetFullName() const {
	return "Namco 163";
}

std::string_view CSoundChipTypeN163::GetChannelShortName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {"N1", "N2", "N3", "N4", "N5", "N6", "N7", "N8"};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::string_view CSoundChipTypeN163::GetChannelFullName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {
		"Namco 1",
		"Namco 2",
		"Namco 3",
		"Namco 4",
		"Namco 5",
		"Namco 6",
		"Namco 7",
		"Namco 8",
	};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::unique_ptr<CSoundChip> CSoundChipTypeN163::MakeSoundDriver(CMixer &mixer, std::uint8_t nInstance) const {
	return std::make_unique<CN163>(mixer, nInstance);
}

std::unique_ptr<CChipHandler> CSoundChipTypeN163::MakeChipHandler(std::uint8_t nInstance) const {
	return CChipHandlerBuilder<CChipHandler> {nInstance, GetID()}
		.With<CChannelHandlerN163>(n163_subindex_t::ch1)
		.With<CChannelHandlerN163>(n163_subindex_t::ch2)
		.With<CChannelHandlerN163>(n163_subindex_t::ch3)
		.With<CChannelHandlerN163>(n163_subindex_t::ch4)
		.With<CChannelHandlerN163>(n163_subindex_t::ch5)
		.With<CChannelHandlerN163>(n163_subindex_t::ch6)
		.With<CChannelHandlerN163>(n163_subindex_t::ch7)
		.With<CChannelHandlerN163>(n163_subindex_t::ch8);
}

ft0cc::doc::effect_type CSoundChipTypeN163::TranslateEffectName(char name, sound_chip_t chip) const {
	for (ft0cc::doc::effect_type fx : N163_EFFECTS)
		if (name == EFF_CHAR[value_cast(fx)])
			return fx;
	for (auto fx : enum_values<ft0cc::doc::effect_type>())
		if (name == EFF_CHAR[value_cast(fx)])
			return fx;
	return ft0cc::doc::effect_type::none;
}



sound_chip_t CSoundChipTypeS5B::GetID() const {
	return sound_chip_t::S5B;
}

std::size_t CSoundChipTypeS5B::GetSupportedChannelCount() const {
	return 3;
}

std::string_view CSoundChipTypeS5B::GetShortName() const {
	return "5B";
}

std::string_view CSoundChipTypeS5B::GetFullName() const {
	return "Sunsoft 5B";
}

std::string_view CSoundChipTypeS5B::GetChannelShortName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {"5B1", "5B2", "5B3"};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::string_view CSoundChipTypeS5B::GetChannelFullName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {
		"5B Square 1",
		"5B Square 2",
		"5B Square 3",
	};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::unique_ptr<CSoundChip> CSoundChipTypeS5B::MakeSoundDriver(CMixer &mixer, std::uint8_t nInstance) const {
	return std::make_unique<CS5B>(mixer, nInstance);
}

std::unique_ptr<CChipHandler> CSoundChipTypeS5B::MakeChipHandler(std::uint8_t nInstance) const {
	return CChipHandlerBuilder<CChipHandlerS5B> {nInstance, GetID()}
		.With<CChannelHandlerS5B>(s5b_subindex_t::square1)
		.With<CChannelHandlerS5B>(s5b_subindex_t::square2)
		.With<CChannelHandlerS5B>(s5b_subindex_t::square3);
}

ft0cc::doc::effect_type CSoundChipTypeS5B::TranslateEffectName(char name, sound_chip_t chip) const {
	for (ft0cc::doc::effect_type fx : S5B_EFFECTS)
		if (name == EFF_CHAR[value_cast(fx)])
			return fx;
	for (auto fx : enum_values<ft0cc::doc::effect_type>())
		if (name == EFF_CHAR[value_cast(fx)])
			return fx;
	return ft0cc::doc::effect_type::none;
}



sound_chip_t CSoundChipTypeSN7::GetID() const {
	return sound_chip_t::SN76489;
}

std::size_t CSoundChipTypeSN7::GetSupportedChannelCount() const {
	return 4;
}

std::string_view CSoundChipTypeSN7::GetShortName() const {
	return "PSG";
}

std::string_view CSoundChipTypeSN7::GetFullName() const {
	return "SN76489";
}

std::string_view CSoundChipTypeSN7::GetChannelShortName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {"SN1", "SN2", "SN3", "SNN"};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::string_view CSoundChipTypeSN7::GetChannelFullName(std::size_t subindex) const {
	using namespace std::string_view_literals;
	constexpr std::string_view NAMES[] = {
		"PSG Square 1",
		"PSG Square 2",
		"PSG Square 3",
		"PSG Noise",
	};
	return subindex < std::size(NAMES) ? NAMES[subindex] :
		throw std::invalid_argument {"Channel with given subindex does not exist"};
}

std::unique_ptr<CSoundChip> CSoundChipTypeSN7::MakeSoundDriver(CMixer &mixer, std::uint8_t nInstance) const {
	return std::make_unique<CSN76489>(mixer, nInstance);
}

std::unique_ptr<CChipHandler> CSoundChipTypeSN7::MakeChipHandler(std::uint8_t nInstance) const {
	return CChipHandlerBuilder<CChipHandler> {nInstance, GetID()}
		.With<CSN7SquareChan>(sn76489_subindex_t::square1)
		.With<CSN7SquareChan>(sn76489_subindex_t::square2)
		.With<CSN7SquareChan>(sn76489_subindex_t::square3)
		.With<CSN7NoiseChan>(sn76489_subindex_t::noise);
}

ft0cc::doc::effect_type CSoundChipTypeSN7::TranslateEffectName(char name, sound_chip_t chip) const {
	for (ft0cc::doc::effect_type fx : SN76489_EFFECTS)
		if (name == EFF_CHAR[value_cast(fx)])
			return fx;
	for (auto fx : enum_values<ft0cc::doc::effect_type>())
		if (name == EFF_CHAR[value_cast(fx)])
			return fx;
	return ft0cc::doc::effect_type::none;
}
