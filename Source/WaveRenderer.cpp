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

#include "WaveRenderer.h"
#include "WaveFile.h"
#include "NumConv.h"

CWaveRenderer::~CWaveRenderer() {
	CloseOutputFile();
}

void CWaveRenderer::SetOutputFile(std::unique_ptr<CWaveFile> pWave) {
	pWave.swap(m_pWaveFile);
}

void CWaveRenderer::CloseOutputFile() {
	if (m_pWaveFile) {
		m_pWaveFile->CloseFile();
		m_pWaveFile.reset();
	}
}

void CWaveRenderer::FlushBuffer(char *pBuf, unsigned Size) const {
	if (m_pWaveFile)
		m_pWaveFile->WriteWave(pBuf, Size);
}

void CWaveRenderer::Start() {
	m_bStarted = true;
}

bool CWaveRenderer::ShouldStartPlayer() {
	return m_iDelayedStart > 0 && !--m_iDelayedStart;
}

bool CWaveRenderer::ShouldStopPlayer() const {
	return m_bRequestRenderStop || m_bStoppingRender;
}

bool CWaveRenderer::ShouldStopRender() {
	if (m_bRequestRenderStop)
		m_bStoppingRender = true;
	if (m_bStoppingRender) {
		if (!m_iDelayedEnd)
			return true;
		else
			--m_iDelayedEnd;
	}
	return false;
}

bool CWaveRenderer::Started() const {
	return m_bStarted;
}

bool CWaveRenderer::Finished() const {
	return m_bFinished;
}

void CWaveRenderer::SetRenderTrack(int Track) {
	m_iRenderTrack = Track;
}

int CWaveRenderer::GetRenderTrack() const {
	return m_iRenderTrack;
}

void CWaveRenderer::FinishRender() {
	m_bRequestRenderStop = true;
}



CWaveRendererTick::CWaveRendererTick(unsigned Ticks, double Rate) :
	CWaveRenderer(), m_iTicksToRender(Ticks), m_fFrameRate(Rate)
{
}

void CWaveRendererTick::Tick() {
	if (m_iRenderTick == m_iTicksToRender)		// // //
		FinishRender();
	else
		++m_iRenderTick;
}

std::string CWaveRendererTick::GetProgressString() const {
	return "Time: " + conv::time_from_uint(static_cast<unsigned>(m_iRenderTick / m_fFrameRate)) +
		" / " + conv::time_from_uint(static_cast<unsigned>(m_iTicksToRender / m_fFrameRate)) +
		" (" + conv::from_int(GetProgressPercent()) + "% done)";
}

int CWaveRendererTick::GetProgressPercent() const {
	return Finished() ? 100 : m_iRenderTick * 100 / m_iTicksToRender;
}



CWaveRendererRow::CWaveRendererRow(unsigned Rows) :
	CWaveRenderer(), m_iRowsToRender(Rows)
{
}

void CWaveRendererRow::StepRow() {
	if (m_iRenderRow == m_iRowsToRender)		// // //
		FinishRender();
	else
		++m_iRenderRow;
}

std::string CWaveRendererRow::GetProgressString() const {
	return "Row: " + std::to_string(m_iRenderRow) + " / " + std::to_string(m_iRowsToRender) +
		" (" + std::to_string(GetProgressPercent()) + "% done)";
}

int CWaveRendererRow::GetProgressPercent() const {
	return Finished() ? 100 : m_iRenderRow * 100 / m_iRowsToRender;
}
