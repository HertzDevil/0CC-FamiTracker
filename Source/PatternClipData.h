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
#include "PatternEditorTypes.h"

namespace ft0cc::doc {
class pattern_note;
} // namespace ft0cc::doc

// Class used by clipboard
class CPatternClipData : public CBinarySerializableInterface {		// // //
public:
	CPatternClipData() = default;
	CPatternClipData(unsigned Channels, unsigned Rows);

	ft0cc::doc::pattern_note *GetPattern(unsigned Channel, unsigned Row);
	const ft0cc::doc::pattern_note *GetPattern(unsigned Channel, unsigned Row) const;

	bool ContainsData() const override;		// // //

private:
	std::size_t GetAllocSize() const override;
	bool ToBytes(array_view<std::byte> Buf) const override;
	bool FromBytes(array_view<const std::byte> Buf) override;

public:
	struct stClipInfo {
		std::uint32_t Channels = 0;			// Number of channels
		std::uint32_t Rows = 0;				// Number of rows
		column_t StartColumn = column_t::Note;		// // // Start column in first channel
		column_t EndColumn = column_t::Note;		// // // End column in last channel
		struct stOleInfo {			// OLE drag and drop info
			std::uint32_t ChanOffset = 0;
			std::uint32_t RowOffset = 0;
		} OleInfo = { };

		constexpr std::size_t GetSize() const noexcept {
			return Channels * Rows;
		}
	} ClipInfo;

	std::unique_ptr<ft0cc::doc::pattern_note[]> pPattern;		// // // Pattern data
};

