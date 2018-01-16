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

#include <array>
#include "FamiTrackerTypes.h"
#include "PatternData.h"

class CTrackData {
public:
	CPatternData &GetPattern(unsigned Pattern);		// // //
	const CPatternData &GetPattern(unsigned Pattern) const;		// // //

	unsigned int GetFramePattern(unsigned Frame) const;
	void SetFramePattern(unsigned Frame, unsigned Pattern);

	int GetEffectColumnCount() const;
	void SetEffectColumnCount(int Count);

private:
	friend class CSongData;
	auto &Patterns() { return m_pPatternData; }
	const auto &Patterns() const { return m_pPatternData; }

	std::array<CPatternData, MAX_PATTERN> m_pPatternData = { };
	std::array<unsigned int, MAX_FRAMES> m_iFrameList = { };
	unsigned char m_iEffectColumns = 0;
};
