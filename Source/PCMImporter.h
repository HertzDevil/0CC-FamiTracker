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

#include "stdafx.h" // TODO: remove
#include <memory>
#include "ft0cc/cpputil/fs.hpp"

namespace ft0cc::doc {
class dpcm_sample;
} // namespace ft0cc::doc

namespace jarh {
class sinc;
} // namespace jarh

class CPCMImporter {
public:
	CPCMImporter();

	bool OpenWaveFile(const fs::path &path);
	std::shared_ptr<ft0cc::doc::dpcm_sample> ConvertFile(int dB, int quality);

	int GetWaveSampleRate() const;
	int GetWaveSampleSize() const;
	int GetWaveChannelCount() const;

private:
	CFile m_fSampleFile;
	ULONGLONG m_ullSampleStart;
	unsigned int m_iWaveSize;

	int m_iSampleSize;
	int m_iChannels;
	int m_iBlockAlign;
	int m_iAvgBytesPerSec;
	int m_iSamplesPerSec;

	std::unique_ptr<jarh::sinc> m_psinc;
};
