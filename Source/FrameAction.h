/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
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

#include "Action.h"
#include "FrameEditorTypes.h"		// // //

class CFrameEditor;		// // //

/*
	\brief A structure responsible for recording the cursor and selection state of the frame
	editor for use by frame actions. Uses CFamiTrackerView since the frame editor class is not
	independent.
*/
struct CFrameEditorState		// TODO maybe merge this with CPatternEditorState
{
	/*!	\brief Constructor of the frame editor state.
		\details On construction, the object retrieves the current state of the frame editor
		immediately. Once created, a state object remains constant and can be applied back to the
		frame editor as many times as desired.
		\param pView Pointer to the tracker view.
		\param Track The track number. */
	CFrameEditorState(const CFamiTrackerView *pEditor, int Track);

	/*!	\brief Applies the state to a frame editor.
		\param pView Pointer to the tracker view. */
	void ApplyState(CFamiTrackerView *pView) const;

	/*!	\brief The current track number at the time of the state's creation. */
	int Track;

	/*!	\brief The current cursor position at the time of the state's creation. */
	CFrameCursorPos Cursor;

	/*!	\brief The current selection position at the time of the state's creation. */
	CFrameSelection Selection;

	/*!	\brief Whether a selection is active at the time of the state's creation. */
	bool IsSelecting;

private:
	CFrameSelection OriginalSelection;
};

// Frame commands
class CFrameAction : public CAction
{
protected:
	CFrameAction();		// // //

public:
	virtual ~CFrameAction();

	void SaveUndoState(const CMainFrame *pMainFrm);		// // //
	void SaveRedoState(const CMainFrame *pMainFrm);		// // //
	void RestoreUndoState(CMainFrame *pMainFrm) const;		// // //
	void RestoreRedoState(CMainFrame *pMainFrm) const;		// // //

protected:
	static int ClipPattern(int Pattern);

protected:
	CFrameEditorState *m_pUndoState = nullptr, *m_pRedoState = nullptr;		// // //
};

// // // built-in frame action subtypes

class CFActionAddFrame : public CFrameAction
{
public:
	CFActionAddFrame() { }
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
};

class CFActionRemoveFrame : public CFrameAction
{
public:
	CFActionRemoveFrame() { }
	~CFActionRemoveFrame() { SAFE_RELEASE(m_pRowClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
private:
	CFrameClipData *m_pRowClipData = nullptr;
};

class CFActionDuplicateFrame : public CFrameAction
{
public:
	CFActionDuplicateFrame() { }
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
};

class CFActionCloneFrame : public CFrameAction
{
public:
	CFActionCloneFrame() { }
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
};

class CFActionFrameCount : public CFrameAction
{
public:
	CFActionFrameCount(int Count) : m_iNewFrameCount(Count) { }
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
	bool Merge(const CAction *Other);		// // //
private:
	int m_iOldFrameCount, m_iNewFrameCount;
};

class CFActionSetPattern : public CFrameAction
{
public:
	CFActionSetPattern(int Pattern) : m_iNewPattern(Pattern) { }
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
	bool Merge(const CAction *Other);		// // //
private:
	int m_iOldPattern, m_iNewPattern;
};

class CFActionSetPatternAll : public CFrameAction
{
public:
	CFActionSetPatternAll(int Pattern) : m_iNewPattern(Pattern) { }
	~CFActionSetPatternAll() { SAFE_RELEASE(m_pRowClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
private:
	int m_iNewPattern;
	CFrameClipData *m_pRowClipData = nullptr;
};

class CFActionChangePattern : public CFrameAction
{
public:
	CFActionChangePattern(int Offset) : m_iPatternOffset(Offset) { }
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
	bool Merge(const CAction *Other);		// // //
private:
	int m_iOldPattern;
	int m_iPatternOffset;
};

class CFActionChangePatternAll : public CFrameAction
{
public:
	CFActionChangePatternAll(int Offset) : m_iPatternOffset(Offset) { }
	~CFActionChangePatternAll() { SAFE_RELEASE(m_pRowClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
private:
	int m_iPatternOffset;
	CFrameClipData *m_pRowClipData = nullptr;
};

class CFActionMoveDown : public CFrameAction
{
public:
	CFActionMoveDown() { }
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
};

class CFActionMoveUp : public CFrameAction
{
public:
	CFActionMoveUp() { }
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
};

class CFActionClonePatterns : public CFrameAction		// // //
{
public:
	CFActionClonePatterns() { }
	~CFActionClonePatterns() { SAFE_RELEASE(m_pClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
private:
	int m_iOldPattern, m_iNewPattern;
	CFrameClipData *m_pClipData = nullptr;
};

class CFActionPaste : public CFrameAction
{
public:
	CFActionPaste(CFrameClipData *pData, int Frame, bool Clone) : m_pClipData(pData), m_iTargetFrame(Frame), m_bClone(Clone) { }
	~CFActionPaste() { SAFE_RELEASE(m_pClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
private:
	CFrameClipData *m_pClipData = nullptr;
	int m_iTargetFrame;
	bool m_bClone;
};

class CFActionPasteOverwrite : public CFrameAction
{
public:
	CFActionPasteOverwrite(CFrameClipData *pData) : m_pClipData(pData) { }
	~CFActionPasteOverwrite() { SAFE_RELEASE(m_pClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
private:
	CFrameClipData *m_pClipData = nullptr, *m_pOldClipData = nullptr;
	CFrameSelection m_TargetSelection;
};

class CFActionDropMove : public CFrameAction
{
public:
	CFActionDropMove(CFrameClipData *pData, int Frame) : m_pClipData(pData), m_iTargetFrame(Frame) { }
	~CFActionDropMove() { SAFE_RELEASE(m_pClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
private:
	CFrameClipData *m_pClipData = nullptr;
	int m_iTargetFrame;
};

class CFActionDeleteSel : public CFrameAction
{
public:
	CFActionDeleteSel() { }
	~CFActionDeleteSel() { SAFE_RELEASE(m_pClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
private:
	CFrameClipData *m_pClipData = nullptr;
};

class CFActionMergeDuplicated : public CFrameAction
{
public:
	CFActionMergeDuplicated() { }
	~CFActionMergeDuplicated() { SAFE_RELEASE(m_pOldClipData); SAFE_RELEASE(m_pClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
private:
	CFrameClipData *m_pClipData = nullptr, *m_pOldClipData = nullptr;
};
