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
#include <unordered_map>
#include <memory>
#include "APU/Types.h"

/*!
	\brief A queue which automatically reassigns notes in the same logical track to different
	physical channels. The same note coming from the same channel in a track may only be played on
	one physical channel.
*/
class CNoteChannelQueue
{
public:
	/*!	\brief Constructor of the note queue for a single track.
		\param Ch List of channel identifiers for this queue. */
	CNoteChannelQueue(std::vector<stChannelID> Ch);

	/*!	\brief Triggers a note on a given channel.
		\param Note The note number.
		\param Channel The channel identifier. */
	stChannelID Trigger(int Note, stChannelID Channel);
	/*!	\brief Releases a note on a given channel.
		\param Note The note number.
		\param Channel The channel identifier. */
	stChannelID Release(int Note, stChannelID Channel);
	/*!	\brief Cuts a note on a given channel.
		\param Note The note number.
		\param Channel The channel identifier. */
	stChannelID Cut(int Note, stChannelID Channel);
	/*!	\brief Stops whatever is played from a specific channel.
		\param Channel The channel identifier.
		\return A vector containing the channel indices that have a note halted. */
	std::vector<stChannelID> StopChannel(stChannelID Channel);
	/*!	\brief Stops all currently playing notes. */
	void StopAll();

	/*!	\brief Stops accepting notes from a given channel.
		\param Channel The channel identifier. */
	void MuteChannel(stChannelID Channel);
	/*!	\brief Resumes accepting notes from a given channel.
		\param Channel The channel identifier. */
	void UnmuteChannel(stChannelID Channel);

private:
	enum class note_state_t : unsigned char;

	const int m_iChannelCount;

	std::vector<stChannelID> m_iChannelMapID;
	std::vector<unsigned> m_iCurrentNote;
	std::vector<bool> m_bChannelMute;

	std::unordered_map<int, note_state_t> m_iNoteState;
	std::unordered_map<int, int> m_iNotePriority;
	std::unordered_map<int, stChannelID> m_iNoteChannel;
};

/*!
	\brief The actual note queue that keeps track of multiple logical tracks.
*/
class CNoteQueue
{
public:
	/*!	\brief Adds physical channels as a single logical track.
		\param Ch List of channel identifiers for the queue. */
	void AddMap(const std::vector<stChannelID> &Ch);
	/*!	\brief Removes all logical tracks. */
	void ClearMaps();

	stChannelID Trigger(int Note, stChannelID Channel);
	stChannelID Release(int Note, stChannelID Channel);
	stChannelID Cut(int Note, stChannelID Channel);
	std::vector<stChannelID> StopChannel(stChannelID Channel);
	void StopAll();

	void MuteChannel(stChannelID Channel);
	void UnmuteChannel(stChannelID Channel);

private:
	std::map<stChannelID, std::shared_ptr<CNoteChannelQueue>> m_Part;
};
