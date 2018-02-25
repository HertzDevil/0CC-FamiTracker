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

#include "ChipHandler.h"
#include <cstdint>

class CChipHandlerVRC7 : public CChipHandler {
public:
	void SetPatchReg(unsigned index, uint8_t val);
	void QueuePatchReg(unsigned index, uint8_t val);
	void RequestPatchUpdate();
	void WritePatch(CAPUInterface &apu);

	bool NeedsPatchUpdate() const;

private:
	void RefreshAfter(CAPUInterface &apu) override;

	// Custom instrument patch
	uint8_t patch_[8] = { };		// // // 050B
	// Each bit represents that the custom patch register on that index has been updated
	uint8_t patch_mask_ = 0u;		// // // 050B
	// True if custom instrument registers needs to be updated
	bool dirty_ = false;
};
