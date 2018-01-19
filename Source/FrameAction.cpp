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

#include "FrameAction.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "SongData.h"		// // //
#include "MainFrm.h"
#include "FrameEditor.h"
#include "ChannelMap.h"		// // //
#include "SongData.h"		// // //
#include "SongView.h"		// // //
#include "ChannelOrder.h"		// // //
#include <unordered_map>		// // //

// // // all dependencies on CMainFrame
#define GET_VIEW() static_cast<CFamiTrackerView *>(MainFrm.GetActiveView())
#define GET_DOCUMENT() GET_VIEW()->GetDocument()
#define GET_MODULE() GET_VIEW()->GetModuleData()
#define GET_SONG_VIEW() GET_VIEW()->GetSongView()
#define GET_SONG() GET_SONG_VIEW()->GetSong()
#define GET_FRAME_EDITOR() MainFrm.GetFrameEditor()
#define GET_SELECTED_TRACK() MainFrm.GetSelectedTrack()

namespace {

struct pairhash {		// // // from http://stackoverflow.com/a/20602159/5756577
	template <typename T, typename U>
	std::size_t operator()(const std::pair<T, U> &x) const {
		return (std::hash<T>()(x.first) * 3) ^ std::hash<U>()(x.second);
	}
};

void ClonePatterns(const CFrameSelection &Sel, CSongView &view) {		// // //
	std::unordered_map<std::pair<int, int>, int, pairhash> NewPatterns;

	CSongData &song = view.GetSong();

	for (auto [b, e] = CFrameIterator::FromSelection(Sel, view); b != e; ++b) {
		for (int c = b.m_iChannel; c < e.m_iChannel; ++c) {
			int OldPattern = b.Get(c);
			auto Index = std::make_pair(c, OldPattern);
			chan_id_t ch = view.GetChannelOrder().TranslateChannel(c);
			if (auto p = NewPatterns.find(Index); p == NewPatterns.end()) {		// // // share common patterns
				NewPatterns[Index] = song.GetFreePatternIndex(ch);
				song.GetPattern(ch, NewPatterns[Index]) = song.GetPattern(ch, OldPattern);
			}
			b.Set(c, NewPatterns[Index]);
		}
	}
}

} // namespace

// // // Frame editor state class

CFrameEditorState::CFrameEditorState(const CFamiTrackerView *pView, int Track) :		// // //
	Track(Track),
	Cursor {static_cast<CMainFrame*>(pView->GetParentFrame())->GetFrameEditor()->GetEditFrame(), pView->GetSelectedChannel()},
	OriginalSelection(static_cast<CMainFrame*>(pView->GetParentFrame())->GetFrameEditor()->GetSelection()),
	IsSelecting(static_cast<CMainFrame*>(pView->GetParentFrame())->GetFrameEditor()->IsSelecting())
{
	Selection = OriginalSelection.GetNormalized();
}

void CFrameEditorState::ApplyState(CFamiTrackerView *pView) const
{
	auto pEditor = static_cast<CMainFrame*>(pView->GetParentFrame())->GetFrameEditor();
	pEditor->SetEditFrame(Cursor.m_iFrame);
	pView->SelectChannel(Cursor.m_iChannel);
	IsSelecting ? pEditor->SetSelection(OriginalSelection) : pEditor->CancelSelection();
}

int CFrameEditorState::GetFrameStart() const
{
	return IsSelecting ? Selection.GstFirstSelectedFrame() : Cursor.m_iFrame;
}

int CFrameEditorState::GetFrameEnd() const
{
	return IsSelecting ? Selection.GetLastSelectedFrame() : Cursor.m_iFrame;
}

int CFrameEditorState::GetChanStart() const
{
	return IsSelecting ? Selection.GetFirstSelectedChannel() : Cursor.m_iChannel;
}

int CFrameEditorState::GetChanEnd() const
{
	return IsSelecting ? Selection.GetLastSelectedChannel() : Cursor.m_iChannel;
}

#define STATE_EXPAND2(st) (st)->Track, (st)->Cursor.m_iFrame, GET_DOCUMENT()->TranslateChannel((st)->Cursor.m_iChannel)



// CFrameAction ///////////////////////////////////////////////////////////////////
//
// Undo/redo commands for frame editor
//

int CFrameAction::ClipPattern(int Pattern)
{
	return std::clamp(Pattern, 0, MAX_PATTERN - 1);		// // //
}

void CFrameAction::SaveUndoState(const CMainFrame &MainFrm)		// // //
{
	CFamiTrackerView *pView = GET_VIEW();
	m_pUndoState = std::make_unique<CFrameEditorState>(pView, GET_SELECTED_TRACK());		// // //
	m_itFrames = make_int_range(m_pUndoState->GetFrameStart(), m_pUndoState->GetFrameEnd() + 1);
	m_itChannels = make_int_range(m_pUndoState->GetChanStart(), m_pUndoState->GetChanEnd() + 1);
}

void CFrameAction::SaveRedoState(const CMainFrame &MainFrm)		// // //
{
	CFamiTrackerView *pView = GET_VIEW();
	m_pRedoState = std::make_unique<CFrameEditorState>(pView, GET_SELECTED_TRACK());		// // //
}

void CFrameAction::RestoreUndoState(CMainFrame &MainFrm) const		// // //
{
	CFamiTrackerView *pView = GET_VIEW();
	m_pUndoState->ApplyState(pView);
}

void CFrameAction::RestoreRedoState(CMainFrame &MainFrm) const		// // //
{
	CFamiTrackerView *pView = GET_VIEW();
	m_pRedoState->ApplyState(pView);
}

void CFrameAction::UpdateViews(CMainFrame &MainFrm) const {		// // //
	GET_DOCUMENT()->UpdateAllViews(NULL, UPDATE_FRAME);
}



// // // built-in frame action subtypes



bool CFActionAddFrame::SaveState(const CMainFrame &MainFrm)
{
	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	return pDoc->GetFrameCount(m_pUndoState->Track) < MAX_FRAMES;
}

void CFActionAddFrame::Undo(CMainFrame &MainFrm)
{
	GET_SONG().DeleteFrames(m_pUndoState->Cursor.m_iFrame + 1, 1);
}

void CFActionAddFrame::Redo(CMainFrame &MainFrm)
{
	GET_SONG().InsertFrame(m_pUndoState->Cursor.m_iFrame + 1);
}



CFActionRemoveFrame::~CFActionRemoveFrame() {
}

bool CFActionRemoveFrame::SaveState(const CMainFrame &MainFrm)
{
	if (GET_SONG().GetFrameCount() <= 1)
		return false;
	m_pRowClipData = GET_FRAME_EDITOR()->CopyFrame(m_pUndoState->Cursor.m_iFrame);
	return true;
}

void CFActionRemoveFrame::Undo(CMainFrame &MainFrm)
{
	GET_SONG().InsertFrame(m_pUndoState->Cursor.m_iFrame);
	GET_FRAME_EDITOR()->PasteInsert(m_pUndoState->Cursor.m_iFrame, *m_pRowClipData);
	GET_FRAME_EDITOR()->SetSelection({*m_pRowClipData, m_pUndoState->Cursor.m_iFrame});
}

void CFActionRemoveFrame::Redo(CMainFrame &MainFrm)
{
	GET_SONG().DeleteFrames(m_pUndoState->Cursor.m_iFrame, 1);
}



bool CFActionDuplicateFrame::SaveState(const CMainFrame &MainFrm)
{
	return GET_SONG().GetFrameCount() < MAX_FRAMES;
}

void CFActionDuplicateFrame::Undo(CMainFrame &MainFrm)
{
	GET_SONG().DeleteFrames(m_pUndoState->Cursor.m_iFrame, 1);
}

void CFActionDuplicateFrame::Redo(CMainFrame &MainFrm)
{
	GET_SONG().DuplicateFrame(m_pUndoState->Cursor.m_iFrame);
}



bool CFActionCloneFrame::SaveState(const CMainFrame &MainFrm)
{
	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	return pDoc->GetFrameCount(m_pUndoState->Track) < MAX_FRAMES;
}

void CFActionCloneFrame::Undo(CMainFrame &MainFrm)
{
	CSongView *pSongView = GET_SONG_VIEW();
	pSongView->ForeachChannel([&] (std::size_t index) {
		pSongView->GetPatternOnFrame(index, m_pUndoState->Cursor.m_iFrame + 1) = CPatternData { };
	});
	GET_SONG().DeleteFrames(m_pUndoState->Cursor.m_iFrame + 1, 1);
}

void CFActionCloneFrame::Redo(CMainFrame &MainFrm) {
	GET_SONG().CloneFrame(m_pUndoState->Cursor.m_iFrame + 1);
}



CFActionFrameCount::CFActionFrameCount(int Count) :
	m_iNewFrameCount(Count)
{
}

bool CFActionFrameCount::SaveState(const CMainFrame &MainFrm)
{
	m_iOldFrameCount = GET_SONG().GetFrameCount();
	return m_iNewFrameCount != m_iOldFrameCount;
}

void CFActionFrameCount::Undo(CMainFrame &MainFrm)
{
	GET_SONG().SetFrameCount(m_iOldFrameCount);
}

void CFActionFrameCount::Redo(CMainFrame &MainFrm)
{
	GET_SONG().SetFrameCount(m_iNewFrameCount);
}

bool CFActionFrameCount::Merge(const CAction &Other)		// // //
{
	auto pAction = dynamic_cast<const CFActionFrameCount *>(&Other);
	if (!pAction)
		return false;
	if (m_pUndoState->Track != pAction->m_pUndoState->Track)
		return false;

	*m_pRedoState = *pAction->m_pRedoState;
	m_iNewFrameCount = pAction->m_iNewFrameCount;
	return true;
}

void CFActionFrameCount::UpdateViews(CMainFrame &MainFrm) const {
	CFrameAction::UpdateViews(MainFrm);
	MainFrm.UpdateControls();
}



CFActionSetPattern::CFActionSetPattern(int Pattern) :
	m_iNewPattern(Pattern)
{
}

CFActionSetPattern::~CFActionSetPattern() {
}

bool CFActionSetPattern::SaveState(const CMainFrame &MainFrm)
{
	m_pClipData = GET_FRAME_EDITOR()->CopySelection(GET_FRAME_EDITOR()->GetSelection());
	return true;
}

void CFActionSetPattern::Undo(CMainFrame &MainFrm)
{
	GET_FRAME_EDITOR()->PasteAt(*m_pClipData, m_pUndoState->Selection.m_cpStart);
}

void CFActionSetPattern::Redo(CMainFrame &MainFrm)
{
	CSongView *pSongView = GET_SONG_VIEW();
	for (int c : m_itChannels)
		for (int f : m_itFrames)
			pSongView->SetFramePattern(c, f, m_iNewPattern);
}

bool CFActionSetPattern::Merge(const CAction &Other)		// // //
{
	auto pAction = dynamic_cast<const CFActionSetPattern *>(&Other);
	if (!pAction)
		return false;
	if (m_pUndoState->Track != pAction->m_pUndoState->Track ||
		m_itFrames != pAction->m_itFrames || m_itChannels != pAction->m_itChannels)
		return false;

	*m_pRedoState = *pAction->m_pRedoState;
	m_iNewPattern = pAction->m_iNewPattern;
	return true;
}



CFActionSetPatternAll::CFActionSetPatternAll(int Pattern) :
	m_iNewPattern(Pattern)
{
}

CFActionSetPatternAll::~CFActionSetPatternAll() {
}

bool CFActionSetPatternAll::SaveState(const CMainFrame &MainFrm)
{
	m_pRowClipData = GET_FRAME_EDITOR()->CopyFrame(m_pUndoState->Cursor.m_iFrame);
	return true;
}

void CFActionSetPatternAll::Undo(CMainFrame &MainFrm)
{
	GET_FRAME_EDITOR()->PasteAt(*m_pRowClipData, m_pUndoState->Cursor);
}

void CFActionSetPatternAll::Redo(CMainFrame &MainFrm)
{
	CSongView *pSongView = GET_SONG_VIEW();
	GET_SONG_VIEW()->ForeachChannel([&] (std::size_t index) {
		pSongView->SetFramePattern(index, m_pUndoState->Cursor.m_iFrame, m_iNewPattern);
	});
}

bool CFActionSetPatternAll::Merge(const CAction &Other)		// // //
{
	auto pAction = dynamic_cast<const CFActionSetPatternAll *>(&Other);
	if (!pAction)
		return false;
	if (m_pUndoState->Track != pAction->m_pUndoState->Track ||
		m_pUndoState->Cursor.m_iFrame != pAction->m_pUndoState->Cursor.m_iFrame)
		return false;

	*m_pRedoState = *pAction->m_pRedoState;
	m_iNewPattern = pAction->m_iNewPattern;
	return true;
}



CFActionChangePattern::CFActionChangePattern(int Offset) :
	m_iPatternOffset(Offset)
{
}

CFActionChangePattern::~CFActionChangePattern() {
}

bool CFActionChangePattern::SaveState(const CMainFrame &MainFrm)
{
	if (!m_iPatternOffset)
		return false;
	m_pClipData = GET_FRAME_EDITOR()->CopySelection(GET_FRAME_EDITOR()->GetSelection());
	return true;
}

void CFActionChangePattern::Undo(CMainFrame &MainFrm)
{
	GET_FRAME_EDITOR()->PasteAt(*m_pClipData, m_pUndoState->Selection.m_cpStart);
}

void CFActionChangePattern::Redo(CMainFrame &MainFrm)
{
	CSongView *pSongView = GET_SONG_VIEW();
	const int f0 = m_pUndoState->GetFrameStart();
	const int c0 = m_pUndoState->IsSelecting ? m_pUndoState->GetChanStart() : 0; // copies entire row by default
	for (int c : m_itChannels)
		for (int f : m_itFrames) {
			int Frame = m_pClipData->GetFrame(f - f0, c - c0);
			int NewFrame = ClipPattern(Frame + m_iPatternOffset);
			if (NewFrame == Frame)
				m_bOverflow = true;
			pSongView->SetFramePattern(c, f, NewFrame);
		}
}

bool CFActionChangePattern::Merge(const CAction &Other)		// // //
{
	auto pAction = dynamic_cast<const CFActionChangePattern *>(&Other);
	if (!pAction)
		return false;
	if (m_pUndoState->Track != pAction->m_pUndoState->Track ||
		m_itFrames != pAction->m_itFrames || m_itChannels != pAction->m_itChannels)
		return false;
	if (m_bOverflow && m_iPatternOffset * pAction->m_iPatternOffset < 0) // different directions
		return false;

	*m_pRedoState = *pAction->m_pRedoState;
	m_iPatternOffset += pAction->m_iPatternOffset;
	if (pAction->m_bOverflow)
		m_bOverflow = true;
	return true;
}



CFActionChangePatternAll::CFActionChangePatternAll(int Offset) :
	m_iPatternOffset(Offset)
{
}

CFActionChangePatternAll::~CFActionChangePatternAll() {
}

bool CFActionChangePatternAll::SaveState(const CMainFrame &MainFrm)
{
	if (!m_iPatternOffset)
		return false;
	m_pRowClipData = GET_FRAME_EDITOR()->CopyFrame(m_pUndoState->Cursor.m_iFrame);
	return true;
}

void CFActionChangePatternAll::Undo(CMainFrame &MainFrm)
{
	GET_FRAME_EDITOR()->PasteAt(*m_pRowClipData, m_pUndoState->Cursor);
}

void CFActionChangePatternAll::Redo(CMainFrame &MainFrm)
{
	CSongView *pSongView = GET_SONG_VIEW();
	pSongView->ForeachChannel([&] (std::size_t index) {
		int Frame = m_pRowClipData->GetFrame(0, index);
		int NewFrame = ClipPattern(Frame + m_iPatternOffset);
		if (NewFrame == Frame)
			m_bOverflow = true;
		pSongView->SetFramePattern(index, m_pUndoState->Cursor.m_iFrame, NewFrame);
	});
}

bool CFActionChangePatternAll::Merge(const CAction &Other)		// // //
{
	auto pAction = dynamic_cast<const CFActionChangePatternAll *>(&Other);
	if (!pAction)
		return false;
	if (m_pUndoState->Track != pAction->m_pUndoState->Track ||
		m_pUndoState->Cursor.m_iFrame != pAction->m_pUndoState->Cursor.m_iFrame)
		return false;
	if (m_bOverflow && m_iPatternOffset * pAction->m_iPatternOffset < 0) // different directions
		return false;

	*m_pRedoState = *pAction->m_pRedoState;
	m_iPatternOffset += pAction->m_iPatternOffset;
	if (pAction->m_bOverflow)
		m_bOverflow = true;
	return true;
}



bool CFActionMoveDown::SaveState(const CMainFrame &MainFrm)
{
	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	return m_pUndoState->Cursor.m_iFrame < static_cast<int>(pDoc->GetFrameCount(m_pUndoState->Track)) - 1;
}

void CFActionMoveDown::Undo(CMainFrame &MainFrm)
{
	GET_SONG().SwapFrames(m_pUndoState->Cursor.m_iFrame, m_pUndoState->Cursor.m_iFrame + 1);
	GET_VIEW()->SelectFrame(m_pUndoState->Cursor.m_iFrame);
}

void CFActionMoveDown::Redo(CMainFrame &MainFrm)
{
	GET_SONG().SwapFrames(m_pUndoState->Cursor.m_iFrame, m_pUndoState->Cursor.m_iFrame + 1);
	GET_VIEW()->SelectFrame(m_pUndoState->Cursor.m_iFrame + 1);
}



bool CFActionMoveUp::SaveState(const CMainFrame &MainFrm)
{
	return m_pUndoState->Cursor.m_iFrame > 0;
}

void CFActionMoveUp::Undo(CMainFrame &MainFrm)
{
	GET_SONG().SwapFrames(m_pUndoState->Cursor.m_iFrame, m_pUndoState->Cursor.m_iFrame - 1);
	GET_VIEW()->SelectFrame(m_pUndoState->Cursor.m_iFrame);
}

void CFActionMoveUp::Redo(CMainFrame &MainFrm)
{
	GET_SONG().SwapFrames(m_pUndoState->Cursor.m_iFrame, m_pUndoState->Cursor.m_iFrame - 1);
	GET_VIEW()->SelectFrame(m_pUndoState->Cursor.m_iFrame - 1);
}



CFActionPaste::CFActionPaste(std::unique_ptr<CFrameClipData> pData, int Frame, bool Clone) :
	m_pClipData(std::move(pData)), m_iTargetFrame(Frame), m_bClone(Clone)
{
}

CFActionPaste::~CFActionPaste() {
}

bool CFActionPaste::SaveState(const CMainFrame &MainFrm)
{
	if (!m_pClipData)
		return false;

	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	return m_pClipData->ClipInfo.Channels <= pDoc->GetChannelCount() &&
		m_pClipData->ClipInfo.Frames + pDoc->GetFrameCount(m_pUndoState->Track) <= MAX_FRAMES;
}

void CFActionPaste::Undo(CMainFrame &MainFrm)
{
	if (m_bClone)
		GET_FRAME_EDITOR()->ClearPatterns(m_pRedoState->Selection);		// // //
	GET_SONG().DeleteFrames(m_iTargetFrame, m_pClipData->ClipInfo.Frames);
}

void CFActionPaste::Redo(CMainFrame &MainFrm)
{
	CFrameEditor *pFrameEditor = GET_FRAME_EDITOR();
	CFrameSelection sel {*m_pClipData, m_iTargetFrame};
	pFrameEditor->PasteInsert(m_iTargetFrame, *m_pClipData);
	if (m_bClone)
		ClonePatterns(sel, *GET_SONG_VIEW());
	pFrameEditor->SetSelection(sel);
}

void CFActionPaste::UpdateViews(CMainFrame &MainFrm) const {
	CFrameAction::UpdateViews(MainFrm);
	MainFrm.UpdateControls();
}



CFActionPasteOverwrite::CFActionPasteOverwrite(std::unique_ptr<CFrameClipData> pData) :
	m_pClipData(std::move(pData))
{
}

CFActionPasteOverwrite::~CFActionPasteOverwrite() {
}

bool CFActionPasteOverwrite::SaveState(const CMainFrame &MainFrm)		// // //
{
	if (!m_pClipData)
		return false;

	m_TargetSelection = CFrameSelection(*m_pClipData, m_pUndoState->Cursor.m_iFrame);

	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	int Frames = pDoc->GetFrameCount(GET_SELECTED_TRACK());
	if (m_TargetSelection.m_cpEnd.m_iFrame > Frames)
		m_TargetSelection.m_cpEnd.m_iFrame = Frames;
	if (m_TargetSelection.m_cpEnd.m_iFrame <= m_TargetSelection.m_cpStart.m_iFrame)
		return false;

	m_pOldClipData = GET_FRAME_EDITOR()->CopySelection(m_TargetSelection);
	return true;
}

void CFActionPasteOverwrite::Undo(CMainFrame &MainFrm)		// // //
{
	GET_FRAME_EDITOR()->PasteAt(*m_pOldClipData, m_pUndoState->Cursor);
}

void CFActionPasteOverwrite::Redo(CMainFrame &MainFrm)		// // //
{
	auto pEditor = GET_FRAME_EDITOR();
	pEditor->PasteAt(*m_pClipData, m_pUndoState->Cursor);
	pEditor->SetSelection(m_TargetSelection);
}



CFActionDropMove::CFActionDropMove(std::unique_ptr<CFrameClipData> pData, int Frame) :
	m_pClipData(std::move(pData)), m_iTargetFrame(Frame)
{
}

CFActionDropMove::~CFActionDropMove() {
}

bool CFActionDropMove::SaveState(const CMainFrame &MainFrm)
{
	return m_pClipData != nullptr;
}

void CFActionDropMove::Undo(CMainFrame &MainFrm)
{
	GET_FRAME_EDITOR()->MoveSelection(m_pRedoState->Selection,
		m_pUndoState->Selection.m_cpStart);
}

void CFActionDropMove::Redo(CMainFrame &MainFrm)
{
	GET_FRAME_EDITOR()->MoveSelection(m_pUndoState->Selection,
		 {m_iTargetFrame, m_pUndoState->Cursor.m_iChannel});
}



CFActionClonePatterns::~CFActionClonePatterns() {
}

bool CFActionClonePatterns::SaveState(const CMainFrame &MainFrm)		// // //
{
	if (m_pUndoState->IsSelecting) {
		m_pClipData = GET_FRAME_EDITOR()->CopySelection(GET_FRAME_EDITOR()->GetSelection());
		return true; // TODO: check this when all patterns are used up
	}
	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	m_iOldPattern = pDoc->GetPatternAtFrame(STATE_EXPAND2(m_pUndoState));
	if (pDoc->IsPatternEmpty(m_pUndoState->Track, pDoc->TranslateChannel(m_pUndoState->Cursor.m_iChannel), m_iOldPattern))
		return false;
	m_iNewPattern = pDoc->GetFirstFreePattern(m_pUndoState->Track, pDoc->TranslateChannel(m_pUndoState->Cursor.m_iChannel));
	return m_iNewPattern != -1;
}

void CFActionClonePatterns::Undo(CMainFrame &MainFrm)		// // //
{
	if (m_pUndoState->IsSelecting) {
		auto pEditor = GET_FRAME_EDITOR();
		pEditor->ClearPatterns(m_pUndoState->Selection);
		ASSERT(m_pClipData);
		pEditor->PasteAt(*m_pClipData, m_pUndoState->Selection.m_cpStart);
	}
	else {
		CSongView *pSongView = GET_SONG_VIEW();
		pSongView->GetPattern(m_pUndoState->Cursor.m_iChannel, m_pUndoState->Cursor.m_iFrame) = CPatternData { };
		pSongView->SetFramePattern(m_pUndoState->Cursor.m_iChannel, m_pUndoState->Cursor.m_iFrame, m_iOldPattern);
	}
}

void CFActionClonePatterns::Redo(CMainFrame &MainFrm)		// // //
{
	CSongView *pSongView = GET_SONG_VIEW();
	if (m_pUndoState->IsSelecting)
		ClonePatterns(m_pUndoState->Selection, *pSongView);
	else {
		pSongView->SetFramePattern(m_pUndoState->Cursor.m_iChannel, m_pUndoState->Cursor.m_iFrame, m_iNewPattern);
		const auto &old = pSongView->GetPattern(m_pUndoState->Cursor.m_iChannel, m_iOldPattern);
		pSongView->GetPattern(m_pUndoState->Cursor.m_iChannel, m_iNewPattern) = old;
	}
}



CFActionDeleteSel::~CFActionDeleteSel() {
}

bool CFActionDeleteSel::SaveState(const CMainFrame &MainFrm)
{
	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	CFrameSelection Sel(m_pUndoState->Selection);
	Sel.m_cpStart.m_iChannel = 0;
	Sel.m_cpEnd.m_iChannel = pDoc->GetChannelCount();
	if (Sel.GetSelectedFrameCount() == pDoc->GetFrameCount(m_pUndoState->Track))
		if (!--Sel.m_cpEnd.m_iFrame)
			return false;
	m_pClipData = GET_FRAME_EDITOR()->CopySelection(Sel);
	return true;
}

void CFActionDeleteSel::Undo(CMainFrame &MainFrm)
{
	CFrameEditor *pFrameEditor = GET_FRAME_EDITOR();
	pFrameEditor->PasteInsert(m_pUndoState->Selection.m_cpStart.m_iFrame, *m_pClipData);
	pFrameEditor->SetSelection({*m_pClipData, m_pUndoState->Selection.m_cpStart.m_iFrame});
}

void CFActionDeleteSel::Redo(CMainFrame &MainFrm)
{
	GET_SONG().DeleteFrames(
		m_pUndoState->Selection.m_cpStart.m_iFrame,
		m_pUndoState->Selection.GetSelectedFrameCount());		// // //
	GET_VIEW()->SelectFrame(m_pUndoState->Selection.m_cpStart.m_iFrame);
	GET_FRAME_EDITOR()->CancelSelection();
}

void CFActionDeleteSel::UpdateViews(CMainFrame &MainFrm) const {
	CFrameAction::UpdateViews(MainFrm);
	MainFrm.UpdateControls();
}



CFActionMergeDuplicated::~CFActionMergeDuplicated() {
}

bool CFActionMergeDuplicated::SaveState(const CMainFrame &MainFrm)
{
	CFrameEditor *pFrameEditor = GET_FRAME_EDITOR();
	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	m_pOldClipData = pFrameEditor->CopyEntire();

	const int Channels = pDoc->GetChannelCount();
	const int Frames = pDoc->GetFrameCount(m_pUndoState->Track);
	m_pClipData = std::make_unique<CFrameClipData>(Channels, Frames);

	unsigned int uiPatternUsed[MAX_PATTERN];
	for (int c = 0; c < Channels; ++c) {
		// mark all as unused
		for (unsigned int ui = 0; ui < MAX_PATTERN; ++ui)
			uiPatternUsed[ui] = MAX_PATTERN;

		// map used patterns to themselves
		for (int f = 0; f < Frames; ++f) {
			unsigned int uiPattern = pDoc->GetPatternAtFrame(m_pUndoState->Track, f, pDoc->TranslateChannel(c));
			uiPatternUsed[uiPattern] = uiPattern;
		}

		// remap duplicates
		for (unsigned int ui = 0; ui < MAX_PATTERN; ++ui) {
			if (uiPatternUsed[ui] == MAX_PATTERN) continue;
			for (unsigned int uj = 0; uj < ui; ++uj)
				if (pDoc->ArePatternsSame(m_pUndoState->Track, pDoc->TranslateChannel(c), ui, uj)) {		// // //
					uiPatternUsed[ui] = uj;
					TRACE("Duplicate: %d = %d\n", ui, uj);
					break;
				}
		}

		// apply mapping
		for (int f = 0; f < Frames; ++f)
			m_pClipData->SetFrame(f, c, uiPatternUsed[pDoc->GetPatternAtFrame(m_pUndoState->Track, f, pDoc->TranslateChannel(c))]);
	}

	return true;
}

void CFActionMergeDuplicated::Undo(CMainFrame &MainFrm)
{
	CFrameEditor *pFrameEditor = GET_FRAME_EDITOR();
	pFrameEditor->PasteAt(*m_pOldClipData, {0, 0});
}

void CFActionMergeDuplicated::Redo(CMainFrame &MainFrm)
{
	CFrameEditor *pFrameEditor = GET_FRAME_EDITOR();
	pFrameEditor->PasteAt(*m_pClipData, {0, 0});
}
