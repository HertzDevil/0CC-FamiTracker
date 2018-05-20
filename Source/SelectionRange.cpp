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
	return {m_iRow, m_iChannel, m_iColumn, m_iFrame};
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


/*
CPatternRowRange::ConstIterator::value_type CPatternRowRange::ConstIterator::operator*() const {
	const stChanNote &note = view_.GetTrack(track_)->GetPatternOnFrame(TranslateFrame()).GetNoteOn(pos_.Row);
	return {note, cb_, ce_};
}

CPatternRowRange::ConstIterator &CPatternRowRange::ConstIterator::operator+=(CPatternRowRange::ConstIterator::difference_type offset) {
	pos_.Row += offset;
	Warp();
	return *this;
}

CPatternRowRange::ConstIterator &CPatternRowRange::ConstIterator::operator-=(CPatternRowRange::ConstIterator::difference_type offset) {
	return operator+=(-offset);
}

CPatternRowRange::ConstIterator &CPatternRowRange::ConstIterator::operator++() {
	return operator+=(1);
}

CPatternRowRange::ConstIterator &CPatternRowRange::ConstIterator::operator--() {
	return operator-=(1);
}

CPatternRowRange::ConstIterator CPatternRowRange::ConstIterator::operator+(CPatternRowRange::ConstIterator::difference_type offset) const {
	auto it = *this;
	return it += offset;
}

CPatternRowRange::ConstIterator CPatternRowRange::ConstIterator::operator-(CPatternRowRange::ConstIterator::difference_type offset) const {
	auto it = *this;
	return it -= offset;
}

CPatternRowRange::ConstIterator CPatternRowRange::ConstIterator::operator++(int) {
	auto it = *this;
	++*this;
	return it;
}

CPatternRowRange::ConstIterator CPatternRowRange::ConstIterator::operator--(int) {
	auto it = *this;
	--*this;
	return it;
}

int CPatternRowRange::ConstIterator::TranslateFrame() const {
	int frames = view_.GetSong().GetFrameCount();
	int f = pos_.Frame % frames;
	return f < 0 ? f + frames : f;
}

void CPatternRowRange::ConstIterator::Warp() {
	if (pos_.Row >= 0) {
		while (true) {
			if (int Length = view_.GetFrameLength(TranslateFrame()); pos_.Row >= Length) {
				pos_.Row -= Length;
				++pos_.Frame;
			}
			else
				return;
		}
	}
	else
		while (pos_.Row < 0) {
			--pos_.Frame;
			pos_.Row += view_.GetFrameLength(TranslateFrame());
		}
}
*/
