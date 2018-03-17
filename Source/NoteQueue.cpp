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

#include "NoteQueue.h"
#include "APU/Types.h"

enum class CNoteChannelQueue::note_state_t : unsigned char {HOLD, RELEASE};

CNoteChannelQueue::CNoteChannelQueue(const std::vector<chan_id_t> &Ch) :
	m_iChannelCount(Ch.size()),
	m_iChannelMapID(Ch),
	m_iCurrentNote(Ch.size(), -1),
	m_bChannelMute(Ch.size())
{
}

chan_id_t CNoteChannelQueue::Trigger(int Note, chan_id_t Channel)
{
	const auto AddNote = [&] (int Index) -> chan_id_t {
		m_iCurrentNote[Index] = Note;
		m_iNoteState[Note] = note_state_t::HOLD;
		int p = 0;
		for (const auto &x : m_iNotePriority)
			if (x.second > p)
				p = x.second;
		m_iNotePriority[Note] = p + 1;
		m_iNoteChannel[Note] = Channel;
		return m_iChannelMapID[Index];
	};

	if (!m_iNoteState.count(Note)) {
		int Pos = -1;
		for (int i = 0; i < m_iChannelCount; ++i)
			if (m_iChannelMapID[i] == Channel) { Pos = i; break; }
		for (int i = 0; i < m_iChannelCount; ++i) {
			if (!m_bChannelMute[Pos] && m_iCurrentNote[Pos] == (unsigned)-1)
				return AddNote(Pos);
			if (++Pos >= m_iChannelCount) Pos = 0;
		}
	}

	if (!m_iNoteState.count(Note)) {
		int p = 0x7FFFFFFF;
		int c = -1;
		for (int i = 0; i < m_iChannelCount; ++i) {
			if (m_bChannelMute[i])
				continue;
			int n = m_iCurrentNote[i];
			if (n && m_iNoteState[n] == note_state_t::RELEASE) {
				int pnew = m_iNotePriority[n];
				if (pnew < p) {
					p = pnew; c = i;
				}
			}
		}
		if (c == -1) for (int i = 0; i < m_iChannelCount; ++i) {
			if (m_bChannelMute[i])
				continue;
			int n = m_iCurrentNote[i];
			if (n && m_iNoteState[n] == note_state_t::HOLD) {
				int pnew = m_iNotePriority[n];
				if (pnew < p) {
					p = pnew; c = i;
				}
			}
		}
		if (c != -1) {
			Cut(m_iCurrentNote[c], chan_id_t::NONE);
			return AddNote(c);
		}
	}

	for (int i = 0; i < m_iChannelCount; ++i)
		if (m_iCurrentNote[i] == (unsigned)Note && m_iNoteChannel[m_iCurrentNote[i]] == Channel)
			return AddNote(i);

	return chan_id_t::NONE;
}

chan_id_t CNoteChannelQueue::Release(int Note, chan_id_t Channel)
{
	auto it = m_iNoteState.find(Note);
	if (it != m_iNoteState.end()) {
		if (it->second == note_state_t::HOLD)
			it->second = note_state_t::RELEASE;
		for (int i = 0; i < m_iChannelCount; ++i)
			if (m_iCurrentNote[i] == (unsigned)Note)
				return m_iChannelMapID[i];
	}
	return chan_id_t::NONE;
}

chan_id_t CNoteChannelQueue::Cut(int Note, chan_id_t Channel)
{
	auto it = m_iNoteState.find(Note);
	if (it != m_iNoteState.end()) {
		m_iNoteState.erase(it);
		auto pit = m_iNotePriority.find(Note);
		if (pit != m_iNotePriority.end()) m_iNotePriority.erase(pit);
		auto pit2 = m_iNoteChannel.find(Note);
		if (pit2 != m_iNoteChannel.end()) m_iNoteChannel.erase(pit2);
		for (int i = 0; i < m_iChannelCount; ++i)
			if (m_iCurrentNote[i] == (unsigned)Note) {
				m_iCurrentNote[i] = -1;
				return m_iChannelMapID[i];
			}
	}
	return chan_id_t::NONE;
}

std::vector<chan_id_t> CNoteChannelQueue::StopChannel(chan_id_t Channel)
{
	std::unordered_map<int, chan_id_t> m {m_iNoteChannel};
	std::vector<chan_id_t> v;

	for (auto [note, ch] : m) if (ch == Channel) {
		Cut(note, ch);
		v.push_back(ch);
	}

	return v;
}

void CNoteChannelQueue::StopAll()
{
	std::unordered_map<int, chan_id_t> m {m_iNoteChannel};
	for (auto [note, ch] : m)
		Cut(note, ch);
}

void CNoteChannelQueue::MuteChannel(chan_id_t Channel)
{
	for (int i = 0; i < m_iChannelCount; ++i)
		if (m_iChannelMapID[i] == Channel && !m_bChannelMute[i]) {
			StopChannel(Channel);
			m_bChannelMute[i] = true;
		}
}

void CNoteChannelQueue::UnmuteChannel(chan_id_t Channel)
{
	for (int i = 0; i < m_iChannelCount; ++i)
		if (m_iChannelMapID[i] == Channel && m_bChannelMute[i])
			m_bChannelMute[i] = false;
}



void CNoteQueue::AddMap(const std::vector<chan_id_t> &Ch)
{
	auto ptr = std::make_shared<CNoteChannelQueue>(Ch);
	for (const auto &x : Ch)
		m_Part[x] = ptr;
}

void CNoteQueue::ClearMaps()
{
	m_Part.clear();
}

chan_id_t CNoteQueue::Trigger(int Note, chan_id_t Channel)
{
	if (auto it = m_Part.find(Channel); it != m_Part.end())
		if (auto ret = it->second->Trigger(Note, Channel); ret != chan_id_t::NONE)
			return ret;
	return chan_id_t::NONE;
}

chan_id_t CNoteQueue::Release(int Note, chan_id_t Channel)
{
	for (const auto &it : m_Part)
		if (auto ret = it.second->Release(Note, Channel); ret != chan_id_t::NONE)
			return ret;
	return chan_id_t::NONE;
}

chan_id_t CNoteQueue::Cut(int Note, chan_id_t Channel)
{
	for (const auto &it : m_Part)
		if (auto ret = it.second->Cut(Note, Channel); ret != chan_id_t::NONE)
			return ret;
	return chan_id_t::NONE;
}

std::vector<chan_id_t> CNoteQueue::StopChannel(chan_id_t Channel)
{
	std::vector<chan_id_t> v;
	for (const auto &it : m_Part) {
		auto ret = it.second->StopChannel(Channel);
		v.insert(v.end(), ret.begin(), ret.end());
	}
	return v;
}

void CNoteQueue::StopAll()
{
	for (const auto &it : m_Part)
		it.second->StopAll();
}

void CNoteQueue::MuteChannel(chan_id_t Channel)
{
	for (const auto &it : m_Part)
		it.second->MuteChannel(Channel);
}

void CNoteQueue::UnmuteChannel(chan_id_t Channel)
{
	for (const auto &it : m_Part)
		it.second->UnmuteChannel(Channel);
}
