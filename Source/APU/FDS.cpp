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

#include "APU/FDS.h"
#include "APU/APU.h"
#include "RegisterState.h"		// // //
#include "APU/ext/FDSSound_new.h"		// // //
#include "APU/Types.h"		// // //

// FDS interface, actual FDS emulation is in FDSSound.cpp

CFDS::CFDS(CMixer &Mixer) :
	CChannel(Mixer, sound_chip_t::FDS, chan_id_t::FDS),
	CSoundChip(Mixer),
	emu_(std::make_unique<xgm::NES_FDS>())
{
	m_pRegisterLogger->AddRegisterRange(0x4040, 0x408F);		// // //
}

CFDS::~CFDS()
{
}

void CFDS::Reset()
{
	emu_->Reset();
}

void CFDS::Write(uint16_t Address, uint8_t Value)
{
	emu_->Write(Address, Value);
}

uint8_t CFDS::Read(uint16_t Address, bool &Mapped)
{
	uint32_t val = 0;
	Mapped = emu_->Read(Address, val);
	return static_cast<uint8_t>(val);
}

void CFDS::EndFrame()
{
	CChannel::EndFrame();
}

void CFDS::Process(uint32_t Time)
{
	if (!Time)
		return;

	const uint32_t TIME_STEP = 32u; // ???

	while (Time) {
		const uint32_t t = Time < TIME_STEP ? Time : TIME_STEP;
		emu_->Tick(t);
		Mix(emu_->Render());
		m_iTime += t;
		Time -= t;
	}
}

double CFDS::GetFreq(int Channel) const		// // //
{
	if (Channel) return 0.;
	int Lo = m_pRegisterLogger->GetRegister(0x4082)->GetValue();
	int Hi = m_pRegisterLogger->GetRegister(0x4083)->GetValue();
	if (Hi & 0x80)
		return 0.;
	Lo |= (Hi << 8) & 0xF00;
	return CAPU::BASE_FREQ_NTSC * (Lo / 4194304.);
}
