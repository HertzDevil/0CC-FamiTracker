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

#include <array>		// // //
#include "ChannelHandler.h"

class CChannelHandlerFDS : public CChannelHandlerInverted, public CChannelHandlerInterfaceFDS {
public:
	explicit CChannelHandlerFDS(stChannelID ch);		// // //
	void	RefreshChannel() override;
protected:
	void	HandleNoteData(stChanNote &pNoteData) override;		// // //
	bool	HandleEffect(effect_t EffNum, unsigned char EffParam) override;		// // //
	void	HandleEmptyNote() override;
	void	HandleCut() override;
	void	HandleRelease() override;
	int		CalculateVolume() const override;		// // //
	bool	CreateInstHandler(inst_type_t Type) override;		// // //
	void	ClearRegisters() override;
	std::string	GetCustomEffectString() const override;		// // //

public:		// // //
	// FDS functions
	void SetFMSpeed(int Speed) override;
	void SetFMDepth(int Depth) override;
	void SetFMDelay(int Delay) override;
	// void SetFMEnable(bool Enable);
	void FillWaveRAM(array_view<unsigned char> Buffer) override;		// // //
	void FillModulationTable(array_view<unsigned char> Buffer) override;		// // //
protected:
	// FDS control variables
	int m_iModulationSpeed;
	int m_iModulationDepth;
	int m_iModulationDelay;
	// Modulation table
	std::array<unsigned char, 64> m_iWaveTable = { };		// // //
	std::array<unsigned char, 32> m_iModTable = { };		// // //
protected:
	int m_iVolModMode;		// // // TODO: make an enum for this
	int m_iVolModRate;
	bool m_bVolModTrigger;

	bool m_bAutoModulation;		// // //
	int m_iModulationOffset;

	int m_iEffModDepth;
	int m_iEffModSpeedHi;
	int m_iEffModSpeedLo;
};
