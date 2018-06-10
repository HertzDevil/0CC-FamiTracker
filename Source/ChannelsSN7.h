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


#pragma once

#include "ChannelHandler.h"

//
// Derived channels, SN76489
//

class CChannelHandlerSN7 : public CChannelHandler {		// // //
public:
	CChannelHandlerSN7(stChannelID ch);

	static void SwapChannels(stChannelID ID);

protected:
	bool HandleEffect(ft0cc::doc::effect_command cmd) override;
	bool CreateInstHandler(inst_type_t Type) override;
	void HandleEmptyNote() override;
	void HandleCut() override;
	void HandleRelease() override;
	int CalculateVolume() const override;		// // //

	void SetStereo(bool Left, bool Right) const;

protected:
	unsigned char m_cSweep;			// Sweep, used by pulse channels

	bool	m_bManualVolume;		// Flag for Exx
	int		m_iInitVolume;			// Initial volume
	// // //
	int		m_iPostEffect;
	int		m_iPostEffectParam;

	static unsigned m_iRegisterPos[3];		// // //
	static uint8_t m_cStereoFlag;		// // //
	static uint8_t m_cStereoFlagLast;		// // //
};

// Square 1
class CSN7SquareChan : public CChannelHandlerSN7 {		// // //
public:
	CSN7SquareChan(stChannelID ch) : CChannelHandlerSN7(ch) { m_iDefaultDuty = 0; }
	void RefreshChannel() override;
protected:
	void ClearRegisters() override;
};

// Noise
class CSN7NoiseChan : public CChannelHandlerSN7 {
public:
	CSN7NoiseChan(stChannelID ch);
	void RefreshChannel() override;
protected:
	bool HandleEffect(ft0cc::doc::effect_command cmd) override;
	void ClearRegisters() override;
	void HandleNote(int MidiNote) override;
	void SetupSlide() override;

	int TriggerNote(int Note) override;

private:
	int m_iLastCtrl;		// // //
	bool m_bNoiseReset;		// // //
	bool m_bTrigger;		// // // 0CC-FT has this built-in
};

// // //
