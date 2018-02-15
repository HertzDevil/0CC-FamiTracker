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

// FamiTracker.h : main header file for the FamiTracker application

#include <memory>		// // //
#include "FamiTrackerTypes.h"		// // //

// Support DLL translations
#define SUPPORT_TRANSLATIONS

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "../resource.h"       // main symbols

// Inter-process commands
enum {
	IPC_LOAD = 1,
	IPC_LOAD_PLAY
};

enum class render_type_t : unsigned char;

// Custom command line reader
class CFTCommandLineInfo : public CCommandLineInfo
{
public:
	CFTCommandLineInfo();		// // //

	void ParseParam(const WCHAR* pszParam, BOOL bFlag, BOOL bLast) override;

	bool m_bLog = false;
	bool m_bExport = false;
	bool m_bPlay = false;
	bool m_bRender = false;		// // //
	CStringW m_strExportFile;
	CStringW m_strExportLogFile;
	CStringW m_strExportDPCMFile;
	unsigned track_ = MAX_TRACKS;
	unsigned render_param_ = 1;		// // //
	render_type_t render_type_;		// // //
};

class CMainFrame;		// // //
class CMIDI;
class CSoundGen;
class CSettings;
class CAccelerator;
class CRecentFileList;		// // //
class CVersionChecker;		// // //

class CMutex;

enum class play_mode_t;		// // // Defined in PlayerCursor.h

/*!
	\brief A MFC document template supporting both .0cc and .ftm file extensions.
*/
class CDocTemplate0CC : public CSingleDocTemplate
{
public:
	CDocTemplate0CC(UINT nIDResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass);
	BOOL GetDocString(CStringW& rString, enum DocStringIndex i) const;
	CDocTemplate::Confidence MatchDocType(LPCWSTR lpszPathName, CDocument*& rpDocMatch);
};

/*!
	\brief A MFC document manager allowing both .0cc and .ftm file extensions to be displayed while
	opening or saving documents.
	\details This class also saves the FTM path of the last load/save operation to the registry.
*/
class CDocManager0CC : public CDocManager
{
public:
	virtual BOOL DoPromptFileName(CStringW& fileName, UINT nIDSTitle,
								  DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate);
};

// CFamiTrackerApp:
// See FamiTracker.cpp for the implementation of this class
//

class CFamiTrackerApp : public CWinApp
{
public:
	// Constructor
	CFamiTrackerApp();

	//
	// Public functions
	//
public:
	void			CheckNewVersion(bool StartUp);		// // //
	void			LoadSoundConfig();
	void			UpdateMenuShortcuts();		// // //
	void			ReloadColorScheme();
	int				GetCPUUsage() const;
	bool			IsThemeActive() const;

	// Tracker player functions
	void			StartPlayer(play_mode_t Mode);
	void			StopPlayer();
	void			StopPlayerAndWait();
	void			TogglePlayer();
	void			ResetPlayer();
	void			SilentEverything();

	// Get-functions
	CMainFrame		*GetMainFrame();		// // //
	CAccelerator	*GetAccelerator()		{ ASSERT(m_pAccel); return m_pAccel.get(); }
	CSoundGen		*GetSoundGenerator()	{ ASSERT(m_pSoundGenerator); return m_pSoundGenerator.get(); }
	CMIDI			*GetMIDI()				{ ASSERT(m_pMIDI); return m_pMIDI.get(); }
	CSettings		*GetSettings()			{ ASSERT(m_pSettings); return m_pSettings; }

	//
	// Private functions
	//
private:
	void CheckAppThemed();
	void ShutDownSynth();
	bool CheckSingleInstance(CFTCommandLineInfo &cmdInfo);
	void RegisterSingleInstance();
	void UnregisterSingleInstance();
	void LoadLocalization();

	// Private variables and objects
private:
	static const int MAX_RECENT_FILES = 8;		// // //

	// Objects
	std::unique_ptr<CMIDI>			m_pMIDI;		// // //
	std::unique_ptr<CAccelerator>	m_pAccel;		// // // Keyboard accelerator
	std::unique_ptr<CSoundGen>		m_pSoundGenerator;		// // // Sound synth & player

	CSettings		*m_pSettings = nullptr;		// Program settings

	// Single instance stuff
	std::unique_ptr<CMutex>			m_pInstanceMutex;
	HANDLE			m_hWndMapFile = NULL;

	bool			m_bThemeActive = false;

	std::unique_ptr<CVersionChecker> m_pVersionChecker;		// // //

#ifdef SUPPORT_TRANSLATIONS
	HINSTANCE		m_hInstResDLL = NULL;
#endif

	// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	// Implementation
	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnIdle(LONG lCount);		// // //
	afx_msg void OnAppAbout();
	afx_msg void OnFileOpen();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnTestExport();
	void OnRecentFilesClear();		// // //
	void OnUpdateRecentFilesClear(CCmdUI *pCmdUI);		// // //
};

extern CFamiTrackerApp theApp;

// Global helper functions
CStringW LoadDefaultFilter(LPCWSTR Name, LPCWSTR Ext);
CStringW LoadDefaultFilter(UINT nID, LPCWSTR Ext);
void AfxFormatString3(CStringW &rString, UINT nIDS, LPCWSTR lpsz1, LPCWSTR lpsz2, LPCWSTR lpsz3);
CStringW MakeIntString(int val, LPCWSTR format = L"%i");
CStringW MakeFloatString(float val, LPCWSTR format = L"%g");
