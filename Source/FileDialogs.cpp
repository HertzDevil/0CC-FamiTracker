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

#include "FileDialogs.h"

CStringW LoadDefaultFilter(LPCWSTR Name, LPCWSTR Ext) {
	// Loads a single filter string including the all files option
	CStringW allFilter;
	VERIFY(allFilter.LoadStringW(AFX_IDS_ALLFILTER));
	return FormattedW(L"%s|%s|%s|*.*||", Name, Ext, (LPCWSTR)CStringW(MAKEINTRESOURCEW(AFX_IDS_ALLFILTER)));
}

CStringW LoadDefaultFilter(UINT nID, LPCWSTR Ext) {
	return LoadDefaultFilter(CStringW(MAKEINTRESOURCEW(nID)), Ext);
}

std::optional<CStringW> GetSavePath(const CStringW &initFName, const CStringW &initPath, const CStringW &FilterName, const CStringW &FilterExt) {
	CStringW defaultExt = FilterExt;
	int index = defaultExt.Find(L';');
	if (index != -1)
		defaultExt.Truncate(index);

	CStringW filter = LoadDefaultFilter(FilterName, FilterExt);
	CFileDialog FileDialog(FALSE, !defaultExt.IsEmpty() ? (LPCWSTR)defaultExt : nullptr, initFName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter);
	if (!initPath.IsEmpty())
		FileDialog.m_pOFN->lpstrInitialDir = initPath;

	if (FileDialog.DoModal() != IDOK)
		return std::nullopt;

	return FileDialog.GetPathName();
}

std::optional<CStringW> GetSavePath(const CStringW &initFName, const CStringW &initPath, UINT nFilterID, const CStringW &FilterExt) {
	return GetSavePath(initFName, initPath, CStringW(MAKEINTRESOURCEW(nFilterID)), FilterExt);
}
