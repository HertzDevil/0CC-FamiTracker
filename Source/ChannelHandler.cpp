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

//
// This is the base class for all classes that takes care of
// translating notes to channel register writes.
//

#include "ChannelHandler.h"
#include "SongState.h"		// // //
#include "InstrumentManager.h"
#include "Instrument.h"		// // //
#include "TrackerChannel.h"		// // //
#include "APU/Types.h"		// // //
#include "SoundGenBase.h"		// // //
#include "FamiTrackerModule.h"		// // //
#include "FamiTrackerEnv.h"		// // //
#include "Settings.h"		// // //
#include "APU/APUInterface.h"		// // //
#include "InstHandler.h"		// // //
#include "NumConv.h"		// // //
#include <algorithm>		// // //

/*
 * Class CChannelHandler
 *
 */

CChannelHandler::CChannelHandler(stChannelID ch, int MaxPeriod, int MaxVolume) :		// // //
	m_iChannelID(ch),		// // //
	m_iInstTypeCurrent(INST_NONE),		// // //
	m_iMaxPeriod(MaxPeriod),
	m_iMaxVolume(MaxVolume)
{
}

CChannelHandler::~CChannelHandler()
{
}

void CChannelHandler::InitChannel(CAPUInterface &apu, array_view<int> VibTable, CSoundGenBase *pSoundGen)		// // //
{
	// Called from main thread

	m_pAPU = &apu;
	if (VibTable.size() == 256u)		// // //
		m_iVibratoTable = VibTable;
	m_pSoundGen = pSoundGen;

	m_bDelayEnabled = false;
}

void CChannelHandler::ConfigureDocument(const CFamiTrackerModule &modfile) {		// // //
	SetVibratoStyle(modfile.GetVibratoStyle());
	SetLinearPitch(modfile.GetLinearPitch());
}

void CChannelHandler::SetLinearPitch(bool bEnable)		// // //
{
	m_bLinearPitch = bEnable;
}

void CChannelHandler::SetVibratoStyle(vibrato_t Style)		// // //
{
	m_iVibratoMode = Style;
}

void CChannelHandler::SetPitch(int Pitch)
{
	// Pitch ranges from -511 to +512
	m_iPitch = std::clamp(Pitch, -511, 511);		// // //
}


/*!	\brief Retrieves the identifier of the channel.
\return The channel's identifier value. */

stChannelID CChannelHandler::GetChannelID() const {		// // //
	return m_iChannelID;
}

int CChannelHandler::GetPitch() const
{
	if (m_iPitch != 0 && m_iNote != -1 && !m_iNoteLookupTable.empty()) {
		// Interpolate pitch
		int LowNote  = std::clamp(m_iNote - PITCH_WHEEL_RANGE, 0, NOTE_COUNT - 1);		// // //
		int HighNote = std::clamp(m_iNote + PITCH_WHEEL_RANGE, 0, NOTE_COUNT - 1);
		int Freq	 = m_iNoteLookupTable[m_iNote];
		int Lower	 = m_iNoteLookupTable[LowNote];
		int Higher	 = m_iNoteLookupTable[HighNote];
		int Pitch	 = (m_iPitch < 0) ? (Freq - Lower) : (Higher - Freq);
		return (Pitch * m_iPitch) / 511;
	}

	return 0;
}

void CChannelHandler::Arpeggiate(unsigned int Note)
{
	SetPeriod(TriggerNote(Note));
}

void CChannelHandler::ForceReloadInstrument()		// // //
{
	m_bForceReload = true;
}

void CChannelHandler::ResetChannel()
{
	// Resets the channel states (volume, instrument & duty)
	// Clears channel registers

	// Instrument
	m_iInstrument		= MAX_INSTRUMENTS;
	m_iInstTypeCurrent	= INST_NONE;		// // //
	m_pInstHandler.reset();		// // //

	// Volume
	m_iVolume			= VOL_COLUMN_MAX;
	m_iDefaultVolume	= (VOL_COLUMN_MAX >> VOL_COLUMN_SHIFT) << VOL_COLUMN_SHIFT;		// // //

	m_iDefaultDuty		= 0;
	m_iDutyPeriod		= 0;		// // //
	m_iInstVolume		= 0;

	// Period
	m_iNote				= -1;		// // //
	m_iPeriod			= 0;

	// Effect states
	m_iEffect			= effect_t::NONE;		// // //
	m_iEffectParam		= 0;		// // //

	m_iPortaSpeed		= 0;
	m_iPortaTo			= 0;
	m_iArpState			= 0;
	m_iVibratoSpeed		= 0;
	m_iVibratoPhase		= m_iVibratoMode == vibrato_t::Up ? 48 : 0;
	m_iTremoloSpeed		= 0;
	m_iTremoloPhase		= 0;
	m_iFinePitch		= 0x80;
	m_iPeriod			= 0;
	m_iVolSlide			= 0;
	m_bDelayEnabled		= false;
	m_iNoteCut			= 0;
	m_iNoteRelease		= 0;		// // //
	m_iNoteVolume		= -1;		// // //
	m_iNewVolume		= m_iDefaultVolume;		// // //
	m_iTranspose		= 0;
	m_bTransposeDown	= false;
	m_iTransposeTarget	= 0;
	m_iVibratoDepth		= 0;
	m_iTremoloDepth		= 0;

	m_iEchoBuffer.fill(ECHO_BUFFER_NONE);		// // //

	// States
	m_bTrigger			= false;		// // //
	m_bRelease			= false;
	m_bGate				= false;

	RegisterKeyState(m_iNote);		// // //

	// Clear channel registers
	ClearRegisters();
}

std::string CChannelHandler::GetStateString() const		// // //
{
	std::string log("Inst.: ");
	if (m_iInstrument == MAX_INSTRUMENTS) // never happens because famitracker will switch to selected inst
		log += "None";
	else
		log += conv::sv_from_uint_hex(m_iInstrument, 2);
	log += "        Vol.: ";
	log += conv::to_digit<char>(m_iDefaultVolume >> VOL_COLUMN_SHIFT);
	log += "        Active effects:";
	return log + GetEffectString();
}

void CChannelHandler::ApplyChannelState(const stChannelState &State)
{
	m_iInstrument = State.Instrument;
	m_iDefaultVolume = m_iVolume = (State.Volume == MAX_VOLUME) ? VOL_COLUMN_MAX : (State.Volume << VOL_COLUMN_SHIFT);
	m_iEchoBuffer = State.Echo;
	if (m_iInstrument != MAX_INSTRUMENTS)
		HandleInstrument(true, true);
	if (State.Effect_LengthCounter >= 0)
		HandleEffect({effect_t::VOLUME, static_cast<uint8_t>(State.Effect_LengthCounter)});
	for (unsigned int i = 0; i < EFFECT_COUNT; ++i)
		if (State.Effect[i] >= 0)
			HandleEffect({static_cast<effect_t>(i), static_cast<uint8_t>(State.Effect[i])});
	if (State.Effect[value_cast(effect_t::FDS_MOD_SPEED_HI)] >= 0x10)
		HandleEffect({effect_t::FDS_MOD_SPEED_HI, static_cast<uint8_t>(State.Effect[value_cast(effect_t::FDS_MOD_SPEED_HI)])});
	if (State.Effect_AutoFMMult >= 0)
		HandleEffect({effect_t::FDS_MOD_DEPTH, static_cast<uint8_t>(State.Effect_AutoFMMult)});
}

std::string CChannelHandler::GetEffectString() const		// // //
{
	std::string str = GetSlideEffectString();

	if (m_iVibratoSpeed)
		str += MakeCommandString({effect_t::VIBRATO, static_cast<uint8_t>((m_iVibratoSpeed << 4) | (m_iVibratoDepth >> 4))});
	if (m_iTremoloSpeed)
		str += MakeCommandString({effect_t::TREMOLO, static_cast<uint8_t>((m_iTremoloSpeed << 4) | (m_iTremoloDepth >> 4))});
	if (m_iVolSlide)
		str += MakeCommandString({effect_t::VOLUME_SLIDE, m_iVolSlide});
	if (m_iFinePitch != 0x80)
		str += MakeCommandString({effect_t::PITCH, static_cast<uint8_t>(m_iFinePitch)});
	if ((m_iDefaultDuty && m_iChannelID.Chip != sound_chip_t::S5B) || (m_iDefaultDuty != 0x40 && m_iChannelID.Chip == sound_chip_t::S5B))
		str += MakeCommandString({effect_t::DUTY_CYCLE, m_iDefaultDuty});

	// run-time effects
	if (m_bDelayEnabled)
		str += MakeCommandString({effect_t::DELAY, m_cDelayCounter + 1u});
	if (m_iNoteRelease)
		str += MakeCommandString({effect_t::NOTE_RELEASE, m_iNoteRelease});
	if (m_iNoteVolume > 0)
		str += MakeCommandString({effect_t::DELAYED_VOLUME, static_cast<uint8_t>((m_iNoteVolume << 4) | (m_iNewVolume >> VOL_COLUMN_SHIFT))});
	if (m_iNoteCut)
		str += MakeCommandString({effect_t::NOTE_CUT, m_iNoteCut});
	if (m_iTranspose)
		str += MakeCommandString({effect_t::TRANSPOSE, static_cast<uint8_t>(((m_iTranspose + (m_bTransposeDown ? 8 : 0)) << 4) | m_iTransposeTarget)});

	str += GetCustomEffectString();
	return str.empty() ? std::string(" None") : str;
}

std::string CChannelHandler::GetSlideEffectString() const		// // //
{
	switch (m_iEffect) {
	case effect_t::ARPEGGIO:
		if (m_iEffectParam)
			return MakeCommandString({m_iEffect, m_iEffectParam});
		break;
	case effect_t::PORTA_UP: case effect_t::PORTA_DOWN: case effect_t::PORTAMENTO:
		if (m_iPortaSpeed)
			return MakeCommandString({m_iEffect, static_cast<uint8_t>(m_iPortaSpeed)});
		break;
	}

	return std::string();
}

std::string CChannelHandler::GetCustomEffectString() const		// // //
{
	return std::string();
}

// Handle common things before letting the channels play the notes
void CChannelHandler::PlayNote(stChanNote NoteData)		// // //
{
	// // // Handle delay commands
	if (HandleDelay(NoteData))
		return;

	// Let the channel play
	HandleNoteData(NoteData);
}

void CChannelHandler::WriteEchoBuffer(const stChanNote &NoteData, std::size_t Pos)
{
	if (Pos >= ECHO_BUFFER_LENGTH)
		return;
	int Value;
	switch (NoteData.Note) {
	case note_t::none: Value = ECHO_BUFFER_NONE; break;
	case note_t::halt: Value = ECHO_BUFFER_HALT; break;
	case note_t::echo: Value = ECHO_BUFFER_ECHO + NoteData.Octave; break;
	default:
		Value = MIDI_NOTE(NoteData.Octave, NoteData.Note);
		for (int i = MAX_EFFECT_COLUMNS - 1; i >= 0; --i) {
			const int Param = NoteData.Effects[i].param & 0x0F;
			if (NoteData.Effects[i].fx == effect_t::SLIDE_UP) {
				Value += Param;
				break;
			}
			else if (NoteData.Effects[i].fx == effect_t::SLIDE_DOWN) {
				Value -= Param;
				break;
			}
			else if (NoteData.Effects[i].fx == effect_t::TRANSPOSE) {
				// Sometimes there are not enough ticks for the transpose to take place
				if (NoteData.Effects[i].param & 0x80)
					Value -= Param;
				else
					Value += Param;
				break;
			}
		}
		Value = std::clamp(Value, 0, NOTE_COUNT - 1);
	}

	m_iEchoBuffer[Pos] = Value;
}

void CChannelHandler::HandleNoteData(stChanNote &NoteData)
{
	unsigned LastInstrument = m_iInstrument;
	int Instrument = NoteData.Instrument;
	bool Trigger = (NoteData.Note != note_t::none) && (NoteData.Note != note_t::halt) && (NoteData.Note != note_t::release) &&
		Instrument != HOLD_INSTRUMENT;		// // // 050B
	bool pushNone = false;

	// // // Echo buffer
	if (NoteData.Note == note_t::echo && NoteData.Octave < ECHO_BUFFER_LENGTH) { // retrieve buffer
		int NewNote = m_iEchoBuffer[NoteData.Octave];
		if (NewNote == ECHO_BUFFER_NONE) {
			NoteData.Note = note_t::none;
			pushNone = true;
		}
		else if (NewNote == ECHO_BUFFER_HALT) NoteData.Note = note_t::halt;
		else {
			NoteData.Note = GET_NOTE(NewNote);
			NoteData.Octave = GET_OCTAVE(NewNote);
		}
	}
	if ((NoteData.Note != note_t::release && NoteData.Note != note_t::none) || pushNone) { // push buffer
		for (int i = std::size(m_iEchoBuffer) - 1; i > 0; --i)
			m_iEchoBuffer[i] = m_iEchoBuffer[i - 1];
		WriteEchoBuffer(NoteData, 0);
	}

	// Clear the note cut effect
	if (NoteData.Note != note_t::none) {
		m_iNoteCut = 0;
		m_iNoteRelease = 0;		// // //
		if (Trigger && m_iNoteVolume == 0 && !m_iVolSlide) {		// // //
			m_iVolume = m_iDefaultVolume;
			m_iNoteVolume = -1;
		}
		m_iTranspose = 0;		// // //
	}

	if (Trigger && (m_iEffect == effect_t::SLIDE_UP || m_iEffect == effect_t::SLIDE_DOWN))
		m_iEffect = effect_t::NONE;

	// Effects
	for (const auto &cmd : NoteData.Effects) {
		HandleEffect(cmd);		// // // single method

		// 0CC: remove this eventually like how the asm handles it
		if (cmd.fx == effect_t::VOLUME_SLIDE && !cmd.param && Trigger && m_iNoteVolume == 0) {		// // //
			m_iVolume = m_iDefaultVolume;
			m_iNoteVolume = -1;
		}
	}

	// Volume
	if (NoteData.Vol < MAX_VOLUME) {
		m_iVolume = NoteData.Vol << VOL_COLUMN_SHIFT;
		m_iDefaultVolume = m_iVolume;		// // //
	}

	// Instrument
	if (NoteData.Note == note_t::halt || NoteData.Note == note_t::release)		// // //
		Instrument = MAX_INSTRUMENTS;	// Ignore instrument for release and halt commands

	if (Instrument != MAX_INSTRUMENTS && Instrument != HOLD_INSTRUMENT)		// // // 050B
		m_iInstrument = Instrument;

	bool NewInstrument = (m_iInstrument != LastInstrument && m_iInstrument != HOLD_INSTRUMENT) ||
		(m_iInstrument == MAX_INSTRUMENTS) || m_bForceReload;		// // // 050B

	if (m_iInstrument == MAX_INSTRUMENTS) {		// // // do nothing
		// m_iInstrument = m_pSoundGen->GetDefaultInstrument();
	}

	switch (NoteData.Note) {		// // // set note value before loading instrument
	case note_t::none: case note_t::halt: case note_t::release: break;
	default: m_iNote = RunNote(NoteData.Octave, NoteData.Note);
	}

	// Note
	switch (NoteData.Note) {
	case note_t::none:
		HandleEmptyNote();
		break;
	case note_t::halt:
		m_bRelease = false;
		HandleCut();
		break;
	case note_t::release:
		HandleRelease();
		break;
	default:
		HandleNote(NoteData.Note, NoteData.Octave);
		break;
	}

	if (Trigger && (m_iEffect == effect_t::SLIDE_DOWN || m_iEffect == effect_t::SLIDE_UP))		// // //
		SetupSlide();

	if ((NewInstrument || Trigger) && m_iInstrument != MAX_INSTRUMENTS) {
		if (!HandleInstrument(Trigger, NewInstrument)) {		// // //
			// m_bForceReload = false;		// // //
			// return;
		}
	}
	m_bForceReload = false;
}

bool CChannelHandler::HandleInstrument(bool Trigger, bool NewInstrument)		// // //
{
	auto pManager = m_pSoundGen->GetInstrumentManager();
	if (!pManager)
		return false;
	std::shared_ptr<CInstrument> pInstrument = pManager->GetInstrument(m_iInstrument);
	if (!pInstrument)
		return false;

	// load instrument here
	inst_type_t instType = pInstrument->GetType();
	if (NewInstrument)
		CreateInstHandler(instType);
	m_iInstTypeCurrent = instType;

	if (!m_pInstHandler)
		return false;
	if (NewInstrument)
		m_pInstHandler->LoadInstrument(pInstrument);
	if (Trigger || m_bForceReload)
		m_pInstHandler->TriggerInstrument();

	return true;
}

bool CChannelHandler::CreateInstHandler(inst_type_t Type)
{
	return false;
}

void CChannelHandler::SetNoteTable(array_view<unsigned> NoteLookupTable)		// // //
{
	// Installs the note lookup table
	if (NoteLookupTable.size() == NOTE_COUNT)
		m_iNoteLookupTable = NoteLookupTable;
}

int CChannelHandler::TriggerNote(int Note)
{
	Note = std::clamp(Note, 0, NOTE_COUNT - 1);		// // //

	// Trigger a note, return note period
	RegisterKeyState(Note);

	if (m_bLinearPitch)		// // //
		return Note << LINEAR_PITCH_AMOUNT;

	if (m_iNoteLookupTable.empty())
		return Note;

	return m_iNoteLookupTable[Note];
}

void CChannelHandler::FinishTick()		// // //
{
	m_bTrigger = false;
}

void CChannelHandler::CutNote()
{
	// Cut currently playing note

	RegisterKeyState(-1);

	m_bGate = false;
	m_iPeriod = 0;
	m_iPortaTo = 0;
}

void CChannelHandler::ReleaseNote()
{
	// Release currently playing note

	RegisterKeyState(-1);

	if (m_pInstHandler) m_pInstHandler->ReleaseInstrument();		// // //
	m_bRelease = true;
}

int CChannelHandler::RunNote(int Octave, note_t Note)
{
	// Run the note and handle portamento
	int NewNote = MIDI_NOTE(Octave, Note);

	int NesFreq = TriggerNote(NewNote);

	if (m_iPortaSpeed > 0 && m_iEffect == effect_t::PORTAMENTO && m_bGate) {		// // //
		if (m_iPeriod == 0)
			m_iPeriod = NesFreq;
		m_iPortaTo = NesFreq;
	}
	else
		m_iPeriod = NesFreq;

	m_bGate = true;

	return NewNote;
}

void CChannelHandler::HandleNote(note_t Note, int Octave)		// // //
{
	m_iDutyPeriod = m_iDefaultDuty;
	m_bTrigger = true;
	m_bRelease = false;
}

void CChannelHandler::SetupSlide()		// // //
{
	const auto GetSlideSpeed = [] (unsigned char param) {
		return ((param & 0xF0) >> 3) + 1;
	};

	switch (m_iEffect) {
	case effect_t::PORTAMENTO:
		m_iPortaSpeed = m_iEffectParam;
		if (m_bGate)		// // //
			m_iPortaTo = TriggerNote(m_iNote);
		break;
	case effect_t::SLIDE_UP:
		m_iNote += m_iEffectParam & 0xF;
		m_iPortaSpeed = GetSlideSpeed(m_iEffectParam);
		m_iPortaTo = TriggerNote(m_iNote);
		break;
	case effect_t::SLIDE_DOWN:
		m_iNote -= m_iEffectParam & 0xF;
		m_iPortaSpeed = GetSlideSpeed(m_iEffectParam);
		m_iPortaTo = TriggerNote(m_iNote);
		break;
	}
}

bool CChannelHandler::HandleEffect(stEffectCommand cmd)
{
	// Handle common effects for all channels

	switch (cmd.fx) {
	case effect_t::PORTAMENTO:
		m_iEffectParam = cmd.param;		// // //
		m_iEffect = effect_t::PORTAMENTO;
		SetupSlide();
		if (!cmd.param)
			m_iPortaTo = 0;
		break;
	case effect_t::VIBRATO:
		m_iVibratoDepth = (cmd.param & 0x0F) << 4;
		m_iVibratoSpeed = cmd.param >> 4;
		if (!cmd.param)
			m_iVibratoPhase = m_iVibratoMode == vibrato_t::Up ? 48 : 0;
		break;
	case effect_t::TREMOLO:
		m_iTremoloDepth = (cmd.param & 0x0F) << 4;
		m_iTremoloSpeed = cmd.param >> 4;
		if (!cmd.param)
			m_iTremoloPhase = 0;
		break;
	case effect_t::ARPEGGIO:
		m_iEffectParam = cmd.param;		// // //
		m_iEffect = effect_t::ARPEGGIO;
		break;
	case effect_t::PITCH:
		m_iFinePitch = cmd.param;
		break;
	case effect_t::PORTA_DOWN:
		m_iPortaSpeed = cmd.param;
		m_iEffectParam = cmd.param;		// // //
		m_iEffect = effect_t::PORTA_DOWN;
		break;
	case effect_t::PORTA_UP:
		m_iPortaSpeed = cmd.param;
		m_iEffectParam = cmd.param;		// // //
		m_iEffect = effect_t::PORTA_UP;
		break;
	case effect_t::SLIDE_UP:		// // //
		m_iEffectParam = cmd.param;
		m_iEffect = effect_t::SLIDE_UP;
		SetupSlide();
		break;
	case effect_t::SLIDE_DOWN:		// // //
		m_iEffectParam = cmd.param;
		m_iEffect = effect_t::SLIDE_DOWN;
		SetupSlide();
		break;
	case effect_t::VOLUME_SLIDE:
		m_iVolSlide = cmd.param;
		if (!cmd.param)		// // //
			m_iDefaultVolume = m_iVolume;
		break;
	case effect_t::NOTE_CUT:
		if (cmd.param >= 0x80) return false;		// // //
		m_iNoteCut = cmd.param + 1;
		break;
	case effect_t::NOTE_RELEASE:		// // //
		if (cmd.param >= 0x80) return false;
		m_iNoteRelease = cmd.param + 1;
		break;
	case effect_t::DELAYED_VOLUME:		// // //
		if (!(cmd.param >> 4) || !(cmd.param & 0xF)) break;
		m_iNoteVolume = (cmd.param >> 4) + 1;
		m_iNewVolume = (cmd.param & 0x0F) << VOL_COLUMN_SHIFT;
		break;
	case effect_t::TRANSPOSE:		// // //
		m_iTranspose = ((cmd.param & 0x70) >> 4) + 1;
		m_iTransposeTarget = cmd.param & 0x0F;
		m_bTransposeDown = (cmd.param & 0x80) != 0;
		break;
//	case effect_t::TARGET_VOLUME_SLIDE:
		// TODO implement
//		break;
	default:
		return false;
	}

	return true;
}

bool CChannelHandler::HandleDelay(stChanNote &NoteData)
{
	// Handle note delay, Gxx

	if (m_bDelayEnabled) {
		m_bDelayEnabled = false;
		HandleNoteData(m_cnDelayed);		// // //
	}

	// Check delay
	for (const auto &cmd : NoteData.Effects) {
		if (cmd.fx == effect_t::DELAY && cmd.param > 0) {
			m_bDelayEnabled = true;
			m_cDelayCounter = cmd.param;

			// Only one delay/row is allowed
			for (auto &cmd2 : NoteData.Effects)
				if (cmd2.fx == effect_t::DELAY)		// // //
					cmd2 = { };

			m_cnDelayed = NoteData;		// // //
			return true;
		}
	}

	return false;
}

void CChannelHandler::UpdateNoteCut()
{
	// Note cut (Sxx)
	if (m_iNoteCut > 0) if (!--m_iNoteCut)
		HandleCut();
}

void CChannelHandler::UpdateNoteRelease()		// // //
{
	// Note release (Lxx)
	if (m_iNoteRelease > 0) if (!--m_iNoteRelease) {
		HandleRelease();
		ReleaseNote();
	}
}

void CChannelHandler::UpdateNoteVolume()		// // //
{
	// Delayed channel volume (Mxy)
	if (m_iNoteVolume > 0) if (!--m_iNoteVolume)
		m_iVolume = m_iNewVolume;
}

void CChannelHandler::UpdateTranspose()		// // //
{
	// Delayed transpose (Txy)
	if (m_iTranspose > 0) if (!--m_iTranspose && m_iNote != -1) {
		// trigger note
		SetNote(m_iNote + m_iTransposeTarget * (m_bTransposeDown ? -1 : 1));
		SetPeriod(TriggerNote(m_iNote));
	}
}

void CChannelHandler::UpdateDelay()
{
	// Delay (Gxx)
	if (m_bDelayEnabled) {
		if (!m_cDelayCounter) {
			m_bDelayEnabled = false;
			PlayNote(m_cnDelayed);		// // //
		}
		else
			--m_cDelayCounter;
	}
}

void CChannelHandler::UpdateVolumeSlide()
{
	// Volume slide (Axy)
	m_iVolume -= (m_iVolSlide & 0x0F);
	if (m_iVolume < 0)
		m_iVolume = 0;

	m_iVolume += (m_iVolSlide & 0xF0) >> 4;
	if (m_iVolume > VOL_COLUMN_MAX)
		m_iVolume = VOL_COLUMN_MAX;
}

void CChannelHandler::UpdateTargetVolumeSlide()
{
	// TODO implement
}

void CChannelHandler::UpdateVibratoTremolo()
{
	// Vibrato and tremolo
	m_iVibratoPhase = (m_iVibratoPhase + m_iVibratoSpeed) & 63;
	m_iTremoloPhase = (m_iTremoloPhase + m_iTremoloSpeed) & 63;
}

void CChannelHandler::PeriodAdd(int Step)
{
	SetPeriod(GetPeriod() + Step);		// // // uniform
}

void CChannelHandler::PeriodRemove(int Step)
{
	SetPeriod(GetPeriod() - Step);		// // //
}

void CChannelHandler::UpdateEffects()
{
	// Handle other effects
	switch (m_iEffect) {
		case effect_t::ARPEGGIO:
			if (m_iEffectParam != 0) {
				if (m_iNote != -1) {		// // //
					int offset = [&] {
						switch (m_iArpState) {
						case 1: return m_iEffectParam >> 4;
						case 2: return m_iEffectParam & 0x0F;
						default: return 0;
						}
					}();
					SetPeriod(TriggerNote(m_iNote + offset));
				}
				if (m_iArpState == 1 && !(m_iEffectParam & 0x0F))
					++m_iArpState;
				m_iArpState = (m_iArpState + 1) % 3;
			}
			break;
		case effect_t::PORTAMENTO:
		case effect_t::SLIDE_UP:		// // //
		case effect_t::SLIDE_DOWN:		// // //
			// Automatic portamento
			if (m_iPortaSpeed > 0 && m_iPortaTo) {		// // //
				if (m_iPeriod > m_iPortaTo) {
					PeriodRemove(m_iPortaSpeed);
					if (m_iPeriod <= m_iPortaTo) {
						SetPeriod(m_iPortaTo);
						if (m_iEffect != effect_t::PORTAMENTO) {
							m_iPortaTo = 0;
							m_iPortaSpeed = 0;
							m_iEffect = effect_t::NONE;
						}
					}
				}
				else if (m_iPeriod < m_iPortaTo) {
					PeriodAdd(m_iPortaSpeed);
					if (m_iPeriod >= m_iPortaTo) {
						SetPeriod(m_iPortaTo);
						if (m_iEffect != effect_t::PORTAMENTO) {
							m_iPortaTo = 0;
							m_iPortaSpeed = 0;
							m_iEffect = effect_t::NONE;
						}
					}
				}
			}
			break;
			/*
		case effect_t::SLIDE_UP:
			if (m_iPortaSpeed > 0) {
				if (m_iPeriod > m_iPortaTo) {
					PeriodRemove(m_iPortaSpeed);
					if (m_iPeriod < m_iPortaTo) {
						SetPeriod(m_iPortaTo);
						m_iPortaTo = 0;
						m_iEffect = effect_t::NONE;
					}
				}
			}
			break;
		case effect_t::SLIDE_DOWN:
			if (m_iPortaSpeed > 0) {
				PeriodAdd(m_iPortaSpeed);
				if (m_iPeriod > m_iPortaTo) {
					SetPeriod(m_iPortaTo);
					m_iPortaTo = 0;
					m_iEffect = effect_t::NONE;
				}
			}
			break;
			*/
		case effect_t::PORTA_DOWN:
			m_bLinearPitch ? PeriodRemove(m_iPortaSpeed) : PeriodAdd(m_iPortaSpeed);		// // //
			break;
		case effect_t::PORTA_UP:
			m_bLinearPitch ? PeriodAdd(m_iPortaSpeed) : PeriodRemove(m_iPortaSpeed);		// // //
			break;
	}
}

void CChannelHandler::ProcessChannel()
{
	// Run all default and common channel processing
	// This gets called each frame
	//

	UpdateDelay();
	UpdateNoteCut();
	UpdateNoteRelease();		// // //
	UpdateNoteVolume();			// // //
	UpdateTranspose();			// // //
	UpdateVolumeSlide();
	UpdateVibratoTremolo();
	UpdateEffects();
	if (m_pInstHandler) m_pInstHandler->UpdateInstrument();		// // //
	// instruments are updated after running effects and before writing to sound registers
}

int CChannelHandler::GetVibrato() const
{
	// Vibrato offset (4xx)
	int VibFreq = 0;

	if ((m_iVibratoPhase & 0xF0) == 0x00)
		VibFreq = m_iVibratoTable[m_iVibratoDepth + m_iVibratoPhase];
	else if ((m_iVibratoPhase & 0xF0) == 0x10)
		VibFreq = m_iVibratoTable[m_iVibratoDepth + 15 - (m_iVibratoPhase - 16)];
	else if ((m_iVibratoPhase & 0xF0) == 0x20)
		VibFreq = -m_iVibratoTable[m_iVibratoDepth + (m_iVibratoPhase - 32)];
	else if ((m_iVibratoPhase & 0xF0) == 0x30)
		VibFreq = -m_iVibratoTable[m_iVibratoDepth + 15 - (m_iVibratoPhase - 48)];

	if (m_iVibratoMode == vibrato_t::Up) {
		VibFreq += m_iVibratoTable[m_iVibratoDepth + 15] + 1;
		VibFreq >>= 1;
	}

//	if (m_bLinearPitch)
//		VibFreq <<= 1;		// // //

	return VibFreq;
}

int CChannelHandler::GetTremolo() const
{
	// Tremolo offset (7xx)
	int TremVol = 0;
	int Phase = m_iTremoloPhase >> 1;

	if ((Phase & 0xF0) == 0x00)
		TremVol = m_iVibratoTable[m_iTremoloDepth + Phase];
	else if ((Phase & 0xF0) == 0x10)
		TremVol = m_iVibratoTable[m_iTremoloDepth + 15 - (Phase - 16)];

	return (TremVol >> 1);
}

int CChannelHandler::GetFinePitch() const
{
	// Fine pitch setting (Pxx)
	return (0x80 - m_iFinePitch);
}

int CChannelHandler::CalculatePeriod() const
{
	int Detune = GetVibrato() - GetFinePitch() - GetPitch();
	int Period = LimitPeriod(GetPeriod() - Detune);		// // //
	if (m_bLinearPitch && !m_iNoteLookupTable.empty()) {
		Period = LimitPeriod(GetPeriod() + Detune);
		int Note = Period >> LINEAR_PITCH_AMOUNT;
		int Sub = Period % (1 << LINEAR_PITCH_AMOUNT);
		int Offset = Note < NOTE_COUNT - 1 ? m_iNoteLookupTable[Note] - m_iNoteLookupTable[Note + 1] : 0;
		Offset = Offset * Sub >> LINEAR_PITCH_AMOUNT;
		if (Sub && !Offset) Offset = 1;
		Period = m_iNoteLookupTable[Note] - Offset;
	}
	return LimitRawPeriod(Period);
}

int CChannelHandler::CalculateVolume() const
{
	// Volume calculation
	return LimitVolume((m_iInstVolume * (m_iVolume >> VOL_COLUMN_SHIFT)) / 15 - GetTremolo());		// // //
}

int CChannelHandler::LimitPeriod(int Period) const		// // // virtual
{
	if (!m_bLinearPitch) return LimitRawPeriod(Period);
	return std::clamp(Period, 0, (NOTE_COUNT - 1) << LINEAR_PITCH_AMOUNT);
}

int CChannelHandler::LimitRawPeriod(int Period) const
{
	return std::clamp(Period, 0, m_iMaxPeriod);
}

int CChannelHandler::LimitVolume(int Volume) const		// // //
{
	if (!m_bGate)
		return 0;

	Volume = std::clamp(Volume, 0, m_iMaxVolume);
	if (Volume == 0 && !Env.GetSettings()->General.bCutVolume && m_iInstVolume > 0 && m_iVolume > 0)		// // //
		return 1;
	return Volume;
}

void CChannelHandler::RegisterKeyState(int Note)
{
	m_iActiveNote = Note;		// // //
}

void CChannelHandler::SetPeriod(int Period)
{
	m_iPeriod = LimitPeriod(Period);
}

int CChannelHandler::GetPeriod() const
{
	return m_iPeriod;
}

void CChannelHandler::SetNote(int Note)
{
	m_iNote = Note;
}

int CChannelHandler::GetNote() const
{
	return m_iNote;
}

int CChannelHandler::GetActiveNote() const {		// // //
	return m_iActiveNote;
}

void CChannelHandler::SetVolume(int Volume)
{
	m_iInstVolume = Volume;
}

int CChannelHandler::GetVolume() const
{
	return m_iInstVolume;
}

int CChannelHandler::GetChannelVolume() const		// // //
{
	return m_iVolume;
}

void CChannelHandler::SetDutyPeriod(int Duty)
{
	m_iDutyPeriod = ConvertDuty(Duty);		// // //
}

int CChannelHandler::GetDutyPeriod() const
{
	return m_iDutyPeriod;
}

unsigned char CChannelHandler::GetArpParam() const
{
	return m_iEffect == effect_t::ARPEGGIO ? m_iEffectParam : 0U;
}

bool CChannelHandler::IsActive() const
{
	return m_bGate;
}

bool CChannelHandler::IsReleasing() const
{
	return m_bRelease;
}

/*
 * Class CChannelHandlerInverted
 *
 */

bool CChannelHandlerInverted::HandleEffect(stEffectCommand cmd)
{
	if (!m_bLinearPitch) switch (cmd.fx) {		// // //
	case effect_t::PORTA_UP:   cmd.fx = effect_t::PORTA_DOWN; break;
	case effect_t::PORTA_DOWN: cmd.fx = effect_t::PORTA_UP; break;
	}
	return CChannelHandler::HandleEffect(cmd);
}

int CChannelHandlerInverted::CalculatePeriod() const
{
	int Period = LimitPeriod(GetPeriod() + GetVibrato() - GetFinePitch() - GetPitch());		// // //
	if (m_bLinearPitch && !m_iNoteLookupTable.empty()) {
		int Note = Period >> LINEAR_PITCH_AMOUNT;
		int Sub = Period % (1 << LINEAR_PITCH_AMOUNT);
		int Offset = Note < NOTE_COUNT - 1 ? m_iNoteLookupTable[Note + 1] - m_iNoteLookupTable[Note] : 0;
		Offset = Offset * Sub >> LINEAR_PITCH_AMOUNT;
		if (Sub && !Offset) Offset = 1;
		Period = m_iNoteLookupTable[Note] + Offset;
	}
	return LimitRawPeriod(Period);
}

std::string CChannelHandlerInverted::GetSlideEffectString() const		// // //
{
	switch (m_iEffect) {
	case effect_t::PORTA_UP:
		return MakeCommandString({effect_t::PORTA_DOWN, static_cast<uint8_t>(m_iPortaSpeed)});
	case effect_t::PORTA_DOWN:
		return MakeCommandString({effect_t::PORTA_UP, static_cast<uint8_t>(m_iPortaSpeed)});
	}
	return CChannelHandler::GetSlideEffectString();
}
