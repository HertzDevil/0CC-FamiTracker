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

#include "FrameEditorModel.h"
#include "FrameClipData.h"
#include "FamiTrackerView.h"
#include "SongData.h"
#include "SongView.h"

void CFrameEditorModel::AssignView(CFamiTrackerView &view) {
	view_ = &view;
}

void CFrameEditorModel::DoClick(int frame, int channel) {
	clicking_ = true;


}

void CFrameEditorModel::DoMove(int frame, int channel) {
	if (!clicking_)
		return;


}

void CFrameEditorModel::DoUnclick(int frame, int channel) {


	clicking_ = false;
}

int CFrameEditorModel::GetCurrentFrame() const {
	return view_->GetSelectedFrame();
}

int CFrameEditorModel::GetCurrentChannel() const {
	return view_->GetSelectedChannel();
}

CFrameCursorPos CFrameEditorModel::GetCurrentPos() const {
	return {GetCurrentFrame(), GetCurrentChannel()};
}

void CFrameEditorModel::SetCurrentFrame(int frame) {
	view_->SelectFrame(frame);
}

void CFrameEditorModel::SetCurrentChannel(int channel) {
	view_->SelectChannel(channel);
}

bool CFrameEditorModel::IsSelecting() const {
	return m_bSelecting;
}

const CFrameSelection *CFrameEditorModel::GetSelection() const {
	return m_bSelecting ? &m_selection : nullptr;
}

CFrameSelection CFrameEditorModel::MakeFrameSelection(int frame) const {
	CSongView *pSongView = view_->GetSongView();
	return {
		{frame, 0},
		{frame + 1, (int)pSongView->GetChannelOrder().GetChannelCount()},
	};
}

CFrameSelection CFrameEditorModel::MakeFullSelection() const {
	CSongView *pSongView = view_->GetSongView();
	return {
		{0, 0},
		{(int)pSongView->GetSong().GetFrameCount(), (int)pSongView->GetChannelOrder().GetChannelCount()},
	};
}

CFrameSelection CFrameEditorModel::GetActiveSelection() const {
	return IsSelecting() ? m_selection : MakeFrameSelection(GetCurrentFrame());
}

void CFrameEditorModel::Select(const CFrameSelection &sel) {
	m_selection = sel;
	m_bSelecting = true;
}

void CFrameEditorModel::Deselect() {
	m_bSelecting = false;
}

void CFrameEditorModel::StartSelection(const CFrameCursorPos &pos) {
	selStart_ = selEnd_ = pos;
	Select(selStart_);		// // //
}

void CFrameEditorModel::ContinueSelection(const CFrameCursorPos &pos) {
	if (IsSelecting()) {
		selEnd_ = pos;
		Select(CFrameSelection::Including(selStart_, selEnd_));
	}
}

void CFrameEditorModel::ContinueFrameSelection(int frame) {
	if (IsSelecting()) {
		CSongView *pSongView = view_->GetSongView();
		selStart_.m_iChannel = 0;
		selEnd_ = {frame, (int)pSongView->GetChannelOrder().GetChannelCount()};
		Select(CFrameSelection::Including(selStart_, selEnd_));
	}
}

bool CFrameEditorModel::IsFrameSelected(int frame) const {
	return IsSelecting() && m_selection.IncludesFrame(frame);
}

bool CFrameEditorModel::IsChannelSelected(int channel) const {
	return IsSelecting() && m_selection.IncludesChannel(channel);
}

CFrameClipData CFrameEditorModel::CopySelection(const CFrameSelection &sel) const {
	auto [b, e] = CFrameIterator::FromSelection(sel, *view_->GetSongView());

	CFrameClipData Data {sel.GetSelectedChanCount(), sel.GetSelectedFrameCount()};
	Data.ClipInfo.FirstChannel = b.m_iChannel;		// // //
	Data.ClipInfo.OleInfo.SourceRowStart = b.m_iFrame;
	Data.ClipInfo.OleInfo.SourceRowEnd = e.m_iFrame - 1;

	int f = 0;
	for (; b != e; ++f) {
		for (int c = b.m_iChannel; c < e.m_iChannel; ++c)
			Data.SetFrame(f, c - b.m_iChannel, b.Get(c));
		++b;
		if (b.m_iFrame == 0)
			break;
	}

	return Data;
}

void CFrameEditorModel::PasteSelection(const CFrameClipData &clipdata, const CFrameCursorPos &pos) {
	CFrameIterator it {*view_->GetSongView(), pos};
	for (int f = 0; f < clipdata.ClipInfo.Frames; ++f) {
		for (int c = 0; c < clipdata.ClipInfo.Channels; ++c)
			it.Set(c + /*it.m_iChannel*/ clipdata.ClipInfo.FirstChannel, clipdata.GetFrame(f, c));
		++it;
		if (it.m_iFrame == 0)
			break;
	}
}
