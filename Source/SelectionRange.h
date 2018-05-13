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

#include <utility>
#include <iterator>
#include <tuple>
#include "PatternEditorTypes.h"

class CConstSongView;
class CSongView;
class stChanNote;

class CPatternIterator {
public:
	CPatternIterator(CSongView &view, const CCursorPos &Pos);
//	CPatternIterator(const CSongView &view, const CCursorPos &Pos);

	static std::pair<CPatternIterator, CPatternIterator> FromCursor(const CCursorPos &Pos, CSongView &view);
	static std::pair<CPatternIterator, CPatternIterator> FromSelection(const CSelection &Sel, CSongView &view);

	CCursorPos GetCursor() const;

	const stChanNote &Get(int Channel) const;
	void Set(int Channel, const stChanNote &Note);

	CPatternIterator &operator+=(int Rows);
	CPatternIterator &operator-=(int Rows);
	CPatternIterator &operator++();
	CPatternIterator operator++(int);
	CPatternIterator &operator--();
	CPatternIterator operator--(int);

	bool operator==(const CPatternIterator &other) const;
	bool operator<(const CPatternIterator &other) const;
	bool operator<=(const CPatternIterator &other) const;

	int m_iFrame;		// // //
	int m_iRow;
	cursor_column_t m_iColumn;		// // //
	int m_iChannel;

protected:
	int TranslateFrame() const;

private:
	void Warp();

protected:
	CSongView &song_view_;
};
