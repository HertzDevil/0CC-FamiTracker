/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2013  Jonathan Liss
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

#include "DSample.h"

using dpcm_sample = ft0cc::doc::dpcm_sample;

/*
 * CDSample
 *
 * This class is used to store delta encoded samples (DPCM).
 *
 */

CDSample::CDSample(unsigned int Size) :
	s_(Size)
{
}

CDSample::CDSample(unsigned int Size, std::unique_ptr<char[]> pData) :
	s_(Size)
{
	for (unsigned i = 0; i < Size; ++i)
		s_.set_sample_at(i, pData[i]);
}

bool CDSample::operator==(const CDSample &other) const {		// // //
	return s_ == other.s_;
}

void CDSample::SetData(unsigned int Size, std::unique_ptr<char[]> pData)		// // //
{
	*this = CDSample(Size, std::move(pData));
}

unsigned int CDSample::GetSize() const
{
	return s_.size();
}

const unsigned char *CDSample::GetData() const
{
	return s_.data();
}

void CDSample::SetName(const char *pName)
{
	s_.rename(pName);
}

const char *CDSample::GetName() const
{
	return s_.name().data();
}

void CDSample::RemoveData(int startsample, int endsample) {		// // //
	s_.cut_samples(startsample, endsample);
}

void CDSample::Tilt(int startsample, int endsample) {		// // //
	s_.tilt(startsample, endsample);
}
