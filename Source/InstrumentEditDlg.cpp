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

#include "InstrumentEditDlg.h"
#include <memory>		// // //
#include <string>
#include "FamiTrackerEnv.h"		// // //
#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"		// // //
#include "InstrumentManager.h"		// // //
#include "SeqInstrument.h"		// // //
#include "Instrument2A03.h"		// // //
#include "InstrumentVRC6.h"		// // //
#include "InstrumentN163.h"		// // //
#include "InstrumentS5B.h"		// // //
#include "InstrumentFDS.h"		// // //
#include "InstrumentVRC7.h"		// // //
#include "FamiTrackerView.h"
#include "SequenceEditor.h"
#include "InstrumentEditPanel.h"
#include "InstrumentEditorSeq.h"		// // //
#include "InstrumentEditorDPCM.h"
#include "InstrumentEditorVRC7.h"
#include "InstrumentEditorFDS.h"
#include "InstrumentEditorFDSEnvelope.h"
#include "InstrumentEditorN163Wave.h"
#include "MainFrm.h"
#include "SoundGen.h"
#include "SongView.h"		// // //
#include "TrackerChannel.h"
#include "DPI.h"		// // //
#include "str_conv/str_conv.hpp"		// // //

// Constants
const int CInstrumentEditDlg::KEYBOARD_WIDTH  = 561;
const int CInstrumentEditDlg::KEYBOARD_HEIGHT = 58;

const LPCWSTR CInstrumentEditDlg::CHIP_NAMES[] = {		// // //
	L"",
	L"2A03",
	L"VRC6",
	L"VRC7",
	L"FDS",
	L"Namco",
	L"Sunsoft",
};

// CInstrumentEditDlg dialog

IMPLEMENT_DYNAMIC(CInstrumentEditDlg, CDialog)

CInstrumentEditDlg::CInstrumentEditDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInstrumentEditDlg::IDD, pParent),
	m_bOpened(false),
	m_fRefreshRate(60.0f),		// // //
	m_iInstrument(-1),
	m_pInstManager(nullptr)		// // //
{
}

CInstrumentEditDlg::~CInstrumentEditDlg()
{
}

void CInstrumentEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CInstrumentEditDlg, CDialog)
	ON_NOTIFY(TCN_SELCHANGE, IDC_INST_TAB, OnTcnSelchangeInstTab)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_NCLBUTTONUP()
END_MESSAGE_MAP()


// CInstrumentEditDlg message handlers

BOOL CInstrumentEditDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_iSelectedInstType = -1;
	m_iLastKey = -1;
	m_bOpened = true;

	m_pPanels.clear();		// // //

	CRect r;		// // //
	GetClientRect(&r);
	int cx = r.Width(), bot = r.bottom;
	GetDlgItem(IDC_INST_TAB)->GetWindowRect(&r);
	GetDesktopWindow()->MapWindowPoints(this, &r);
	auto pKeyboard = GetDlgItem(IDC_KEYBOARD);
	m_KeyboardRect.left   = -1 + (cx - KEYBOARD_WIDTH) / 2;
	m_KeyboardRect.top    = -1 + r.bottom + (bot - r.bottom - KEYBOARD_HEIGHT) / 2;
	m_KeyboardRect.right  =  1 + m_KeyboardRect.left + KEYBOARD_WIDTH;
	m_KeyboardRect.bottom =  1 + m_KeyboardRect.top + KEYBOARD_HEIGHT;
	pKeyboard->MoveWindow(m_KeyboardRect);
	m_KeyboardRect.DeflateRect(1, 1, 1, 1);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentEditDlg::InsertPane(std::unique_ptr<CInstrumentEditPanel> pPanel, bool Show) {		// // //
	CRect Rect, ParentRect;
	CTabCtrl *pTabControl = static_cast<CTabCtrl*>(GetDlgItem(IDC_INST_TAB));

	pPanel->SetInstrumentManager(m_pInstManager);		// // //

	pTabControl->GetWindowRect(&ParentRect);
	pTabControl->InsertItem(m_pPanels.size(), pPanel->GetTitle());		// // //

	pPanel->Create(pPanel->GetIDD(), this);
	pPanel->GetWindowRect(&Rect);
	Rect.MoveToXY(ParentRect.left - Rect.left + DPI::SX(1), ParentRect.top - Rect.top + DPI::SY(21));
	Rect.bottom -= DPI::SY(2);
	Rect.right += DPI::SX(1);
	pPanel->MoveWindow(Rect);
	pPanel->ShowWindow(Show ? SW_SHOW : SW_HIDE);

	if (Show) {
		pTabControl->SetCurSel(m_pPanels.size());
		pPanel->SetFocus();
		m_pFocusPanel = pPanel.get();
	}

	m_pPanels.push_back(std::move(pPanel));		// // //
}

void CInstrumentEditDlg::ClearPanels()
{
	static_cast<CTabCtrl*>(GetDlgItem(IDC_INST_TAB))->DeleteAllItems();

	for (auto &x : m_pPanels)		// // //
		if (x)
			x->DestroyWindow();
	m_pPanels.clear();

	m_iInstrument = -1;
}

void CInstrumentEditDlg::SetCurrentInstrument(int Index)
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	std::shared_ptr<CInstrument> pInstrument = pDoc->GetModule()->GetInstrumentManager()->GetInstrument(Index);
	int InstType = pInstrument->GetType();

	// Dialog title
	auto sv = conv::to_wide(pInstrument->GetName());
	CStringW Title;
	AfxFormatString1(Title, IDS_INSTRUMENT_EDITOR_TITLE, FormattedW(L"%02X. %.*s (%s)", Index, sv.size(), sv.data(), CHIP_NAMES[InstType]));		// // //
	SetWindowTextW(Title);

	if (InstType != m_iSelectedInstType) {
		ShowWindow(SW_HIDE);
		ClearPanels();

		switch (InstType) {
			case INST_2A03: {
					chan_id_t Type = CFamiTrackerView::GetView()->GetSelectedChannelID();		// // //
					bool bShowDPCM = (Type == chan_id_t::DPCM) || (std::static_pointer_cast<CInstrument2A03>(pInstrument)->AssignedSamples());
					InsertPane(std::make_unique<CInstrumentEditorSeq>(nullptr, L"2A03 settings",
						CInstrument2A03::SEQUENCE_NAME, 15, 3, INST_2A03), !bShowDPCM);		// // //
					InsertPane(std::make_unique<CInstrumentEditorDPCM>(), bShowDPCM);
				}
				break;
			case INST_VRC6:
				InsertPane(std::make_unique<CInstrumentEditorSeq>(nullptr, L"Konami VRC6",
					CInstrumentVRC6::SEQUENCE_NAME, 15, 7, INST_VRC6), true);
				break;
			case INST_VRC7:
				InsertPane(std::make_unique<CInstrumentEditorVRC7>(), true);
				break;
			case INST_FDS:
				InsertPane(std::make_unique<CInstrumentEditorFDS>(), true);
				InsertPane(std::make_unique<CInstrumentEditorFDSEnvelope>(), false);
				break;
			case INST_N163:
				InsertPane(std::make_unique<CInstrumentEditorSeq>(nullptr, L"Envelopes",
					CInstrumentN163::SEQUENCE_NAME, 15, CInstrumentN163::MAX_WAVE_COUNT - 1, INST_N163), true);
				InsertPane(std::make_unique<CInstrumentEditorN163Wave>(), false);
				break;
			case INST_S5B:
				InsertPane(std::make_unique<CInstrumentEditorSeq>(nullptr, L"Sunsoft 5B",
					CInstrumentS5B::SEQUENCE_NAME, 15, 255, INST_S5B), true);
				break;
		}

		m_iSelectedInstType = InstType;
	}

	for (const auto &x : m_pPanels)		// // //
		if (x)
			x->SelectInstrument(pInstrument);

	ShowWindow(SW_SHOW);
	UpdateWindow();

	m_iSelectedInstType = InstType;
}

float CInstrumentEditDlg::GetRefreshRate() const		// // //
{
	return m_fRefreshRate;
}

void CInstrumentEditDlg::SetRefreshRate(float Rate)		// // //
{
	m_fRefreshRate = Rate;
}

void CInstrumentEditDlg::OnTcnSelchangeInstTab(NMHDR *pNMHDR, LRESULT *pResult)
{
	CTabCtrl *pTabControl = static_cast<CTabCtrl*>(GetDlgItem(IDC_INST_TAB));
	int Selection = pTabControl->GetCurSel();

	for (std::size_t i = 0; i < std::size(m_pPanels); ++i)		// // //
		if (m_pPanels[i])
			m_pPanels[i]->ShowWindow(i == Selection ? SW_SHOW : SW_HIDE);

	m_pFocusPanel = m_pPanels[Selection].get();

	*pResult = 0;
}

/*!
	\brief Helper class to automatically select the previous drawing object when the context goes
	out of scope.
*/
struct CDCObjectContext		// // // TODO: put it somewhere else, maybe Graphics.h
{
	CDCObjectContext(CDC &dc, CGdiObject *obj) : _dc(dc)
	{
		_obj = dc.SelectObject(obj);
	}
	~CDCObjectContext()
	{
		_dc.SelectObject(_obj);
	}
private:
	CDC &_dc;
	CGdiObject *_obj;
};

void CInstrumentEditDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// Do not call CDialog::OnPaint() for painting messages

	const int WHITE_KEY_W	= 10;
	const int BLACK_KEY_W	= 8;

	CBitmap Bmp, WhiteKeyBmp, BlackKeyBmp, WhiteKeyMarkBmp, BlackKeyMarkBmp;
	Bmp.CreateCompatibleBitmap(&dc, 800, 800);
	WhiteKeyBmp.LoadBitmap(IDB_KEY_WHITE);
	BlackKeyBmp.LoadBitmap(IDB_KEY_BLACK);
	WhiteKeyMarkBmp.LoadBitmap(IDB_KEY_WHITE_MARK);
	BlackKeyMarkBmp.LoadBitmap(IDB_KEY_BLACK_MARK);

	CDC BackDC;
	BackDC.CreateCompatibleDC(&dc);
	CDCObjectContext c {BackDC, &Bmp};		// // //

	CDC WhiteKey;
	WhiteKey.CreateCompatibleDC(&dc);
	CDCObjectContext c2 {WhiteKey, &WhiteKeyBmp};		// // //

	CDC BlackKey;
	BlackKey.CreateCompatibleDC(&dc);
	CDCObjectContext c3 {BlackKey, &BlackKeyBmp};		// // //

	const note_t WHITE[]   = {note_t::C, note_t::D, note_t::E, note_t::F, note_t::G, note_t::A, note_t::B};
	const note_t BLACK_1[] = {note_t::Cs, note_t::Ds};
	const note_t BLACK_2[] = {note_t::Fs, note_t::Gs, note_t::As};

	note_t Note = GET_NOTE(m_iActiveKey);		// // //
	int Octave = GET_OCTAVE(m_iActiveKey);

	for (int j = 0; j < 8; j++) {
		int Pos = (WHITE_KEY_W * 7) * j;

		for (int i = 0; i < 7; i++) {
			bool Selected = (Note == WHITE[i]) && (Octave == j) && m_iActiveKey != -1;
			WhiteKey.SelectObject(Selected ? WhiteKeyMarkBmp : WhiteKeyBmp);
			int Offset = i * WHITE_KEY_W;
			BackDC.BitBlt(Pos + Offset, 0, 100, 100, &WhiteKey, 0, 0, SRCCOPY);
		}

		for (int i = 0; i < 2; i++) {
			bool Selected = (Note == BLACK_1[i]) && (Octave == j) && m_iActiveKey != -1;
			BlackKey.SelectObject(Selected ? BlackKeyMarkBmp : BlackKeyBmp);
			int Offset = i * WHITE_KEY_W + WHITE_KEY_W / 2 + 1;
			BackDC.BitBlt(Pos + Offset, 0, 100, 100, &BlackKey, 0, 0, SRCCOPY);
		}

		for (int i = 0; i < 3; i++) {
			bool Selected = (Note == BLACK_2[i]) && (Octave == j) && m_iActiveKey != -1;
			BlackKey.SelectObject(Selected ? BlackKeyMarkBmp : BlackKeyBmp);
			int Offset = (i + 3) * WHITE_KEY_W + WHITE_KEY_W / 2 + 1;
			BackDC.BitBlt(Pos + Offset, 0, 100, 100, &BlackKey, 0, 0, SRCCOPY);
		}
	}

	dc.BitBlt(m_KeyboardRect.left, m_KeyboardRect.top, KEYBOARD_WIDTH, KEYBOARD_HEIGHT, &BackDC, 0, 0, SRCCOPY);		// // //
}

void CInstrumentEditDlg::ChangeNoteState(int Note)
{
	// A MIDI key number or -1 to disable

	m_iActiveKey = Note;

	if (m_hWnd)
		RedrawWindow(m_KeyboardRect, 0, RDW_INVALIDATE);		// // //
}

void CInstrumentEditDlg::SwitchOnNote(int x, int y)
{
	CFamiTrackerView *pView = CFamiTrackerView::GetView();
	const CChannelOrder &Order = pView->GetSongView()->GetChannelOrder();
	CMainFrame *pFrameWnd = static_cast<CMainFrame*>(GetParent());
	chan_id_t Channel = pView->GetSelectedChannelID();		// // //

	// // // Send to respective channels whenever cursor is outside instrument chip
	if (m_iSelectedInstType == INST_2A03) {
		if (m_pPanels[0]->IsWindowVisible()) {
			if (Order.HasChannel(chan_id_t::SQUARE1) && (GetChipFromChannel(chan_id_t::SQUARE1) != GetChipFromChannel(Channel) || Channel == chan_id_t::DPCM))
				pView->SelectChannel(Order.GetChannelIndex(chan_id_t::SQUARE1));
		}
		else if (m_pPanels[1]->IsWindowVisible()) {
			if (Order.HasChannel(chan_id_t::DPCM) && Channel != chan_id_t::DPCM)
				pView->SelectChannel(Order.GetChannelIndex(chan_id_t::DPCM));
		}
	}
	else {
		chan_id_t First = chan_id_t::NONE;
		switch (m_iSelectedInstType) {
//		case INST_2A03: First = chan_id_t::SQUARE1; break;
		case INST_VRC6: First = chan_id_t::VRC6_PULSE1; break;
		case INST_N163: First = chan_id_t::N163_CH1; break;
		case INST_FDS:  First = chan_id_t::FDS; break;
		case INST_VRC7: First = chan_id_t::VRC7_CH1; break;
		case INST_S5B:  First = chan_id_t::S5B_CH1; break;
		}
		if (Order.HasChannel(First) && GetChipFromChannel(First) != GetChipFromChannel(Channel))
			pView->SelectChannel(Order.GetChannelIndex(First));
	}
	Channel = pView->GetSelectedChannelID();		// // //

	stChanNote NoteData;

	if (m_KeyboardRect.PtInRect({x, y})) {
		int KeyPos = (x - m_KeyboardRect.left) % 70;		// // //
		int Octave = (x - m_KeyboardRect.left) / 70;
		note_t Note;		// // //

		if (y > m_KeyboardRect.top + 38) {
			// Only white keys
			     if (KeyPos >= 60) Note = note_t::B;
			else if (KeyPos >= 50) Note = note_t::A;
			else if (KeyPos >= 40) Note = note_t::G;
			else if (KeyPos >= 30) Note = note_t::F;
			else if (KeyPos >= 20) Note = note_t::E;
			else if (KeyPos >= 10) Note = note_t::D;
			else if (KeyPos >=  0) Note = note_t::C;
		}
		else {
			// Black and white keys
			     if (KeyPos >= 62) Note = note_t::B;
			else if (KeyPos >= 56) Note = note_t::As;
			else if (KeyPos >= 53) Note = note_t::A;
			else if (KeyPos >= 46) Note = note_t::Gs;
			else if (KeyPos >= 43) Note = note_t::G;
			else if (KeyPos >= 37) Note = note_t::Fs;
			else if (KeyPos >= 30) Note = note_t::F;
			else if (KeyPos >= 23) Note = note_t::E;
			else if (KeyPos >= 16) Note = note_t::Ds;
			else if (KeyPos >= 13) Note = note_t::D;
			else if (KeyPos >=  7) Note = note_t::Cs;
			else if (KeyPos >=  0) Note = note_t::C;
		}

		int NewNote = MIDI_NOTE(Octave, Note);		// // //
		if (NewNote != m_iLastKey) {
			NoteData.Note			= Note;
			NoteData.Octave			= Octave;
			NoteData.Vol			= MAX_VOLUME - 1;
			NoteData.Instrument		= pFrameWnd->GetSelectedInstrumentIndex();

			Env.GetSoundGenerator()->QueueNote(Channel, NoteData, NOTE_PRIO_2);
			Env.GetSoundGenerator()->ForceReloadInstrument(Channel);		// // //
			m_iLastKey = NewNote;
		}
	}
	else {
		NoteData.Note			= pView->DoRelease() ? note_t::RELEASE : note_t::HALT;//note_t::HALT;

		Env.GetSoundGenerator()->QueueNote(Channel, NoteData, NOTE_PRIO_2);

		m_iLastKey = -1;
	}
}

void CInstrumentEditDlg::SwitchOffNote(bool ForceHalt)
{
	CFamiTrackerView *pView = CFamiTrackerView::GetView();
	CMainFrame *pFrameWnd = static_cast<CMainFrame*>(GetParent());

	stChanNote NoteData;		// // //
	NoteData.Note			= (pView->DoRelease() && !ForceHalt) ? note_t::RELEASE : note_t::HALT;
	NoteData.Instrument		= pFrameWnd->GetSelectedInstrumentIndex();

	Env.GetSoundGenerator()->QueueNote(pView->GetSelectedChannelID(), NoteData, NOTE_PRIO_2);

	m_iLastKey = -1;
}

void CInstrumentEditDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	SwitchOnNote(point.x, point.y);
	CDialog::OnLButtonDown(nFlags, point);
}

void CInstrumentEditDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	SwitchOffNote(false);
	CDialog::OnLButtonUp(nFlags, point);
}

void CInstrumentEditDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (nFlags & MK_LBUTTON)
		SwitchOnNote(point.x, point.y);

	CDialog::OnMouseMove(nFlags, point);
}

void CInstrumentEditDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	SwitchOnNote(point.x, point.y);
	CDialog::OnLButtonDblClk(nFlags, point);
}

BOOL CInstrumentEditDlg::DestroyWindow()
{
	ClearPanels();

	m_iSelectedInstType = -1;
	m_iInstrument = -1;
	m_bOpened = false;

	return CDialog::DestroyWindow();
}

void CInstrumentEditDlg::OnOK()
{
//	DestroyWindow();
//	CDialog::OnOK();
}

void CInstrumentEditDlg::OnCancel()
{
	DestroyWindow();
}

void CInstrumentEditDlg::OnNcLButtonUp(UINT nHitTest, CPoint point)
{
	// Mouse was released outside the client area
	SwitchOffNote(true);
	CDialog::OnNcLButtonUp(nHitTest, point);
}

bool CInstrumentEditDlg::IsOpened() const
{
	return m_bOpened;
}

void CInstrumentEditDlg::SetInstrumentManager(CInstrumentManager *pManager)
{
	m_pInstManager = pManager;
}
