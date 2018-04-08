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

#include "SettingsService.h"
#include "Settings.h"
#include "FamiTrackerEnv.h"
#include "ColorScheme.h"
#include "ModuleException.h"
#include "ft0cc/enum_traits.h"

// Settings types

CSettingBase::CSettingBase(LPCWSTR pSection, LPCWSTR pEntry) :
	m_pSection(pSection), m_pEntry(pEntry)
{
}

void CSettingBase::UpdateDefault(LPCWSTR pSection, LPCWSTR pEntry) {		// // //
	m_pSectionSecond = pSection;
	m_pEntrySecond = pEntry;
}



namespace {

template <typename T>
T GetRegValue(LPCWSTR section, LPCWSTR entry, const T &default_value) {
	if constexpr (std::is_enum_v<T>)
		return enum_cast<T>(Env.GetMainApp()->GetProfileIntW(section, entry, static_cast<int>(default_value)));
	else if constexpr (std::is_integral_v<T>)
		return static_cast<T>(Env.GetMainApp()->GetProfileIntW(section, entry, static_cast<int>(default_value)));
	else if constexpr (std::is_same_v<T, std::wstring>) {
		CStringW v = Env.GetMainApp()->GetProfileStringW(section, entry, default_value.data());
		return std::wstring {(LPCWSTR)v, static_cast<std::size_t>(v.GetLength())};
	}
	else if constexpr (std::is_same_v<T, fs::path>) {
		CStringW v = Env.GetMainApp()->GetProfileStringW(section, entry, default_value.c_str());
		return fs::path {(LPCWSTR)v, (LPCWSTR)v + v.GetLength()};
	}
	else
		static_assert(!sizeof(T), "Unknown value type");
}

template <typename T>
void SetRegValue(LPCWSTR section, LPCWSTR entry, const T &val) {
	if constexpr (std::is_enum_v<T>)
		Env.GetMainApp()->WriteProfileInt(section, entry, static_cast<int>(val));
	else if constexpr (std::is_integral_v<T>)
		Env.GetMainApp()->WriteProfileInt(section, entry, static_cast<int>(val));
	else if constexpr (std::is_same_v<T, std::wstring>)
		Env.GetMainApp()->WriteProfileStringW(section, entry, CStringW {val.data(), static_cast<int>(val.size())});
	else if constexpr (std::is_same_v<T, fs::path>) {
		auto wstr = val.wstring();
		Env.GetMainApp()->WriteProfileStringW(section, entry, CStringW {wstr.data(), static_cast<int>(wstr.size())});
	}
	else
		static_assert(!sizeof(T), "Unknown value type");
}

} // namespace



template <class T>
class CSettingType : public CSettingBase {
public:
	using setting_type = T;

	using CSettingBase::CSettingBase;

private:
	void LoadFromRegistry() override {
		setting_type Value = GetDefaultSetting();		// // //
		{
			stOldSettingContext s;
			if (m_pSectionSecond)
				Value = GetRegValue(m_pSectionSecond, m_pEntrySecond, Value);
			Value = GetRegValue(m_pSection, m_pEntry, Value);
		}
		if (m_pSectionSecond)
			Value = GetRegValue(m_pSectionSecond, m_pEntrySecond, Value);
		WriteSetting(GetRegValue(m_pSection, m_pEntry, Value));
	}

	void SaveToRegistry() override {
		SetRegValue(m_pSection, m_pEntry, ReadSetting());
	}

	void UseDefaultSetting() override {
		WriteSetting(GetDefaultSetting());
	}

	virtual setting_type ReadSetting() const = 0;
	virtual setting_type GetDefaultSetting() const = 0;
	virtual void WriteSetting(const setting_type &val) = 0;
};

template <class T>
class CSettingPublic : public CSettingType<T> {
public:
	using setting_type = T;

	CSettingPublic(LPCWSTR pSection, LPCWSTR pEntry, const setting_type &defaultVal, setting_type *pVar) :
		CSettingType<T>(pSection, pEntry), m_tDefaultValue(defaultVal), m_pVariable(pVar)
	{
	}

private:
	setting_type ReadSetting() const override {
		return *m_pVariable;
	}

	setting_type GetDefaultSetting() const override {
		return m_tDefaultValue;
	}

	void WriteSetting(const setting_type &val) override {
		*m_pVariable = val;
	}

	setting_type *m_pVariable = nullptr;
	setting_type m_tDefaultValue;
};

class CSettingPath : public CSettingType<fs::path> {
public:
	using setting_type = fs::path;

	CSettingPath(LPCWSTR pSection, LPCWSTR pEntry, PATHS path_type) :
		CSettingType(pSection, pEntry), m_iPathType(path_type)
	{
	}

private:
	setting_type ReadSetting() const override {
		return CSettings::GetInstance().GetPath(m_iPathType);
	}

	setting_type GetDefaultSetting() const override {
		return L"";
	}

	void WriteSetting(const setting_type &val) override {
		CSettings::GetInstance().SetPath(val, m_iPathType);
	}

	PATHS m_iPathType;
};



// // // registry context

stOldSettingContext::stOldSettingContext()
{
	std::free((void*)Env.GetMainApp()->m_pszProfileName);
	Env.GetMainApp()->m_pszProfileName = L"FamiTracker";
}

stOldSettingContext::~stOldSettingContext()
{
	CStringW s(MAKEINTRESOURCEW(AFX_IDS_APP_TITLE));
	Env.GetMainApp()->m_pszProfileName = _wcsdup(s);
}



CSettingsService::CSettingsService() :
	m_pSettingData(&CSettings::GetInstance())
{
	SetupSettings();
	TRACE(L"Settings: Added %u settings\n", m_pSettings.size());
}

CSettings *CSettingsService::GetSettings() {
	return m_pSettingData;
}

const CSettings *CSettingsService::GetSettings() const {
	return m_pSettingData;
}

void CSettingsService::LoadSettings() {
	for (auto &x : m_pSettings)
		x->LoadFromRegistry();
}

void CSettingsService::SaveSettings() {
	for (auto &x : m_pSettings)
		x->SaveToRegistry();
}

void CSettingsService::DefaultSettings() {
	for (auto &x : m_pSettings)
		x->UseDefaultSetting();
}

void CSettingsService::DeleteSettings() {
	// Delete all settings from registry
	HKEY hKey = Env.GetMainApp()->GetAppRegistryKey();
	Env.GetMainApp()->DelRegTree(hKey, L"");
}

template <typename T>
CSettingBase &CSettingsService::NewSetting(LPCWSTR Section, LPCWSTR Entry, T Default, T &Variable) {
	return *m_pSettings.emplace_back(std::make_unique<CSettingPublic<T>>(Section, Entry, std::move(Default), &Variable));
}

void CSettingsService::SetupSettings() {
	//
	// This function defines all settings in the program that are stored in registry
	// All settings are loaded on program start and saved when closing the program
	//

	const auto NewPathSetting = [&] (LPCWSTR Section, LPCWSTR Entry, PATHS PathType) {
		return m_pSettings.emplace_back(std::make_unique<CSettingPath>(Section, Entry, PathType)).get();
	};

	auto &s = CSettings::GetInstance();

	stOldSettingContext cxt;		// // //
	/*
		location priorities:
		1. HKCU/SOFTWARE/0CC-FamiTracker
		2. HKCU/SOFTWARE/0CC-FamiTracker, original key (using UpdateDefault)
		3. HKCU/SOFTWARE/FamiTracker
		4. HKCU/SOFTWARE/FamiTracker, original key (using UpdateDefault)
		5. Default value
	*/

	// General
	NewSetting(L"General", L"Edit style", edit_style_t::FT2, s.General.iEditStyle);
	NewSetting(L"General", L"Page step size", 4, s.General.iPageStepSize);
	NewSetting(L"General", L"Wrap cursor", true, s.General.bWrapCursor);
	NewSetting(L"General", L"Wrap across frames", true, s.General.bWrapFrames);
	NewSetting(L"General", L"Free cursor edit",	false, s.General.bFreeCursorEdit);
	NewSetting(L"General", L"Wave preview", true, s.General.bWavePreview);
	NewSetting(L"General", L"Key repeat", true, s.General.bKeyRepeat);
	NewSetting(L"General", L"Hex row display", true, s.General.bRowInHex);
	NewSetting(L"General", L"Frame preview", true, s.General.bFramePreview);
	NewSetting(L"General", L"No DPCM reset", false, s.General.bNoDPCMReset);
	NewSetting(L"General", L"No Step moving", false, s.General.bNoStepMove);
	NewSetting(L"General", L"Delete pull up", false, s.General.bPullUpDelete);
	NewSetting(L"General", L"Backups", false, s.General.bBackups);
	NewSetting(L"General", L"Single instance", false, s.General.bSingleInstance);
	NewSetting(L"General", L"Preview full row", false, s.General.bPreviewFullRow);
	NewSetting(L"General", L"Double click selection", false, s.General.bDblClickSelect);
	// // //
	NewSetting(L"General", L"Wrap pattern values", false, s.General.bWrapPatternValue);
	NewSetting(L"General", L"Cut sub-volume", false, s.General.bCutVolume);
	NewSetting(L"General", L"Use old FDS volume table", false, s.General.bFDSOldVolume);
	NewSetting(L"General", L"Retrieve channel state", false, s.General.bRetrieveChanState);
	NewSetting(L"General", L"Overflow paste mode", false, s.General.bOverflowPaste);
	NewSetting(L"General", L"Show skipped rows", false, s.General.bShowSkippedRows);
	NewSetting(L"General", L"Hexadecimal keypad", false, s.General.bHexKeypad);
	NewSetting(L"General", L"Multi-frame selection", false, s.General.bMultiFrameSel);
	NewSetting(L"General", L"Check for new versions", true, s.General.bCheckVersion);

	// // // Version / Compatibility info
	NewSetting(L"Version", L"Module error level", MODULE_ERROR_DEFAULT, s.Version.iErrorLevel);

	// Keys
	NewSetting(L"Keys", L"Note cut", (int)'1', s.Keys.iKeyNoteCut);
	NewSetting(L"Keys", L"Note release", VK_OEM_5, s.Keys.iKeyNoteRelease); // \|
	NewSetting(L"Keys", L"Clear field", VK_OEM_MINUS, s.Keys.iKeyClear);
	NewSetting(L"Keys", L"Repeat", 0x00, s.Keys.iKeyRepeat);
	NewSetting(L"Keys", L"Echo buffer", 0x00, s.Keys.iKeyEchoBuffer);		// // //

	// Sound
	NewSetting(L"Sound", L"Audio Device", 0, s.Sound.iDevice);
	NewSetting(L"Sound", L"Sample rate", 44100, s.Sound.iSampleRate);
	NewSetting(L"Sound", L"Sample size", 16, s.Sound.iSampleSize);
	NewSetting(L"Sound", L"Buffer length", 40, s.Sound.iBufferLength);
	NewSetting(L"Sound", L"Bass filter freq", 30, s.Sound.iBassFilter);
	NewSetting(L"Sound", L"Treble filter freq", 12000, s.Sound.iTrebleFilter);
	NewSetting(L"Sound", L"Treble filter damping", 24, s.Sound.iTrebleDamping);
	NewSetting(L"Sound", L"Volume", 100, s.Sound.iMixVolume);

	// Midi
	NewSetting(L"MIDI", L"Device", 0, s.Midi.iMidiDevice);
	NewSetting(L"MIDI", L"Out Device", 0, s.Midi.iMidiOutDevice);
	NewSetting(L"MIDI", L"Master sync", false, s.Midi.bMidiMasterSync);
	NewSetting(L"MIDI", L"Key release", false, s.Midi.bMidiKeyRelease);
	NewSetting(L"MIDI", L"Channel map", false, s.Midi.bMidiChannelMap);
	NewSetting(L"MIDI", L"Velocity control", false, s.Midi.bMidiVelocity);
	NewSetting(L"MIDI", L"Auto Arpeggio", false, s.Midi.bMidiArpeggio);

	// Appearance
	NewSetting(L"Appearance", L"Background", DEFAULT_COLOR_SCHEME.BACKGROUND, s.Appearance.iColBackground);
	NewSetting(L"Appearance", L"Background highlighted", DEFAULT_COLOR_SCHEME.BACKGROUND_HILITE, s.Appearance.iColBackgroundHilite);
	NewSetting(L"Appearance", L"Background highlighted 2", DEFAULT_COLOR_SCHEME.BACKGROUND_HILITE2, s.Appearance.iColBackgroundHilite2);
	NewSetting(L"Appearance", L"Pattern text", DEFAULT_COLOR_SCHEME.TEXT_NORMAL, s.Appearance.iColPatternText);
	NewSetting(L"Appearance", L"Pattern text highlighted", DEFAULT_COLOR_SCHEME.TEXT_HILITE, s.Appearance.iColPatternTextHilite);
	NewSetting(L"Appearance", L"Pattern text highlighted 2", DEFAULT_COLOR_SCHEME.TEXT_HILITE2, s.Appearance.iColPatternTextHilite2);
	NewSetting(L"Appearance", L"Pattern instrument", DEFAULT_COLOR_SCHEME.TEXT_INSTRUMENT, s.Appearance.iColPatternInstrument);
	NewSetting(L"Appearance", L"Pattern volume", DEFAULT_COLOR_SCHEME.TEXT_VOLUME, s.Appearance.iColPatternVolume);
	NewSetting(L"Appearance", L"Pattern effect", DEFAULT_COLOR_SCHEME.TEXT_EFFECT, s.Appearance.iColPatternEffect);
	NewSetting(L"Appearance", L"Selection", DEFAULT_COLOR_SCHEME.SELECTION, s.Appearance.iColSelection);
	NewSetting(L"Appearance", L"Cursor", DEFAULT_COLOR_SCHEME.CURSOR, s.Appearance.iColCursor);
	// // //
	NewSetting(L"Appearance", L"Current row (normal mode)", DEFAULT_COLOR_SCHEME.ROW_NORMAL, s.Appearance.iColCurrentRowNormal);
	NewSetting(L"Appearance", L"Current row (edit mode)", DEFAULT_COLOR_SCHEME.ROW_EDIT, s.Appearance.iColCurrentRowEdit);
	NewSetting(L"Appearance", L"Current row (playing)", DEFAULT_COLOR_SCHEME.ROW_PLAYING, s.Appearance.iColCurrentRowPlaying);
	NewSetting(L"Appearance", L"Pattern font", std::wstring {FONT_FACE}, s.Appearance.strFont)
		.UpdateDefault(L"General", L"Pattern font");
	NewSetting(L"Appearance", L"Pattern font size", FONT_SIZE, s.Appearance.iFontSize)
		.UpdateDefault(L"General", L"Pattern font size");
	NewSetting(L"Appearance", L"Frame font", std::wstring {FONT_FACE}, s.Appearance.strFrameFont)
		.UpdateDefault(L"General", L"Frame font");		// // // 050B
	NewSetting(L"Appearance", L"Pattern colors", true, s.Appearance.bPatternColor)
		.UpdateDefault(L"Appearance", L"Pattern colors");
	NewSetting(L"Appearance", L"Display flats", false, s.Appearance.bDisplayFlats)
		.UpdateDefault(L"Appearance", L"Display flats");

	// Window position
	NewSetting(L"Window position", L"Left", 100, s.WindowPos.iLeft);
	NewSetting(L"Window position", L"Top", 100, s.WindowPos.iTop);
	NewSetting(L"Window position", L"Right", 950, s.WindowPos.iRight);
	NewSetting(L"Window position", L"Bottom", 920, s.WindowPos.iBottom);
	NewSetting(L"Window position", L"State", win_state_t::Normal, s.WindowPos.iState);

	// Display
	NewSetting(L"Display", L"Average BPM", false, s.Display.bAverageBPM);		// // // 050B
	NewSetting(L"Display", L"Channel state", false, s.Display.bChannelState);		// // // 050B todo
	NewSetting(L"Display", L"Register state", false, s.Display.bRegisterState);		// // // 050B

	// Other
	NewSetting(L"Other", L"Sample window state", 0, s.SampleWinState);
	NewSetting(L"Other", L"Frame editor position", 0, s.FrameEditPos);
	NewSetting(L"Other", L"Control panel position", 0, s.ControlPanelPos);		// // // 050B todo
	NewSetting(L"Other", L"Follow mode", true, s.bFollowMode);
	NewSetting(L"Other", L"Meter decay rate", false, s.bFastMeterDecayRate);		// // // 050B

	// // // Paths
	NewPathSetting(L"Paths", L"FTM path", PATH_FTM);
	NewPathSetting(L"Paths", L"FTI path", PATH_FTI);
	NewPathSetting(L"Paths", L"NSF path", PATH_NSF);
	NewPathSetting(L"Paths", L"DMC path", PATH_DMC);
	NewPathSetting(L"Paths", L"WAV path", PATH_WAV);
	NewPathSetting(L"Paths", L"Instrument menu", PATH_INST);		// // //

	// Mixing
	NewSetting(L"Mixer", L"APU1", 0, s.ChipLevels.iLevelAPU1);
	NewSetting(L"Mixer", L"APU2", 0, s.ChipLevels.iLevelAPU2);
	NewSetting(L"Mixer", L"VRC6", 0, s.ChipLevels.iLevelVRC6);
	NewSetting(L"Mixer", L"VRC7", 0, s.ChipLevels.iLevelVRC7);
	NewSetting(L"Mixer", L"MMC5", 0, s.ChipLevels.iLevelMMC5);
	NewSetting(L"Mixer", L"FDS", 0, s.ChipLevels.iLevelFDS);
	NewSetting(L"Mixer", L"N163", 0, s.ChipLevels.iLevelN163);
	NewSetting(L"Mixer", L"S5B", 0, s.ChipLevels.iLevelS5B);
}
