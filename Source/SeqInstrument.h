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

#include "Instrument.h"
#include "FamiTrackerTypes.h"
#include <memory>
#include <unordered_map>

class CSequence;
enum class sequence_t : unsigned;

class CSeqInstrument : public CInstrument		// // //
{
public:
	CSeqInstrument(inst_type_t type);
	std::unique_ptr<CInstrument> Clone() const override;
	void	Store(CDocumentFile *pDocFile) const override;
	void	Load(CDocumentFile *pDocFile) override;
	bool	CanRelease() const override;

	virtual bool	GetSeqEnable(sequence_t SeqType) const;		// // //
	virtual void	SetSeqEnable(sequence_t SeqType, bool Enable);
	virtual int		GetSeqIndex(sequence_t SeqType) const;
	virtual void	SetSeqIndex(sequence_t SeqType, int Value);

	virtual std::shared_ptr<CSequence> GetSequence(sequence_t SeqType) const;		// // //
	virtual void	SetSequence(sequence_t SeqType, std::shared_ptr<CSequence> pSeq);		// // // register sequence in document

	// static const int SEQUENCE_TYPES[] = {sequence_t::Volume, sequence_t::Arpeggio, sequence_t::Pitch, sequence_t::HiPitch, sequence_t::DutyCycle};
	virtual const char *GetSequenceName(int Index) const { return nullptr; }		// // //

protected:
	void	OnBlankInstrument() override;		// // //
	void	CloneFrom(const CInstrument *pSeq) override;		// // //
	CSeqInstrument *CopySequences(const CSeqInstrument *const src);		// // //

	void	DoSaveFTI(CSimpleFile &File) const override;
	void	DoLoadFTI(CSimpleFile &File, int iVersion) override;

private:
	std::unordered_map<sequence_t, std::pair<bool, int>> seq_indices_;		// // //
};
