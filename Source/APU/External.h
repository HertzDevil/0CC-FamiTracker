/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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

#ifndef EXTERNAL_H
#define EXTERNAL_H

class CMixer;

class CExternal {
public:
	CExternal() {};
	CExternal(CMixer *pMixer) : m_pMixer(pMixer) {};
	virtual ~CExternal() {};

	virtual void	Reset() = 0;
	virtual void	Process(uint32_t Time) = 0;
	virtual void	EndFrame() = 0;

	virtual void	Write(uint16_t Address, uint8_t Value) = 0;
	virtual uint8_t	Read(uint16_t Address, bool &Mapped) = 0;

protected:
	CMixer *m_pMixer;
};

#endif /* EXTERNAL_H */