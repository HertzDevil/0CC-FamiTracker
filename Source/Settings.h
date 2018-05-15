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

#include "ft0cc/fs.h"		// // //

// CSettings command target

enum class edit_style_t { FT2, MPT, IT };		// // // renamed

enum class win_state_t { Normal, Maximized };		// // //

enum PATHS {
	PATH_FTM,
	PATH_FTI,
	PATH_NSF,
	PATH_DMC,
	PATH_WAV,
	PATH_INST,		// // //

	PATH_COUNT,
};

enum module_error_level_t : unsigned char;		// // //

// Settings collection
class CSettings {
private:
	CSettings() = default;

public:
	fs::path GetPath(unsigned int PathType) const;		// // //
	void SetPath(fs::path PathName, unsigned int PathType);		// // //

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
		edit_style_t iEditStyle;		// // //
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
		module_error_level_t iErrorLevel;
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
		unsigned long	iColBackground;
		unsigned long	iColBackgroundHilite;
		unsigned long	iColBackgroundHilite2;
		unsigned long	iColPatternText;
		unsigned long	iColPatternTextHilite;
		unsigned long	iColPatternTextHilite2;
		unsigned long	iColPatternInstrument;
		unsigned long	iColPatternVolume;
		unsigned long	iColPatternEffect;
		unsigned long	iColSelection;
		unsigned long	iColCursor;
		unsigned long	iColCurrentRowNormal;		// // //
		unsigned long	iColCurrentRowEdit;
		unsigned long	iColCurrentRowPlaying;

		std::wstring strFont;		// // //
		std::wstring strFrameFont;		// // // 050B
		int		iFontSize;
		bool	bPatternColor;
		bool	bDisplayFlats;
	} Appearance;

	struct {
		int		iLeft;
		int		iTop;
		int		iRight;
		int		iBottom;
		win_state_t iState;
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
	bool bFollowMode;
	bool bFastMeterDecayRate;		// // // 050B
	bool bLinearNamcoMixing;		// // //

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

private:
	fs::path Paths[PATH_COUNT];		// // //
};
