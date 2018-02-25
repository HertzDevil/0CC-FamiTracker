#include "ChipFactory.h"
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

#include "ChipFactory.h"
#include "APU/Types.h"

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

std::unique_ptr<CChipHandler> CChipFactory::Make(sound_chip_t id) {
	switch (id) {
	case sound_chip_t::APU: {
		auto chip = std::make_unique<CChipHandler>();
		chip->AddChannelHandler(std::make_unique<C2A03Square>(chan_id_t::SQUARE1));
		chip->AddChannelHandler(std::make_unique<C2A03Square>(chan_id_t::SQUARE2));
		chip->AddChannelHandler(std::make_unique<CTriangleChan>(chan_id_t::TRIANGLE));
		chip->AddChannelHandler(std::make_unique<CNoiseChan>(chan_id_t::NOISE));
		chip->AddChannelHandler(std::make_unique<CDPCMChan>(chan_id_t::DPCM));
		return chip;
	}
	case sound_chip_t::VRC6: {
		auto chip = std::make_unique<CChipHandler>();
		chip->AddChannelHandler(std::make_unique<CVRC6Square>(chan_id_t::VRC6_PULSE1));
		chip->AddChannelHandler(std::make_unique<CVRC6Square>(chan_id_t::VRC6_PULSE2));
		chip->AddChannelHandler(std::make_unique<CVRC6Sawtooth>(chan_id_t::VRC6_SAWTOOTH));
		return chip;
	}
	case sound_chip_t::VRC7: {
		auto chip = std::make_unique<CChipHandlerVRC7>();
		for (unsigned i = 0; i < MAX_CHANNELS_VRC7; ++i)
			chip->AddChannelHandler(std::make_unique<CChannelHandlerVRC7>(MakeChannelIndex(id, i), *chip));
		return chip;
	}
	case sound_chip_t::FDS: {
		auto chip = std::make_unique<CChipHandler>();
		chip->AddChannelHandler(std::make_unique<CChannelHandlerFDS>(chan_id_t::FDS));
		return chip;
	}
	case sound_chip_t::MMC5: {
		auto chip = std::make_unique<CChipHandler>();
		chip->AddChannelHandler(std::make_unique<CChannelHandlerMMC5>(chan_id_t::MMC5_SQUARE1));
		chip->AddChannelHandler(std::make_unique<CChannelHandlerMMC5>(chan_id_t::MMC5_SQUARE2));
		return chip;
	}
	case sound_chip_t::N163: {
		auto chip = std::make_unique<CChipHandler>();
		for (unsigned i = 0; i < MAX_CHANNELS_N163; ++i)
			chip->AddChannelHandler(std::make_unique<CChannelHandlerN163>(MakeChannelIndex(id, i)));
		return chip;
	}
	case sound_chip_t::S5B: {
		auto chip = std::make_unique<CChipHandlerS5B>();
		for (unsigned i = 0; i < MAX_CHANNELS_S5B; ++i)
			chip->AddChannelHandler(std::make_unique<CChannelHandlerS5B>(MakeChannelIndex(id, i), *chip));
		return chip;
	}
	default:
		return nullptr;
	}
}
