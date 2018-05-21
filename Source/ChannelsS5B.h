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

#include "ChannelHandler.h"

class CChipHandlerS5B;		// // //

//
// Derived channels, 5B
//

class CChannelHandlerS5B : public CChannelHandler, public CChannelHandlerInterfaceS5B {
public:
	CChannelHandlerS5B(stChannelID ch, CChipHandlerS5B &parent);		// / //
	void	ResetChannel() override;
	void	RefreshChannel() override;

	void	SetNoiseFreq(int Pitch) override final;		// // //

protected:
	// // //
	bool	HandleEffect(ft0cc::doc::effect_command cmd) override;		// // //
	void	HandleNote(int MidiNote) override;		// // //
	void	HandleEmptyNote() override;
	void	HandleCut() override;
	void	HandleRelease() override;
	bool	CreateInstHandler(inst_type_t Type) override;		// // //

	int		CalculateVolume() const override;		// // //
	int		ConvertDuty(int Duty) const override;		// // //
	void	ClearRegisters() override;
	std::string	GetCustomEffectString() const override;		// // //

protected:
	void WriteReg(int Reg, int Value);
	void UpdateAutoEnvelope(int Period);		// // // 050B

	// Instance members
protected:
	CChipHandlerS5B &chip_handler_;

	bool m_bEnvelopeEnabled;		// // // 050B
	int m_iAutoEnvelopeShift;		// // // 050B
	bool m_bUpdate;
};
