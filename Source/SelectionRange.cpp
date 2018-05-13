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

#include "SelectionRange.h"
#include "PatternEditorTypes.h"
#include "SongView.h"
#include "SongData.h"

CPatternIterator::CPatternIterator(CSongView &view, const CCursorPos &Pos) :
	m_iFrame(Pos.Ypos.Frame),
	m_iRow(Pos.Ypos.Row),
	m_iColumn(Pos.Xpos.Column),
	m_iChannel(Pos.Xpos.Track),
	song_view_(view)
{
	Warp();
}

std::pair<CPatternIterator, CPatternIterator> CPatternIterator::FromCursor(const CCursorPos &Pos, CSongView &view)
{
	return std::make_pair(
		CPatternIterator {view, Pos},
		CPatternIterator {view, Pos}
	);
}

std::pair<CPatternIterator, CPatternIterator> CPatternIterator::FromSelection(const CSelection &Sel, CSongView &view)
{
	CCursorPos it, end;
	Sel.Normalize(it, end);
	return std::make_pair(
		CPatternIterator {view, it},
		CPatternIterator {view, end}
	);
}

CCursorPos CPatternIterator::GetCursor() const {
	return {m_iFrame, m_iRow, m_iColumn, m_iChannel};
}

const stChanNote &CPatternIterator::Get(int Channel) const
{
	return song_view_.GetPatternOnFrame(Channel, TranslateFrame()).GetNoteOn(m_iRow);
}

void CPatternIterator::Set(int Channel, const stChanNote &Note)
{
	song_view_.GetPatternOnFrame(Channel, TranslateFrame()).SetNoteOn(m_iRow, Note);
}

CPatternIterator &CPatternIterator::operator+=(int Rows)
{
	m_iRow += Rows;
	Warp();
	return *this;
}

CPatternIterator &CPatternIterator::operator-=(int Rows)
{
	return operator+=(-Rows);
}

CPatternIterator &CPatternIterator::operator++()
{
	return operator+=(1);
}

CPatternIterator CPatternIterator::operator++(int)
{
	CPatternIterator tmp(*this);
	operator+=(1);
	return tmp;
}

CPatternIterator &CPatternIterator::operator--()
{
	return operator+=(-1);
}

CPatternIterator CPatternIterator::operator--(int)
{
	CPatternIterator tmp(*this);
	operator+=(-1);
	return tmp;
}

bool CPatternIterator::operator==(const CPatternIterator &other) const
{
	return m_iFrame == other.m_iFrame && m_iRow == other.m_iRow;
}

bool CPatternIterator::operator<(const CPatternIterator &other) const		// // //
{
	return m_iFrame < other.m_iFrame || (m_iFrame == other.m_iFrame && m_iRow < other.m_iRow);
}

bool CPatternIterator::operator<=(const CPatternIterator &other) const		// // //
{
	return !(other < *this);
}

int CPatternIterator::TranslateFrame() const {
	int frames = song_view_.GetSong().GetFrameCount();
	int f = m_iFrame % frames;
	return f < 0 ? f + frames : f;
}

void CPatternIterator::Warp()
{
	if (m_iRow >= 0) {
		while (true) {
			if (int Length = song_view_.GetFrameLength(TranslateFrame()); m_iRow >= Length) {
				m_iRow -= Length;
				++m_iFrame;
			}
			else
				return;
		}
	}
	else
		while (m_iRow < 0) {
			--m_iFrame;
			m_iRow += song_view_.GetFrameLength(TranslateFrame());
		}
}
