/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"

void CFrameEditorModel::AssignDocument(CFamiTrackerDoc &doc, CFamiTrackerView &view) {
	doc_ = &doc;
	view_ = &view;
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
	CFrameSelection Sel;
	Sel.m_cpStart.m_iFrame = Sel.m_cpEnd.m_iFrame = frame;
	Sel.m_cpEnd.m_iChannel = doc_->GetChannelCount() - 1;
	return Sel;
}

CFrameSelection CFrameEditorModel::MakeFullSelection(int track) const {
	CFrameSelection Sel;
	Sel.m_cpEnd.m_iFrame = doc_->GetFrameCount(track) - 1;
	Sel.m_cpEnd.m_iChannel = doc_->GetChannelCount() - 1;
	return Sel;
}

CFrameSelection CFrameEditorModel::GetActiveSelection() const {
	return IsSelecting() ? m_selection : MakeFrameSelection(view_->GetSelectedFrame());
}

void CFrameEditorModel::Select(const CFrameSelection &sel) {
	m_selection = sel;
	m_bSelecting = true;
}

void CFrameEditorModel::Deselect() {
	m_bSelecting = false;
}

void CFrameEditorModel::StartSelection(int frame, int channel) {
	m_bSelecting = true;
	m_selection.m_cpStart.m_iFrame = m_selection.m_cpEnd.m_iFrame = frame;		// // //
	m_selection.m_cpStart.m_iChannel = m_selection.m_cpEnd.m_iChannel = channel;		// // //
}

void CFrameEditorModel::ContinueSelection(int frame, int channel) {
	if (IsSelecting())
		m_selection.m_cpEnd = CFrameCursorPos(frame, channel);
}

void CFrameEditorModel::ContinueFrameSelection(int frame) {
	if (IsSelecting()) {
		m_selection.m_cpEnd.m_iFrame = frame;
		m_selection.m_cpStart.m_iChannel = 0;
		m_selection.m_cpEnd.m_iChannel = doc_->GetChannelCount() - 1;
	}
}

bool CFrameEditorModel::IsFrameSelected(int frame) const {
	return IsSelecting() && frame >= m_selection.GetFrameStart() && frame <= m_selection.GetFrameEnd();
}

bool CFrameEditorModel::IsChannelSelected(int channel) const {
	return IsSelecting() && channel >= m_selection.GetChanStart() && channel <= m_selection.GetChanEnd();
}

std::unique_ptr<CFrameClipData> CFrameEditorModel::CopySelection(const CFrameSelection &sel, int track) const {
	auto [b, e] = CFrameIterator::FromSelection(sel, const_cast<CFamiTrackerDoc *>(doc_), track); // TODO: remove cast
	const int Frames = e.m_iFrame - b.m_iFrame;

	auto pData = std::make_unique<CFrameClipData>(e.m_iChannel - b.m_iChannel, e.m_iFrame - b.m_iFrame);
	pData->ClipInfo.FirstChannel = b.m_iChannel;		// // //
	pData->ClipInfo.OleInfo.SourceRowStart = b.m_iFrame;
	pData->ClipInfo.OleInfo.SourceRowEnd = e.m_iFrame - 1;

	for (; b != e; ++b)
		for (int c = b.m_iChannel; c < e.m_iChannel; ++c)
			pData->SetFrame(b.m_iFrame, c - b.m_iChannel, b.Get(c));

	return pData;
}
