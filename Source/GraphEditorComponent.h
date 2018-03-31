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

#include "stdafx.h"
#include <cstdint>

class CGraphEditor;
class CSequence;

class CGraphEditorComponent {
public:
	CGraphEditorComponent(CGraphEditor &parent, CRect region);
	virtual ~CGraphEditorComponent() noexcept = default;

	bool ContainsPoint(CPoint point) const;

	void OnMouseHover(CPoint point) { DoOnMouseHover(point); }

	void OnLButtonDown(CPoint point) { DoOnLButtonDown(point); }
	void OnLButtonMove(CPoint point) { DoOnLButtonMove(point); }
	void OnLButtonUp(CPoint point) { DoOnLButtonUp(point); }

	void OnRButtonDown(CPoint point) { DoOnRButtonDown(point); }
	void OnRButtonMove(CPoint point) { DoOnRButtonMove(point); }
	void OnRButtonUp(CPoint point) { DoOnRButtonUp(point); }

	void OnPaint(CDC &dc);

protected:
	void CursorChanged(int idx);

private:
	virtual void DoOnMouseHover(CPoint point) { }

	virtual void DoOnLButtonDown(CPoint point) { }
	virtual void DoOnLButtonMove(CPoint point) { }
	virtual void DoOnLButtonUp(CPoint point) { }

	virtual void DoOnRButtonDown(CPoint point) { }
	virtual void DoOnRButtonMove(CPoint point) { }
	virtual void DoOnRButtonUp(CPoint point) { }

	virtual void DoOnPaint(CDC &dc) { }

protected:
	CGraphEditor &parent_;
	CRect region_;
	CFont font_;
};
