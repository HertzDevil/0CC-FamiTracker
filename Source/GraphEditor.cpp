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

#include "GraphEditor.h"
#include "Sequence.h"		// // //
#include "FamiTrackerEnv.h"		// // //
#include "SoundGen.h"
#include "SequenceEditorMessage.h"		// // //
#include "DPI.h"		// // //
#include "Color.h"		// // //
#include "GraphEditorComponent.h"		// // //

// CGraphEditor

// The graphical sequence editor

IMPLEMENT_DYNAMIC(CGraphEditor, CWnd)

BEGIN_MESSAGE_MAP(CGraphEditor, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

CGraphEditor::CGraphEditor(std::shared_ptr<CSequence> pSequence) :		// // //
	m_pSequence(std::move(pSequence)),
	m_iLastPlayPos(0)
{
}

CGraphEditor::~CGraphEditor() noexcept {		// // //
}

std::shared_ptr<CSequence> CGraphEditor::GetSequence() {		// // //
	return m_pSequence && m_pSequence->GetItemCount() ? m_pSequence : nullptr;
}

std::shared_ptr<const CSequence> CGraphEditor::GetSequence() const {		// // //
	return m_pSequence && m_pSequence->GetItemCount() ? m_pSequence : nullptr;
}

BOOL CGraphEditor::OnEraseBkgnd(CDC* DC)
{
	return FALSE;
}

BOOL CGraphEditor::CreateEx(DWORD dwExStyle, LPCWSTR lpszClassName, LPCWSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, LPVOID lpParam)
{
	if (CWnd::CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, lpParam) == -1)
		return -1;

	// Calculate the draw areas
	GetClientRect(m_ClientRect);

	m_GraphRect = m_ClientRect;
	m_GraphRect.left += DPI::SX(GRAPH_LEFT);		// // //
	m_GraphRect.top += DPI::SY(5);
	m_GraphRect.bottom -= DPI::SY(16);

	m_pParentWnd = pParentWnd;

	RedrawWindow();

	CDC *pDC = GetDC();

	m_BackDC.CreateCompatibleDC(pDC);
	m_Bitmap.CreateCompatibleBitmap(pDC, m_ClientRect.Width(), m_ClientRect.Height());
	m_BackDC.SelectObject(&m_Bitmap);

	ReleaseDC(pDC);

	Initialize();
	CreateComponents();		// // //

	CWnd::SetTimer(0, 30, NULL);

	return 0;
}

void CGraphEditor::Initialize()
{
	// Allow extra initialization
}

void CGraphEditor::AddGraphComponent(std::unique_ptr<CGraphEditorComponent> pCom) {		// // //
	components_.push_back(std::move(pCom));
}

void CGraphEditor::OnTimer(UINT nIDEvent)
{
	if (m_pSequence) {
		int Pos = Env.GetSoundGenerator()->GetSequencePlayPos(m_pSequence);
		//int Pos = m_pSequence->GetPlayPos();
		if (Pos != m_iLastPlayPos) {
			m_iCurrentPlayPos = Pos;
			RedrawWindow();
		}
		m_iLastPlayPos = Pos;
	}
}

BOOL CGraphEditor::PreTranslateMessage(MSG *pMsg) {		// // //
	// disable double-clicking
	switch (pMsg->message) {
	case WM_LBUTTONDBLCLK:
		pMsg->message = WM_LBUTTONDOWN;
		break;
	case WM_MBUTTONDBLCLK:
		pMsg->message = WM_MBUTTONDOWN;
		break;
	case WM_RBUTTONDBLCLK:
		pMsg->message = WM_RBUTTONDOWN;
		break;
	}

	return CWnd::PreTranslateMessage(pMsg);
}

void CGraphEditor::OnPaint() {
	CPaintDC dc {this};
	for (auto &pCom : components_)		// // //
		pCom->OnPaint(m_BackDC);

#ifdef _DEBUG		// // //
	m_BackDC.FillSolidRect(m_ClientRect.left, m_ClientRect.top, m_ClientRect.Width(), 1, MakeRGB(255, 255, 0));
	m_BackDC.FillSolidRect(m_ClientRect.left, m_ClientRect.top, 1, m_ClientRect.Height(), MakeRGB(255, 255, 0));
	m_BackDC.FillSolidRect(m_ClientRect.left, m_ClientRect.bottom - 1, m_ClientRect.Width(), 1, MakeRGB(255, 255, 0));
	m_BackDC.FillSolidRect(m_ClientRect.right - 1, m_ClientRect.top, 1, m_ClientRect.Height(), MakeRGB(255, 255, 0));

	m_BackDC.FillSolidRect(m_GraphRect.left, m_GraphRect.top, m_GraphRect.Width(), 1, MakeRGB(0, 255, 0));
	m_BackDC.FillSolidRect(m_GraphRect.left, m_GraphRect.top, 1, m_GraphRect.Height(), MakeRGB(0, 255, 0));
	m_BackDC.FillSolidRect(m_GraphRect.left, m_GraphRect.bottom - 1, m_GraphRect.Width(), 1, MakeRGB(0, 255, 0));
	m_BackDC.FillSolidRect(m_GraphRect.right - 1, m_GraphRect.top, 1, m_GraphRect.Height(), MakeRGB(0, 255, 0));
#endif

	if (this == GetFocus()) {
		CRect focusRect = m_ClientRect;
		m_BackDC.SetBkColor(0);
		m_BackDC.DrawFocusRect(focusRect);
	}

	dc.BitBlt(0, 0, m_ClientRect.Width(), m_ClientRect.Height(), &m_BackDC, 0, 0, SRCCOPY);
}

int CGraphEditor::GetItemWidth() const
{
	unsigned Count = GetItemCount();		// // //
	return Count ? std::min(m_GraphRect.Width() / static_cast<int>(Count), ITEM_MAX_WIDTH) : 0;		// // //
}

int CGraphEditor::GetCurrentPlayPos() const {		// // //
	return m_iCurrentPlayPos;
}

CRect CGraphEditor::GetClientArea() const {		// // //
	return m_ClientRect;
}

int CGraphEditor::GetItemIndex(CPoint point) const {		// // //
	return GetItemCount() && point.x >= GRAPH_LEFT ? (point.x - GRAPH_LEFT) / GetItemWidth() : -1;
}

int CGraphEditor::GetItemGridIndex(CPoint point) const {		// // //
	int ItemWidth = GetItemWidth();
	int idx = point.x - GRAPH_LEFT + ItemWidth / 2;
	return idx >= 0 ? idx / ItemWidth : -1;
}

int CGraphEditor::GetItemCount() const {		// // //
	return m_pSequence ? static_cast<int>(m_pSequence->GetItemCount()) : 0;
}

void CGraphEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
	SetFocus();

	if (!GetItemCount())
		return;

	for (auto &pCom : components_)
		if (pCom->ContainsPoint(point)) {
			focused_ = pCom.get();		// // //
			pCom->OnLButtonDown(point);
			break;
		}

	// Notify parent
	CursorChanged(point);

	CWnd::OnLButtonDown(nFlags, point);
}

void CGraphEditor::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	Invalidate();

	if (focused_) {		// // //
		focused_->OnLButtonUp(point);
		focused_ = nullptr;
	}

	RedrawWindow();
	CWnd::OnLButtonUp(nFlags, point);
}

void CGraphEditor::OnRButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
	SetFocus();

	if (!GetItemCount())
		return;

	for (auto &pCom : components_)
		if (pCom->ContainsPoint(point)) {
			focused_ = pCom.get();		// // //
			pCom->OnRButtonDown(point);
			break;
		}

	// Notify parent
	CursorChanged(point);

	CWnd::OnRButtonDown(nFlags, point);
}

void CGraphEditor::OnRButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	Invalidate();

	if (focused_) {		// // //
		focused_->OnRButtonUp(point);
		focused_ = nullptr;
	}

	RedrawWindow();
	CWnd::OnRButtonUp(nFlags, point);
}

void CGraphEditor::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!GetItemCount())
		return;

	if (nFlags & MK_LBUTTON) {
		if (focused_)
			focused_->OnLButtonMove(point);
	}
	else if (nFlags & MK_RBUTTON) {
		if (focused_)
			focused_->OnRButtonMove(point);
	}
	else // No button
		for (auto &pCom : components_)
			pCom->OnMouseHover(point);

	// Notify parent
	CursorChanged(point);

	CWnd::OnMouseMove(nFlags, point);
}

void CGraphEditor::ItemModified() {		// // //
	RedrawWindow(NULL);
	m_pParentWnd->PostMessageW(WM_SEQUENCE_CHANGED, 1);
}

void CGraphEditor::CursorChanged(CPoint point)
{
	if (int Pos = GetItemIndex(point); Pos >= 0 && Pos < GetItemCount())
		m_pParentWnd->PostMessageW(WM_CURSOR_CHANGE, Pos, m_pSequence->GetItem(Pos));
	else
		m_pParentWnd->PostMessageW(WM_CURSOR_CHANGE, 0, 0);
}

void CGraphEditor::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);
	RedrawWindow();
}

void CGraphEditor::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);
	RedrawWindow();
}

// Bar graph editor (volume and duty setting)

void CBarGraphEditor::CreateComponents() {		// // //
	AddGraphComponent(std::make_unique<GraphEditorComponents::CBarGraph>(*this, m_GraphRect, m_iLevels));

	CRect bottomRect = m_ClientRect;		// // //
	bottomRect.left += GRAPH_LEFT;
	bottomRect.top = bottomRect.bottom - DPI::SY(16);
	AddGraphComponent(std::make_unique<GraphEditorComponents::CLoopReleaseBar>(*this, bottomRect));
}

// Pitch graph editor

void CPitchGraphEditor::CreateComponents() {		// // //
	AddGraphComponent(std::make_unique<GraphEditorComponents::CPitchGraph>(*this, m_GraphRect));

	CRect bottomRect = m_ClientRect;		// // //
	bottomRect.left += GRAPH_LEFT;
	bottomRect.top = bottomRect.bottom - DPI::SY(16);
	AddGraphComponent(std::make_unique<GraphEditorComponents::CLoopReleaseBar>(*this, bottomRect));
}

// Sunsoft noise editor

void CNoiseEditor::CreateComponents() {		// // //
	CRect flagsRect = m_ClientRect;		// // //
	flagsRect.left += GRAPH_LEFT;
	flagsRect.bottom -= DPI::SY(16);
	flagsRect.top = flagsRect.bottom - GraphEditorComponents::CNoiseSelector::BUTTON_HEIGHT * 3;
	AddGraphComponent(std::make_unique<GraphEditorComponents::CNoiseSelector>(*this, flagsRect));

	AddGraphComponent(std::make_unique<GraphEditorComponents::CNoiseGraph>(*this, m_GraphRect, m_iItems));

	CRect bottomRect = m_ClientRect;		// // //
	bottomRect.left += GRAPH_LEFT;
	bottomRect.top = bottomRect.bottom - DPI::SY(16);
	AddGraphComponent(std::make_unique<GraphEditorComponents::CLoopReleaseBar>(*this, bottomRect));
}

// Arpeggio graph editor

IMPLEMENT_DYNAMIC(CArpeggioGraphEditor, CGraphEditor)

BEGIN_MESSAGE_MAP(CArpeggioGraphEditor, CGraphEditor)
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

int CArpeggioGraphEditor::GetGraphScrollOffset() const {		// // //
	return m_iScrollOffset;
}

void CArpeggioGraphEditor::Initialize()
{
	// Setup scrollbar
	const int SCROLLBAR_WIDTH = ::GetSystemMetrics(SM_CXHSCROLL);		// // //

	m_GraphRect.right -= SCROLLBAR_WIDTH;
	CRect scrollBarRect {m_GraphRect.right, m_ClientRect.top, m_GraphRect.right + SCROLLBAR_WIDTH, m_ClientRect.bottom};		// // //
	m_cScrollBar.Create(SBS_VERT | SBS_LEFTALIGN | WS_CHILD | WS_VISIBLE, scrollBarRect, this, 0);

	SCROLLINFO info = MakeScrollInfo();		// // //
	m_iScrollMax = info.nPos;

	if (m_pSequence && m_pSequence->GetItemCount() > 0) {
		m_iScrollOffset = m_pSequence->GetItem(0);
		if (m_pSequence->GetSetting() == SETTING_ARP_SCHEME) {		// // //
			m_iScrollOffset &= 0x3F;
			if (m_iScrollOffset > ARPSCHEME_MAX)
				m_iScrollOffset -= 0x40;
		}

		info.nPos = std::clamp(m_iScrollMax - m_iScrollOffset, info.nMin, info.nMax);		// // //
	}
	else
		m_iScrollOffset = 0;

	m_cScrollBar.SetScrollInfo(&info);
	m_cScrollBar.ShowScrollBar(TRUE);
	m_cScrollBar.EnableScrollBar(ESB_ENABLE_BOTH);

	m_ClientRect.right -= SCROLLBAR_WIDTH;
}

void CArpeggioGraphEditor::CreateComponents() {
	AddGraphComponent(std::make_unique<GraphEditorComponents::CCellGraph>(*this, m_GraphRect, ITEMS));

	CRect bottomRect = m_ClientRect;		// // //
	bottomRect.left += GRAPH_LEFT;
	bottomRect.top = bottomRect.bottom - DPI::SY(16);
	AddGraphComponent(std::make_unique<GraphEditorComponents::CLoopReleaseBar>(*this, bottomRect));
}

SCROLLINFO CArpeggioGraphEditor::MakeScrollInfo() const {		// // //
	SCROLLINFO info = {sizeof(SCROLLINFO)};
	info.fMask = SIF_ALL;

	switch (m_pSequence->GetSetting()) {		// // //
	case SETTING_ARP_FIXED:
		info.nMax = 84;
		info.nMin = 0;
		info.nPage = 1;
		info.nPos = 84;
		break;
	case SETTING_ARP_SCHEME:
		info.nMax = 72;
		info.nMin = 0;
		info.nPage = 1;
		info.nPos = ARPSCHEME_MAX;
		break;
	default:
		info.nMax = 192;
		info.nMin = 0;
		info.nPage = 10;
		info.nPos = 96;
	}

	return info;
}

void CArpeggioGraphEditor::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	switch (nSBCode) {
		case SB_LINEDOWN:
			--m_iScrollOffset;
			break;
		case SB_LINEUP:
			++m_iScrollOffset;
			break;
		case SB_PAGEDOWN:
			m_iScrollOffset -= ITEMS / 2;
			break;
		case SB_PAGEUP:
			m_iScrollOffset += ITEMS / 2;
			break;
		case SB_TOP:
			m_iScrollOffset = m_iScrollMax;
			break;
		case SB_BOTTOM:
			m_iScrollOffset = (m_pSequence->GetSetting() != SETTING_ARP_FIXED) ? -m_iScrollMax : 0;
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			m_iScrollOffset = m_iScrollMax - (int)nPos;
			break;
		case SB_ENDSCROLL:
		default:
			return;
	}

	if (m_iScrollOffset > m_iScrollMax)
		m_iScrollOffset = m_iScrollMax;

	if (m_pSequence->GetSetting() != SETTING_ARP_FIXED) {
		if (m_iScrollOffset < -m_iScrollMax)
			m_iScrollOffset = -m_iScrollMax;
	}
	else {
		if (m_iScrollOffset < 0)
			m_iScrollOffset = 0;
	}

	pScrollBar->SetScrollPos(m_iScrollMax - m_iScrollOffset);

	RedrawWindow();

	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

BOOL CArpeggioGraphEditor::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (zDelta < 0)
		OnVScroll(SB_LINEDOWN, 0, &m_cScrollBar);
	else
		OnVScroll(SB_LINEUP, 0, &m_cScrollBar);
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}
