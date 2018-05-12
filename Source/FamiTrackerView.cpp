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

#include "FamiTrackerView.h"
#include "FamiTrackerDoc.h"
#include "MainFrm.h"
#include "MIDI.h"
#include <cmath>
#include "InstrumentEditDlg.h"
#include "SoundGen.h"
#include "PatternAction.h"
#include "PatternEditor.h"
#include "FrameEditor.h"
#include "Settings.h"
#include "Accelerator.h"
#include "TrackerChannel.h"
#include "Clipboard.h"
// // //
#include "FamiTrackerEnv.h"
#include "SoundChipService.h"
#include "FamiTrackerModule.h"
#include "Bookmark.h"
#include "BookmarkCollection.h"
#include "ChannelOrder.h"
#include "SongView.h"
#include "DetuneDlg.h"
#include "StretchDlg.h"
#include "RecordSettingsDlg.h"
#include "SplitKeyboardDlg.h"
#include "NoteQueue.h"
#include "Arpeggiator.h"
#include "PatternClipData.h"
#include "StringClipData.h"
#include "ModuleAction.h"
#include "InstrumentRecorder.h"
#include "InstrumentManager.h"
#include "Instrument.h"
#include "Sequence.h"
#include "SoundChipSet.h"
#include "Assertion.h"
#include "str_conv/str_conv.hpp"

// Clipboard ID
const WCHAR CFamiTrackerView::CLIPBOARD_ID[] = L"FamiTracker Pattern";

// Effect texts
// 0CC: add verbose description as in modplug
const LPCWSTR EFFECT_TEXTS[] = {		// // //
	L"(not used)",
	L"Fxx - Set speed to XX, cancels groove",
	L"Fxx - Set tempo to XX",
	L"Bxx - Jump to beginning of frame XX",
	L"Dxx - Skip to row XX of next frame",
	L"Cxx - Halt song",
	L"Exx - Set length counter index to XX",
	L"EEx - Set length counter mode, bit 0 = length counter, bit 1 = disable loop",
	L"3xx - Automatic portamento, XX = speed",
	L"(not used)",
	L"Hxy - Hardware sweep up, X = speed, Y = shift",
	L"Ixy - Hardware sweep down, X = speed, Y = shift",
	L"0xy - Arpeggio, X = second note, Y = third note",
	L"4xy - Vibrato, X = speed, Y = depth",
	L"7xy - Tremolo, X = speed, Y = depth",
	L"Pxx - Fine pitch, XX - 80 = offset",
	L"Gxx - Row delay, XX = number of frames",
	L"Zxx - DPCM delta counter setting, XX = DC bias",
	L"1xx - Slide up, XX = speed",
	L"2xx - Slide down, XX = speed",
	L"Vxx - Set Square duty / Noise mode to XX",
	L"Vxx - Set N163 wave index to XX",
	L"Vxx - Set VRC7 patch index to XX",
	L"Yxx - Set DPCM sample offset to XX",
	L"Qxy - Portamento up, X = speed, Y = notes",
	L"Rxy - Portamento down, X = speed, Y = notes",
	L"Axy - Volume slide, X = up, Y = down",
	L"Sxx - Note cut, XX = frames to wait",
	L"Sxx - Triangle channel linear counter, XX - 80 = duration",
	L"Xxx - DPCM retrigger, XX = frames to wait",
	L"Mxy - Delayed channel volume, X = frames to wait, Y = channel volume",
	L"Hxx - FDS modulation depth, XX = depth, 3F = highest",
	L"Hxx - Auto FDS modulation ratio, XX - 80 = multiplier",
	L"I0x - FDS modulation rate, high byte; disable auto modulation",
	L"Ixy - Auto FDS modulation, X = multiplier, Y + 1 = divider",
	L"Jxx - FDS modulation rate, low byte",
	L"W0x - DPCM pitch, F = highest",
	L"H0y - 5B envelope shape, bit 3 = Continue, bit 2 = Attack, bit 1 = Alternate, bit 0 = Hold",
	L"Hxy - Auto 5B envelope, X - 8 = shift amount, Y = shape",
	L"Ixx - 5B envelope rate, high byte",
	L"Jxx - 5B envelope rate, low byte",
	L"Wxx - 5B noise pitch, 1F = lowest",
	L"Hxx - VRC7 custom patch port, XX = register address",
	L"Ixx - VRC7 custom patch write, XX = register value",
	L"Lxx - Note release, XX = frames to wait",
	L"Oxx - Set groove to XX",
	L"Txy - Delayed transpose (upward), X = frames to wait, Y = semitone offset",
	L"Txy - Delayed transpose (downward), X - 8 = frames to wait, Y = semitone offset",
	L"Zxx - N163 wave buffer access, XX = position in bytes",
	L"Exx - FDS volume envelope (attack), XX = rate",
	L"Exx - FDS volume envelope (decay), XX - 40 = rate",
	L"Zxx - Auto FDS modulation rate bias, XX - 80 = offset",
};

// OLE copy and mix
#define	DROPEFFECT_COPY_MIX	( 8 )

const int SINGLE_STEP = 1;				// Size of single step moves (default: 1)

// Timer IDs
enum {
	TMR_UPDATE,
	TMR_SCROLL,
};

// CFamiTrackerView

IMPLEMENT_DYNCREATE(CFamiTrackerView, CView)

BEGIN_MESSAGE_MAP(CFamiTrackerView, CView)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_TIMER()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_XBUTTONDOWN()		// // //
	ON_WM_MENUCHAR()
	ON_WM_SYSKEYDOWN()
	ON_COMMAND(ID_EDIT_PASTEMIX, OnEditPasteMix)
	ON_COMMAND(ID_EDIT_INSTRUMENTMASK, OnEditInstrumentMask)
	ON_COMMAND(ID_EDIT_VOLUMEMASK, OnEditVolumeMask)
	ON_COMMAND(ID_EDIT_INTERPOLATE, OnEditInterpolate)
	ON_COMMAND(ID_EDIT_REVERSE, OnEditReverse)
	ON_COMMAND(ID_EDIT_REPLACEINSTRUMENT, OnEditReplaceInstrument)
	ON_COMMAND(ID_TRANSPOSE_DECREASENOTE, OnTransposeDecreasenote)
	ON_COMMAND(ID_TRANSPOSE_DECREASEOCTAVE, OnTransposeDecreaseoctave)
	ON_COMMAND(ID_TRANSPOSE_INCREASENOTE, OnTransposeIncreasenote)
	ON_COMMAND(ID_TRANSPOSE_INCREASEOCTAVE, OnTransposeIncreaseoctave)
	ON_COMMAND(ID_DECREASEVALUES, OnDecreaseValues)
	ON_COMMAND(ID_INCREASEVALUES, OnIncreaseValues)
	ON_COMMAND(ID_TRACKER_PLAYROW, OnTrackerPlayrow)
	ON_COMMAND(ID_TRACKER_EDIT, OnTrackerEdit)
	ON_COMMAND(ID_TRACKER_TOGGLECHANNEL, OnTrackerToggleChannel)
	ON_COMMAND(ID_TRACKER_SOLOCHANNEL, OnTrackerSoloChannel)
	ON_COMMAND(ID_CMD_INCREASESTEPSIZE, OnIncreaseStepSize)
	ON_COMMAND(ID_CMD_DECREASESTEPSIZE, OnDecreaseStepSize)
	ON_COMMAND(ID_CMD_STEP_UP, OnOneStepUp)
	ON_COMMAND(ID_CMD_STEP_DOWN, OnOneStepDown)
	ON_COMMAND(ID_POPUP_TOGGLECHANNEL, OnTrackerToggleChannel)
	ON_COMMAND(ID_POPUP_SOLOCHANNEL, OnTrackerSoloChannel)
	ON_COMMAND(ID_POPUP_UNMUTEALLCHANNELS, OnTrackerUnmuteAllChannels)
//	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
//	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditDelete)
	ON_UPDATE_COMMAND_UI(ID_EDIT_INSTRUMENTMASK, OnUpdateEditInstrumentMask)
	ON_UPDATE_COMMAND_UI(ID_EDIT_VOLUMEMASK, OnUpdateEditVolumeMask)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_EDIT, OnUpdateTrackerEdit)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTEMIX, OnUpdateEditPaste)
	ON_WM_NCMOUSEMOVE()
	ON_COMMAND(ID_BLOCK_START, OnBlockStart)
	ON_COMMAND(ID_BLOCK_END, OnBlockEnd)
	ON_COMMAND(ID_POPUP_PICKUPROW, OnPickupRow)
	ON_MESSAGE(WM_USER_MIDI_EVENT, OnUserMidiEvent)
	ON_MESSAGE(WM_USER_PLAYER, OnUserPlayerEvent)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	// // //
	ON_MESSAGE(WM_USER_DUMP_INST, OnUserDumpInst)
	ON_COMMAND(ID_MODULE_DETUNE, OnTrackerDetune)
	ON_UPDATE_COMMAND_UI(ID_FIND_NEXT, OnUpdateFind)
	ON_UPDATE_COMMAND_UI(ID_FIND_PREVIOUS, OnUpdateFind)
	ON_COMMAND(ID_DECREASEVALUESCOARSE, OnCoarseDecreaseValues)
	ON_COMMAND(ID_INCREASEVALUESCOARSE, OnCoarseIncreaseValues)
//	ON_COMMAND(ID_EDIT_PASTEOVERWRITE, OnEditPasteOverwrite)
	ON_COMMAND(ID_EDIT_PASTEINSERT, OnEditPasteInsert)
//	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTEOVERWRITE, OnUpdateEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTEINSERT, OnUpdateEditPaste)
	ON_COMMAND(ID_PASTESPECIAL_CURSOR, OnEditPasteSpecialCursor)
	ON_COMMAND(ID_PASTESPECIAL_SELECTION, OnEditPasteSpecialSelection)
	ON_COMMAND(ID_PASTESPECIAL_FILL, OnEditPasteSpecialFill)
	ON_UPDATE_COMMAND_UI(ID_PASTESPECIAL_CURSOR, OnUpdatePasteSpecial)
	ON_UPDATE_COMMAND_UI(ID_PASTESPECIAL_SELECTION, OnUpdatePasteSpecial)
	ON_UPDATE_COMMAND_UI(ID_PASTESPECIAL_FILL, OnUpdatePasteSpecial)
	ON_COMMAND(ID_BOOKMARKS_TOGGLE, OnBookmarksToggle)
	ON_COMMAND(ID_BOOKMARKS_NEXT, OnBookmarksNext)
	ON_COMMAND(ID_BOOKMARKS_PREVIOUS, OnBookmarksPrevious)
	ON_COMMAND(ID_EDIT_SPLITKEYBOARD, OnEditSplitKeyboard)
	ON_COMMAND(ID_TRACKER_TOGGLECHIP, OnTrackerToggleChip)
	ON_COMMAND(ID_TRACKER_SOLOCHIP, OnTrackerSoloChip)
	ON_COMMAND(ID_TRACKER_RECORDTOINST, OnTrackerRecordToInst)
	ON_COMMAND(ID_TRACKER_RECORDERSETTINGS, OnTrackerRecorderSettings)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_RECORDTOINST, OnUpdateDisableWhilePlaying)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_RECORDERSETTINGS, OnUpdateDisableWhilePlaying)
	ON_COMMAND(ID_RECALL_CHANNEL_STATE, OnRecallChannelState)
	ON_COMMAND(ID_DECAY_FAST, CMainFrame::OnDecayFast)		// // //
	ON_COMMAND(ID_DECAY_SLOW, CMainFrame::OnDecaySlow)		// // //
END_MESSAGE_MAP()

namespace {

// Convert keys 0-F to numbers, -1 = invalid key
int ConvertKeyToHex(int Key) {
	switch (Key) {
	case '0': case VK_NUMPAD0: return 0x00;
	case '1': case VK_NUMPAD1: return 0x01;
	case '2': case VK_NUMPAD2: return 0x02;
	case '3': case VK_NUMPAD3: return 0x03;
	case '4': case VK_NUMPAD4: return 0x04;
	case '5': case VK_NUMPAD5: return 0x05;
	case '6': case VK_NUMPAD6: return 0x06;
	case '7': case VK_NUMPAD7: return 0x07;
	case '8': case VK_NUMPAD8: return 0x08;
	case '9': case VK_NUMPAD9: return 0x09;
	case 'A': return 0x0A;
	case 'B': return 0x0B;
	case 'C': return 0x0C;
	case 'D': return 0x0D;
	case 'E': return 0x0E;
	case 'F': return 0x0F;

	// case KEY_DOT:
	// case KEY_DASH:
	//	return 0x80;
	}

	return -1;
}

int ConvertKeyExtra(int Key) {		// // //
	switch (Key) {
	case VK_DIVIDE:   return 0x0A;
	case VK_MULTIPLY: return 0x0B;
	case VK_SUBTRACT: return 0x0C;
	case VK_ADD:      return 0x0D;
	case VK_RETURN:   return 0x0E;
	case VK_DECIMAL:  return 0x0F;
	}

	return -1;
}

} // namespace

// CFamiTrackerView construction/destruction

CFamiTrackerView::CFamiTrackerView() :
	m_iClipboard(0),
	m_iMoveKeyStepping(1),
	m_iInsertKeyStepping(1),
	m_bEditEnable(false),
	m_bMaskInstrument(false),
	m_bMaskVolume(true),
	m_bSwitchToInstrument(false),
	m_iPastePos(paste_pos_t::CURSOR),		// // //
	m_iSwitchToInstrument(-1),
	m_bFollowMode(true),
	m_bCompactMode(false),		// // //
	m_iMarkerFrame(-1),		// // // 050B
	m_iMarkerRow(-1),		// // // 050B
	m_iSplitNote(-1),		// // //
	m_iSplitInstrument(MAX_INSTRUMENTS),		// // //
	m_iSplitTranspose(0),		// // //
	m_iNoteCorrection(),		// // //
	m_pNoteQueue(std::make_unique<CNoteQueue>()),		// // //
	m_nDropEffect(DROPEFFECT_NONE),
	m_bDragSource(false),
	m_pPatternEditor(std::make_unique<CPatternEditor>())		// // //
{
	// Register this object in the sound generator
	CSoundGen *pSoundGen = Env.GetSoundGenerator();
	ASSERT_VALID(pSoundGen);

	pSoundGen->AssignView(this);
	m_pArpeggiator = &pSoundGen->GetArpeggiator();		// // //
}

CFamiTrackerView::~CFamiTrackerView()
{
}



#ifdef _DEBUG
CFamiTrackerDoc *CFamiTrackerView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFamiTrackerDoc)));
	auto pDoc = (CFamiTrackerDoc*)m_pDocument;
	ASSERT_VALID(pDoc);		// // //
	return pDoc;
}
#endif //_DEBUG



//
// Static functions
//

CFamiTrackerView *CFamiTrackerView::GetView()
{
	CFrameWnd *pFrame = static_cast<CFrameWnd*>(AfxGetApp()->m_pMainWnd);
	ASSERT_VALID(pFrame);

	CView *pView = pFrame->GetActiveView();

	if (!pView)
		return NULL;

	// Fail if view is of wrong kind
	// (this could occur with splitter windows, or additional
	// views on a single document
	if (!pView->IsKindOf(RUNTIME_CLASS(CFamiTrackerView)))
		return NULL;

	return static_cast<CFamiTrackerView*>(pView);
}

// Creation / destroy

int CFamiTrackerView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Install a timer for screen updates, 20ms
	SetTimer(TMR_UPDATE, 20, NULL);

	m_DropTarget.Register(this);

	// Setup pattern editor
	m_pPatternEditor->ApplyColorScheme();

	// Create clipboard format
	m_iClipboard = ::RegisterClipboardFormatW(CLIPBOARD_ID);

	if (m_iClipboard == 0)
		AfxMessageBox(IDS_CLIPBOARD_ERROR, MB_ICONERROR);

	return 0;
}

void CFamiTrackerView::OnDestroy()
{
	// Kill timers
	KillTimer(TMR_UPDATE);
	KillTimer(TMR_SCROLL);

	CView::OnDestroy();
}


// CFamiTrackerView message handlers

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tracker drawing routines
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::OnDraw(CDC* pDC)
{
	// How should we protect the DC in this method?

	// Check document
	if (!GetDocument()->IsFileLoaded()) {
		const WCHAR str[] = L"No module loaded.";		// // //
		pDC->FillSolidRect(0, 0, m_iWindowWidth, m_iWindowHeight, 0x000000);
		pDC->SetTextColor(0xFFFFFF);
		CRect textRect(0, 0, m_iWindowWidth, m_iWindowHeight);
		pDC->DrawTextW(str, std::size(str), &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		return;
	}

	// Don't draw when rendering to wave file
	CSoundGen *pSoundGen = Env.GetSoundGenerator();
	if (pSoundGen == NULL || pSoundGen->IsBackgroundTask())
		return;

	m_pPatternEditor->DrawScreen(*pDC, this);		// // //
	GetMainFrame()->GetFrameEditor()->DrawScreen(pDC);		// // //
}

BOOL CFamiTrackerView::OnEraseBkgnd(CDC* pDC)
{
	// Check document
	if (!GetDocument()->IsFileLoaded())
		return FALSE;

	// Called when the background should be erased
	m_pPatternEditor->CreateBackground(*pDC);		// // //

	return FALSE;
}

void CFamiTrackerView::SetupColors()
{
	// Color scheme has changed
	CMainFrame *pMainFrame = static_cast<CMainFrame*>(GetParentFrame());
	m_pPatternEditor->ApplyColorScheme();

	m_pPatternEditor->InvalidatePatternData();
	m_pPatternEditor->InvalidateBackground();
	RedrawPatternEditor();

	// Frame editor
	CFrameEditor *pFrameEditor = GetFrameEditor();
	pFrameEditor->SetupColors();
	pFrameEditor->RedrawFrameEditor();

	pMainFrame->RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
}

void CFamiTrackerView::UpdateMeters()
{
	// TODO: Change this to use the ordinary drawing routines
	m_csDrawLock.Lock();

	CDC *pDC = GetDC();
	if (pDC && pDC->m_hDC) {
		m_pPatternEditor->DrawMeters(*pDC);		// // //
		ReleaseDC(pDC);
	}

	m_csDrawLock.Unlock();
}

void CFamiTrackerView::InvalidateCursor()
{
	// Cursor has moved, redraw screen
	m_pPatternEditor->InvalidateCursor();
	RedrawPatternEditor();
	RedrawFrameEditor();

	CCursorPos p = m_pPatternEditor->GetCursor();
	if (m_cpLastCursor != p) {		// // //
		m_cpLastCursor = p;
		static_cast<CMainFrame*>(GetParentFrame())->ResetFind();		// // //
	}
}

void CFamiTrackerView::InvalidateHeader()
{
	// Header area has changed (channel muted etc...)
	m_pPatternEditor->InvalidateHeader();
	RedrawWindow(m_pPatternEditor->GetHeaderRect(), NULL, RDW_INVALIDATE);
}

void CFamiTrackerView::InvalidatePatternEditor()
{
	// Pattern data has changed, redraw screen
	RedrawPatternEditor();
	// ??? TODO do we need this??
	RedrawFrameEditor();
}

void CFamiTrackerView::InvalidateFrameEditor()
{
	// Frame data has changed, redraw frame editor

	CFrameEditor *pFrameEditor = GetFrameEditor();
	pFrameEditor->InvalidateFrameData();

	RedrawFrameEditor();
	// Update pattern editor according to selected frame
	RedrawPatternEditor();
}

void CFamiTrackerView::RedrawPatternEditor()
{
	// Redraw the pattern editor, partial or full if needed
	bool NeedErase = m_pPatternEditor->CursorUpdated();

	if (NeedErase)
		RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
	else
		RedrawWindow(m_pPatternEditor->GetInvalidatedRect(), NULL, RDW_INVALIDATE);
}

void CFamiTrackerView::RedrawFrameEditor()
{
	// Redraw the frame editor
	CFrameEditor *pFrameEditor = GetFrameEditor();
	pFrameEditor->RedrawFrameEditor();
}

CMainFrame *CFamiTrackerView::GetMainFrame() const {
#ifdef _DEBUG
	ASSERT(GetParentFrame()->IsKindOf(RUNTIME_CLASS(CMainFrame)));
	auto pMainFrame = (CMainFrame *)GetParentFrame();
	ASSERT_VALID(pMainFrame);
	return pMainFrame;
#else
	return static_cast<CMainFrame* >(GetParentFrame());
#endif
}

CFrameEditor *CFamiTrackerView::GetFrameEditor() const
{
	CMainFrame *pMainFrame = GetMainFrame();		// // //
	CFrameEditor *pFrameEditor = pMainFrame->GetFrameEditor();
	ASSERT_VALID(pFrameEditor);
	return pFrameEditor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General
////////////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT CFamiTrackerView::OnUserMidiEvent(WPARAM wParam, LPARAM lParam)
{
	TranslateMidiMessage();
	return 0;
}

LRESULT CFamiTrackerView::OnUserPlayerEvent(WPARAM wParam, LPARAM lParam)
{
	// Player is playing
	// TODO clean up
//	int Frame = (int)wParam;
//	int Row = (int)lParam;

	m_pPatternEditor->InvalidateCursor();
	RedrawPatternEditor();

	//m_pPatternEditor->AdjustCursor();	// Fix frame editor cursor
	RedrawFrameEditor();
	return 0;
}

void CFamiTrackerView::CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType)
{
	// Window size has changed
	m_iWindowWidth	= lpClientRect->right - lpClientRect->left;
	m_iWindowHeight	= lpClientRect->bottom - lpClientRect->top;

	m_iWindowWidth  -= ::GetSystemMetrics(SM_CXEDGE) * 2;
	m_iWindowHeight -= ::GetSystemMetrics(SM_CYEDGE) * 2;

	m_pPatternEditor->SetWindowSize(m_iWindowWidth, m_iWindowHeight);
	m_pPatternEditor->InvalidateBackground();
	// Update cursor since first visible channel might change
	m_pPatternEditor->CursorUpdated();

	CView::CalcWindowRect(lpClientRect, nAdjustType);
}

// Scroll

void CFamiTrackerView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	m_pPatternEditor->OnVScroll(nSBCode, nPos);
	InvalidateCursor();

	CView::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CFamiTrackerView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	m_pPatternEditor->OnHScroll(nSBCode, nPos);
	InvalidateCursor();

	CView::OnHScroll(nSBCode, nPos, pScrollBar);
}

// Mouse

void CFamiTrackerView::OnRButtonUp(UINT nFlags, CPoint point)
{
	// Popup menu
	CRect WinRect;
	CMenu *pPopupMenu, PopupMenuBar;

	if (m_pPatternEditor->CancelDragging()) {
		InvalidateCursor();
		CView::OnRButtonUp(nFlags, point);
		return;
	}

	m_pPatternEditor->OnMouseRDown(point);

	GetWindowRect(WinRect);

	if (m_pPatternEditor->IsOverHeader(point)) {
		// Pattern header
		m_iMenuChannel = TranslateChannel(m_pPatternEditor->GetChannelAtPoint(point.x));		// // //
		PopupMenuBar.LoadMenuW(IDR_PATTERN_HEADER_POPUP);
		Env.GetMainFrame()->UpdateMenu(&PopupMenuBar);
		pPopupMenu = PopupMenuBar.GetSubMenu(0);
		pPopupMenu->EnableMenuItem(ID_TRACKER_RECORDTOINST, Env.GetSoundGenerator()->IsPlaying() ? MFS_DISABLED : MFS_ENABLED);		// // //
		pPopupMenu->EnableMenuItem(ID_TRACKER_RECORDERSETTINGS, Env.GetSoundGenerator()->IsPlaying() ? MFS_DISABLED : MFS_ENABLED);		// // //
		CMenu *pMeterMenu = pPopupMenu->GetSubMenu(6);		// // // 050B
		decay_rate_t Rate = Env.GetSoundGenerator()->GetMeterDecayRate();
		pMeterMenu->CheckMenuItem(Rate == decay_rate_t::Fast ? ID_DECAY_FAST : ID_DECAY_SLOW, MF_CHECKED | MF_BYCOMMAND);
		pPopupMenu->TrackPopupMenu(TPM_RIGHTBUTTON, point.x + WinRect.left, point.y + WinRect.top, this);
	}
	else if (m_pPatternEditor->IsOverPattern(point)) {		// // // 050B todo
		// Pattern area
		m_iMenuChannel = { };
		PopupMenuBar.LoadMenuW(IDR_PATTERN_POPUP);
		Env.GetMainFrame()->UpdateMenu(&PopupMenuBar);
		pPopupMenu = PopupMenuBar.GetSubMenu(0);
		// Send messages to parent in order to get the menu options working
		pPopupMenu->TrackPopupMenu(TPM_RIGHTBUTTON, point.x + WinRect.left, point.y + WinRect.top, GetParentFrame());
	}

	CView::OnRButtonUp(nFlags, point);
}

void CFamiTrackerView::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetTimer(TMR_SCROLL, 10, NULL);

	m_pPatternEditor->OnMouseDown(point);
	SetCapture();	// Capture mouse
	InvalidateCursor();

	if (m_pPatternEditor->IsOverHeader(point))
		InvalidateHeader();

	CView::OnLButtonDown(nFlags, point);
}

void CFamiTrackerView::OnLButtonUp(UINT nFlags, CPoint point)
{
	KillTimer(TMR_SCROLL);

	m_pPatternEditor->OnMouseUp(point);
	ReleaseCapture();	// Release mouse

	InvalidateCursor();
	InvalidateHeader();

	/*
	if (m_bControlPressed && !m_pPatternEditor->IsSelecting()) {
		m_pPatternEditor->JumpToRow(GetSelectedRow());
		Env.GetSoundGenerator()->StartPlayer(play_mode_t::CURSOR);
	}
	*/

	CView::OnLButtonUp(nFlags, point);
}

void CFamiTrackerView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (Env.GetSettings()->General.bDblClickSelect && !m_pPatternEditor->IsOverHeader(point))
		return;

	m_pPatternEditor->OnMouseDblClk(point);
	InvalidateCursor();
	CView::OnLButtonDblClk(nFlags, point);
}

void CFamiTrackerView::OnXButtonDown(UINT nFlags, UINT nButton, CPoint point)		// // //
{
	/*
	switch (nFlags & (MK_CONTROL | MK_SHIFT)) {
	case MK_CONTROL:
		if (nButton == XBUTTON2)
			OnEditCopy();
		else if (nButton == XBUTTON1)
			OnEditPaste();
		break;
	case MK_SHIFT:
		m_iMenuChannel = m_pPatternEditor->GetChannelAtPoint(point.x);
		if (nButton == XBUTTON2)
			OnTrackerToggleChannel();
		else if (nButton == XBUTTON1)
			OnTrackerSoloChannel();
		break;
	default:
		if (nButton == XBUTTON2)
			static_cast<CMainFrame*>(GetParentFrame())->OnEditRedo();
		else if (nButton == XBUTTON1)
			static_cast<CMainFrame*>(GetParentFrame())->OnEditUndo();
	}
	*/
	CView::OnXButtonDown(nFlags, nButton, point);
}

void CFamiTrackerView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (nFlags & MK_LBUTTON) {
		// Left button down
		m_pPatternEditor->OnMouseMove(nFlags, point);
		InvalidateCursor();
	}
	else {
		// Left button up
		if (m_pPatternEditor->OnMouseHover(nFlags, point))
			InvalidateHeader();
	}

	CView::OnMouseMove(nFlags, point);
}

BOOL CFamiTrackerView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	std::unique_ptr<CAction> pAction;		// // //

	bool bShiftPressed = IsShiftPressed();
	bool bControlPressed = IsControlPressed();

	if (bControlPressed && bShiftPressed) {
		if (zDelta < 0)
			m_pPatternEditor->NextFrame();
		else
			m_pPatternEditor->PreviousFrame();
		InvalidateFrameEditor();		// // //
	}
	else if (bControlPressed) {
		if (!(Env.GetSoundGenerator()->IsPlaying() && !IsSelecting() && m_bFollowMode))		// // //
			pAction = std::make_unique<CPActionTranspose>(zDelta > 0 ? 1 : -1);		// // //
	}
	else if (bShiftPressed) {
		if (!(Env.GetSoundGenerator()->IsPlaying() && !IsSelecting() && m_bFollowMode))
			pAction = std::make_unique<CPActionScrollValues>(zDelta > 0 ? 1 : -1);		// // //
	}
	else
		m_pPatternEditor->OnMouseScroll(zDelta);

	pAction ? (void)AddAction(std::move(pAction)) : InvalidateCursor();

	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

// End of mouse

void CFamiTrackerView::OnKillFocus(CWnd* pNewWnd)
{
	CView::OnKillFocus(pNewWnd);
	m_bHasFocus = false;
	m_pPatternEditor->SetFocus(false);
	InvalidateCursor();
}

void CFamiTrackerView::OnSetFocus(CWnd* pOldWnd)
{
	CView::OnSetFocus(pOldWnd);
	m_bHasFocus = true;
	m_pPatternEditor->SetFocus(true);
	CMainFrame *pMainFrm = static_cast<CMainFrame*>(GetParentFrame());		// // //
	pMainFrm->GetFrameEditor()->CancelSelection();
	InvalidateCursor();
}

void CFamiTrackerView::OnTimer(UINT_PTR nIDEvent)
{
	// Timer callback function
	switch (nIDEvent) {
		// Drawing updates when playing
		case TMR_UPDATE:
			PeriodicUpdate();
			break;

		// Auto-scroll timer
		case TMR_SCROLL:
			if (m_pPatternEditor->ScrollTimerCallback()) {
				// Redraw entire since pattern layout might change
				RedrawPatternEditor();
			}
			break;
	}

	CView::OnTimer(nIDEvent);
}

void CFamiTrackerView::PeriodicUpdate()
{
	// Called periodically by an background timer

	CMainFrame *pMainFrm = GetMainFrame();		// // //

	if (const CSoundGen *pSoundGen = Env.GetSoundGenerator()) {
		// Skip updates when doing background tasks (WAV render for example)
		if (!pSoundGen->IsBackgroundTask()) {

			int PlayTicks = pSoundGen->GetPlayerTicks();
			int PlayTime = (PlayTicks * 10) / GetModuleData()->GetFrameRate();

			// Play time
			int Min = PlayTime / 600;
			int Sec = (PlayTime / 10) % 60;
			int mSec = PlayTime % 10;

			pMainFrm->SetIndicatorTime(Min, Sec, mSec);

			auto [Frame, Row] = pSoundGen->GetPlayerPos();		// // //

			pMainFrm->SetIndicatorPos(Frame, Row);

			if (GetDocument()->IsFileLoaded()) {
				UpdateMeters();
				// // //
			}
		}

		if (GetSongView()->GetChannelOrder().GetChannelCount()) {		// // //
			int Note = pSoundGen->GetChannelNote(GetSelectedChannelID());		// // //
			if (m_iLastNoteState != Note) {
				pMainFrm->ChangeNoteState(Note);
				m_iLastNoteState = Note;
			}
		}
	}

	// Switch instrument
	if (m_iSwitchToInstrument != -1) {
		SetInstrument(m_iSwitchToInstrument);
		m_iSwitchToInstrument = -1;
	}
}

void CFamiTrackerView::UpdateSongView() {		// // //
	song_view_ = GetModuleData()->MakeSongView(GetMainFrame()->GetSelectedTrack(),
		Env.GetSettings()->General.bShowSkippedRows);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Menu commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::OnEditCopy()
{
	if (m_pPatternEditor->GetSelectionCondition() == sel_condition_t::NONTERMINAL_SKIP) {		// // //
		CMainFrame *pMainFrm = GetMainFrame();		// // //
		MessageBeep(MB_ICONWARNING);
		pMainFrm->SetMessageText(IDS_SEL_NONTERMINAL_SKIP);
		return;
	}

	CClipboard::CopyToClipboard(this, m_iClipboard, m_pPatternEditor->Copy());		// // //
}

void CFamiTrackerView::OnEditCut()
{
	if (!m_bEditEnable) return;		// // //
	OnEditCopy();
	OnEditDelete();
}

void CFamiTrackerView::OnEditPaste()
{
	if (m_bEditEnable)
		DoPaste(paste_mode_t::DEFAULT);
}

void CFamiTrackerView::OnEditPasteMix()		// // //
{
	if (m_bEditEnable)
		DoPaste(paste_mode_t::MIX);
}

void CFamiTrackerView::OnEditPasteOverwrite()		// // //
{
	if (m_bEditEnable)
		DoPaste(paste_mode_t::OVERWRITE);
}

void CFamiTrackerView::OnEditPasteInsert()		// // //
{
	if (m_bEditEnable)
		DoPaste(paste_mode_t::INSERT);
}

void CFamiTrackerView::DoPaste(paste_mode_t Mode) {		// // //
	if (auto ClipData = CClipboard::RestoreFromClipboard<CPatternClipData>(this, m_iClipboard))		// // //
		AddAction(std::make_unique<CPActionPaste>(*std::move(ClipData), Mode, m_iPastePos));
}

void CFamiTrackerView::OnEditDelete()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(std::make_unique<CPActionClearSel>());		// // //
}

void CFamiTrackerView::OnTrackerEdit()
{
	m_bEditEnable = !m_bEditEnable;

	if (m_bEditEnable)
		GetParentFrame()->SetMessageText(IDS_EDIT_MODE);
	else
		GetParentFrame()->SetMessageText(IDS_NORMAL_MODE);

	m_pPatternEditor->InvalidateBackground();
	m_pPatternEditor->InvalidateHeader();
	m_pPatternEditor->InvalidateCursor();
	RedrawPatternEditor();
	UpdateNoteQueues();		// // //
}

LRESULT CFamiTrackerView::OnUserDumpInst(WPARAM wParam, LPARAM lParam)		// // //
{
	AddAction(std::make_unique<ModuleAction::CAddInst>(GetModuleData()->GetInstrumentManager()->GetFirstUnused(),
		Env.GetSoundGenerator()->GetRecordInstrument()));
	Env.GetSoundGenerator()->ResetDumpInstrument();
	InvalidateHeader();

	return 0;
}

void CFamiTrackerView::OnTrackerDetune()			// // //
{
	CFamiTrackerModule *pModule = GetModuleData();
	CDetuneDlg DetuneDlg;
	DetuneDlg.AssignModule(*pModule);
	if (DetuneDlg.DoModal() != IDOK)
		return;
	GetDocument()->ModifyIrreversible();
	const int *Table = DetuneDlg.GetDetuneTable();
	for (int i = 0; i < 6; ++i)
		for (int j = 0; j < NOTE_COUNT; ++j)
			pModule->SetDetuneOffset(i, j, *(Table + j + i * NOTE_COUNT));
	pModule->SetTuning(DetuneDlg.GetDetuneSemitone(), DetuneDlg.GetDetuneCent());		// // // 050B
	Env.GetSoundGenerator()->DocumentPropertiesChanged(GetDocument());
}

void CFamiTrackerView::OnTransposeDecreasenote()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(std::make_unique<CPActionTranspose>(-1));
}

void CFamiTrackerView::OnTransposeDecreaseoctave()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(std::make_unique<CPActionTranspose>(-NOTE_RANGE));
}

void CFamiTrackerView::OnTransposeIncreasenote()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(std::make_unique<CPActionTranspose>(1));
}

void CFamiTrackerView::OnTransposeIncreaseoctave()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(std::make_unique<CPActionTranspose>(NOTE_RANGE));
}

void CFamiTrackerView::OnDecreaseValues()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(std::make_unique<CPActionScrollValues>(-1));
}

void CFamiTrackerView::OnIncreaseValues()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(std::make_unique<CPActionScrollValues>(1));
}

void CFamiTrackerView::OnCoarseDecreaseValues()		// // //
{
	if (!m_bEditEnable) return;
	AddAction(std::make_unique<CPActionScrollValues>(-16));
}

void CFamiTrackerView::OnCoarseIncreaseValues()		// // //
{
	if (!m_bEditEnable) return;
	AddAction(std::make_unique<CPActionScrollValues>(16));
}

void CFamiTrackerView::OnEditInstrumentMask()
{
	m_bMaskInstrument = !m_bMaskInstrument;
}

void CFamiTrackerView::OnEditVolumeMask()
{
	m_bMaskVolume = !m_bMaskVolume;
}

void CFamiTrackerView::OnEditSelectall()
{
	m_pPatternEditor->SelectAll();
	InvalidateCursor();
}

void CFamiTrackerView::OnEditSelectnone()		// // //
{
	m_pPatternEditor->CancelSelection();
	InvalidateCursor();
}

void CFamiTrackerView::OnEditSelectrow()		// // //
{
	m_pPatternEditor->SetSelection(SEL_SCOPE_VROW | SEL_SCOPE_HFRAME);
	InvalidateCursor();
}

void CFamiTrackerView::OnEditSelectcolumn()		// // //
{
	m_pPatternEditor->SetSelection(SEL_SCOPE_VFRAME | SEL_SCOPE_HCOL);
	InvalidateCursor();
}

void CFamiTrackerView::OnEditSelectpattern()		// // //
{
	m_pPatternEditor->SetSelection(SEL_SCOPE_VFRAME | SEL_SCOPE_HCHAN);
	InvalidateCursor();
}

void CFamiTrackerView::OnEditSelectframe()		// // //
{
	m_pPatternEditor->SetSelection(SEL_SCOPE_VFRAME | SEL_SCOPE_HFRAME);
	InvalidateCursor();
}

void CFamiTrackerView::OnEditSelectchannel()		// // //
{
	m_pPatternEditor->SetSelection(SEL_SCOPE_VTRACK | SEL_SCOPE_HCHAN);
	InvalidateCursor();
}

void CFamiTrackerView::OnEditSelecttrack()		// // //
{
	m_pPatternEditor->SetSelection(SEL_SCOPE_VTRACK | SEL_SCOPE_HFRAME);
	InvalidateCursor();
}

void CFamiTrackerView::OnTrackerPlayrow()
{
	if (auto *pSoundGen = Env.GetSoundGenerator(); pSoundGen && !pSoundGen->IsPlaying()) {		// // //
		pSoundGen->PlaySingleRow(static_cast<CMainFrame*>(GetParentFrame())->GetSelectedTrack());
		m_pPatternEditor->MoveDown(1);
		InvalidateCursor();
	}
}

void CFamiTrackerView::OnEditCopyAsVolumeSequence()		// // //
{
	auto str = m_pPatternEditor->GetVolumeColumn();
	(void)CClipboard::CopyToClipboard(this, CF_UNICODETEXT, CStringClipData<wchar_t> {std::wstring {conv::to_sv(str)}});
}

void CFamiTrackerView::OnEditCopyAsText()		// // //
{
	auto str = m_pPatternEditor->GetSelectionAsText();
	(void)CClipboard::CopyToClipboard(this, CF_UNICODETEXT, CStringClipData<wchar_t> {std::wstring {conv::to_sv(str)}});
}

void CFamiTrackerView::OnEditCopyAsPPMCK()		// // //
{
	auto str = m_pPatternEditor->GetSelectionAsPPMCK();
	(void)CClipboard::CopyToClipboard(this, CF_UNICODETEXT, CStringClipData<wchar_t> {std::wstring {conv::to_sv(str)}});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UI updates
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::OnInitialUpdate()
{
	//
	// Called when the view is first attached to a document,
	// when a file is loaded or new document is created.
	//

	CFamiTrackerDoc* pDoc = GetDocument();
	CFamiTrackerModule *pModule = GetModuleData();

	CMainFrame *pMainFrame = GetMainFrame();		// // //

	CFrameEditor *pFrameEditor = GetFrameEditor();

	TRACE(L"View: OnInitialUpdate (%s)\n", pDoc->GetTitle());

	// Setup order window
	pFrameEditor->AssignView(*this);		// // //
	m_pPatternEditor->SetDocument(pModule, this);

	// Always start with first track
	pMainFrame->SelectTrack(0);
	UpdateSongView();		// // //

	// Notify the pattern view about new document & view
	m_pPatternEditor->ResetCursor();
	pFrameEditor->ResetCursor();		// // //

	// Update mainframe with new document settings
	pMainFrame->UpdateInstrumentList();
	pMainFrame->SetSongInfo(*pModule);		// // //
	pMainFrame->UpdateTrackBox();
	pMainFrame->DisplayOctave();
	pMainFrame->UpdateControls();
	pMainFrame->ResetUndo();
	pMainFrame->ResizeFrameWindow();

	// Fetch highlight
	m_pPatternEditor->SetHighlight(pModule->GetHighlight(0));		// // //
	pMainFrame->SetHighlightRows(pModule->GetHighlight(0));

	// Follow mode
	SetFollowMode(Env.GetSettings()->bFollowMode);

	// Setup speed/tempo (TODO remove?)
	Env.GetSoundGenerator()->AssignModule(*pModule);		// // //
	Env.GetSoundGenerator()->ResetState();
	Env.GetSoundGenerator()->ResetTempo();
	Env.GetSoundGenerator()->SetMeterDecayRate(Env.GetSettings()->bFastMeterDecayRate ? decay_rate_t::Fast : decay_rate_t::Slow);		// // // 050B
	Env.GetSoundGenerator()->DocumentPropertiesChanged(pDoc);		// // //

	// Default
	SetInstrument(0);

	UnmuteAllChannels();		// // //

	UpdateNoteQueues();		// // //

	// Draw screen
	m_pPatternEditor->InvalidateBackground();
	m_pPatternEditor->InvalidatePatternData();
	RedrawPatternEditor();

	pFrameEditor->InvalidateFrameData();
	RedrawFrameEditor();

	if (Env.GetMainFrame()->IsWindowVisible())		// // //
		SetFocus();

	// Display comment box
	if (pModule->ShowsCommentOnOpen())		// // //
		pMainFrame->PostMessageW(WM_COMMAND, ID_MODULE_COMMENTS);

	// Call OnUpdate
	//CView::OnInitialUpdate();
}

void CFamiTrackerView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
	// Called when the document has changed
	CMainFrame *pMainFrm = GetMainFrame();		// // //

	// Handle new flags
	switch (lHint) {
	// Track has been added, removed or changed
	case UPDATE_TRACK:
		if (Env.GetSoundGenerator()->IsPlaying())
			Env.GetSoundGenerator()->StopPlayer();		// // //
		UpdateSongView();		// // //
		pMainFrm->UpdateTrackBox();
		m_pPatternEditor->InvalidateBackground();
		m_pPatternEditor->InvalidatePatternData();
		m_pPatternEditor->InvalidateHeader();
		RedrawPatternEditor();
		break;
	// Pattern data has been edited
	case UPDATE_PATTERN:
		m_pPatternEditor->InvalidatePatternData();
		RedrawPatternEditor();
		break;
	// Frame data has been edited
	case UPDATE_FRAME:
		InvalidateFrameEditor();

		m_pPatternEditor->InvalidatePatternData();
		RedrawPatternEditor();
		pMainFrm->UpdateBookmarkList();		// // //
		break;
	// Instrument has been added / removed
	case UPDATE_INSTRUMENT:
		pMainFrm->UpdateInstrumentList();
		m_pPatternEditor->InvalidatePatternData();
		RedrawPatternEditor();
		break;
	// Module properties has changed (including channel count)
	case UPDATE_PROPERTIES:
		// Old
		UpdateSongView();		// // //
		m_pPatternEditor->ResetCursor();
		//m_pPatternEditor->Modified();
		pMainFrm->ResetUndo();
		pMainFrm->ResizeFrameWindow();

		m_pPatternEditor->InvalidateBackground();	// Channel count might have changed, recalculated pattern layout
		m_pPatternEditor->InvalidatePatternData();
		m_pPatternEditor->InvalidateHeader();
		RedrawPatternEditor();
		UpdateNoteQueues();		// // //
		break;
	// Row highlight option has changed
	case UPDATE_HIGHLIGHT:
		m_pPatternEditor->SetHighlight(GetModuleData()->GetHighlight(pMainFrm->GetSelectedTrack()));		// // //
		m_pPatternEditor->InvalidatePatternData();
		RedrawPatternEditor();
		break;
	// Effect columns has changed
	case UPDATE_COLUMNS:
		m_pPatternEditor->InvalidateBackground();	// Recalculate pattern layout
		m_pPatternEditor->InvalidateHeader();
		m_pPatternEditor->InvalidatePatternData();
		RedrawPatternEditor();
		break;
	// Document is closing
	case UPDATE_CLOSE:
		// Old
		pMainFrm->CloseGrooveSettings();		// // //
		pMainFrm->CloseBookmarkSettings();		// // //
		pMainFrm->UpdateBookmarkList();		// // //
		pMainFrm->CloseInstrumentEditor();
		break;
	}
}

void CFamiTrackerView::TrackChanged(unsigned int Track)
{
	// Called when the selected track has changed
	CMainFrame *pMainFrm = GetMainFrame();		// // //

	UpdateSongView();		// // //
	SetMarker(-1, -1);		// // //
	m_pPatternEditor->ResetCursor();
	m_pPatternEditor->InvalidatePatternData();
	m_pPatternEditor->InvalidateBackground();
	m_pPatternEditor->InvalidateHeader();
	RedrawPatternEditor();

	pMainFrm->UpdateTrackBox();

	InvalidateFrameEditor();
	RedrawFrameEditor();
}

// GUI elements updates

void CFamiTrackerView::OnUpdateEditInstrumentMask(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bMaskInstrument ? 1 : 0);
}

void CFamiTrackerView::OnUpdateEditVolumeMask(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bMaskVolume ? 1 : 0);
}

void CFamiTrackerView::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!m_pPatternEditor->IsSelecting() ? 0 : 1);
}

void CFamiTrackerView::OnUpdateEditCut(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!m_pPatternEditor->IsSelecting() ? 0 : 1);
}

void CFamiTrackerView::OnUpdateEditPaste(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(IsClipboardAvailable() ? 1 : 0);
}

void CFamiTrackerView::OnUpdateEditDelete(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!m_pPatternEditor->IsSelecting() ? 0 : 1);
}

void CFamiTrackerView::OnUpdateTrackerEdit(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bEditEnable ? 1 : 0);
}

void CFamiTrackerView::OnEditPasteSpecialCursor()		// // //
{
	m_iPastePos = paste_pos_t::CURSOR;
}

void CFamiTrackerView::OnEditPasteSpecialSelection()
{
	m_iPastePos = paste_pos_t::SELECTION;
}

void CFamiTrackerView::OnEditPasteSpecialFill()
{
	m_iPastePos = paste_pos_t::FILL;
}

void CFamiTrackerView::OnUpdatePasteSpecial(CCmdUI *pCmdUI)
{
	if (pCmdUI->m_pMenu != NULL)
		pCmdUI->m_pMenu->CheckMenuRadioItem(ID_PASTESPECIAL_CURSOR, ID_PASTESPECIAL_FILL, ID_PASTESPECIAL_CURSOR + (unsigned)m_iPastePos, MF_BYCOMMAND);
}

void CFamiTrackerView::OnUpdateDisableWhilePlaying(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!Env.GetSoundGenerator()->IsPlaying());
}

void CFamiTrackerView::SetMarker(int Frame, int Row)		// // // 050B
{
	m_iMarkerFrame = Frame;
	m_iMarkerRow = Row;
	m_pPatternEditor->InvalidatePatternData();
	RedrawPatternEditor();
	GetFrameEditor()->InvalidateFrameData();
	RedrawFrameEditor();
}

bool CFamiTrackerView::IsMarkerValid() const		// // //
{
	if (m_iMarkerFrame < 0 || m_iMarkerRow < 0)
		return false;

	const CConstSongView *pSongView = GetSongView();

	if (m_iMarkerFrame >= static_cast<int>(pSongView->GetSong().GetFrameCount()))
		return false;
	if (m_iMarkerRow >= (int)pSongView->GetFrameLength(m_iMarkerFrame))
		return false;
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tracker playing routines
////////////////////////////////////////////////////////////////////////////////////////////////////////////

// // //
void CFamiTrackerView::PlayerPlayNote(stChannelID Channel, const stChanNote &pNote)
{
	// Callback from sound thread
	if (m_bSwitchToInstrument && pNote.Instrument < MAX_INSTRUMENTS && pNote.Note != note_t::none &&
		GetSongView()->GetChannelOrder().GetChannelIndex(Channel) == GetSelectedChannel())
		m_iSwitchToInstrument = pNote.Instrument;
}

unsigned int CFamiTrackerView::GetSelectedFrame() const
{
	return m_pPatternEditor->GetFrame();
}

unsigned int CFamiTrackerView::GetSelectedChannel() const
{
	return m_pPatternEditor->GetChannel();
}

stChannelID CFamiTrackerView::TranslateChannel(unsigned Index) const {		// // //
	return GetSongView()->GetChannelOrder().TranslateChannel(Index);
}

stChannelID CFamiTrackerView::GetSelectedChannelID() const {		// // //
	return TranslateChannel(GetSelectedChannel());
}

CTrackerChannel &CFamiTrackerView::GetTrackerChannel(std::size_t Index) const {		// // //
	return *Env.GetSoundGenerator()->GetTrackerChannel(TranslateChannel(Index));
}

CSongView *CFamiTrackerView::GetSongView() {		// // //
	return song_view_.get();
}

const CConstSongView *CFamiTrackerView::GetSongView() const {		// // //
	return song_view_.get();
}

CFamiTrackerModule *CFamiTrackerView::GetModuleData() {
	return GetDocument()->GetModule();
}

const CFamiTrackerModule *CFamiTrackerView::GetModuleData() const {
	return GetDocument()->GetModule();
}

unsigned int CFamiTrackerView::GetSelectedRow() const
{
	return m_pPatternEditor->GetRow();
}

// // //
std::pair<unsigned, unsigned> CFamiTrackerView::GetSelectedPos() const {
	return std::make_pair(GetSelectedFrame(), GetSelectedRow());
}

CPlayerCursor CFamiTrackerView::GetPlayerCursor(play_mode_t Mode) const {		// // //
	const unsigned Track = static_cast<CMainFrame*>(GetParentFrame())->GetSelectedTrack();
	const CSongData &song = *GetModuleData()->GetSong(Track);

	switch (Mode) {
	case play_mode_t::Frame:
		return CPlayerCursor {song, Track, GetSelectedFrame(), 0};
	case play_mode_t::RepeatFrame: {
		auto cur = CPlayerCursor {song, Track, GetSelectedFrame(), 0};
		cur.EnableFrameLoop();
		return cur;
	}
	case play_mode_t::Cursor:
		return CPlayerCursor {song, Track, GetSelectedFrame(), GetSelectedRow()};
	case play_mode_t::Marker:		// // // 050B
		if (GetMarkerFrame() != -1 && GetMarkerRow() != -1)
			return CPlayerCursor {song, Track,
				static_cast<unsigned>(GetMarkerFrame()), static_cast<unsigned>(GetMarkerRow())};
		break;
	case play_mode_t::Song:
		return CPlayerCursor {song, Track};
	}

	DEBUG_BREAK();
	return CPlayerCursor {song, Track};
}

void CFamiTrackerView::SetFollowMode(bool Mode)
{
	m_bFollowMode = Mode;
	m_pPatternEditor->SetFollowMove(Mode);
}

bool CFamiTrackerView::GetFollowMode() const
{
	return m_bFollowMode;
}

void CFamiTrackerView::SetCompactMode(bool Mode)		// // //
{
	m_bCompactMode = Mode;
	m_pPatternEditor->SetCompactMode(Mode);
	InvalidateCursor();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::MoveCursorNextChannel()
{
	m_pPatternEditor->NextChannel();
	InvalidateCursor();
}

void CFamiTrackerView::MoveCursorPrevChannel()
{
	m_pPatternEditor->PreviousChannel();
	InvalidateCursor();
}

void CFamiTrackerView::SelectNextFrame()
{
	m_pPatternEditor->NextFrame();
	InvalidateFrameEditor();
}

void CFamiTrackerView::SelectPrevFrame()
{
	m_pPatternEditor->PreviousFrame();
	InvalidateFrameEditor();
}

void CFamiTrackerView::SelectFrame(unsigned int Frame)
{
	ASSERT(Frame < MAX_FRAMES);
	m_pPatternEditor->MoveToFrame(Frame);
	InvalidateCursor();
}

void CFamiTrackerView::SelectRow(unsigned int Row)
{
	ASSERT(Row < MAX_PATTERN_LENGTH);
	m_pPatternEditor->MoveToRow(Row);
	InvalidateCursor();
}

void CFamiTrackerView::SelectChannel(unsigned int Channel)
{
	m_pPatternEditor->MoveToChannel(Channel);
	InvalidateCursor();
}

// // // TODO: move these to CMainFrame?

void CFamiTrackerView::OnBookmarksToggle()
{
	if ((Env.GetSoundGenerator()->IsPlaying() && m_bFollowMode) || !m_bEditEnable)
		return;

	const int Frame = GetSelectedFrame();
	const int Row = GetSelectedRow();

	CBookmarkCollection &Col = static_cast<CMainFrame *>(GetParentFrame())->GetCurrentSong()->GetBookmarks();
	if (Col.FindAt(Frame, Row) != nullptr)
		Col.RemoveAt(Frame, Row);
	else {
		auto pMark = std::make_unique<CBookmark>(Frame, Row);
		pMark->m_Highlight.First = pMark->m_Highlight.Second = -1;
		pMark->m_bPersist = false;
		pMark->m_sName = "Bookmark " + std::to_string(Col.GetCount() + 1);
		Col.AddBookmark(std::move(pMark));
	}

	static_cast<CMainFrame*>(GetParentFrame())->UpdateBookmarkList();
	SetFocus();
	GetDocument()->ModifyIrreversible();
	m_pPatternEditor->InvalidatePatternData();
	RedrawPatternEditor();
	GetFrameEditor()->InvalidateFrameData();
	RedrawFrameEditor();
}

void CFamiTrackerView::OnBookmarksNext()
{
	if (Env.GetSoundGenerator()->IsPlaying() && m_bFollowMode)
		return;

	CMainFrame *pMainFrame = static_cast<CMainFrame*>(GetParentFrame());
	CBookmarkCollection &Col = static_cast<CMainFrame *>(GetParentFrame())->GetCurrentSong()->GetBookmarks();

	if (CBookmark *pMark = Col.FindNext(GetSelectedFrame(), GetSelectedRow())) {
		SelectFrame(pMark->m_iFrame);
		SelectRow(pMark->m_iRow);
		pMainFrame->SetMessageText(AfxFormattedW(IDS_BOOKMARK_FORMAT, conv::to_wide(pMark->m_sName).data(),
			(pMark->m_Highlight.First != -1) ? FormattedW(L"%i", pMark->m_Highlight.First) : CStringW(L"None"),
			(pMark->m_Highlight.Second != -1) ? FormattedW(L"%i", pMark->m_Highlight.Second) : CStringW(L"None")));
		pMainFrame->UpdateBookmarkList(Col.GetBookmarkIndex(pMark));
		SetFocus();
	}
	else {
		MessageBeep(MB_ICONINFORMATION);
		pMainFrame->SetMessageText(IDS_BOOKMARK_EMPTY);
	}
}

void CFamiTrackerView::OnBookmarksPrevious()
{
	if (Env.GetSoundGenerator()->IsPlaying() && m_bFollowMode)
		return;

	CMainFrame *pMainFrame = static_cast<CMainFrame*>(GetParentFrame());
	CBookmarkCollection &Col = static_cast<CMainFrame *>(GetParentFrame())->GetCurrentSong()->GetBookmarks();

	if (CBookmark *pMark = Col.FindPrevious(GetSelectedFrame(), GetSelectedRow())) {
		SelectFrame(pMark->m_iFrame);
		SelectRow(pMark->m_iRow);
		pMainFrame->SetMessageText(AfxFormattedW(IDS_BOOKMARK_FORMAT, conv::to_wide(pMark->m_sName).data(),
			(pMark->m_Highlight.First != -1) ? FormattedW(L"%i", pMark->m_Highlight.First) : CStringW(L"None"),
			(pMark->m_Highlight.Second != -1) ? FormattedW(L"%i", pMark->m_Highlight.Second) : CStringW(L"None")));
		pMainFrame->UpdateBookmarkList(Col.GetBookmarkIndex(pMark));
		SetFocus();
	}
	else {
		MessageBeep(MB_ICONINFORMATION);
		pMainFrame->SetMessageText(IDS_BOOKMARK_EMPTY);
	}
}

void CFamiTrackerView::OnEditSplitKeyboard()		// // //
{
	CSplitKeyboardDlg dlg;
	dlg.m_bSplitEnable = m_iSplitNote != -1;
	dlg.m_iSplitNote = m_iSplitNote;
	dlg.m_iSplitChannel = m_iSplitChannel;
	dlg.m_iSplitInstrument = m_iSplitInstrument;
	dlg.m_iSplitTranspose = m_iSplitTranspose;

	if (dlg.DoModal() == IDOK) {
		if (dlg.m_bSplitEnable) {
			m_iSplitNote = dlg.m_iSplitNote;
			m_iSplitChannel = dlg.m_iSplitChannel;
			m_iSplitInstrument = dlg.m_iSplitInstrument;
			m_iSplitTranspose = dlg.m_iSplitTranspose;
		}
		else {
			m_iSplitNote = -1;
			m_iSplitChannel = { };
			m_iSplitInstrument = MAX_INSTRUMENTS;
			m_iSplitTranspose = 0;
		}
	}
}

void CFamiTrackerView::ToggleChannel(stChannelID Channel)
{
	SetChannelMute(Channel, !IsChannelMuted(Channel));
	InvalidateHeader();
}

void CFamiTrackerView::SoloChannel(stChannelID Channel)
{
	const CChannelOrder &order = GetSongView()->GetChannelOrder();		// // //

	if (IsChannelSolo(Channel))
		order.ForeachChannel([&] (stChannelID i) { // Revert channels
			SetChannelMute(i, false);
		});
	else
		order.ForeachChannel([&] (stChannelID i) { // Solo selected channel
			SetChannelMute(i, i != Channel);
		});

	InvalidateHeader();
}

void CFamiTrackerView::ToggleChip(stChannelID Channel)		// // //
{
	const CChannelOrder &order = GetSongView()->GetChannelOrder();
	bool shouldMute = false;

	order.ForeachChannel([&] (stChannelID i) {
		if (!shouldMute && i.Chip == Channel.Chip && !IsChannelMuted(i))
			shouldMute = true;
	});

	order.ForeachChannel([&] (stChannelID i) {
		if (i.Chip == Channel.Chip)
			SetChannelMute(i, shouldMute);
	});

	InvalidateHeader();
}

void CFamiTrackerView::SoloChip(stChannelID Channel)		// // //
{
	const CChannelOrder &order = GetSongView()->GetChannelOrder();

	if (IsChipSolo(Channel.Chip))
		order.ForeachChannel([&] (stChannelID i) {
			SetChannelMute(i, false);
		});
	else
		order.ForeachChannel([&] (stChannelID i) {
			SetChannelMute(i, i.Chip != Channel.Chip);
		});

	InvalidateHeader();
}

void CFamiTrackerView::UnmuteAllChannels()
{
	Env.GetSoundChipService()->ForeachTrack([&] (stChannelID id) {		// // //
		SetChannelMute(id, false);
	});

	InvalidateHeader();
}

bool CFamiTrackerView::IsChannelSolo(stChannelID Channel) const		// // //
{
	bool solo = true;

	GetSongView()->GetChannelOrder().ForeachChannel([&] (stChannelID i) {
		if (solo && !IsChannelMuted(i) && i != Channel)
			solo = false;
	});

	return solo;
}

bool CFamiTrackerView::IsChipSolo(sound_chip_t Chip) const		// // //
{
	bool solo = true;

	GetSongView()->GetChannelOrder().ForeachChannel([&] (stChannelID i) {
		if (solo && !IsChannelMuted(i) && i.Chip != Chip)
			solo = false;
	});

	return solo;
}

void CFamiTrackerView::SetChannelMute(stChannelID Channel, bool bMute)
{
	if (IsChannelMuted(Channel) != bMute) {		// // //
		HaltNoteSingle(GetSongView()->GetChannelOrder().GetChannelIndex(Channel));
//		if (bMute)
//			m_pNoteQueue->MuteChannel(Channel);
//		else
//			m_pNoteQueue->UnmuteChannel(Channel);
	}
	Env.GetSoundGenerator()->SetChannelMute(Channel, bMute);		// // //
}

bool CFamiTrackerView::IsChannelMuted(stChannelID Channel) const
{
	return Env.GetSoundGenerator()->IsChannelMuted(Channel);
}

void CFamiTrackerView::SetInstrument(int Instrument)
{
	CMainFrame *pMainFrm = GetMainFrame();		// // //

	if (Instrument >= MAX_INSTRUMENTS)		// // // 050B
		return; // may be called by emptying inst field or using &&

	pMainFrm->SelectInstrument(Instrument);
	m_LastNote.Instrument = GetInstrument(); // Gets actual selected instrument //  Instrument;
}

unsigned int CFamiTrackerView::GetInstrument() const
{
	CMainFrame *pMainFrm = GetMainFrame();		// // //
	return pMainFrm->GetSelectedInstrumentIndex();
}

unsigned int CFamiTrackerView::GetSplitInstrument() const		// // //
{
	return m_iSplitInstrument;
}

void CFamiTrackerView::StepDown()
{
	// Update pattern length in case it has changed
	m_pPatternEditor->UpdatePatternLength();

	if (m_iInsertKeyStepping)
		m_pPatternEditor->MoveDown(m_iInsertKeyStepping);

	InvalidateCursor();
}

void CFamiTrackerView::InsertNote(const stChanNote &Cell) {		// // //
	// Inserts a note
	m_LastNote.Note = Cell.Note;		// // //
	m_LastNote.Octave = Cell.Octave;		// // //

	auto pAction = std::make_unique<CPActionEditNote>(Cell);		// // //
	auto &Action = *pAction; // TODO: remove
	if (AddAction(std::move(pAction))) {
		const CSettings *pSettings = Env.GetSettings();
		if (m_pPatternEditor->GetColumn() == cursor_column_t::NOTE && !Env.GetSoundGenerator()->IsPlaying() && m_iInsertKeyStepping > 0 && !pSettings->Midi.bMidiMasterSync) {
			StepDown();
			Action.SaveRedoState(static_cast<CMainFrame &>(*GetParentFrame()));		// // //
		}
	}
}

stChanNote CFamiTrackerView::GetInputNote(note_t Note, int Octave, std::size_t Index, int Velocity) {		// // //
	stChannelID Channel = TranslateChannel(Index);		// // //
	const int Frame = GetSelectedFrame();
	const int Row = GetSelectedRow();

	stChanNote Cell = GetSongView()->GetPatternOnFrame(Index, Frame).GetNoteOn(Row);		// // //

	Cell.Note = Note;

	if (Cell.Note != note_t::halt && Cell.Note != note_t::release) {
		Cell.Octave = Octave;

		if (!m_bMaskInstrument && Cell.Instrument != HOLD_INSTRUMENT)		// // // 050B
			Cell.Instrument = GetInstrument();

		if (!m_bMaskVolume) {
			Cell.Vol = m_LastNote.Vol;
			if (Velocity < 128)
				Cell.Vol = Velocity * MAX_VOLUME / 128;
		}
		if (Cell.Note == note_t::echo) {
			if (Cell.Octave > ECHO_BUFFER_LENGTH - 1)
				Cell.Octave = ECHO_BUFFER_LENGTH - 1;
		}
		else if (Cell.Note != note_t::none) {		// // //
			if (IsAPUNoise(Channel)) {		// // //
				unsigned int MidiNote = (Cell.ToMidiNote() % 16) + 16;
				Cell.Octave = ft0cc::doc::oct_from_midi(MidiNote);
				Cell.Note = ft0cc::doc::pitch_from_midi(MidiNote);
			}
			else
				SplitKeyboardAdjust(Cell, Channel);
		}
	}

	// Quantization
	if (Env.GetSettings()->Midi.bMidiMasterSync)
		if (int Delay = Env.GetMIDI()->GetQuantization(); Delay > 0)
			Cell.Effects[0] = {effect_t::DELAY, static_cast<uint8_t>(Delay)};

	return Cell;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Note playing routines
//////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::PlayNote(std::size_t Index, int MidiNote, unsigned int Velocity) const {		// // //
	if (MidiNote < 0 || MidiNote >= NOTE_COUNT)
		return;

	// Play a note in a channel
	stChanNote NoteData;		// // //

	NoteData.Note		= ft0cc::doc::pitch_from_midi(MidiNote);
	NoteData.Octave		= ft0cc::doc::oct_from_midi(MidiNote);
	NoteData.Instrument	= GetInstrument();
	if (Env.GetSettings()->Midi.bMidiVelocity)
		NoteData.Vol = Velocity / 8;
/*
	if (Env.GetSettings()->General.iEditStyle == edit_style_t::IT)
		NoteData.Instrument	= m_LastNote.Instrument;
	else
		NoteData.Instrument	= GetInstrument();
*/
	const CChannelOrder &order = GetSongView()->GetChannelOrder();
	const CSongData &song = GetSongView()->GetSong();

	stChannelID Channel = SplitAdjustChannel(TranslateChannel(Index), NoteData);
	if (order.HasChannel(Channel)) {
		stChannelID ret = m_pNoteQueue->Trigger(MidiNote, Channel);
		if (order.HasChannel(ret)) {
			SplitKeyboardAdjust(NoteData, ret);
			Env.GetSoundGenerator()->QueueNote(ret, NoteData, NOTE_PRIO_2);		// // //
			Env.GetSoundGenerator()->ForceReloadInstrument(ret);		// // //
		}
	}

	if (Env.GetSettings()->General.bPreviewFullRow) {
		int Frame = GetSelectedFrame();
		int Row = GetSelectedRow();

		order.ForeachChannel([&] (stChannelID i) {
			if (!IsChannelMuted(i) && i != Channel)
				Env.GetSoundGenerator()->QueueNote(i, song.GetActiveNote(i, Frame, Row),
					(i == Channel) ? NOTE_PRIO_2 : NOTE_PRIO_1);		// // //
		});
	}
}

void CFamiTrackerView::ReleaseNote(std::size_t Index, int MidiNote) const {
	if (MidiNote < 0 || MidiNote >= NOTE_COUNT)
		return;

	// Releases a channel
	stChanNote NoteData;		// // //

	NoteData.Note = note_t::release;
	NoteData.Instrument = GetInstrument();

	stChannelID Channel = SplitAdjustChannel(TranslateChannel(Index), NoteData);
	const CChannelOrder &order = GetSongView()->GetChannelOrder();
	if (order.HasChannel(Channel)) {
		stChannelID ch = m_pNoteQueue->Cut(MidiNote, Channel);
//		stChannelID ch = m_pNoteQueue->Release(MidiNote, Channel);
		if (order.HasChannel(ch))
			Env.GetSoundGenerator()->QueueNote(ch, NoteData, NOTE_PRIO_2);

		if (Env.GetSettings()->General.bPreviewFullRow) {
			NoteData.Note = note_t::halt;
			NoteData.Instrument = MAX_INSTRUMENTS;

			order.ForeachChannel([&] (stChannelID i) {
				if (i != ch)
					Env.GetSoundGenerator()->QueueNote(i, NoteData, NOTE_PRIO_1);
			});
		}
	}
}

void CFamiTrackerView::HaltNote(std::size_t Index, int MidiNote) const {
	if (MidiNote < 0 || MidiNote >= NOTE_COUNT)
		return;

	// Halts a channel
	stChanNote NoteData;		// // //

	NoteData.Note = note_t::halt;
	NoteData.Instrument = GetInstrument();

	stChannelID Channel = SplitAdjustChannel(TranslateChannel(Index), NoteData);
	const CChannelOrder &order = GetSongView()->GetChannelOrder();
	if (order.HasChannel(Channel)) {
		stChannelID ch = m_pNoteQueue->Cut(MidiNote, Channel);
		if (order.HasChannel(ch))
			Env.GetSoundGenerator()->QueueNote(ch, NoteData, NOTE_PRIO_2);

		if (Env.GetSettings()->General.bPreviewFullRow) {
			NoteData.Instrument = MAX_INSTRUMENTS;

			order.ForeachChannel([&] (stChannelID i) {
				if (i != ch)
					Env.GetSoundGenerator()->QueueNote(i, NoteData, NOTE_PRIO_1);
			});
		}
	}
}

void CFamiTrackerView::HaltNoteSingle(std::size_t Index) const
{
	// Halts one single channel only
	stChanNote NoteData;		// // //

	NoteData.Note = note_t::halt;
	NoteData.Instrument = GetInstrument();

	stChannelID Channel = SplitAdjustChannel(TranslateChannel(Index), NoteData); // ?
	const CChannelOrder &order = GetSongView()->GetChannelOrder();
	if (order.HasChannel(Channel)) {
		for (const auto &i : m_pNoteQueue->StopChannel(Channel)) {
			if (order.HasChannel(i))
				Env.GetSoundGenerator()->QueueNote(i, NoteData, NOTE_PRIO_2);
		}
	}

	if (Env.GetSoundGenerator()->IsPlaying())
		Env.GetSoundGenerator()->QueueNote(Channel, NoteData, NOTE_PRIO_2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MIDI note handling functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
namespace {

void FixNoise(unsigned int &MidiNote, unsigned int &Octave, unsigned int &Note)
{
	// NES noise channel
	MidiNote = (MidiNote % 16) + 16;
	Octave = ft0cc::doc::oct_from_midi(MidiNote);
	Note = ft0cc::doc::pitch_from_midi(MidiNote);
}

} // namespace
*/

// Play a note
void CFamiTrackerView::TriggerMIDINote(std::size_t Index, unsigned int MidiNote, unsigned int Velocity, bool Insert)
{
	if (MidiNote >= NOTE_COUNT) MidiNote = NOTE_COUNT - 1;		// // //

	// Play a MIDI note
	unsigned int Octave = ft0cc::doc::oct_from_midi(MidiNote);
	note_t Note = ft0cc::doc::pitch_from_midi(MidiNote);
//	if (TranslateChannel(Channel) == stChannelID::NOISE)
//		FixNoise(MidiNote, MidiNote);

	if (!Env.GetSettings()->Midi.bMidiVelocity)
		Velocity = Env.GetSettings()->General.iEditStyle == edit_style_t::IT ? m_LastNote.Vol * 8 : 127;

	if (!(Env.GetSoundGenerator()->IsPlaying() && m_bEditEnable && !m_bFollowMode))		// // //
		PlayNote(Index, MidiNote, Velocity);

	if (Insert)
		InsertNote(GetInputNote(Note, Octave, Index, Velocity + 1));

	if (Env.GetSettings()->Midi.bMidiArpeggio) {		// // //
		m_pArpeggiator->TriggerNote(MidiNote);
		UpdateArpDisplay();
	}

	m_iLastMIDINote = MidiNote;

	TRACE(L"%i: Trigger note %i on channel %i\n", GetTickCount(), MidiNote, Index);
}

// Cut the currently playing note
void CFamiTrackerView::CutMIDINote(std::size_t Index, unsigned int MidiNote, bool InsertCut)
{
	if (MidiNote >= NOTE_COUNT) MidiNote = NOTE_COUNT - 1;		// // //

	// Cut a MIDI note
//	if (TranslateChannel(Channel) == stChannelID::NOISE)
//		FixNoise(MidiNote, MidiNote);

	// Cut note
	if (!(Env.GetSoundGenerator()->IsPlaying() && m_bEditEnable && !m_bFollowMode))		// // //
		if (!m_bEditEnable || m_iLastMIDINote == MidiNote)
			HaltNote(Index, MidiNote);

	if (InsertCut)
		InsertNote(GetInputNote(note_t::halt, 0, Index, 0));

	if (Env.GetSettings()->Midi.bMidiArpeggio) {		// // //
		m_pArpeggiator->CutNote(MidiNote);
		UpdateArpDisplay();
	}

	// IT-mode, cut note on cuts
	if (Env.GetSettings()->General.iEditStyle == edit_style_t::IT)
		HaltNote(Index, MidiNote);		// // //

	TRACE(L"%i: Cut note %i on channel %i\n", GetTickCount(), MidiNote, Index);
}

// Release the currently playing note
void CFamiTrackerView::ReleaseMIDINote(std::size_t Index, unsigned int MidiNote, bool InsertCut)
{
	if (MidiNote >= NOTE_COUNT) MidiNote = NOTE_COUNT - 1;		// // //

	// Release a MIDI note
//	if (TranslateChannel(Channel) == stChannelID::NOISE)
//		FixNoise(MidiNote, MidiNote);

	// Cut note
	if (!(Env.GetSoundGenerator()->IsPlaying() && m_bEditEnable && !m_bFollowMode))		// // //
		if (!m_bEditEnable || m_iLastMIDINote == MidiNote)
			ReleaseNote(Index, MidiNote);

	if (InsertCut)
		InsertNote(GetInputNote(note_t::release, 0, Index, 0));

	if (Env.GetSettings()->Midi.bMidiArpeggio) {		// // //
		m_pArpeggiator->ReleaseNote(MidiNote);
		UpdateArpDisplay();
	}

	// IT-mode, release note
	if (Env.GetSettings()->General.iEditStyle == edit_style_t::IT)
		ReleaseNote(Index, MidiNote);		// // //

	TRACE(L"%i: Release note %i on channel %i\n", GetTickCount(), MidiNote, Index);
}

void CFamiTrackerView::UpdateArpDisplay()
{
	if (auto str = m_pArpeggiator->GetStateString(); !str.empty())		// // //
		GetParentFrame()->SetMessageText(conv::to_wide(str).data());
}

void CFamiTrackerView::UpdateNoteQueues() {		// // //
	const CConstSongView *pSongView = GetSongView();
	const int Channels = pSongView->GetChannelOrder().GetChannelCount();

	m_pNoteQueue->ClearMaps();

	if (m_bEditEnable || Env.GetSettings()->Midi.bMidiArpeggio)
		pSongView->GetChannelOrder().ForeachChannel([&] (stChannelID i) {
			m_pNoteQueue->AddMap({i});
		});
	else {
		m_pNoteQueue->AddMap({apu_subindex_t::triangle});
		m_pNoteQueue->AddMap({apu_subindex_t::noise});
		m_pNoteQueue->AddMap({apu_subindex_t::dpcm});

		CSoundChipSet chips = GetModuleData()->GetSoundChipSet();

		if (chips.ContainsChip(sound_chip_t::VRC6)) {
			m_pNoteQueue->AddMap({vrc6_subindex_t::pulse1, vrc6_subindex_t::pulse2});
			m_pNoteQueue->AddMap({vrc6_subindex_t::sawtooth});
		}
		if (chips.ContainsChip(sound_chip_t::VRC7))
			m_pNoteQueue->AddMap({
				vrc7_subindex_t::ch1, vrc7_subindex_t::ch2, vrc7_subindex_t::ch3,
				vrc7_subindex_t::ch4, vrc7_subindex_t::ch5, vrc7_subindex_t::ch6,
			});
		if (chips.ContainsChip(sound_chip_t::FDS))
			m_pNoteQueue->AddMap({fds_subindex_t::wave});
		if (chips.ContainsChip(sound_chip_t::MMC5))
			m_pNoteQueue->AddMap({apu_subindex_t::pulse1, apu_subindex_t::pulse2, mmc5_subindex_t::pulse1, mmc5_subindex_t::pulse2});
		else
			m_pNoteQueue->AddMap({apu_subindex_t::pulse1, apu_subindex_t::pulse2});
		if (chips.ContainsChip(sound_chip_t::N163)) {
			std::vector<stChannelID> n;
			std::size_t NamcoChannels = GetModuleData()->GetNamcoChannels();
			for (std::size_t i = 0; i < NamcoChannels; ++i)
				n.emplace_back(sound_chip_t::N163, static_cast<std::uint8_t>(i));
			m_pNoteQueue->AddMap(n);
		}
		if (chips.ContainsChip(sound_chip_t::S5B))
			m_pNoteQueue->AddMap({s5b_subindex_t::square1, s5b_subindex_t::square2, s5b_subindex_t::square3});
	}

//	for (int i = 0; i < Channels; ++i)
//		if (IsChannelMuted(i))
//			m_pNoteQueue->MuteChannel(pDoc->GetChannelType(i));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Tracker input routines
//////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// API keyboard handling routines
//

bool CFamiTrackerView::IsShiftPressed() const
{
	return (::GetKeyState(VK_SHIFT) & 0x80) == 0x80;
}

bool CFamiTrackerView::IsControlPressed() const
{
	return (::GetKeyState(VK_CONTROL) & 0x80) == 0x80;
}

void CFamiTrackerView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Called when a key is pressed
	if (GetFocus() != this)
		return;

	auto pFrame = static_cast<CMainFrame*>(GetParentFrame());		// // //
	if (pFrame && pFrame->TypeInstrumentNumber(ConvertKeyToHex(nChar)))
		return;

	if (nChar >= VK_NUMPAD0 && nChar <= VK_NUMPAD9) {
		// Switch instrument
		if (m_pPatternEditor->GetColumn() == cursor_column_t::NOTE) {
			SetInstrument(nChar - VK_NUMPAD0);
			return;
		}
	}

	if ((nChar == VK_ADD || nChar == VK_SUBTRACT) && Env.GetSettings()->General.bHexKeypad)		// // //
		HandleKeyboardInput(nChar);
	else if (!Env.GetSettings()->General.bHexKeypad || !(nChar == VK_RETURN && !(nFlags & KF_EXTENDED))) switch (nChar) {
		case VK_UP:
			OnKeyDirUp();
			break;
		case VK_DOWN:
			OnKeyDirDown();
			break;
		case VK_LEFT:
			OnKeyDirLeft();
			break;
		case VK_RIGHT:
			OnKeyDirRight();
			break;
		case VK_HOME:
			OnKeyHome();
			break;
		case VK_END:
			OnKeyEnd();
			break;
		case VK_PRIOR:
			OnKeyPageUp();
			break;
		case VK_NEXT:
			OnKeyPageDown();
			break;
		case VK_TAB:
			OnKeyTab();
			break;
		case VK_ADD:
			KeyIncreaseAction();
			break;
		case VK_SUBTRACT:
			KeyDecreaseAction();
			break;
		case VK_DELETE:
			OnKeyDelete();
			break;
		case VK_INSERT:
			OnKeyInsert();
			break;
		case VK_BACK:
			OnKeyBackspace();
			break;
		// // //

		// Octaves, unless overridden
		case VK_F2: pFrame->SelectOctave(0); break;
		case VK_F3: pFrame->SelectOctave(1); break;
		case VK_F4: pFrame->SelectOctave(2); break;
		case VK_F5: pFrame->SelectOctave(3); break;
		case VK_F6: pFrame->SelectOctave(4); break;
		case VK_F7: pFrame->SelectOctave(5); break;
		case VK_F8: pFrame->SelectOctave(6); break;
		case VK_F9: pFrame->SelectOctave(7); break;

		default:
			HandleKeyboardInput(nChar);
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CFamiTrackerView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// This is called when a key + ALT is pressed
	if (nChar >= VK_NUMPAD0 && nChar <= VK_NUMPAD9) {
		SetStepping(nChar - VK_NUMPAD0);
		return;
	}

	switch (nChar) {
		case VK_LEFT:
			m_pPatternEditor->MoveChannelLeft();
			InvalidateCursor();
			break;
		case VK_RIGHT:
			m_pPatternEditor->MoveChannelRight();
			InvalidateCursor();
			break;
	}

	CView::OnSysKeyDown(nChar, nRepCnt, nFlags);
}

void CFamiTrackerView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Called when a key is released
	HandleKeyboardNote(nChar, false);
	m_cKeyList[nChar] = 0;

	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

//
// Custom key handling routines
//

void CFamiTrackerView::OnKeyDirUp()
{
	// Move up
	m_pPatternEditor->MoveUp(m_iMoveKeyStepping);
	InvalidateCursor();
}

void CFamiTrackerView::OnKeyDirDown()
{
	// Move down
	m_pPatternEditor->MoveDown(m_iMoveKeyStepping);
	InvalidateCursor();
}

void CFamiTrackerView::OnKeyDirLeft()
{
	// Move left
	m_pPatternEditor->MoveLeft();
	InvalidateCursor();
}

void CFamiTrackerView::OnKeyDirRight()
{
	// Move right
	m_pPatternEditor->MoveRight();
	InvalidateCursor();
}

void CFamiTrackerView::OnKeyTab()
{
	// Move between channels
	bool bShiftPressed = IsShiftPressed();

	if (bShiftPressed)
		m_pPatternEditor->PreviousChannel();
	else
		m_pPatternEditor->NextChannel();

	InvalidateCursor();
}

void CFamiTrackerView::OnKeyPageUp()
{
	// Move page up
	int PageSize = Env.GetSettings()->General.iPageStepSize;
	m_pPatternEditor->MoveUp(PageSize);
	InvalidateCursor();
}

void CFamiTrackerView::OnKeyPageDown()
{
	// Move page down
	int PageSize = Env.GetSettings()->General.iPageStepSize;
	m_pPatternEditor->MoveDown(PageSize);
	InvalidateCursor();
}

void CFamiTrackerView::OnKeyHome()
{
	// Move home
	m_pPatternEditor->OnHomeKey();
	InvalidateCursor();
}

void CFamiTrackerView::OnKeyEnd()
{
	// Move to end
	m_pPatternEditor->OnEndKey();
	InvalidateCursor();
}

void CFamiTrackerView::OnKeyInsert()
{
	// Insert row
	if (PreventRepeat(VK_INSERT, true) || !m_bEditEnable)		// // //
		return;

	if (m_pPatternEditor->IsSelecting())
		AddAction(std::make_unique<CPActionInsertAtSel>());		// // //
	else
		AddAction(std::make_unique<CPActionInsertRow>());
}

void CFamiTrackerView::OnKeyBackspace()
{
	// Pull up row
	if (!m_bEditEnable)		// // //
		return;

	if (m_pPatternEditor->IsSelecting()) {
		AddAction(std::make_unique<CPActionDeleteAtSel>());
	}
	else {
		if (PreventRepeat(VK_BACK, true))
			return;
		auto pAction = std::make_unique<CPActionDeleteRow>(true, true);		// // //
		auto &Action = *pAction; // TODO: remove
		if (AddAction(std::move(pAction))) {
			m_pPatternEditor->MoveUp(1);
			InvalidateCursor();
			Action.SaveRedoState(static_cast<CMainFrame &>(*GetParentFrame()));		// // //
		}
	}
}

void CFamiTrackerView::OnKeyDelete()
{
	// Delete row data
	bool bShiftPressed = IsShiftPressed();

	if (PreventRepeat(VK_DELETE, true) || !m_bEditEnable)		// // //
		return;

	if (m_pPatternEditor->IsSelecting()) {
		OnEditDelete();
	}
	else {
		bool bPullUp = Env.GetSettings()->General.bPullUpDelete || bShiftPressed;
		auto pAction = std::make_unique<CPActionDeleteRow>(bPullUp, false);		// // //
		auto &Action = *pAction; // TODO: remove
		if (AddAction(std::move(pAction)))
			if (!bPullUp) {
				StepDown();
				Action.SaveRedoState(static_cast<CMainFrame &>(*GetParentFrame()));		// // //
			}
	}
}

void CFamiTrackerView::KeyIncreaseAction()
{
	if (!m_bEditEnable)		// // //
		return;

	AddAction(std::make_unique<CPActionScrollField>(1));		// // //
}

void CFamiTrackerView::KeyDecreaseAction()
{
	if (!m_bEditEnable)		// // //
		return;

	AddAction(std::make_unique<CPActionScrollField>(-1));		// // //
}

bool CFamiTrackerView::EditInstrumentColumn(stChanNote &Note, int Key, bool &StepDown, bool &MoveRight, bool &MoveLeft)
{
	edit_style_t EditStyle = Env.GetSettings()->General.iEditStyle;
	const cursor_column_t Column = m_pPatternEditor->GetColumn();		// // //

	if (!m_bEditEnable)
		return false;

	if (CheckClearKey(Key)) {
		SetInstrument(Note.Instrument = MAX_INSTRUMENTS);	// Indicate no instrument selected
		if (EditStyle != edit_style_t::MPT)
			StepDown = true;
		return true;
	}
	else if (CheckRepeatKey(Key)) {
		SetInstrument(Note.Instrument = m_LastNote.Instrument);
		if (EditStyle != edit_style_t::MPT)
			StepDown = true;
		return true;
	}
	else if (Key == 'H') {		// // // 050B
		SetInstrument(Note.Instrument = HOLD_INSTRUMENT);
		if (EditStyle != edit_style_t::MPT)
			StepDown = true;
		return true;
	}

	int Value = ConvertKeyToHex(Key);
	unsigned char Mask, Shift;

	if (Value == -1 && Env.GetSettings()->General.bHexKeypad)		// // //
		Value = ConvertKeyExtra(Key);
	if (Value == -1)
		return false;

	if (Column == cursor_column_t::INSTRUMENT1) {
		Mask = 0x0F;
		Shift = 4;
	}
	else {
		Mask = 0xF0;
		Shift = 0;
	}

	if (Note.Instrument == MAX_INSTRUMENTS || Note.Instrument == HOLD_INSTRUMENT)		// // // 050B
		Note.Instrument = 0;

	switch (EditStyle) {
		case edit_style_t::FT2: // FT2
			Note.Instrument = (Note.Instrument & Mask) | (Value << Shift);
			StepDown = true;
			break;
		case edit_style_t::MPT: // MPT
			Note.Instrument = ((Note.Instrument & 0x0F) << 4) | Value & 0x0F;
			if (Note.Instrument >= MAX_INSTRUMENTS)		// // //
				Note.Instrument &= 0x0F;
			break;
		case edit_style_t::IT: // IT
			Note.Instrument = (Note.Instrument & Mask) | (Value << Shift);
			if (Column == cursor_column_t::INSTRUMENT1)
				MoveRight = true;
			else if (Column == cursor_column_t::INSTRUMENT2) {
				MoveLeft = true;
				StepDown = true;
			}
			break;
	}

	if (Note.Instrument > (MAX_INSTRUMENTS - 1))
		Note.Instrument = (MAX_INSTRUMENTS - 1);
	// // //
	SetInstrument(Note.Instrument);

	return true;
}

bool CFamiTrackerView::EditVolumeColumn(stChanNote &Note, int Key, bool &bStepDown)
{
	edit_style_t EditStyle = Env.GetSettings()->General.iEditStyle;

	if (!m_bEditEnable)
		return false;

	if (CheckClearKey(Key)) {
		Note.Vol = MAX_VOLUME;
		if (EditStyle != edit_style_t::MPT)
			bStepDown = true;
		m_LastNote.Vol = MAX_VOLUME;
		return true;
	}
	else if (CheckRepeatKey(Key)) {
		Note.Vol = m_LastNote.Vol;
		if (EditStyle != edit_style_t::MPT)
			bStepDown = true;
		return true;
	}

	int Value = ConvertKeyToHex(Key);

	if (Value == -1 && Env.GetSettings()->General.bHexKeypad)		// // //
		Value = ConvertKeyExtra(Key);
	if (Value == -1)
		return false;

	m_LastNote.Vol = Note.Vol = Value;		// // //

	if (EditStyle != edit_style_t::MPT)
		bStepDown = true;

	return true;
}

bool CFamiTrackerView::EditEffNumberColumn(stChanNote &Note, unsigned char nChar, int EffectIndex, bool &bStepDown)
{
	edit_style_t EditStyle = Env.GetSettings()->General.iEditStyle;

	if (!m_bEditEnable)
		return false;

	if (CheckRepeatKey(nChar)) {
		Note.Effects[EffectIndex] = m_LastNote.Effects[0];
		if (EditStyle != edit_style_t::MPT)		// // //
			bStepDown = true;
		if (m_bEditEnable && Note.Effects[EffectIndex].fx != effect_t::none)		// // //
			GetParentFrame()->SetMessageText(GetEffectHint(Note, EffectIndex));
		return true;
	}

	if (CheckClearKey(nChar)) {
		Note.Effects[EffectIndex].fx = effect_t::none;
		if (EditStyle != edit_style_t::MPT)
			bStepDown = true;
		return true;
	}

	if (nChar >= VK_NUMPAD0 && nChar <= VK_NUMPAD9)
		nChar = '0' + nChar - VK_NUMPAD0;

	if (effect_t Effect = Env.GetSoundChipService()->TranslateEffectName(nChar, GetSelectedChannelID().Chip); Effect != effect_t::none) {		// // //
		Note.Effects[EffectIndex].fx = Effect;
		if (m_bEditEnable && Note.Effects[EffectIndex].fx != effect_t::none)		// // //
			GetParentFrame()->SetMessageText(GetEffectHint(Note, EffectIndex));
		switch (EditStyle) {
			case edit_style_t::MPT:	// Modplug
				if (Effect == m_LastNote.Effects[0].fx)
					Note.Effects[EffectIndex].param = m_LastNote.Effects[0].param;
				break;
			default:
				bStepDown = true;
		}
		m_LastNote.Effects[0].fx = Effect;
		m_LastNote.Effects[0].param = Note.Effects[EffectIndex].param;		// // //
		return true;
	}

	return false;
}

bool CFamiTrackerView::EditEffParamColumn(stChanNote &Note, int Key, int EffectIndex, bool &bStepDown, bool &bMoveRight, bool &bMoveLeft)
{
	edit_style_t EditStyle = Env.GetSettings()->General.iEditStyle;		// // //
	const cursor_column_t Column = m_pPatternEditor->GetColumn();		// // //
	int Value = ConvertKeyToHex(Key);

	if (!m_bEditEnable)
		return false;

	if (CheckRepeatKey(Key)) {
		Note.Effects[EffectIndex] = m_LastNote.Effects[0];
		if (EditStyle != edit_style_t::MPT)		// // //
			bStepDown = true;
		if (m_bEditEnable && Note.Effects[EffectIndex].fx != effect_t::none)		// // //
			GetParentFrame()->SetMessageText(GetEffectHint(Note, EffectIndex));
		return true;
	}

	if (CheckClearKey(Key)) {
		Note.Effects[EffectIndex].param = 0;
		if (EditStyle != edit_style_t::MPT)
			bStepDown = true;
		return true;
	}

	if (Value == -1 && Env.GetSettings()->General.bHexKeypad)		// // //
		Value = ConvertKeyExtra(Key);
	if (Value == -1)
		return false;

	unsigned char Mask, Shift;
	if (Column == cursor_column_t::EFF1_PARAM1 || Column == cursor_column_t::EFF2_PARAM1 || Column == cursor_column_t::EFF3_PARAM1 || Column == cursor_column_t::EFF4_PARAM1) {
		Mask = 0x0F;
		Shift = 4;
	}
	else {
		Mask = 0xF0;
		Shift = 0;
	}

	switch (EditStyle) {
		case edit_style_t::FT2:	// FT2
			Note.Effects[EffectIndex].param = (Note.Effects[EffectIndex].param & Mask) | Value << Shift;
			bStepDown = true;
			break;
		case edit_style_t::MPT:	// Modplug
			Note.Effects[EffectIndex].param = ((Note.Effects[EffectIndex].param & 0x0F) << 4) | Value & 0x0F;
			break;
		case edit_style_t::IT:	// IT
			Note.Effects[EffectIndex].param = (Note.Effects[EffectIndex].param & Mask) | Value << Shift;
			if (Mask == 0x0F)
				bMoveRight = true;
			else {
				bMoveLeft = true;
				bStepDown = true;
			}
			break;
	}

	m_LastNote.Effects[0] = Note.Effects[EffectIndex];		// // //

	if (m_bEditEnable && Note.Effects[EffectIndex].fx != effect_t::none)		// // //
		GetParentFrame()->SetMessageText(GetEffectHint(Note, EffectIndex));

	return true;
}

void CFamiTrackerView::HandleKeyboardInput(unsigned char nChar)		// // //
{
	if (Env.GetAccelerator()->IsKeyUsed(nChar)) return;		// // //

	edit_style_t EditStyle = Env.GetSettings()->General.iEditStyle;		// // //
	int Index = 0;

	int Frame = GetSelectedFrame();
	int Row = GetSelectedRow();
	cursor_column_t Column = m_pPatternEditor->GetColumn();

	bool bStepDown = false;
	bool bMoveRight = false;
	bool bMoveLeft = false;

	// Watch for repeating keys
	if (PreventRepeat(nChar, m_bEditEnable))
		return;

	// Get the note data
	stChanNote Note = GetSongView()->GetPatternOnFrame(GetSelectedChannel(), Frame).GetNoteOn(Row);		// // //

	// Make all effect columns look the same, save an index instead
	switch (Column) {
		case cursor_column_t::EFF1_NUM:	Column = cursor_column_t::EFF1_NUM;	Index = 0; break;
		case cursor_column_t::EFF2_NUM:	Column = cursor_column_t::EFF1_NUM;	Index = 1; break;
		case cursor_column_t::EFF3_NUM:	Column = cursor_column_t::EFF1_NUM;	Index = 2; break;
		case cursor_column_t::EFF4_NUM:	Column = cursor_column_t::EFF1_NUM;	Index = 3; break;
		case cursor_column_t::EFF1_PARAM1:	Column = cursor_column_t::EFF1_PARAM1; Index = 0; break;
		case cursor_column_t::EFF2_PARAM1:	Column = cursor_column_t::EFF1_PARAM1; Index = 1; break;
		case cursor_column_t::EFF3_PARAM1:	Column = cursor_column_t::EFF1_PARAM1; Index = 2; break;
		case cursor_column_t::EFF4_PARAM1:	Column = cursor_column_t::EFF1_PARAM1; Index = 3; break;
		case cursor_column_t::EFF1_PARAM2:	Column = cursor_column_t::EFF1_PARAM2; Index = 0; break;
		case cursor_column_t::EFF2_PARAM2:	Column = cursor_column_t::EFF1_PARAM2; Index = 1; break;
		case cursor_column_t::EFF3_PARAM2:	Column = cursor_column_t::EFF1_PARAM2; Index = 2; break;
		case cursor_column_t::EFF4_PARAM2:	Column = cursor_column_t::EFF1_PARAM2; Index = 3; break;
	}

	if (Column != cursor_column_t::NOTE && !m_bEditEnable)		// // //
		HandleKeyboardNote(nChar, true);
	switch (Column) {
		// Note & octave column
		case cursor_column_t::NOTE:
			if (CheckRepeatKey(nChar)) {
				Note.Note = m_LastNote.Note;		// // //
				Note.Octave = m_LastNote.Octave;
			}
			else if (CheckEchoKey(nChar)) {		// // //
				m_LastNote.Note = Note.Note = note_t::echo;
				m_LastNote.Octave = Note.Octave = static_cast<CMainFrame*>(GetParentFrame())->GetSelectedOctave();		// // //
				if (Note.Octave > ECHO_BUFFER_LENGTH - 1)
					Note.Octave = ECHO_BUFFER_LENGTH - 1;
				if (!m_bMaskInstrument)
					Note.Instrument = GetInstrument();
			}
			else if (CheckClearKey(nChar)) {
				// Remove note
				m_LastNote.Note = Note.Note = note_t::none;		// // //
				m_LastNote.Octave = Note.Octave = 0;
			}
			else {
				// This is special
				HandleKeyboardNote(nChar, true);
				return;
			}
			if (EditStyle != edit_style_t::MPT)		// // //
				bStepDown = true;
			break;
		// Instrument column
		case cursor_column_t::INSTRUMENT1:
		case cursor_column_t::INSTRUMENT2:
			if (!EditInstrumentColumn(Note, nChar, bStepDown, bMoveRight, bMoveLeft))
				return;
			break;
		// Volume column
		case cursor_column_t::VOLUME:
			if (!EditVolumeColumn(Note, nChar, bStepDown))
				return;
			break;
		// Effect number
		case cursor_column_t::EFF1_NUM:
			if (!EditEffNumberColumn(Note, nChar, Index, bStepDown))
				return;
			break;
		// Effect parameter
		case cursor_column_t::EFF1_PARAM1:
		case cursor_column_t::EFF1_PARAM2:
			if (!EditEffParamColumn(Note, nChar, Index, bStepDown, bMoveRight, bMoveLeft))
				return;
			break;
	}

	if (CheckClearKey(nChar) && IsControlPressed())		// // //
		Note = stChanNote { };

	// Something changed, store pattern data in document and update screen
	if (m_bEditEnable) {
		auto pAction = std::make_unique<CPActionEditNote>(Note);		// // //
		auto &Action = *pAction; // TODO: remove
		if (AddAction(std::move(pAction))) {
			if (bMoveLeft)
				m_pPatternEditor->MoveLeft();
			if (bMoveRight)
				m_pPatternEditor->MoveRight();
			if (bStepDown)
				StepDown();
			InvalidateCursor();
			Action.SaveRedoState(static_cast<CMainFrame &>(*GetParentFrame()));		// // //
		}
	}
}

bool CFamiTrackerView::DoRelease() const
{
	// Return true if there are a valid release sequence for selected instrument
	const auto *pManager = GetModuleData()->GetInstrumentManager();		// // //
	auto pInstrument = pManager->GetInstrument(GetInstrument());		// // //
	return pInstrument && pInstrument->CanRelease();
}

void CFamiTrackerView::HandleKeyboardNote(char nChar, bool Pressed)
{
	if (Env.GetAccelerator()->IsKeyUsed(nChar))
		return;		// // //

	// Play a note from the keyboard
	int Note = TranslateKey(nChar);
	int Channel = GetSelectedChannel();

	if (Pressed) {
		if (CheckHaltKey(nChar)) {
			if (m_bEditEnable)
				CutMIDINote(Channel, m_iLastPressedKey, true);
			else {
				for (const auto &x : m_iNoteCorrection)		// // //
					CutMIDINote(Channel, TranslateKey(x.first), false);
				m_iNoteCorrection.clear();
			}
		}
		else if (CheckReleaseKey(nChar)) {
			if (m_bEditEnable)
				ReleaseMIDINote(Channel, m_iLastPressedKey, true);
			else {
				for (const auto &x : m_iNoteCorrection)		// // //
					ReleaseMIDINote(Channel, TranslateKey(x.first), false);
				m_iNoteCorrection.clear();
			}
		}
		else {
			// Invalid key
			if (Note == -1)
				return;
			TriggerMIDINote(Channel, Note, 0x7F, m_bEditEnable);
			m_iLastPressedKey = Note;
			m_iNoteCorrection[nChar] = 0;		// // //
		}
	}
	else {
		if (Note == -1)
			return;
		// IT doesn't cut the note when key is released
		if (Env.GetSettings()->General.iEditStyle != edit_style_t::IT) {
			// Find if note release should be used
			// TODO: make this an option instead?
			if (DoRelease())
				ReleaseMIDINote(Channel, Note, false);
			else
				CutMIDINote(Channel, Note, false);
			auto it = m_iNoteCorrection.find(nChar);		// // //
			if (it != m_iNoteCorrection.end())
				m_iNoteCorrection.erase(it);
		}
		else {
			m_pArpeggiator->ReleaseNote(Note);		// // //
		}
	}
}

void CFamiTrackerView::SplitKeyboardAdjust(stChanNote &Note, stChannelID Channel) const		// // //
{
	ASSERT(Note.Note >= note_t::C && Note.Note <= note_t::B);
	if (m_iSplitNote != -1 && !IsAPUNoise(Channel)) {
		int MidiNote = Note.ToMidiNote();
		if (MidiNote <= m_iSplitNote) {
			MidiNote = std::clamp(MidiNote + m_iSplitTranspose, 0, NOTE_COUNT - 1);
			Note.Octave = ft0cc::doc::oct_from_midi(MidiNote);
			Note.Note = ft0cc::doc::pitch_from_midi(MidiNote);

			if (m_iSplitInstrument != MAX_INSTRUMENTS)
				Note.Instrument = m_iSplitInstrument;
		}
	}
}

stChannelID CFamiTrackerView::SplitAdjustChannel(stChannelID Channel, const stChanNote &Note) const		// // //
{
	if (!m_bEditEnable && m_iSplitChannel.Chip != sound_chip_t::none)
		if (m_iSplitNote != -1 && Note.ToMidiNote() <= m_iSplitNote)
			if (GetSongView()->GetChannelOrder().HasChannel(m_iSplitChannel))
				return m_iSplitChannel;
	return Channel;
}

bool CFamiTrackerView::CheckClearKey(unsigned char Key) const
{
	return (Key == Env.GetSettings()->Keys.iKeyClear);
}

bool CFamiTrackerView::CheckReleaseKey(unsigned char Key) const
{
	return (Key == Env.GetSettings()->Keys.iKeyNoteRelease);
}

bool CFamiTrackerView::CheckHaltKey(unsigned char Key) const
{
	return (Key == Env.GetSettings()->Keys.iKeyNoteCut);
}

bool CFamiTrackerView::CheckEchoKey(unsigned char Key) const		// // //
{
	return (Key == Env.GetSettings()->Keys.iKeyEchoBuffer);
}

bool CFamiTrackerView::CheckRepeatKey(unsigned char Key) const
{
	return (Key == Env.GetSettings()->Keys.iKeyRepeat);
}

int CFamiTrackerView::TranslateKeyModplug(unsigned char Key) const
{
	// Modplug conversion
	note_t KeyNote = note_t::none;
	int KeyOctave = 0;
	int Octave = static_cast<CMainFrame*>(GetParentFrame())->GetSelectedOctave();		// // // 050B

	const auto &NoteData = GetSongView()->GetPatternOnFrame(GetSelectedChannel(), GetSelectedFrame()).GetNoteOn(GetSelectedRow());

	if (m_bEditEnable && Key >= '0' && Key <= '9') {		// // //
		KeyOctave = Key - '1';
		if (KeyOctave < 0) KeyOctave += 10;
		if (KeyOctave >= OCTAVE_RANGE) KeyOctave = OCTAVE_RANGE - 1;
		KeyNote = NoteData.Note;
	}

	// Convert key to a note, Modplug style
	switch (Key) {
	case u8'Q':			KeyNote = note_t::C;	KeyOctave = Octave;	break;
	case u8'W':			KeyNote = note_t::Cs;	KeyOctave = Octave;	break;
	case u8'E':			KeyNote = note_t::D;	KeyOctave = Octave;	break;
	case u8'R':			KeyNote = note_t::Ds;	KeyOctave = Octave;	break;
	case u8'T':			KeyNote = note_t::E;	KeyOctave = Octave;	break;
	case u8'Y':			KeyNote = note_t::F;	KeyOctave = Octave;	break;
	case u8'U':			KeyNote = note_t::Fs;	KeyOctave = Octave;	break;
	case u8'I':			KeyNote = note_t::G;	KeyOctave = Octave;	break;
	case u8'O':			KeyNote = note_t::Gs;	KeyOctave = Octave;	break;
	case u8'P':			KeyNote = note_t::A;	KeyOctave = Octave;	break;
	case VK_OEM_4:		KeyNote = note_t::As;	KeyOctave = Octave;	break; // [{
	case VK_OEM_6:		KeyNote = note_t::B;	KeyOctave = Octave;	break; // ]}

	case u8'A':			KeyNote = note_t::C;	KeyOctave = Octave + 1;	break;
	case u8'S':			KeyNote = note_t::Cs;	KeyOctave = Octave + 1;	break;
	case u8'D':			KeyNote = note_t::D;	KeyOctave = Octave + 1;	break;
	case u8'F':			KeyNote = note_t::Ds;	KeyOctave = Octave + 1;	break;
	case u8'G':			KeyNote = note_t::E;	KeyOctave = Octave + 1;	break;
	case u8'H':			KeyNote = note_t::F;	KeyOctave = Octave + 1;	break;
	case u8'J':			KeyNote = note_t::Fs;	KeyOctave = Octave + 1;	break;
	case u8'K':			KeyNote = note_t::G;	KeyOctave = Octave + 1;	break;
	case u8'L':			KeyNote = note_t::Gs;	KeyOctave = Octave + 1;	break;
	case VK_OEM_1:		KeyNote = note_t::A;	KeyOctave = Octave + 1;	break; // ;:
	case VK_OEM_7:		KeyNote = note_t::As;	KeyOctave = Octave + 1;	break; // '"
	//case VK_OEM_2:	KeyNote = note_t::B;	KeyOctave = Octave + 1;	break; // /?

	case u8'Z':			KeyNote = note_t::C;	KeyOctave = Octave + 2;	break;
	case u8'X':			KeyNote = note_t::Cs;	KeyOctave = Octave + 2;	break;
	case u8'C':			KeyNote = note_t::D;	KeyOctave = Octave + 2;	break;
	case u8'V':			KeyNote = note_t::Ds;	KeyOctave = Octave + 2;	break;
	case u8'B':			KeyNote = note_t::E;	KeyOctave = Octave + 2;	break;
	case u8'N':			KeyNote = note_t::F;	KeyOctave = Octave + 2;	break;
	case u8'M':			KeyNote = note_t::Fs;	KeyOctave = Octave + 2;	break;
	case VK_OEM_COMMA:	KeyNote = note_t::G;	KeyOctave = Octave + 2;	break; // ,<
	case VK_OEM_PERIOD:	KeyNote = note_t::Gs;	KeyOctave = Octave + 2;	break; // .>
	case VK_OEM_2:		KeyNote = note_t::A;	KeyOctave = Octave + 2;	break; // /?
	}

	// Invalid
	if (KeyNote == note_t::none)
		return -1;

	auto it = m_iNoteCorrection.find(Key);		// // //
	if (it != m_iNoteCorrection.end())
		KeyOctave += it->second;

	// Return a MIDI note
	return ft0cc::doc::midi_note(KeyOctave, KeyNote);
}

int CFamiTrackerView::TranslateKeyDefault(unsigned char Key) const
{
	// Default conversion
	note_t KeyNote = note_t::none;
	int KeyOctave = static_cast<CMainFrame*>(GetParentFrame())->GetSelectedOctave();		// // // 050B

	// Convert key to a note
	switch (Key) {
		case u8'2':			KeyNote = note_t::Cs;	KeyOctave += 1;	break;
		case u8'3':			KeyNote = note_t::Ds;	KeyOctave += 1;	break;
		case u8'5':			KeyNote = note_t::Fs;	KeyOctave += 1;	break;
		case u8'6':			KeyNote = note_t::Gs;	KeyOctave += 1;	break;
		case u8'7':			KeyNote = note_t::As;	KeyOctave += 1;	break;
		case u8'9':			KeyNote = note_t::Cs;	KeyOctave += 2;	break;
		case u8'0':			KeyNote = note_t::Ds;	KeyOctave += 2;	break;
		case VK_OEM_PLUS:	KeyNote = note_t::Fs;	KeyOctave += 2;	break; // =+

		case u8'Q':			KeyNote = note_t::C;	KeyOctave += 1;	break;
		case u8'W':			KeyNote = note_t::D;	KeyOctave += 1;	break;
		case u8'E':			KeyNote = note_t::E;	KeyOctave += 1;	break;
		case u8'R':			KeyNote = note_t::F;	KeyOctave += 1;	break;
		case u8'T':			KeyNote = note_t::G;	KeyOctave += 1;	break;
		case u8'Y':			KeyNote = note_t::A;	KeyOctave += 1;	break;
		case u8'U':			KeyNote = note_t::B;	KeyOctave += 1;	break;
		case u8'I':			KeyNote = note_t::C;	KeyOctave += 2;	break;
		case u8'O':			KeyNote = note_t::D;	KeyOctave += 2;	break;
		case u8'P':			KeyNote = note_t::E;	KeyOctave += 2;	break;
		case VK_OEM_4:		KeyNote = note_t::F;	KeyOctave += 2;	break; // [{
		case VK_OEM_6:		KeyNote = note_t::G;	KeyOctave += 2;	break; // ]}

		case u8'S':			KeyNote = note_t::Cs;					break;
		case u8'D':			KeyNote = note_t::Ds;					break;
		case u8'G':			KeyNote = note_t::Fs;					break;
		case u8'H':			KeyNote = note_t::Gs;					break;
		case u8'J':			KeyNote = note_t::As;					break;
		case u8'L':			KeyNote = note_t::Cs;	KeyOctave += 1;	break;
		case VK_OEM_1:		KeyNote = note_t::Ds;	KeyOctave += 1;	break; // ;:

		case u8'Z':			KeyNote = note_t::C;					break;
		case u8'X':			KeyNote = note_t::D;					break;
		case u8'C':			KeyNote = note_t::E;					break;
		case u8'V':			KeyNote = note_t::F;					break;
		case u8'B':			KeyNote = note_t::G;					break;
		case u8'N':			KeyNote = note_t::A;					break;
		case u8'M':			KeyNote = note_t::B;					break;
		case VK_OEM_COMMA:	KeyNote = note_t::C;	KeyOctave += 1;	break; // ,<
		case VK_OEM_PERIOD:	KeyNote = note_t::D;	KeyOctave += 1;	break; // .>
		case VK_OEM_2:		KeyNote = note_t::E;	KeyOctave += 1;	break; // /?
	}

	// Invalid
	if (KeyNote == note_t::none)
		return -1;

	auto it = m_iNoteCorrection.find(Key);		// // //
	if (it != m_iNoteCorrection.end())
		KeyOctave += it->second;

	// Return a MIDI note
	return ft0cc::doc::midi_note(KeyOctave, KeyNote);
}

int CFamiTrackerView::TranslateKey(unsigned char Key) const
{
	// Translates a keyboard character into a MIDI note

	// For modplug users
	if (Env.GetSettings()->General.iEditStyle == edit_style_t::MPT)
		return TranslateKeyModplug(Key);

	// Default
	return TranslateKeyDefault(Key);
}

bool CFamiTrackerView::PreventRepeat(unsigned char Key, bool Insert)
{
	if (m_cKeyList[Key] == 0)
		m_cKeyList[Key] = 1;
	else {
		if ((!Env.GetSettings()->General.bKeyRepeat || !Insert))
			return true;
	}

	return false;
}

void CFamiTrackerView::RepeatRelease(unsigned char Key)
{
	m_cKeyList.fill(0);		// // //
}

//
// Note preview
//

bool CFamiTrackerView::PreviewNote(unsigned char Key)
{
	if (PreventRepeat(Key, false))
		return false;

	int Note = TranslateKey(Key);

	TRACE(L"View: Note preview\n");

	if (Note > 0) {
		TriggerMIDINote(GetSelectedChannel(), Note, 0x7F, false);
		return true;
	}

	return false;
}

void CFamiTrackerView::PreviewRelease(unsigned char Key)
{
	m_cKeyList.fill(0);		// // //

	int Note = TranslateKey(Key);

	if (Note > 0) {
		if (DoRelease())
			ReleaseMIDINote(GetSelectedChannel(), Note, false);
		else
			CutMIDINote(GetSelectedChannel(), Note, false);
	}
}

//
// MIDI in routines
//

void CFamiTrackerView::TranslateMidiMessage()
{
	// Check and handle MIDI messages

	CMIDI *pMIDI = Env.GetMIDI();
	CSongView *pSongView = GetSongView();		// // //
	CStringW Status;

	if (!pMIDI || !pSongView)
		return;

	unsigned char Message, Channel, Data1, Data2;
	while (pMIDI->ReadMessage(Message, Channel, Data1, Data2)) {

		if (Message != 0x0F) {
			if (!Env.GetSettings()->Midi.bMidiChannelMap)
				Channel = GetSelectedChannel();
			Channel = std::clamp((int)Channel, 0, (int)pSongView->GetChannelOrder().GetChannelCount() - 1);		// // //
		}

		// Translate key releases to note off messages
		if (Message == MIDI_MSG_NOTE_ON && Data2 == 0) {
			Message = MIDI_MSG_NOTE_OFF;
		}

		if (Message == MIDI_MSG_NOTE_ON || Message == MIDI_MSG_NOTE_OFF) {
			// Remove two octaves from MIDI notes
			Data1 -= 24;
			if (Data1 > 127)
				return;
		}

		switch (Message) {
			case MIDI_MSG_NOTE_ON:
				TriggerMIDINote(Channel, Data1, Data2, m_bEditEnable);		// // //
				Status = AfxFormattedW(IDS_MIDI_MESSAGE_ON_FORMAT,
					FormattedW(L"%i", Data1 % 12),
					FormattedW(L"%i", Data1 / 12),
					FormattedW(L"%i", Data2));
				break;

			case MIDI_MSG_NOTE_OFF:
				// MIDI key is released, don't input note break into pattern
				if (DoRelease())
					ReleaseMIDINote(Channel, Data1, false);
				else
					CutMIDINote(Channel, Data1, false);
				Status = CStringW(MAKEINTRESOURCEW(IDS_MIDI_MESSAGE_OFF));
				break;

			case MIDI_MSG_PITCH_WHEEL:
				{
					int PitchValue = 0x2000 - ((Data1 & 0x7F) | ((Data2 & 0x7F) << 7));
					GetTrackerChannel(Channel).SetPitch(-PitchValue / 0x10);		// // //
				}
				break;

			case 0x0F:
				if (Channel == 0x08) {
					m_pPatternEditor->MoveDown(m_iInsertKeyStepping);
					InvalidateCursor();
				}
				break;
		}
	}

	if (Status.GetLength() > 0)
		GetParentFrame()->SetMessageText(Status);
}

//
// Effects menu
//

void CFamiTrackerView::OnTrackerToggleChannel()
{
	if (m_iMenuChannel.Chip == sound_chip_t::none)
		m_iMenuChannel = GetSelectedChannelID();

	ToggleChannel(m_iMenuChannel);

	m_iMenuChannel = { };
}

void CFamiTrackerView::OnTrackerSoloChannel()
{
	if (m_iMenuChannel.Chip == sound_chip_t::none)
		m_iMenuChannel = GetSelectedChannelID();

	SoloChannel(m_iMenuChannel);

	m_iMenuChannel = { };
}

void CFamiTrackerView::OnTrackerToggleChip()		// // //
{
	if (m_iMenuChannel.Chip == sound_chip_t::none)
		m_iMenuChannel = GetSelectedChannelID();

	ToggleChip(m_iMenuChannel);

	m_iMenuChannel = { };
}

void CFamiTrackerView::OnTrackerSoloChip()		// // //
{
	if (m_iMenuChannel.Chip == sound_chip_t::none)
		m_iMenuChannel = GetSelectedChannelID();

	SoloChip(m_iMenuChannel);

	m_iMenuChannel = { };
}

void CFamiTrackerView::OnTrackerUnmuteAllChannels()
{
	UnmuteAllChannels();
}

void CFamiTrackerView::OnTrackerRecordToInst()		// // //
{
	if (m_iMenuChannel.Chip == sound_chip_t::none)
		m_iMenuChannel = GetSelectedChannelID();

	CInstrumentManager *pManager = GetModuleData()->GetInstrumentManager();
	m_iMenuChannel = { };

	if (IsDPCM(m_iMenuChannel) || m_iMenuChannel.Chip == sound_chip_t::VRC7) {
		AfxMessageBox(IDS_DUMP_NOT_SUPPORTED, MB_ICONERROR); return;
	}
	if (pManager->GetInstrumentCount() >= MAX_INSTRUMENTS) {
		AfxMessageBox(IDS_INST_LIMIT, MB_ICONERROR); return;
	}
	if (m_iMenuChannel.Chip != sound_chip_t::FDS) {
		inst_type_t Type = INST_NONE;
		switch (m_iMenuChannel.Chip) {
		case sound_chip_t::APU: case sound_chip_t::MMC5: Type = INST_2A03; break;
		case sound_chip_t::VRC6: Type = INST_VRC6; break;
		case sound_chip_t::N163: Type = INST_N163; break;
		case sound_chip_t::S5B:  Type = INST_S5B; break;
		}
		if (Type != INST_NONE)
			for (auto i : enum_values<sequence_t>())
				if (pManager->GetFreeSequenceIndex(Type, i, nullptr) == -1) {
					AfxMessageBox(IDS_SEQUENCE_LIMIT, MB_ICONERROR);
					return;
				}
	}

	if (IsChannelMuted(GetSelectedChannelID()))
		ToggleChannel(GetSelectedChannelID());
	Env.GetSoundGenerator()->SetRecordChannel(m_iMenuChannel == Env.GetSoundGenerator()->GetRecordChannel() ? stChannelID { } : m_iMenuChannel);
	InvalidateHeader();
}

void CFamiTrackerView::OnTrackerRecorderSettings()
{
	if (CRecordSettingsDlg dlg; dlg.DoModal() == IDOK)
		Env.GetSoundGenerator()->SetRecordSetting(dlg.GetRecordSetting());
}

void CFamiTrackerView::AdjustOctave(int Delta)		// // //
{
	for (auto &x : m_iNoteCorrection)
		x.second -= Delta;
}

void CFamiTrackerView::OnIncreaseStepSize()
{
	if (m_iInsertKeyStepping < MAX_PATTERN_LENGTH)		// // //
		SetStepping(m_iInsertKeyStepping + 1);
}

void CFamiTrackerView::OnDecreaseStepSize()
{
	if (m_iInsertKeyStepping > 0)
		SetStepping(m_iInsertKeyStepping - 1);
}

void CFamiTrackerView::SetStepping(int Step)
{
	m_iInsertKeyStepping = Step;

	if (Step > 0 && !Env.GetSettings()->General.bNoStepMove)
		m_iMoveKeyStepping = Step;
	else
		m_iMoveKeyStepping = 1;

	static_cast<CMainFrame*>(GetParentFrame())->UpdateControls();
}

void CFamiTrackerView::OnEditInterpolate()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(std::make_unique<CPActionInterpolate>());
}

void CFamiTrackerView::OnEditReverse()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(std::make_unique<CPActionReverse>());
}

void CFamiTrackerView::OnEditReplaceInstrument()
{
	if (!m_bEditEnable) return;		// // //
	AddAction(std::make_unique<CPActionReplaceInst>(GetInstrument()));
}

void CFamiTrackerView::OnEditExpandPatterns()		// // //
{
	if (!m_bEditEnable) return;
	AddAction(std::make_unique<CPActionStretch>(std::vector<int> {1, 0}));
}

void CFamiTrackerView::OnEditShrinkPatterns()		// // //
{
	if (!m_bEditEnable) return;
	AddAction(std::make_unique<CPActionStretch>(std::vector<int> {2}));
}

void CFamiTrackerView::OnEditStretchPatterns()		// // //
{
	if (!m_bEditEnable) return;

	CStretchDlg StretchDlg;
	AddAction(std::make_unique<CPActionStretch>(StretchDlg.GetStretchMap()));
}

void CFamiTrackerView::OnNcMouseMove(UINT nHitTest, CPoint point)
{
	if (m_pPatternEditor->OnMouseNcMove())
		InvalidateHeader();

	CView::OnNcMouseMove(nHitTest, point);
}

void CFamiTrackerView::OnOneStepUp()
{
	m_pPatternEditor->MoveUp(SINGLE_STEP);
	InvalidateCursor();
}

void CFamiTrackerView::OnOneStepDown()
{
	m_pPatternEditor->MoveDown(SINGLE_STEP);
	InvalidateCursor();
}

void CFamiTrackerView::MakeSilent()
{
	m_cKeyList.fill(0);		// // //
}

bool CFamiTrackerView::IsSelecting() const
{
	return m_pPatternEditor->IsSelecting();
}

bool CFamiTrackerView::IsClipboardAvailable() const
{
	return ::IsClipboardFormatAvailable(m_iClipboard) == TRUE;
}

void CFamiTrackerView::OnBlockStart()
{
	m_pPatternEditor->SetBlockStart();
	InvalidateCursor();
}

void CFamiTrackerView::OnBlockEnd()
{
	m_pPatternEditor->SetBlockEnd();
	InvalidateCursor();
}

void CFamiTrackerView::OnPickupRow()
{
	// Get row info
	int Frame = GetSelectedFrame();
	int Row = GetSelectedRow();

	const auto &Note = GetSongView()->GetPatternOnFrame(GetSelectedChannel(), Frame).GetNoteOn(Row);		// // //

	m_LastNote.Note = Note.Note;		// // //
	m_LastNote.Octave = Note.Octave;
	m_LastNote.Vol = Note.Vol;
	m_LastNote.Instrument = Note.Instrument;

	if (Note.Instrument != MAX_INSTRUMENTS)
		SetInstrument(Note.Instrument);

	column_t Col = GetSelectColumn(m_pPatternEditor->GetColumn());		// // //
	if (Col >= column_t::Effect1) {
		unsigned fx = value_cast(Col) - value_cast(column_t::Effect1);
		m_LastNote.Effects[0] = Note.Effects[fx];
	}
}

bool CFamiTrackerView::AddAction(std::unique_ptr<CAction> pAction) const		// // //
{
	// Performs an action and adds it to the undo queue
	CMainFrame *pMainFrm = static_cast<CMainFrame*>(GetParentFrame());
	return pMainFrm ? pMainFrm->AddAction(std::move(pAction)) : false;
}

// OLE support

DROPEFFECT CFamiTrackerView::OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	TRACE(L"OLE: OnDragEnter\n");

//	sel_condition_t Cond = m_pPatternEditor->GetSelectionCondition();
	if (m_pPatternEditor->GetSelectionCondition() == sel_condition_t::NONTERMINAL_SKIP) {		// // //
		MessageBeep(MB_ICONWARNING);
		static_cast<CMainFrame*>(GetParentFrame())->SetMessageText(IDS_SEL_NONTERMINAL_SKIP);
		m_nDropEffect = DROPEFFECT_NONE;
	}
	else if (m_pPatternEditor->GetSelectionCondition() == sel_condition_t::REPEATED_ROW) {		// // //
		MessageBeep(MB_ICONWARNING);
		static_cast<CMainFrame*>(GetParentFrame())->SetMessageText(IDS_SEL_REPEATED_ROW);
		m_nDropEffect = DROPEFFECT_NONE;
	}
	else if (m_bEditEnable && pDataObject->IsDataAvailable(m_iClipboard)) {		// // //
		if (dwKeyState & (MK_CONTROL | MK_SHIFT)) {
			m_nDropEffect = DROPEFFECT_COPY;
			m_bDropMix = (dwKeyState & MK_SHIFT) == MK_SHIFT;		// // //
		}
		else
			m_nDropEffect = DROPEFFECT_MOVE;

		// Get drag rectangle
		CPatternClipData DragData;		// // //
		CClipboard::ReadGlobalMemory(DragData, pDataObject->GetGlobalData(m_iClipboard));

		// Begin drag operation
		m_pPatternEditor->BeginDrag(DragData);
		m_pPatternEditor->UpdateDrag(point);

		InvalidateCursor();
	}

	return m_nDropEffect;
}

void CFamiTrackerView::OnDragLeave()
{
	TRACE(L"OLE: OnDragLeave\n");

	if (m_nDropEffect != DROPEFFECT_NONE) {
		m_pPatternEditor->EndDrag();
		InvalidateCursor();
	}

	m_nDropEffect = DROPEFFECT_NONE;

	CView::OnDragLeave();
}

DROPEFFECT CFamiTrackerView::OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	TRACE(L"OLE: OnDragOver\n");

	// Update drag'n'drop cursor
	if (m_nDropEffect != DROPEFFECT_NONE) {
		m_pPatternEditor->UpdateDrag(point);
		InvalidateCursor();
	}

	return m_nDropEffect;
}

BOOL CFamiTrackerView::OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	TRACE(L"OLE: OnDrop\n");

	BOOL Result = FALSE;

	// Perform drop
	if (m_bEditEnable && m_nDropEffect != DROPEFFECT_NONE) {		// // //
		bool bCopy = (dropEffect == DROPEFFECT_COPY) || (m_bDragSource == false);

		m_pPatternEditor->UpdateDrag(point);

		// Get clipboard data
		CPatternClipData ClipData;		// // //
		if (CClipboard::ReadGlobalMemory(ClipData, pDataObject->GetGlobalData(m_iClipboard))) {
			m_pPatternEditor->PerformDrop(std::move(ClipData), bCopy, m_bDropMix);		// // // ???
			m_bDropped = true;
		}

		InvalidateCursor();

		Result = TRUE;
	}

	m_nDropEffect = DROPEFFECT_NONE;

	return Result;
}

void CFamiTrackerView::BeginDragData(int ChanOffset, int RowOffset)
{
	TRACE(L"OLE: BeginDragData\n");

	CPatternClipData ClipData = m_pPatternEditor->Copy();		// // //

	ClipData.ClipInfo.OleInfo.ChanOffset = ChanOffset;
	ClipData.ClipInfo.OleInfo.RowOffset = RowOffset;

	m_bDragSource = true;
	m_bDropped = false;

	DROPEFFECT res = CClipboard::DragDropTransfer(ClipData, m_iClipboard, DROPEFFECT_COPY | DROPEFFECT_MOVE);		// // // calls DropData

	if (!m_bDropped) { // Target was another window
		if (res & DROPEFFECT_MOVE)
			AddAction(std::make_unique<CPActionClearSel>());		// // // Delete data
		m_pPatternEditor->CancelSelection();
	}

	m_bDragSource = false;
}

bool CFamiTrackerView::IsDragging() const
{
	return m_bDragSource;
}

void CFamiTrackerView::EditReplace(stChanNote &Note)		// // //
{
	AddAction(std::make_unique<CPActionEditNote>(Note));
	InvalidateCursor();
	// pAction->SaveRedoState(static_cast<CMainFrame*>(GetParentFrame()));		// // //
}

CPatternEditor *CFamiTrackerView::GetPatternEditor() const {		// // //
	ASSERT(m_pPatternEditor);
	return m_pPatternEditor.get();
}

void CFamiTrackerView::OnUpdateFind(CCmdUI *pCmdUI)		// // //
{
	InvalidateCursor();
}

void CFamiTrackerView::OnRecallChannelState() {		// // //
	GetParentFrame()->SetMessageText(conv::to_wide(Env.GetSoundGenerator()->RecallChannelState(GetSelectedChannelID())).data());
}

CStringW CFamiTrackerView::GetEffectHint(const stChanNote &Note, int Column) const		// // //
{
	auto Index = value_cast(Note.Effects[Column].fx);
	uint8_t Param = Note.Effects[Column].param;
	if (enum_cast<effect_t>(Index) != Note.Effects[Column].fx)
		return L"Undefined effect";

	sound_chip_t Chip = GetSelectedChannelID().Chip;
	const int xy = 0;
	if (Index > value_cast(effect_t::FDS_VOLUME)       || (Index == value_cast(effect_t::FDS_VOLUME)       && Param >= 0x40u))
		++Index;
	if (Index > value_cast(effect_t::TRANSPOSE)        || (Index == value_cast(effect_t::TRANSPOSE)        && Param >= 0x80u))
		++Index;
	if (Index > value_cast(effect_t::SUNSOFT_ENV_TYPE) || (Index == value_cast(effect_t::SUNSOFT_ENV_TYPE) && Param >= 0x10u))
		++Index;
	if (Index > value_cast(effect_t::FDS_MOD_SPEED_HI) || (Index == value_cast(effect_t::FDS_MOD_SPEED_HI) && Param >= 0x10u))
		++Index;
	if (Index > value_cast(effect_t::FDS_MOD_DEPTH)    || (Index == value_cast(effect_t::FDS_MOD_DEPTH)    && Param >= 0x80u))
		++Index;
	if (Index > value_cast(effect_t::NOTE_CUT)         || (Index == value_cast(effect_t::NOTE_CUT)         && Param >= 0x80u && IsAPUTriangle(GetSelectedChannelID())))
		++Index;
	if (Index > value_cast(effect_t::DUTY_CYCLE)       || (Index == value_cast(effect_t::DUTY_CYCLE)       && (Chip == sound_chip_t::VRC7 || Chip == sound_chip_t::N163)))
		++Index;
	if (Index > value_cast(effect_t::DUTY_CYCLE)       || (Index == value_cast(effect_t::DUTY_CYCLE)       && Chip == sound_chip_t::N163))
		++Index;
	if (Index > value_cast(effect_t::VOLUME)           || (Index == value_cast(effect_t::VOLUME)           && Param >= 0xE0u))
		++Index;
	if (Index > value_cast(effect_t::SPEED)            || (Index == value_cast(effect_t::SPEED)            && Param >= GetModuleData()->GetSpeedSplitPoint()))
		++Index;

	return EFFECT_TEXTS[Index];
}
