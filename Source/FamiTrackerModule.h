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

#include <string>
#include "FamiTrackerTypes.h"

class CFamiTrackerModule {
public:
	static constexpr std::size_t METADATA_FIELD_LENGTH = 32;

	CFamiTrackerModule() = default;
	~CFamiTrackerModule();

	void DeleteContents();

	// module metadata
	std::string_view GetModuleName() const;
	std::string_view GetModuleArtist() const;
	std::string_view GetModuleCopyright() const;
	std::string_view GetComment() const;
	bool ShowsCommentOnOpen() const;
	void SetModuleName(std::string_view sv);
	void SetModuleArtist(std::string_view sv);
	void SetModuleCopyright(std::string_view sv);
	void SetComment(std::string_view comment, bool showOnOpen);

	// global info
	machine_t GetMachine() const;
	unsigned int GetEngineSpeed() const;
	vibrato_t GetVibratoStyle() const;
	bool GetLinearPitch() const;
	int GetSpeedSplitPoint() const;

	void SetMachine(machine_t Machine);
	void SetEngineSpeed(unsigned int Speed);
	void SetVibratoStyle(vibrato_t Style);
	void SetLinearPitch(bool Enable);
	void SetSpeedSplitPoint(int SplitPoint);

	// detune
	int GetDetuneOffset(int Chip, int Note) const;
	void SetDetuneOffset(int Chip, int Note, int Detune);
	void ResetDetuneTables();
	int GetTuningSemitone() const;		// // // 050B
	int GetTuningCent() const;		// // // 050B
	void SetTuning(int Semitone, int Cent);		// // // 050B

private:
	machine_t		m_iMachine;
	unsigned int	m_iEngineSpeed;
	vibrato_t		m_iVibratoStyle;
	bool			m_bLinearPitch;
	unsigned int	m_iSpeedSplitPoint;
	int				m_iDetuneTable[6][96] = { };		// // // Detune tables
	int				m_iDetuneSemitone, m_iDetuneCent;		// // // 050B tuning

	std::string m_strName;
	std::string m_strArtist;
	std::string m_strCopyright;

	std::string m_strComment;
	bool m_bDisplayComment = false;
};
