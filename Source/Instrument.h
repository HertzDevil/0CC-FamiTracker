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

#include <string>
#include <memory>

// Instrument types
enum inst_type_t : unsigned {
	INST_NONE = 0,
	INST_2A03 = 1,
	INST_VRC6,
	INST_VRC7,
	INST_FDS,
	INST_N163,
	INST_S5B,
	INST_SN76489,		// // //
};

// External classes
class CChunk;
class CDocumentFile;
class CSimpleFile;
class CInstrumentManagerInterface;		// // // break cyclic dependencies

// Instrument base class
class CInstrument {
public:
	CInstrument(inst_type_t type);										// // // ctor with instrument type

	virtual ~CInstrument() noexcept = default;
	virtual std::unique_ptr<CInstrument> Clone() const = 0;				// // // virtual copy ctor

	std::string_view GetName() const;		// // //
	void SetName(std::string_view Name);		// // //

	void RegisterManager(CInstrumentManagerInterface *pManager);		// // //
	virtual void OnBlankInstrument();									// // // Setup some initial values

	inst_type_t GetType() const;										// // // Returns instrument type
	virtual bool CanRelease() const = 0;

	virtual void Store(CDocumentFile *pDocFile) const = 0;				// Saves the instrument to the module
	virtual bool Load(CDocumentFile *pDocFile) = 0;						// Loads the instrument from a module
	void SaveFTI(CSimpleFile &File) const;								// // // Saves to an FTI file
	void LoadFTI(CSimpleFile &File, int iVersion);						// // // Loads from an FTI file

protected:
	virtual void CloneFrom(const CInstrument *pInst);					// // // virtual copying

private:
	virtual void DoSaveFTI(CSimpleFile &File) const = 0;				// // //
	virtual void DoLoadFTI(CSimpleFile &File, int iVersion) = 0;		// // //

public:
	static const int INST_NAME_MAX = 128;

protected:
	std::string name_;		// // //
	inst_type_t m_iType;		// // //
	CInstrumentManagerInterface *m_pInstManager = nullptr;		// // //
};
