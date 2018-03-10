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

class CDocumentFile;
class CSimpleFile;
class CInstrument;

class CInstrumentIO {
public:
	virtual ~CInstrumentIO() noexcept = default;

	virtual void WriteToModule(const CInstrument &inst, CDocumentFile &file) const = 0;
	virtual void ReadFromModule(CInstrument &inst, CDocumentFile &file) const = 0;
	virtual void WriteToFTI(const CInstrument &inst, CSimpleFile &file) const = 0;
	virtual void ReadFromFTI(CInstrument &inst, CSimpleFile &file) const = 0;
};

class CInstrumentIONull final : public CInstrumentIO {
	void WriteToModule(const CInstrument &inst, CDocumentFile &file) const override;
	void ReadFromModule(CInstrument &inst, CDocumentFile &file) const override;
	void WriteToFTI(const CInstrument &inst, CSimpleFile &file) const override;
	void ReadFromFTI(CInstrument &inst, CSimpleFile &file) const override;
};

class CInstrumentIOSeq : public CInstrumentIO {
protected:
	void WriteToModule(const CInstrument &inst, CDocumentFile &file) const override;
	void ReadFromModule(CInstrument &inst, CDocumentFile &file) const override;
	void WriteToFTI(const CInstrument &inst, CSimpleFile &file) const override;
	void ReadFromFTI(CInstrument &inst, CSimpleFile &file) const override;
};

class CInstrumentIO2A03 : public CInstrumentIOSeq {
protected:
	void WriteToModule(const CInstrument &inst, CDocumentFile &file) const override;
	void ReadFromModule(CInstrument &inst, CDocumentFile &file) const override;
	void WriteToFTI(const CInstrument &inst, CSimpleFile &file) const override;
	void ReadFromFTI(CInstrument &inst, CSimpleFile &file) const override;
};

class CInstrumentIOVRC7 : public CInstrumentIO {
protected:
	void WriteToModule(const CInstrument &inst, CDocumentFile &file) const override;
	void ReadFromModule(CInstrument &inst, CDocumentFile &file) const override;
	void WriteToFTI(const CInstrument &inst, CSimpleFile &file) const override;
	void ReadFromFTI(CInstrument &inst, CSimpleFile &file) const override;
};

class CSequence;

class CInstrumentIOFDS : public CInstrumentIO {
protected:
	void WriteToModule(const CInstrument &inst, CDocumentFile &file) const override;
	void ReadFromModule(CInstrument &inst, CDocumentFile &file) const override;
	void WriteToFTI(const CInstrument &inst, CSimpleFile &file) const override;
	void ReadFromFTI(CInstrument &inst, CSimpleFile &file) const override;

	static void DoubleVolume(CSequence &seq);
};

class CInstrumentION163 : public CInstrumentIOSeq {
protected:
	void WriteToModule(const CInstrument &inst, CDocumentFile &file) const override;
	void ReadFromModule(CInstrument &inst, CDocumentFile &file) const override;
	void WriteToFTI(const CInstrument &inst, CSimpleFile &file) const override;
	void ReadFromFTI(CInstrument &inst, CSimpleFile &file) const override;
};
