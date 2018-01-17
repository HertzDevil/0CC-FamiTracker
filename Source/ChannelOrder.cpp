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

#include "ChannelOrder.h"
#include "APU/Types.h"

chan_id_t CChannelOrder::TranslateChannel(std::size_t index) const {
	return index < order_.size() ? order_[index] : chan_id_t::NONE;
}

bool CChannelOrder::HasChannel(chan_id_t chan) const {
	auto it = indices_.find(chan);
	return it != indices_.end();
}

std::size_t CChannelOrder::GetChannelIndex(chan_id_t chan) const {
	auto it = indices_.find(chan);
	return it != indices_.end() ? it->second : (std::size_t)-1;
}

std::size_t CChannelOrder::GetChannelCount() const {
	return order_.size();
}

bool CChannelOrder::AddChannel(chan_id_t chan) {
	std::size_t index = order_.size();
	if (indices_.try_emplace(chan, index).second) {
		order_.push_back(chan);
		return true;
	}
	return false;
}
