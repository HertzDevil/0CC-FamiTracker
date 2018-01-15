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

#include "FrameEditorTypes.h"
#include <algorithm>
#include "SongData.h"
#include "FrameClipData.h"
#include "ChannelMap.h"

// // // CFrameSelection class

CFrameSelection::CFrameSelection(const CFrameCursorPos &cursor) :
	m_cpStart(cursor), m_cpEnd {cursor.m_iFrame + 1, cursor.m_iChannel + 1}
{
}

CFrameSelection::CFrameSelection(const CFrameCursorPos &b, const CFrameCursorPos &e) :
	m_cpStart(b), m_cpEnd(e)
{
}

CFrameSelection::CFrameSelection(const CFrameClipData &clipdata, int frame) :
	m_cpStart {frame, clipdata.ClipInfo.FirstChannel},
	m_cpEnd {frame + clipdata.ClipInfo.Frames, clipdata.ClipInfo.FirstChannel + clipdata.ClipInfo.Channels}
{
}

CFrameSelection CFrameSelection::Including(const CFrameCursorPos &lhs, const CFrameCursorPos &rhs) {
	return {
		{
			std::min(lhs.m_iFrame, rhs.m_iFrame),
			std::min(lhs.m_iChannel, rhs.m_iChannel),
		},
		{
			std::max(lhs.m_iFrame, rhs.m_iFrame) + 1,
			std::max(lhs.m_iChannel, rhs.m_iChannel) + 1,
		},
	};
}

int CFrameSelection::GstFirstSelectedFrame() const
{
	return std::min(m_cpStart.m_iFrame, m_cpEnd.m_iFrame);
}

int CFrameSelection::GetLastSelectedFrame() const
{
	return std::max(m_cpStart.m_iFrame, m_cpEnd.m_iFrame) - 1;
}

int CFrameSelection::GetSelectedFrameCount() const {
	return std::abs(m_cpEnd.m_iFrame - m_cpStart.m_iFrame);
}

int CFrameSelection::GetFirstSelectedChannel() const
{
	return std::min(m_cpStart.m_iChannel, m_cpEnd.m_iChannel);
}

int CFrameSelection::GetLastSelectedChannel() const
{
	return std::max(m_cpStart.m_iChannel, m_cpEnd.m_iChannel) - 1;
}

int CFrameSelection::GetSelectedChanCount() const {
	return std::abs(m_cpEnd.m_iChannel - m_cpStart.m_iChannel);
}

CFrameCursorPos CFrameSelection::GetCursorStart() const {
	return {GstFirstSelectedFrame(), GetFirstSelectedChannel()};
}

CFrameCursorPos CFrameSelection::GetCursorEnd() const {
	return {GetLastSelectedFrame(), GetLastSelectedChannel()};
}

bool CFrameSelection::IncludesFrame(int frame) const {
	return frame >= GstFirstSelectedFrame() && frame <= GetLastSelectedFrame();
}

bool CFrameSelection::IncludesChannel(int channel) const {
	return channel >= GetFirstSelectedChannel() && channel <= GetLastSelectedChannel();
}

void CFrameSelection::Normalize() {
	if (m_cpStart.m_iFrame > m_cpEnd.m_iFrame)
		std::swap(m_cpStart.m_iFrame, m_cpEnd.m_iFrame);
	if (m_cpStart.m_iChannel > m_cpEnd.m_iChannel)
		std::swap(m_cpStart.m_iChannel, m_cpEnd.m_iChannel);
}

void CFrameSelection::Normalize(CFrameCursorPos &Begin, CFrameCursorPos &End) const
{
	Begin = {GstFirstSelectedFrame(), GetFirstSelectedChannel()};
	End = {GetLastSelectedFrame() + 1, GetLastSelectedChannel() + 1};
}

CFrameSelection CFrameSelection::GetNormalized() const
{
	CFrameSelection Sel;
	Normalize(Sel.m_cpStart, Sel.m_cpEnd);
	return Sel;
}

// // // CFrameIterator class

CFrameIterator::CFrameIterator(const CChannelMap &map, CSongData &song, const CFrameCursorPos &Pos) :
	CFrameCursorPos(Pos), map_(map), song_(song)
{
}

std::pair<CFrameIterator, CFrameIterator> CFrameIterator::FromSelection(const CFrameSelection &Sel, const CChannelMap &map, CSongData &song)
{
	CFrameCursorPos it, end;
	Sel.Normalize(it, end);
	return std::make_pair(
		CFrameIterator {map, song, it},
		CFrameIterator {map, song, end}
	);
}

int CFrameIterator::Get(int Channel) const
{
	return song_.GetFramePattern(NormalizeFrame(m_iFrame), map_.GetChannelType(Channel));
}

void CFrameIterator::Set(int Channel, int Frame)
{
	song_.SetFramePattern(NormalizeFrame(m_iFrame), map_.GetChannelType(Channel), Frame);
}

CFrameIterator &CFrameIterator::operator+=(const int Frames)
{
	m_iFrame = NormalizeFrame(m_iFrame + Frames);
	return *this;
}

CFrameIterator &CFrameIterator::operator-=(const int Frames)
{
	return operator+=(-Frames);
}

CFrameIterator &CFrameIterator::operator++()
{
	return operator+=(1);
}

CFrameIterator CFrameIterator::operator++(int)
{
	CFrameIterator tmp(*this);
	operator+=(1);
	return tmp;
}

CFrameIterator &CFrameIterator::operator--()
{
	return operator+=(-1);
}

CFrameIterator CFrameIterator::operator--(int)
{
	CFrameIterator tmp(*this);
	operator+=(-1);
	return tmp;
}

bool CFrameIterator::operator==(const CFrameIterator &other) const
{
	return m_iFrame == other.m_iFrame;
}

bool CFrameIterator::operator!=(const CFrameIterator &other) const
{
	return m_iFrame != other.m_iFrame;
}

int CFrameIterator::NormalizeFrame(int Frame) const
{
	int Frames = song_.GetFrameCount();
	return (Frame % Frames + Frames) % Frames;
}
