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

#include "ChannelFactory.h"
#include "Channels2A03.h"
#include "ChannelsVRC6.h"
#include "ChannelsVRC7.h"
#include "ChannelsFDS.h"
#include "ChannelsMMC5.h"
#include "ChannelsN163.h"
#include "ChannelsS5B.h"
#include "ChannelsSN7.h"
#include "APU/Types.h"

// // // Default implementation for channel factory

std::unique_ptr<CChannelHandler> CChannelFactory::Make(chan_id_t id) {
	auto chan = MakeImpl(id);
	if (chan)
		chan->SetChannelID(id);
	return chan;
}

std::unique_ptr<CChannelHandler> CChannelFactory::MakeImpl(chan_id_t id) {
	switch (id) {
	case chan_id_t::SQUARE1: case chan_id_t::SQUARE2:
		return std::make_unique<C2A03Square>();
	case chan_id_t::TRIANGLE:
		return std::make_unique<CTriangleChan>();
	case chan_id_t::NOISE:
		return std::make_unique<CNoiseChan>();
	case chan_id_t::DPCM:
		return std::make_unique<CDPCMChan>();

	case chan_id_t::VRC6_PULSE1: case chan_id_t::VRC6_PULSE2:
		return std::make_unique<CVRC6Square>();
	case chan_id_t::VRC6_SAWTOOTH:
		return std::make_unique<CVRC6Sawtooth>();

	case chan_id_t::VRC7_CH1: case chan_id_t::VRC7_CH2: case chan_id_t::VRC7_CH3:
	case chan_id_t::VRC7_CH4: case chan_id_t::VRC7_CH5: case chan_id_t::VRC7_CH6:
		return std::make_unique<CVRC7Channel>();

	case chan_id_t::FDS:
		return std::make_unique<CChannelHandlerFDS>();

	case chan_id_t::MMC5_SQUARE1: case chan_id_t::MMC5_SQUARE2:
		return std::make_unique<CChannelHandlerMMC5>();

	case chan_id_t::N163_CH1: case chan_id_t::N163_CH2: case chan_id_t::N163_CH3: case chan_id_t::N163_CH4:
	case chan_id_t::N163_CH5: case chan_id_t::N163_CH6: case chan_id_t::N163_CH7: case chan_id_t::N163_CH8:
		return std::make_unique<CChannelHandlerN163>();

	case chan_id_t::S5B_CH1: case chan_id_t::S5B_CH2: case chan_id_t::S5B_CH3:
		return std::make_unique<CChannelHandlerS5B>();

	case chan_id_t::SN76489_CH1: case chan_id_t::SN76489_CH2: case chan_id_t::SN76489_CH3:
		return std::make_unique<CSN7SquareChan>();
	case chan_id_t::SN76489_NOISE:
		return std::make_unique<CSN7NoiseChan>();
	}

	return nullptr;
}
