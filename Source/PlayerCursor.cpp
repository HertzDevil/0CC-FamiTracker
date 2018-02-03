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

#include "PlayerCursor.h"
#include "SongData.h"

CPlayerCursor::CPlayerCursor(const CSongData &song, unsigned index) :
	song_(song), track_(index)
{
}

CPlayerCursor::CPlayerCursor(const CSongData &song, unsigned index, unsigned frame, unsigned row) :
	song_(song), track_(index), frame_(frame), row_(row)
{
}

const CSongData &CPlayerCursor::GetSong() const {
	return song_;
}

void CPlayerCursor::QueueFrame(unsigned frame) {
	queue_ = frame;
}

void CPlayerCursor::EnableFrameLoop() {
	loop_ = true;
}

void CPlayerCursor::Tick() {
	++tick_;
	++total_ticks_;
}

void CPlayerCursor::StepRow() {
	if (++row_ >= song_.GetPatternLength()) {
		row_ = 0;
		MoveToCheckedFrame(frame_ + 1);
	}
	++total_rows_;
	tick_ = 0;
}

void CPlayerCursor::SetPosition(unsigned frame, unsigned row) {
	frame_ = frame;
	row_ = row;
	tick_ = 0;
}

void CPlayerCursor::MoveToRow(unsigned Row) {
	row_ = Row;
	++total_rows_;
	tick_ = 0;
}

void CPlayerCursor::MoveToFrame(unsigned frame) {
	frame_ = frame % song_.GetFrameCount();
	++total_frames_;
}

void CPlayerCursor::MoveToCheckedFrame(unsigned frame) {
	MoveToFrame(queue_ ? DequeueFrame() : loop_ ? frame_ : frame);
}

void CPlayerCursor::DoBxx(unsigned frame) {
	const unsigned Max = song_.GetFrameCount() - 1;
	MoveToFrame(frame <= Max ? frame : Max);
	MoveToRow(0);
}

void CPlayerCursor::DoCxx() {
	++total_frames_;
}

void CPlayerCursor::DoDxx(unsigned row) {
	const unsigned Max = song_.GetPatternLength() - 1;
	MoveToCheckedFrame(frame_ + 1);
	MoveToRow(row <= Max ? row : Max);
}

unsigned CPlayerCursor::GetCurrentSong() const noexcept {
	return track_;
}

unsigned CPlayerCursor::GetCurrentFrame() const noexcept {
	return frame_;
}

unsigned CPlayerCursor::GetCurrentRow() const noexcept {
	return row_;
}

unsigned CPlayerCursor::GetCurrentTick() const noexcept {
	return tick_;
}

unsigned CPlayerCursor::GetTotalFrames() const noexcept {
	return total_frames_;
}

unsigned CPlayerCursor::GetTotalRows() const noexcept {
	return total_rows_;
}

unsigned CPlayerCursor::GetTotalTicks() const noexcept {
	return total_ticks_;
}

std::optional<unsigned> CPlayerCursor::GetQueuedFrame() const noexcept {
	return queue_;
}

unsigned CPlayerCursor::DequeueFrame() {
	auto frame = *queue_;
	queue_.reset();
	return frame;
}
