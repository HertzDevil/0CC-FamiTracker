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
#include "FamiTracker.h"
#include "ColorScheme.h"

// // // registry context

stOldSettingContext::stOldSettingContext()
{
	free((void*)theApp.m_pszProfileName);
	theApp.m_pszProfileName = L"FamiTracker";
}

stOldSettingContext::~stOldSettingContext()
{
	CStringW s;
	s.LoadStringW(AFX_IDS_APP_TITLE);
	theApp.m_pszProfileName = _wcsdup(s);
}

// CSettings

CSettings &CSettings::GetInstance()		// // //
{
	static CSettings Object;
	return Object;
}

CSettings::CSettings() {
	SetupSettings();
	TRACE(L"Settings: Added %u settings\n", m_pSettings.size());
}

void CSettings::SetupSettings()
{
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

	stOldSettingContext s;		// // //
	/*
		location priorities:
		1. HKCU/SOFTWARE/0CC-FamiTracker
		2. HKCU/SOFTWARE/0CC-FamiTracker, original key (using UpdateDefault)
		3. HKCU/SOFTWARE/FamiTracker
		4. HKCU/SOFTWARE/FamiTracker, original key (using UpdateDefault)
		5. Default value
	*/

	// General
	SETTING_INT(L"General", L"Edit style", EDIT_STYLE_FT2, &General.iEditStyle);
	SETTING_INT(L"General", L"Page step size", 4, &General.iPageStepSize);
	SETTING_BOOL(L"General", L"Wrap cursor", true, &General.bWrapCursor);
	SETTING_BOOL(L"General", L"Wrap across frames", true, &General.bWrapFrames);
	SETTING_BOOL(L"General", L"Free cursor edit",	false, &General.bFreeCursorEdit);
	SETTING_BOOL(L"General", L"Wave preview", true, &General.bWavePreview);
	SETTING_BOOL(L"General", L"Key repeat", true, &General.bKeyRepeat);
	SETTING_BOOL(L"General", L"Hex row display", true, &General.bRowInHex);
	SETTING_BOOL(L"General", L"Frame preview", true, &General.bFramePreview);
	SETTING_BOOL(L"General", L"No DPCM reset", false, &General.bNoDPCMReset);
	SETTING_BOOL(L"General", L"No Step moving", false, &General.bNoStepMove);
	SETTING_BOOL(L"General", L"Delete pull up", false, &General.bPullUpDelete);
	SETTING_BOOL(L"General", L"Backups", false, &General.bBackups);
	SETTING_BOOL(L"General", L"Single instance", false, &General.bSingleInstance);
	SETTING_BOOL(L"General", L"Preview full row", false, &General.bPreviewFullRow);
	SETTING_BOOL(L"General", L"Double click selection", false, &General.bDblClickSelect);
	// // //
	SETTING_BOOL(L"General", L"Wrap pattern values", false, &General.bWrapPatternValue);
	SETTING_BOOL(L"General", L"Cut sub-volume", false, &General.bCutVolume);
	SETTING_BOOL(L"General", L"Use old FDS volume table", false, &General.bFDSOldVolume);
	SETTING_BOOL(L"General", L"Retrieve channel state", false, &General.bRetrieveChanState);
	SETTING_BOOL(L"General", L"Overflow paste mode", false, &General.bOverflowPaste);
	SETTING_BOOL(L"General", L"Show skipped rows", false, &General.bShowSkippedRows);
	SETTING_BOOL(L"General", L"Hexadecimal keypad", false, &General.bHexKeypad);
	SETTING_BOOL(L"General", L"Multi-frame selection", false, &General.bMultiFrameSel);
	SETTING_BOOL(L"General", L"Check for new versions", true, &General.bCheckVersion);

	// // // Version / Compatibility info
	SETTING_INT(L"Version", L"Module error level", MODULE_ERROR_DEFAULT, &Version.iErrorLevel);

	// Keys
	SETTING_INT(L"Keys", L"Note cut", 0x31, &Keys.iKeyNoteCut);
	SETTING_INT(L"Keys", L"Note release", 0xDC, &Keys.iKeyNoteRelease);
	SETTING_INT(L"Keys", L"Clear field", 0xBD, &Keys.iKeyClear);
	SETTING_INT(L"Keys", L"Repeat", 0x00, &Keys.iKeyRepeat);
	SETTING_INT(L"Keys", L"Echo buffer", 0x00, &Keys.iKeyEchoBuffer);		// // //

	// Sound
	SETTING_INT(L"Sound", L"Audio Device", 0, &Sound.iDevice);
	SETTING_INT(L"Sound", L"Sample rate", 44100, &Sound.iSampleRate);
	SETTING_INT(L"Sound", L"Sample size", 16, &Sound.iSampleSize);
	SETTING_INT(L"Sound", L"Buffer length", 40, &Sound.iBufferLength);
	SETTING_INT(L"Sound", L"Bass filter freq", 30, &Sound.iBassFilter);
	SETTING_INT(L"Sound", L"Treble filter freq", 12000, &Sound.iTrebleFilter);
	SETTING_INT(L"Sound", L"Treble filter damping", 24, &Sound.iTrebleDamping);
	SETTING_INT(L"Sound", L"Volume", 100, &Sound.iMixVolume);

	// Midi
	SETTING_INT(L"MIDI", L"Device", 0, &Midi.iMidiDevice);
	SETTING_INT(L"MIDI", L"Out Device", 0, &Midi.iMidiOutDevice);
	SETTING_BOOL(L"MIDI", L"Master sync", false, &Midi.bMidiMasterSync);
	SETTING_BOOL(L"MIDI", L"Key release", false, &Midi.bMidiKeyRelease);
	SETTING_BOOL(L"MIDI", L"Channel map", false, &Midi.bMidiChannelMap);
	SETTING_BOOL(L"MIDI", L"Velocity control", false,	&Midi.bMidiVelocity);
	SETTING_BOOL(L"MIDI", L"Auto Arpeggio", false, &Midi.bMidiArpeggio);

	// Appearance
	SETTING_INT(L"Appearance", L"Background", DEFAULT_COLOR_SCHEME.BACKGROUND, &Appearance.iColBackground);
	SETTING_INT(L"Appearance", L"Background highlighted", DEFAULT_COLOR_SCHEME.BACKGROUND_HILITE, &Appearance.iColBackgroundHilite);
	SETTING_INT(L"Appearance", L"Background highlighted 2", DEFAULT_COLOR_SCHEME.BACKGROUND_HILITE2, &Appearance.iColBackgroundHilite2);
	SETTING_INT(L"Appearance", L"Pattern text", DEFAULT_COLOR_SCHEME.TEXT_NORMAL, &Appearance.iColPatternText);
	SETTING_INT(L"Appearance", L"Pattern text highlighted", DEFAULT_COLOR_SCHEME.TEXT_HILITE, &Appearance.iColPatternTextHilite);
	SETTING_INT(L"Appearance", L"Pattern text highlighted 2", DEFAULT_COLOR_SCHEME.TEXT_HILITE2, &Appearance.iColPatternTextHilite2);
	SETTING_INT(L"Appearance", L"Pattern instrument", DEFAULT_COLOR_SCHEME.TEXT_INSTRUMENT, &Appearance.iColPatternInstrument);
	SETTING_INT(L"Appearance", L"Pattern volume", DEFAULT_COLOR_SCHEME.TEXT_VOLUME, &Appearance.iColPatternVolume);
	SETTING_INT(L"Appearance", L"Pattern effect", DEFAULT_COLOR_SCHEME.TEXT_EFFECT, &Appearance.iColPatternEffect);
	SETTING_INT(L"Appearance", L"Selection", DEFAULT_COLOR_SCHEME.SELECTION, &Appearance.iColSelection);
	SETTING_INT(L"Appearance", L"Cursor", DEFAULT_COLOR_SCHEME.CURSOR, &Appearance.iColCursor);
	// // //
	SETTING_INT(L"Appearance", L"Current row (normal mode)", DEFAULT_COLOR_SCHEME.ROW_NORMAL, &Appearance.iColCurrentRowNormal);
	SETTING_INT(L"Appearance", L"Current row (edit mode)", DEFAULT_COLOR_SCHEME.ROW_EDIT, &Appearance.iColCurrentRowEdit);
	SETTING_INT(L"Appearance", L"Current row (playing)", DEFAULT_COLOR_SCHEME.ROW_PLAYING, &Appearance.iColCurrentRowPlaying);
	SETTING_STRING(L"Appearance", L"Pattern font", FONT_FACE, &Appearance.strFont)
		->UpdateDefault(L"General", L"Pattern font");
	SETTING_INT(L"Appearance", L"Pattern font size", FONT_SIZE, &Appearance.iFontSize)
		->UpdateDefault(L"General", L"Pattern font size");
	SETTING_STRING(L"Appearance", L"Frame font", FONT_FACE, &Appearance.strFrameFont)
		->UpdateDefault(L"General", L"Frame font");		// // // 050B
	SETTING_BOOL(L"Appearance", L"Pattern colors", true, &Appearance.bPatternColor)
		->UpdateDefault(L"Appearance", L"Pattern colors");
	SETTING_BOOL(L"Appearance", L"Display flats", false, &Appearance.bDisplayFlats)
		->UpdateDefault(L"Appearance", L"Display flats");

	// Window position
	SETTING_INT(L"Window position", L"Left", 100, &WindowPos.iLeft);
	SETTING_INT(L"Window position", L"Top", 100, &WindowPos.iTop);
	SETTING_INT(L"Window position", L"Right", 950, &WindowPos.iRight);
	SETTING_INT(L"Window position", L"Bottom", 920, &WindowPos.iBottom);
	SETTING_INT(L"Window position", L"State", STATE_NORMAL, &WindowPos.iState);

	// Display
	SETTING_BOOL(L"Display", L"Average BPM", false, &Display.bAverageBPM);		// // // 050B
	SETTING_BOOL(L"Display", L"Channel state", false, &Display.bChannelState);		// // // 050B todo
	SETTING_BOOL(L"Display", L"Register state", false, &Display.bRegisterState);		// // // 050B

	// Other
	SETTING_INT(L"Other", L"Sample window state", 0, &SampleWinState);
	SETTING_INT(L"Other", L"Frame editor position", 0, &FrameEditPos);
	SETTING_INT(L"Other", L"Control panel position", 0, &ControlPanelPos);		// // // 050B todo
	SETTING_BOOL(L"Other", L"Follow mode", true, &FollowMode);
	SETTING_BOOL(L"Other", L"Meter decay rate", false, &MeterDecayRate);		// // // 050B

	// Paths
	SETTING_STRING(L"Paths", L"FTM path", L"", &Paths[PATH_FTM]);
	SETTING_STRING(L"Paths", L"FTI path", L"", &Paths[PATH_FTI]);
	SETTING_STRING(L"Paths", L"NSF path", L"", &Paths[PATH_NSF]);
	SETTING_STRING(L"Paths", L"DMC path", L"", &Paths[PATH_DMC]);
	SETTING_STRING(L"Paths", L"WAV path", L"", &Paths[PATH_WAV]);

	SETTING_STRING(L"Paths", L"Instrument menu", L"", &InstrumentMenuPath);

	// Mixing
	SETTING_INT(L"Mixer", L"APU1", 0, &ChipLevels.iLevelAPU1);
	SETTING_INT(L"Mixer", L"APU2", 0, &ChipLevels.iLevelAPU2);
	SETTING_INT(L"Mixer", L"VRC6", 0, &ChipLevels.iLevelVRC6);
	SETTING_INT(L"Mixer", L"VRC7", 0, &ChipLevels.iLevelVRC7);
	SETTING_INT(L"Mixer", L"MMC5", 0, &ChipLevels.iLevelMMC5);
	SETTING_INT(L"Mixer", L"FDS", 0, &ChipLevels.iLevelFDS);
	SETTING_INT(L"Mixer", L"N163", 0, &ChipLevels.iLevelN163);
	SETTING_INT(L"Mixer", L"S5B", 0, &ChipLevels.iLevelS5B);
}

template<class T>
CSettingBase *CSettings::AddSetting(LPCWSTR pSection, LPCWSTR pEntry, T tDefault, T *pVariable)
{
	return m_pSettings.emplace_back(std::make_unique<CSettingType<T>>(pSection, pEntry, tDefault, pVariable)).get();		// // //
}

// CSettings member functions

void CSettings::LoadSettings()
{
	for (auto &x : m_pSettings)		// // //
		x->Load();
}

void CSettings::SaveSettings()
{
	for (auto &x : m_pSettings)		// // //
		x->Save();
}

void CSettings::DefaultSettings()
{
	for (auto &x : m_pSettings)		// // //
		x->Default();
}

void CSettings::DeleteSettings()
{
	// Delete all settings from registry
	HKEY hKey = theApp.GetAppRegistryKey();
	theApp.DelRegTree(hKey, L"");
}

void CSettings::SetWindowPos(int Left, int Top, int Right, int Bottom, int State)
{
	WindowPos.iLeft = Left;
	WindowPos.iTop = Top;
	WindowPos.iRight = Right;
	WindowPos.iBottom = Bottom;
	WindowPos.iState = State;
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

// Settings types

template<class T>
void CSettingType<T>::Load()
{
	T Value = m_tDefaultValue;		// // //
	{
		stOldSettingContext s;
		if (m_pSectionSecond)
			Value = theApp.GetProfileIntW(m_pSectionSecond, m_pEntrySecond, Value);
		Value = theApp.GetProfileIntW(m_pSection, m_pEntry, Value);
	}
	if (m_pSectionSecond)
		Value = theApp.GetProfileIntW(m_pSectionSecond, m_pEntrySecond, Value);
	*m_pVariable = theApp.GetProfileIntW(m_pSection, m_pEntry, Value);
}

template<>
void CSettingType<bool>::Load()
{
	bool Value = m_tDefaultValue;		// // //
	{
		stOldSettingContext s;
		if (m_pSectionSecond)
			Value = theApp.GetProfileIntW(m_pSectionSecond, m_pEntrySecond, Value) != 0;
		Value = theApp.GetProfileIntW(m_pSection, m_pEntry, Value) != 0;
	}
	if (m_pSectionSecond)
		Value = theApp.GetProfileIntW(m_pSectionSecond, m_pEntrySecond, Value) != 0;
	*m_pVariable = theApp.GetProfileIntW(m_pSection, m_pEntry, Value) != 0;
}

template<>
void CSettingType<CStringW>::Load()
{
	CStringW Value = m_tDefaultValue;		// // //
	{
		stOldSettingContext s;
		if (m_pSectionSecond)
			Value = theApp.GetProfileStringW(m_pSectionSecond, m_pEntrySecond, Value);
		Value = theApp.GetProfileStringW(m_pSection, m_pEntry, Value);
	}
	if (m_pSectionSecond)
		Value = theApp.GetProfileStringW(m_pSectionSecond, m_pEntrySecond, Value);
	*m_pVariable = theApp.GetProfileStringW(m_pSection, m_pEntry, Value);
}

template<class T>
void CSettingType<T>::Save()
{
	theApp.WriteProfileInt(m_pSection, m_pEntry, *m_pVariable);
}

template<>
void CSettingType<CStringW>::Save()
{
	theApp.WriteProfileStringW(m_pSection, m_pEntry, *m_pVariable);
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
