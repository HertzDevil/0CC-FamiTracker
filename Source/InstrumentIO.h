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

#include "ModuleException.h"

class CDocumentFile;
class CSimpleFile;
class CInstrument;

class CInstrumentIO {
public:
	explicit CInstrumentIO(module_error_level_t err_lv);
	virtual ~CInstrumentIO() noexcept = default;

	void WriteToModule(const CInstrument &inst, CDocumentFile &file, unsigned inst_index) const;
	virtual void ReadFromModule(CInstrument &inst, CDocumentFile &file) const = 0;
	void WriteToFTI(const CInstrument &inst, CSimpleFile &file) const;
	void ReadFromFTI(CInstrument &inst, CSimpleFile &file, int fti_ver) const;

private:
	virtual void DoWriteToModule(const CInstrument &inst, CDocumentFile &file) const = 0;
	virtual void DoReadFromFTI(CInstrument &inst, CSimpleFile &file, int fti_ver) const = 0;
	virtual void DoWriteToFTI(const CInstrument &inst, CSimpleFile &file) const = 0;

protected:
	template <module_error_level_t l = MODULE_ERROR_DEFAULT, typename T, typename U, typename V>
	T AssertRange(T Value, U Min, V Max, const std::string &Desc) const;

private:
	module_error_level_t err_lv_;
};

class CInstrumentIONull final : public CInstrumentIO {
public:
	using CInstrumentIO::CInstrumentIO;

private:
	void DoWriteToModule(const CInstrument &inst, CDocumentFile &file) const override;
	void ReadFromModule(CInstrument &inst, CDocumentFile &file) const override;
	void DoWriteToFTI(const CInstrument &inst, CSimpleFile &file) const override;
	void DoReadFromFTI(CInstrument &inst, CSimpleFile &file, int fti_ver) const override;
};

class CInstrumentIOSeq : public CInstrumentIO {
public:
	using CInstrumentIO::CInstrumentIO;

protected:
	void DoWriteToModule(const CInstrument &inst, CDocumentFile &file) const override;
	void ReadFromModule(CInstrument &inst, CDocumentFile &file) const override;
	void DoWriteToFTI(const CInstrument &inst, CSimpleFile &file) const override;
	void DoReadFromFTI(CInstrument &inst, CSimpleFile &file, int fti_ver) const override;
};

class CInstrumentIO2A03 : public CInstrumentIOSeq {
public:
	using CInstrumentIOSeq::CInstrumentIOSeq;

protected:
	void DoWriteToModule(const CInstrument &inst, CDocumentFile &file) const override;
	void ReadFromModule(CInstrument &inst, CDocumentFile &file) const override;
	void DoWriteToFTI(const CInstrument &inst, CSimpleFile &file) const override;
	void DoReadFromFTI(CInstrument &inst, CSimpleFile &file, int fti_ver) const override;
};

class CInstrumentIOVRC7 : public CInstrumentIO {
public:
	using CInstrumentIO::CInstrumentIO;

protected:
	void DoWriteToModule(const CInstrument &inst, CDocumentFile &file) const override;
	void ReadFromModule(CInstrument &inst, CDocumentFile &file) const override;
	void DoWriteToFTI(const CInstrument &inst, CSimpleFile &file) const override;
	void DoReadFromFTI(CInstrument &inst, CSimpleFile &file, int fti_ver) const override;
};

class CSequence;

class CInstrumentIOFDS : public CInstrumentIO {
public:
	using CInstrumentIO::CInstrumentIO;

protected:
	void DoWriteToModule(const CInstrument &inst, CDocumentFile &file) const override;
	void ReadFromModule(CInstrument &inst, CDocumentFile &file) const override;
	void DoWriteToFTI(const CInstrument &inst, CSimpleFile &file) const override;
	void DoReadFromFTI(CInstrument &inst, CSimpleFile &file, int fti_ver) const override;

	static void DoubleVolume(CSequence &seq);
};

class CInstrumentION163 : public CInstrumentIOSeq {
public:
	using CInstrumentIOSeq::CInstrumentIOSeq;

protected:
	void DoWriteToModule(const CInstrument &inst, CDocumentFile &file) const override;
	void ReadFromModule(CInstrument &inst, CDocumentFile &file) const override;
	void DoWriteToFTI(const CInstrument &inst, CSimpleFile &file) const override;
	void DoReadFromFTI(CInstrument &inst, CSimpleFile &file, int fti_ver) const override;
};
