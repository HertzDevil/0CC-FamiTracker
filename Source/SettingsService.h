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
#include <vector>
#include <memory>
#include "stdafx.h"

class CSettings;

// // // moved from Settings.h

// // // helper class for loading settings from official famitracker
struct stOldSettingContext {
	stOldSettingContext();
	~stOldSettingContext();
};

// Base class for settings
class CSettingBase {
public:
	CSettingBase(LPCWSTR pSection, LPCWSTR pEntry);
	virtual ~CSettingBase() noexcept = default;		// // //
	virtual void LoadFromRegistry() = 0;
	virtual void SaveToRegistry() = 0;
	virtual void UseDefaultSetting() = 0;
	virtual void UpdateDefault(LPCWSTR pSection, LPCWSTR pEntry);		// // /

protected:
	LPCWSTR m_pSection;
	LPCWSTR m_pEntry;
	LPCWSTR m_pSectionSecond = nullptr;		// // //
	LPCWSTR m_pEntrySecond = nullptr;		// // //
};

class CSettingsService {
public:
	CSettingsService();
	CSettingsService(const CSettingsService &) = delete;
	CSettingsService(CSettingsService &&) = delete;

	CSettings *GetSettings();
	const CSettings *GetSettings() const;
	void LoadSettings();
	void SaveSettings();
	void DefaultSettings();
	void DeleteSettings();

private:
	template <typename T>
	CSettingBase &NewSetting(LPCWSTR Section, LPCWSTR Entry, T Default, T &Variable);

	void SetupSettings();

	std::vector<std::unique_ptr<CSettingBase>> m_pSettings;

	CSettings *m_pSettingData = nullptr;
};
