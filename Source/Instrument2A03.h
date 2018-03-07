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
#include <array>		// // //

namespace ft0cc::doc {
class dpcm_sample;
} // namespace ft0cc::doc

class CInstrument2A03 : public CSeqInstrument {
public:
	CInstrument2A03();
	std::unique_ptr<CInstrument> Clone() const override;
	void	Store(CDocumentFile *pFile) const override;
	bool	Load(CDocumentFile *pDocFile) override;

private:
	int		GetSampleCount() const;		// // // 050B

public:
	// // // Samples
	unsigned GetSampleIndex(int MidiNote) const;		// // //
	char	GetSamplePitch(int MidiNote) const;
	bool	GetSampleLoop(int MidiNote) const;
	char	GetSampleLoopOffset(int MidiNote) const;
	char	GetSampleDeltaValue(int MidiNote) const;
	void	SetSampleIndex(int MidiNote, unsigned Sample);		// // //
	void	SetSamplePitch(int MidiNote, char Pitch);
	void	SetSampleLoop(int MidiNote, bool Loop);
	void	SetSampleLoopOffset(int MidiNote, char Offset);
	void	SetSampleDeltaValue(int MidiNote, char Offset);

	bool	AssignedSamples() const;
	std::shared_ptr<ft0cc::doc::dpcm_sample> GetDSample(int MidiNote) const;		// // //

protected:
	void	CloneFrom(const CInstrument *pInst) override;		// // //

private:
	void	DoSaveFTI(CSimpleFile &File) const override;
	void	DoLoadFTI(CSimpleFile &File, int iVersion) override;

public:
	static const char *const SEQUENCE_NAME[];
	const char *GetSequenceName(int Index) const override { return SEQUENCE_NAME[Index]; }		// // //

private:
	struct DPCMAssignment {
		unsigned Index = 0u;
		uint8_t Pitch = 0x0Fu;
		uint8_t LoopOffset = 0;
		uint8_t Delta = (uint8_t)-1;
	};

	std::array<DPCMAssignment, NOTE_COUNT> m_Assignments;		// // //
};
