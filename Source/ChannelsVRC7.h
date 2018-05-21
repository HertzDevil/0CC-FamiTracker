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

//
// Derived channels, VRC7
//

enum vrc7_command_t {
	CMD_NONE,
	CMD_NOTE_ON,
	CMD_NOTE_TRIGGER,
	CMD_NOTE_OFF,
	CMD_NOTE_HALT,
	CMD_NOTE_RELEASE,
};

class CChipHandlerVRC7;		// // //

class CChannelHandlerVRC7 : public CChannelHandlerInverted, public CChannelHandlerInterfaceVRC7 {		// // //
public:
	CChannelHandlerVRC7(stChannelID ch, CChipHandlerVRC7 &parent);		// // //

	void	SetPatch(unsigned char Patch);		// // //
	void	SetCustomReg(size_t Index, unsigned char Val);		// // //

protected:
	void	HandleNoteData(ft0cc::doc::pattern_note &pNoteData) override;		// // //
	bool	HandleEffect(ft0cc::doc::effect_command cmd) override;		// // //
	void	HandleEmptyNote() override;
	void	HandleCut() override;
	void	HandleRelease() override;
	void	HandleNote(int MidiNote) override;
	void	RunNote(int MidiNote) override;		// // //
	bool	CreateInstHandler(inst_type_t Type) override;		// // //
	void	SetupSlide() override;		// // //
	int		CalculateVolume() const override;
	int		CalculatePeriod() const override;		// // //

	void	UpdateNoteRelease() override;		// // //
	int		TriggerNote(int Note) override;

	void	RefreshChannel() override;		// // //
	void	ClearRegisters() override;

protected:
	void CorrectOctave();		// // //
	unsigned int GetFnum(int Note) const;
	void RegWrite(unsigned char Reg, unsigned char Value);

protected:
	int		m_iTriggeredNote = 0;
	int		m_iOctave;
	int		m_iOldOctave;		// // //
	int		m_iCustomPort;		// // // 050B

	CChipHandlerVRC7 &chip_handler_;

	vrc7_command_t m_iCommand = CMD_NONE;
	char	m_iPatch;
	bool	m_bHold;
};
