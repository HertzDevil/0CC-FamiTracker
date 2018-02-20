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

#include "InstrumentTypeImpl.h"

#include "Instrument2A03.h"
#include "InstrumentVRC6.h"
#include "InstrumentFDS.h"
#include "InstrumentVRC7.h"
#include "InstrumentN163.h"
#include "InstrumentS5B.h"
#include "InstrumentSN7.h"

#include "InstCompiler.h"

template <typename Inst, typename CompileT, inst_type_t ID>
inst_type_t CInstrumentTypeImpl<Inst, CompileT, ID>::GetID() const {
	return ID;
}

template<typename Inst, typename CompileT, inst_type_t ID>
std::unique_ptr<CInstrument> CInstrumentTypeImpl<Inst, CompileT, ID>::MakeInstrument() const {
	return std::make_unique<Inst>();
}

template<typename Inst, typename CompileT, inst_type_t ID>
const CInstCompiler &CInstrumentTypeImpl<Inst, CompileT, ID>::GetChunkCompiler() const {
	static CompileT compiler_ = { };
	return compiler_;
}

template class CInstrumentTypeImpl<CInstrument2A03, CInstCompilerSeq , INST_2A03>;
template class CInstrumentTypeImpl<CInstrumentVRC6, CInstCompilerSeq , INST_VRC6>;
template class CInstrumentTypeImpl<CInstrumentVRC7, CInstCompilerVRC7, INST_VRC7>;
template class CInstrumentTypeImpl<CInstrumentFDS , CInstCompilerFDS , INST_FDS >;
template class CInstrumentTypeImpl<CInstrumentN163, CInstCompilerN163, INST_N163>;
template class CInstrumentTypeImpl<CInstrumentS5B , CInstCompilerSeq , INST_S5B >;
template class CInstrumentTypeImpl<CInstrumentSN7 , CInstCompilerSeq , INST_SN76489>;

inst_type_t CInstrumentTypeNull::GetID() const {
	return INST_NONE;
}

std::unique_ptr<CInstrument> CInstrumentTypeNull::MakeInstrument() const {
	return nullptr;
}

const CInstCompiler &CInstrumentTypeNull::GetChunkCompiler() const {
	static CInstCompilerNull compiler_ = { };
	return compiler_;
}
