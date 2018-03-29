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

#include "GraphEditorComponent.h"
#include "Color.h"



CGraphEditorComponent::CGraphEditorComponent(CGraphEditor &parent, CRect region) :
	parent_(parent), region_(region)
{
	font_.CreateFontW(-10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"Verdana");		// // //
}

bool CGraphEditorComponent::ContainsPoint(CPoint point) const {
	return region_.PtInRect(point) == TRUE;
}

void CGraphEditorComponent::OnPaint(CDC &dc) {
	DoOnPaint(dc);

#ifdef _DEBUG
	const COLORREF COL_DEBUG_RECT = MakeRGB(255, 0, 0);
	dc.FillSolidRect(region_.left, region_.top, region_.Width(), 1, COL_DEBUG_RECT);
	dc.FillSolidRect(region_.left, region_.top, 1, region_.Height(), COL_DEBUG_RECT);
	dc.FillSolidRect(region_.left, region_.bottom - 1, region_.Width(), 1, COL_DEBUG_RECT);
	dc.FillSolidRect(region_.right - 1, region_.top, 1, region_.Height(), COL_DEBUG_RECT);
#endif
}
