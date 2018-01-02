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

#include "FrameEditor.h"
#include <algorithm>
#include <cmath>
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "FrameAction.h"
#include "CompoundAction.h"		// // //
#include "Accelerator.h"
#include "PatternEditor.h"
#include "SoundGen.h"
#include "Settings.h"
#include "Graphics.h"
#include "Color.h"		// // //
#include "Clipboard.h"
#include "Bookmark.h"		// // //
#include "BookmarkCollection.h"		// // //
#include "DPI.h"		// // //
#include "FrameClipData.h"		// // //
#include "NumConv.h"		// // //
#include "FrameEditorModel.h"		// // //
#include "SongData.h"		// // //

/*
 * CFrameEditor
 * This is the frame(order) editor to the left in the control panel
 *
 */

const TCHAR CFrameEditor::CLIPBOARD_ID[] = _T("FamiTracker Frames");

IMPLEMENT_DYNAMIC(CFrameEditor, CWnd)

CFrameEditor::CFrameEditor(CMainFrame *pMainFrm) :
	m_pMainFrame(pMainFrm),
	m_pDocument(NULL),
	m_pView(NULL),
	model_(std::make_unique<CFrameEditorModel>()),		// // //
	m_iClipboard(0),
	m_iWinWidth(0),
	m_iWinHeight(0),
	m_iFirstChannel(0),
	m_iCursorEditDigit(0),
	m_iRowsVisible(0),
	m_iMiddleRow(0),
	m_bInputEnable(false),
	m_bCursor(false),
	m_bInvalidated(false),
	m_iLastCursorFrame(0),
	m_iLastCursorChannel(0),
	m_iLastPlayFrame(0),
	m_hAccel(0),
	m_bStartDrag(false),
	m_bDropped(false),
	m_iDragRow(0),
	m_bLastRow(false),		// // //
	m_iTopScrollArea(0),
	m_iBottomScrollArea(0),
	m_DropTarget(this),
	m_iDragThresholdX(0),
	m_iDragThresholdY(0),
	m_iUpdates(0),
	m_iPaints(0)
{
}

CFrameEditor::~CFrameEditor()
{
}

BEGIN_MESSAGE_MAP(CFrameEditor, CWnd)
	ON_WM_PAINT()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
	ON_WM_NCMOUSEMOVE()
	ON_WM_CONTEXTMENU()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_KEYDOWN()
	ON_WM_TIMER()
	ON_COMMAND(ID_FRAME_CUT, OnEditCut)
	ON_COMMAND(ID_FRAME_COPY, OnEditCopy)
	ON_COMMAND(ID_FRAME_PASTE, OnEditPaste)
	ON_COMMAND(ID_FRAME_PASTENEWPATTERNS, OnEditPasteNewPatterns)
	ON_COMMAND(ID_FRAME_DELETE, OnEditDelete)
	ON_COMMAND(ID_MODULE_INSERTFRAME, OnModuleInsertFrame)
	ON_COMMAND(ID_MODULE_REMOVEFRAME, OnModuleRemoveFrame)
	ON_COMMAND(ID_MODULE_DUPLICATEFRAME, OnModuleDuplicateFrame)
	ON_COMMAND(ID_MODULE_DUPLICATEFRAMEPATTERNS, OnModuleDuplicateFramePatterns)
	ON_COMMAND(ID_MODULE_MOVEFRAMEDOWN, OnModuleMoveFrameDown)
	ON_COMMAND(ID_MODULE_MOVEFRAMEUP, OnModuleMoveFrameUp)
	// // //
	ON_COMMAND(ID_FRAME_PASTEOVERWRITE, OnEditPasteOverwrite)
//	ON_UPDATE_COMMAND_UI(ID_FRAME_PASTEOVERWRITE, OnUpdateEditPasteOverwrite)
	ON_COMMAND(ID_MODULE_DUPLICATECURRENTPATTERN, OnModuleDuplicateCurrentPattern)
END_MESSAGE_MAP()


void CFrameEditor::AssignDocument(CFamiTrackerDoc *pDoc, CFamiTrackerView *pView)
{
	m_pDocument = pDoc;
	m_pView		= pView;
	model_->AssignDocument(*pDoc, *pView);		// // //
}

// CFrameEditor message handlers

int CFrameEditor::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_hAccel = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_FRAMEWND));

	m_Font.CreateFont(DPI::SY(14), 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
					  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
					  theApp.GetSettings()->Appearance.strFrameFont);		// // // 050B

	m_iClipboard = ::RegisterClipboardFormat(CLIPBOARD_ID);

	m_DropTarget.Register(this);
	m_DropTarget.SetClipBoardFormat(m_iClipboard);

	m_iDragThresholdX = ::GetSystemMetrics(SM_CXDRAG);
	m_iDragThresholdY = ::GetSystemMetrics(SM_CYDRAG);

	return 0;
}

void CFrameEditor::OnPaint()
{
	CPaintDC dc(this); // device context for painting

//#define BENCHMARK

	// Do not call CWnd::OnPaint() for painting messages
	const CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	// Get window size
	CRect WinRect;
	GetClientRect(&WinRect);

	if (!CFamiTrackerDoc::GetDoc()->IsFileLoaded()) {
		dc.FillSolidRect(WinRect, 0);
		return;
	}
	else if (pSoundGen == NULL)
		return;
	else if (pSoundGen->IsBackgroundTask())
		return;

	// Check if width has changed, delete objects then
	if (m_bmpBack.m_hObject != NULL) {
		CSize size = m_bmpBack.GetBitmapDimension();
		if (size.cx != m_iWinWidth) {
			m_bmpBack.DeleteObject();
			m_dcBack.DeleteDC();
		}
	}

	// Allocate object
	if (m_dcBack.m_hDC == NULL) {
		m_bmpBack.CreateCompatibleBitmap(&dc, m_iWinWidth, m_iWinHeight);
		m_bmpBack.SetBitmapDimension(m_iWinWidth, m_iWinHeight);
		m_dcBack.CreateCompatibleDC(&dc);
		m_dcBack.SelectObject(&m_bmpBack);
		InvalidateFrameData();
	}

	if (NeedUpdate()) {
		DrawFrameEditor(&m_dcBack);
		++m_iUpdates;
	}

	m_bInvalidated = false;

	dc.BitBlt(0, 0, m_iWinWidth, m_iWinHeight, &m_dcBack, 0, 0, SRCCOPY);

#ifdef BENCHMARK
	++m_iPaints;
	CString txt;
	txt.Format("Updates %i", m_iUpdates);
	dc.TextOut(1, 1, txt);
	txt.Format("Paints %i", m_iPaints);
	dc.TextOut(1, 18, txt);
#endif
}

void CFrameEditor::DrawFrameEditor(CDC *pDC)
{
	// Draw the frame editor window

	const COLORREF QUEUE_COLOR	  = 0x108010;	// Colour of row in play queue
	const COLORREF RED_BAR_COLOR  = 0x4030A0;
	const COLORREF BLUE_BAR_COLOR = 0xA02030;

	// Cache settings
	const COLORREF ColBackground	= theApp.GetSettings()->Appearance.iColBackground;
	const COLORREF ColText			= theApp.GetSettings()->Appearance.iColPatternText;
	const COLORREF ColTextHilite	= theApp.GetSettings()->Appearance.iColPatternTextHilite;
	const COLORREF ColCursor		= theApp.GetSettings()->Appearance.iColCursor;
	const COLORREF ColSelect		= theApp.GetSettings()->Appearance.iColSelection;
	const COLORREF ColDragCursor	= INTENSITY(ColBackground) > 0x80 ? BLACK : WHITE;
	const COLORREF ColSelectEdge	= BLEND(ColSelect, WHITE, .7);
	const COLORREF ColTextDimmed	= DIM(theApp.GetSettings()->Appearance.iColPatternText, .9);

	const CFamiTrackerDoc *pDoc = m_pDocument;		// // //
	const CFamiTrackerView *pView = m_pView;
	const auto &song = m_pMainFrame->GetCurrentSong();		// // //

	const int FrameCount	= song.GetFrameCount();
	const int ChannelCount	= pDoc->GetChannelCount();
	int ActiveFrame			= GetEditFrame();		// // //
	int ActiveChannel		= pView->GetSelectedChannel();

	m_iFirstChannel = std::max(ActiveChannel - (ChannelCount - 1), std::min(ActiveChannel, m_iFirstChannel));

	if (m_bLastRow)		// // //
		++ActiveFrame;
	ActiveFrame = std::max(0, std::min(FrameCount, ActiveFrame));

	CFont *pOldFont = m_dcBack.SelectObject(&m_Font);

	m_dcBack.SetBkMode(TRANSPARENT);
	m_dcBack.SetTextAlign(TA_CENTER);		// // //

	// Draw background
	m_dcBack.FillSolidRect(0, 0, m_iWinWidth, m_iWinHeight, ColBackground);

	// Selected row
	COLORREF RowColor = BLEND(ColCursor, ColBackground, .5);

	if (GetFocus() == this) {
		if (m_bInputEnable)
			RowColor = BLEND(RED_BAR_COLOR, 0, .8);
		else
			RowColor = BLEND(BLUE_BAR_COLOR, ColBackground, .8);
	}

	// Draw selected row
	GradientBar(m_dcBack, DPI::Rect(0, m_iMiddleRow * ROW_HEIGHT + 3, m_iWinWidth, ROW_HEIGHT + 1), RowColor, ColBackground);		// // //

	const int FirstVisibleFrame = ActiveFrame - m_iMiddleRow;
	const int BeginFrame = std::max(0, FirstVisibleFrame);		// // //
	const int EndFrame = std::min(FrameCount, m_iRowsVisible + FirstVisibleFrame);		// // //

	// Draw rows

	// // // Highlight by bookmarks
	const CBookmarkCollection &Col = song.GetBookmarks();		// // //
	for (int Frame = BeginFrame; Frame <= EndFrame; ++Frame) {
		int line = Frame - FirstVisibleFrame;
		const int ypos = line * ROW_HEIGHT;
		CRect RowRect = DPI::Rect(0, ypos + 4, m_iWinWidth, ROW_HEIGHT - 1);		// // //

		if (line != m_iMiddleRow)
			for (unsigned j = 0, Count = Col.GetCount(); j < Count; ++j)
				if (Col.GetBookmark(j)->m_iFrame == Frame) {
					GradientBar(m_dcBack, RowRect, theApp.GetSettings()->Appearance.iColBackgroundHilite, ColBackground);		// // //
					break;
				}
	}

	// // // 050B row marker
	if (int Marker = pView->GetMarkerFrame(); Marker >= BeginFrame && Marker <= EndFrame) {
		int line = Marker - FirstVisibleFrame;
		const int ypos = line * ROW_HEIGHT;
		GradientBar(m_dcBack, DPI::Rect(2, ypos + 4, ROW_COLUMN_WIDTH - 5, ROW_HEIGHT - 1), ColCursor, DIM(ColCursor, .3));		// // //
	}

	// Play cursor
	const int PlayFrame = theApp.GetSoundGenerator()->GetPlayerPos().first;		// // //
	if (!pView->GetFollowMode() && theApp.IsPlaying())
		if (PlayFrame >= BeginFrame && PlayFrame <= EndFrame) {
			int line = PlayFrame - FirstVisibleFrame;
			const int ypos = line * ROW_HEIGHT;
			CRect RowRect = DPI::Rect(0, ypos + 4, m_iWinWidth, ROW_HEIGHT - 1);		// // //
			GradientBar(m_dcBack, RowRect, theApp.GetSettings()->Appearance.iColCurrentRowPlaying, ColBackground);		// // //
		}

	// Queue cursor
	if (int Queue = theApp.GetSoundGenerator()->GetQueueFrame(); Queue >= BeginFrame && Queue <= EndFrame) {
		int line = Queue - FirstVisibleFrame;
		const int ypos = line * ROW_HEIGHT;
		CRect RowRect = DPI::Rect(0, ypos + 4, m_iWinWidth, ROW_HEIGHT - 1);		// // //
		GradientBar(m_dcBack, RowRect, QUEUE_COLOR, ColBackground);
	}

	// Selection
	if (auto pSel = model_->GetSelection()) {		// // //
		const int SelectStart	= pSel->GstFirstSelectedFrame();
		const int SelectEnd		= pSel->GetLastSelectedFrame();
		const int CBegin		= pSel->GetFirstSelectedChannel();
		const int CEnd			= pSel->GetLastSelectedChannel();
		for (int Frame = std::max(SelectStart, BeginFrame); Frame <= std::min(SelectEnd, EndFrame); ++Frame) {
			int line = Frame - FirstVisibleFrame;
			const int ypos = line * ROW_HEIGHT;
			CRect RowRect = DPI::Rect(ROW_COLUMN_WIDTH + FRAME_ITEM_WIDTH * CBegin, ypos + 3,
				FRAME_ITEM_WIDTH * (CEnd - CBegin + 1), ROW_HEIGHT);		// // //
			RowRect.OffsetRect(2, 0);
			RowRect.InflateRect(1, 0);
			m_dcBack.FillSolidRect(RowRect, ColSelect);
			if (Frame == SelectStart)
				m_dcBack.FillSolidRect(RowRect.left, RowRect.top, RowRect.Width(), 1, ColSelectEdge);
			if (Frame == SelectEnd)
				m_dcBack.FillSolidRect(RowRect.left, RowRect.bottom - 1, RowRect.Width(), 1, ColSelectEdge);
			m_dcBack.FillSolidRect(RowRect.left, RowRect.top, 1, RowRect.Height(), ColSelectEdge);		// // //
			m_dcBack.FillSolidRect(RowRect.right - 1, RowRect.top, 1, RowRect.Height(), ColSelectEdge);		// // //
		}
	}

	// Cursor box
	if (int Frame = m_iMiddleRow + FirstVisibleFrame; Frame >= BeginFrame && Frame <= EndFrame) {
		const int ypos = m_iMiddleRow * ROW_HEIGHT;
		int x = ((ActiveChannel - m_iFirstChannel) * FRAME_ITEM_WIDTH);
		int y = m_iMiddleRow * ROW_HEIGHT + 3;
		CRect CursorRect = DPI::Rect(ROW_COLUMN_WIDTH + 2 + x, y, FRAME_ITEM_WIDTH, ROW_HEIGHT + 1);

		GradientBar(m_dcBack, CursorRect, ColCursor, ColBackground);		// // //
		m_dcBack.Draw3dRect(CursorRect, BLEND(ColCursor, WHITE, .9), BLEND(ColCursor, ColBackground, .6));

		// Flashing black box indicating that input is active
		if (m_bInputEnable && m_bCursor)
			m_dcBack.FillSolidRect(DPI::Rect(ROW_COLUMN_WIDTH + 4 + x + CURSOR_WIDTH * m_iCursorEditDigit, y + 2, CURSOR_WIDTH, ROW_HEIGHT - 3), ColBackground);
	}

	// // // Extra input frame
	if (FrameCount <= EndFrame) {
		int line = FrameCount - FirstVisibleFrame;
		const int ypos = line * ROW_HEIGHT;
		m_dcBack.SetTextColor(ColTextHilite);
		m_dcBack.TextOut(DPI::SX(3 + FRAME_ITEM_WIDTH / 2), DPI::SY(ypos + 3), _T(">>"));

		COLORREF CurrentColor = IsLineDimmed(line) ? ColTextDimmed : ColText;		// // //

		for (int j = 0; j < ChannelCount; ++j) {
			int Chan = j + m_iFirstChannel;

			//m_dcBack.SetTextColor(CurrentColor);
			m_dcBack.SetTextColor(DIM(CurrentColor, .7));

			m_dcBack.TextOut(DPI::SX(28 + j * FRAME_ITEM_WIDTH + FRAME_ITEM_WIDTH / 2), DPI::SY(ypos + 3), _T("--"));
		}
	}

	// Frame patterns
	const bool bHexRows = theApp.GetSettings()->General.bRowInHex;
	for (int Frame = BeginFrame; Frame <= EndFrame; ++Frame) {
		if (Frame != FrameCount) {
			int line = Frame - FirstVisibleFrame;
			const int ypos = line * ROW_HEIGHT;
			bool bSelectedRow = model_->IsFrameSelected(Frame);		// // //
			m_dcBack.SetTextColor(ColTextHilite);
			m_dcBack.TextOut(DPI::SX(3 + FRAME_ITEM_WIDTH / 2), DPI::SY(ypos + 3),
				(bHexRows ? conv::sv_from_uint_hex(Frame, 2) : conv::sv_from_uint(Frame, 3)).data());

			COLORREF CurrentColor = IsLineDimmed(line) ? ColTextDimmed : ColText;		// // //

			for (int j = 0; j < ChannelCount; ++j) {
				int Chan = j + m_iFirstChannel;

				unsigned index = song.GetFramePattern(Frame, Chan);		// // //
				unsigned activeIndex = song.GetFramePattern(ActiveFrame, Chan);

				// Dim patterns that are different from current
				if ((!m_bLastRow && index == activeIndex) || bSelectedRow)
					m_dcBack.SetTextColor(CurrentColor);
				else
					m_dcBack.SetTextColor(DIM(CurrentColor, .7));

				m_dcBack.TextOut(DPI::SX(28 + j * FRAME_ITEM_WIDTH + FRAME_ITEM_WIDTH / 2), DPI::SY(ypos + 3),
					conv::sv_from_uint_hex(index, 2).data());		// // //
			}
		}
	}

	// Draw cursor when dragging
	if (m_DropTarget.IsDragging()) {
		auto pSel = model_->GetSelection();		// // //
		if (!pSel || (m_iDragRow <= pSel->GstFirstSelectedFrame() || m_iDragRow >= (pSel->GetLastSelectedFrame() + 1))) {
			if (m_iDragRow >= FirstVisibleFrame && m_iDragRow <= FirstVisibleFrame + m_iRowsVisible) {
				const int CBegin = pSel->GetFirstSelectedChannel();
				const int CEnd = pSel->GetLastSelectedChannel();
				int x = ROW_COLUMN_WIDTH + CBegin * FRAME_ITEM_WIDTH + 2;		// // //
				int y = m_iDragRow - FirstVisibleFrame;
				m_dcBack.FillSolidRect(DPI::Rect(x, y * ROW_HEIGHT + 3, FRAME_ITEM_WIDTH * (CEnd - CBegin + 1) + 1, 2), ColDragCursor);
			}
		}
	}

	COLORREF colSeparator = BLEND(ColBackground, Invert(ColBackground), .75);

	// Row number separator
	m_dcBack.FillSolidRect(DPI::SX(ROW_COLUMN_WIDTH - 1), 0, DPI::SY(1), m_iWinHeight, colSeparator);

	// Save draw info
	m_iLastCursorFrame = ActiveFrame;
	m_iLastCursorChannel = ActiveChannel;
	m_iLastPlayFrame = PlayFrame;

	// Restore old objects
	m_dcBack.SelectObject(pOldFont);

	// Update scrollbars
	if (FrameCount == 1)
		SetScrollRange(SB_VERT, 0, 1);
	else
		SetScrollRange(SB_VERT, 0, FrameCount - 1);

	SetScrollPos(SB_VERT, ActiveFrame);
	SetScrollRange(SB_HORZ, 0, ChannelCount - 1);
	SetScrollPos(SB_HORZ, ActiveChannel);
}

void CFrameEditor::UpdateHighlightLine(int line) {		// // //
	if (m_iHighlightLine != line) {
		m_iHighlightLine = line;
		InvalidateFrameData();
	}
}

bool CFrameEditor::IsLineDimmed(int line) const {
	return m_iHighlightLine != -1 && line != m_iHighlightLine;
}

void CFrameEditor::SetupColors()
{
	// Color scheme has changed
	InvalidateFrameData();
}

void CFrameEditor::DrawScreen(CDC *pDC) {		// // //
	if (NeedUpdate())
		RedrawWindow();
}

void CFrameEditor::RedrawFrameEditor()
{
	// Public method for redrawing this editor

}

bool CFrameEditor::NeedUpdate() const
{
	// Checks if the cursor or frame data has changed and area needs to be redrawn

	const CFamiTrackerView *pView = CFamiTrackerView::GetView();
	const CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	const int ActiveFrame	= GetEditFrame();		// // //
	const int ActiveChannel = pView->GetSelectedChannel();
	const int PlayFrame	    = theApp.GetSoundGenerator()->GetPlayerPos().first;		// // //

	if (m_iLastCursorFrame != ActiveFrame)
		return true;

	if (m_iLastCursorChannel != ActiveChannel)
		return true;

	if (m_iLastPlayFrame != PlayFrame)
		return true;

	return m_bInvalidated;
}

void CFrameEditor::InvalidateFrameData()
{
	// Frame data has changed, must update
	m_bInvalidated = true;
	CWnd::Invalidate();		// // //
}

void CFrameEditor::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	switch (nSBCode) {
	case SB_ENDSCROLL:
		return;
	case SB_LINEDOWN:
	case SB_PAGEDOWN:
		m_pView->SelectNextFrame();
		break;
	case SB_PAGEUP:
	case SB_LINEUP:
		m_pView->SelectPrevFrame();
		break;
	case SB_TOP:
		m_pView->SelectFrame(0);		// // //
		break;
	case SB_BOTTOM:
		m_pView->SelectFrame(m_pDocument->GetFrameCount(m_pMainFrame->GetSelectedTrack()) - 1);
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		if (m_pDocument->GetFrameCount(m_pMainFrame->GetSelectedTrack()) > 1)
			SetEditFrame(nPos);		// // //
		break;
	}

	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CFrameEditor::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	switch (nSBCode) {
	case SB_ENDSCROLL:
		return;
	case SB_LINERIGHT:
	case SB_PAGERIGHT:
		m_pView->MoveCursorNextChannel();
		break;
	case SB_PAGELEFT:
	case SB_LINELEFT:
		m_pView->MoveCursorPrevChannel();
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		m_pView->SelectChannel(nPos);
		break;
	}

	CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CFrameEditor::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);

	// Install frame editor's accelerator
	theApp.GetAccelerator()->SetAccelerator(m_hAccel);

	InvalidateFrameData();
}

void CFrameEditor::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);
	m_bInputEnable = false;
//	CancelSelection();		// // //
	m_bLastRow = false;		// // //
	InvalidateFrameData();
	theApp.GetAccelerator()->SetAccelerator(NULL);
}

void CFrameEditor::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	const int PAGE_SIZE = 4;

	if (m_bInputEnable) {
		// Keyboard input is active
		int Track = m_pMainFrame->GetSelectedTrack();
		bool bShift = (::GetKeyState(VK_SHIFT) & 0x80) == 0x80;

		const int MaxChannel = m_pDocument->GetChannelCount() - 1;		// // //
		const int FrameCount = m_pDocument->GetFrameCount(Track);
		const int Channel = model_->GetCurrentChannel();
		const int Frame = GetEditFrame();		// // //
		const int MaxFrame = FrameCount - (IsSelecting() ? 1 : 0);

		switch (nChar) {
		case VK_RETURN:
			m_pView->SetFocus();
			break;
		case VK_INSERT:
			OnModuleInsertFrame();
			break;
		case VK_DELETE:
			OnModuleRemoveFrame();
			break;
		case VK_UP:
		case VK_DOWN:
		case VK_LEFT:
		case VK_RIGHT:
		case VK_NEXT:
		case VK_PRIOR:
		case VK_HOME:
		case VK_END:
			if (bShift && !m_bLastRow) {		// // //
				if (!model_->IsSelecting())
					model_->StartSelection(CFrameCursorPos {GetEditFrame(), Channel});
			}
			else
				CancelSelection();

			switch (nChar) {
			case VK_LEFT:
				m_pView->SelectChannel(Channel ? Channel - 1 : MaxChannel);
				break;
			case VK_RIGHT:
				m_pView->SelectChannel(Channel == MaxChannel ? 0 : Channel + 1);
				break;
			case VK_UP:
				SetEditFrame(Frame ? Frame - 1 : MaxFrame);		// // //
				break;
			case VK_DOWN:
				SetEditFrame(Frame == MaxFrame ? 0 : Frame + 1);		// // //
				break;
			case VK_NEXT:
				SetEditFrame(std::min(MaxFrame, Frame + PAGE_SIZE));		// // //
				break;
			case VK_PRIOR:
				SetEditFrame(std::max(0, Frame - PAGE_SIZE));		// // //
				break;
			case VK_HOME:
				SetEditFrame(0);		// // //
				break;
			case VK_END:
				SetEditFrame(MaxFrame);		// // //
				break;
			}

			m_iCursorEditDigit = 0;
			if (bShift && !m_bLastRow) {
				model_->ContinueSelection(CFrameCursorPos {GetEditFrame(), Channel});		// // //
				InvalidateFrameData();
			}
			break;
		default:
			int Num = -1;
			if (nChar >= '0' && nChar <= '9')
				Num = nChar - '0';
			else if (nChar >= VK_NUMPAD0 && nChar <= VK_NUMPAD9)
				Num = nChar - VK_NUMPAD0;
			else if (nChar >= 'A' && nChar <= 'F')
				Num = nChar - 'A' + 10;
			if (Num == -1)
				break;

			if (IsSelecting()) {		// // //
				int Pattern;
				if (m_iCursorEditDigit == 0) {
					Pattern = Num << 4;
					m_iCursorEditDigit = 1;
				}
				else if (m_iCursorEditDigit == 1) {
					Pattern = m_pDocument->GetPatternAtFrame(Track, GetSelection().GstFirstSelectedFrame(), GetSelection().GetFirstSelectedChannel()) | Num;
					m_iCursorEditDigit = 0;
				}
				m_pMainFrame->AddAction(std::make_unique<CFActionSetPattern>(Pattern));
			}
			else if (!m_bLastRow || FrameCount < MAX_FRAMES) {		// // //
				int Pattern = m_bLastRow ? 0 : m_pDocument->GetPatternAtFrame(Track, Frame, Channel);		// // //
				if (m_iCursorEditDigit == 0)
					Pattern = (Pattern & 0x0F) | (Num << 4);
				else if (m_iCursorEditDigit == 1)
					Pattern = (Pattern & 0xF0) | Num;
				Pattern = std::min(Pattern, MAX_PATTERN - 1);

				if (m_bLastRow) // TODO: use CCompoundAction
					m_pMainFrame->AddAction(std::make_unique<CFActionFrameCount>(static_cast<int>(FrameCount) + 1));
				if (m_pMainFrame->ChangeAllPatterns())
					m_pMainFrame->AddAction(std::make_unique<CFActionSetPatternAll>(Pattern));
				else
					m_pMainFrame->AddAction(std::make_unique<CFActionSetPattern>(Pattern));

				const int SelectedChannel = (model_->GetCurrentChannel() + 1) % m_pDocument->GetAvailableChannels();		// // //
				const int SelectedFrame = model_->GetCurrentFrame();

				if (m_iCursorEditDigit == 1) {
					m_iCursorEditDigit = 0;
					m_bCursor = true;
					m_pView->SelectChannel(SelectedChannel);		// // //
				}
				else
					m_iCursorEditDigit = 1;
			}
		}
	}

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CFrameEditor::OnTimer(UINT_PTR nIDEvent)
{
	if (m_bInputEnable) {
		m_bCursor = !m_bCursor;
		InvalidateFrameData();
	}

	CWnd::OnTimer(nIDEvent);
}

BOOL CFrameEditor::PreTranslateMessage(MSG* pMsg)
{
	// Temporary fix, accelerated messages must be sent to the main window
	if (theApp.GetAccelerator()->Translate(theApp.m_pMainWnd->m_hWnd, pMsg)) {
		return TRUE;
	}

	if (pMsg->message == WM_KEYDOWN) {
		OnKeyDown(pMsg->wParam, pMsg->lParam & 0xFFFF, pMsg->lParam & 0xFF0000);
		// Remove the beep
		pMsg->message = WM_NULL;
	}

	return CWnd::PreTranslateMessage(pMsg);
}

int CFrameEditor::GetRowFromPoint(const CPoint &point, bool DropTarget) const
{
	// Translate a point value to a row
	int Delta = (point.y - DPI::SY(TOP_OFFSET)) / DPI::SY(ROW_HEIGHT);		// // //
	int NewFrame = GetEditFrame() - m_iMiddleRow + Delta;
	int MaxFrame = m_pDocument->GetFrameCount(m_pMainFrame->GetSelectedTrack()) - (DropTarget ? 0 : 1);

	return std::max(0, std::min(MaxFrame, NewFrame));
}

int CFrameEditor::GetChannelFromPoint(const CPoint &point) const
{
	// Translate a point value to a channel
	if (IsOverFrameColumn(point))
		return -1;
	int Offs = point.x - DPI::SX(ROW_COLUMN_WIDTH) - 2;		// // //
	return std::min(m_pDocument->GetChannelCount() - 1, Offs / DPI::SX(FRAME_ITEM_WIDTH));
}

bool CFrameEditor::IsOverFrameColumn(const CPoint &point) const		// // //
{
	return point.x < DPI::SX(ROW_COLUMN_WIDTH) + 2;
}

CFrameCursorPos CFrameEditor::TranslateFramePos(const CPoint &point, bool DropTarget) const {		// // //
	return {GetRowFromPoint(point, DropTarget), GetChannelFromPoint(point)};
}

unsigned int CFrameEditor::CalcWidth(int Channels) const
{
	return ROW_COLUMN_WIDTH + FRAME_ITEM_WIDTH * Channels + 25;
}

//// Mouse ////////////////////////////////////////////////////////////////////////////////////////

void CFrameEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_ButtonPoint = point;
	m_LastClickPos = TranslateFramePos(point, false);		// // //

	if (auto pSel = model_->GetSelection()) {
		if (model_->IsFrameSelected(m_LastClickPos.m_iFrame) && model_->IsChannelSelected(m_LastClickPos.m_iChannel))		// // //
			m_bStartDrag = true;
		else if (nFlags & MK_SHIFT) {
			model_->ContinueSelection(m_LastClickPos);		// // //
			InvalidateFrameData();
		}
		else
			model_->Deselect();
	}
	else if (nFlags & MK_SHIFT) {
		model_->StartSelection(model_->GetCurrentPos());
		model_->ContinueSelection(m_LastClickPos);
		m_bFullFrameSelect = false;		// // //
	}

	CWnd::OnLButtonDown(nFlags, point);
}

void CFrameEditor::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (IsSelecting()) {
		if (m_bStartDrag) {
			model_->Deselect();
			m_bStartDrag = false;
			m_pView->SetFocus();
		}
		InvalidateFrameData();
	}
	else {
		CFrameCursorPos pos = TranslateFramePos(point, false);		// // //
		if ((nFlags & MK_CONTROL) && theApp.IsPlaying()) {
			// Queue this frame
			if (pos.m_iFrame == theApp.GetSoundGenerator()->GetQueueFrame())
				// Remove
				theApp.GetSoundGenerator()->SetQueueFrame(-1);
			else
				// Set new
				theApp.GetSoundGenerator()->SetQueueFrame(pos.m_iFrame);

			InvalidateFrameData();
		}
		else {
			// Switch to frame
			SetEditFrame(GetRowFromPoint(point, GetFocus() == this));		// // // allow one-past-the-end
			if (pos.m_iChannel >= 0)
				m_pView->SelectChannel(std::min(pos.m_iChannel, m_pDocument->GetChannelCount() - 1));		// // //
		}
	}

	CWnd::OnLButtonUp(nFlags, point);
}

void CFrameEditor::OnMouseMove(UINT nFlags, CPoint point)
{
	if (nFlags & MK_LBUTTON) {
		bool mouseMoved = std::abs(m_ButtonPoint.x - point.x) > m_iDragThresholdX ||
			std::abs(m_ButtonPoint.y - point.y) > m_iDragThresholdY;

		if (!IsSelecting()) {
			if (mouseMoved) {
				m_bFullFrameSelect = m_LastClickPos.m_iChannel < 0;		// // //
				model_->StartSelection(m_LastClickPos);
				EnableInput();		// // //
			}
		}

		if (m_bStartDrag) {
			if (mouseMoved)
				InitiateDrag();
		}
		else if (IsSelecting()) {
			CFrameCursorPos pos = TranslateFramePos(point, false);		// // //
			AutoScroll(point);
			if (m_bFullFrameSelect)		// // //
				model_->ContinueFrameSelection(pos.m_iFrame);
			else
				model_->ContinueSelection({pos.m_iFrame, std::max(0, pos.m_iChannel)});
			InvalidateFrameData();
		}
	}

	// Highlight
	UpdateHighlightLine((point.y - DPI::SY(TOP_OFFSET)) / DPI::SY(ROW_HEIGHT));		// // //
	CWnd::OnMouseMove(nFlags, point);
}

void CFrameEditor::OnNcMouseMove(UINT nHitTest, CPoint point)
{
	UpdateHighlightLine(-1);		// // //
	CWnd::OnNcMouseMove(nHitTest, point);
}

void CFrameEditor::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// Select channel and enable edit mode

	const int Channel  = GetChannelFromPoint(point);
	const int NewFrame = GetRowFromPoint(point, true);		// // // allow one-past-the-end

	SetEditFrame(NewFrame);		// // //

	if (Channel >= 0)
		m_pView->SelectChannel(Channel);

	if (m_bInputEnable)
		m_pView->SetFocus();
	else
		EnableInput();

	CWnd::OnLButtonDblClk(nFlags, point);
}

BOOL CFrameEditor::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (zDelta > 0) {
		// Up
		m_pView->SelectPrevFrame();
	}
	else {
		// Down
		m_pView->SelectNextFrame();
	}

	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void CFrameEditor::AutoScroll(const CPoint &point)
{
	const int Row = GetRowFromPoint(point, true);		// // //
	const int Rows = m_pDocument->GetFrameCount(m_pMainFrame->GetSelectedTrack()) + 1;

	if (point.y <= m_iTopScrollArea && Row > 0) {
		m_pView->SelectPrevFrame();
	}
	else if (point.y >= m_iBottomScrollArea && Row < (Rows - 1)) {
		m_pView->SelectNextFrame();
	}
}

void CFrameEditor::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	// Popup menu
	CMenu *pPopupMenu, PopupMenuBar;
	PopupMenuBar.LoadMenu(IDR_FRAME_POPUP);
	m_pMainFrame->UpdateMenu(&PopupMenuBar);
	pPopupMenu = PopupMenuBar.GetSubMenu(0);

	// Paste menu item
	BOOL ClipboardAvailable = IsClipboardAvailable();
	pPopupMenu->EnableMenuItem(ID_FRAME_PASTE, MF_BYCOMMAND | (ClipboardAvailable ? MF_ENABLED : MF_DISABLED));
	pPopupMenu->EnableMenuItem(ID_FRAME_PASTENEWPATTERNS, MF_BYCOMMAND | (ClipboardAvailable ? MF_ENABLED : MF_DISABLED));

	pPopupMenu->TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, this);
}

void CFrameEditor::EnableInput()
{
	// Set focus and enable input, input is disabled when focus is lost
	SetFocus();

	m_bInputEnable = true;
	m_bCursor = true;
	m_iCursorEditDigit = 0;

	SetTimer(0, 500, NULL);	// Cursor timer

	InvalidateFrameData();
}

void CFrameEditor::OnEditCut()
{
	OnEditCopy();
	OnEditDelete();
}

void CFrameEditor::OnEditCopy()
{
	CClipboard Clipboard(this, m_iClipboard);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	Clipboard.TryCopy(*CopySelection(GetSelection(), m_pMainFrame->GetSelectedTrack()));		// // //
}

std::unique_ptr<CFrameClipData> CFrameEditor::RestoreFrameClipData() {		// // //
	if (auto pClipData = std::make_unique<CFrameClipData>(); CClipboard {this, m_iClipboard}.TryRestore(*pClipData))
		return pClipData;
	return nullptr;
}

void CFrameEditor::OnEditPaste()
{
	m_pMainFrame->AddAction(std::make_unique<CFActionPaste>(RestoreFrameClipData(), GetEditFrame(), false));		// // //
}

void CFrameEditor::OnEditPasteOverwrite()		// // //
{
	m_pMainFrame->AddAction(std::make_unique<CFActionPasteOverwrite>(RestoreFrameClipData()));		// // //
}

void CFrameEditor::OnUpdateEditPasteOverwrite(CCmdUI *pCmdUI)		// // //
{
	pCmdUI->Enable(IsClipboardAvailable() ? 1 : 0);
}

void CFrameEditor::OnEditPasteNewPatterns()
{
	m_pMainFrame->AddAction(std::make_unique<CFActionPaste>(RestoreFrameClipData(), GetEditFrame(), true));		// // //
}

void CFrameEditor::OnEditDelete()
{
	SetSelection(model_->GetActiveSelection());		// // //
	m_pMainFrame->AddAction(std::make_unique<CFActionDeleteSel>( ));		// // //
}

std::pair<CFrameIterator, CFrameIterator> CFrameEditor::GetIterators() const		// // //
{
	int Track = m_pMainFrame->GetSelectedTrack();
	auto pSel = model_->GetSelection();
	return CFrameIterator::FromSelection(pSel ? *pSel : model_->GetCurrentPos(), m_pDocument->GetSongData(Track));
}

std::unique_ptr<CFrameClipData> CFrameEditor::CopySelection(const CFrameSelection &Sel, unsigned song) const		// // //
{
	return model_->CopySelection(Sel, song);
}

std::unique_ptr<CFrameClipData> CFrameEditor::CopyFrame(unsigned frame, unsigned song) const		// // //
{
	return CopySelection(model_->MakeFrameSelection(frame), song);
}

std::unique_ptr<CFrameClipData> CFrameEditor::CopyEntire(unsigned song) const		// // //
{
	return CopySelection(model_->MakeFullSelection(song), song);
}

void CFrameEditor::PasteInsert(unsigned int Track, int Frame, const CFrameClipData &ClipData)		// // //
{
	m_pDocument->AddFrames(Track, Frame, ClipData.ClipInfo.Frames);
	PasteAt(Track, ClipData, CFrameSelection {ClipData, Frame}.GetCursorStart());
}

void CFrameEditor::PasteAt(unsigned int Track, const CFrameClipData &ClipData, const CFrameCursorPos &Pos)		// // //
{
	model_->PasteSelection(ClipData, Pos, Track);
}

void CFrameEditor::ClearPatterns(unsigned int Track, const CFrameSelection &Sel)		// // //
{
	for (auto [b, e] = CFrameIterator::FromSelection(Sel, m_pDocument->GetSongData(Track)); b != e; ++b)
		for (int c = b.m_iChannel; c < e.m_iChannel; ++c)
			m_pDocument->ClearPattern(Track, b.m_iFrame, c);
}

bool CFrameEditor::InputEnabled() const
{
	return m_bInputEnable;
}

void CFrameEditor::ResetCursor()		// // //
{
	m_pView->SelectFrame(0);
	m_pView->SelectChannel(0);
	m_bLastRow = false;
	CancelSelection();
	InvalidateFrameData();
}

void CFrameEditor::OnModuleInsertFrame()
{
	m_pMainFrame->OnModuleInsertFrame();
}

void CFrameEditor::OnModuleRemoveFrame()
{
	m_pMainFrame->OnModuleRemoveFrame();
}

void CFrameEditor::OnModuleDuplicateFrame()
{
	m_pMainFrame->OnModuleDuplicateFrame();
}

void CFrameEditor::OnModuleDuplicateFramePatterns()
{
	m_pMainFrame->OnModuleDuplicateFramePatterns();
}

void CFrameEditor::OnModuleMoveFrameDown()
{
	m_pMainFrame->OnModuleMoveframedown();
}

void CFrameEditor::OnModuleMoveFrameUp()
{
	m_pMainFrame->OnModuleMoveframeup();
}

void CFrameEditor::OnModuleDuplicateCurrentPattern()		// // //
{
	m_pMainFrame->OnModuleDuplicateCurrentPattern();
}

void CFrameEditor::OnEditSelectpattern()		// // //
{
	SetSelection(model_->GetCurrentPos());
}

void CFrameEditor::OnEditSelectframe()		// // //
{
	SetSelection(model_->MakeFrameSelection(model_->GetCurrentFrame()));
}

void CFrameEditor::OnEditSelectchannel()		// // //
{
	int maxframe = m_pDocument->GetFrameCount(m_pMainFrame->GetSelectedTrack()) - 1;
	int channel = model_->GetCurrentChannel();
	SetSelection(CFrameSelection::Including({0, channel}, {maxframe, channel}));
}

void CFrameEditor::OnEditSelectall()		// // //
{
	SetSelection(model_->MakeFullSelection(m_pMainFrame->GetSelectedTrack()));
}

void CFrameEditor::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	m_iWinWidth = cx;
	m_iWinHeight = cy;

	int Offset = DPI::SY(TOP_OFFSET);		// // // 050B
	int Height = DPI::SY(ROW_HEIGHT);		// // // 050B

	// Get number of rows visible
	m_iRowsVisible = (cy - Offset) / Height;		// // //
	m_iMiddleRow = m_iRowsVisible / 2;

	m_iTopScrollArea = ROW_HEIGHT;
	m_iBottomScrollArea = cy - ROW_HEIGHT * 2;

	// Delete the back buffer
	m_bmpBack.DeleteObject();
	m_dcBack.DeleteDC();
}

void CFrameEditor::CancelSelection()
{
	if (IsSelecting())		// // //
		InvalidateFrameData();
	model_->Deselect();		// // //
	m_bStartDrag = false;
}

void CFrameEditor::InitiateDrag()
{
	auto pClipData = CopySelection(GetSelection(), m_pMainFrame->GetSelectedTrack());		// // //

	DROPEFFECT res = pClipData->DragDropTransfer(m_iClipboard, DROPEFFECT_COPY | DROPEFFECT_MOVE);		// // // calls DropData

	if (!m_bDropped) { // Target was another window
		if (res & DROPEFFECT_MOVE)
			m_pMainFrame->AddAction(std::make_unique<CFActionDeleteSel>());		// // // Delete data
		CancelSelection();
	}

	m_bStartDrag = false;
}

int CFrameEditor::GetEditFrame() const		// // //
{
	int Frame = model_->GetCurrentFrame();
	if (m_bLastRow)
		if (Frame != m_pDocument->GetFrameCount(m_pMainFrame->GetSelectedTrack()) - 1) {
			m_bLastRow = false;
			SetEditFrame(++Frame);
		}
	return Frame + (m_bLastRow ? 1 : 0);
}

void CFrameEditor::SetEditFrame(int Frame) const		// // //
{
	int Frames = m_pDocument->GetFrameCount(m_pMainFrame->GetSelectedTrack());
//	if (m_bLastRow != (Frame >= Frames))
//		InvalidateFrameData();
	if (m_bLastRow = (Frame >= Frames))
		Frame = Frames - 1;
	m_pView->SelectFrame(Frame);
}

bool CFrameEditor::IsCopyValid(COleDataObject* pDataObject) const
{
	// Return true if the number of pasted frames will fit
	CFrameClipData ClipData;		// // //
	HGLOBAL hMem = pDataObject->GetGlobalData(m_iClipboard);
	if (ClipData.ReadGlobalMemory(hMem)) {		// // //
		int Frames = ClipData.ClipInfo.Frames;
		return (m_pDocument->GetFrameCount(m_pMainFrame->GetSelectedTrack()) + Frames) <= MAX_FRAMES;
	}
	return false;
}

void CFrameEditor::UpdateDrag(const CPoint &point)
{
	CPoint newPoint(point.x, point.y + ROW_HEIGHT / 2);
	m_iDragRow = GetRowFromPoint(newPoint, true);
	AutoScroll(point);
	InvalidateFrameData();
}

BOOL CFrameEditor::DropData(COleDataObject* pDataObject, DROPEFFECT dropEffect)
{
	// Drag'n'drop operation completed

	const int Track  = m_pMainFrame->GetSelectedTrack();
	const int Frames = m_pDocument->GetFrameCount(Track);

	ASSERT(m_iDragRow >= 0 && m_iDragRow <= Frames);

	// Get frame data
	auto pClipData = std::make_unique<CFrameClipData>();		// // //
	if (!pClipData->ReadGlobalMemory(pDataObject->GetGlobalData(m_iClipboard)))		// // //
		return FALSE;

	const int SelectStart = pClipData->ClipInfo.OleInfo.SourceRowStart;
	const int SelectEnd	  = pClipData->ClipInfo.OleInfo.SourceRowEnd;

	// Add action
	switch (dropEffect) {
	case DROPEFFECT_MOVE:
		// Move
		if (m_bStartDrag) {		// // // same window
			// Disallow dragging to the same area
			if (m_iDragRow >= SelectStart && m_iDragRow <= (SelectEnd + 1))		// // //
				return FALSE;
			int target = m_iDragRow - (m_iDragRow > SelectStart ? pClipData->ClipInfo.Frames : 0);
			m_pMainFrame->AddAction(std::make_unique<CFActionDropMove>(std::move(pClipData), target));
			break;
		}
		[[fallthrough]];
	case DROPEFFECT_COPY:
		// Copy
		if (!m_pMainFrame->AddAction(std::make_unique<CFActionPaste>(
			std::move(pClipData), m_iDragRow, m_DropTarget.CopyToNewPatterns())))		// // //
			return FALSE;
		break;
	}

	m_bDropped = true;

	InvalidateFrameData();

	return TRUE;
}

void CFrameEditor::MoveSelection(unsigned int Track, const CFrameSelection &Sel, const CFrameCursorPos &Target)		// // //
{
	if (Target.m_iFrame == Sel.GstFirstSelectedFrame())
		return;
	CFrameSelection Normal = Sel.GetNormalized();
	CFrameCursorPos startPos = Sel.GetCursorStart();
	auto pData = model_->CopySelection(Sel, Track);
	const int Frames = Sel.GetSelectedFrameCount();

	int Delta = Target.m_iFrame - Sel.GstFirstSelectedFrame();
	if (Delta > 0) {
		// [Sel           ]                       [Target
		CFrameSelection Tail = Normal;
		Tail.m_cpStart.m_iFrame = startPos.m_iFrame + Frames;
		Tail.m_cpEnd.m_iFrame = Target.m_iFrame + Frames;
		//                [Tail                   ]
		auto pRest = model_->CopySelection(Tail, Track);
		PasteAt(Track, *pRest, Normal.m_cpStart);
		// <Tail                   >
		startPos.m_iFrame += Delta;
		PasteAt(Track, *pData, startPos);
		//                         <Sel           >
	}
	else {
		//    [Target              [Sel    ]
		CFrameSelection Head = Normal;
		Head.m_cpEnd.m_iFrame = startPos.m_iFrame;
		Head.m_cpStart.m_iFrame = Target.m_iFrame;
		//    [Head                ]
		auto pRest = model_->CopySelection(Head, Track);
		PasteAt(Track, *pData, Head.m_cpStart);
		PasteAt(Track, *pRest, {Head.m_cpStart.m_iFrame + Frames, Head.m_cpStart.m_iChannel});
	}
	Normal.m_cpStart.m_iFrame += Delta;
	Normal.m_cpEnd.m_iFrame += Delta;
	SetSelection(Normal);
}

CFrameSelection CFrameEditor::GetSelection() const		// // //
{
	return model_->GetActiveSelection();
}

bool CFrameEditor::IsSelecting() const		// // //
{
	return model_->IsSelecting();
}

void CFrameEditor::SetSelection(const CFrameSelection &Sel)		// // //
{
	model_->Select(Sel);
	InvalidateFrameData();
}

bool CFrameEditor::IsClipboardAvailable() const
{
	return ::IsClipboardFormatAvailable(m_iClipboard) == TRUE;
}

// CFrameEditorDropTarget ////////////

void CFrameEditorDropTarget::SetClipBoardFormat(UINT iClipboard)
{
	m_iClipboard = iClipboard;
}

DROPEFFECT CFrameEditorDropTarget::OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	CFrameWnd *pMainFrame = dynamic_cast<CFrameWnd*>(theApp.m_pMainWnd);

	if (pDataObject->IsDataAvailable(m_iClipboard)) {
		if (dwKeyState & MK_CONTROL) {
			if (m_pParent->IsCopyValid(pDataObject)) {
				m_nDropEffect = DROPEFFECT_COPY;
				m_bCopyNewPatterns = true;
				if (pMainFrame != NULL)
					pMainFrame->SetMessageText(IDS_FRAME_DROP_COPY_NEW);
			}
		}
		else if (dwKeyState & MK_SHIFT) {
			if (m_pParent->IsCopyValid(pDataObject)) {
				m_nDropEffect = DROPEFFECT_COPY;
				m_bCopyNewPatterns = false;
				if (pMainFrame != NULL)
					pMainFrame->SetMessageText(IDS_FRAME_DROP_COPY);
			}
		}
		else {
			m_nDropEffect = DROPEFFECT_MOVE;
			if (pMainFrame != NULL)
				pMainFrame->SetMessageText(IDS_FRAME_DROP_MOVE);
		}
		m_pParent->UpdateDrag(point);
	}

	return m_nDropEffect;
}

DROPEFFECT CFrameEditorDropTarget::OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	m_pParent->UpdateDrag(point);
	return m_nDropEffect;
}

BOOL CFrameEditorDropTarget::OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	m_nDropEffect = DROPEFFECT_NONE;
	m_pParent->UpdateDrag(point);
	return m_pParent->DropData(pDataObject, dropEffect);
}

void CFrameEditorDropTarget::OnDragLeave(CWnd* pWnd)
{
	m_nDropEffect = DROPEFFECT_NONE;
	m_pParent->Invalidate();
}

bool CFrameEditorDropTarget::IsDragging() const
{
	return m_nDropEffect != DROPEFFECT_NONE;
}

bool CFrameEditorDropTarget::CopyToNewPatterns() const
{
	return m_bCopyNewPatterns;
}
