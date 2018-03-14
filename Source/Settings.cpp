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

/*
 *  Add new program settings to the SetupSettings function,
 *  three macros are provided for the type of setting you want to add.
 *  (SETTING_INT, SETTING_BOOL, SETTING_STRING)
 *
 */

#include "Settings.h"

// CSettings

CSettings &CSettings::GetInstance()		// // //
{
	static CSettings Object;
	return Object;
}

// CSettings member functions

CStringW &CSettings::GetPath(unsigned int PathType)		// // //
{
	ASSERT(PathType < PATH_COUNT);
	return Paths[PathType];
}

const CStringW &CSettings::GetPath(unsigned int PathType) const
{
	ASSERT(PathType < PATH_COUNT);
	return Paths[PathType];
}

void CSettings::SetPath(const CStringW &PathName, unsigned int PathType)
{
	ASSERT(PathType < PATH_COUNT);

	// Remove file name if there is a
	if (PathName.Right(1) == L"\\" || PathName.Find(L'\\') == -1)
		Paths[PathType] = PathName;
	else
		Paths[PathType] = PathName.Left(PathName.ReverseFind(L'\\'));
}
