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

#include <optional>		// // //
#include "stdafx.h"		// // //
#include "../resource.h"		// // //
#include "array_view.h"		// // //

class CBinarySerializableInterface;

namespace CClipboard {

// Clipboard wrapper class, using this ensures that clipboard is closed when finished
class CClipboardHandle {
public:
	explicit CClipboardHandle(CWnd *parent);
	~CClipboardHandle();
	explicit operator bool() const noexcept;

private:
	BOOL suc_ = FALSE;
};

class CMemoryLock {
public:
	explicit CMemoryLock(HGLOBAL hMem);
	~CMemoryLock();
	explicit operator bool() const noexcept;

	std::byte *data() const noexcept;
	std::size_t size() const;

private:
	HGLOBAL hmem_;
	std::byte *data_ = nullptr;
};

bool CopyToClipboard(CWnd *parent, CLIPFORMAT ClipboardID, const CBinarySerializableInterface &ser);
bool ReadGlobalMemory(CBinarySerializableInterface& ser, HGLOBAL hMem);

template <typename T>
std::optional<T> RestoreFromClipboard(CWnd *parent, CLIPFORMAT ClipboardID) {
	if (auto hnd = CClipboardHandle {parent}) {
		if (::IsClipboardFormatAvailable(ClipboardID) == TRUE) {
			if (HGLOBAL hMem = ::GetClipboardData(ClipboardID))
				if (auto x = std::make_optional<T>(); ReadGlobalMemory(*x, hMem))
					return x;

			AfxMessageBox(IDS_CLIPBOARD_PASTE_ERROR);
			return std::nullopt;
		}
		AfxMessageBox(IDS_CLIPBOARD_NOT_AVALIABLE);
		return std::nullopt;
	}
	AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
	return std::nullopt;
}

DROPEFFECT DragDropTransfer(const CBinarySerializableInterface &ser, CLIPFORMAT clipboardID, DWORD effects);

} // namespace CClipboard
