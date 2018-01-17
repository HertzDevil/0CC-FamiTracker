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

#include "TrackData.h"

CPatternData &CTrackData::GetPattern(unsigned Pattern) {
	return m_pPatternData.at(Pattern);
}

const CPatternData &CTrackData::GetPattern(unsigned Pattern) const {
	return m_pPatternData.at(Pattern);
}

unsigned int CTrackData::GetFramePattern(unsigned Frame) const {
	return Frame < m_iFrameList.size() ? m_iFrameList[Frame] : MAX_PATTERN;
}

void CTrackData::SetFramePattern(unsigned Frame, unsigned Pattern) {
	if (Frame < m_iFrameList.size())
		m_iFrameList[Frame] = Pattern;
}

unsigned CTrackData::GetEffectColumnCount() const {
	return m_iEffectColumns;
}

void CTrackData::SetEffectColumnCount(unsigned Count) {
	m_iEffectColumns = Count;
}
