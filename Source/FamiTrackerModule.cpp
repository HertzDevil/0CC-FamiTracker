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

#include "FamiTrackerModule.h"
#include "SongData.h"
#include "InstrumentManager.h"
#include "ChannelMap.h"
#include "SongView.h"
#include "PeriodTables.h"
#include <cmath>

#include "InstrumentManager.h"
#include "Instrument2A03.h"
#include "DSampleManager.h"
#include "Sequence.h"

CFamiTrackerModule::CFamiTrackerModule() :
	m_pChannelMap(std::make_unique<CChannelMap>()),
	m_pInstrumentManager(std::make_unique<CInstrumentManager>())
{
	AllocateSong(0);
}

CFamiTrackerModule::~CFamiTrackerModule() {
}

std::string_view CFamiTrackerModule::GetModuleName() const {
	return m_strName;
}

std::string_view CFamiTrackerModule::GetModuleArtist() const {
	return m_strArtist;
}

std::string_view CFamiTrackerModule::GetModuleCopyright() const {
	return m_strCopyright;
}

std::string_view CFamiTrackerModule::GetComment() const {
	return m_strComment;
}

bool CFamiTrackerModule::ShowsCommentOnOpen() const {
	return m_bDisplayComment;
}

void CFamiTrackerModule::SetModuleName(std::string_view sv) {
	m_strName = sv.substr(0, METADATA_FIELD_LENGTH - 1);		// // //
}

void CFamiTrackerModule::SetModuleArtist(std::string_view sv) {
	m_strArtist = sv.substr(0, METADATA_FIELD_LENGTH - 1);		// // //
}

void CFamiTrackerModule::SetModuleCopyright(std::string_view sv) {
	m_strCopyright = sv.substr(0, METADATA_FIELD_LENGTH - 1);		// // //
}

void CFamiTrackerModule::SetComment(std::string_view comment, bool showOnOpen) {
	m_strComment = comment;
	m_bDisplayComment = showOnOpen;
}

CChannelOrder &CFamiTrackerModule::GetChannelOrder() const {
	return m_pChannelMap->GetChannelOrder();
}

CSoundChipSet CFamiTrackerModule::GetSoundChipSet() const {
	return m_pChannelMap->GetExpansionFlag();
}

void CFamiTrackerModule::SetChannelMap(std::unique_ptr<CChannelMap> pMap) {
	m_pChannelMap = std::move(pMap);
	if (HasExpansionChips())
		SetMachine(machine_t::NTSC);
}

bool CFamiTrackerModule::HasExpansionChips() const {
	return GetSoundChipSet().WithoutChip(sound_chip_t::APU).HasChips();
}

bool CFamiTrackerModule::HasExpansionChip(sound_chip_t Chip) const {
	return m_pChannelMap->HasExpansionChip(Chip);
}

int CFamiTrackerModule::GetNamcoChannels() const {
	return m_pChannelMap->GetChipChannelCount(sound_chip_t::N163);
}

std::unique_ptr<CSongView> CFamiTrackerModule::MakeSongView(unsigned index) {		// // //
	return std::make_unique<CSongView>(GetChannelOrder(), *GetSong(index));
}

std::unique_ptr<CConstSongView> CFamiTrackerModule::MakeSongView(unsigned index) const {		// // //
	return std::make_unique<CConstSongView>(GetChannelOrder(), *GetSong(index));
}

machine_t CFamiTrackerModule::GetMachine() const {
	return m_iMachine;
}

unsigned int CFamiTrackerModule::GetEngineSpeed() const {
	return m_iEngineSpeed;
}

unsigned int CFamiTrackerModule::GetFrameRate() const {
	unsigned Rate = GetEngineSpeed();		// // //
	if (Rate == 0)
		return (GetMachine() == machine_t::NTSC) ? FRAME_RATE_NTSC : FRAME_RATE_PAL;
	return Rate;
}

vibrato_t CFamiTrackerModule::GetVibratoStyle() const {
	return m_iVibratoStyle;
}

bool CFamiTrackerModule::GetLinearPitch() const {
	return m_bLinearPitch;
}

int CFamiTrackerModule::GetSpeedSplitPoint() const {
	return m_iSpeedSplitPoint;
}

void CFamiTrackerModule::SetMachine(machine_t machine) {
	m_iMachine = machine; // enum_cast<machine_t>(machine);
}

void CFamiTrackerModule::SetEngineSpeed(unsigned int speed) {
	if (speed > 0 && speed < 16)
		speed = 16;
	m_iEngineSpeed = speed;
}

void CFamiTrackerModule::SetVibratoStyle(vibrato_t style) {
	m_iVibratoStyle = style;
}

void CFamiTrackerModule::SetLinearPitch(bool enable) {
	m_bLinearPitch = enable;
}

void CFamiTrackerModule::SetSpeedSplitPoint(int splitPoint) {
	m_iSpeedSplitPoint = splitPoint;
}

std::array<int, 256> CFamiTrackerModule::MakeVibratoTable() const {		// // //
	const double NEW_VIBRATO_DEPTH[] = {
		1.0, 1.5, 2.5, 4.0, 5.0, 7.0, 10.0, 12.0, 14.0, 17.0, 22.0, 30.0, 44.0, 64.0, 96.0, 128.0,
	};

	const double OLD_VIBRATO_DEPTH[] = {
		1.0, 1.0, 2.0, 3.0, 4.0, 7.0, 8.0, 15.0, 16.0, 31.0, 32.0, 63.0, 64.0, 127.0, 128.0, 255.0,
	};

	std::array<int, 256> table = { };
	const double PI = std::acos(-1);

	for (int depth = 0; depth < 16; ++depth) {
		for (int phase = 0; phase < 16; ++phase) {
			auto t = (double)phase;
			table[depth * 16 + phase] = (int)(GetVibratoStyle() == vibrato_t::Bidir ?
				(std::sin(t * PI / 32.) * NEW_VIBRATO_DEPTH[depth] /*+ .5*/) :
				(t * OLD_VIBRATO_DEPTH[depth] / 16. + 1.));
		}
	}

	return table;
}

CPeriodTables CFamiTrackerModule::MakePeriodTables() const {		// // //
	CPeriodTables table;

	const double A440_NOTE = 45. - GetTuningSemitone() - GetTuningCent() / 100.;
	const double clock_ntsc = MASTER_CLOCK_NTSC / 16.;
	const double clock_pal  = MASTER_CLOCK_PAL / 16.;

	for (int i = 0; i < NOTE_COUNT; ++i) {
		// Frequency (in Hz)
		double Freq = 440. * std::pow(2.0, (i - A440_NOTE) / 12.);
		double Pitch;

		// 2A07
		Pitch = (clock_pal / Freq) - 0.5;
		table.pal_period[i] = (unsigned int)(Pitch - GetDetuneOffset(1, i));		// // //

		// 2A03 / MMC5 / VRC6
		Pitch = (clock_ntsc / Freq) - 0.5;
		table.ntsc_period[i] = (unsigned int)(Pitch - GetDetuneOffset(0, i));		// // //
		table.s5b_period[i] = table.ntsc_period[i] + 1;		// correction

		// VRC6 Saw
		Pitch = ((clock_ntsc * 16.0) / (Freq * 14.0)) - 0.5;
		table.saw_period[i] = (unsigned int)(Pitch - GetDetuneOffset(2, i));		// // //

		// FDS
		Pitch = (Freq * 65536.0) / (clock_ntsc / 1.0) + 0.5;
		table.fds_freq[i] = (unsigned int)(Pitch + GetDetuneOffset(4, i));		// // //

		// N163
		Pitch = ((Freq * GetNamcoChannels() * 983040.0) / clock_ntsc + 0.5) / 4;		// // //
		table.n163_freq[i] = (unsigned int)(Pitch + GetDetuneOffset(5, i));		// // //

		if (table.n163_freq[i] > 0xFFFF)	// 0x3FFFF
			table.n163_freq[i] = 0xFFFF;	// 0x3FFFF

		// // // Sunsoft 5B uses NTSC table

		// // // VRC7
		if (i < NOTE_RANGE) {
			Pitch = Freq * 262144.0 / 49716.0 + 0.5;
			unsigned Reg = (unsigned int)(Pitch + GetDetuneOffset(3, i));
			for (int j = 0; j < OCTAVE_RANGE; ++j)
				table.vrc7_freq[i + j * NOTE_RANGE] = Reg;		// // //
		}
	}

	return table;
}

int CFamiTrackerModule::GetDetuneOffset(int chip, int note) const {		// // //
	return m_iDetuneTable[chip][note];
}

void CFamiTrackerModule::SetDetuneOffset(int chip, int note, int offset) {		// // //
	m_iDetuneTable[chip][note] = offset;
}

void CFamiTrackerModule::ResetDetuneTables() {		// // //
	for (auto &table : m_iDetuneTable)
		for (auto &d : table)
			d = 0;
}

int CFamiTrackerModule::GetTuningSemitone() const {		// // // 050B
	return m_iDetuneSemitone;
}

int CFamiTrackerModule::GetTuningCent() const {		// // // 050B
	return m_iDetuneCent;
}

void CFamiTrackerModule::SetTuning(int semitone, int cent) {		// // // 050B
	m_iDetuneSemitone = semitone;
	m_iDetuneCent = cent;
}

CSongData *CFamiTrackerModule::GetSong(unsigned index) {
	// Ensure track is allocated
	AllocateSong(index);
	return index < GetSongCount() ? m_pTracks[index].get() : nullptr;
}

const CSongData *CFamiTrackerModule::GetSong(unsigned index) const {
	return index < GetSongCount() ? m_pTracks[index].get() : nullptr;
}

std::size_t CFamiTrackerModule::GetSongCount() const {
	return m_pTracks.size();
}

std::unique_ptr<CSongData> CFamiTrackerModule::MakeNewSong() const {
	auto pSong = std::make_unique<CSongData>(CSongData::DEFAULT_ROW_COUNT);
	pSong->SetSongTempo(GetMachine() == machine_t::NTSC ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL);
	if (GetSongCount() > 0)
		pSong->SetRowHighlight(GetSong(0)->GetRowHighlight());
	return pSong;
}

CInstrumentManager *CFamiTrackerModule::GetInstrumentManager() const {
	return m_pInstrumentManager.get();
}

CSequenceManager *CFamiTrackerModule::GetSequenceManager(int InstType) const {
	return GetInstrumentManager()->GetSequenceManager(InstType);
}

CDSampleManager *CFamiTrackerModule::GetDSampleManager() const {
	return GetInstrumentManager()->GetDSampleManager();
}

void CFamiTrackerModule::SwapInstruments(unsigned first, unsigned second) {
	// Swap instruments
	GetInstrumentManager()->SwapInstruments(first, second);		// // //

	// Scan patterns
	VisitSongs([&] (CSongData &song) {
		song.VisitPatterns([&] (CPatternData &pat) {
			pat.VisitRows([&] (stChanNote &note, unsigned row) {
				if (note.Instrument == first)
					note.Instrument = second;
				else if (note.Instrument == second)
					note.Instrument = first;
			});
		});
	});
}

bool CFamiTrackerModule::AllocateSong(unsigned index) {
	// Allocate a new song if not already done
	for (unsigned i = GetSongCount(); i <= index; ++i)
		if (!InsertSong(i, MakeNewSong()))
			return false;
	return true;
}

bool CFamiTrackerModule::InsertSong(unsigned index, std::unique_ptr<CSongData> pSong) {		// // //
	if (index <= GetSongCount() && index < MAX_TRACKS) {
		m_pTracks.insert(m_pTracks.begin() + index, std::move(pSong));
		return true;
	}
	return false;
}

std::unique_ptr<CSongData> CFamiTrackerModule::ReplaceSong(unsigned index, std::unique_ptr<CSongData> pSong) {		// // //
	m_pTracks[index].swap(pSong);
	return pSong;
}

std::unique_ptr<CSongData> CFamiTrackerModule::ReleaseSong(unsigned index) {		// // //
	if (index >= GetSongCount())
		return nullptr;

	// Move down all other tracks
	auto song = std::move(m_pTracks[index]);
	m_pTracks.erase(m_pTracks.cbegin() + index);		// // //
	if (!GetSongCount())
		AllocateSong(0);

	return song;
}

void CFamiTrackerModule::RemoveSong(unsigned index) {
	(void)ReleaseSong(index);
}

void CFamiTrackerModule::SwapSongs(unsigned lhs, unsigned rhs) {
	m_pTracks[lhs].swap(m_pTracks[rhs]);		// // //
}

std::shared_ptr<ft0cc::doc::groove> CFamiTrackerModule::GetGroove(unsigned index) {
	return index < MAX_GROOVE ? m_pGrooveTable[index] : nullptr;
}

std::shared_ptr<const ft0cc::doc::groove> CFamiTrackerModule::GetGroove(unsigned index) const {
	return index < MAX_GROOVE ? m_pGrooveTable[index] : nullptr;
}

bool CFamiTrackerModule::HasGroove(unsigned index) const {
	return index < MAX_GROOVE && static_cast<bool>(m_pGrooveTable[index]);
}

void CFamiTrackerModule::SetGroove(unsigned index, std::shared_ptr<ft0cc::doc::groove> pGroove) {
	m_pGrooveTable[index] = std::move(pGroove);
}

unsigned CFamiTrackerModule::GetGrooveCount() const {
	unsigned count = 0;
	for (auto &x : m_pGrooveTable)
		if (x)
			++count;
	return count;
}

const stHighlight &CFamiTrackerModule::GetHighlight(unsigned song) const {		// // //
	return GetSong(song)->GetRowHighlight();
}

void CFamiTrackerModule::SetHighlight(const stHighlight &hl) {		// // //
	VisitSongs([&hl] (CSongData &song) {
		song.SetRowHighlight(hl);
	});
}

void CFamiTrackerModule::SetHighlight(unsigned song, const stHighlight &hl) {		// // //
	GetSong(song)->SetRowHighlight(hl);
}

void CFamiTrackerModule::RemoveUnusedPatterns() {
	const CChannelOrder &order = GetChannelOrder();

	VisitSongs([&] (CSongData &song) {
		song.VisitPatterns([&] (CPatternData &pattern, stChannelID c, unsigned p) {
			if (!order.HasChannel(c) || !song.IsPatternInUse(c, p))
				pattern = CPatternData { };
		});
	});
}

void CFamiTrackerModule::RemoveUnusedInstruments() {
	bool used[MAX_INSTRUMENTS] = { };		// // //

	VisitSongs([&] (const CSongData &song) {		// // //
		int length = song.GetPatternLength();
		GetChannelOrder().ForeachChannel([&] (stChannelID Channel) {
			for (unsigned int Frame = 0; Frame < song.GetFrameCount(); ++Frame)
				song.GetPatternOnFrame(Channel, Frame).VisitRows(length, [&] (const stChanNote &note) {
					if (note.Instrument < MAX_INSTRUMENTS)
						used[note.Instrument] = true;
				});
		});
	});

	auto *pManager = GetInstrumentManager();

	pManager->VisitInstruments([&] (CInstrument &, std::size_t i) {
		if (!used[i])
			pManager->RemoveInstrument(i);
	});

	const inst_type_t inst[] = {INST_2A03, INST_VRC6, INST_N163, INST_S5B};

	// Also remove unused sequences
	for (unsigned int i = 0; i < MAX_SEQUENCES; ++i)
		foreachSeq([&] (sequence_t j) {		// // //
			for (auto c : inst)
				if (auto pSeq = pManager->GetSequence(c, j, i); pSeq && pSeq->GetItemCount() > 0) {		// // //
					bool Used = false;
					for (int k = 0; k < MAX_INSTRUMENTS; ++k) {
						if (pManager->IsInstrumentUsed(k) && pManager->GetInstrumentType(k) == c) {
							auto pInstrument = std::static_pointer_cast<CSeqInstrument>(pManager->GetInstrument(k));
							if (pInstrument->GetSeqIndex(j) == i && pInstrument->GetSeqEnable(j)) {		// // //
								Used = true; break;
							}
						}
					}
					if (!Used)
						pSeq->Clear();		// // //
				}
	});
}

void CFamiTrackerModule::RemoveUnusedDSamples() {
	bool AssignUsed[MAX_INSTRUMENTS][NOTE_COUNT] = { };

	auto &Manager = *GetDSampleManager();
	auto &InstManager = *GetInstrumentManager();

	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (Manager.IsSampleUsed(i)) {
			bool Used = false;
			VisitSongs([&] (const CSongData &song) {
				for (unsigned int Frame = 0; Frame < song.GetFrameCount(); ++Frame) {
					unsigned int Pattern = song.GetFramePattern(Frame, apu_subindex_t::dpcm);
					for (unsigned int Row = 0; Row < song.GetPatternLength(); ++Row) {
						const auto &Note = song.GetPatternData(apu_subindex_t::dpcm, Pattern, Row);		// // //
						int Index = Note.Instrument;
						if (!IsNote(Note.Note) || Index == MAX_INSTRUMENTS)
							continue;		// // //
						if (InstManager.GetInstrumentType(Index) != INST_2A03)
							continue;
						AssignUsed[Index][MIDI_NOTE(Note.Octave, Note.Note)] = true;
						auto pInst = std::static_pointer_cast<CInstrument2A03>(InstManager.GetInstrument(Index));
						if (pInst->GetSampleIndex(MIDI_NOTE(Note.Octave, Note.Note)) == i)
							Used = true;
					}
				}
			});
			if (!Used)
				Manager.RemoveDSample(i);
		}
	}
	// also remove unused assignments
	InstManager.VisitInstruments([&] (CInstrument &inst, std::size_t i) {
		if (auto pInst = dynamic_cast<CInstrument2A03 *>(&inst))
			for (int n = 0; n < NOTE_COUNT; ++n)
				if (!AssignUsed[i][n])
					pInst->SetSampleIndex(n, CInstrument2A03::NO_DPCM);
	});
}
