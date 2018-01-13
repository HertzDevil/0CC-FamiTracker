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

void CFamiTrackerModule::SetModuleName(std::string_view sv) {
	m_strName = sv.substr(0, METADATA_FIELD_LENGTH - 1);		// // //
}

void CFamiTrackerModule::SetModuleArtist(std::string_view sv) {
	m_strArtist = sv.substr(0, METADATA_FIELD_LENGTH - 1);		// // //
}

void CFamiTrackerModule::SetModuleCopyright(std::string_view sv) {
	m_strCopyright = sv.substr(0, METADATA_FIELD_LENGTH - 1);		// // //
}

std::string_view CFamiTrackerModule::GetComment() const {
	return m_strComment;
}

bool CFamiTrackerModule::ShowsCommentOnOpen() const {
	return m_bDisplayComment;
}

void CFamiTrackerModule::SetComment(std::string_view comment, bool showOnOpen) {
	m_strComment = comment;
	m_bDisplayComment = showOnOpen;
}
