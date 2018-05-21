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

#include "FamiTrackerDefines.h"
#include "ft0cc/doc/effect_type.hpp"
#include "APU/Types.h"
#include <memory>
#include <string>
#include <array>
#include <map>

class CFamiTrackerModule;
namespace ft0cc::doc {
struct effect_command;
class pattern_note;
} // namespace ft0cc::doc

std::string MakeCommandString(ft0cc::doc::effect_command cmd);		// // //

// // // special echo buffer constants
inline constexpr int ECHO_BUFFER_NONE = 0xFF;
inline constexpr int ECHO_BUFFER_HALT = 0x7F;
inline constexpr int ECHO_BUFFER_ECHO = 0x80;

// // // Channel state information
class stChannelState {
	friend class CSongState;

public:
	stChannelState();

	/*!	\brief Obtains a human-readable form of a channel state object.
	\warning The output of this method is neither guaranteed nor required to match that of
	CChannelHandler::GetStateString.
	\param State A reference to the channel state object.
	\return A string representing the channel's state.
	\relates CChannelHandler
	*/
	std::string GetStateString() const;

	stChannelID ChannelID;
	int Instrument = MAX_INSTRUMENTS;
	int Volume = MAX_VOLUME;
	std::array<int, enum_count<ft0cc::doc::effect_type>() + 1> Effect = { };
	int Effect_LengthCounter = -1;
	int Effect_AutoFMMult = -1;
	std::array<int, ECHO_BUFFER_LENGTH> Echo = { };

private:
	void HandleNote(const ft0cc::doc::pattern_note &Note, unsigned EffColumns);
	void HandleNormalCommand(ft0cc::doc::effect_command cmd);
	void HandleSlideCommand(ft0cc::doc::effect_command cmd);
	void HandleExxCommand2A03(unsigned char param);
	void HandleSxxCommand(unsigned char param);

	int BufferPos = 0;
	std::array<int, ECHO_BUFFER_LENGTH> Transpose = { };
};

class CSongState {
public:
	void Retrieve(const CFamiTrackerModule &modfile, unsigned Track, unsigned Frame, unsigned Row);
	std::string GetChannelStateString(const CFamiTrackerModule &modfile, stChannelID chan) const;

	std::map<stChannelID, stChannelState> State;
	int Tempo = -1;
	int Speed = -1;
	int GroovePos = -1; // -1: disable groove
};
