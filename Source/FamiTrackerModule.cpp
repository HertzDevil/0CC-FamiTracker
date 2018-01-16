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

CFamiTrackerModule::CFamiTrackerModule(CFTMComponentInterface &parent) :
	parent_(parent),
	m_pInstrumentManager(std::make_unique<CInstrumentManager>(&parent))
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

machine_t CFamiTrackerModule::GetMachine() const {
	return m_iMachine;
}

unsigned int CFamiTrackerModule::GetEngineSpeed() const {
	return m_iEngineSpeed;
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
	auto pSong = std::make_unique<CSongData>(parent_);
	pSong->SetSongTempo(GetMachine() == NTSC ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL);
	if (GetSongCount() > 0)
		pSong->SetRowHighlight(GetSong(0)->GetRowHighlight());
	return pSong;
}

CInstrumentManager *CFamiTrackerModule::GetInstrumentManager() const {
	return m_pInstrumentManager.get();
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

void CFamiTrackerModule::SetGroove(unsigned index, std::shared_ptr<groove> pGroove) {
	m_pGrooveTable[index] = std::move(pGroove);
}

const stHighlight &CFamiTrackerModule::GetHighlight(unsigned int song) const {		// // //
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
