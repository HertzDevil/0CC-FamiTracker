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

#include "PeriodTables.h"
#include "DetuneTable.h"
#include "Assertion.h"

unsigned CPeriodTables::ReadTable(int Index, int Table) const {
	switch (Table) {
	case CDetuneTable::DETUNE_NTSC: return ntsc_period[Index]; break;
	case CDetuneTable::DETUNE_PAL:  return pal_period[Index]; break;
	case CDetuneTable::DETUNE_SAW:  return saw_period[Index]; break;
	case CDetuneTable::DETUNE_VRC7: return vrc7_freq[Index]; break;
	case CDetuneTable::DETUNE_FDS:  return fds_freq[Index]; break;
	case CDetuneTable::DETUNE_N163: return n163_freq[Index]; break;
	case CDetuneTable::DETUNE_S5B:  return s5b_period[Index]; break;
	}
	DEBUG_BREAK();
	return ntsc_period[Index];
}
