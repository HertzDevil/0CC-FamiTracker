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
#include "ArrayStream.h"		// // //

CFrameClipData::CFrameClipData(unsigned Channels, unsigned Frames) :
	ClipInfo({Channels, Frames}),		// // //
	pFrames(std::make_unique<value_type[]>(Channels * Frames))
{
}

CFrameSelection CFrameClipData::AsSelection(unsigned startFrame) const {		// // //
	return {
		{static_cast<int>(startFrame), static_cast<int>(ClipInfo.FirstChannel)},
		{static_cast<int>(startFrame + ClipInfo.Frames), static_cast<int>(ClipInfo.FirstChannel + ClipInfo.Channels)},
	};
}

std::size_t CFrameClipData::GetAllocSize() const
{
	return sizeof(ClipInfo) + ClipInfo.GetSize() * sizeof(value_type);
}

bool CFrameClipData::ContainsData() const {		// // //
	return pFrames != nullptr;
}

bool CFrameClipData::ToBytes(array_view<std::byte> Buf) const		// // //
{
	try {
		if (Buf.size() >= GetAllocSize()) {
			CArrayStream stream {Buf};

			stream.WriteInt<std::uint32_t>(ClipInfo.Channels);
			stream.WriteInt<std::uint32_t>(ClipInfo.Frames);
			stream.WriteInt<std::uint32_t>(ClipInfo.FirstChannel);
			stream.WriteInt<std::uint32_t>(ClipInfo.OleInfo.SourceRowStart);
			stream.WriteInt<std::uint32_t>(ClipInfo.OleInfo.SourceRowEnd);

			const std::size_t Size = ClipInfo.GetSize();
			for (std::size_t i = 0; i < Size; ++i)
				stream.WriteInt<value_type>(pFrames[i]);
			return true;
		}
		return false;
	}
	catch (CBinaryIOException &) {
		return false;
	}
}

bool CFrameClipData::FromBytes(array_view<const std::byte> Buf)		// // //
{
	try {
		if (Buf.size() >= sizeof(stClipInfo)) {
			CConstArrayStream stream {Buf};

			stClipInfo info = { };
			info.Channels = stream.ReadInt<std::uint32_t>();
			info.Frames = stream.ReadInt<std::uint32_t>();
			info.FirstChannel = stream.ReadInt<std::uint32_t>();
			info.OleInfo.SourceRowStart = stream.ReadInt<std::uint32_t>();
			info.OleInfo.SourceRowEnd = stream.ReadInt<std::uint32_t>();

			const std::size_t Size = info.GetSize();
			if (Buf.size() >= sizeof(stClipInfo) + Size * sizeof(value_type)) {
				pFrames = std::make_unique<value_type[]>(Size);
				ClipInfo = info;
				for (std::size_t i = 0; i < Size; ++i)
					pFrames[i] = stream.ReadInt<value_type>();
				return true;
			}
		}
		return false;
	}
	catch (CBinaryIOException &) {
		return false;
	}
}

int CFrameClipData::GetFrame(unsigned Frame, unsigned Channel) const
{
	Assert(Frame < ClipInfo.Frames);
	Assert(Channel < ClipInfo.Channels);

	return pFrames[Frame * ClipInfo.Channels + Channel];		// // //
}

void CFrameClipData::SetFrame(unsigned Frame, unsigned Channel, CFrameClipData::value_type Pattern)
{
	Assert(Frame < ClipInfo.Frames);
	Assert(Channel < ClipInfo.Channels);

	pFrames[Frame * ClipInfo.Channels + Channel] = Pattern;		// // //
}
