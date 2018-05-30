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
	using value_type = std::int32_t;

public:
	CFrameClipData() = default;
	CFrameClipData(unsigned Channels, unsigned Frames);

	CFrameSelection AsSelection(unsigned startFrame) const;		// // //

	int  GetFrame(unsigned Frame, unsigned Channel) const;
	void SetFrame(unsigned Frame, unsigned Channel, value_type Pattern);

	bool ContainsData() const override;		// // //

private:
	std::size_t GetAllocSize() const override;
	bool ToBytes(array_view<std::byte> Buf) const override;
	bool FromBytes(array_view<const std::byte> Buf) override;

public:
	// Clip info
	struct stClipInfo {
		std::uint32_t Channels = 0;
		std::uint32_t Frames = 0;
		std::uint32_t FirstChannel = 0;
		struct stOleInfo {
			std::uint32_t SourceRowStart = 0;
			std::uint32_t SourceRowEnd = 0;
		} OleInfo = { };

		constexpr std::size_t GetSize() const noexcept {
			return Channels * Frames;
		}
	} ClipInfo;

	// Clip data
	std::unique_ptr<value_type[]> pFrames;
};
