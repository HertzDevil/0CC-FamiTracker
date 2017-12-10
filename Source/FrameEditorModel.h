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

	int GetCurrentFrame() const;
	int GetCurrentChannel() const;
	CFrameCursorPos GetCurrentPos() const;
	void SetCurrentFrame(int frame);
	void SetCurrentChannel(int channel);

	bool IsSelecting() const;
	const CFrameSelection *GetSelection() const;
	CFrameSelection MakeFrameSelection(int frame) const;
	CFrameSelection MakeFullSelection(int track) const;
	CFrameSelection GetActiveSelection() const;

	void Select(const CFrameSelection &sel);
	void Deselect();
	void StartSelection(int frame, int channel);
	void ContinueSelection(int frame, int channel);
	void ContinueFrameSelection(int frame);

	bool IsFrameSelected(int frame) const;
	bool IsChannelSelected(int channel) const;

	std::unique_ptr<CFrameClipData> CopySelection(const CFrameSelection &sel, int track) const;

private:
	bool m_bSelecting = false;
	CFrameSelection m_selection;		// // //

	CFamiTrackerDoc *doc_ = nullptr;
	CFamiTrackerView *view_ = nullptr;
};
