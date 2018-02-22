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

#include "VisualizerWnd.h"
#include "FamiTrackerEnv.h"		// // //
#include "../resource.h"		// // //
#include "Settings.h"
#include "VisualizerScope.h"
#include "VisualizerSpectrum.h"
#include "VisualizerStatic.h"

// Thread entry helper

UINT CVisualizerWnd::ThreadProcFunc(LPVOID pParam)
{
	CVisualizerWnd *pObj = reinterpret_cast<CVisualizerWnd*>(pParam);

	if (pObj == NULL || !pObj->IsKindOf(RUNTIME_CLASS(CVisualizerWnd)))
		return 1;

	return pObj->ThreadProc();
}

// CSampleWindow

IMPLEMENT_DYNAMIC(CVisualizerWnd, CWnd)

CVisualizerWnd::CVisualizerWnd() :
	m_pStates {
		std::make_unique<CVisualizerScope>(false),
		std::make_unique<CVisualizerScope>(true),
		std::make_unique<CVisualizerSpectrum>(4),		// // //
		std::make_unique<CVisualizerSpectrum>(1),
		std::make_unique<CVisualizerStatic>(),
	},
	m_iCurrentState(0),
	m_bThreadRunning(false),
	m_pWorkerThread(NULL),
	m_iBufferSize(0),
	m_hNewSamples(NULL),
	m_bNoAudio(false)
{
}

CVisualizerWnd::~CVisualizerWnd()
{
}

HANDLE CVisualizerWnd::GetThreadHandle() const {		// // //
	return m_pWorkerThread->m_hThread;
}

BEGIN_MESSAGE_MAP(CVisualizerWnd, CWnd)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_PAINT()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

// State methods

void CVisualizerWnd::NextState()
{
	m_csBuffer.Lock();
	m_iCurrentState = (m_iCurrentState + 1) % std::size(m_pStates);
	m_csBuffer.Unlock();

	Invalidate();

	Env.GetSettings()->SampleWinState = m_iCurrentState;
}

// CSampleWindow message handlers

void CVisualizerWnd::SetSampleRate(int SampleRate)
{
	for (auto &state : m_pStates)		// // //
		if (state)
			state->SetSampleRate(SampleRate);
}

void CVisualizerWnd::FlushSamples(array_view<int16_t> Samples)		// // //
{
	if (!m_bThreadRunning)
		return;

	if (Samples.size() != m_iBufferSize) {
		m_csBuffer.Lock();
		m_iBufferSize = Samples.size();
		m_pBuffer1 = std::make_unique<short[]>(m_iBufferSize);		// // //
		m_pBuffer2 = std::make_unique<short[]>(m_iBufferSize);
		m_pFillBuffer = &m_pBuffer1;
		m_csBuffer.Unlock();
	}

	m_csBufferSelect.Lock();
	memcpy(m_pFillBuffer->get(), Samples.data(), Samples.size());
	m_csBufferSelect.Unlock();

	SetEvent(m_hNewSamples);
}

void CVisualizerWnd::ReportAudioProblem()
{
	m_bNoAudio = true;
	Invalidate();
}

UINT CVisualizerWnd::ThreadProc()
{
	DWORD nThreadID = AfxGetThread()->m_nThreadID;
	m_bThreadRunning = true;

	TRACE(L"Visualizer: Started thread (0x%04x)\n", nThreadID);

	while (m_bThreadRunning && ::WaitForSingleObject(m_hNewSamples, INFINITE) == WAIT_OBJECT_0) {

		m_bNoAudio = false;

		// Switch buffers
		m_csBufferSelect.Lock();

		auto pDrawBuffer = m_pFillBuffer->get();

		if (m_pFillBuffer == &m_pBuffer1)
			m_pFillBuffer = &m_pBuffer2;
		else
			m_pFillBuffer = &m_pBuffer1;

		m_csBufferSelect.Unlock();

		// Draw
		m_csBuffer.Lock();

		CDC *pDC = GetDC();
		if (pDC != NULL) {
			m_pStates[m_iCurrentState]->SetSampleData(pDrawBuffer, m_iBufferSize);
			m_pStates[m_iCurrentState]->Draw();
			m_pStates[m_iCurrentState]->Display(pDC, false);
			ReleaseDC(pDC);
		}

		m_csBuffer.Unlock();
	}

	TRACE(L"Visualizer: Closed thread (0x%04x)\n", nThreadID);

	return 0;
}

BOOL CVisualizerWnd::CreateEx(DWORD dwExStyle, LPCWSTR lpszClassName, LPCWSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
	// This is saved
	m_iCurrentState = Env.GetSettings()->SampleWinState;

	// Create an event used to signal that new samples are available
	m_hNewSamples = CreateEvent(NULL, FALSE, FALSE, NULL);

	BOOL Result = CWnd::CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);

	if (Result) {
		// Get client rect and create visualizers
		CRect crect;
		GetClientRect(&crect);
		for (int i = 0; i < STATE_COUNT; ++i) {
			m_pStates[i]->Create(crect.Width(), crect.Height());
		}

		// Create a worker thread
		m_pWorkerThread = AfxBeginThread(&ThreadProcFunc, (LPVOID)this, THREAD_PRIORITY_BELOW_NORMAL);
	}

	return Result;
}

void CVisualizerWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	NextState();
	CWnd::OnLButtonDown(nFlags, point);
}

void CVisualizerWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	NextState();
	CWnd::OnLButtonDblClk(nFlags, point);
}

void CVisualizerWnd::OnPaint()
{
	m_csBuffer.Lock();

	CPaintDC dc(this); // device context for painting

	if (m_bNoAudio) {
		CRect rect;
		GetClientRect(rect);
		dc.DrawTextW(L"No audio", rect, DT_CENTER | DT_VCENTER);
	}
	else
		m_pStates[m_iCurrentState]->Display(&dc, true);

	m_csBuffer.Unlock();
}

void CVisualizerWnd::OnRButtonUp(UINT nFlags, CPoint point)
{
	CMenu PopupMenuBar;
	PopupMenuBar.LoadMenuW(IDR_SAMPLE_WND_POPUP);

	CMenu *pPopupMenu = PopupMenuBar.GetSubMenu(0);

	CPoint menuPoint;
	CRect rect;

	GetWindowRect(rect);

	menuPoint.x = rect.left + point.x;
	menuPoint.y = rect.top + point.y;

	static const UINT menuIds[] = {
		ID_POPUP_SAMPLESCOPE1,
		ID_POPUP_SAMPLESCOPE2,
		ID_POPUP_SPECTRUMANALYZER,
		ID_POPUP_SPECTRUMANALYZER2,		// // //
		ID_POPUP_NOTHING
	};

	pPopupMenu->CheckMenuItem(menuIds[m_iCurrentState], MF_BYCOMMAND | MF_CHECKED);

	UINT Result = pPopupMenu->TrackPopupMenu(TPM_RETURNCMD, menuPoint.x, menuPoint.y, this);

	m_csBuffer.Lock();

	for (size_t i = 0; i < std::size(menuIds); i++)		// // //
		if (Result == menuIds[i]) {
			m_iCurrentState = i;
			break;
		}

	m_csBuffer.Unlock();

	Invalidate();
	Env.GetSettings()->SampleWinState = m_iCurrentState;

	CWnd::OnRButtonUp(nFlags, point);
}

void CVisualizerWnd::OnDestroy()
{
	// Shut down worker thread
	if (m_pWorkerThread != NULL) {
		HANDLE hThread = m_pWorkerThread->m_hThread;

		m_bThreadRunning = false;
		::SetEvent(m_hNewSamples);

		TRACE(L"Visualizer: Joining thread...\n");
		if (::WaitForSingleObject(hThread, 5000) == WAIT_OBJECT_0) {
			::CloseHandle(m_hNewSamples);
			m_hNewSamples = NULL;
			m_pWorkerThread = NULL;
			TRACE(L"Visualizer: Thread has finished.\n");
		}
		else {
			TRACE(L"Visualizer: Could not shutdown worker thread\n");
		}
	}

	CWnd::OnDestroy();
}
