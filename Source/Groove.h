/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
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


#pragma once

#include "ft0cc/doc/groove.hpp"

#define MAX_GROOVE_SIZE 128

class CGroove : public ft0cc::doc::groove
{
public:
	CGroove(int Speed = 0) : groove(Speed) { }

	void Copy(const CGroove *Source) { return groove::copy(Source); }
	void Clear(unsigned char Speed) { return groove::clear(Speed); }
	unsigned char GetEntry(int Index) const { return groove::entry(Index); }
	void SetEntry(unsigned char Index, unsigned char Value) { return groove::set_entry(Index, Value); }
	unsigned char GetSize() const { return groove::size(); }
	void SetSize(unsigned char Size) { return groove::resize(Size); }
	float GetAverage() const { return groove::average(); }
};