/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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

#include "DSampleManager.h"

const unsigned int CDSampleManager::MAX_DSAMPLES = 64;

CDSampleManager::CDSampleManager() : m_pDSample(MAX_DSAMPLES)
{
}

std::shared_ptr<CDSample> CDSampleManager::GetDSample(unsigned Index)
{
	return Index < m_pDSample.size() ? m_pDSample[Index] : nullptr;
}

std::shared_ptr<CDSample> CDSampleManager::ReleaseDSample(unsigned Index) {
	return Index < m_pDSample.size() ? std::move(m_pDSample[Index]) : nullptr;
}

std::shared_ptr<const CDSample> CDSampleManager::GetDSample(unsigned Index) const
{
	return Index < m_pDSample.size() ? m_pDSample[Index] : nullptr;
}

bool CDSampleManager::SetDSample(unsigned Index, std::shared_ptr<CDSample> pSamp)
{
	if (Index >= m_pDSample.size())
		return false;
	bool Changed = m_pDSample[Index] != pSamp;
	m_pDSample[Index] = std::move(pSamp);
	return Changed;
}

bool CDSampleManager::IsSampleUsed(unsigned Index) const
{
	return Index < m_pDSample.size() && m_pDSample[Index] != nullptr;
}

unsigned int CDSampleManager::GetSampleCount() const
{
	unsigned int Count = 0;
	for (const auto &x : m_pDSample)
		if (x)
			++Count;
	return Count;
}

unsigned int CDSampleManager::GetFirstFree() const
{
	for (size_t i = 0; i < MAX_DSAMPLES; ++i)
		if (!m_pDSample[i])
			return i;
	return -1;
}

unsigned int CDSampleManager::GetTotalSize() const
{
	unsigned int Size = 0;
	for (const auto &x : m_pDSample)
		if (x)
			Size += x->GetSize();
	return Size;
}

