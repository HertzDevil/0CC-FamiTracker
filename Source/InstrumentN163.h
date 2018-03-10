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

#include "SeqInstrument.h"
#include <array>
#include "array_view.h"		// // //

class CInstrumentN163 : public CSeqInstrument {
public:
	CInstrumentN163();
	std::unique_ptr<CInstrument> Clone() const override;

public:
	unsigned GetWaveSize() const;		// // //
	void	SetWaveSize(unsigned size);
	int		GetWavePos() const;
	void	SetWavePos(int pos);
	int		GetSample(int wave, int pos) const;
	void	SetSample(int wave, int pos, int sample);
	array_view<int> GetSamples(int wave) const;		// // //
	void	SetSamples(int wave, array_view<int> buf);		// // //
	/*
	void	SetAutoWavePos(bool Enable);
	bool	GetAutoWavePos() const;
	*/
	void	SetWaveCount(int count);
	int		GetWaveCount() const;

	bool	IsWaveEqual(const CInstrumentN163 &Instrument) const;		// // //

	bool	InsertNewWave(int Index);		// // //
	bool	RemoveWave(int Index);		// // //

protected:
	void	CloneFrom(const CInstrument *pInst) override;		// // //

private:
	void	DoSaveFTI(CSimpleFile &File) const override;
	void	DoLoadFTI(CSimpleFile &File, int iVersion) override;

public:
	static const int MAX_WAVE_SIZE = 240;		// Wave size (240 samples)		// // //
	static const int MAX_WAVE_COUNT = 64;		// Number of waves

public:
	static const char *const SEQUENCE_NAME[];
	const char *GetSequenceName(int Index) const override { return SEQUENCE_NAME[Index]; }		// // //

private:
	std::array<std::array<int, MAX_WAVE_SIZE>, MAX_WAVE_COUNT> m_iSamples = { };		// // //
	unsigned m_iWaveSize;
	int		m_iWavePos;
	bool	m_bAutoWavePos;		// // // 050B
	int		m_iWaveCount;
};
