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

class stChanNote;

// Class used by clipboard
class CPatternClipData : public CBinarySerializableInterface {		// // //
public:
	CPatternClipData() = default;
	CPatternClipData(int Channels, int Rows);

	stChanNote *GetPattern(int Channel, int Row);
	const stChanNote *GetPattern(int Channel, int Row) const;

private:
	std::size_t GetAllocSize() const override;
	bool ContainsData() const override;		// // //
	bool ToBytes(unsigned char *pBuf) const override;
	bool FromBytes(const unsigned char *pBuf) override;

public:
	struct {
		int Channels = 0;			// Number of channels
		int Rows = 0;				// Number of rows
		column_t StartColumn = column_t::Note;		// // // Start column in first channel
		column_t EndColumn = column_t::Note;		// // // End column in last channel
		struct {				// OLE drag and drop info
			int ChanOffset = 0;
			int RowOffset = 0;
		} OleInfo = { };
	} ClipInfo;

	std::unique_ptr<stChanNote[]> pPattern;		// // // Pattern data
	int Size = 0;					// Pattern data size, in rows * columns
};

