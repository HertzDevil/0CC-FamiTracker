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

// Visualizer classes

#include "stdafx.h"		// // //
#include <memory>		// // //
#include <vector>		// // //
#include "array_view.h"		// // //

class CVisualizerBase;		// // //

// CVisualizerWnd

class CVisualizerWnd : public CWnd
{
	DECLARE_DYNAMIC(CVisualizerWnd)

public:
	CVisualizerWnd();
	virtual ~CVisualizerWnd();

	HANDLE GetThreadHandle() const;		// // //

	void SetSampleRate(int SampleRate);
	void FlushSamples(array_view<int16_t> Samples);		// // //
	void ReportAudioProblem();

private:
	static UINT ThreadProcFunc(LPVOID pParam);

	void NextState();
	UINT ThreadProc();
	template <typename F>
	void LockedState(F f) const;
	template <typename F>
	void LockedBuffer(F f) const;

	std::vector<std::unique_ptr<CVisualizerBase>> m_pStates;		// // //
	unsigned int m_iCurrentState;

	std::vector<int16_t> m_pBuffer1;		// // //
	std::vector<int16_t> m_pBuffer2;

	HANDLE m_hNewSamples;

	bool m_bNoAudio;

	// Thread
	CWinThread *m_pWorkerThread = nullptr;
	bool m_bThreadRunning;

	mutable CCriticalSection m_csBufferSelect;
	mutable CCriticalSection m_csBuffer;

public:
	virtual BOOL CreateEx(DWORD dwExStyle, LPCWSTR lpszClassName, LPCWSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
};
