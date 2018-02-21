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

#include "PatternAction.h"
#include "FamiTrackerEnv.h"		// // //
#include "FamiTrackerView.h"
#include "FamiTrackerViewMessage.h"		// // //
#include "Settings.h"		// // //
#include "MainFrm.h"
#include "PatternEditor.h"
#include "PatternClipData.h"		// // //
#include "FamiTrackerModule.h"		// // //
#include "SongView.h"		// // //

// // // all dependencies on CMainFrame
#define GET_VIEW() static_cast<CFamiTrackerView *>(MainFrm.GetActiveView())
#define GET_MODULE() GET_VIEW()->GetModuleData()
#define GET_SONG_VIEW() GET_VIEW()->GetSongView()
#define GET_PATTERN_EDITOR() GET_VIEW()->GetPatternEditor()
#define UPDATE_CONTROLS() MainFrm.UpdateControls()

// // // Pattern editor state class

CPatternEditorState::CPatternEditorState(const CPatternEditor &Editor) :
	Cursor(Editor.GetCursor()),
	OriginalSelection(Editor.GetSelection()),
	IsSelecting(Editor.IsSelecting())
{
	Selection = OriginalSelection.GetNormalized();
}

void CPatternEditorState::ApplyState(CPatternEditor &Editor) const
{
	Editor.MoveCursor(Cursor);
	IsSelecting ? Editor.SetSelection(OriginalSelection) : Editor.CancelSelection();
	Editor.InvalidateCursor();
}

// CPatternAction /////////////////////////////////////////////////////////////////
//
// Undo/redo commands for pattern editor
//

CPatternAction::~CPatternAction()
{
}

bool CPatternAction::SetTargetSelection(const CMainFrame &MainFrm, CSelection &Sel)		// // //
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	CCursorPos Start;

	if ((m_iPastePos == PASTE_SELECTION || m_iPastePos == PASTE_FILL) && !m_bSelecting)
		m_iPastePos = PASTE_CURSOR;

	switch (m_iPastePos) { // m_iColumn will be written later
	case PASTE_CURSOR:
		Start = m_pUndoState->Cursor;
		break;
	case PASTE_DRAG:
		Start.m_iFrame = m_dragTarget.GetFrameStart();
		Start.m_iRow = m_dragTarget.GetRowStart();
		Start.m_iChannel = m_dragTarget.GetChanStart();
		break;
	case PASTE_SELECTION:
	case PASTE_FILL:
		Start.m_iFrame = m_selection.GetFrameStart();
		Start.m_iRow = m_selection.GetRowStart();
		Start.m_iChannel = m_selection.GetChanStart();
		break;
	}

	auto pSongView = GET_SONG_VIEW();

	CPatternIterator End(*pSongView, Start);

	if (m_iPasteMode == PASTE_INSERT) {
		End.m_iFrame = Start.m_iFrame;
		End.m_iRow = pPatternEditor->GetCurrentPatternLength(End.m_iFrame) - 1;
	}
	else
		End += m_pClipData->ClipInfo.Rows - 1;

	switch (m_iPastePos) {
	case PASTE_FILL:
		End.m_iFrame = m_selection.GetFrameEnd();
		End.m_iRow = m_selection.GetRowEnd();
		End.m_iChannel = m_selection.GetChanEnd();
		Start.m_iColumn = GetCursorStartColumn(m_pClipData->ClipInfo.StartColumn);
		End.m_iColumn = GetCursorEndColumn(
			!((End.m_iChannel - Start.m_iChannel + 1) % m_pClipData->ClipInfo.Channels) ?
			m_pClipData->ClipInfo.EndColumn :
			static_cast<column_t>(COLUMN_EFF1 + pSongView->GetEffectColumnCount(End.m_iChannel)));
		break;
	case PASTE_DRAG:
		End.m_iChannel += m_pClipData->ClipInfo.Channels - 1;
		Start.m_iColumn = m_dragTarget.GetColStart();
		End.m_iColumn = m_dragTarget.GetColEnd();
		break;
	default:
		End.m_iChannel += m_pClipData->ClipInfo.Channels - 1;
		Start.m_iColumn = GetCursorStartColumn(m_pClipData->ClipInfo.StartColumn);
		End.m_iColumn = GetCursorEndColumn(m_pClipData->ClipInfo.EndColumn);
	}

	const bool bOverflow = Env.GetSettings()->General.bOverflowPaste;
	if (!bOverflow && End.m_iFrame > Start.m_iFrame) {
		End.m_iFrame = Start.m_iFrame;
		End.m_iRow = pPatternEditor->GetCurrentPatternLength(End.m_iFrame) - 1;
	}

	const unsigned EFBEGIN = GetCursorStartColumn(COLUMN_EFF1);
	int OFFS = 3 * (GetSelectColumn(m_pUndoState->Cursor.m_iColumn) - m_pClipData->ClipInfo.StartColumn);
	if (static_cast<int>(EFBEGIN - Start.m_iColumn) > OFFS)
		OFFS = EFBEGIN - Start.m_iColumn;
	if (Start.m_iChannel == End.m_iChannel && Start.m_iColumn >= EFBEGIN && End.m_iColumn >= EFBEGIN) {
		if (m_iPastePos != PASTE_DRAG) {
			End.m_iColumn = static_cast<cursor_column_t>(End.m_iColumn + OFFS);
			Start.m_iColumn = static_cast<cursor_column_t>(Start.m_iColumn + OFFS);
			if (End.m_iColumn > C_EFF4_PARAM2)
				End.m_iColumn = C_EFF4_PARAM2;
		}
	}

	CSelection New;
	New.m_cpStart = Start;
	New.m_cpEnd = End;
	pPatternEditor->SetSelection(New);

	sel_condition_t Cond = pPatternEditor->GetSelectionCondition();
	if (Cond == SEL_CLEAN) {
		Sel = New;
		return true;
	}
	else {
		pPatternEditor->SetSelection(m_selection);
		if (!m_bSelecting) pPatternEditor->CancelSelection();
		int Confirm = IDYES;
		switch (Cond) {
		case SEL_REPEATED_ROW:
			Confirm = AfxMessageBox(IDS_PASTE_REPEATED_ROW, MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2);
			break;
		case SEL_NONTERMINAL_SKIP: case SEL_TERMINAL_SKIP:
			if (!bOverflow) break;
			Confirm = AfxMessageBox(IDS_PASTE_NONTERMINAL, MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2);
			break;
		}
		if (Confirm == IDYES) {
			pPatternEditor->SetSelection(Sel = New);
			return true;
		}
		else {
			return false;
		}
	}
}

void CPatternAction::DeleteSelection(CSongView &view, const CSelection &Sel) const		// // //
{
	auto it = CPatternIterator::FromSelection(Sel, view);
	const column_t ColStart = GetSelectColumn(it.first.m_iColumn);
	const column_t ColEnd = GetSelectColumn(it.second.m_iColumn);

	stChanNote BLANK;

	do for (int i = it.first.m_iChannel; i <= it.second.m_iChannel; ++i) {
		auto NoteData = it.first.Get(i);
		CopyNoteSection(&NoteData, &BLANK, PASTE_DEFAULT,
						i == it.first.m_iChannel ? ColStart : COLUMN_NOTE,
						i == it.second.m_iChannel ? ColEnd : COLUMN_EFF4);
		it.first.Set(i, NoteData);
	} while (++it.first <= it.second);
}

bool CPatternAction::ValidateSelection(const CPatternEditor &Editor) const		// // //
{
	if (!m_pUndoState->IsSelecting)
		return false;
	switch (Editor.GetSelectionCondition(m_pUndoState->Selection)) {
	case SEL_CLEAN:
		return true;
	case SEL_REPEATED_ROW:
		static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(IDS_SEL_REPEATED_ROW); break;
	case SEL_NONTERMINAL_SKIP:
		static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(IDS_SEL_NONTERMINAL_SKIP); break;
	case SEL_TERMINAL_SKIP:
		static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(IDS_SEL_TERMINAL_SKIP); break;
	}
	MessageBeep(MB_ICONWARNING);
	return false;
}

void CPatternAction::UpdateViews(CMainFrame &MainFrm) const		// // //
{
//	MainFrm.GetActiveDocument()->UpdateAllViews(NULL, UPDATE_PATTERN);
	MainFrm.GetActiveDocument()->UpdateAllViews(NULL, UPDATE_FRAME); // cursor might have moved to different channel
}

std::pair<CPatternIterator, CPatternIterator> CPatternAction::GetIterators(CSongView &view) const
{
	return m_pUndoState->IsSelecting ?
		CPatternIterator::FromSelection(m_pUndoState->Selection, view) :
		CPatternIterator::FromCursor(m_pUndoState->Cursor, view);
}

// Undo / Redo base methods

void CPatternAction::SaveUndoState(const CMainFrame &MainFrm)		// // //
{
	// Save undo cursor position
	CFamiTrackerView *pView = GET_VIEW();
	const CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();

	m_pUndoState = std::make_unique<CPatternEditorState>(*pPatternEditor);
	m_bSelecting = pPatternEditor->IsSelecting();
	m_selection = pPatternEditor->GetSelection();
}

void CPatternAction::SaveRedoState(const CMainFrame &MainFrm)		// // //
{
	m_pRedoState = std::make_unique<CPatternEditorState>(*GET_PATTERN_EDITOR());
}

void CPatternAction::RestoreUndoState(CMainFrame &MainFrm) const		// // //
{
	if (m_pUndoState)
		m_pUndoState->ApplyState(*GET_PATTERN_EDITOR());
}

void CPatternAction::RestoreRedoState(CMainFrame &MainFrm) const		// // //
{
	if (m_pRedoState)
		m_pRedoState->ApplyState(*GET_PATTERN_EDITOR());
}



CPSelectionAction::~CPSelectionAction() {
}

bool CPSelectionAction::SaveState(const CMainFrame &MainFrm)
{
	const CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	m_pUndoClipData = pPatternEditor->CopyRaw(m_pUndoState->Selection);
	return true;
}

void CPSelectionAction::Undo(CMainFrame &MainFrm)
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	pPatternEditor->PasteRaw(*m_pUndoClipData, m_pUndoState->Selection.m_cpStart);
}



// // // built-in pattern action subtypes



CPActionEditNote::CPActionEditNote(const stChanNote &Note) :
	m_NewNote(Note)
{
}

bool CPActionEditNote::SaveState(const CMainFrame &MainFrm)
{
	m_OldNote = GET_SONG_VIEW()->GetPatternOnFrame(m_pUndoState->Cursor.m_iChannel, m_pUndoState->Cursor.m_iFrame)
		.GetNoteOn(m_pUndoState->Cursor.m_iRow);		// // //
	return true;
}

void CPActionEditNote::Undo(CMainFrame &MainFrm)
{
	GET_SONG_VIEW()->GetPatternOnFrame(m_pUndoState->Cursor.m_iChannel, m_pUndoState->Cursor.m_iFrame)
		.SetNoteOn(m_pUndoState->Cursor.m_iRow, m_OldNote);		// // //
}

void CPActionEditNote::Redo(CMainFrame &MainFrm)
{
	GET_SONG_VIEW()->GetPatternOnFrame(m_pUndoState->Cursor.m_iChannel, m_pUndoState->Cursor.m_iFrame)
		.SetNoteOn(m_pUndoState->Cursor.m_iRow, m_NewNote);		// // //
}



CPActionReplaceNote::CPActionReplaceNote(const stChanNote &Note, int Frame, int Row, int Channel) :
	m_NewNote(Note), m_iFrame(Frame), m_iRow(Row), m_iChannel(Channel)
{
}

bool CPActionReplaceNote::SaveState(const CMainFrame &MainFrm)
{
	m_OldNote = GET_SONG_VIEW()->GetPatternOnFrame(m_iChannel, m_iFrame).GetNoteOn(m_iRow);		// // //
	return true;
}

void CPActionReplaceNote::Undo(CMainFrame &MainFrm)
{
	GET_SONG_VIEW()->GetPatternOnFrame(m_iChannel, m_iFrame).SetNoteOn(m_iRow, m_OldNote);		// // //
}

void CPActionReplaceNote::Redo(CMainFrame &MainFrm)
{
	GET_SONG_VIEW()->GetPatternOnFrame(m_iChannel, m_iFrame).SetNoteOn(m_iRow, m_NewNote);		// // //
}



bool CPActionInsertRow::SaveState(const CMainFrame &MainFrm)
{
	CSongView *pSongView = GET_SONG_VIEW();
	m_OldNote = GET_SONG_VIEW()->GetPatternOnFrame(m_pUndoState->Cursor.m_iChannel, m_pUndoState->Cursor.m_iFrame)
		.GetNoteOn(pSongView->GetSong().GetPatternLength() - 1);		// // //
	return true;
}

void CPActionInsertRow::Undo(CMainFrame &MainFrm)
{
	CSongView *pSongView = GET_SONG_VIEW();

	pSongView->PullUp(m_pUndoState->Cursor.m_iChannel, m_pUndoState->Cursor.m_iFrame, m_pUndoState->Cursor.m_iRow);
	pSongView->GetPatternOnFrame(m_pUndoState->Cursor.m_iChannel, m_pUndoState->Cursor.m_iFrame)
		.SetNoteOn(pSongView->GetSong().GetPatternLength() - 1, m_OldNote);		// // //
}

void CPActionInsertRow::Redo(CMainFrame &MainFrm)
{
	GET_SONG_VIEW()->InsertRow(m_pUndoState->Cursor.m_iChannel, m_pUndoState->Cursor.m_iFrame, m_pUndoState->Cursor.m_iRow);
}



CPActionDeleteRow::CPActionDeleteRow(bool PullUp, bool Backspace) :
	m_bPullUp(PullUp), m_bBack(Backspace)
{
}

bool CPActionDeleteRow::SaveState(const CMainFrame &MainFrm)
{
	if (m_bBack && !m_pUndoState->Cursor.m_iRow)
		return false;
	m_iRow = m_pUndoState->Cursor.m_iRow - (m_bBack ? 1 : 0);
	m_OldNote = GET_SONG_VIEW()->GetPatternOnFrame(m_pUndoState->Cursor.m_iChannel, m_pUndoState->Cursor.m_iFrame)
		.GetNoteOn(m_iRow);		// // //

	m_NewNote = m_OldNote;
	switch (m_pUndoState->Cursor.m_iColumn) {
	case C_NOTE:			// Note
		m_NewNote.Note = note_t::NONE;
		m_NewNote.Octave = 0;
		m_NewNote.Instrument = MAX_INSTRUMENTS;	// Fix the old behaviour
		m_NewNote.Vol = MAX_VOLUME;
		break;
	case C_INSTRUMENT1:		// Instrument
	case C_INSTRUMENT2:
		m_NewNote.Instrument = MAX_INSTRUMENTS;
		break;
	case C_VOLUME:			// Volume
		m_NewNote.Vol = MAX_VOLUME;
		break;
	case C_EFF1_NUM:			// Effect 1
	case C_EFF1_PARAM1:
	case C_EFF1_PARAM2:
		m_NewNote.EffNumber[0] = EF_NONE;
		m_NewNote.EffParam[0] = 0;
		break;
	case C_EFF2_NUM:		// Effect 2
	case C_EFF2_PARAM1:
	case C_EFF2_PARAM2:
		m_NewNote.EffNumber[1] = EF_NONE;
		m_NewNote.EffParam[1] = 0;
		break;
	case C_EFF3_NUM:		// Effect 3
	case C_EFF3_PARAM1:
	case C_EFF3_PARAM2:
		m_NewNote.EffNumber[2] = EF_NONE;
		m_NewNote.EffParam[2] = 0;
		break;
	case C_EFF4_NUM:		// Effect 4
	case C_EFF4_PARAM1:
	case C_EFF4_PARAM2:
		m_NewNote.EffNumber[3] = EF_NONE;
		m_NewNote.EffParam[3] = 0;
		break;
	}

	return true;
}

void CPActionDeleteRow::Undo(CMainFrame &MainFrm)
{
	CSongView *pSongView = GET_SONG_VIEW();
	if (m_bPullUp)
		pSongView->InsertRow(m_pUndoState->Cursor.m_iChannel, m_pUndoState->Cursor.m_iFrame, m_iRow);
	pSongView->GetPatternOnFrame(m_pUndoState->Cursor.m_iChannel, m_pUndoState->Cursor.m_iFrame)
		.SetNoteOn(m_iRow, m_OldNote);		// // //
}

void CPActionDeleteRow::Redo(CMainFrame &MainFrm)
{
	CSongView *pSongView = GET_SONG_VIEW();
	pSongView->GetPatternOnFrame(m_pUndoState->Cursor.m_iChannel, m_pUndoState->Cursor.m_iFrame)
		.SetNoteOn(m_iRow, m_NewNote);		// // //
	if (m_bPullUp)
		pSongView->PullUp(m_pUndoState->Cursor.m_iChannel, m_pUndoState->Cursor.m_iFrame, m_iRow);
}



CPActionScrollField::CPActionScrollField(int Amount) :		// // //
	m_iAmount(Amount)
{
}

bool CPActionScrollField::SaveState(const CMainFrame &MainFrm)
{
	const auto ScrollFunc = [&] (unsigned char &Old, int Limit) {
		int New = static_cast<int>(Old) + m_iAmount;
		if (Env.GetSettings()->General.bWrapPatternValue) {
			New %= Limit;
			if (New < 0)
				New += Limit;
		}
		else {
			New = std::clamp(New, 0, Limit - 1);
		}
		Old = static_cast<unsigned char>(New);
	};

	m_OldNote = GET_SONG_VIEW()->GetPatternOnFrame(m_pUndoState->Cursor.m_iChannel, m_pUndoState->Cursor.m_iFrame)
		.GetNoteOn(m_pUndoState->Cursor.m_iRow);		// // //
	m_NewNote = m_OldNote;

	switch (m_pUndoState->Cursor.m_iColumn) {
	case C_INSTRUMENT1: case C_INSTRUMENT2:
		ScrollFunc(m_NewNote.Instrument, MAX_INSTRUMENTS);
		return m_OldNote.Instrument < MAX_INSTRUMENTS && m_OldNote.Instrument != HOLD_INSTRUMENT;		// // // 050B
	case C_VOLUME:
		ScrollFunc(m_NewNote.Vol, MAX_VOLUME);
		return m_OldNote.Vol < MAX_VOLUME;
	case C_EFF1_NUM: case C_EFF1_PARAM1: case C_EFF1_PARAM2:
		ScrollFunc(m_NewNote.EffParam[0], 0x100);
		return m_OldNote.EffNumber[0] != EF_NONE;
	case C_EFF2_NUM: case C_EFF2_PARAM1: case C_EFF2_PARAM2:
		ScrollFunc(m_NewNote.EffParam[1], 0x100);
		return m_OldNote.EffNumber[1] != EF_NONE;
	case C_EFF3_NUM: case C_EFF3_PARAM1: case C_EFF3_PARAM2:
		ScrollFunc(m_NewNote.EffParam[2], 0x100);
		return m_OldNote.EffNumber[2] != EF_NONE;
	case C_EFF4_NUM: case C_EFF4_PARAM1: case C_EFF4_PARAM2:
		ScrollFunc(m_NewNote.EffParam[3], 0x100);
		return m_OldNote.EffNumber[3] != EF_NONE;
	}

	return false;
}

void CPActionScrollField::Undo(CMainFrame &MainFrm)
{
	GET_SONG_VIEW()->GetPatternOnFrame(m_pUndoState->Cursor.m_iChannel, m_pUndoState->Cursor.m_iFrame)
		.SetNoteOn(m_pUndoState->Cursor.m_iRow, m_OldNote);		// // //
}

void CPActionScrollField::Redo(CMainFrame &MainFrm)
{
	GET_SONG_VIEW()->GetPatternOnFrame(m_pUndoState->Cursor.m_iChannel, m_pUndoState->Cursor.m_iFrame)
		.SetNoteOn(m_pUndoState->Cursor.m_iRow, m_NewNote);		// // //
}



// // // TODO: move stuff in CPatternAction::SetTargetSelection to the redo state
/*
CPSelectionAction::~CPSelectionAction()
{
}

bool CPSelectionAction::SaveState(const CMainFrame &MainFrm)
{
	const CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	m_pUndoClipData = pPatternEditor->CopyRaw(m_pUndoState->Selection);
	return true;
}

void CPSelectionAction::Undo(CMainFrame &MainFrm)
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	pPatternEditor->PasteRaw(m_pUndoClipData, m_pUndoState->Selection.m_cpStart);
}
*/

CPActionPaste::CPActionPaste(std::unique_ptr<CPatternClipData> pClipData, paste_mode_t Mode, paste_pos_t Pos)
{
	m_pClipData = std::move(pClipData);
	m_iPasteMode = Mode;
	m_iPastePos = Pos;
}

bool CPActionPaste::SaveState(const CMainFrame &MainFrm) {
	if (!SetTargetSelection(MainFrm, m_newSelection))		// // //
		return false;
	m_pUndoClipData = GET_PATTERN_EDITOR()->CopyRaw();
	return true;
}

void CPActionPaste::Undo(CMainFrame &MainFrm) {
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	pPatternEditor->SetSelection(m_newSelection);		// // //
	pPatternEditor->PasteRaw(*m_pUndoClipData);
}

void CPActionPaste::Redo(CMainFrame &MainFrm) {
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	pPatternEditor->Paste(*m_pClipData, m_iPasteMode, m_iPastePos);		// // //
}



void CPActionClearSel::Redo(CMainFrame &MainFrm)
{
	DeleteSelection(*GET_SONG_VIEW(), m_pUndoState->Selection);
}



CPActionDeleteAtSel::~CPActionDeleteAtSel()
{
}

bool CPActionDeleteAtSel::SaveState(const CMainFrame &MainFrm)
{
	if (!m_pUndoState->IsSelecting) return false;

	const CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	m_cpTailPos = CCursorPos {
		m_pUndoState->Selection.m_cpEnd.m_iRow + 1,
		m_pUndoState->Selection.m_cpStart.m_iChannel,
		m_pUndoState->Selection.m_cpStart.m_iColumn,
		m_pUndoState->Selection.m_cpEnd.m_iFrame
	};
	m_pUndoHead = pPatternEditor->CopyRaw(m_pUndoState->Selection);
	int Length = pPatternEditor->GetCurrentPatternLength(m_cpTailPos.m_iFrame) - 1;
	if (m_cpTailPos.m_iRow <= Length)
		m_pUndoTail = pPatternEditor->CopyRaw(CSelection {m_cpTailPos, CCursorPos {
			Length,
			m_pUndoState->Selection.m_cpEnd.m_iChannel,
			m_pUndoState->Selection.m_cpEnd.m_iColumn,
			m_cpTailPos.m_iFrame
		}});
	return true;
}

void CPActionDeleteAtSel::Undo(CMainFrame &MainFrm)
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	pPatternEditor->PasteRaw(*m_pUndoHead, m_pUndoState->Selection.m_cpStart);
	if (m_pUndoTail)
		pPatternEditor->PasteRaw(*m_pUndoTail, m_cpTailPos);
}

void CPActionDeleteAtSel::Redo(CMainFrame &MainFrm)
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();

	CSelection Sel(m_pUndoState->Selection);
	Sel.m_cpEnd.m_iRow = pPatternEditor->GetCurrentPatternLength(Sel.m_cpEnd.m_iFrame) - 1;
	DeleteSelection(*GET_SONG_VIEW(), Sel);
	if (m_pUndoTail)
		pPatternEditor->PasteRaw(*m_pUndoTail, m_pUndoState->Selection.m_cpStart);
	pPatternEditor->CancelSelection();
}



CPActionInsertAtSel::~CPActionInsertAtSel()
{
}

bool CPActionInsertAtSel::SaveState(const CMainFrame &MainFrm)
{
	if (!m_pUndoState->IsSelecting) return false;

	const CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	m_cpTailPos = CCursorPos {
		pPatternEditor->GetCurrentPatternLength(m_pUndoState->Selection.m_cpEnd.m_iFrame) - 1,
		m_pUndoState->Selection.m_cpStart.m_iChannel,
		m_pUndoState->Selection.m_cpStart.m_iColumn,
		m_pUndoState->Selection.m_cpEnd.m_iFrame
	};
	CCursorPos HeadEnd {
		m_cpTailPos.m_iRow,
		m_pUndoState->Selection.m_cpEnd.m_iChannel,
		m_pUndoState->Selection.m_cpEnd.m_iColumn,
		m_cpTailPos.m_iFrame
	};

	m_pUndoTail = pPatternEditor->CopyRaw(CSelection {m_cpTailPos, CCursorPos {HeadEnd}});
	if (--HeadEnd.m_iRow < 0) {
		--HeadEnd.m_iFrame;
		HeadEnd.m_iRow += pPatternEditor->GetCurrentPatternLength(HeadEnd.m_iFrame);
	}
	if (m_pUndoState->Selection.m_cpStart <= HeadEnd) {
		m_pUndoHead = pPatternEditor->CopyRaw(CSelection {m_pUndoState->Selection.m_cpStart, HeadEnd});
		m_cpHeadPos = m_pUndoState->Selection.m_cpStart;
		if (++m_cpHeadPos.m_iRow >= pPatternEditor->GetCurrentPatternLength(m_cpHeadPos.m_iFrame)) {
			++m_cpHeadPos.m_iFrame;
			m_cpHeadPos.m_iRow = 0;
		}
	}

	return true;
}

void CPActionInsertAtSel::Undo(CMainFrame &MainFrm)
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	pPatternEditor->PasteRaw(*m_pUndoTail, m_cpTailPos);
	if (m_pUndoHead)
		pPatternEditor->PasteRaw(*m_pUndoHead, m_pUndoState->Selection.m_cpStart);
}

void CPActionInsertAtSel::Redo(CMainFrame &MainFrm)
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();

	CSelection Sel(m_pUndoState->Selection);
	Sel.m_cpEnd.m_iRow = pPatternEditor->GetCurrentPatternLength(Sel.m_cpEnd.m_iFrame) - 1;
	DeleteSelection(*GET_SONG_VIEW(), Sel);
	if (m_pUndoHead)
		pPatternEditor->PasteRaw(*m_pUndoHead, m_cpHeadPos);
}



CPActionTranspose::CPActionTranspose(transpose_t Type) : m_iTransposeMode(Type)
{
}

void CPActionTranspose::Redo(CMainFrame &MainFrm)
{
	CSongView *pSongView = GET_SONG_VIEW();
	auto it = GetIterators(*pSongView);

	int ChanStart     = (m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpStart : m_pUndoState->Cursor).m_iChannel;
	int ChanEnd       = (m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpEnd : m_pUndoState->Cursor).m_iChannel;
	column_t ColStart = GetSelectColumn(
		(m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpStart : m_pUndoState->Cursor).m_iColumn);
	column_t ColEnd   = GetSelectColumn(
		(m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpEnd : m_pUndoState->Cursor).m_iColumn);

	const bool bSingular = it.first == it.second && !m_pUndoState->IsSelecting;
	const unsigned Length = pSongView->GetSong().GetPatternLength();

	int Row = 0;		// // //
	int oldRow = -1;
	do {
		if (it.first.m_iRow <= oldRow)
			Row += Length + it.first.m_iRow - oldRow - 1;
		for (int i = ChanStart; i <= ChanEnd; ++i) {
			if (!m_pUndoState->Selection.IsColumnSelected(COLUMN_NOTE, i))
				continue;
			stChanNote Note = *(m_pUndoClipData->GetPattern(i - ChanStart, Row));		// // //
			if (Note.Note == note_t::ECHO) {
				if (!bSingular)
					continue;
				switch (m_iTransposeMode) {		// // //
				case TRANSPOSE_DEC_NOTES: case TRANSPOSE_DEC_OCTAVES:
					if (Note.Octave > 0)
						--Note.Octave;
					break;
				case TRANSPOSE_INC_NOTES: case TRANSPOSE_INC_OCTAVES:
					if (Note.Octave < ECHO_BUFFER_LENGTH)
						++Note.Octave;
					break;
				}
			}
			else if (IsNote(Note.Note)) {		// // //
				static const int AMOUNT[] = {-1, 1, -NOTE_RANGE, NOTE_RANGE};
				int NewNote = MIDI_NOTE(Note.Octave, Note.Note) + AMOUNT[m_iTransposeMode];
				if (NewNote < 0) NewNote = 0;
				if (NewNote >= NOTE_COUNT) NewNote = NOTE_COUNT - 1;
				Note.Note = GET_NOTE(NewNote);
				Note.Octave = GET_OCTAVE(NewNote);
			}
			else
				continue;
			it.first.Set(i, Note);
		}
		++Row;
		oldRow = it.first.m_iRow;
	} while (++it.first <= it.second);
}



CPActionScrollValues::CPActionScrollValues(int Amount) : m_iAmount(Amount)
{
}

void CPActionScrollValues::Redo(CMainFrame &MainFrm)
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	CSongView *pSongView = GET_SONG_VIEW();
	auto it = GetIterators(*pSongView);
	int ChanStart     = (m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpStart : m_pUndoState->Cursor).m_iChannel;
	int ChanEnd       = (m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpEnd : m_pUndoState->Cursor).m_iChannel;
	column_t ColStart = GetSelectColumn(
		(m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpStart : m_pUndoState->Cursor).m_iColumn);
	column_t ColEnd   = GetSelectColumn(
		(m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpEnd : m_pUndoState->Cursor).m_iColumn);

	const bool bSingular = it.first == it.second && !m_pUndoState->IsSelecting;
	const unsigned Length = pSongView->GetSong().GetPatternLength();

	const auto WarpFunc = [this] (unsigned char &x, int Lim) {
		int Val = x + m_iAmount;
		if (Env.GetSettings()->General.bWrapPatternValue) {
			Val %= Lim;
			if (Val < 0) Val += Lim;
		}
		else {
			if (Val >= Lim) Val = Lim - 1;
			if (Val < 0) Val = 0;
		}
		x = static_cast<unsigned char>(Val);
	};

	int Row = 0;
	int oldRow = -1;
	do {
		if (it.first.m_iRow <= oldRow)
			Row += Length + it.first.m_iRow - oldRow - 1;
		for (int i = ChanStart; i <= ChanEnd; ++i) {
			auto Note = *(m_pUndoClipData->GetPattern(i - ChanStart, Row));		// // //
			for (unsigned k = COLUMN_INSTRUMENT; k < COLUMNS; ++k) {
				if (i == ChanStart && k < ColStart)
					continue;
				if (i == ChanEnd && k > ColEnd)
					continue;
				switch (k) {
				case COLUMN_INSTRUMENT:
					if (Note.Instrument == MAX_INSTRUMENTS || Note.Instrument == HOLD_INSTRUMENT) break;		// // // 050B
					WarpFunc(Note.Instrument, MAX_INSTRUMENTS);
					break;
				case COLUMN_VOLUME:
					if (Note.Vol == MAX_VOLUME) break;
					WarpFunc(Note.Vol, MAX_VOLUME);
					break;
				case COLUMN_EFF1: case COLUMN_EFF2: case COLUMN_EFF3: case COLUMN_EFF4:
					if (Note.EffNumber[k - COLUMN_EFF1] == EF_NONE) break;
					if (bSingular) switch (Note.EffNumber[k - COLUMN_EFF1]) {
					case EF_SWEEPUP: case EF_SWEEPDOWN: case EF_ARPEGGIO: case EF_VIBRATO: case EF_TREMOLO:
					case EF_SLIDE_UP: case EF_SLIDE_DOWN: case EF_VOLUME_SLIDE: case EF_DELAYED_VOLUME: case EF_TRANSPOSE:
						unsigned char Hi = Note.EffParam[k - COLUMN_EFF1] >> 4;
						unsigned char Lo = Note.EffParam[k - COLUMN_EFF1] & 0x0F;
						WarpFunc(pPatternEditor->GetColumn() % 3 == 2 ? Hi : Lo, 0x10);
						Note.EffParam[k - COLUMN_EFF1] = (Hi << 4) | Lo;
						continue;
					}
					WarpFunc(Note.EffParam[k - COLUMN_EFF1], 0x100);
					break;
				}
			}
			it.first.Set(i, Note);
		}
		++Row;
		oldRow = it.first.m_iRow;
	} while (++it.first <= it.second);
}



bool CPActionInterpolate::SaveState(const CMainFrame &MainFrm)
{
	const CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	if (!ValidateSelection(*pPatternEditor))
		return false;
	m_iSelectionSize = pPatternEditor->GetSelectionSize();
	return CPSelectionAction::SaveState(MainFrm);
}

void CPActionInterpolate::Redo(CMainFrame &MainFrm)
{
	CSongView *pSongView = GET_SONG_VIEW();
	auto it = GetIterators(*pSongView);
	const CSelection &Sel = m_pUndoState->Selection;

	for (int i = Sel.m_cpStart.m_iChannel; i <= Sel.m_cpEnd.m_iChannel; ++i) {
		const int Columns = pSongView->GetEffectColumnCount(i) + 4;		// // //
		for (int j = 0; j < Columns; ++j) {
			if (!Sel.IsColumnSelected(static_cast<column_t>(j), i)) continue;
			CPatternIterator r {it.first};		// // //
			const auto &StartData = r.Get(i);
			const auto &EndData = it.second.Get(i);
			double StartValHi, StartValLo;		// // //
			double EndValHi, EndValLo;
			double DeltaHi, DeltaLo;
			bool TwoParam = false;
			effect_t Effect;
			switch (j) {
			case COLUMN_NOTE:
				if (!IsNote(StartData.Note) || !IsNote(EndData.Note))
					continue;
				StartValLo = (float)MIDI_NOTE(StartData.Octave, StartData.Note);
				EndValLo = (float)MIDI_NOTE(EndData.Octave, EndData.Note);
				break;
			case COLUMN_INSTRUMENT:
				if (StartData.Instrument == MAX_INSTRUMENTS || EndData.Instrument == MAX_INSTRUMENTS)
					continue;
				if (StartData.Instrument == HOLD_INSTRUMENT || EndData.Instrument == HOLD_INSTRUMENT)		// // // 050B
					continue;
				StartValLo = (float)StartData.Instrument;
				EndValLo = (float)EndData.Instrument;
				break;
			case COLUMN_VOLUME:
				if (StartData.Vol == MAX_VOLUME || EndData.Vol == MAX_VOLUME)
					continue;
				StartValLo = (float)StartData.Vol;
				EndValLo = (float)EndData.Vol;
				break;
			case COLUMN_EFF1: case COLUMN_EFF2: case COLUMN_EFF3: case COLUMN_EFF4:
				if (StartData.EffNumber[j - 3] == EF_NONE || EndData.EffNumber[j - 3] == EF_NONE
					|| StartData.EffNumber[j - 3] != EndData.EffNumber[j - 3])
					continue;
				StartValLo = (float)StartData.EffParam[j - 3];
				EndValLo = (float)EndData.EffParam[j - 3];
				Effect = StartData.EffNumber[j - 3];
				switch (Effect) {
				case EF_SWEEPUP: case EF_SWEEPDOWN: case EF_SLIDE_UP: case EF_SLIDE_DOWN:
				case EF_ARPEGGIO: case EF_VIBRATO: case EF_TREMOLO:
				case EF_VOLUME_SLIDE: case EF_DELAYED_VOLUME: case EF_TRANSPOSE:
					TwoParam = true;
				}
				break;
			}

			if (TwoParam) {
				StartValHi = std::floor(StartValLo / 16.0);
				StartValLo = std::fmod(StartValLo, 16.0);
				EndValHi = std::floor(EndValLo / 16.0);
				EndValLo = std::fmod(EndValLo, 16.0);
			}
			else
				StartValHi = EndValHi = 0.0;
			DeltaHi = (EndValHi - StartValHi) / float(m_iSelectionSize - 1);
			DeltaLo = (EndValLo - StartValLo) / float(m_iSelectionSize - 1);

			while (++r < it.second) {
				StartValLo += DeltaLo;
				StartValHi += DeltaHi;
				auto Note = r.Get(i);
				switch (j) {
				case COLUMN_NOTE:
					Note.Note = GET_NOTE((int)StartValLo);
					Note.Octave = GET_OCTAVE((int)StartValLo);
					break;
				case COLUMN_INSTRUMENT:
					Note.Instrument = (int)StartValLo;
					break;
				case COLUMN_VOLUME:
					Note.Vol = (int)StartValLo;
					break;
				case COLUMN_EFF1: case COLUMN_EFF2: case COLUMN_EFF3: case COLUMN_EFF4:
					Note.EffNumber[j - 3] = Effect;
					Note.EffParam[j - 3] = (int)StartValLo + ((int)StartValHi << 4);
					break;
				}
				r.Set(i, Note);
			}
		}
	}
}



bool CPActionReverse::SaveState(const CMainFrame &MainFrm)
{
	if (!ValidateSelection(*GET_PATTERN_EDITOR()))
		return false;
	return CPSelectionAction::SaveState(MainFrm);
}

void CPActionReverse::Redo(CMainFrame &MainFrm)
{
	CSongView *pSongView = GET_SONG_VIEW();
	auto it = GetIterators(*pSongView);
	const CSelection &Sel = m_pUndoState->Selection;

	const column_t ColStart = GetSelectColumn(Sel.m_cpStart.m_iColumn);
	const column_t ColEnd = GetSelectColumn(Sel.m_cpEnd.m_iColumn);

	while (it.first < it.second) {
		for (int c = Sel.m_cpStart.m_iChannel; c <= Sel.m_cpEnd.m_iChannel; ++c) {
			auto NoteBegin = it.first.Get(c);
			auto NoteEnd = it.second.Get(c);
			if (c == Sel.m_cpStart.m_iChannel && ColStart > 0) {		// // //
				auto Temp = NoteEnd;
				CopyNoteSection(&NoteEnd, &NoteBegin, PASTE_DEFAULT, COLUMN_NOTE, static_cast<column_t>(ColStart - 1));
				CopyNoteSection(&NoteBegin, &Temp, PASTE_DEFAULT, COLUMN_NOTE, static_cast<column_t>(ColStart - 1));
			}
			if (c == Sel.m_cpEnd.m_iChannel && ColEnd < COLUMN_EFF4) {
				auto Temp = NoteEnd;
				CopyNoteSection(&NoteEnd, &NoteBegin, PASTE_DEFAULT, static_cast<column_t>(ColEnd + 1), COLUMN_EFF4);
				CopyNoteSection(&NoteBegin, &Temp, PASTE_DEFAULT, static_cast<column_t>(ColEnd + 1), COLUMN_EFF4);
			}
			it.first.Set(c, NoteEnd);
			it.second.Set(c, NoteBegin);
		}
		++it.first;
		--it.second;
	}
}



CPActionReplaceInst::CPActionReplaceInst(unsigned Index) :
	m_iInstrumentIndex(Index)
{
}

bool CPActionReplaceInst::SaveState(const CMainFrame &MainFrm)
{
	if (!m_pUndoState->IsSelecting || m_iInstrumentIndex > static_cast<unsigned>(MAX_INSTRUMENTS))
		return false;
	return CPSelectionAction::SaveState(MainFrm);
}

void CPActionReplaceInst::Redo(CMainFrame &MainFrm)
{
	auto it = GetIterators(*GET_SONG_VIEW());
	const CSelection &Sel = m_pUndoState->Selection;

	const int cBegin = Sel.GetChanStart() + (Sel.IsColumnSelected(COLUMN_INSTRUMENT, Sel.GetChanStart()) ? 0 : 1);
	const int cEnd = Sel.GetChanEnd() - (Sel.IsColumnSelected(COLUMN_INSTRUMENT, Sel.GetChanEnd()) ? 0 : 1);

	do for (int i = cBegin; i <= cEnd; ++i) {
		const auto &Note = it.first.Get(i);
		if (Note.Instrument != MAX_INSTRUMENTS && Note.Instrument != HOLD_INSTRUMENT)		// // // 050B
			const_cast<stChanNote &>(Note).Instrument = m_iInstrumentIndex; // TODO: non-const CPatternIterator
	} while (++it.first <= it.second);
}



CPActionDragDrop::CPActionDragDrop(std::unique_ptr<CPatternClipData> pClipData, bool bDelete, bool bMix, const CSelection &pDragTarget) :
	m_bDragDelete(bDelete), m_bDragMix(bMix)
//	m_pClipData(pClipData), m_dragTarget(pDragTarget)
{
	m_pClipData		= std::move(pClipData);
	m_dragTarget	= pDragTarget;
	m_iPastePos		= PASTE_DRAG;
}

bool CPActionDragDrop::SaveState(const CMainFrame &MainFrm)
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	if (m_bDragDelete)
		m_pAuxiliaryClipData = pPatternEditor->CopyRaw();
	if (!SetTargetSelection(MainFrm, m_newSelection))		// // //
		return false;
	m_pUndoClipData = pPatternEditor->CopyRaw();
	return true;
}

void CPActionDragDrop::Undo(CMainFrame &MainFrm)
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	pPatternEditor->SetSelection(m_newSelection);
	pPatternEditor->PasteRaw(*m_pUndoClipData);
	if (m_bDragDelete)
		pPatternEditor->PasteRaw(*m_pAuxiliaryClipData.get(), m_pUndoState->Selection.m_cpStart);
}

void CPActionDragDrop::Redo(CMainFrame &MainFrm)
{
	if (m_bDragDelete)
		DeleteSelection(*GET_SONG_VIEW(), m_pUndoState->Selection);		// // //
//	GET_PATTERN_EDITOR()->Paste(*m_pClipData, m_iPasteMode, m_iPastePos);		// // //
	GET_PATTERN_EDITOR()->DragPaste(*m_pClipData, m_dragTarget, m_bDragMix);
}



bool CPActionPatternLen::SaveState(const CMainFrame &MainFrm)
{
	m_iOldPatternLen = GET_SONG_VIEW()->GetSong().GetPatternLength();
	return m_iNewPatternLen != m_iOldPatternLen;
}

void CPActionPatternLen::Undo(CMainFrame &MainFrm)
{
	GET_SONG_VIEW()->GetSong().SetPatternLength(m_iOldPatternLen);
	UPDATE_CONTROLS();
}

void CPActionPatternLen::Redo(CMainFrame &MainFrm)
{
	GET_SONG_VIEW()->GetSong().SetPatternLength(m_iNewPatternLen);
	UPDATE_CONTROLS();
}

bool CPActionPatternLen::Merge(const CAction &Other)		// // //
{
	auto pAction = dynamic_cast<const CPActionPatternLen *>(&Other);
	if (!pAction)
		return false;

	*m_pRedoState = *pAction->m_pRedoState;
	m_iNewPatternLen = pAction->m_iNewPatternLen;
	return true;
}



CPActionStretch::CPActionStretch(const std::vector<int> &Stretch) :
	m_iStretchMap(Stretch)
{
}

bool CPActionStretch::SaveState(const CMainFrame &MainFrm)
{
	if (m_iStretchMap.empty())
		return false;
	if (!ValidateSelection(*GET_PATTERN_EDITOR()))
		return false;
	return CPSelectionAction::SaveState(MainFrm);
}

void CPActionStretch::Redo(CMainFrame &MainFrm)
{
	CSongView *pSongView = GET_SONG_VIEW();
	auto it = GetIterators(*pSongView);
	const CSelection &Sel = m_pUndoState->Selection;
	CPatternIterator s {it.first};

	const column_t ColStart = GetSelectColumn(Sel.m_cpStart.m_iColumn);
	const column_t ColEnd = GetSelectColumn(Sel.m_cpEnd.m_iColumn);

	int Pos = 0;
	int Offset = 0;
	int oldRow = -1;
	do {
		stChanNote BLANK;
		for (int i = Sel.m_cpStart.m_iChannel; i <= Sel.m_cpEnd.m_iChannel; ++i) {
			const auto &Source = (Offset < m_pUndoClipData->ClipInfo.Rows && m_iStretchMap[Pos] > 0) ?
				*(m_pUndoClipData->GetPattern(i - Sel.m_cpStart.m_iChannel, Offset)) : BLANK;		// // //
			auto Target = it.first.Get(i);
			CopyNoteSection(&Target, &Source, PASTE_DEFAULT,
							i == Sel.m_cpStart.m_iChannel ? ColStart : COLUMN_NOTE,
							i == Sel.m_cpEnd.m_iChannel ? ColEnd : COLUMN_EFF4);
			it.first.Set(i, Target);
		}
		int dist = m_iStretchMap[Pos++];
		for (int i = 0; i < dist; ++i) {
			++Offset;
			oldRow = s.m_iRow;
			++s;
			if (s.m_iRow <= oldRow)
				Offset += pSongView->GetSong().GetPatternLength() + s.m_iRow - oldRow - 1;
		}
		Pos %= m_iStretchMap.size();
	} while (++it.first <= it.second);
}



CPActionEffColumn::CPActionEffColumn(int Channel, int Count) :		// // //
	m_iChannel(Channel), m_iNewColumns(Count)
{
}

bool CPActionEffColumn::SaveState(const CMainFrame &MainFrm)
{
	if (m_iNewColumns >= static_cast<int>(MAX_EFFECT_COLUMNS))
		return false;
	m_iOldColumns = GET_SONG_VIEW()->GetEffectColumnCount(m_iChannel);
	return true;
}

void CPActionEffColumn::Undo(CMainFrame &MainFrm)
{
	GET_SONG_VIEW()->SetEffectColumnCount(m_iChannel, m_iOldColumns);
}

void CPActionEffColumn::Redo(CMainFrame &MainFrm)
{
	GET_SONG_VIEW()->SetEffectColumnCount(m_iChannel, m_iNewColumns);
}

void CPActionEffColumn::UpdateViews(CMainFrame &MainFrm) const		// // //
{
	MainFrm.GetActiveDocument()->UpdateAllViews(NULL, UPDATE_COLUMNS);
}



CPActionHighlight::CPActionHighlight(const stHighlight &Hl) :		// // //
	m_NewHighlight(Hl)
{
}

bool CPActionHighlight::SaveState(const CMainFrame &MainFrm)
{
	m_OldHighlight = GET_MODULE()->GetHighlight(0);
	return m_NewHighlight != m_OldHighlight;
}

void CPActionHighlight::Undo(CMainFrame &MainFrm)
{
	GET_MODULE()->SetHighlight(m_OldHighlight);
}

void CPActionHighlight::Redo(CMainFrame &MainFrm)
{
	GET_MODULE()->SetHighlight(m_NewHighlight);
}

void CPActionHighlight::UpdateViews(CMainFrame &MainFrm) const
{
	MainFrm.GetActiveDocument()->UpdateAllViews(NULL, UPDATE_HIGHLIGHT);
}



bool CPActionUniquePatterns::SaveState(const CMainFrame &MainFrm) {
	const auto &Song = GET_SONG_VIEW()->GetSong();
	const int Frames = Song.GetFrameCount();

	songNew_ = std::make_unique<CSongData>(Song.GetPatternLength());
	songNew_->SetSongSpeed(Song.GetSongSpeed());
	songNew_->SetSongTempo(Song.GetSongTempo());
	songNew_->SetFrameCount(Frames);
	songNew_->SetSongGroove(Song.GetSongGroove());
	songNew_->SetTitle(Song.GetTitle());

	GET_SONG_VIEW()->GetChannelOrder().ForeachChannel([&] (chan_id_t chan) {
		songNew_->SetEffectColumnCount(chan, Song.GetEffectColumnCount(chan));
		for (int f = 0; f < Frames; f++) {
			songNew_->SetFramePattern(f, chan, f);
			songNew_->GetPattern(chan, f) = Song.GetPatternOnFrame(chan, f);
		}
	});

	return true;
}

void CPActionUniquePatterns::Undo(CMainFrame &MainFrm) {
	songNew_ = GET_MODULE()->ReplaceSong(index_, std::move(song_));
}

void CPActionUniquePatterns::Redo(CMainFrame &MainFrm) {
	song_ = GET_MODULE()->ReplaceSong(index_, std::move(songNew_));
}

void CPActionUniquePatterns::UpdateViews(CMainFrame &MainFrm) const {
	MainFrm.GetActiveDocument()->UpdateAllViews(NULL, UPDATE_FRAME);
}



bool CPActionClearAll::SaveState(const CMainFrame &MainFrm) {
	const auto &Song = GET_SONG_VIEW()->GetSong();

	songNew_ = std::make_unique<CSongData>(Song.GetPatternLength());
	songNew_->SetSongSpeed(Song.GetSongSpeed());
	songNew_->SetSongTempo(Song.GetSongTempo());
	songNew_->SetSongGroove(Song.GetSongGroove());
	songNew_->SetTitle(Song.GetTitle());

	return true;
}

void CPActionClearAll::Undo(CMainFrame &MainFrm) {
	songNew_ = GET_MODULE()->ReplaceSong(index_, std::move(song_));
}

void CPActionClearAll::Redo(CMainFrame &MainFrm) {
	song_ = GET_MODULE()->ReplaceSong(index_, std::move(songNew_));
}

void CPActionClearAll::UpdateViews(CMainFrame &MainFrm) const {
	MainFrm.GetActiveDocument()->UpdateAllViews(NULL, UPDATE_TRACK);
}
