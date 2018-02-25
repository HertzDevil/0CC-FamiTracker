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

class CChannelHandler;
class CAPUInterface;

// // // handler for sound chip instance

class CChipHandler {
public:
	virtual ~CChipHandler() noexcept = default;

	CChipHandler(const CChipHandler &) = delete;
	CChipHandler &operator=(const CChipHandler &) = delete;
	CChipHandler(CChipHandler &&) noexcept = default;
	CChipHandler &operator=(CChipHandler &&) noexcept = default;

protected:
	CChipHandler() noexcept = default;

public:
	virtual void RefreshBefore(CAPUInterface &apu);
	virtual void RefreshAfter(CAPUInterface &apu);

	void AddChannelHandler(CChannelHandler &ch);

protected:
	const auto &GetChannelHandlers() const {
		return channels_;
	}

private:
	std::vector<CChannelHandler *> channels_;
};
