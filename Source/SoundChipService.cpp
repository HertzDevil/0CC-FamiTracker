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

#include "SoundChipService.h"
#include "SoundChipTypeImpl.h"
#include "ft0cc/doc/effect_type.hpp"
#include "EffectName.h"
#include "APU/Types.h"
#include "APU/SoundChip.h"
#include "ChipHandler.h"
#include "ChannelOrder.h"
#include "ChannelMap.h"
#include "SoundChipSet.h"
#include "Assertion.h"

void CSoundChipService::AddType(std::unique_ptr<CSoundChipType> stype) {
	sound_chip_t id = stype->GetID();

	if (id == sound_chip_t::none)
		throw std::invalid_argument {"Cannot add sound chip with ID sound_chip_t::none"};
	if (types_.count(id))
		throw std::invalid_argument {"Cannot add sound chips with same ID"};

	types_.try_emplace(id, std::move(stype));
}

void CSoundChipService::AddDefaultTypes() {
	AddType(std::make_unique<CSoundChipType2A03>());
	AddType(std::make_unique<CSoundChipTypeVRC6>());
	AddType(std::make_unique<CSoundChipTypeVRC7>());
	AddType(std::make_unique<CSoundChipTypeFDS>());
	AddType(std::make_unique<CSoundChipTypeMMC5>());
	AddType(std::make_unique<CSoundChipTypeN163>());
	AddType(std::make_unique<CSoundChipTypeS5B>());
	AddType(std::make_unique<CSoundChipTypeSN7>());
}

CChannelOrder CSoundChipService::MakeFullOrder() const {
	CChannelOrder order;
	ForeachTrack([&] (stChannelID id) {
		order.AddChannel(id);
	});
	return order;
}

std::size_t CSoundChipService::GetSupportedChannelCount(sound_chip_t chip) const {
	auto *pChip = GetTypePtr(chip);
	return pChip ? pChip->GetSupportedChannelCount() : static_cast<std::size_t>(0u);
}

std::string_view CSoundChipService::GetChipShortName(sound_chip_t chip) const {
	return GetType(chip).GetShortName();
}

std::string_view CSoundChipService::GetChipFullName(sound_chip_t chip) const {
	return GetType(chip).GetFullName();
}

std::string_view CSoundChipService::GetChannelShortName(stChannelID ch) const {
	for (auto &x : types_)
		if (x.first == ch.Chip)
			return x.second->GetChannelShortName(ch.Subindex);
	throw std::invalid_argument {"Channel with given ID does not exist"};
}

std::string_view CSoundChipService::GetChannelFullName(stChannelID ch) const {
	for (auto &x : types_)
		if (x.first == ch.Chip)
			return x.second->GetChannelFullName(ch.Subindex);
	throw std::invalid_argument {"Channel with given ID does not exist"};
}

std::unique_ptr<CChannelMap> CSoundChipService::MakeChannelMap(CSoundChipSet chips, unsigned n163chs) const {
	Assert(n163chs <= MAX_CHANNELS_N163 && (chips.ContainsChip(sound_chip_t::N163) == (n163chs != 0)));

	auto map = std::make_unique<CChannelMap>(chips, n163chs);		// // //

	ForeachTrack([&] (stChannelID id) {
		if (map->SupportsChannel(id))
			map->GetChannelOrder().AddChannel(id);
	});

	return map;
}

std::unique_ptr<CSoundChip> CSoundChipService::MakeSoundChipDriver(sound_chip_t chip, CMixer &mixer, std::uint8_t nInstance) const {
	return GetType(chip).MakeSoundDriver(mixer, nInstance);
}

std::unique_ptr<CChipHandler> CSoundChipService::MakeChipHandler(sound_chip_t chip, std::uint8_t nInstance) const {
	return GetType(chip).MakeChipHandler(nInstance);
}

ft0cc::doc::effect_type CSoundChipService::TranslateEffectName(char name, sound_chip_t chip) const {
	auto it = types_.find(chip);
	return it != types_.end() ? it->second->TranslateEffectName(name, chip) : ft0cc::doc::effect_type::none;
}

const CSoundChipType &CSoundChipService::GetType(sound_chip_t chip) const {
	auto it = types_.find(chip);
	if (it == types_.end())
		throw std::invalid_argument {"Sound chip type with given ID does not exist"};
	return *it->second;
}

const CSoundChipType *CSoundChipService::GetTypePtr(sound_chip_t chip) const {
	auto it = types_.find(chip);
	return it == types_.end() ? nullptr : std::addressof(*it->second);
}
