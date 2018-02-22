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

#include "InstHandler.h"
#include "Sequence.h"
#include <unordered_map>

class CSeqInstrument;

/*!
	\brief Class for sequence instrument handlers.
	\details Sequence instruments are the major type of instruments provided by FamiTracker;
	they are used by all sound channels except for the Konami VRC7.
*/
class CSeqInstHandler : public CInstHandler
{
public:
	/*!	\brief Constants representing the state of each sequence. */
	enum seq_state_t {		// // //
		SEQ_STATE_DISABLED, /*!< Current sequence is not enabled. */
		SEQ_STATE_RUNNING,	/*!< Current sequence is running. */
		SEQ_STATE_END,		/*!< Current sequence has just finished running the last tick. */
		SEQ_STATE_HALT,		/*!< Current sequence has finished running until the next note. */
	};

	/*!	\brief Constructor of the sequence instrument handler.
		\details A default duty value must be provided in the parameters.
		\param pInterface Pointer to the channel interface.
		\param Vol Default volume for instruments used by this handler.
		\param Duty Default duty cycle for instruments used by this handler. */
	CSeqInstHandler(CChannelHandlerInterface *pInterface, int Vol, int Duty);

	void LoadInstrument(std::shared_ptr<CInstrument> pInst) override;
	void TriggerInstrument() override;
	void ReleaseInstrument() override;
	void UpdateInstrument() override;

	/*!	\brief Obtains the current sequence state of a given sequence type.
		\param Index The sequence type.
		\return The sequence state of the given sequence type. */
	seq_state_t GetSequenceState(sequence_t Index) const;

protected:
	/*!	\brief Processes the value retrieved from a sequence.
		\return True if the sequence has finished processing.
		\param Seq Reference to the sequence.
		\param Pos Sequence position. */
	virtual bool ProcessSequence(const CSequence &Seq, int Pos);

	/*!	\brief Prepares a sequence type for use by CSeqInstHandler::UpdateInstrument.
		\param Index The sequence type.
		\param pSequence Pointer to the sequence. */
	void SetupSequence(sequence_t Index, std::shared_ptr<const CSequence> pSequence);

	/*!	\brief Clears a sequence type from use.
		\param Index The sequence type. */
	void ClearSequence(sequence_t Index);

protected:
	/*! \brief A struct containing runtime state of a given sequence type. */
	struct seq_info_t {
		void Trigger();
		void Release();
		void Step(bool isReleasing);

		/*!	\brief Pointer to the sequence used by the current instrument. */
		std::shared_ptr<const CSequence> m_pSequence;
		/*!	\brief State of the current sequence type. */
		seq_state_t m_iSeqState = SEQ_STATE_DISABLED;
		/*!	\brief Tick index of the current sequence type. */
		int m_iSeqPointer = 0;
	};
	/*! \brief Sequence states for the default sequence types. */
	std::unordered_map<sequence_t, seq_info_t> m_SequenceInfo;

	/*!	\brief The current duty cycle of the instrument.
		\details The exact interpretation of this member may not be identical across sound channels.
		\warning Currently unused. */
	int				m_iDutyParam;
	/*!	\brief The default duty cycle of the instrument.
		\details On triggering a new note, the duty cycle value is reset to this value.
		\warning Currently unused. */
	const int		m_iDefaultDuty;
};
