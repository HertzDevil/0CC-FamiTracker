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

#include "PatternClipData.h"
#include "ft0cc/doc/pattern_note.hpp"
#include "Assertion.h"		// // //
#include <cstring>		// // //

CPatternClipData::CPatternClipData(int Channels, int Rows) :
	ClipInfo({Channels, Rows}),		// // //
	pPattern(std::make_unique<ft0cc::doc::pattern_note[]>(Channels * Rows)),
	Size(Channels * Rows)
{
}

std::size_t CPatternClipData::GetAllocSize() const
{
	return sizeof(ClipInfo) + Size * sizeof(ft0cc::doc::pattern_note);
}

bool CPatternClipData::ContainsData() const {		// // //
	return pPattern != nullptr;
}

bool CPatternClipData::ToBytes(array_view<std::byte> Buf) const		// // //
{
	if (Buf.size() >= GetAllocSize()) {
		std::memcpy(Buf.data(), &ClipInfo, sizeof(ClipInfo));
		std::memcpy(Buf.data() + sizeof(ClipInfo), pPattern.get(), Size * sizeof(ft0cc::doc::pattern_note));		// // //
		return true;
	}
	return false;
}

bool CPatternClipData::FromBytes(array_view<const std::byte> Buf)		// // //
{
	if (Buf.size() >= GetAllocSize()) {
		std::memcpy(&ClipInfo, Buf.data(), sizeof(ClipInfo));
		Size = ClipInfo.Channels * ClipInfo.Rows;
		pPattern = std::make_unique<ft0cc::doc::pattern_note[]>(Size);		// // //
		std::memcpy(pPattern.get(), Buf.data() + sizeof(ClipInfo), Size * sizeof(ft0cc::doc::pattern_note));
		return true;
	}
	return false;
}

ft0cc::doc::pattern_note *CPatternClipData::GetPattern(int Channel, int Row)
{
	Assert(Channel < ClipInfo.Channels);
	Assert(Row < ClipInfo.Rows);

	return &pPattern[Channel * ClipInfo.Rows + Row];
}

const ft0cc::doc::pattern_note *CPatternClipData::GetPattern(int Channel, int Row) const
{
	Assert(Channel < ClipInfo.Channels);
	Assert(Row < ClipInfo.Rows);

	return &pPattern[Channel * ClipInfo.Rows + Row];
}
