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

class CSoundChipService {
public:
	void AddType(std::unique_ptr<CSoundChipType> stype);
	void AddDefaultTypes();

	std::size_t GetSupportedChannelCount(sound_chip_t chip) const;
	std::size_t GetChannelSubindex(chan_id_t ch) const;
	chan_id_t MakeChannelIndex(sound_chip_t chip, std::size_t subindex) const;

	std::string_view GetShortChipName(sound_chip_t chip) const;
	std::string_view GetFullChipName(sound_chip_t chip) const;
	std::string_view GetShortChannelName(chan_id_t ch) const;
	std::string_view GetFullChannelName(chan_id_t ch) const;

	sound_chip_t GetChipFromChannel(chan_id_t ch) const;
	sound_chip_t GetChipFromString(std::string_view sv) const;

	std::unique_ptr<CSoundChip> MakeSoundChipDriver(sound_chip_t chip, CMixer &mixer) const;
	std::unique_ptr<CChipHandler> MakeChipHandler(sound_chip_t chip) const;

	// void (*F)(sound_chip_t chip)
	template <typename F>
	void ForeachType(F f) const {
		for (auto &x : types_)
			f(x.first);
	}

private:
	const CSoundChipType &GetType(sound_chip_t chip) const;
	const CSoundChipType *GetTypePtr(sound_chip_t chip) const;

	std::map<sound_chip_t, std::unique_ptr<CSoundChipType>> types_;
};
