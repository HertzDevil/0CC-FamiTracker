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

stChannelID CChannelOrder::TranslateChannel(std::size_t index) const {
	return index < order_.size() ? stChannelID {order_[index]} : stChannelID { };
}

bool CChannelOrder::HasChannel(stChannelID chan) const {
	auto it = indices_.find(chan);
	return it != indices_.end();
}

std::size_t CChannelOrder::GetChannelIndex(stChannelID chan) const {
	auto it = indices_.find(chan);
	return it != indices_.end() ? it->second : (std::size_t)-1;
}

std::size_t CChannelOrder::GetChannelCount() const {
	return order_.size();
}

bool CChannelOrder::AddChannel(stChannelID chan) {
	std::size_t index = order_.size();
	if (indices_.try_emplace(chan, index).second) {
		order_.push_back(chan);
		return true;
	}
	return false;
}

CChannelOrder CChannelOrder::Canonicalize() const {
	const stChannelID CANONICAL_ORDER[] = {
		apu_subindex_t::pulse1, apu_subindex_t::pulse2, apu_subindex_t::triangle, apu_subindex_t::noise,
		mmc5_subindex_t::pulse1, mmc5_subindex_t::pulse2, mmc5_subindex_t::pcm,
		vrc6_subindex_t::pulse1, vrc6_subindex_t::pulse2, vrc6_subindex_t::sawtooth,
		n163_subindex_t::ch1, n163_subindex_t::ch2, n163_subindex_t::ch3, n163_subindex_t::ch4,
		n163_subindex_t::ch5, n163_subindex_t::ch6, n163_subindex_t::ch7, n163_subindex_t::ch8,
		fds_subindex_t::wave,
		s5b_subindex_t::square1, s5b_subindex_t::square2, s5b_subindex_t::square3,
		vrc7_subindex_t::ch1, vrc7_subindex_t::ch2, vrc7_subindex_t::ch3,
		vrc7_subindex_t::ch4, vrc7_subindex_t::ch5, vrc7_subindex_t::ch6,
		apu_subindex_t::dpcm,
	};

	CChannelOrder orderNew;

	for (const auto &ch : CANONICAL_ORDER)
		if (HasChannel(ch))
			orderNew.AddChannel(ch);

	return orderNew;
}
