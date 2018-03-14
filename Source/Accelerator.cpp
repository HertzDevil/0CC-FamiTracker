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

#include "Accelerator.h"
#include "FamiTrackerEnv.h"		// // //
#include "../resource.h"		// // //
#include "SettingsService.h"		// // // stOldSettingContext

/*
 * This class is used to translate key shortcuts -> command messages
 *
 * Translation is now done using win32 functions
 */

// List of modifier strings
const LPCWSTR CAccelerator::MOD_NAMES[] = {		// // //
	L"None",
	L"Alt",
	L"Ctrl",
	L"Ctrl+Alt",
	L"Shift",
	L"Shift+Alt",
	L"Shift+Ctrl",
	L"Shift+Ctrl+Alt",
};

// Default shortcut table
const std::vector<stAccelEntry> CAccelerator::DEFAULT_TABLE {
	{L"Decrease octave",				MOD_NONE,		VK_DIVIDE,		ID_CMD_OCTAVE_PREVIOUS},
	{L"Increase octave",				MOD_NONE,		VK_MULTIPLY,	ID_CMD_OCTAVE_NEXT},
	{L"Play / Stop",					MOD_NONE,		VK_RETURN,		ID_TRACKER_TOGGLE_PLAY},
	{L"Play",							MOD_NONE,		0,				ID_TRACKER_PLAY},
	{L"Play from start",				MOD_NONE,		VK_F5,			ID_TRACKER_PLAY_START},
	{L"Play from cursor",				MOD_NONE,		VK_F7,			ID_TRACKER_PLAY_CURSOR},
	{L"Play from row marker",			MOD_CONTROL,	VK_F7,			ID_TRACKER_PLAY_MARKER, L"Play from bookmark"},		// // // 050B
	{L"Play and loop pattern",			MOD_NONE,		VK_F6,			ID_TRACKER_PLAYPATTERN},
	{L"Play row",						MOD_CONTROL,	VK_RETURN,		ID_TRACKER_PLAYROW},
	{L"Stop",							MOD_NONE,		VK_F8,			ID_TRACKER_STOP},
	{L"Edit enable/disable",			MOD_NONE,		VK_SPACE,		ID_TRACKER_EDIT},
	{L"Set row marker",					MOD_CONTROL,	'B',			ID_TRACKER_SET_MARKER, L"Set row marker"},		// // // 050B
	{L"Paste and mix",					MOD_CONTROL,	'M',			ID_EDIT_PASTEMIX},
	{L"Paste and overwrite",			MOD_NONE,		0,				ID_EDIT_PASTEOVERWRITE},			// // //
	{L"Paste and insert",				MOD_NONE,		0,				ID_EDIT_PASTEINSERT},				// // //
	{L"Select all",						MOD_CONTROL,	'A',			ID_EDIT_SELECTALL},
	{L"Deselect",						MOD_NONE,		VK_ESCAPE,		ID_SELECT_NONE},					// // //
	{L"Select row",						MOD_NONE,		0,				ID_SELECT_ROW},						// // //
	{L"Select column",					MOD_NONE,		0,				ID_SELECT_COLUMN},					// // //
	{L"Select pattern",					MOD_NONE,		0,				ID_SELECT_PATTERN},					// // //
	{L"Select frame",					MOD_NONE,		0,				ID_SELECT_FRAME},					// // //
	{L"Select channel",					MOD_NONE,		0,				ID_SELECT_CHANNEL},					// // //
	{L"Select track",					MOD_NONE,		0,				ID_SELECT_TRACK},					// // //
	{L"Select in other editor",			MOD_NONE,		0,				ID_SELECT_OTHER},					// // //
	{L"Go to row",						MOD_ALT,		'G',			ID_EDIT_GOTO},						// // //
	{L"Toggle channel",					MOD_ALT,		VK_F9,			ID_TRACKER_TOGGLECHANNEL},
	{L"Solo channel",					MOD_ALT,		VK_F10,			ID_TRACKER_SOLOCHANNEL},
	{L"Toggle chip",					MOD_CONTROL|MOD_ALT, VK_F9,		ID_TRACKER_TOGGLECHIP},				// // //
	{L"Solo chip",						MOD_CONTROL|MOD_ALT, VK_F10,	ID_TRACKER_SOLOCHIP},				// // //
	{L"Record to instrument",			MOD_NONE,		0,				ID_TRACKER_RECORDTOINST},			// // //
	{L"Interpolate",					MOD_CONTROL,	'G',			ID_EDIT_INTERPOLATE},
	{L"Go to next frame",				MOD_CONTROL,	VK_RIGHT,		ID_NEXT_FRAME},
	{L"Go to previous frame",			MOD_CONTROL,	VK_LEFT,		ID_PREV_FRAME},
	{L"Toggle bookmark",				MOD_CONTROL,	'K',			ID_BOOKMARKS_TOGGLE},				// // //
	{L"Next bookmark",					MOD_CONTROL,	VK_NEXT,		ID_BOOKMARKS_NEXT},					// // //
	{L"Previous bookmark",				MOD_CONTROL,	VK_PRIOR,		ID_BOOKMARKS_PREVIOUS},				// // //
	{L"Transpose, decrease notes",		MOD_CONTROL,	VK_F1,			ID_TRANSPOSE_DECREASENOTE},
	{L"Transpose, increase notes",		MOD_CONTROL,	VK_F2,			ID_TRANSPOSE_INCREASENOTE},
	{L"Transpose, decrease octaves",	MOD_CONTROL,	VK_F3,			ID_TRANSPOSE_DECREASEOCTAVE},
	{L"Transpose, increase octaves",	MOD_CONTROL,	VK_F4,			ID_TRANSPOSE_INCREASEOCTAVE},
	{L"Increase pattern",				MOD_NONE,		VK_ADD,			IDC_FRAME_INC},
	{L"Decrease pattern",				MOD_NONE,		VK_SUBTRACT,	IDC_FRAME_DEC},
	{L"Next instrument",				MOD_CONTROL,	VK_DOWN,		ID_CMD_NEXT_INSTRUMENT},
	{L"Previous instrument",			MOD_CONTROL,	VK_UP,			ID_CMD_PREV_INSTRUMENT},
	{L"Type instrument number",			MOD_NONE,		0,				ID_CMD_INST_NUM},					// // //
	{L"Mask instruments",				MOD_ALT,		'T',			ID_EDIT_INSTRUMENTMASK},
	{L"Mask volume",					MOD_ALT,		'V',			ID_EDIT_VOLUMEMASK},				// // //
	{L"Edit instrument",				MOD_CONTROL,	'I',			ID_INSTRUMENT_EDIT},
	{L"Increase step size",				MOD_CONTROL,	VK_ADD,			ID_CMD_INCREASESTEPSIZE},
	{L"Decrease step size",				MOD_CONTROL,	VK_SUBTRACT,	ID_CMD_DECREASESTEPSIZE},
	{L"Follow mode",					MOD_NONE,		VK_SCROLL,		IDC_FOLLOW_TOGGLE},
	{L"Duplicate frame",				MOD_CONTROL,	'D',			ID_MODULE_DUPLICATEFRAME},
	{L"Insert frame",					MOD_NONE,		0,				ID_MODULE_INSERTFRAME},
	{L"Remove frame",					MOD_NONE,		0,				ID_MODULE_REMOVEFRAME},
	{L"Reverse",						MOD_CONTROL,	'R',			ID_EDIT_REVERSE},
	{L"Select frame editor",			MOD_NONE,		VK_F3,			ID_FOCUS_FRAME_EDITOR},
	{L"Select pattern editor",			MOD_NONE,		VK_F2,			ID_FOCUS_PATTERN_EDITOR},
	{L"Move one step up",				MOD_ALT,		VK_UP,			ID_CMD_STEP_UP},
	{L"Move one step down",				MOD_ALT,		VK_DOWN,		ID_CMD_STEP_DOWN},
	{L"Replace instrument",				MOD_ALT,		'S',			ID_EDIT_REPLACEINSTRUMENT},
	{L"Toggle control panel",			MOD_NONE,		0,				ID_VIEW_CONTROLPANEL},
	{L"Display effect list",			MOD_NONE,		0,				ID_HELP_EFFECTTABLE},
	{L"Select block start",				MOD_ALT,		'B',			ID_BLOCK_START},
	{L"Select block end",				MOD_ALT,		'E',			ID_BLOCK_END},
	{L"Pick up row settings",			MOD_NONE,		0,				ID_POPUP_PICKUPROW},
	{L"Next song",						MOD_NONE,		0,				ID_NEXT_SONG},
	{L"Previous song",					MOD_NONE,		0,				ID_PREV_SONG},
	{L"Expand patterns",				MOD_NONE,		0,				ID_EDIT_EXPANDPATTERNS},
	{L"Shrink patterns",				MOD_NONE,		0,				ID_EDIT_SHRINKPATTERNS},
	{L"Stretch patterns",				MOD_NONE,		0,				ID_EDIT_STRETCHPATTERNS},			// // //
	{L"Clone frame",					MOD_NONE,		0,				ID_MODULE_DUPLICATEFRAMEPATTERNS, L"Duplicate patterns"},		// // //
	{L"Clone pattern",					MOD_ALT,		'D',			ID_MODULE_DUPLICATECURRENTPATTERN, L"Duplicate current pattern"},	// // //
	{L"Decrease pattern values",		MOD_SHIFT,		VK_F1,			ID_DECREASEVALUES},
	{L"Increase pattern values",		MOD_SHIFT,		VK_F2,			ID_INCREASEVALUES},
	{L"Coarse decrease values",			MOD_SHIFT,		VK_F3,			ID_DECREASEVALUESCOARSE},			// // //
	{L"Coarse increase values",			MOD_SHIFT,		VK_F4,			ID_INCREASEVALUESCOARSE},			// // //
	{L"Toggle find / replace tab",		MOD_CONTROL,	'F',			ID_EDIT_FIND_TOGGLE},				// // //
	{L"Find next",						MOD_NONE,		0,				ID_FIND_NEXT},						// // //
	{L"Find previous",					MOD_NONE,		0,				ID_FIND_PREVIOUS},					// // //
	{L"Recall channel state",			MOD_NONE,		0,				ID_RECALL_CHANNEL_STATE},			// // //
	{L"Compact View",					MOD_NONE,		0,				IDC_COMPACT_TOGGLE},				// // //
};

const int CAccelerator::ACCEL_COUNT = DEFAULT_TABLE.size();

// Registry key
const LPCWSTR CAccelerator::SHORTCUTS_SECTION = L"Shortcuts";		// // //

namespace {

// Translate internal modifier -> windows modifier
BYTE GetMod(int Mod) {
	return ((Mod & MOD_CONTROL) ? FCONTROL : 0) | ((Mod & MOD_SHIFT) ? FSHIFT : 0) | ((Mod & MOD_ALT) ? FALT : 0);
}

} // namespace

// Class instance functions

CAccelerator::CAccelerator() :
	m_pEntriesTable(ACCEL_COUNT),		// // //
	m_pAccelTable(ACCEL_COUNT)		// // //
{
	TRACE(L"Accelerator: Accelerator table contains %d items\n", ACCEL_COUNT);
}

CAccelerator::~CAccelerator()
{
	ASSERT(m_hAccel == nullptr);
}

LPCWSTR CAccelerator::GetItemName(int Item) const
{
	return m_pEntriesTable[Item].name;
}

int CAccelerator::GetItemKey(int Item) const
{
	return m_pEntriesTable[Item].key;
}

int CAccelerator::GetItemMod(int Item) const
{
	return m_pEntriesTable[Item].mod;
}

int CAccelerator::GetDefaultKey(int Item) const
{
	return DEFAULT_TABLE[Item].key;
}

int CAccelerator::GetDefaultMod(int Item) const
{
	return DEFAULT_TABLE[Item].mod;
}

LPCWSTR CAccelerator::GetItemModName(int Item) const
{
	return MOD_NAMES[m_pEntriesTable[Item].mod];
}

LPCWSTR CAccelerator::GetItemKeyName(int Item) const
{
	if (m_pEntriesTable[Item].key > 0) {
		return GetVKeyName(m_pEntriesTable[Item].key);
	}

	return L"None";
}

CStringW CAccelerator::GetVKeyName(int virtualKey) const {		// // //
	unsigned int scanCode = MapVirtualKeyW(virtualKey, MAPVK_VK_TO_VSC);

	// because MapVirtualKey strips the extended bit for some keys
	switch (virtualKey) {
		case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN: // arrow keys
		case VK_PRIOR: case VK_NEXT: // page up and page down
		case VK_END: case VK_HOME:
		case VK_INSERT: case VK_DELETE:
		case VK_DIVIDE:	 // numpad slash
		case VK_NUMLOCK:
			scanCode |= 0x100; // set extended bit
			break;
	}

	WCHAR keyName[50] = { };

	if (GetKeyNameTextW(scanCode << 16, keyName, std::size(keyName)) != 0)
		return keyName;

	return L"";
}

void CAccelerator::StoreShortcut(int Item, int Key, int Mod)
{
	m_pEntriesTable[Item].key = Key;
	m_pEntriesTable[Item].mod = Mod;
}

CStringW CAccelerator::GetShortcutString(int id) const {		// // //
	for (const auto &x : m_pEntriesTable) {		// // //
		if (x.id == id) {
			CStringW KeyName = GetVKeyName(x.key);
			if (KeyName.GetLength() > 1)
				KeyName = KeyName.Mid(0, 1).MakeUpper() + KeyName.Mid(1, KeyName.GetLength() - 1).MakeLower();
			if (x.mod > 0)
				return FormattedW(L"\t%s+%s", MOD_NAMES[x.mod], (LPCWSTR)KeyName);
			else
				return FormattedW(L"\t%s", (LPCWSTR)KeyName);
		}
	}

	return L"";
}

bool CAccelerator::IsKeyUsed(int nChar) const		// // //
{
	return m_iUsedKeys.count(nChar & 0xFF) > 0;
}

// Registry storage/loading

void CAccelerator::SaveShortcuts(CSettings *pSettings) const
{
	// Save values
	auto *App = Env.GetMainApp();		// // //
	for (const auto &x : m_pEntriesTable)		// // //
		App->WriteProfileInt(SHORTCUTS_SECTION, x.name, (x.mod << 8) | x.key);
}

void CAccelerator::LoadShortcuts(CSettings *pSettings)
{
	auto *App = Env.GetMainApp();		// // //

	// Set up names and default values
	LoadDefaults();

	m_iUsedKeys.clear();		// // //

	// Load custom values, if exists
	/*
		location priorities:
		1. HKCU/SOFTWARE/0CC-FamiTracker
		1. HKCU/SOFTWARE/0CC-FamiTracker, original key
		2. HKCU/SOFTWARE/FamiTracker
		3. HKCU/SOFTWARE/FamiTracker, original key (using stAccelEntry::orig_name)
		4. default value
	*/
	for (auto &x : m_pEntriesTable) {		// // //
		int Setting = (x.mod << 8) | x.key;
		{		// // //
			stOldSettingContext s;
			if (x.orig_name != nullptr)		// // //
				Setting = App->GetProfileIntW(SHORTCUTS_SECTION, x.orig_name, Setting);
			Setting = App->GetProfileIntW(SHORTCUTS_SECTION, x.name, Setting);
		}
		if (x.orig_name != nullptr)		// // //
			Setting = App->GetProfileIntW(SHORTCUTS_SECTION, x.orig_name, Setting);
		Setting = App->GetProfileIntW(SHORTCUTS_SECTION, x.name, Setting);

		x.key = Setting & 0xFF;
		x.mod = Setting >> 8;
		if (x.mod == MOD_NONE && x.key)		// // //
			m_iUsedKeys.insert(x.key);
	}
}

void CAccelerator::LoadDefaults()
{
	m_pEntriesTable = DEFAULT_TABLE;		// // //
}

void CAccelerator::Setup()
{
	// Translate key table -> windows accelerator table

	if (m_hAccel) {
		DestroyAcceleratorTable(m_hAccel);
		m_hAccel = NULL;
	}

	m_pAccelTable.clear();		// // //
	for (const auto &x : m_pEntriesTable)
		if (x.key) {
			ACCEL a;
			a.fVirt = FVIRTKEY | GetMod(x.mod);
			a.key = x.key;
			a.cmd = x.id;
			m_pAccelTable.push_back(a);
		}
	m_hAccel = CreateAcceleratorTableW(m_pAccelTable.data(), ACCEL_COUNT);
}

BOOL CAccelerator::Translate(HWND hWnd, MSG *pMsg)
{
	if (m_hAdditionalAccel) {
		if (TranslateAcceleratorW(hWnd, m_hAdditionalAccel, pMsg)) {
			return TRUE;
		}
	}

	if (m_hAccel) {
		if (TranslateAcceleratorW(hWnd, m_hAccel, pMsg)) {
			return TRUE;
		}
	}

	return FALSE;
}

void CAccelerator::Shutdown()
{
	if (m_hAccel) {
		DestroyAcceleratorTable(m_hAccel);
		m_hAccel = NULL;
	}
}

void CAccelerator::SetAccelerator(HACCEL hAccel)
{
	m_hAdditionalAccel = hAccel;
}
