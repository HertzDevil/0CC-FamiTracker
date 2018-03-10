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

#include <memory>
#include <unordered_map>
#include "InstrumentType.h"

class CInstrument;
class CInstCompiler;
class CInstrumentIO;
enum inst_type_t : unsigned;

class CInstrumentService {
public:
	std::unique_ptr<CInstrument> Make(inst_type_t index) const;
	const CInstrumentIO &GetInstrumentIO(inst_type_t index) const;
	const CInstCompiler &GetChunkCompiler(inst_type_t index) const;

	void AddType(std::unique_ptr<CInstrumentType> itype);
	void AddDefaultTypes();

private:
	CInstrumentType &GetInstrumentType(inst_type_t index) const;

	std::unordered_map<inst_type_t, std::unique_ptr<CInstrumentType>> types_;
};
