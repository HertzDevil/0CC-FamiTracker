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

#include "APU/Channel.h"

class C2A03Chan : public CChannel {		// // //
public:
	using CChannel::CChannel;		// // //

	uint16_t GetPeriod() const;

	static constexpr unsigned SEQUENCER_FREQUENCY = 240;

	static constexpr uint8_t LENGTH_TABLE[] = {
		0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06,
		0xA0, 0x08, 0x3C, 0x0A, 0x0E, 0x0C, 0x1A, 0x0E,
		0x0C, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16,
		0xC0, 0x18, 0x48, 0x1A, 0x10, 0x1C, 0x20, 0x1E,
	};

protected:
	// Variables used by channels
	uint8_t		m_iControlReg;
	uint8_t		m_iEnabled;
	uint16_t	m_iPeriod;
	uint16_t	m_iLengthCounter;
	uint32_t	m_iCounter;
};
