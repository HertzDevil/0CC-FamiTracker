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
	CGraphEditorComponent(CGraphEditor &parent, CRect region) : parent_(parent), region_(region) { }
	virtual ~CGraphEditorComponent() noexcept = default;

	bool ContainsPoint(CPoint point) const;

	void OnLButtonDown(CPoint point) { DoOnLButtonDown(point); }
	void OnLButtonMove(CPoint point) { DoOnLButtonMove(point); }
	void OnRButtonDown(CPoint point) { DoOnRButtonDown(point); }
	void OnRButtonMove(CPoint point) { DoOnRButtonMove(point); }
	void OnPaint(CDC &dc) { DoOnPaint(dc); }

private:
	virtual void DoOnLButtonDown(CPoint point) { }
	virtual void DoOnLButtonMove(CPoint point) { }
	virtual void DoOnRButtonDown(CPoint point) { }
	virtual void DoOnRButtonMove(CPoint point) { }
	virtual void DoOnPaint(CDC &dc) { }

protected:
	CGraphEditor &parent_;
	CRect region_;
};

namespace GraphEditorComponents {

class CBarGraph : public CGraphEditorComponent {
};

class CCellGraph : public CGraphEditorComponent {
};

class CPitchGraph : public CGraphEditorComponent {
};

class CLoopReleaseBar : public CGraphEditorComponent {
public:
	CLoopReleaseBar(CGraphEditor &parent, CRect region);

private:
	void DrawTagPoint(CDC &dc, int index, LPCWSTR str, COLORREF col);

	void DoOnLButtonDown(CPoint point) override;
	void DoOnLButtonMove(CPoint point) override;
	void DoOnRButtonDown(CPoint point) override;
	void DoOnRButtonMove(CPoint point) override;
	void DoOnPaint(CDC &dc) override;

	CFont font_;
	bool enable_loop_ = true;
};

class CArpSchemeSelector : public CGraphEditorComponent {
};

class CNoiseSelector : public CGraphEditorComponent {
public:
	static constexpr int BUTTON_HEIGHT = 9;

	using CGraphEditorComponent::CGraphEditorComponent;

private:
	void ModifyFlag(int idx, int flag);

	static bool CheckFlag(int8_t val, int flag);

	void DoOnLButtonDown(CPoint point) override;
	void DoOnLButtonMove(CPoint point) override;
	void DoOnPaint(CDC &dc) override;

	bool enable_flag_ = true;
	int last_idx_ = -1;
	int last_flag_ = -1;
};

} // namespace GraphEditorComponents
