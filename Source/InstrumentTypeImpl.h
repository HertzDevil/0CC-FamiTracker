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

#include "InstrumentType.h"
#include "Instrument.h" // inst_type_t

template <typename Inst, typename IOT, typename CompileT, inst_type_t ID>
class CInstrumentTypeImpl : public CInstrumentType {
	inst_type_t GetID() const override;
	std::unique_ptr<CInstrument> MakeInstrument() const override;
	std::unique_ptr<CInstrumentIO> GetInstrumentIO(module_error_level_t err_lv) const override;
	const CInstCompiler &GetChunkCompiler() const override;
};

class CInstrumentTypeNull final : public CInstrumentType {
	inst_type_t GetID() const override;
	std::unique_ptr<CInstrument> MakeInstrument() const override;
	std::unique_ptr<CInstrumentIO> GetInstrumentIO(module_error_level_t err_lv) const override;
	const CInstCompiler &GetChunkCompiler() const override;
};

class CInstrument2A03;
class CInstrumentVRC6;
class CInstrumentVRC7;
class CInstrumentFDS ;
class CInstrumentN163;
class CInstrumentS5B ;
class CInstrumentSN7;

class CInstrumentIOSeq;
class CInstrumentIO2A03;
class CInstrumentIOVRC7;
class CInstrumentIOFDS;
class CInstrumentION163;

class CInstCompilerSeq;
class CInstCompilerVRC7;
class CInstCompilerFDS;
class CInstCompilerN163;

extern template class CInstrumentTypeImpl<CInstrument2A03, CInstrumentIO2A03, CInstCompilerSeq , INST_2A03>;
extern template class CInstrumentTypeImpl<CInstrumentVRC6, CInstrumentIOSeq , CInstCompilerSeq , INST_VRC6>;
extern template class CInstrumentTypeImpl<CInstrumentVRC7, CInstrumentIOVRC7, CInstCompilerVRC7, INST_VRC7>;
extern template class CInstrumentTypeImpl<CInstrumentFDS , CInstrumentIOFDS , CInstCompilerFDS , INST_FDS >;
extern template class CInstrumentTypeImpl<CInstrumentN163, CInstrumentION163, CInstCompilerN163, INST_N163>;
extern template class CInstrumentTypeImpl<CInstrumentS5B , CInstrumentIOSeq , CInstCompilerSeq , INST_S5B >;
extern template class CInstrumentTypeImpl<CInstrumentSN7 , CInstrumentIOSeq , CInstCompilerSeq , INST_SN76489>;

using CInstrumentType2A03 = CInstrumentTypeImpl<CInstrument2A03, CInstrumentIO2A03, CInstCompilerSeq , INST_2A03>;
using CInstrumentTypeVRC6 = CInstrumentTypeImpl<CInstrumentVRC6, CInstrumentIOSeq , CInstCompilerSeq , INST_VRC6>;
using CInstrumentTypeVRC7 = CInstrumentTypeImpl<CInstrumentVRC7, CInstrumentIOVRC7, CInstCompilerVRC7, INST_VRC7>;
using CInstrumentTypeFDS  = CInstrumentTypeImpl<CInstrumentFDS , CInstrumentIOFDS , CInstCompilerFDS , INST_FDS >;
using CInstrumentTypeN163 = CInstrumentTypeImpl<CInstrumentN163, CInstrumentION163, CInstCompilerN163, INST_N163>;
using CInstrumentTypeS5B  = CInstrumentTypeImpl<CInstrumentS5B , CInstrumentIOSeq , CInstCompilerSeq , INST_S5B >;
using CInstrumentTypeSN7  = CInstrumentTypeImpl<CInstrumentSN7 , CInstrumentIOSeq , CInstCompilerSeq , INST_SN76489>;
