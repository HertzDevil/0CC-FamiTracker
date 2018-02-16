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

#include "InstrumentRecorder.h"
#include "InstrumentManager.h"
#include "FamiTrackerModule.h"
#include "FamiTrackerViewMessage.h"
#include "SoundGen.h"
#include "SeqInstrument.h"
#include "InstrumentFDS.h"
#include "InstrumentN163.h"
#include "InstrumentService.h"
#include "DetuneTable.h"
#include "FamiTrackerEnv.h"
#include "Sequence.h"
#include "ChannelName.h"

CInstrumentRecorder::CInstrumentRecorder(CSoundGen *pSG) :
	m_pSoundGen(pSG),
	m_iRecordChannel(chan_id_t::NONE),
	m_iDumpCount(0),
	m_iRecordWaveCache(nullptr)
{
	m_stRecordSetting.Interval = MAX_SEQUENCE_ITEMS;
	m_stRecordSetting.InstCount = 1;
	m_stRecordSetting.Reset = true;
	foreachSeq([&] (sequence_t i) {
		m_pSequenceCache[i] = std::make_shared<CSequence>(i);
	});
	ResetRecordCache();
}

CInstrumentRecorder::~CInstrumentRecorder()
{
}

void CInstrumentRecorder::AssignModule(CFamiTrackerModule &modfile) {
	m_pModule = &modfile;
}

void CInstrumentRecorder::StartRecording()
{
	m_iDumpCount = m_stRecordSetting.InstCount;
	ResetRecordCache();
	InitRecordInstrument();
}

void CInstrumentRecorder::StopRecording(CWnd *pView)
{
	if (*m_pDumpInstrument != nullptr && pView != nullptr)
		pView->PostMessageW(WM_USER_DUMP_INST);
	--m_iDumpCount;
}

void CInstrumentRecorder::RecordInstrument(const unsigned Tick, CWnd *pView)		// // //
{
	unsigned int Intv = static_cast<unsigned>(m_stRecordSetting.Interval);
	if (m_iRecordChannel == chan_id_t::NONE || Tick > Intv * m_stRecordSetting.InstCount + 1) return;
	if (Tick % Intv == 1 && Tick > Intv) {
		if (*m_pDumpInstrument != nullptr && pView != nullptr) {
			pView->PostMessageW(WM_USER_DUMP_INST);
			m_pDumpInstrument++;
		}
		--m_iDumpCount;
	}
	bool Temp = *m_pDumpInstrument == nullptr;
	int Pos = (Tick - 1) % Intv;

	signed char Val = 0;

	int PitchReg = 0;
	int Detune = 0x7FFFFFFF;
	unsigned ID = GetChannelSubIndex(m_iRecordChannel);

	sound_chip_t Chip = GetChipFromChannel(m_iRecordChannel);
	const auto REG = [&] (int x) { return m_pSoundGen->GetReg(Chip, x); };

	switch (Chip) {
	case sound_chip_t::APU:
		PitchReg = m_iRecordChannel == chan_id_t::NOISE ? (0x0F & REG(0x400E)) :
					(REG(0x4002 | (ID << 2)) | (0x07 & REG(0x4003 | (ID << 2))) << 8); break;
	case sound_chip_t::VRC6:
		PitchReg = REG(0x9001 + (ID << 12)) | ((0x0F & REG(0x9002 + (ID << 12))) << 8); break;
	case sound_chip_t::FDS:
		PitchReg = REG(0x4082) | (0x0F & REG(0x4083)) << 8; break;
	case sound_chip_t::MMC5:
		PitchReg = (REG(0x5002 | (ID << 2)) | (0x07 & REG(0x5003 | (ID << 2))) << 8); break;
	case sound_chip_t::N163:
		PitchReg = (REG(0x78 - (ID << 3))
					| (REG(0x7A - (ID << 3))) << 8
					| (0x03 & REG(0x7C - (ID << 3))) << 16) >> 2; // N163_PITCH_SLIDE_SHIFT;
		break;
	case sound_chip_t::S5B:
		PitchReg = (REG(ID << 1) | (0x0F & REG(1 + (ID << 1))) << 8); break;
	}

	CDetuneTable::type_t Table;
	switch (Chip) {
	case sound_chip_t::APU:  Table = m_pModule->GetMachine() == PAL ? CDetuneTable::DETUNE_PAL : CDetuneTable::DETUNE_NTSC; break;
	case sound_chip_t::VRC6: Table = m_iRecordChannel == chan_id_t::VRC6_SAWTOOTH ? CDetuneTable::DETUNE_SAW : CDetuneTable::DETUNE_NTSC; break;
	case sound_chip_t::VRC7: Table = CDetuneTable::DETUNE_VRC7; break;
	case sound_chip_t::FDS:  Table = CDetuneTable::DETUNE_FDS; break;
	case sound_chip_t::MMC5: Table = CDetuneTable::DETUNE_NTSC; break;
	case sound_chip_t::N163: Table = CDetuneTable::DETUNE_N163; break;
	case sound_chip_t::S5B:  Table = CDetuneTable::DETUNE_S5B; break;
	}
	int Note = 0;
	if (m_iRecordChannel == chan_id_t::NOISE) {
		Note = PitchReg ^ 0xF; Detune = 0;
	}
	else for (int i = 0; i < NOTE_COUNT; i++) {
		int diff = PitchReg - m_pSoundGen->ReadPeriodTable(i, Table);
		if (std::abs(diff) < std::abs(Detune)) {
			Note = i; Detune = diff;
		}
	}

	inst_type_t InstType = INST_NONE; // optimize this
	switch (Chip) {
	case sound_chip_t::APU: case sound_chip_t::MMC5: InstType = INST_2A03; break;
	case sound_chip_t::VRC6: InstType = INST_VRC6; break;
	// case sound_chip_t::VRC7: Type = INST_VRC7; break;
	case sound_chip_t::FDS:  InstType = INST_FDS; break;
	case sound_chip_t::N163: InstType = INST_N163; break;
	case sound_chip_t::S5B:  InstType = INST_S5B; break;
	}

	switch (InstType) {
	case INST_2A03: case INST_VRC6: case INST_N163: case INST_S5B:
		foreachSeq([&] (sequence_t s) {
			switch (s) {
			case sequence_t::Volume:
				switch (Chip) {
				case sound_chip_t::APU:
					Val = m_iRecordChannel == chan_id_t::TRIANGLE ? ((0x7F & REG(0x4008)) ? 15 : 0) : (0x0F & REG(0x4000 | (ID << 2))); break;
				case sound_chip_t::VRC6:
					Val = m_iRecordChannel == chan_id_t::VRC6_SAWTOOTH ? (0x0F & (REG(0xB000) >> 1)) : (0x0F & REG(0x9000 + (ID << 12))); break;
				case sound_chip_t::MMC5:
					Val = 0x0F & REG(0x5000 | (ID << 2)); break;
				case sound_chip_t::N163:
					Val = 0x0F & REG(0x7F - (ID << 3)); break;
				case sound_chip_t::S5B:
					Val = 0x0F & REG(0x08 + ID); break;
				}
				break;
			case sequence_t::Arpeggio: Val = static_cast<char>(Note); break;
			case sequence_t::Pitch: Val = static_cast<char>(Detune % 16); break;
			case sequence_t::HiPitch: Val = static_cast<char>(Detune / 16); break;
			case sequence_t::DutyCycle:
				switch (Chip) {
				case sound_chip_t::APU:
					Val = m_iRecordChannel == chan_id_t::TRIANGLE ? 0 :
						m_iRecordChannel == chan_id_t::NOISE ? (0x01 & REG(0x400E) >> 7) : (0x03 & REG(0x4000 | (ID << 2)) >> 6); break;
				case sound_chip_t::VRC6:
					Val = m_iRecordChannel == chan_id_t::VRC6_SAWTOOTH ? (0x01 & (REG(0xB000) >> 5)) : (0x07 & REG(0x9000 + (ID << 12)) >> 4); break;
				case sound_chip_t::MMC5:
					Val = 0x03 & REG(0x5000 | (ID << 2)) >> 6; break;
				case sound_chip_t::N163:
					Val = 0;
					if (m_iRecordWaveCache == NULL) {
						int Size = 0x100 - (0xFC & REG(0x7C - (ID << 3)));
						if (Size <= CInstrumentN163::MAX_WAVE_SIZE) {
							m_iRecordWaveSize = Size;
							m_iRecordWaveCache = std::make_unique<char[]>(m_iRecordWaveSize * CInstrumentN163::MAX_WAVE_COUNT);
							m_iRecordWaveCount = 0;
						}
					}
					if (m_iRecordWaveCache != nullptr) {
						int Count = 0x100 - (0xFC & REG(0x7C - (ID << 3)));
						if (Count == m_iRecordWaveSize) {
							int pos = REG(0x7E - (ID << 3));
							auto Wave = std::make_unique<char[]>(Count);
							for (int j = 0; j < Count; j++)
								Wave[j] = 0x0F & REG((pos + j) >> 1) >> ((j & 0x01) ? 4 : 0);
							for (int j = 1; j <= m_iRecordWaveCount; j++) {
								if (!memcmp(Wave.get(), m_iRecordWaveCache.get() + j * Count, Count)) {
									Val = j; goto outer;
								}
							}
							if (m_iRecordWaveCount < CInstrumentN163::MAX_WAVE_COUNT - 1) {
								Val = ++m_iRecordWaveCount;
								memcpy(m_iRecordWaveCache.get() + Val * Count, Wave.get(), Count);
							}
							else
								Val = m_pSequenceCache[s]->GetItem(Pos - 1);
						outer:
							;
						}
					}
					break;
				case sound_chip_t::S5B:
					Val = 0x1F & REG(0x06) | (0x10 & REG(0x08 + ID)) << 1
						| (0x01 << ID & ~REG(0x07)) << (6 - ID) | (0x08 << ID & ~REG(0x07)) << (4 - ID); break;
				}
				break;
			}
			m_pSequenceCache[s]->SetItemCount(Pos + 1);
			m_pSequenceCache[s]->SetItem(Pos, Val);
		});
		break;
	case INST_FDS:
		foreachSeq([&] (sequence_t k) {
			switch (k) {
			case sequence_t::Volume: Val = 0x3F & REG(0x4080); if (Val > 0x20) Val = 0x20; break;
			case sequence_t::Arpeggio: Val = static_cast<char>(Note); break;
			case sequence_t::Pitch: Val = static_cast<char>(Detune); break;
			default: return;
			}
			m_pSequenceCache[k]->SetItemCount(Pos + 1);
			m_pSequenceCache[k]->SetItem(Pos, Val);
		});
	}

	if (!(Tick % Intv))
		FinalizeRecordInstrument();
}

std::unique_ptr<CInstrument> CInstrumentRecorder::GetRecordInstrument(unsigned Tick)
{
	FinalizeRecordInstrument();
	return std::move(m_pDumpCache[Tick / m_stRecordSetting.Interval - (m_pSoundGen->IsPlaying() ? 1 : 0)]);
}

chan_id_t CInstrumentRecorder::GetRecordChannel() const
{
	return m_iRecordChannel;
}

void CInstrumentRecorder::SetRecordChannel(chan_id_t Channel)
{
	m_iRecordChannel = Channel;
}

const stRecordSetting &CInstrumentRecorder::GetRecordSetting() const
{
	return m_stRecordSetting;
}

void CInstrumentRecorder::SetRecordSetting(const stRecordSetting &Setting)
{
	m_stRecordSetting = Setting;
}

void CInstrumentRecorder::ResetDumpInstrument()
{
	if (m_iDumpCount < 0) return;
	if (m_pSoundGen->IsPlaying()) {
		--m_pDumpInstrument;
		if (m_pDumpInstrument >= m_pDumpCache.data())
			ReleaseCurrent();
		++m_pDumpInstrument;
	}
	if (m_iDumpCount && *m_pDumpInstrument == nullptr) {
		InitRecordInstrument();
	}
	else {
//		if (*m_pDumpInstrument != nullptr)
//			FinalizeRecordInstrument();
		if (!m_iDumpCount || !m_pSoundGen->IsPlaying()) {
			m_iRecordChannel = chan_id_t::NONE;
			if (m_stRecordSetting.Reset) {
				m_stRecordSetting.Interval = MAX_SEQUENCE_ITEMS;
				m_stRecordSetting.InstCount = 1;
			}
			ReleaseCurrent();
		}
	}
}

void CInstrumentRecorder::ResetRecordCache()
{
	for (auto &x : m_pDumpCache)
		x.reset();
	m_pDumpInstrument = &m_pDumpCache[0];
	foreachSeq([&] (sequence_t i) {
		m_pSequenceCache[i]->Clear();
	});
}

void CInstrumentRecorder::ReleaseCurrent()
{
	if (*m_pDumpInstrument) {
		FinalizeRecordInstrument();
		m_pDumpInstrument->reset();
	}
}

void CInstrumentRecorder::InitRecordInstrument()
{
	if (m_pModule->GetInstrumentManager()->GetInstrumentCount() >= MAX_INSTRUMENTS) {
		m_iDumpCount = 0; m_iRecordChannel = chan_id_t::NONE; return;
	}
	inst_type_t Type = [&] { // optimize this
		switch (GetChipFromChannel(m_iRecordChannel)) {
		case sound_chip_t::APU: case sound_chip_t::MMC5: return INST_2A03;
		case sound_chip_t::VRC6: return INST_VRC6;
		// case sound_chip_t::VRC7: return INST_VRC7;
		case sound_chip_t::FDS:  return INST_FDS;
		case sound_chip_t::N163: return INST_N163;
		case sound_chip_t::S5B:  return INST_S5B;
		}
		return INST_NONE;
	}();
	*m_pDumpInstrument = Env.GetInstrumentService()->Make(Type);		// // //
	if (!*m_pDumpInstrument) return;

	auto str = std::string {GetChannelFullName(m_iRecordChannel)};
	(*m_pDumpInstrument)->SetName(u8"from " + str);

	if (Type == INST_FDS) {
		m_pSequenceCache[sequence_t::Arpeggio]->SetSetting(SETTING_ARP_FIXED);
		return;
	}
	if (auto Inst = dynamic_cast<CSeqInstrument *>(m_pDumpInstrument->get())) {
		foreachSeq([&] (sequence_t i) {
			Inst->SetSeqEnable(i, 1);
			Inst->SetSeqIndex(i, m_pModule->GetInstrumentManager()->GetFreeSequenceIndex(Type, i, nullptr));
		});
		m_pSequenceCache[sequence_t::Arpeggio]->SetSetting(SETTING_ARP_FIXED);
		// m_pSequenceCache[sequence_t::Pitch]->SetSetting(SETTING_PITCH_ABSOLUTE);
		// m_pSequenceCache[sequence_t::HiPitch]->SetSetting(SETTING_PITCH_ABSOLUTE);
		// VRC6 sawtooth 64-step volume
		if (m_iRecordChannel == chan_id_t::TRIANGLE)
			Inst->SetSeqEnable(sequence_t::DutyCycle, 0);
	}
	m_iRecordWaveSize = 32; // DEFAULT_WAVE_SIZE
	m_iRecordWaveCount = 0;
}

void CInstrumentRecorder::FinalizeRecordInstrument()
{
	if (*m_pDumpInstrument == NULL) return;
	inst_type_t InstType = (*m_pDumpInstrument)->GetType();
	CSeqInstrument *Inst = dynamic_cast<CSeqInstrument *>(m_pDumpInstrument->get());
	CInstrumentFDS *FDSInst = dynamic_cast<CInstrumentFDS *>(m_pDumpInstrument->get());
	CInstrumentN163 *N163Inst = dynamic_cast<CInstrumentN163 *>(m_pDumpInstrument->get());
	if (Inst != NULL) {
		(*m_pDumpInstrument)->RegisterManager(m_pModule->GetInstrumentManager());
		foreachSeq([&] (sequence_t i) {
			if (Inst->GetSeqEnable(i) != 0) {
				m_pSequenceCache[i]->SetLoopPoint(m_pSequenceCache[i]->GetItemCount() - 1);
				Inst->SetSequence(i, m_pSequenceCache[i]);
			}
			m_pSequenceCache[i] = std::make_shared<CSequence>(i);
		});
	}
	switch (InstType) {
	case INST_FDS:
		ASSERT(FDSInst != NULL);
		/*
		for (int i = 0; i < CInstrumentFDS::SEQUENCE_COUNT; i++) {
			const auto Seq = FDSInst->GetSequence(i);
			ASSERT(Seq != NULL);
			m_pSequenceCache[i]->SetLoopPoint(m_pSequenceCache[i]->GetItemCount() - 1);
			FDSInst->SetSequence(i, m_pSequenceCache[i]);
			m_pSequenceCache[i] = std::make_shared<CSequence>(i);
		}
		*/
		for (int i = 0; i < CInstrumentFDS::WAVE_SIZE; i++)
			FDSInst->SetSample(i, 0x3F & m_pSoundGen->GetReg(sound_chip_t::FDS, 0x4040 + i));
		FDSInst->SetModulationDepth(0x3F & m_pSoundGen->GetReg(sound_chip_t::FDS, 0x4084));
		FDSInst->SetModulationSpeed(m_pSoundGen->GetReg(sound_chip_t::FDS, 0x4086) | (0x0F & m_pSoundGen->GetReg(sound_chip_t::FDS, 0x4087)) << 8);
		break;
	case INST_N163:
		ASSERT(N163Inst != NULL);
		int offs = GetChannelSubIndex(m_iRecordChannel) << 3;
		int pos = m_pSoundGen->GetReg(sound_chip_t::N163, 0x7E - offs);
		N163Inst->SetWavePos(pos);
		N163Inst->SetWaveSize(m_iRecordWaveSize);
		N163Inst->SetWaveCount(m_iRecordWaveCount + 1);
		if (m_iRecordWaveCache) {
			for (int i = 0; i <= m_iRecordWaveCount; i++) for (int j = 0; j < m_iRecordWaveSize; j++)
				N163Inst->SetSample(i, j, m_iRecordWaveCache[m_iRecordWaveSize * i + j]);
			m_iRecordWaveCache.reset();
		}
		else for (int j = 0; j < m_iRecordWaveSize; j++) // fallback for blank recording
			N163Inst->SetSample(0, j, 0);
		break;
	}
}
