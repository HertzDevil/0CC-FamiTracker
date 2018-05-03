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

#include "FrameClipData.h"
#include "FrameEditorTypes.h"		// // //
#include "Assertion.h"		// // //
#include <cstring>		// // //

CFrameClipData::CFrameClipData(int Channels, int Frames) :
	ClipInfo({Channels, Frames}),		// // //
	pFrames(std::make_unique<int[]>(Channels * Frames)),
	iSize(Channels * Frames)
{
}

CFrameSelection CFrameClipData::AsSelection(int startFrame) const {		// // //
	return {
		{startFrame, ClipInfo.FirstChannel},
		{startFrame + ClipInfo.Frames, ClipInfo.FirstChannel + ClipInfo.Channels},
	};
}

std::size_t CFrameClipData::GetAllocSize() const
{
	return sizeof(ClipInfo) + sizeof(int) * iSize;
}

bool CFrameClipData::ContainsData() const {		// // //
	return pFrames != nullptr;
}

bool CFrameClipData::ToBytes(std::byte *pBuf, std::size_t buflen) const		// // //
{
	if (buflen >= GetAllocSize()) {
		std::memcpy(pBuf, &ClipInfo, sizeof(ClipInfo));
		std::memcpy(pBuf + sizeof(ClipInfo), pFrames.get(), sizeof(int) * iSize);
		return true;
	}
	return false;
}

bool CFrameClipData::FromBytes(array_view<std::byte> Buf)		// // //
{
	if (Buf.size() >= GetAllocSize()) {
		std::memcpy(&ClipInfo, Buf.data(), sizeof(ClipInfo));
		iSize = ClipInfo.Channels * ClipInfo.Frames;
		pFrames = std::make_unique<int[]>(iSize);
		std::memcpy(pFrames.get(), Buf.data() + sizeof(ClipInfo), sizeof(int) * iSize);
		return true;
	}
	return false;
}

int CFrameClipData::GetFrame(int Frame, int Channel) const
{
	Assert(Frame >= 0 && Frame < ClipInfo.Frames);
	Assert(Channel >= 0 && Channel < ClipInfo.Channels);

	return pFrames[Frame * ClipInfo.Channels + Channel];		// // //
}

void CFrameClipData::SetFrame(int Frame, int Channel, int Pattern)
{
	Assert(Frame >= 0 && Frame < ClipInfo.Frames);
	Assert(Channel >= 0 && Channel < ClipInfo.Channels);

	pFrames[Frame * ClipInfo.Channels + Channel] = Pattern;		// // //
}
