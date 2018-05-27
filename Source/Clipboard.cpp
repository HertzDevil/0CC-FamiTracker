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

#include "Clipboard.h"
#include "BinarySerializable.h"		// // //

CClipboard::CClipboardHandle::CClipboardHandle(CWnd *parent) :
	suc_(parent->OpenClipboard())
{
}

CClipboard::CClipboardHandle::~CClipboardHandle() {
	if (static_cast<bool>(*this))
		::CloseClipboard();
}

CClipboard::CClipboardHandle::operator bool() const noexcept {
	return suc_ == TRUE;
}



CClipboard::CMemoryLock::CMemoryLock(HGLOBAL hMem) :
	hmem_(hMem), data_(reinterpret_cast<std::byte *>(::GlobalLock(hMem)))
{
}

CClipboard::CMemoryLock::~CMemoryLock() {
	if (data_)
		::GlobalUnlock(hmem_);
}

CClipboard::CMemoryLock::operator bool() const noexcept {
	return data_ != nullptr;
}

array_view<std::byte> CClipboard::CMemoryLock::Buffer() const noexcept {
	return {data_, hmem_ && data_ ? ::GlobalSize(hmem_) : 0u};
}



namespace {

HGLOBAL AllocateGlobalMemory(const CBinarySerializableInterface &ser) {
	return ::GlobalAlloc(GMEM_MOVEABLE, ser.GetAllocSize());
}

bool WriteGlobalMemory(const CBinarySerializableInterface &ser, HGLOBAL hMem) {
	if (ser.ContainsData())
		if (auto pByte = CClipboard::CMemoryLock {hMem})
			return ser.ToBytes(pByte.Buffer());
	return false;
}

} // namespace



bool CClipboard::CopyToClipboard(CWnd *parent, CLIPFORMAT ClipboardID, const CBinarySerializableInterface &ser) {
	if (auto hnd = CClipboardHandle {parent}) {
		if (auto hMem = AllocateGlobalMemory(ser)) {
			if (WriteGlobalMemory(ser, hMem)) {
				::EmptyClipboard();
				::SetClipboardData(ClipboardID, hMem);
				return true;
			}
		}
		AfxMessageBox(IDS_CLIPBOARD_COPY_ERROR);
		return false;
	}
	AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
	return false;
}

bool CClipboard::ReadGlobalMemory(CBinarySerializableInterface &ser, HGLOBAL hMem) {
	if (auto pByte = CMemoryLock {hMem})
		return ser.FromBytes(pByte.Buffer());
	return false;
}

DROPEFFECT CClipboard::DragDropTransfer(const CBinarySerializableInterface &ser, CLIPFORMAT clipboardID, DWORD effects) {
	DROPEFFECT res = DROPEFFECT_NONE;
	if (HGLOBAL hMem = AllocateGlobalMemory(ser)) {
		if (WriteGlobalMemory(ser, hMem)) {
			// Setup OLE
			COleDataSource Src;
			Src.CacheGlobalData(clipboardID, hMem);
			res = Src.DoDragDrop(effects); // calls DropData
		}
		::GlobalFree(hMem);
	}
	return res;
}
