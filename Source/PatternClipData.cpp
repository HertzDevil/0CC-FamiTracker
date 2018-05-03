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
#include "PatternNote.h"
#include "Assertion.h"		// // //
#include <cstring>		// // //

CPatternClipData::CPatternClipData(int Channels, int Rows) :
	ClipInfo({Channels, Rows}),		// // //
	pPattern(std::make_unique<stChanNote[]>(Channels * Rows)),
	Size(Channels * Rows)
{
}

std::size_t CPatternClipData::GetAllocSize() const
{
	return sizeof(ClipInfo) + Size * sizeof(stChanNote);
}

bool CPatternClipData::ContainsData() const {		// // //
	return pPattern != nullptr;
}

bool CPatternClipData::ToBytes(std::byte *pBuf, std::size_t buflen) const		// // //
{
	if (buflen >= GetAllocSize()) {
		std::memcpy(pBuf, &ClipInfo, sizeof(ClipInfo));
		std::memcpy(pBuf + sizeof(ClipInfo), pPattern.get(), Size * sizeof(stChanNote));		// // //
		return true;
	}
	return false;
}

bool CPatternClipData::FromBytes(array_view<std::byte> Buf)		// // //
{
	if (Buf.size() >= GetAllocSize()) {
		std::memcpy(&ClipInfo, Buf.data(), sizeof(ClipInfo));
		Size = ClipInfo.Channels * ClipInfo.Rows;
		pPattern = std::make_unique<stChanNote[]>(Size);		// // //
		std::memcpy(pPattern.get(), Buf.data() + sizeof(ClipInfo), Size * sizeof(stChanNote));
		return true;
	}
	return false;
}

stChanNote *CPatternClipData::GetPattern(int Channel, int Row)
{
	Assert(Channel < ClipInfo.Channels);
	Assert(Row < ClipInfo.Rows);

	return &pPattern[Channel * ClipInfo.Rows + Row];
}

const stChanNote *CPatternClipData::GetPattern(int Channel, int Row) const
{
	Assert(Channel < ClipInfo.Channels);
	Assert(Row < ClipInfo.Rows);

	return &pPattern[Channel * ClipInfo.Rows + Row];
}
