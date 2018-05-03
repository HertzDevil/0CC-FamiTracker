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
#include "../resource.h"		// // //
#include "BinarySerializable.h"		// // //
#include <memory>		// // //

// CClipboard //////////////////////////////////////////////////////////////////

CClipboard::CClipboard(CWnd *pWnd, UINT Clipboard) : m_bOpened(pWnd->OpenClipboard() == TRUE), m_iClipboard(Clipboard), m_hMemory(NULL)
{
}

CClipboard::~CClipboard()
{
	if (m_hMemory != NULL)
		::GlobalUnlock(m_hMemory);

	if (m_bOpened)
		::CloseClipboard();
}

bool CClipboard::IsOpened() const
{
	return m_bOpened;
}

bool CClipboard::SetString(const CStringW &str) const {		// // //
	return SetDataPointer(array_view<WCHAR>(str, str.GetLength() + 1).as_bytes());
}

void CClipboard::SetData(HGLOBAL hMemory) const
{
	ASSERT(m_bOpened);

	::EmptyClipboard();
	::SetClipboardData(m_iClipboard, hMemory);
}

bool CClipboard::SetDataPointer(array_view<unsigned char> Data) const		// // //
{
	ASSERT(m_bOpened);

	if (HGLOBAL hMemory = ::GlobalAlloc(GMEM_MOVEABLE, Data.size())) {
		if (LPVOID pClipData = ::GlobalLock(hMemory)) {
			Data.copy(reinterpret_cast<unsigned char *>(pClipData), Data.size());
			SetData(hMemory);
			::GlobalUnlock(hMemory);
			return true;
		}
	}
	return false;
}

bool CClipboard::GetData(HGLOBAL &hMemory) const		// // //
{
	ASSERT(m_bOpened);
	if (!IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return false;
	}
	if (!IsDataAvailable()) {
		AfxMessageBox(IDS_CLIPBOARD_NOT_AVALIABLE);
		::CloseClipboard();
		return false;
	}
	hMemory = ::GetClipboardData(m_iClipboard);
	if (hMemory == nullptr) {
		AfxMessageBox(IDS_CLIPBOARD_PASTE_ERROR);
		return false;
	}
	return true;
}

LPVOID CClipboard::GetDataPointer()
{
	if (!GetData(m_hMemory))		// // //
		return NULL;

	return ::GlobalLock(m_hMemory);
}

bool CClipboard::IsDataAvailable() const
{
	return ::IsClipboardFormatAvailable(m_iClipboard) == TRUE;
}

bool CClipboard::TryCopy(const CBinarySerializableInterface &res) {		// // //
	if (auto hMem = AllocateGlobalMemory(res)) {
		if (WriteGlobalMemory(res, hMem)) {
			SetData(hMem);
			return true;
		}
	}
	return false;
}

bool CClipboard::TryRestore(CBinarySerializableInterface &res) const {		// // //
	if (HGLOBAL hMem; GetData(hMem))		// // //
		return ReadGlobalMemory(res, hMem);
	return false;
}

HGLOBAL CClipboard::AllocateGlobalMemory(const CBinarySerializableInterface &ser) {
	return ::GlobalAlloc(GMEM_MOVEABLE, ser.GetAllocSize());
}

bool CClipboard::WriteGlobalMemory(const CBinarySerializableInterface &ser, HGLOBAL hMem) {
	if (ser.ContainsData())
		if (auto pByte = reinterpret_cast<std::byte *>(::GlobalLock(hMem))) {
			std::size_t sz = ::GlobalSize(hMem);
			bool result = ser.ToBytes(pByte, sz);
			::GlobalUnlock(hMem);
			return result;
		}
	return false;
}

bool CClipboard::ReadGlobalMemory(CBinarySerializableInterface &ser, HGLOBAL hMem) {
	if (auto pByte = reinterpret_cast<const std::byte *>(::GlobalLock(hMem))) {
		std::size_t sz = ::GlobalSize(hMem);
		bool result = ser.FromBytes({pByte, sz});
		::GlobalUnlock(hMem);
		return result;
	}
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
