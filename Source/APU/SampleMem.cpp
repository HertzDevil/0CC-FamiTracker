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

#include "SampleMem.h"

uint8_t CSampleMem::ReadMem(uint16_t Address) const {
	uint16_t Addr = (Address - 0xC000);// % m_iMemSize;
	if (Addr >= m_pMemory.size())		// // //
		return 0;
	return m_pMemory[Addr];
};

void CSampleMem::SetMem(array_view<uint8_t> Memory) {
	m_pMemory = Memory;
};

void CSampleMem::Clear() {
	m_pMemory.clear();
}
