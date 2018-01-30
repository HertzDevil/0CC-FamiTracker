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

	CPatternData &GetPatternOnFrame(unsigned Frame);		// // //
	const CPatternData &GetPatternOnFrame(unsigned Frame) const;		// // //

	unsigned int GetFramePattern(unsigned Frame) const;
	void SetFramePattern(unsigned Frame, unsigned Pattern);

	unsigned GetEffectColumnCount() const;
	void SetEffectColumnCount(unsigned Count);

	// void (*F)(CPatternData &pattern [, std::size_t p_index])
	template <typename F>
	void VisitPatterns(F f) {
		if constexpr (std::is_invocable_v<F, CPatternData &, std::size_t>) {
			std::size_t p_index = 0;
			for (auto &pattern : m_pPatternData)
				f(pattern, p_index++);
		}
		else if constexpr (std::is_invocable_v<F, CPatternData &>) {
			for (auto &pattern : m_pPatternData)
				f(pattern);
		}
		else
			static_assert(false, "Unknown function signature");
	}

	// void (*F)(const CPatternData &pattern [, std::size_t p_index])
	template <typename F>
	void VisitPatterns(F f) const {
		if constexpr (std::is_invocable_v<F, const CPatternData &, std::size_t>) {
			std::size_t p_index = 0;
			for (auto &pattern : m_pPatternData)
				f(pattern, p_index++);
		}
		else if constexpr (std::is_invocable_v<F, const CPatternData &>) {
			for (auto &pattern : m_pPatternData)
				f(pattern);
		}
		else
			static_assert(false, "Unknown function signature");
	}

private:
	std::array<CPatternData, MAX_PATTERN> m_pPatternData = { };
	std::array<unsigned int, MAX_FRAMES> m_iFrameList = { };
	unsigned char m_iEffectColumns = 0;
};
