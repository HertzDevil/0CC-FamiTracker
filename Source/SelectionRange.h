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
namespace ft0cc::doc {
class pattern_note;
} // namespace ft0cc::doc

class CPatternIterator {
public:
	CPatternIterator(CSongView &view, const CCursorPos &Pos);
//	CPatternIterator(const CSongView &view, const CCursorPos &Pos);

	static std::pair<CPatternIterator, CPatternIterator> FromCursor(const CCursorPos &Pos, CSongView &view);
	static std::pair<CPatternIterator, CPatternIterator> FromSelection(const CSelection &Sel, CSongView &view);

	CCursorPos GetCursor() const;

	const ft0cc::doc::pattern_note &Get(int Channel) const;
	void Set(int Channel, const ft0cc::doc::pattern_note &Note);

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

/*
// range of rows, track and column are fixed
class CPatternRowRange {
public:
	class ConstIterator {
	public:
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = std::tuple<const ft0cc::doc::pattern_note &, column_t, column_t>;
		using reference = value_type &;
		using pointer = value_type *;
		using difference_type = int;

		constexpr ConstIterator(stRowPos pos, CConstSongView &view, int track, column_t cb, column_t ce) noexcept :
			view_(view), pos_(pos), track_(track), cb_(cb), ce_(ce) { }

		constexpr int compare(const ConstIterator &other) const noexcept {
			return pos_.compare(other.pos_);
		}

		value_type operator*() const;

		ConstIterator &operator+=(difference_type offset);
		ConstIterator &operator-=(difference_type offset);
		ConstIterator &operator++();
		ConstIterator &operator--();

		ConstIterator operator+(difference_type offset) const;
		ConstIterator operator-(difference_type offset) const;
		ConstIterator operator++(int);
		ConstIterator operator--(int);

	private:
		int TranslateFrame() const;
		void Warp();

		CConstSongView &view_;
		stRowPos pos_;
		int track_;
		column_t cb_;
		column_t ce_;
	};

};

ENABLE_CX_STRONG_ORDERING(CPatternRowRange::ConstIterator);

// range of columns, frame and row are fixed
class CPatternColumnRange {
public:
	class ConstIterator {
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = const ft0cc::doc::pattern_note;
		using reference = value_type &;
		using pointer = value_type *;
		using difference_type = int;

		constexpr ConstIterator(stColumnPos pos, CConstSongView &view, stRowPos row) noexcept :
			view_(view), pos_(pos), row_(row) { }

		constexpr int compare(const ConstIterator &other) const noexcept {
			return pos_.compare(other.pos_);
		}

	private:
		CConstSongView &view_;
		stColumnPos pos_;
		stRowPos row_;
	};

};

ENABLE_CX_STRONG_ORDERING(CPatternColumnRange::ConstIterator);
*/
