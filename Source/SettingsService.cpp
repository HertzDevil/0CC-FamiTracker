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

// // // registry context

stOldSettingContext::stOldSettingContext()
{
	std::free((void*)Env.GetMainApp()->m_pszProfileName);
	Env.GetMainApp()->m_pszProfileName = L"FamiTracker";
}

stOldSettingContext::~stOldSettingContext()
{
	CStringW s;
	s.LoadStringW(AFX_IDS_APP_TITLE);
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
		x->Load();
}

void CSettingsService::SaveSettings() {
	for (auto &x : m_pSettings)
		x->Save();
}

void CSettingsService::DefaultSettings() {
	for (auto &x : m_pSettings)
		x->Default();
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
		return AddSetting<int>(Section, Entry, Default, Variable);
	};
	const auto SETTING_BOOL = [&] (LPCWSTR Section, LPCWSTR Entry, bool Default, bool *Variable) {
		return AddSetting<bool>(Section, Entry, Default, Variable);
	};
	const auto SETTING_STRING = [&] (LPCWSTR Section, LPCWSTR Entry, CStringW Default, CStringW *Variable) {
		return AddSetting<CStringW>(Section, Entry, Default, Variable);
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
	SETTING_INT(L"Keys", L"Note cut", 0x31, &s.Keys.iKeyNoteCut);
	SETTING_INT(L"Keys", L"Note release", 0xDC, &s.Keys.iKeyNoteRelease);
	SETTING_INT(L"Keys", L"Clear field", 0xBD, &s.Keys.iKeyClear);
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

	// Paths
	SETTING_STRING(L"Paths", L"FTM path", L"", &s.GetPath(PATH_FTM));
	SETTING_STRING(L"Paths", L"FTI path", L"", &s.GetPath(PATH_FTI));
	SETTING_STRING(L"Paths", L"NSF path", L"", &s.GetPath(PATH_NSF));
	SETTING_STRING(L"Paths", L"DMC path", L"", &s.GetPath(PATH_DMC));
	SETTING_STRING(L"Paths", L"WAV path", L"", &s.GetPath(PATH_WAV));

	SETTING_STRING(L"Paths", L"Instrument menu", L"", &s.InstrumentMenuPath);

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

template<class T>
CSettingBase *CSettingsService::AddSetting(LPCWSTR pSection, LPCWSTR pEntry, T tDefault, T *pVariable) {
	return m_pSettings.emplace_back(std::make_unique<CSettingType<T>>(pSection, pEntry, tDefault, pVariable)).get();		// // //
}

// Settings types

template<class T>
void CSettingType<T>::Load()
{
	T Value = m_tDefaultValue;		// // //
	{
		stOldSettingContext s;
		if (m_pSectionSecond)
			Value = Env.GetMainApp()->GetProfileIntW(m_pSectionSecond, m_pEntrySecond, Value);
		Value = Env.GetMainApp()->GetProfileIntW(m_pSection, m_pEntry, Value);
	}
	if (m_pSectionSecond)
		Value = Env.GetMainApp()->GetProfileIntW(m_pSectionSecond, m_pEntrySecond, Value);
	*m_pVariable = Env.GetMainApp()->GetProfileIntW(m_pSection, m_pEntry, Value);
}

template<>
void CSettingType<bool>::Load()
{
	bool Value = m_tDefaultValue;		// // //
	{
		stOldSettingContext s;
		if (m_pSectionSecond)
			Value = Env.GetMainApp()->GetProfileIntW(m_pSectionSecond, m_pEntrySecond, Value) != 0;
		Value = Env.GetMainApp()->GetProfileIntW(m_pSection, m_pEntry, Value) != 0;
	}
	if (m_pSectionSecond)
		Value = Env.GetMainApp()->GetProfileIntW(m_pSectionSecond, m_pEntrySecond, Value) != 0;
	*m_pVariable = Env.GetMainApp()->GetProfileIntW(m_pSection, m_pEntry, Value) != 0;
}

template<>
void CSettingType<CStringW>::Load()
{
	CStringW Value = m_tDefaultValue;		// // //
	{
		stOldSettingContext s;
		if (m_pSectionSecond)
			Value = Env.GetMainApp()->GetProfileStringW(m_pSectionSecond, m_pEntrySecond, Value);
		Value = Env.GetMainApp()->GetProfileStringW(m_pSection, m_pEntry, Value);
	}
	if (m_pSectionSecond)
		Value = Env.GetMainApp()->GetProfileStringW(m_pSectionSecond, m_pEntrySecond, Value);
	*m_pVariable = Env.GetMainApp()->GetProfileStringW(m_pSection, m_pEntry, Value);
}

template<class T>
void CSettingType<T>::Save()
{
	Env.GetMainApp()->WriteProfileInt(m_pSection, m_pEntry, *m_pVariable);
}

template<>
void CSettingType<CStringW>::Save()
{
	Env.GetMainApp()->WriteProfileStringW(m_pSection, m_pEntry, *m_pVariable);
}

template<class T>
void CSettingType<T>::Default()
{
	*m_pVariable = m_tDefaultValue;
}

void CSettingBase::UpdateDefault(LPCWSTR pSection, LPCWSTR pEntry)		// // //
{
	m_pSectionSecond = pSection;
	m_pEntrySecond = pEntry;
}
