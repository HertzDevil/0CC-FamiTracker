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

#include "FamiTrackerDefines.h"
#include "ft0cc/doc/pattern_note.hpp"
#include <array>
#include <memory>
#include "ft0cc/cpputil/iter.hpp"

// // // the real pattern class
class CPatternData {
	static constexpr unsigned max_size = MAX_PATTERN_LENGTH;

	using elem_t = std::array<ft0cc::doc::pattern_note, max_size>;

public:
	CPatternData() = default;
	CPatternData(const CPatternData &other);
	CPatternData(CPatternData &&other) noexcept = default;
	CPatternData &operator=(const CPatternData &other);
	CPatternData &operator=(CPatternData &&other) noexcept = default;
	~CPatternData() noexcept;

	ft0cc::doc::pattern_note &GetNoteOn(unsigned row);
	const ft0cc::doc::pattern_note &GetNoteOn(unsigned row) const;
	void SetNoteOn(unsigned row, const ft0cc::doc::pattern_note &note);

	bool operator==(const CPatternData &other) const noexcept;
	bool operator!=(const CPatternData &other) const noexcept;
//	explicit operator bool() const noexcept;

	unsigned GetMaximumSize() const noexcept;
	unsigned GetNoteCount(int rowcount) const;
	bool IsEmpty() const;

	iter_range<elem_t::iterator, elem_t::iterator> Rows();
	iter_range<elem_t::const_iterator, elem_t::const_iterator> Rows() const;
	iter_range<elem_t::iterator, elem_t::iterator> Rows(unsigned rowcount);
	iter_range<elem_t::const_iterator, elem_t::const_iterator> Rows(unsigned rowcount) const;

private:
	void Allocate();

	std::unique_ptr<elem_t> data_;
};
