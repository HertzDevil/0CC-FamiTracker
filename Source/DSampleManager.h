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

#include <vector>
#include <memory>

namespace ft0cc::doc {
class dpcm_sample;
} // namespace ft0cc::doc

class CDSampleManager
{
	using dpcm_sample = ft0cc::doc::dpcm_sample;

public:
	CDSampleManager();
	std::shared_ptr<dpcm_sample> GetDSample(unsigned Index);
	std::shared_ptr<dpcm_sample> ReleaseDSample(unsigned Index);
	std::shared_ptr<const dpcm_sample> GetDSample(unsigned Index) const;
	bool SetDSample(unsigned Index, std::shared_ptr<dpcm_sample> pSamp);

	bool IsSampleUsed(unsigned Index) const;
	unsigned int GetSampleCount() const;
	unsigned int GetFirstFree() const;
	unsigned int GetTotalSize() const;

	static const unsigned MAX_DSAMPLES;

private:
	std::vector<std::shared_ptr<dpcm_sample>> m_pDSample;
};
