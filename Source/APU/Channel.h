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

#include <cstdint>		// // //
#include "Types.h"		// // //

class CMixer;

//
// This class is used to derive the audio channels
//

class CChannel {
public:
	CChannel(CMixer &Mixer, uint8_t Chip, chan_id_t ID);

	virtual ~CChannel() noexcept = default;
	virtual void EndFrame();

	chan_id_t GetChannelType() const;		// // //

	virtual double GetFrequency() const = 0;		// // //

protected:
	void Mix(int32_t Value);		// // //

protected:
	CMixer		*m_pMixer;			// The mixer

	uint32_t	m_iTime = 0;		// Cycle counter, resets every new frame
	chan_id_t	m_iChanId;			// // // This channels unique ID
	uint8_t		m_iChip;			// Chip
	int32_t		m_iLastValue = 0;	// Last value sent to mixer
};
