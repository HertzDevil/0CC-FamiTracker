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

#include <map>
#include "SoundChipType.h"

class CChannelOrder;
class CSoundChipSet;
class CChannelMap;
enum class effect_t : std::uint8_t;

class CSoundChipService {
public:
	void AddType(std::unique_ptr<CSoundChipType> stype);
	void AddDefaultTypes();

	CChannelOrder MakeFullOrder() const;

	std::size_t GetSupportedChannelCount(sound_chip_t chip) const;

	std::string_view GetChipShortName(sound_chip_t chip) const;
	std::string_view GetChipFullName(sound_chip_t chip) const;
	std::string_view GetChannelShortName(stChannelID ch) const;
	std::string_view GetChannelFullName(stChannelID ch) const;

	std::unique_ptr<CChannelMap> MakeChannelMap(CSoundChipSet chips, unsigned n163chs) const; // TODO: remove n163chs
	std::unique_ptr<CSoundChip> MakeSoundChipDriver(sound_chip_t chip, CMixer &mixer, std::uint8_t nInstance) const;
	std::unique_ptr<CChipHandler> MakeChipHandler(sound_chip_t chip, std::uint8_t nInstance) const;

	effect_t TranslateEffectName(char name, sound_chip_t chip) const;

	// void (*F)(sound_chip_t chip)
	template <typename F>
	void ForeachType(F f) const {
		for (auto &x : types_)
			f(x.first);
	}

	// void (*F)(stChannelID track)
	template <typename F>
	void ForeachTrack(F f) const {
		for (auto &x : types_)
			for (std::size_t i = 0, n = x.second->GetSupportedChannelCount(); i < n; ++i)
				f(stChannelID {x.first, static_cast<std::uint8_t>(i)});
	}

private:
	const CSoundChipType &GetType(sound_chip_t chip) const;
	const CSoundChipType *GetTypePtr(sound_chip_t chip) const;

	std::map<sound_chip_t, std::unique_ptr<CSoundChipType>> types_;
};
