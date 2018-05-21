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
#include "SelectionRange.h"		// // //
#include "FamiTrackerModule.h"		// // //
#include "SongView.h"		// // //

// // // all dependencies on CMainFrame
#define GET_VIEW() MainFrm.GetTrackerView()
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

	if ((m_iPastePos == paste_pos_t::SELECTION || m_iPastePos == paste_pos_t::FILL) && !m_bSelecting)
		m_iPastePos = paste_pos_t::CURSOR;

	switch (m_iPastePos) { // Xpos.Column will be written later
	case paste_pos_t::CURSOR:
		Start = m_pUndoState->Cursor;
		break;
	case paste_pos_t::DRAG:
		Start.Ypos.Frame = m_dragTarget.GetFrameStart();
		Start.Ypos.Row = m_dragTarget.GetRowStart();
		Start.Xpos.Track = m_dragTarget.GetChanStart();
		break;
	case paste_pos_t::SELECTION:
	case paste_pos_t::FILL:
		Start.Ypos.Frame = m_selection.GetFrameStart();
		Start.Ypos.Row = m_selection.GetRowStart();
		Start.Xpos.Track = m_selection.GetChanStart();
		break;
	}

	auto pSongView = GET_SONG_VIEW();

	CPatternIterator End(*pSongView, Start);

	if (m_iPasteMode == paste_mode_t::INSERT) {
		End.m_iFrame = Start.Ypos.Frame;
		End.m_iRow = pPatternEditor->GetCurrentPatternLength(End.m_iFrame) - 1;
	}
	else
		End += m_ClipData.ClipInfo.Rows - 1;

	switch (m_iPastePos) {
	case paste_pos_t::FILL:
		End.m_iFrame = m_selection.GetFrameEnd();
		End.m_iRow = m_selection.GetRowEnd();
		End.m_iChannel = m_selection.GetChanEnd();
		Start.Xpos.Column = GetCursorStartColumn(m_ClipData.ClipInfo.StartColumn);
		End.m_iColumn = GetCursorEndColumn(
			!((End.m_iChannel - Start.Xpos.Track + 1) % m_ClipData.ClipInfo.Channels) ?
			m_ClipData.ClipInfo.EndColumn :
			static_cast<column_t>(value_cast(column_t::Effect1) + pSongView->GetEffectColumnCount(End.m_iChannel)));
		break;
	case paste_pos_t::DRAG:
		End.m_iChannel += m_ClipData.ClipInfo.Channels - 1;
		Start.Xpos.Column = m_dragTarget.GetColStart();
		End.m_iColumn = m_dragTarget.GetColEnd();
		break;
	default:
		End.m_iChannel += m_ClipData.ClipInfo.Channels - 1;
		Start.Xpos.Column = GetCursorStartColumn(m_ClipData.ClipInfo.StartColumn);
		End.m_iColumn = GetCursorEndColumn(m_ClipData.ClipInfo.EndColumn);
	}

	const bool bOverflow = FTEnv.GetSettings()->General.bOverflowPaste;
	if (!bOverflow && End.m_iFrame > Start.Ypos.Frame) {
		End.m_iFrame = Start.Ypos.Frame;
		End.m_iRow = pPatternEditor->GetCurrentPatternLength(End.m_iFrame) - 1;
	}

	const cursor_column_t EFBEGIN = GetCursorStartColumn(column_t::Effect1);
	int OFFS = 3 * (value_cast(GetSelectColumn(m_pUndoState->Cursor.Xpos.Column)) - value_cast(m_ClipData.ClipInfo.StartColumn));
	if (OFFS < static_cast<int>(value_cast(EFBEGIN) - value_cast(Start.Xpos.Column)))
		OFFS = value_cast(EFBEGIN) - value_cast(Start.Xpos.Column);
	if (Start.Xpos.Track == End.m_iChannel && Start.Xpos.Column >= EFBEGIN && End.m_iColumn >= EFBEGIN) {
		if (m_iPastePos != paste_pos_t::DRAG) {
			End.m_iColumn = static_cast<cursor_column_t>(value_cast(End.m_iColumn) + OFFS);
			Start.Xpos.Column = static_cast<cursor_column_t>(value_cast(Start.Xpos.Column) + OFFS);
			if (End.m_iColumn > cursor_column_t::EFF4_PARAM2)
				End.m_iColumn = cursor_column_t::EFF4_PARAM2;
		}
	}

	CSelection New;
	New.m_cpStart = Start;
	New.m_cpEnd = End.GetCursor();
	pPatternEditor->SetSelection(New);

	sel_condition_t Cond = pPatternEditor->GetSelectionCondition();
	if (Cond == sel_condition_t::CLEAN) {
		Sel = New;
		return true;
	}
	else {
		pPatternEditor->SetSelection(m_selection);
		if (!m_bSelecting) pPatternEditor->CancelSelection();
		int Confirm = IDYES;
		switch (Cond) {
		case sel_condition_t::REPEATED_ROW:
			Confirm = AfxMessageBox(IDS_PASTE_REPEATED_ROW, MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2);
			break;
		case sel_condition_t::NONTERMINAL_SKIP: case sel_condition_t::TERMINAL_SKIP:
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
	auto [b, e] = CPatternIterator::FromSelection(Sel, view);
	const column_t ColStart = GetSelectColumn(b.m_iColumn);
	const column_t ColEnd = GetSelectColumn(e.m_iColumn);

	ft0cc::doc::pattern_note BLANK;

	do for (int i = b.m_iChannel; i <= e.m_iChannel; ++i) {
		auto NoteData = b.Get(i);
		CopyNoteSection(NoteData, BLANK,
			i == b.m_iChannel ? ColStart : column_t::Note,
			i == e.m_iChannel ? ColEnd : column_t::Effect4);
		b.Set(i, NoteData);
	} while (++b <= e);
}

bool CPatternAction::ValidateSelection(const CPatternEditor &Editor) const		// // //
{
	if (!m_pUndoState->IsSelecting)
		return false;
	switch (Editor.GetSelectionCondition(m_pUndoState->Selection)) {
	case sel_condition_t::CLEAN:
		return true;
	case sel_condition_t::REPEATED_ROW:
		static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(IDS_SEL_REPEATED_ROW); break;
	case sel_condition_t::NONTERMINAL_SKIP:
		static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(IDS_SEL_NONTERMINAL_SKIP); break;
	case sel_condition_t::TERMINAL_SKIP:
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
	m_UndoClipData = pPatternEditor->CopyRaw(m_pUndoState->Selection);
	return true;
}

void CPSelectionAction::Undo(CMainFrame &MainFrm)
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	pPatternEditor->PasteRaw(m_UndoClipData, m_pUndoState->Selection.m_cpStart);
}



// // // built-in pattern action subtypes



CPActionEditNote::CPActionEditNote(const ft0cc::doc::pattern_note &Note) :
	m_NewNote(Note)
{
}

bool CPActionEditNote::SaveState(const CMainFrame &MainFrm)
{
	m_OldNote = GET_SONG_VIEW()->GetPatternOnFrame(m_pUndoState->Cursor.Xpos.Track, m_pUndoState->Cursor.Ypos.Frame)
		.GetNoteOn(m_pUndoState->Cursor.Ypos.Row);		// // //
	return true;
}

void CPActionEditNote::Undo(CMainFrame &MainFrm)
{
	GET_SONG_VIEW()->GetPatternOnFrame(m_pUndoState->Cursor.Xpos.Track, m_pUndoState->Cursor.Ypos.Frame)
		.SetNoteOn(m_pUndoState->Cursor.Ypos.Row, m_OldNote);		// // //
}

void CPActionEditNote::Redo(CMainFrame &MainFrm)
{
	GET_SONG_VIEW()->GetPatternOnFrame(m_pUndoState->Cursor.Xpos.Track, m_pUndoState->Cursor.Ypos.Frame)
		.SetNoteOn(m_pUndoState->Cursor.Ypos.Row, m_NewNote);		// // //
}



CPActionReplaceNote::CPActionReplaceNote(const ft0cc::doc::pattern_note &Note, int Frame, int Row, int Channel) :
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
	m_OldNote = GET_SONG_VIEW()->GetPatternOnFrame(m_pUndoState->Cursor.Xpos.Track, m_pUndoState->Cursor.Ypos.Frame)
		.GetNoteOn(pSongView->GetSong().GetPatternLength() - 1);		// // //
	return true;
}

void CPActionInsertRow::Undo(CMainFrame &MainFrm)
{
	CSongView *pSongView = GET_SONG_VIEW();

	pSongView->PullUp(m_pUndoState->Cursor.Xpos.Track, m_pUndoState->Cursor.Ypos.Frame, m_pUndoState->Cursor.Ypos.Row);
	pSongView->GetPatternOnFrame(m_pUndoState->Cursor.Xpos.Track, m_pUndoState->Cursor.Ypos.Frame)
		.SetNoteOn(pSongView->GetSong().GetPatternLength() - 1, m_OldNote);		// // //
}

void CPActionInsertRow::Redo(CMainFrame &MainFrm)
{
	GET_SONG_VIEW()->InsertRow(m_pUndoState->Cursor.Xpos.Track, m_pUndoState->Cursor.Ypos.Frame, m_pUndoState->Cursor.Ypos.Row);
}



CPActionDeleteRow::CPActionDeleteRow(bool PullUp, bool Backspace) :
	m_bPullUp(PullUp), m_bBack(Backspace)
{
}

bool CPActionDeleteRow::SaveState(const CMainFrame &MainFrm)
{
	if (m_bBack && !m_pUndoState->Cursor.Ypos.Row)
		return false;
	m_iRow = m_pUndoState->Cursor.Ypos.Row - (m_bBack ? 1 : 0);
	m_OldNote = GET_SONG_VIEW()->GetPatternOnFrame(m_pUndoState->Cursor.Xpos.Track, m_pUndoState->Cursor.Ypos.Frame)
		.GetNoteOn(m_iRow);		// // //

	m_NewNote = m_OldNote;
	switch (m_pUndoState->Cursor.Xpos.Column) {
	case cursor_column_t::NOTE:			// Note
		m_NewNote.set_note(ft0cc::doc::pitch::none);
		m_NewNote.set_oct(0);
		m_NewNote.set_inst(MAX_INSTRUMENTS);	// Fix the old behaviour
		m_NewNote.set_vol(MAX_VOLUME);
		break;
	case cursor_column_t::INSTRUMENT1:		// Instrument
	case cursor_column_t::INSTRUMENT2:
		m_NewNote.set_inst(MAX_INSTRUMENTS);
		break;
	case cursor_column_t::VOLUME:			// Volume
		m_NewNote.set_vol(MAX_VOLUME);
		break;
	case cursor_column_t::EFF1_NUM:		// Effect 1
	case cursor_column_t::EFF1_PARAM1:
	case cursor_column_t::EFF1_PARAM2:
		m_NewNote.set_fx_cmd(0, { });
		break;
	case cursor_column_t::EFF2_NUM:		// Effect 2
	case cursor_column_t::EFF2_PARAM1:
	case cursor_column_t::EFF2_PARAM2:
		m_NewNote.set_fx_cmd(1, { });
		break;
	case cursor_column_t::EFF3_NUM:		// Effect 3
	case cursor_column_t::EFF3_PARAM1:
	case cursor_column_t::EFF3_PARAM2:
		m_NewNote.set_fx_cmd(2, { });
		break;
	case cursor_column_t::EFF4_NUM:		// Effect 4
	case cursor_column_t::EFF4_PARAM1:
	case cursor_column_t::EFF4_PARAM2:
		m_NewNote.set_fx_cmd(3, { });
		break;
	}

	return true;
}

void CPActionDeleteRow::Undo(CMainFrame &MainFrm)
{
	CSongView *pSongView = GET_SONG_VIEW();
	if (m_bPullUp)
		pSongView->InsertRow(m_pUndoState->Cursor.Xpos.Track, m_pUndoState->Cursor.Ypos.Frame, m_iRow);
	pSongView->GetPatternOnFrame(m_pUndoState->Cursor.Xpos.Track, m_pUndoState->Cursor.Ypos.Frame)
		.SetNoteOn(m_iRow, m_OldNote);		// // //
}

void CPActionDeleteRow::Redo(CMainFrame &MainFrm)
{
	CSongView *pSongView = GET_SONG_VIEW();
	pSongView->GetPatternOnFrame(m_pUndoState->Cursor.Xpos.Track, m_pUndoState->Cursor.Ypos.Frame)
		.SetNoteOn(m_iRow, m_NewNote);		// // //
	if (m_bPullUp)
		pSongView->PullUp(m_pUndoState->Cursor.Xpos.Track, m_pUndoState->Cursor.Ypos.Frame, m_iRow);
}



CPActionScrollField::CPActionScrollField(int Amount) :		// // //
	m_iAmount(Amount)
{
}

bool CPActionScrollField::SaveState(const CMainFrame &MainFrm)
{
	const auto ScrollFunc = [&] (unsigned char Old, int Limit) {
		int New = static_cast<int>(Old) + m_iAmount;
		if (FTEnv.GetSettings()->General.bWrapPatternValue) {
			New %= Limit;
			if (New < 0)
				New += Limit;
		}
		else {
			New = std::clamp(New, 0, Limit - 1);
		}
		return static_cast<unsigned char>(New);
	};

	m_OldNote = GET_SONG_VIEW()->GetPatternOnFrame(m_pUndoState->Cursor.Xpos.Track, m_pUndoState->Cursor.Ypos.Frame)
		.GetNoteOn(m_pUndoState->Cursor.Ypos.Row);		// // //
	m_NewNote = m_OldNote;

	switch (m_pUndoState->Cursor.Xpos.Column) {
	case cursor_column_t::INSTRUMENT1: case cursor_column_t::INSTRUMENT2:
		m_NewNote.set_inst(ScrollFunc(m_NewNote.inst(), MAX_INSTRUMENTS));
		return m_OldNote.inst() < MAX_INSTRUMENTS && m_OldNote.inst() != HOLD_INSTRUMENT;		// // // 050B
	case cursor_column_t::VOLUME:
		m_NewNote.set_vol(ScrollFunc(m_NewNote.vol(), MAX_VOLUME));
		return m_OldNote.vol() < MAX_VOLUME;
	case cursor_column_t::EFF1_NUM: case cursor_column_t::EFF1_PARAM1: case cursor_column_t::EFF1_PARAM2:
		m_NewNote.set_fx_param(0, ScrollFunc(m_NewNote.fx_param(0), 0x100));
		return m_OldNote.fx_name(0) != ft0cc::doc::effect_type::none;
	case cursor_column_t::EFF2_NUM: case cursor_column_t::EFF2_PARAM1: case cursor_column_t::EFF2_PARAM2:
		m_NewNote.set_fx_param(1, ScrollFunc(m_NewNote.fx_param(1), 0x100));
		return m_OldNote.fx_name(1) != ft0cc::doc::effect_type::none;
	case cursor_column_t::EFF3_NUM: case cursor_column_t::EFF3_PARAM1: case cursor_column_t::EFF3_PARAM2:
		m_NewNote.set_fx_param(2, ScrollFunc(m_NewNote.fx_param(2), 0x100));
		return m_OldNote.fx_name(2) != ft0cc::doc::effect_type::none;
	case cursor_column_t::EFF4_NUM: case cursor_column_t::EFF4_PARAM1: case cursor_column_t::EFF4_PARAM2:
		m_NewNote.set_fx_param(3, ScrollFunc(m_NewNote.fx_param(3), 0x100));
		return m_OldNote.fx_name(3) != ft0cc::doc::effect_type::none;
	}

	return false;
}

void CPActionScrollField::Undo(CMainFrame &MainFrm)
{
	GET_SONG_VIEW()->GetPatternOnFrame(m_pUndoState->Cursor.Xpos.Track, m_pUndoState->Cursor.Ypos.Frame)
		.SetNoteOn(m_pUndoState->Cursor.Ypos.Row, m_OldNote);		// // //
}

void CPActionScrollField::Redo(CMainFrame &MainFrm)
{
	GET_SONG_VIEW()->GetPatternOnFrame(m_pUndoState->Cursor.Xpos.Track, m_pUndoState->Cursor.Ypos.Frame)
		.SetNoteOn(m_pUndoState->Cursor.Ypos.Row, m_NewNote);		// // //
}



// // // TODO: move stuff in CPatternAction::SetTargetSelection to the redo state
/*
CPSelectionAction::~CPSelectionAction()
{
}

bool CPSelectionAction::SaveState(const CMainFrame &MainFrm)
{
	const CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	&m_UndoClipData = pPatternEditor->CopyRaw(m_pUndoState->Selection);
	return true;
}

void CPSelectionAction::Undo(CMainFrame &MainFrm)
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	pPatternEditor->PasteRaw(&m_UndoClipData, m_pUndoState->Selection.m_cpStart);
}
*/

CPActionPaste::CPActionPaste(CPatternClipData ClipData, paste_mode_t Mode, paste_pos_t Pos)
{
	m_ClipData = std::move(ClipData);
	m_iPasteMode = Mode;
	m_iPastePos = Pos;
}

bool CPActionPaste::SaveState(const CMainFrame &MainFrm) {
	if (!SetTargetSelection(MainFrm, m_newSelection))		// // //
		return false;
	m_UndoClipData = GET_PATTERN_EDITOR()->CopyRaw();
	return true;
}

void CPActionPaste::Undo(CMainFrame &MainFrm) {
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	pPatternEditor->SetSelection(m_newSelection);		// // //
	pPatternEditor->PasteRaw(m_UndoClipData);
}

void CPActionPaste::Redo(CMainFrame &MainFrm) {
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	pPatternEditor->Paste(m_ClipData, m_iPasteMode, m_iPastePos);		// // //
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
		m_pUndoState->Selection.m_cpEnd.Ypos.Row + 1,
		m_pUndoState->Selection.m_cpStart.Xpos.Track,
		m_pUndoState->Selection.m_cpStart.Xpos.Column,
		m_pUndoState->Selection.m_cpEnd.Ypos.Frame
	};
	m_UndoHead = pPatternEditor->CopyRaw(m_pUndoState->Selection);
	int Length = pPatternEditor->GetCurrentPatternLength(m_cpTailPos.Ypos.Frame) - 1;
	if (m_cpTailPos.Ypos.Row <= Length)
		m_UndoTail = pPatternEditor->CopyRaw(CSelection {m_cpTailPos, CCursorPos {
			Length,
			m_pUndoState->Selection.m_cpEnd.Xpos.Track,
			m_pUndoState->Selection.m_cpEnd.Xpos.Column,
			m_cpTailPos.Ypos.Frame
		}});
	return true;
}

void CPActionDeleteAtSel::Undo(CMainFrame &MainFrm)
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	pPatternEditor->PasteRaw(m_UndoHead, m_pUndoState->Selection.m_cpStart);
	if (m_UndoTail.ContainsData())
		pPatternEditor->PasteRaw(m_UndoTail, m_cpTailPos);
}

void CPActionDeleteAtSel::Redo(CMainFrame &MainFrm)
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();

	CSelection Sel(m_pUndoState->Selection);
	Sel.m_cpEnd.Ypos.Row = pPatternEditor->GetCurrentPatternLength(Sel.m_cpEnd.Ypos.Frame) - 1;
	DeleteSelection(*GET_SONG_VIEW(), Sel);
	if (m_UndoTail.ContainsData())
		pPatternEditor->PasteRaw(m_UndoTail, m_pUndoState->Selection.m_cpStart);
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
		pPatternEditor->GetCurrentPatternLength(m_pUndoState->Selection.m_cpEnd.Ypos.Frame) - 1,
		m_pUndoState->Selection.m_cpStart.Xpos.Track,
		m_pUndoState->Selection.m_cpStart.Xpos.Column,
		m_pUndoState->Selection.m_cpEnd.Ypos.Frame
	};
	CCursorPos HeadEnd {
		m_cpTailPos.Ypos.Row,
		m_pUndoState->Selection.m_cpEnd.Xpos.Track,
		m_pUndoState->Selection.m_cpEnd.Xpos.Column,
		m_cpTailPos.Ypos.Frame
	};

	m_UndoTail = pPatternEditor->CopyRaw(CSelection {m_cpTailPos, CCursorPos {HeadEnd}});
	if (--HeadEnd.Ypos.Row < 0)
		HeadEnd.Ypos.Row += pPatternEditor->GetCurrentPatternLength(--HeadEnd.Ypos.Frame);
	if (m_pUndoState->Selection.m_cpStart.Ypos <= HeadEnd.Ypos) {
		m_UndoHead = pPatternEditor->CopyRaw(CSelection {m_pUndoState->Selection.m_cpStart, HeadEnd});
		m_cpHeadPos = m_pUndoState->Selection.m_cpStart;
		if (++m_cpHeadPos.Ypos.Row >= pPatternEditor->GetCurrentPatternLength(m_cpHeadPos.Ypos.Frame)) {
			++m_cpHeadPos.Ypos.Frame;
			m_cpHeadPos.Ypos.Row = 0;
		}
	}

	return true;
}

void CPActionInsertAtSel::Undo(CMainFrame &MainFrm)
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	pPatternEditor->PasteRaw(m_UndoTail, m_cpTailPos);
	if (m_UndoHead.ContainsData())
		pPatternEditor->PasteRaw(m_UndoHead, m_pUndoState->Selection.m_cpStart);
}

void CPActionInsertAtSel::Redo(CMainFrame &MainFrm)
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();

	CSelection Sel(m_pUndoState->Selection);
	Sel.m_cpEnd.Ypos.Row = pPatternEditor->GetCurrentPatternLength(Sel.m_cpEnd.Ypos.Frame) - 1;
	DeleteSelection(*GET_SONG_VIEW(), Sel);
	if (m_UndoHead.ContainsData())
		pPatternEditor->PasteRaw(m_UndoHead, m_cpHeadPos);
}



CPActionTranspose::CPActionTranspose(int Amount) : m_iTransposeAmount(Amount)
{
}

void CPActionTranspose::Redo(CMainFrame &MainFrm)
{
	CSongView *pSongView = GET_SONG_VIEW();
	auto [b, e] = GetIterators(*pSongView);

	int ChanStart     = (m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpStart : m_pUndoState->Cursor).Xpos.Track;
	int ChanEnd       = (m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpEnd : m_pUndoState->Cursor).Xpos.Track;

	const bool bSingular = b == e && !m_pUndoState->IsSelecting;
	const unsigned Length = pSongView->GetSong().GetPatternLength();

	int Row = 0;		// // //
	int oldRow = -1;
	do {
		if (b.m_iRow <= oldRow)
			Row += Length + b.m_iRow - oldRow - 1;
		for (int i = ChanStart; i <= ChanEnd; ++i) {
			if (!m_pUndoState->Selection.IsColumnSelected(column_t::Note, i))
				continue;
			ft0cc::doc::pattern_note Note = *(m_UndoClipData.GetPattern(i - ChanStart, Row));		// // //
			if (Note.note() == ft0cc::doc::pitch::echo) {
				if (!bSingular)
					continue;
				switch (m_iTransposeAmount) {		// // //
				case -1: case 1:
					if (Note.oct() > 0)
						Note.set_oct(Note.oct() - 1);
					break;
				case -NOTE_RANGE: case NOTE_RANGE:
					if (Note.oct() < ECHO_BUFFER_LENGTH - 1)
						Note.set_oct(Note.oct() + 1);
					break;
				}
			}
			else if (is_note(Note.note())) {		// // //
				int NewNote = std::clamp(Note.midi_note() + m_iTransposeAmount, 0, NOTE_COUNT - 1);
				Note.set_note(ft0cc::doc::pitch_from_midi(NewNote));
				Note.set_oct(ft0cc::doc::oct_from_midi(NewNote));
			}
			else
				continue;
			b.Set(i, Note);
		}
		++Row;
		oldRow = b.m_iRow;
	} while (++b <= e);
}



CPActionScrollValues::CPActionScrollValues(int Amount) : m_iAmount(Amount)
{
}

void CPActionScrollValues::Redo(CMainFrame &MainFrm)
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	CSongView *pSongView = GET_SONG_VIEW();
	auto [b, e] = GetIterators(*pSongView);
	int ChanStart     = (m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpStart : m_pUndoState->Cursor).Xpos.Track;
	int ChanEnd       = (m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpEnd : m_pUndoState->Cursor).Xpos.Track;
	column_t ColStart = GetSelectColumn(
		(m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpStart : m_pUndoState->Cursor).Xpos.Column);
	column_t ColEnd   = GetSelectColumn(
		(m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpEnd : m_pUndoState->Cursor).Xpos.Column);

	const bool bSingular = b == e && !m_pUndoState->IsSelecting;
	const unsigned Length = pSongView->GetSong().GetPatternLength();

	const auto WarpFunc = [this] (unsigned char x, int Lim) {
		int Val = x + m_iAmount;
		if (FTEnv.GetSettings()->General.bWrapPatternValue) {
			Val %= Lim;
			if (Val < 0) Val += Lim;
		}
		else {
			if (Val >= Lim) Val = Lim - 1;
			if (Val < 0) Val = 0;
		}
		return static_cast<unsigned char>(Val);
	};

	int Row = 0;
	int oldRow = -1;
	do {
		if (b.m_iRow <= oldRow)
			Row += Length + b.m_iRow - oldRow - 1;
		for (int i = ChanStart; i <= ChanEnd; ++i) {
			auto Note = *(m_UndoClipData.GetPattern(i - ChanStart, Row));		// // //
			for (column_t k = column_t::Instrument; k <= column_t::Effect4; k = static_cast<column_t>(value_cast(k) + 1)) {
				if (i == ChanStart && k < ColStart)
					continue;
				if (i == ChanEnd && k > ColEnd)
					continue;
				switch (k) {
				case column_t::Instrument:
					if (Note.inst() == MAX_INSTRUMENTS || Note.inst() == HOLD_INSTRUMENT) break;		// // // 050B
					Note.set_inst(WarpFunc(Note.inst(), MAX_INSTRUMENTS));
					break;
				case column_t::Volume:
					if (Note.vol() == MAX_VOLUME) break;
					Note.set_vol(WarpFunc(Note.vol(), MAX_VOLUME));
					break;
				case column_t::Effect1: case column_t::Effect2: case column_t::Effect3: case column_t::Effect4:
				{
					unsigned fx = value_cast(k) - value_cast(column_t::Effect1);
					if (Note.fx_name(fx) == ft0cc::doc::effect_type::none)
						break;
					if (bSingular) switch (Note.fx_name(fx)) {
					case ft0cc::doc::effect_type::SWEEPUP: case ft0cc::doc::effect_type::SWEEPDOWN: case ft0cc::doc::effect_type::ARPEGGIO: case ft0cc::doc::effect_type::VIBRATO: case ft0cc::doc::effect_type::TREMOLO:
					case ft0cc::doc::effect_type::SLIDE_UP: case ft0cc::doc::effect_type::SLIDE_DOWN: case ft0cc::doc::effect_type::VOLUME_SLIDE: case ft0cc::doc::effect_type::DELAYED_VOLUME: case ft0cc::doc::effect_type::TRANSPOSE:
						unsigned char Hi = Note.fx_param(fx) >> 4;
						unsigned char Lo = Note.fx_param(fx) & 0x0F;
						WarpFunc(value_cast(pPatternEditor->GetColumn()) % 3 == 2 ? Hi : Lo, 0x10);
						Note.set_fx_param(fx, (Hi << 4) | Lo);
						continue;
					}
					WarpFunc(Note.fx_param(fx), 0x100);
					break;
				}
				}
			}
			b.Set(i, Note);
		}
		++Row;
		oldRow = b.m_iRow;
	} while (++b <= e);
}



bool CPActionInterpolate::SaveState(const CMainFrame &MainFrm)
{
	const CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	if (!ValidateSelection(*pPatternEditor))
		return false;
	m_iSelectionSize = pPatternEditor->GetSelectionSize(pPatternEditor->GetSelection());
	return CPSelectionAction::SaveState(MainFrm);
}

void CPActionInterpolate::Redo(CMainFrame &MainFrm)
{
	CSongView *pSongView = GET_SONG_VIEW();
	auto [b, e] = GetIterators(*pSongView);
	const CSelection &Sel = m_pUndoState->Selection;

	for (int i = Sel.m_cpStart.Xpos.Track; i <= Sel.m_cpEnd.Xpos.Track; ++i) {
		const int Columns = pSongView->GetEffectColumnCount(i) + 4;		// // //
		for (int j = 0; j < Columns; ++j) {
			if (!Sel.IsColumnSelected(static_cast<column_t>(j), i)) continue;
			CPatternIterator r {b};		// // //
			const auto &StartData = r.Get(i);
			const auto &EndData = e.Get(i);
			double StartValHi = 0., StartValLo = 0.;		// // //
			double EndValHi = 0., EndValLo = 0.;
			double DeltaHi = 0., DeltaLo = 0.;
			bool TwoParam = false;
			ft0cc::doc::effect_type Effect = ft0cc::doc::effect_type::none;
			switch (static_cast<column_t>(j)) {
			case column_t::Note:
				if (!is_note(StartData.note()) || !is_note(EndData.note()))
					continue;
				StartValLo = (float)StartData.midi_note();
				EndValLo = (float)EndData.midi_note();
				break;
			case column_t::Instrument:
				if (StartData.inst() == MAX_INSTRUMENTS || EndData.inst() == MAX_INSTRUMENTS)
					continue;
				if (StartData.inst() == HOLD_INSTRUMENT || EndData.inst() == HOLD_INSTRUMENT)		// // // 050B
					continue;
				StartValLo = (float)StartData.inst();
				EndValLo = (float)EndData.inst();
				break;
			case column_t::Volume:
				if (StartData.vol() == MAX_VOLUME || EndData.vol() == MAX_VOLUME)
					continue;
				StartValLo = (float)StartData.vol();
				EndValLo = (float)EndData.vol();
				break;
			case column_t::Effect1: case column_t::Effect2: case column_t::Effect3: case column_t::Effect4:
				if (StartData.fx_name(j - 3) == ft0cc::doc::effect_type::none || EndData.fx_name(j - 3) == ft0cc::doc::effect_type::none ||
					StartData.fx_name(j - 3) != EndData.fx_name(j - 3))
					continue;
				StartValLo = (float)StartData.fx_param(j - 3);
				EndValLo = (float)EndData.fx_param(j - 3);
				Effect = StartData.fx_name(j - 3);
				switch (Effect) {
				case ft0cc::doc::effect_type::SWEEPUP: case ft0cc::doc::effect_type::SWEEPDOWN: case ft0cc::doc::effect_type::SLIDE_UP: case ft0cc::doc::effect_type::SLIDE_DOWN:
				case ft0cc::doc::effect_type::ARPEGGIO: case ft0cc::doc::effect_type::VIBRATO: case ft0cc::doc::effect_type::TREMOLO:
				case ft0cc::doc::effect_type::VOLUME_SLIDE: case ft0cc::doc::effect_type::DELAYED_VOLUME: case ft0cc::doc::effect_type::TRANSPOSE:
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

			while (++r < e) {
				StartValLo += DeltaLo;
				StartValHi += DeltaHi;
				auto Note = r.Get(i);
				switch (static_cast<column_t>(j)) {
				case column_t::Note:
					Note.set_note(ft0cc::doc::pitch_from_midi((int)StartValLo));
					Note.set_oct(ft0cc::doc::oct_from_midi((int)StartValLo));
					break;
				case column_t::Instrument:
					Note.set_inst((int)StartValLo);
					break;
				case column_t::Volume:
					Note.set_vol((int)StartValLo);
					break;
				case column_t::Effect1: case column_t::Effect2: case column_t::Effect3: case column_t::Effect4:
					Note.set_fx_cmd(j - 3, {Effect, static_cast<uint8_t>((int)StartValLo + ((int)StartValHi << 4))});
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
	auto [b, e] = GetIterators(*pSongView);
	const CSelection &Sel = m_pUndoState->Selection;

	const column_t ColStart = GetSelectColumn(Sel.m_cpStart.Xpos.Column);
	const column_t ColEnd = GetSelectColumn(Sel.m_cpEnd.Xpos.Column);

	while (b < e) {
		for (int c = Sel.m_cpStart.Xpos.Track; c <= Sel.m_cpEnd.Xpos.Track; ++c) {
			auto NoteBegin = b.Get(c);
			auto NoteEnd = e.Get(c);
			if (c == Sel.m_cpStart.Xpos.Track && ColStart > column_t::Note) {		// // //
				auto Temp = NoteEnd;
				CopyNoteSection(NoteEnd, NoteBegin, column_t::Note, static_cast<column_t>(value_cast(ColStart) - 1));
				CopyNoteSection(NoteBegin, Temp, column_t::Note, static_cast<column_t>(value_cast(ColStart) - 1));
			}
			if (c == Sel.m_cpEnd.Xpos.Track && ColEnd < column_t::Effect4) {
				auto Temp = NoteEnd;
				CopyNoteSection(NoteEnd, NoteBegin, static_cast<column_t>(value_cast(ColEnd) + 1), column_t::Effect4);
				CopyNoteSection(NoteBegin, Temp, static_cast<column_t>(value_cast(ColEnd) + 1), column_t::Effect4);
			}
			b.Set(c, NoteEnd);
			e.Set(c, NoteBegin);
		}
		++b;
		--e;
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
	auto [b, e] = GetIterators(*GET_SONG_VIEW());
	const CSelection &Sel = m_pUndoState->Selection;

	const int cBegin = Sel.GetChanStart() + (Sel.IsColumnSelected(column_t::Instrument, Sel.GetChanStart()) ? 0 : 1);
	const int cEnd = Sel.GetChanEnd() - (Sel.IsColumnSelected(column_t::Instrument, Sel.GetChanEnd()) ? 0 : 1);

	do for (int i = cBegin; i <= cEnd; ++i) {
		const auto &Note = b.Get(i);
		if (Note.inst() != MAX_INSTRUMENTS && Note.inst() != HOLD_INSTRUMENT)		// // // 050B
			const_cast<ft0cc::doc::pattern_note &>(Note).set_inst(m_iInstrumentIndex); // TODO: non-const CPatternIterator
	} while (++b <= e);
}



CPActionDragDrop::CPActionDragDrop(CPatternClipData ClipData, bool bDelete, bool bMix, const CSelection &pDragTarget) :
	m_bDragDelete(bDelete), m_bDragMix(bMix)
//	&m_ClipData(pClipData), m_dragTarget(pDragTarget)
{
	m_ClipData		= std::move(ClipData);
	m_dragTarget	= pDragTarget;
	m_iPastePos		= paste_pos_t::DRAG;
}

bool CPActionDragDrop::SaveState(const CMainFrame &MainFrm)
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	if (m_bDragDelete)
		m_AuxiliaryClipData = pPatternEditor->CopyRaw();
	if (!SetTargetSelection(MainFrm, m_newSelection))		// // //
		return false;
	m_UndoClipData = pPatternEditor->CopyRaw();
	return true;
}

void CPActionDragDrop::Undo(CMainFrame &MainFrm)
{
	CPatternEditor *pPatternEditor = GET_PATTERN_EDITOR();
	pPatternEditor->SetSelection(m_newSelection);
	pPatternEditor->PasteRaw(m_UndoClipData);
	if (m_bDragDelete)
		pPatternEditor->PasteRaw(m_AuxiliaryClipData, m_pUndoState->Selection.m_cpStart);
}

void CPActionDragDrop::Redo(CMainFrame &MainFrm)
{
	if (m_bDragDelete)
		DeleteSelection(*GET_SONG_VIEW(), m_pUndoState->Selection);		// // //
//	GET_PATTERN_EDITOR()->Paste(m_ClipData, m_iPasteMode, m_iPastePos);		// // //
	GET_PATTERN_EDITOR()->DragPaste(m_ClipData, m_dragTarget, m_bDragMix);
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
	auto [b, e] = GetIterators(*pSongView);
	const CSelection &Sel = m_pUndoState->Selection;
	CPatternIterator s {b};

	const column_t ColStart = GetSelectColumn(Sel.m_cpStart.Xpos.Column);
	const column_t ColEnd = GetSelectColumn(Sel.m_cpEnd.Xpos.Column);

	int Pos = 0;
	int Offset = 0;
	int oldRow = -1;
	do {
		ft0cc::doc::pattern_note BLANK;
		for (int i = Sel.m_cpStart.Xpos.Track; i <= Sel.m_cpEnd.Xpos.Track; ++i) {
			const auto &Source = (Offset < m_UndoClipData.ClipInfo.Rows && m_iStretchMap[Pos] > 0) ?
				*(m_UndoClipData.GetPattern(i - Sel.m_cpStart.Xpos.Track, Offset)) : BLANK;		// // //
			auto Target = b.Get(i);
			CopyNoteSection(Target, Source,
				i == Sel.m_cpStart.Xpos.Track ? ColStart : column_t::Note,
				i == Sel.m_cpEnd.Xpos.Track ? ColEnd : column_t::Effect4);
			b.Set(i, Target);
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
	} while (++b <= e);
}



CPActionEffColumn::CPActionEffColumn(int Channel, int Count) :		// // //
	m_iChannel(Channel), m_iNewColumns(Count)
{
}

bool CPActionEffColumn::SaveState(const CMainFrame &MainFrm)
{
	if (m_iNewColumns > static_cast<unsigned>(MAX_EFFECT_COLUMNS) || !m_iNewColumns)
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
	songNew_->SetBookmarks(Song.GetBookmarks());
	songNew_->SetRowHighlight(Song.GetRowHighlight());

	GET_SONG_VIEW()->GetChannelOrder().ForeachChannel([&] (stChannelID chan) {
		songNew_->SetEffectColumnCount(chan, Song.GetEffectColumnCount(chan));
		for (int f = 0; f < Frames; ++f) {
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
	MainFrm.GetActiveDocument()->UpdateAllViews(NULL, UPDATE_TRACK);
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
	MainFrm.GetActiveDocument()->UpdateAllViews(NULL, UPDATE_FRAME);
}
