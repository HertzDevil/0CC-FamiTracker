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

#include "SequenceManager.h"
#include "SequenceCollection.h"

CSequenceManager::CSequenceManager(int Count)
{
	for (int i = 0; i < Count; ++i) {
		auto s = static_cast<sequence_t>(i); // TODO: remove
		m_pCollection.emplace(s, std::make_unique<CSequenceCollection>(s));
	}
}

int CSequenceManager::GetCount() const
{
	return m_pCollection.size();
}

CSequenceCollection *CSequenceManager::GetCollection(sequence_t Index)
{
	return m_pCollection[Index].get();
}

const CSequenceCollection *CSequenceManager::GetCollection(sequence_t Index) const
{
	auto it = m_pCollection.find(Index);
	return it != m_pCollection.end() ? it->second.get() : nullptr;
}
