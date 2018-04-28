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
#include "ft0cc/enum_traits.h"		// // //

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
enum cursor_column_t : unsigned int {		// // // moved from FamiTrackerDoc.h
	C_NOTE,
	C_INSTRUMENT1,
	C_INSTRUMENT2,
	C_VOLUME,
	C_EFF1_NUM,
	C_EFF1_PARAM1,
	C_EFF1_PARAM2,
	C_EFF2_NUM,
	C_EFF2_PARAM1,
	C_EFF2_PARAM2,
	C_EFF3_NUM,
	C_EFF3_PARAM1,
	C_EFF3_PARAM2,
	C_EFF4_NUM,
	C_EFF4_PARAM1,
	C_EFF4_PARAM2,
};

// Column layout
enum class column_t : unsigned int {		// // //
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

inline column_t GetSelectColumn(cursor_column_t Column)
{
	// Return first column for a specific column field
	static const column_t COLUMN_INDICES[] = {
		column_t::Note,
		column_t::Instrument, column_t::Instrument,
		column_t::Volume,
		column_t::Effect1, column_t::Effect1, column_t::Effect1,
		column_t::Effect2, column_t::Effect2, column_t::Effect2,
		column_t::Effect3, column_t::Effect3, column_t::Effect3,
		column_t::Effect4, column_t::Effect4, column_t::Effect4,
	};

	return COLUMN_INDICES[Column];
}

inline cursor_column_t GetCursorStartColumn(column_t Column)
{
	static const cursor_column_t COL_START[] = {
		C_NOTE, C_INSTRUMENT1, C_VOLUME, C_EFF1_NUM, C_EFF2_NUM, C_EFF3_NUM, C_EFF4_NUM,
	};

	return COL_START[value_cast(Column)];
}

inline cursor_column_t GetCursorEndColumn(column_t Column)
{
	static const cursor_column_t COL_END[] = {
		C_NOTE, C_INSTRUMENT2, C_VOLUME, C_EFF1_PARAM2, C_EFF2_PARAM2, C_EFF3_PARAM2, C_EFF4_PARAM2,
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

class CSongData;		// // //
class CSongView;		// // //
class stChanNote;

class CFamiTrackerDoc;

// Cursor position
class CCursorPos {
public:
	CCursorPos();
	CCursorPos(int Row, int Channel, cursor_column_t Column, int Frame);		// // //
	bool operator!=(const CCursorPos &other) const;
	bool operator<(const CCursorPos &other) const;
	bool operator<=(const CCursorPos &other) const;
	bool IsValid(int RowCount, int ChannelCount) const;		// // //

public:
	int m_iFrame;		// // //
	int m_iRow;
	cursor_column_t m_iColumn;		// // //
	int m_iChannel;
};

// Selection
class CSelection {
public:
	int  GetRowStart() const;
	int  GetRowEnd() const;
	cursor_column_t GetColStart() const;		// // //
	cursor_column_t GetColEnd() const;		// // //
	int  GetChanStart() const;
	int  GetChanEnd() const;
	int  GetFrameStart() const;		// // //
	int  GetFrameEnd() const;		// // //

	bool IsSameStartPoint(const CSelection &selection) const;
	bool IsColumnSelected(column_t Column, int Channel) const;

	void Normalize(CCursorPos &Begin, CCursorPos &End) const;		// // //
	CSelection GetNormalized() const;		// // //
public:
	CCursorPos m_cpStart;
	CCursorPos m_cpEnd;
};

class CPatternIterator : public CCursorPos {		// // //
public:
	CPatternIterator(CSongView &view, const CCursorPos &Pos);
//	CPatternIterator(const CSongView &view, const CCursorPos &Pos);

	static std::pair<CPatternIterator, CPatternIterator> FromCursor(const CCursorPos &Pos, CSongView &view);
	static std::pair<CPatternIterator, CPatternIterator> FromSelection(const CSelection &Sel, CSongView &view);

	const stChanNote &Get(int Channel) const;
	void Set(int Channel, const stChanNote &Note);
	void Step();

	CPatternIterator &operator+=(const int Rows);
	CPatternIterator &operator-=(const int Rows);
	CPatternIterator &operator++();
	CPatternIterator operator++(int);
	CPatternIterator &operator--();
	CPatternIterator operator--(int);
	bool operator==(const CPatternIterator &other) const;

protected:
	int TranslateFrame() const;

private:
	void Warp();

protected:
	CSongView &song_view_;
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
