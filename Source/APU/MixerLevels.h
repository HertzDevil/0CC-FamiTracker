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


#pragma once

//#define LINEAR_MIXING

enum chan_id_t;

struct stLevels2A03SS {
	void Offset(chan_id_t ChanID, int Value);
	double CalcPin() const;

private:
	int sq1_ = 0;
	int sq2_ = 0;
};

struct stLevels2A03TND {
	void Offset(chan_id_t ChanID, int Value);
	double CalcPin() const;

private:
	int tri_ = 0;
	int noi_ = 0;
	int dmc_ = 0;
};

struct stLevelsMono {
	void Offset(chan_id_t ChanID, int Value);
	double CalcPin() const;

private:
	int lvl_ = 0;
};
