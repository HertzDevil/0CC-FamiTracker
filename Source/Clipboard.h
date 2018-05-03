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

#include "stdafx.h"		// // //
#include "array_view.h"		// // //

class CBinarySerializableInterface;		// // //

// Clipboard wrapper class, using this ensures that clipboard is closed when finished
class CClipboard
{
public:
	CClipboard(CWnd *pWnd, UINT Clipboard);
	~CClipboard();

	bool	IsOpened() const;
	bool	SetString(const CStringW &str) const;		// // //
	bool	GetData(HGLOBAL &hMemory) const;		// // //
	bool	SetDataPointer(array_view<unsigned char> data) const;		// // //
	LPVOID	GetDataPointer();
	bool	IsDataAvailable()const;

	bool	TryCopy(const CBinarySerializableInterface &res);		// // //
	bool	TryRestore(CBinarySerializableInterface &res) const;		// // //

	static bool WriteGlobalMemory(const CBinarySerializableInterface &ser, HGLOBAL hMem);
	static bool ReadGlobalMemory(CBinarySerializableInterface &ser, HGLOBAL hMem);
	static DROPEFFECT DragDropTransfer(const CBinarySerializableInterface &ser, CLIPFORMAT clipboardID, DWORD effects);

private:
	static HGLOBAL AllocateGlobalMemory(const CBinarySerializableInterface &ser);

	void	SetData(HGLOBAL hMemory) const;

	bool m_bOpened;
	UINT m_iClipboard;
	HGLOBAL m_hMemory;
};
