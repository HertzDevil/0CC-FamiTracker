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
#include <utility>
#include <algorithm>		// // //
#include "ft0cc/enum_traits.h"		// // //
#include "StrongOrdering.h"		// /// //

// Helper types for the pattern editor

// // // Scroll modes
enum scroll_dir_t {
	SCROLL_NONE  = 0x00,
	SCROLL_UP    = 0x01,
	SCROLL_DOWN  = 0x02,
	SCROLL_RIGHT = 0x04,
	SCROLL_LEFT  = 0x08,
};

// // // Selection scope
enum sel_scope_t {
	SEL_SCOPE_NONE   = 0x00,
	SEL_SCOPE_VROW   = 0x01,
	SEL_SCOPE_VFRAME = 0x02,
	SEL_SCOPE_VTRACK = 0x03,
	SEL_SCOPE_HCOL   = 0x10,
	SEL_SCOPE_HCHAN  = 0x20,
	SEL_SCOPE_HFRAME = 0x30,
};

// Cursor columns
enum class cursor_column_t : unsigned {		// // // moved from FamiTrackerDoc.h
	NOTE,
	INSTRUMENT1,
	INSTRUMENT2,
	VOLUME,
	EFF1_NUM,
	EFF1_PARAM1,
	EFF1_PARAM2,
	EFF2_NUM,
	EFF2_PARAM1,
	EFF2_PARAM2,
	EFF3_NUM,
	EFF3_PARAM1,
	EFF3_PARAM2,
	EFF4_NUM,
	EFF4_PARAM1,
	EFF4_PARAM2,
};

// Column layout
enum class column_t : unsigned {		// // //
	Note,
	Instrument,
	Volume,
	Effect1,
	Effect2,
	Effect3,
	Effect4,
};
const unsigned int COLUMNS = 7;		// // // moved from FamiTrackerDoc.h

// // // moved from PatternEditor.h

// Return first column for a specific column field
constexpr column_t GetSelectColumn(cursor_column_t Column) {		// // //
	constexpr column_t COLUMN_INDICES[] = {
		column_t::Note,
		column_t::Instrument, column_t::Instrument,
		column_t::Volume,
		column_t::Effect1, column_t::Effect1, column_t::Effect1,
		column_t::Effect2, column_t::Effect2, column_t::Effect2,
		column_t::Effect3, column_t::Effect3, column_t::Effect3,
		column_t::Effect4, column_t::Effect4, column_t::Effect4,
	};

	return COLUMN_INDICES[value_cast(Column)];
}

constexpr cursor_column_t GetCursorStartColumn(column_t Column) {
	constexpr cursor_column_t COL_START[] = {
		cursor_column_t::NOTE,
		cursor_column_t::INSTRUMENT1,
		cursor_column_t::VOLUME,
		cursor_column_t::EFF1_NUM,
		cursor_column_t::EFF2_NUM,
		cursor_column_t::EFF3_NUM,
		cursor_column_t::EFF4_NUM,
	};

	return COL_START[value_cast(Column)];
}

constexpr cursor_column_t GetCursorEndColumn(column_t Column) {
	constexpr cursor_column_t COL_END[] = {
		cursor_column_t::NOTE,
		cursor_column_t::INSTRUMENT2,
		cursor_column_t::VOLUME,
		cursor_column_t::EFF1_PARAM2,
		cursor_column_t::EFF2_PARAM2,
		cursor_column_t::EFF3_PARAM2,
		cursor_column_t::EFF4_PARAM2,
	};

	return COL_END[value_cast(Column)];
}

// // // Paste modes
enum class paste_mode_t : unsigned int {
	DEFAULT,
	MIX,
	OVERWRITE,
	INSERT,
};

enum class paste_pos_t : unsigned int {
	CURSOR,
	SELECTION,
	FILL,
	DRAG,
};

// // // Selection condition
enum class sel_condition_t : unsigned char {
	CLEAN,				// safe for operations
	REPEATED_ROW,		// same row included in multiple frames
	// UNKNOWN_SIZE,	// skip effect outside selection, unused
	NONTERMINAL_SKIP,	// skip effect on non-terminal row
	TERMINAL_SKIP,		// skip effect on last row
};

struct stRowPos {
	int Frame = 0;
	int Row = 0;

	constexpr int compare(const stRowPos &other) const noexcept {
		return Frame < other.Frame ? -1 : Frame > other.Frame ? 1 :
			Row < other.Row ? -1 : Row > other.Row ? 1 : 0;
	}
};
ENABLE_STRONG_ORDERING(stRowPos);

struct stColumnPos {
	int Track = 0;
	cursor_column_t Column = cursor_column_t::NOTE;

	constexpr int compare(const stColumnPos &other) const noexcept {
		return Track < other.Track ? -1 : Track > other.Track ? 1 :
			Column < other.Column ? -1 : Column > other.Column ? 1 : 0;
	}
};
ENABLE_STRONG_ORDERING(stColumnPos);

// Cursor position
struct CCursorPos {
	constexpr CCursorPos() noexcept = default;
	constexpr CCursorPos(int Row, int Track, cursor_column_t Column, int Frame) noexcept :		// // //
		Ypos {Frame, Row}, Xpos {Track, Column}
	{
	}

	constexpr bool operator==(const CCursorPos &other) const noexcept {		// // //
		return Ypos == other.Ypos && Xpos == other.Xpos;
	}
	constexpr bool operator!=(const CCursorPos &other) const noexcept {
		return !(*this == other);
	}

	constexpr int GetFrame() const noexcept {
		return Ypos.Frame;
	}
	constexpr int GetRow() const noexcept {
		return Ypos.Row;
	}
	constexpr int GetTrack() const noexcept {
		return Xpos.Track;
	}
	constexpr cursor_column_t GetColumn() const noexcept {
		return Xpos.Column;
	}

	constexpr bool IsAbove(const CCursorPos &other) const noexcept {
		return Ypos < other.Ypos;
	}

	constexpr bool IsValid(int RowCount, int TrackCount) const noexcept {		// // //
		// Check if a valid pattern position
		//if (Ypos.Frame < -FrameCount || Ypos.Frame >= 2 * FrameCount)		// // //
		//	return false;
		if (Xpos.Track < 0 || Xpos.Track >= TrackCount)
			return false;
		if (Ypos.Row < 0 || Ypos.Row >= RowCount)
			return false;
		if (Xpos.Column > cursor_column_t::EFF4_PARAM2)		// // //
			return false;

		return true;
	}

public:
	stRowPos Ypos;
	stColumnPos Xpos;
};

// Selection
struct CSelection {
	constexpr int GetRowStart() const noexcept {
		return std::min(m_cpStart.Ypos, m_cpEnd.Ypos).Row;
	}
	constexpr int GetRowEnd() const noexcept {
		return std::max(m_cpStart.Ypos, m_cpEnd.Ypos).Row;
	}
	constexpr cursor_column_t GetColStart() const noexcept {
		return GetCursorStartColumn(GetSelectColumn(std::min(m_cpStart.Xpos, m_cpEnd.Xpos).Column));
	}		// // //
	constexpr cursor_column_t GetColEnd() const noexcept {
		return GetCursorEndColumn(GetSelectColumn(std::max(m_cpStart.Xpos, m_cpEnd.Xpos).Column));
	}		// // //
	constexpr int GetChanStart() const noexcept {
		return std::min(m_cpStart.Xpos.Track, m_cpEnd.Xpos.Track);
	}
	constexpr int GetChanEnd() const noexcept {
		return std::max(m_cpStart.Xpos.Track, m_cpEnd.Xpos.Track);
	}
	constexpr int GetFrameStart() const noexcept {		// // //
		return std::min(m_cpStart.Ypos.Frame, m_cpEnd.Ypos.Frame);
	}		// // //
	constexpr int GetFrameEnd() const noexcept {		// // //
		return std::max(m_cpStart.Ypos.Frame, m_cpEnd.Ypos.Frame);
	}		// // //

	constexpr bool IsSameStartPoint(const CSelection &other) const noexcept {
		return GetChanStart() == other.GetChanStart() &&
			GetRowStart() == other.GetRowStart() &&
			GetColStart() == other.GetColStart() &&
			GetFrameStart() == other.GetFrameStart();		// // //
	}
	constexpr bool IsColumnSelected(column_t Column, int Channel) const noexcept {
		column_t SelStart = GetSelectColumn(GetColStart());		// // //
		column_t SelEnd = GetSelectColumn(GetColEnd());

		return (Channel > GetChanStart() || (Channel == GetChanStart() && Column >= SelStart))		// // //
			&& (Channel < GetChanEnd() || (Channel == GetChanEnd() && Column <= SelEnd));
	}

	constexpr void Normalize(CCursorPos &Begin, CCursorPos &End) const noexcept {		// // //
		CCursorPos Temp {GetRowStart(), GetChanStart(), GetColStart(), GetFrameStart()};
		End = CCursorPos {GetRowEnd(), GetChanEnd(), GetColEnd(), GetFrameEnd()};
		Begin = Temp;
	}
	constexpr CSelection GetNormalized() const noexcept {		// // //
		CSelection Sel;
		Normalize(Sel.m_cpStart, Sel.m_cpEnd);
		return Sel;
	}

public:
	CCursorPos m_cpStart;
	CCursorPos m_cpEnd;
};

/*
// Pattern layout
class CPatternEditorLayout {
public:
	CPatternEditorLayout();

	void SetSize(int Width, int Height);
	void CalculateLayout();

private:
	int		m_iPatternWidth;				// Width of channels in pattern area
	int		m_iPatternHeight;				// Height of channels in pattern area
	int		m_iLinesVisible;				// Number of lines visible on screen (may include one incomplete line)
	int		m_iLinesFullVisible;			// Number of lines full visible on screen
	int		m_iChannelsVisible;				// Number of channels visible on screen (may include one incomplete channel)
	int		m_iChannelsFullVisible;			// Number of channels full visible on screen
	int		m_iFirstChannel;				// First drawn channel
	int		m_iRowHeight;					// Height of each row in pixels

	int		m_iChannelWidths[MAX_CHANNELS];	// Cached width in pixels of each channel
	int		m_iChannelOffsets[MAX_CHANNELS];// Cached x position of channels
	int		m_iColumns[MAX_CHANNELS];		// Cached number of columns in each channel
};

*/
