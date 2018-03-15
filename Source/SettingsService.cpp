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
T GetRegValue(LPCWSTR section, LPCWSTR key, const T &defaultVal) = delete;

template <>
int GetRegValue<int>(LPCWSTR section, LPCWSTR entry, const int &default_value) {
	return Env.GetMainApp()->GetProfileIntW(section, entry, default_value);
}

template <>
bool GetRegValue<bool>(LPCWSTR section, LPCWSTR entry, const bool &default_value) {
	return Env.GetMainApp()->GetProfileIntW(section, entry, default_value) != 0;
}

template <>
std::wstring GetRegValue<std::wstring>(LPCWSTR section, LPCWSTR entry, const std::wstring &default_value) {
	CStringW v = Env.GetMainApp()->GetProfileStringW(section, entry, default_value.data());
	return std::wstring((LPCWSTR)v, v.GetLength());
}

template <>
fs::path GetRegValue<fs::path>(LPCWSTR section, LPCWSTR entry, const fs::path &default_value) {
	CStringW v = Env.GetMainApp()->GetProfileStringW(section, entry, default_value.c_str());
	return fs::path {(LPCWSTR)v, (LPCWSTR)v + v.GetLength()};
}

template <typename T>
void SetRegValue(LPCWSTR section, LPCWSTR key, const T &val) = delete;

template <>
void SetRegValue<int>(LPCWSTR section, LPCWSTR entry, const int &val) {
	Env.GetMainApp()->WriteProfileInt(section, entry, val);
}

template <>
void SetRegValue<bool>(LPCWSTR section, LPCWSTR entry, const bool &val) {
	Env.GetMainApp()->WriteProfileInt(section, entry, val ? 1 : 0);
}

template <>
void SetRegValue<std::wstring>(LPCWSTR section, LPCWSTR entry, const std::wstring &val) {
	Env.GetMainApp()->WriteProfileStringW(section, entry, CStringW(val.data(), val.size()));
}

template <>
void SetRegValue<fs::path>(LPCWSTR section, LPCWSTR entry, const fs::path &val) {
	SetRegValue(section, entry, val.wstring());
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

void CSettingsService::SetupSettings() {
	//
	// This function defines all settings in the program that are stored in registry
	// All settings are loaded on program start and saved when closing the program
	//

	// The SETTING macros takes four arguments:
	//
	//  1. Registry section
	//  2. Registry key name
	//  3. Default value
	//  4. A variable that contains the setting, loaded on program startup and saved on shutdown
	//

	const auto SETTING_INT = [&] (LPCWSTR Section, LPCWSTR Entry, int Default, int *Variable) {		// // //
		return m_pSettings.emplace_back(std::make_unique<CSettingPublic<int>>(Section, Entry, Default, Variable)).get();
	};
	const auto SETTING_BOOL = [&] (LPCWSTR Section, LPCWSTR Entry, bool Default, bool *Variable) {
		return m_pSettings.emplace_back(std::make_unique<CSettingPublic<bool>>(Section, Entry, Default, Variable)).get();
	};
	const auto SETTING_STRING = [&] (LPCWSTR Section, LPCWSTR Entry, std::wstring Default, std::wstring *Variable) {
		return m_pSettings.emplace_back(std::make_unique<CSettingPublic<std::wstring>>(Section, Entry, std::move(Default), Variable)).get();
	};
	const auto SETTING_PATH = [&] (LPCWSTR Section, LPCWSTR Entry, PATHS PathType) {
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
	SETTING_INT(L"General", L"Edit style", EDIT_STYLE_FT2, &s.General.iEditStyle);
	SETTING_INT(L"General", L"Page step size", 4, &s.General.iPageStepSize);
	SETTING_BOOL(L"General", L"Wrap cursor", true, &s.General.bWrapCursor);
	SETTING_BOOL(L"General", L"Wrap across frames", true, &s.General.bWrapFrames);
	SETTING_BOOL(L"General", L"Free cursor edit",	false, &s.General.bFreeCursorEdit);
	SETTING_BOOL(L"General", L"Wave preview", true, &s.General.bWavePreview);
	SETTING_BOOL(L"General", L"Key repeat", true, &s.General.bKeyRepeat);
	SETTING_BOOL(L"General", L"Hex row display", true, &s.General.bRowInHex);
	SETTING_BOOL(L"General", L"Frame preview", true, &s.General.bFramePreview);
	SETTING_BOOL(L"General", L"No DPCM reset", false, &s.General.bNoDPCMReset);
	SETTING_BOOL(L"General", L"No Step moving", false, &s.General.bNoStepMove);
	SETTING_BOOL(L"General", L"Delete pull up", false, &s.General.bPullUpDelete);
	SETTING_BOOL(L"General", L"Backups", false, &s.General.bBackups);
	SETTING_BOOL(L"General", L"Single instance", false, &s.General.bSingleInstance);
	SETTING_BOOL(L"General", L"Preview full row", false, &s.General.bPreviewFullRow);
	SETTING_BOOL(L"General", L"Double click selection", false, &s.General.bDblClickSelect);
	// // //
	SETTING_BOOL(L"General", L"Wrap pattern values", false, &s.General.bWrapPatternValue);
	SETTING_BOOL(L"General", L"Cut sub-volume", false, &s.General.bCutVolume);
	SETTING_BOOL(L"General", L"Use old FDS volume table", false, &s.General.bFDSOldVolume);
	SETTING_BOOL(L"General", L"Retrieve channel state", false, &s.General.bRetrieveChanState);
	SETTING_BOOL(L"General", L"Overflow paste mode", false, &s.General.bOverflowPaste);
	SETTING_BOOL(L"General", L"Show skipped rows", false, &s.General.bShowSkippedRows);
	SETTING_BOOL(L"General", L"Hexadecimal keypad", false, &s.General.bHexKeypad);
	SETTING_BOOL(L"General", L"Multi-frame selection", false, &s.General.bMultiFrameSel);
	SETTING_BOOL(L"General", L"Check for new versions", true, &s.General.bCheckVersion);

	// // // Version / Compatibility info
	SETTING_INT(L"Version", L"Module error level", MODULE_ERROR_DEFAULT, &s.Version.iErrorLevel);

	// Keys
	SETTING_INT(L"Keys", L"Note cut", '1', &s.Keys.iKeyNoteCut);
	SETTING_INT(L"Keys", L"Note release", VK_OEM_5, &s.Keys.iKeyNoteRelease); // \|
	SETTING_INT(L"Keys", L"Clear field", VK_OEM_MINUS, &s.Keys.iKeyClear);
	SETTING_INT(L"Keys", L"Repeat", 0x00, &s.Keys.iKeyRepeat);
	SETTING_INT(L"Keys", L"Echo buffer", 0x00, &s.Keys.iKeyEchoBuffer);		// // //

	// Sound
	SETTING_INT(L"Sound", L"Audio Device", 0, &s.Sound.iDevice);
	SETTING_INT(L"Sound", L"Sample rate", 44100, &s.Sound.iSampleRate);
	SETTING_INT(L"Sound", L"Sample size", 16, &s.Sound.iSampleSize);
	SETTING_INT(L"Sound", L"Buffer length", 40, &s.Sound.iBufferLength);
	SETTING_INT(L"Sound", L"Bass filter freq", 30, &s.Sound.iBassFilter);
	SETTING_INT(L"Sound", L"Treble filter freq", 12000, &s.Sound.iTrebleFilter);
	SETTING_INT(L"Sound", L"Treble filter damping", 24, &s.Sound.iTrebleDamping);
	SETTING_INT(L"Sound", L"Volume", 100, &s.Sound.iMixVolume);

	// Midi
	SETTING_INT(L"MIDI", L"Device", 0, &s.Midi.iMidiDevice);
	SETTING_INT(L"MIDI", L"Out Device", 0, &s.Midi.iMidiOutDevice);
	SETTING_BOOL(L"MIDI", L"Master sync", false, &s.Midi.bMidiMasterSync);
	SETTING_BOOL(L"MIDI", L"Key release", false, &s.Midi.bMidiKeyRelease);
	SETTING_BOOL(L"MIDI", L"Channel map", false, &s.Midi.bMidiChannelMap);
	SETTING_BOOL(L"MIDI", L"Velocity control", false, &s.Midi.bMidiVelocity);
	SETTING_BOOL(L"MIDI", L"Auto Arpeggio", false, &s.Midi.bMidiArpeggio);

	// Appearance
	SETTING_INT(L"Appearance", L"Background", DEFAULT_COLOR_SCHEME.BACKGROUND, &s.Appearance.iColBackground);
	SETTING_INT(L"Appearance", L"Background highlighted", DEFAULT_COLOR_SCHEME.BACKGROUND_HILITE, &s.Appearance.iColBackgroundHilite);
	SETTING_INT(L"Appearance", L"Background highlighted 2", DEFAULT_COLOR_SCHEME.BACKGROUND_HILITE2, &s.Appearance.iColBackgroundHilite2);
	SETTING_INT(L"Appearance", L"Pattern text", DEFAULT_COLOR_SCHEME.TEXT_NORMAL, &s.Appearance.iColPatternText);
	SETTING_INT(L"Appearance", L"Pattern text highlighted", DEFAULT_COLOR_SCHEME.TEXT_HILITE, &s.Appearance.iColPatternTextHilite);
	SETTING_INT(L"Appearance", L"Pattern text highlighted 2", DEFAULT_COLOR_SCHEME.TEXT_HILITE2, &s.Appearance.iColPatternTextHilite2);
	SETTING_INT(L"Appearance", L"Pattern instrument", DEFAULT_COLOR_SCHEME.TEXT_INSTRUMENT, &s.Appearance.iColPatternInstrument);
	SETTING_INT(L"Appearance", L"Pattern volume", DEFAULT_COLOR_SCHEME.TEXT_VOLUME, &s.Appearance.iColPatternVolume);
	SETTING_INT(L"Appearance", L"Pattern effect", DEFAULT_COLOR_SCHEME.TEXT_EFFECT, &s.Appearance.iColPatternEffect);
	SETTING_INT(L"Appearance", L"Selection", DEFAULT_COLOR_SCHEME.SELECTION, &s.Appearance.iColSelection);
	SETTING_INT(L"Appearance", L"Cursor", DEFAULT_COLOR_SCHEME.CURSOR, &s.Appearance.iColCursor);
	// // //
	SETTING_INT(L"Appearance", L"Current row (normal mode)", DEFAULT_COLOR_SCHEME.ROW_NORMAL, &s.Appearance.iColCurrentRowNormal);
	SETTING_INT(L"Appearance", L"Current row (edit mode)", DEFAULT_COLOR_SCHEME.ROW_EDIT, &s.Appearance.iColCurrentRowEdit);
	SETTING_INT(L"Appearance", L"Current row (playing)", DEFAULT_COLOR_SCHEME.ROW_PLAYING, &s.Appearance.iColCurrentRowPlaying);
	SETTING_STRING(L"Appearance", L"Pattern font", FONT_FACE, &s.Appearance.strFont)
		->UpdateDefault(L"General", L"Pattern font");
	SETTING_INT(L"Appearance", L"Pattern font size", FONT_SIZE, &s.Appearance.iFontSize)
		->UpdateDefault(L"General", L"Pattern font size");
	SETTING_STRING(L"Appearance", L"Frame font", FONT_FACE, &s.Appearance.strFrameFont)
		->UpdateDefault(L"General", L"Frame font");		// // // 050B
	SETTING_BOOL(L"Appearance", L"Pattern colors", true, &s.Appearance.bPatternColor)
		->UpdateDefault(L"Appearance", L"Pattern colors");
	SETTING_BOOL(L"Appearance", L"Display flats", false, &s.Appearance.bDisplayFlats)
		->UpdateDefault(L"Appearance", L"Display flats");

	// Window position
	SETTING_INT(L"Window position", L"Left", 100, &s.WindowPos.iLeft);
	SETTING_INT(L"Window position", L"Top", 100, &s.WindowPos.iTop);
	SETTING_INT(L"Window position", L"Right", 950, &s.WindowPos.iRight);
	SETTING_INT(L"Window position", L"Bottom", 920, &s.WindowPos.iBottom);
	SETTING_INT(L"Window position", L"State", STATE_NORMAL, &s.WindowPos.iState);

	// Display
	SETTING_BOOL(L"Display", L"Average BPM", false, &s.Display.bAverageBPM);		// // // 050B
	SETTING_BOOL(L"Display", L"Channel state", false, &s.Display.bChannelState);		// // // 050B todo
	SETTING_BOOL(L"Display", L"Register state", false, &s.Display.bRegisterState);		// // // 050B

	// Other
	SETTING_INT(L"Other", L"Sample window state", 0, &s.SampleWinState);
	SETTING_INT(L"Other", L"Frame editor position", 0, &s.FrameEditPos);
	SETTING_INT(L"Other", L"Control panel position", 0, &s.ControlPanelPos);		// // // 050B todo
	SETTING_BOOL(L"Other", L"Follow mode", true, &s.FollowMode);
	SETTING_BOOL(L"Other", L"Meter decay rate", false, &s.MeterDecayRate);		// // // 050B

	// // // Paths
	SETTING_PATH(L"Paths", L"FTM path", PATH_FTM);
	SETTING_PATH(L"Paths", L"FTI path", PATH_FTI);
	SETTING_PATH(L"Paths", L"NSF path", PATH_NSF);
	SETTING_PATH(L"Paths", L"DMC path", PATH_DMC);
	SETTING_PATH(L"Paths", L"WAV path", PATH_WAV);
	SETTING_PATH(L"Paths", L"Instrument menu", PATH_INST);		// // //

	// Mixing
	SETTING_INT(L"Mixer", L"APU1", 0, &s.ChipLevels.iLevelAPU1);
	SETTING_INT(L"Mixer", L"APU2", 0, &s.ChipLevels.iLevelAPU2);
	SETTING_INT(L"Mixer", L"VRC6", 0, &s.ChipLevels.iLevelVRC6);
	SETTING_INT(L"Mixer", L"VRC7", 0, &s.ChipLevels.iLevelVRC7);
	SETTING_INT(L"Mixer", L"MMC5", 0, &s.ChipLevels.iLevelMMC5);
	SETTING_INT(L"Mixer", L"FDS", 0, &s.ChipLevels.iLevelFDS);
	SETTING_INT(L"Mixer", L"N163", 0, &s.ChipLevels.iLevelN163);
	SETTING_INT(L"Mixer", L"S5B", 0, &s.ChipLevels.iLevelS5B);
}
