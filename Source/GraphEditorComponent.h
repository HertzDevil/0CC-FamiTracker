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

namespace GraphEditorComponents {

class CGraphBase : public CGraphEditorComponent {
public:
	using CGraphEditorComponent::CGraphEditorComponent;

	virtual int GetItemTop() const = 0;
	int GetItemBottom() const;
	virtual int GetItemHeight() const = 0;
	int GetItemWidth() const;
	int GetItemIndex(CPoint point) const;
	int GetItemGridIndex(CPoint point) const;

	void CursorChanged(CPoint point);

private:
	void ModifyItem(CPoint point, bool redraw);
	virtual int GetItemValue(CPoint point) const = 0;
	virtual int GetMinItemValue() const = 0;
	virtual int GetMaxItemValue() const = 0;
	virtual int GetSequenceItemValue(int idx, int val) const;

	void DoOnMouseHover(CPoint point) override;
	void DoOnLButtonDown(CPoint point) override;
	void DoOnLButtonMove(CPoint point) override;
	void DoOnRButtonDown(CPoint point) override;
	void DoOnRButtonMove(CPoint point) override;
	void DoOnRButtonUp(CPoint point) override;

	template <COLORREF COL_BG1, COLORREF COL_BG2, COLORREF COL_EDGE1, COLORREF COL_EDGE2>
	void DrawRect(CDC &dc, int x, int y, int w, int h, bool flat);

protected:
	void DrawRect(CDC &dc, int x, int y, int w, int h);
	void DrawPlayRect(CDC &dc, int x, int y, int w, int h);
	void DrawCursorRect(CDC &dc, int x, int y, int w, int h);
	void DrawShadowRect(CDC &dc, int x, int y, int w, int h);
	void DrawLine(CDC &dc);

	void DrawBackground(CDC &dc, int Lines, bool DrawMarks, int MarkOffset);
	virtual void DrawRange(CDC &dc, int Max, int Min);

private:
	static constexpr int ITEM_MAX_WIDTH = 40;

protected:
	int m_iHighlightedItem = -1;
	int m_iHighlightedValue = 0;
	CPoint m_ptLineStart = { };
	CPoint m_ptLineEnd = { };

	enum class edit_t { None, Line, Point };
	edit_t m_iEditing = edit_t::None;
};

class CBarGraph : public CGraphBase {
public:
	CBarGraph(CGraphEditor &parent, CRect region, int items);

private:
	int GetItemValue(CPoint point) const override;
	int GetMinItemValue() const override;
	int GetMaxItemValue() const override;

	int GetItemTop() const override;
	int GetItemHeight() const override;

	void DoOnPaint(CDC &dc) override;

	int items_;
};

class CCellGraph : public CGraphBase {
public:
	CCellGraph(CGraphEditor &parent, CRect region, int items);

private:
	int GetItemValue(CPoint point) const override;
	int GetMinItemValue() const override;
	int GetMaxItemValue() const override;
	int GetSequenceItemValue(int Index, int Value) const override;

	int GetItemTop() const override;
	int GetItemHeight() const override;

	void DrawRange(CDC &dc, int Max, int Min) override;

	void DoOnPaint(CDC &dc) override;

	int items_;
};

class CPitchGraph : public CGraphBase {
public:
	using CGraphBase::CGraphBase;

private:
	int GetItemValue(CPoint point) const override;
	int GetMinItemValue() const override;
	int GetMaxItemValue() const override;

	int GetItemTop() const override;
	int GetItemHeight() const override;

	void DoOnPaint(CDC &dc) override;
};

class CNoiseGraph : public CGraphBase {
public:
	CNoiseGraph(CGraphEditor &parent, CRect region, int items);

private:
	int GetItemValue(CPoint point) const override;
	int GetMinItemValue() const override;
	int GetMaxItemValue() const override;
	int GetSequenceItemValue(int Index, int Value) const override;

	int GetItemTop() const override;
	int GetItemHeight() const override;

	void DoOnPaint(CDC &dc) override;

	int items_;
};

class CLoopReleaseBar : public CGraphEditorComponent {
public:
	CLoopReleaseBar(CGraphEditor &parent, CRect region, CGraphBase &graph_parent);

private:
	void DrawTagPoint(CDC &dc, int index, LPCWSTR str, COLORREF col);

	void DoOnLButtonDown(CPoint point) override;
	void DoOnLButtonMove(CPoint point) override;
	void DoOnRButtonDown(CPoint point) override;
	void DoOnRButtonMove(CPoint point) override;
	void DoOnPaint(CDC &dc) override;

	CGraphBase &graph_parent_;
	bool enable_loop_ = true;
};

class CArpSchemeSelector : public CGraphEditorComponent {
};

class CNoiseSelector : public CGraphEditorComponent {
public:
	static constexpr int BUTTON_HEIGHT = 9;

	CNoiseSelector(CGraphEditor &parent, CRect region, CGraphBase &graph_parent);

private:
	void ModifyFlag(int idx, int flag);

	static bool CheckFlag(int8_t val, int flag);

	void DoOnLButtonDown(CPoint point) override;
	void DoOnLButtonMove(CPoint point) override;
	void DoOnPaint(CDC &dc) override;

	CGraphBase &graph_parent_;
	int last_idx_ = -1;
	int last_flag_ = -1;
	bool enable_flag_ = true;
};

} // namespace GraphEditorComponents
