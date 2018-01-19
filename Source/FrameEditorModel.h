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

#include "FrameEditorTypes.h"
#include <memory>

class CFrameEditor;
class CFamiTrackerDoc;
class CFamiTrackerView;
class CFrameClipData;

class CFrameEditorModel {
public:
	void AssignDocument(CFamiTrackerDoc &doc, CFamiTrackerView &view);

	void DoClick(int frame, int channel);
	void DoMove(int frame, int channel);
	void DoUnclick(int frame, int channel);

	int GetCurrentFrame() const;
	int GetCurrentChannel() const;
	CFrameCursorPos GetCurrentPos() const;
	void SetCurrentFrame(int frame);
	void SetCurrentChannel(int channel);

	bool IsSelecting() const;
	const CFrameSelection *GetSelection() const;
	CFrameSelection MakeFrameSelection(int frame) const;
	CFrameSelection MakeFullSelection() const;
	CFrameSelection GetActiveSelection() const;

	void Select(const CFrameSelection &sel);
	void Deselect();
	void StartSelection(const CFrameCursorPos &pos);
	void ContinueSelection(const CFrameCursorPos &pos);
	void ContinueFrameSelection(int frame);

	bool IsFrameSelected(int frame) const;
	bool IsChannelSelected(int channel) const;

	std::unique_ptr<CFrameClipData> CopySelection(const CFrameSelection &sel) const;
	void PasteSelection(const CFrameClipData &clipdata, const CFrameCursorPos &pos);

private:
	CFamiTrackerDoc *doc_ = nullptr;
	CFamiTrackerView *view_ = nullptr;

	CFrameSelection m_selection;
	CFrameCursorPos selStart_;
	CFrameCursorPos selEnd_;

	bool m_bSelecting = false;
	bool clicking_ = false;
};
