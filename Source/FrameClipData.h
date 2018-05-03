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

#include <memory>
#include "BinarySerializable.h"		// // //

struct CFrameSelection;		// // //

class CFrameClipData : public CBinarySerializableInterface {		// // //
public:
	CFrameClipData() = default;
	CFrameClipData(int Channels, int Frames);

	CFrameSelection AsSelection(int startFrame) const;		// // //

	int  GetFrame(int Frame, int Channel) const;
	void SetFrame(int Frame, int Channel, int Pattern);

private:
	std::size_t GetAllocSize() const override;
	bool ContainsData() const override;		// // //
	bool ToBytes(std::byte *pBuf, std::size_t buflen) const override;
	bool FromBytes(array_view<std::byte> Buf) override;

public:
	// Clip info
	struct {
		int Channels = 0;
		int Frames = 0;
		int FirstChannel = 0;
		struct {
			int SourceRowStart = 0;
			int SourceRowEnd = 0;
		} OleInfo = { };
	} ClipInfo;

	// Clip data
	std::unique_ptr<int[]> pFrames;
	int iSize = 0;
};
