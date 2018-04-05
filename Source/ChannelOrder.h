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

#include <vector>
#include <map>
#include "APU/Types.h"

// // // common functionality between CChannelMap and CSongView

class CChannelOrder {
public:
	stChannelID TranslateChannel(std::size_t index) const;
	bool HasChannel(stChannelID chan) const;
	std::size_t GetChannelIndex(stChannelID chan) const;
	std::size_t GetChannelCount() const;

	bool AddChannel(stChannelID chan);
	bool RemoveChannel(stChannelID chan);

	CChannelOrder BuiltinOrder() const;
	CChannelOrder Canonicalize() const;

	// void (*F)(stChannelID chan)
	template <typename F>
	void ForeachChannel(F f) const {
		for (stChannelID ch : order_)
			f(ch);
	}

private:
	std::vector<stChannelID> order_;
	std::map<stChannelID, std::size_t> indices_;
};
