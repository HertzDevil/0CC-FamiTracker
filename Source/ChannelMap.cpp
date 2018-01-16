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

#include "ChannelMap.h"
#include "TrackerChannel.h"
#include "APU/Types.h"		// // //

/*
 *  This class contains the expansion chip definitions & instruments.
 *
 */

CChannelMap::CChannelMap() : CChannelMap(SNDCHIP_NONE, 0) {
}

CChannelMap::CChannelMap(sound_chip_flag_t chips, unsigned n163chs) :
	chips_(chips), n163chs_(n163chs)
{
}

void CChannelMap::ResetChannels()
{
	m_pChannels.clear();		// // //
	m_iChannelIndices.clear();
}

void CChannelMap::RegisterChannel(CTrackerChannel &Channel)		// // //
{
	// Adds a channel to the channel map
	m_iChannelIndices.try_emplace(Channel.GetID(), m_pChannels.size());		// // //
	m_pChannels.push_back(&Channel);
}

bool CChannelMap::SupportsChannel(const CTrackerChannel &ch) const {		// // //
	return HasExpansionChip(ch.GetChip()) && !(ch.GetChip() == SNDCHIP_N163 &&
		GetChannelSubIndex(ch.GetID()) >= GetChipChannelCount(SNDCHIP_N163));
}

CTrackerChannel &CChannelMap::GetChannel(int index) const		// // //
{
	return *m_pChannels[index];
}

int CChannelMap::GetChannelIndex(chan_id_t chan) const {		// // //
	// Translate channel ID to index, returns -1 if not found
	auto it = m_iChannelIndices.find(chan);
	return it != m_iChannelIndices.cend() ? it->second : -1;
}

bool CChannelMap::HasChannel(chan_id_t chan) const {		// // //
	auto it = m_iChannelIndices.find(chan);
	return it != m_iChannelIndices.cend();
}

int CChannelMap::GetChannelCount() const {		// // //
	return m_pChannels.size();
}

chan_id_t CChannelMap::GetChannelType(int index) const {		// // //
	return GetChannel(index).GetID();
}

sound_chip_t CChannelMap::GetChipType(int index) const {
	return GetChannel(index).GetChip();
}

sound_chip_flag_t CChannelMap::GetExpansionFlag() const noexcept {		// // //
	return chips_;
}

unsigned CChannelMap::GetChipChannelCount(sound_chip_t chip) const {
	if (chip == SNDCHIP_N163)
		return HasExpansionChip(chip) ? n163chs_ : 0;

	unsigned count = 0;
	for (auto pChan : m_pChannels)
		if (pChan->GetChip() == chip)
			++count;
	return count;
}

bool CChannelMap::HasExpansionChip(sound_chip_t chips) const noexcept {
	return chips_.ContainsChip(chips);
}
