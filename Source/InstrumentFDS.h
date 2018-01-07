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

#include "SeqInstrument.h"		// // //
#include <vector>
#include <memory>

class CInstrumentFDS : public CSeqInstrument {
public:
	CInstrumentFDS();
	std::unique_ptr<CInstrument> Clone() const override;
	void	Store(CDocumentFile *pDocFile) const override;
	bool	Load(CDocumentFile *pDocFile) override;
	bool	CanRelease() const override;

public:
	unsigned char GetSample(int Index) const;
	void	SetSample(int Index, int Sample);
	int		GetModulationSpeed() const;
	void	SetModulationSpeed(int Speed);
	int		GetModulation(int Index) const;
	void	SetModulation(int Index, int Value);
	int		GetModulationDepth() const;
	void	SetModulationDepth(int Depth);
	int		GetModulationDelay() const;
	void	SetModulationDelay(int Delay);
	bool	GetModulationEnable() const;
	void	SetModulationEnable(bool Enable);

protected:
	void	CloneFrom(const CInstrument *pInst) override;		// // //

private:
	void StoreSequence(CDocumentFile &DocFile, const CSequence &Seq) const;		// // //
	std::shared_ptr<CSequence> LoadSequence(CDocumentFile &DocFile, unsigned SeqType) const;
	void StoreInstSequence(CSimpleFile &File, const CSequence &Seq) const;		// // //
	std::shared_ptr<CSequence> LoadInstSequence(CSimpleFile &File, unsigned SeqType) const;		// // //
	void DoubleVolume() const;		// // //

	void	OnBlankInstrument() override;		// // //
	void	DoSaveFTI(CSimpleFile &File) const override;
	void	DoLoadFTI(CSimpleFile &File, int iVersion) override;

public:
	static const int WAVE_SIZE = 64;
	static const int MOD_SIZE = 32;
	static const int SEQUENCE_COUNT = 3;		// // //
	static const char *const SEQUENCE_NAME[];
	const char *GetSequenceName(int Index) const override { return SEQUENCE_NAME[Index]; }		// // //

private:
	// Instrument data
	std::vector<std::shared_ptr<CSequence>> m_pSequence;
	unsigned char m_iSamples[64];
	unsigned char m_iModulation[32];
	int			  m_iModulationSpeed;
	int			  m_iModulationDepth;
	int			  m_iModulationDelay;
	bool		  m_bModulationEnable;

public: // // // porting CSeqInstrument
	int		GetSeqEnable(int Index) const override;
	int		GetSeqIndex(int Index) const override;
	void	SetSeqIndex(int Index, int Value) override;
	std::shared_ptr<CSequence> GetSequence(int SeqType) const override;		// // //
	void	SetSequence(int SeqType, std::shared_ptr<CSequence> pSeq) override;		// // //
};
