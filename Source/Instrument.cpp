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

#include "Instrument.h"
#include "InstrumentManagerInterface.h"		// // //
#include "SimpleFile.h"		// // //

namespace {

const std::string_view DEFAULT_INST_NAME = "New instrument";		// // //

// FTI instruments files
const std::string_view INST_HEADER = "FTI";		// // // moved
const std::string_view INST_VERSION = "2.4";

} // namespace

/*
 * Class CInstrument, base class for instruments
 *
 */

CInstrument::CInstrument(inst_type_t type) : m_iType(type)		// // //
{
}

void CInstrument::OnBlankInstrument() {		// // //
	SetName(DEFAULT_INST_NAME);
}

void CInstrument::CloneFrom(const CInstrument *pSeq)
{
	SetName(pSeq->GetName());
	m_iType = pSeq->GetType();
}

void CInstrument::SetName(std::string_view Name)		// // //
{
	name_ = Name.substr(0, INST_NAME_MAX - 1);
}

std::string_view CInstrument::GetName() const		// // //
{
	return name_;
}

void CInstrument::RegisterManager(CInstrumentManagerInterface *pManager)		// // //
{
	m_pInstManager = pManager;
}

void CInstrument::SaveFTI(CSimpleFile &File) const {
	// Write header
	File.WriteBytes(INST_HEADER);
	File.WriteBytes(INST_VERSION);

	// Write type
	File.WriteInt8(GetType());

	// Write name
	File.WriteString(GetName());

	// Write instrument data
	DoSaveFTI(File);
}

void CInstrument::LoadFTI(CSimpleFile &File, int iVersion) {		// // //
	SetName(File.ReadString());		// // //
	DoLoadFTI(File, iVersion);
}

inst_type_t CInstrument::GetType() const		// // //
{
	return m_iType;
}
