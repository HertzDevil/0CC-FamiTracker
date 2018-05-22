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

#include "VisualizerSpectrum.h"
#include "Graphics.h"
#include "Color.h"		// // //

/*
 * Displays a spectrum analyzer
 *
 */

CVisualizerSpectrum::CVisualizerSpectrum(int Size) :		// // //
	m_iBarSize(Size)
{
}

void CVisualizerSpectrum::Create(int Width, int Height)
{
	CVisualizerBase::Create(Width, Height);

	std::fill(m_pBlitBuffer.get(), m_pBlitBuffer.get() + Width * Height, BG_COLOR);		// // //
}

void CVisualizerSpectrum::SetSampleRate(int SampleRate)
{
	fft_buffer_ = FftBuffer<FFT_POINTS> { };		// // //

	m_fFftPoint.fill(0.f);		// // //

	m_pSamples = { };		// // //
	m_iFillPos = 0;
}

void CVisualizerSpectrum::SetSampleData(array_view<const int16_t> Samples)		// // //
{
	CVisualizerBase::SetSampleData(Samples);

	fft_buffer_.CopyIn(Samples.begin(), Samples.size());
	fft_buffer_.Transform();
}

void CVisualizerSpectrum::Draw()
{
	const float SCALING = 250.0f;
	const int OFFSET = 0;
	const float DECAY = 3.0f;

	float Step = 0.2f * (float(FFT_POINTS) / float(m_iWidth)) * m_iBarSize;		// // //
	float Pos = 2;	// Add a small offset to remove note on/off actions

	int LastStep = 0;

	for (int i = 0; i < m_iWidth / m_iBarSize; ++i) {		// // //
		int iStep = int(Pos + 0.5f);

		float level = 0;
		int steps = (iStep - LastStep) + 1;
		for (int j = 0; j < steps; ++j)
			level += float(fft_buffer_.GetIntensity(LastStep + j)) / SCALING;
		level /= steps;

		// linear -> db
		level = (20 * std::log10(level));// *0.8f;

		if (level < 0.0f)
			level = 0.0f;
		if (level > float(m_iHeight))
			level = float(m_iHeight);

		if (iStep != LastStep) {
			if (level >= m_fFftPoint[iStep])
				m_fFftPoint[iStep] = level;
			else
				m_fFftPoint[iStep] -= DECAY;

			if (m_fFftPoint[iStep] < 1.0f)
				m_fFftPoint[iStep] = 0.0f;
		}

		level = m_fFftPoint[iStep];

		for (int y = 0; y < m_iHeight; ++y) {
			COLORREF Color = BLEND(MakeRGB(255, 96, 96), WHITE, y / (level + 1));
			if (y == 0)
				Color = DIM(Color, .9);
			if (m_iBarSize > 1 && (y & 1))		// // //
				Color = DIM(Color, .4);
			for (int x = 0; x < m_iBarSize; ++x) {		// // //
				if (m_iBarSize > 1 && x == m_iBarSize - 1)
					Color = DIM(Color, .5);
				m_pBlitBuffer[(m_iHeight - 1 - y) * m_iWidth + i * m_iBarSize + x + OFFSET] = y < level ? Color : BG_COLOR;
			}
		}

		LastStep = iStep;
		Pos += Step;
	}
}
