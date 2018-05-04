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


#pragma once

#include "Action.h"
#include "FrameEditorTypes.h"		// // //
#include "IntRange.h"		// // //
#include <memory>		// // //
#include "FrameClipData.h"		// // //

class CFrameEditor;		// // //
class CFamiTrackerView;
class CMainFrame;

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
		\param View Reference to the tracker view. */
	explicit CFrameEditorState(const CFamiTrackerView &View);

	/*!	\brief Applies the state to a frame editor.
		\param View Reference to the tracker view. */
	void ApplyState(CFamiTrackerView &View) const;

	/*!	\brief Obtains the first selected frame.
		\return Starting frame index. */
	int GetFrameStart() const;
	/*!	\brief Obtains the last selected frame.
		\return Ending channel index. */
	int GetFrameEnd() const;
	/*!	\brief Obtains the first selected channel.
		\return Starting frame index. */
	int GetChanStart() const;
	/*!	\brief Obtains the last selected channel.
		\return Ending channel index. */
	int GetChanEnd() const;

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
	CFrameAction() = default;		// // //

	void SaveUndoState(const CMainFrame &MainFrm) override;		// // //
	void SaveRedoState(const CMainFrame &MainFrm) override;		// // //
	void RestoreUndoState(CMainFrame &MainFrm) const override;		// // //
	void RestoreRedoState(CMainFrame &MainFrm) const override;		// // //
	void UpdateViews(CMainFrame &MainFrm) const override;		// // //

protected:
	static int ClipPattern(int Pattern);

	CIntRange<int> m_itFrames, m_itChannels;

protected:
	std::unique_ptr<CFrameEditorState> m_pUndoState, m_pRedoState;		// // //
};

// // // built-in frame action subtypes

class CFActionAddFrame : public CFrameAction
{
public:
	CFActionAddFrame() = default;
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
};

class CFActionRemoveFrame : public CFrameAction
{
public:
	CFActionRemoveFrame() = default;
	~CFActionRemoveFrame();
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
private:
	CFrameClipData m_RowClipData;
};

class CFActionDuplicateFrame : public CFrameAction
{
public:
	CFActionDuplicateFrame() = default;
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
};

class CFActionCloneFrame : public CFrameAction
{
public:
	CFActionCloneFrame() = default;
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
};

class CFActionFrameCount : public CFrameAction
{
public:
	CFActionFrameCount(int Count);
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	bool Merge(const CAction &Other) override;		// // //
	void UpdateViews(CMainFrame &MainFrm) const override;
private:
	int m_iOldFrameCount, m_iNewFrameCount;
};

class CFActionSetPattern : public CFrameAction
{
public:
	CFActionSetPattern(int Pattern);
	~CFActionSetPattern();
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	bool Merge(const CAction &Other) override;		// // //
private:
	int m_iNewPattern;
	CFrameClipData m_ClipData;
};

class CFActionSetPatternAll : public CFrameAction
{
public:
	CFActionSetPatternAll(int Pattern);
	~CFActionSetPatternAll();
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	bool Merge(const CAction &Other) override;		// // //
private:
	int m_iNewPattern;
	CFrameClipData m_RowClipData;
};

class CFActionChangePattern : public CFrameAction
{
public:
	CFActionChangePattern(int Offset);
	~CFActionChangePattern();
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	bool Merge(const CAction &Other) override;		// // //
private:
	int m_iPatternOffset;
	CFrameClipData m_ClipData;
	mutable bool m_bOverflow = false;
};

class CFActionChangePatternAll : public CFrameAction
{
public:
	CFActionChangePatternAll(int Offset);
	~CFActionChangePatternAll();
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	bool Merge(const CAction &Other) override;		// // //
private:
	int m_iPatternOffset;
	CFrameClipData m_RowClipData;
	mutable bool m_bOverflow = false;
};

class CFActionMoveDown : public CFrameAction
{
public:
	CFActionMoveDown() = default;
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
};

class CFActionMoveUp : public CFrameAction
{
public:
	CFActionMoveUp() = default;
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
};

class CFActionClonePatterns : public CFrameAction		// // //
{
public:
	CFActionClonePatterns() = default;
	~CFActionClonePatterns();
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
private:
	int m_iOldPattern, m_iNewPattern;
	CFrameClipData m_ClipData;
};

class CFActionPaste : public CFrameAction
{
public:
	CFActionPaste(CFrameClipData Data, int Frame, bool Clone);
	~CFActionPaste();
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	void UpdateViews(CMainFrame &MainFrm) const override;
private:
	CFrameClipData m_ClipData;
	int m_iTargetFrame;
	bool m_bClone;
};

class CFActionPasteOverwrite : public CFrameAction
{
public:
	CFActionPasteOverwrite(CFrameClipData Data);
	~CFActionPasteOverwrite();
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
private:
	CFrameClipData m_ClipData, m_OldClipData;
	CFrameSelection m_TargetSelection;
};

class CFActionDropMove : public CFrameAction
{
public:
	CFActionDropMove(CFrameClipData Data, int Frame);
	~CFActionDropMove();
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
private:
	CFrameClipData m_ClipData;
	int m_iTargetFrame;
};

class CFActionDeleteSel : public CFrameAction
{
public:
	CFActionDeleteSel() = default;
	~CFActionDeleteSel();
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	void UpdateViews(CMainFrame &MainFrm) const override;
private:
	CFrameClipData m_ClipData;
};

class CFActionMergeDuplicated : public CFrameAction
{
public:
	CFActionMergeDuplicated() = default;
	~CFActionMergeDuplicated();
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
private:
	CFrameClipData m_ClipData, m_OldClipData;
};
