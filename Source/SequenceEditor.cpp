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
#include <string>
#include "Instrument.h"		// // // inst_type_t
#include "Sequence.h"
#include "../resource.h"		// // // CInstrumentEditDlg
#include "InstrumentEditDlg.h"		// // // GetRefreshRate()
#include "InstrumentEditPanel.h"
#include "SizeEditor.h"
#include "GraphEditor.h"
#include "SequenceSetting.h"
#include "SequenceEditorMessage.h"		// // //
#include "DPI.h"		// // //
#include "SequenceParser.h"		// // //
#include "str_conv/str_conv.hpp"		// / ///

// This file contains the sequence editor and sequence size control

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

	m_cFont.CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Tahoma");

	m_pParent = pParentWnd;

	CRect GraphRect;
	GetClientRect(GraphRect);
	GraphRect.bottom -= 25;

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

	if (this == GetFocus()) {
		CRect focusRect = rect;
		focusRect.DeflateRect(rect.Height() - 1, 2, rect.Height() + 1, 2);
		dc.DrawFocusRect(focusRect);
	}

	// Update size editor
	if (m_pSequence)
		m_pSizeEditor->SetValue(m_pSequence->GetItemCount());

	dc.SelectObject(&m_cFont);
	dc.TextOutW(7, rect.bottom - 19, L"Size:");

	CStringW LengthStr;
	float Rate;		// // //
	Rate = static_cast<CInstrumentEditDlg*>(static_cast<CSequenceInstrumentEditPanel*>(m_pParent)->GetParent())->GetRefreshRate();
	LengthStr.Format(L"%.0f ms  ", (1000.0f * m_pSizeEditor->GetValue()) / Rate);

	dc.TextOutW(120, rect.bottom - 19, LengthStr);
}

LRESULT CSequenceEditor::OnSizeChange(WPARAM wParam, LPARAM lParam)
{
	// Number of sequence items has changed
	m_pSequence->SetItemCount(wParam);
	m_pGraphEditor->RedrawWindow();
	RedrawWindow();
	PostMessage(WM_SEQUENCE_CHANGED, 1);

	return TRUE;
}

LRESULT CSequenceEditor::OnCursorChange(WPARAM wParam, LPARAM lParam)
{
	// Graph cursor has changed
	CDC *pDC = GetDC();
	pDC->SelectObject(&m_cFont);

	CRect rect;
	GetClientRect(rect);

	CStringW Text;
	if (m_pConversion != nullptr)		// // //
		Text.Format(L"{%i, %s}        ", wParam, conv::to_wide(m_pConversion->ToString(static_cast<char>(lParam))).data());
	else
		Text.Format(L"{%i, %i}        ", wParam, lParam);
	pDC->TextOutW(170, rect.bottom - 19, Text);

	ReleaseDC(pDC);
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

	switch (m_pSequence->GetSequenceType()) {
	case sequence_t::Volume:		// // //
		if (m_iInstrumentType == INST_VRC6) {
			ASSERT(dynamic_cast<CBarGraphEditor*>(m_pGraphEditor.get()));
			static_cast<CBarGraphEditor*>(m_pGraphEditor.get())->SetMaxItems(
				m_pSequence->GetSetting() == SETTING_VOL_64_STEPS ? 0x3F : 0x0F);
		}
		break;
	case sequence_t::Arpeggio:
		ASSERT(dynamic_cast<CArpeggioGraphEditor*>(m_pGraphEditor.get()));
		static_cast<CArpeggioGraphEditor*>(m_pGraphEditor.get())->ChangeSetting();
		break;
	}

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

	// Create the graph
	switch (m_pSequence->GetSequenceType()) {
	case sequence_t::Volume:
		if (m_iInstrumentType == INST_VRC6 && m_pSequence->GetSetting() == SETTING_VOL_64_STEPS)
			m_pGraphEditor = std::make_unique<CBarGraphEditor>(m_pSequence, 0x3F);		// // //
		else
			m_pGraphEditor = std::make_unique<CBarGraphEditor>(m_pSequence, m_iMaxVol);
		break;
	case sequence_t::Arpeggio:
		m_pGraphEditor = std::make_unique<CArpeggioGraphEditor>(m_pSequence);
		break;
	case sequence_t::Pitch:
	case sequence_t::HiPitch:
		m_pGraphEditor = std::make_unique<CPitchGraphEditor>(m_pSequence);
		break;
	case sequence_t::DutyCycle:
		if (InstrumentType == INST_S5B)
			m_pGraphEditor = std::make_unique<CNoiseEditor>(m_pSequence, 31);
		else
			m_pGraphEditor = std::make_unique<CBarGraphEditor>(m_pSequence, m_iMaxDuty);
		break;
	}

	m_pSetting->SelectSequence(m_pSequence, InstrumentType);

	CRect GraphRect;
	GetClientRect(GraphRect);
	GraphRect.bottom -= 25;

	if (m_pGraphEditor->CreateEx(NULL, NULL, L"", WS_CHILD, GraphRect, this, 0) == -1)
		return;

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
