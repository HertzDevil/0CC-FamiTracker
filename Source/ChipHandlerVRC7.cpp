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

#include "ChipHandlerVRC7.h"
#include "ChannelsVRC7.h"
#include "APU/APUInterface.h"
#include <iterator>

void CChipHandlerVRC7::SetPatchReg(unsigned index, uint8_t val) {
	if (index < std::size(patch_) && !(patch_mask_ & (1 << index)))		// // // 050B
		patch_[index] = val;
}

void CChipHandlerVRC7::QueuePatchReg(unsigned index, uint8_t val) {
	if (index < std::size(patch_)) {
		patch_[index] = val;
		patch_mask_ |= 1 << index;
		RequestPatchUpdate();
	}
}

void CChipHandlerVRC7::RequestPatchUpdate() {
	dirty_ = true;
}

void CChipHandlerVRC7::RefreshAfter(CAPUInterface &apu) {
	CChipHandler::RefreshAfter(apu);

	if (dirty_) {
		for (unsigned i = 0; i < std::size(patch_); ++i) {
			apu.Write(0x9010, i);
			apu.Write(0x9030, patch_[i]);
		}
		dirty_ = false;
	}
	patch_mask_ = 0u;
}
