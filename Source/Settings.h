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

#include "stdafx.h"		// // //
#include <memory>		// // //
#include <vector>		// // //

// CSettings command target

enum EDIT_STYLES {		// // // renamed
	EDIT_STYLE_FT2 = 0,		// FT2
	EDIT_STYLE_MPT = 1,		// ModPlug
	EDIT_STYLE_IT = 2,		// IT
};

enum WIN_STATES {
	STATE_NORMAL,
	STATE_MAXIMIZED,
};

enum PATHS {
	PATH_FTM,
	PATH_FTI,
	PATH_NSF,
	PATH_DMC,
	PATH_WAV,

	PATH_COUNT,
};

// Settings collection
class CSettings {
private:
	CSettings() = default;

public:
	CStringW &GetPath(unsigned int PathType);		// // //
	const CStringW &GetPath(unsigned int PathType) const;		// // //
	void SetPath(const CStringW &PathName, unsigned int PathType);

public:
	static CSettings &GetInstance();		// // //

public:
	// Local cache of all settings (all public)

	struct {
		bool	bWrapCursor;
		bool	bWrapFrames;
		bool	bFreeCursorEdit;
		bool	bWavePreview;
		bool	bKeyRepeat;
		bool	bRowInHex;
		bool	bFramePreview;
		int		iEditStyle;
		bool	bNoDPCMReset;
		bool	bNoStepMove;
		int		iPageStepSize;
		bool	bPullUpDelete;
		bool	bBackups;
		bool	bSingleInstance;
		bool	bPreviewFullRow;
		bool	bDblClickSelect;
		bool	bWrapPatternValue;		// // //
		bool	bCutVolume;
		bool	bFDSOldVolume;
		bool	bRetrieveChanState;
		bool	bOverflowPaste;
		bool	bShowSkippedRows;
		bool	bHexKeypad;
		bool	bMultiFrameSel;
		bool	bCheckVersion;		// // //
	} General;

	struct {
		int		iErrorLevel;
	} Version;		// // //

	struct {
		int		iDevice;
		int		iSampleRate;
		int		iSampleSize;
		int		iBufferLength;
		int		iBassFilter;
		int		iTrebleFilter;
		int		iTrebleDamping;
		int		iMixVolume;
	} Sound;

	struct {
		int		iMidiDevice;
		int		iMidiOutDevice;
		bool	bMidiMasterSync;
		bool	bMidiKeyRelease;
		bool	bMidiChannelMap;
		bool	bMidiVelocity;
		bool	bMidiArpeggio;
	} Midi;

	struct {
		int		iColBackground;
		int		iColBackgroundHilite;
		int		iColBackgroundHilite2;
		int		iColPatternText;
		int		iColPatternTextHilite;
		int		iColPatternTextHilite2;
		int		iColPatternInstrument;
		int		iColPatternVolume;
		int		iColPatternEffect;
		int		iColSelection;
		int		iColCursor;
		int		iColCurrentRowNormal;		// // //
		int		iColCurrentRowEdit;
		int		iColCurrentRowPlaying;

		CStringW	strFont;		// // //
		CStringW	strFrameFont;		// // // 050B
		int		iFontSize;
		bool	bPatternColor;
		bool	bDisplayFlats;
	} Appearance;

	struct {
		int		iLeft;
		int		iTop;
		int		iRight;
		int		iBottom;
		int		iState;
	} WindowPos;

	struct {
		int		iKeyNoteCut;
		int		iKeyNoteRelease;
		int		iKeyClear;
		int		iKeyRepeat;
		int		iKeyEchoBuffer;		// // //
	} Keys;

	struct {
		bool	bAverageBPM;
		bool	bChannelState;
		bool	bRegisterState;
	} Display;		// // // 050B

	// Other
	int SampleWinState;
	int FrameEditPos;
	int ControlPanelPos;		// // // 050B
	bool FollowMode;
	bool MeterDecayRate;		// // // 050B
	bool m_bNamcoMixing;		// // //

	struct {
		int		iLevelAPU1;
		int		iLevelAPU2;
		int		iLevelVRC6;
		int		iLevelVRC7;
		int		iLevelMMC5;
		int		iLevelFDS;
		int		iLevelN163;
		int		iLevelS5B;
	} ChipLevels;

	CStringW InstrumentMenuPath;

private:
	CStringW Paths[PATH_COUNT];		// // //
};
