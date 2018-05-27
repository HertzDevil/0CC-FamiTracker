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

#include "SequenceEditor.h"
#include <memory>		// // //
#include "Instrument.h"		// // // inst_type_t
#include "Sequence.h"
#include "../resource.h"		// // // CInstrumentEditDlg
#include "InstrumentEditDlg.h"		// // // GetRefreshRate()
#include "InstrumentEditPanel.h"
#include "SizeEditor.h"
#include "GraphEditor.h"
#include "GraphEditorFactory.h"		// // //
#include "SequenceSetting.h"
#include "SequenceEditorMessage.h"		// // //
#include "DPI.h"		// // //
#include "SequenceParser.h"		// // //
#include "str_conv/str_conv.hpp"		// / ///
#include "Color.h"		// // //

// This file contains the sequence editor and sequence size control

namespace {

constexpr int BOTTOM_MARGIN = 25;		// // //

} // namespace

// CSequenceEditor

IMPLEMENT_DYNAMIC(CSequenceEditor, CWnd)

CSequenceEditor::CSequenceEditor() : CWnd(),		// // //
	m_pSizeEditor(std::make_unique<CSizeEditor>(this)),
	m_pSetting(std::make_unique<CSequenceSetting>(this)),
	m_iMaxVol(15),
	m_iMaxDuty(3),
	m_pParent(NULL),
	m_pSequence(NULL),
	m_iInstrumentType(0)
{
}

CSequenceEditor::~CSequenceEditor()
{
}

BEGIN_MESSAGE_MAP(CSequenceEditor, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()		// // //
	ON_WM_LBUTTONDOWN()
	ON_MESSAGE(WM_SIZE_CHANGE, OnSizeChange)
	ON_MESSAGE(WM_CURSOR_CHANGE, OnCursorChange)
	ON_MESSAGE(WM_SEQUENCE_CHANGED, OnSequenceChanged)
	ON_MESSAGE(WM_SETTING_CHANGED, OnSettingChanged)
END_MESSAGE_MAP()

BOOL CSequenceEditor::CreateEditor(CWnd *pParentWnd, const RECT &rect)
{
	CRect menuRect;

	if (CWnd::CreateEx(WS_EX_STATICEDGE, NULL, L"", WS_CHILD | WS_VISIBLE, rect, pParentWnd, 0) == -1)
		return -1;

	m_cFont.CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH | FF_DONTCARE, L"Tahoma");

	m_pParent = pParentWnd;

	CRect GraphRect;
	GetClientRect(GraphRect);
	GraphRect.bottom -= DPI::SY(BOTTOM_MARGIN);

	CDC *pDC = GetDC();		// // //
	m_BackDC.CreateCompatibleDC(pDC);
	m_Bitmap.CreateCompatibleBitmap(pDC, GraphRect.Width(), DPI::SY(BOTTOM_MARGIN));
	m_BackDC.SelectObject(&m_Bitmap);
	ReleaseDC(pDC);

	if (m_pSizeEditor->CreateEx(NULL, NULL, L"", WS_CHILD | WS_VISIBLE, CRect(34, GraphRect.bottom + 5, 94, GraphRect.bottom + 22), this, 0) == -1)
		return -1;

	menuRect = CRect(GraphRect.right - 72, GraphRect.bottom + 5, GraphRect.right - 2, GraphRect.bottom + 24);

	// Sequence settings editor
	if (m_pSetting->CreateEx(NULL, NULL, L"", WS_CHILD | WS_VISIBLE, menuRect, this, 0) == -1)
		return -1;

	m_pSetting->Setup(&m_cFont);

	return 0;
}

void CSequenceEditor::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect rect;
	GetClientRect(rect);
	rect.top = rect.bottom - DPI::SY(BOTTOM_MARGIN);

	m_BackDC.FillSolidRect(0, 0, rect.Width(), BOTTOM_MARGIN, GREY(240));		// // //

	// Update size editor
	if (m_pSequence)
		m_pSizeEditor->SetValue(m_pSequence->GetItemCount());

	m_BackDC.SelectObject(&m_cFont);
	m_BackDC.SetBkMode(TRANSPARENT);		// // //
	m_BackDC.SetTextColor(BLACK);
	m_BackDC.SetTextAlign(TA_LEFT);
	m_BackDC.TextOutW(7, DPI::SY(6), L"Size:");

	float Rate = static_cast<CInstrumentEditDlg*>(static_cast<CSequenceInstrumentEditPanel*>(m_pParent)->GetParent())->GetRefreshRate();
	m_BackDC.TextOutW(120, DPI::SY(6), FormattedW(L"%.0f ms", (1000.0f * m_pSizeEditor->GetValue()) / Rate));

	if (m_iLastIndex != -1) {		// // //
		CStringW Text = m_pConversion ?
			FormattedW(L"{%i, %s}", m_iLastIndex, conv::to_wide(m_pConversion->ToString(m_iLastValue)).data()) :
			FormattedW(L"{%i, %i}", m_iLastIndex, m_iLastValue);
		m_BackDC.TextOutW(170, DPI::SY(6), Text);
	}

	m_pSizeEditor->Paint(m_BackDC, {34, 5});		// // //
	m_pSetting->Paint(m_BackDC, {rect.Width() - 72, 5});

	dc.BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), &m_BackDC, 0, 0, SRCCOPY);		// // //
}

BOOL CSequenceEditor::OnEraseBkgnd(CDC *pDC) {		// // //
	return FALSE;
}

LRESULT CSequenceEditor::OnSizeChange(WPARAM wParam, LPARAM lParam)
{
	// Number of sequence items has changed
	m_pSequence->SetItemCount(wParam);
	m_pGraphEditor->RedrawWindow();
	RedrawWindow();
	PostMessageW(WM_SEQUENCE_CHANGED, 1);

	return TRUE;
}

LRESULT CSequenceEditor::OnCursorChange(WPARAM wParam, LPARAM lParam)
{
	// Graph cursor has changed
	auto idx = static_cast<int>(wParam);		// // //
	auto val = static_cast<int8_t>(lParam);
	if (m_iLastIndex != idx || m_iLastValue != val) {
		m_iLastIndex = idx;
		m_iLastValue = val;
		RedrawWindow();
	}
	return TRUE;
}

LRESULT CSequenceEditor::OnSequenceChanged(WPARAM wParam, LPARAM lParam)
{
	if (this == NULL)	// TODO: is this needed?
		return FALSE;

	SequenceChangedMessage(wParam == 1);

	return TRUE;
}

LRESULT CSequenceEditor::OnSettingChanged(WPARAM wParam, LPARAM lParam)		// // //
{
	// Called when the setting selector has changed
	SelectSequence(m_pSequence, m_iInstrumentType);
	m_pSetting->RedrawWindow();
	RedrawWindow();

	return TRUE;
}

void CSequenceEditor::SetMaxValues(int MaxVol, int MaxDuty)
{
	m_iMaxVol = MaxVol;
	m_iMaxDuty = MaxDuty;
}

void CSequenceEditor::SetConversion(const CSeqConversionBase &Conv)		// // //
{
	m_pConversion = &Conv;
}

void CSequenceEditor::SequenceChangedMessage(bool Changed)
{
	static_cast<CSequenceInstrumentEditPanel*>(m_pParent)->UpdateSequenceString(Changed);		// // //

	// Set flag in document
	if (Changed)
		if (CFrameWnd *pMainFrame = dynamic_cast<CFrameWnd*>(AfxGetMainWnd()))		// // //
			pMainFrame->GetActiveDocument()->SetModifiedFlag();
}

//const int SEQ_SUNSOFT_NOISE = sequence_t::DutyCycle + 1;

void CSequenceEditor::SelectSequence(std::shared_ptr<CSequence> pSequence, int InstrumentType)		// // //
{
	// Select a sequence to edit
	m_pSequence = std::move(pSequence);
	m_iInstrumentType = InstrumentType;

	DestroyGraphEditor();

	CRect GraphRect;
	GetClientRect(GraphRect);
	GraphRect.bottom -= DPI::SY(BOTTOM_MARGIN);		// // //

	CGraphEditorFactory factory;		// // //
	m_pGraphEditor = factory.Make(this, GraphRect, m_pSequence, InstrumentType, m_iMaxVol, m_iMaxDuty);

	m_pSetting->SelectSequence(m_pSequence, InstrumentType);

	m_pGraphEditor->UpdateWindow();
	m_pGraphEditor->ShowWindow(SW_SHOW);

	m_pSizeEditor->SetValue(m_pSequence->GetItemCount());

	Invalidate();
	RedrawWindow();

	// Update sequence string
	SequenceChangedMessage(false);
}

BOOL CSequenceEditor::DestroyWindow()
{
	DestroyGraphEditor();
	return CWnd::DestroyWindow();
}

void CSequenceEditor::DestroyGraphEditor()
{
	if (m_pGraphEditor) {
		m_pGraphEditor->ShowWindow(SW_HIDE);
		m_pGraphEditor->DestroyWindow();
		m_pGraphEditor.reset();		// // //
	}
}

void CSequenceEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
	CWnd::OnLButtonDown(nFlags, point);
	// Set focus to parent to allow keyboard note preview
	//GetParent()->SetFocus();
	SetFocus();
}
