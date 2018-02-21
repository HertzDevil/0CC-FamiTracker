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

#include "stdafx.h"
#include "FamiTracker.h"
#include "version.h"		// // //
#include "Exception.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"		// // //
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "AboutDlg.h"
#include "MIDI.h"
#include "SoundGen.h"
#include "Accelerator.h"
#include "Settings.h"
#include "CommandLineExport.h"
#include "WinSDK/VersionHelpers.h"		// // //
#include "WaveRenderer.h"		// // //
#include "WaveRendererFactory.h"		// // //
#include "VersionChecker.h"		// // //
#include <iostream>		// // //
#include "str_conv/str_conv.hpp"		// // //

#include <afxadv.h>		// // // CRecentFileList
#if !defined(WIP) && !defined(_DEBUG)		// // //
#include <afxpriv.h>
#endif

// Single instance-stuff
const WCHAR FT_SHARED_MUTEX_NAME[]	= L"FamiTrackerMutex";	// Name of global mutex
const WCHAR FT_SHARED_MEM_NAME[]	= L"FamiTrackerWnd";		// Name of global memory area
const DWORD	SHARED_MEM_SIZE			= 256;

namespace {

template <class T, typename... Args>
inline std::unique_ptr<T>
make_unique_nothrow(Args&&... args) {
	try {
		return std::make_unique<T>(std::forward<Args>(args)...);
	}
	catch (std::bad_alloc) {
		return nullptr;
	}
}

} // namespace

// CFamiTrackerApp

BEGIN_MESSAGE_MAP(CFamiTrackerApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_RECENTFILES_CLEAR, OnRecentFilesClear)		// // //
	ON_UPDATE_COMMAND_UI(ID_FILE_MRU_FILE1, OnUpdateRecentFilesClear)		// // //
END_MESSAGE_MAP()

// Include this for windows xp style in visual studio 2005 or later
#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")

// CFamiTrackerApp construction

CFamiTrackerApp::CFamiTrackerApp()
{
	// Place all significant initialization in InitInstance
	EnableHtmlHelp();

#ifdef ENABLE_CRASH_HANDLER
	// This will cover the whole process
	InstallExceptionHandler();
#endif /* ENABLE_CRASH_HANDLER */
}


// The one and only CFamiTrackerApp object
CFamiTrackerApp	theApp;

// CFamiTrackerApp initialization

BOOL CFamiTrackerApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();
#ifdef SUPPORT_TRANSLATIONS
	LoadLocalization();
#endif
	CWinApp::InitInstance();

	TRACE(L"App: InitInstance\n");

	if (!AfxOleInit()) {
		TRACE(L"OLE initialization failed\n");
	}

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(L"");
	LoadStdProfileSettings(MAX_RECENT_FILES);  // Load standard INI file options (including MRU)

	// Load program settings
	m_pSettings = &CSettings::GetInstance();		// // //
	m_pSettings->LoadSettings();

	// Parse command line for standard shell commands, DDE, file open + some custom ones
	CFTCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	if (CheckSingleInstance(cmdInfo))
		return FALSE;

	// Load custom accelerator
	m_pAccel = make_unique_nothrow<CAccelerator>();		// // //
	if (!m_pAccel)
		return FALSE;
	m_pAccel->LoadShortcuts(m_pSettings);
	m_pAccel->Setup();

	// Create the MIDI interface
	m_pMIDI = make_unique_nothrow<CMIDI>();
	if (!m_pMIDI)
		return FALSE;

	// Create sound generator
	m_pSoundGenerator = make_unique_nothrow<CSoundGen>();		// // //
	if (!m_pSoundGenerator)
		return FALSE;

	// Start sound generator thread, initially suspended
	if (!m_pSoundGenerator->CreateThread(CREATE_SUSPENDED)) {
		// If failed, restore and save default settings
		m_pSettings->DefaultSettings();
		m_pSettings->SaveSettings();
		// Show message and quit
		AfxMessageBox(IDS_START_ERROR, MB_ICONERROR);
		return FALSE;
	}

	// Check if the application is themed
	CheckAppThemed();

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	auto pDocTemplate = make_unique_nothrow<CDocTemplate0CC>(		// // //
		IDR_MAINFRAME,
		RUNTIME_CLASS(CFamiTrackerDoc),
		RUNTIME_CLASS(CMainFrame),
		RUNTIME_CLASS(CFamiTrackerView)).release(); // owned by CDocManager

	if (!pDocTemplate)
		return FALSE;

	if (!m_pDocManager)		// // //
		m_pDocManager = make_unique_nothrow<CDocManager0CC>().release(); // owned by CWinApp
	m_pDocManager->AddDocTemplate(pDocTemplate);

	// Work-around to enable file type registration in windows vista/7
	if (IsWindowsVistaOrGreater()) {		// // //
		HKEY HKCU;
		long res_reg = ::RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Classes", &HKCU);
		if(res_reg == ERROR_SUCCESS)
			RegOverridePredefKey(HKEY_CLASSES_ROOT, HKCU);
	}

	// Enable DDE Execute open
	EnableShellOpen();

	// Skip this if in wip/beta mode
#if !defined(WIP) && !defined(_DEBUG)
	// Add shell options
	RegisterShellFileTypes();		// // //
	static const LPCWSTR FILE_ASSOC_NAME = L"0CC-FamiTracker Module";
	RegSetValueW(HKEY_CLASSES_ROOT, L"0CCFamiTracker.Document", REG_SZ, FILE_ASSOC_NAME, wcslen(FILE_ASSOC_NAME) * sizeof(WCHAR));
	// Add an option to play files
	CStringW strPathName, strTemp, strFileTypeId;
	AfxGetModuleShortFileName(AfxGetInstanceHandle(), strPathName);
	CStringW strOpenCommandLine = strPathName;
	strOpenCommandLine += L" /play \"%1\"";
	if (pDocTemplate->GetDocString(strFileTypeId, CDocTemplate::regFileTypeId) && !strFileTypeId.IsEmpty()) {
		strTemp.Format(L"%s\\shell\\play\\%s", (LPCWSTR)strFileTypeId, L"command");
		RegSetValueW(HKEY_CLASSES_ROOT, strTemp, REG_SZ, strOpenCommandLine, wcslen(strOpenCommandLine) * sizeof(WCHAR));
	}
#endif

	// // // WAV render
	if (cmdInfo.m_bRender) {
		CRuntimeClass *pRuntimeClass = RUNTIME_CLASS(CFamiTrackerDoc);
		std::unique_ptr<CObject> pObject {pRuntimeClass->CreateObject()};
		if (!pObject || !pObject->IsKindOf(pRuntimeClass)) {
			std::cerr << "Error: unable to create CFamiTrackerDoc\n";
			ExitProcess(1);
			return FALSE;
		}
		auto &doc = static_cast<CFamiTrackerDoc &>(*pObject);
		if (!doc.OnOpenDocument(cmdInfo.m_strFileName)) {
			std::cerr << "Error: unable to open document: " << cmdInfo.m_strFileName << '\n';
			ExitProcess(1);
			return FALSE;
		}
		if ((unsigned)cmdInfo.track_ >= doc.GetModule()->GetSongCount())
			cmdInfo.track_ = 0;
		m_pSoundGenerator->AssignDocument(&doc);
		m_pSoundGenerator->InitializeSound(NULL);

		std::unique_ptr<CWaveRenderer> render = CWaveRendererFactory::Make(*doc.GetModule(), cmdInfo.track_, cmdInfo.render_type_, cmdInfo.render_param_);
		if (!render) {
			std::cerr << "Error: unable to create wave renderer!\n";
			ExitProcess(1);
			return FALSE;
		}
		render->SetRenderTrack(cmdInfo.track_);
		if (!m_pSoundGenerator->RenderToFile(cmdInfo.m_strExportFile, std::move(render))) {
			std::cerr << "Error: unable to render WAV file: " << cmdInfo.m_strExportFile << '\n';
			ExitProcess(1);
			return FALSE;
		}
		while (!m_pSoundGenerator->IsRendering())
			;
		std::cout << "Rendering started... ";
		while (m_pSoundGenerator->IsRendering())
			;//			m_pSoundGenerator->IdleLoop();
		m_pSoundGenerator->StopPlayer();
		m_pSoundGenerator->RemoveDocument();
		ShutDownSynth();
		std::cout << "Done." << std::endl;
		ExitProcess(0);
		return TRUE;
	}

	// Handle command line export
	if (cmdInfo.m_bExport) {
		CCommandLineExport exporter;
		exporter.CommandLineExport(cmdInfo.m_strFileName, cmdInfo.m_strExportFile, cmdInfo.m_strExportLogFile, cmdInfo.m_strExportDPCMFile);
		ExitProcess(0);
	}

	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo)) {
		if (cmdInfo.m_nShellCommand == CCommandLineInfo::AppUnregister) {
			// Also clear settings from registry when unregistering
			m_pSettings->DeleteSettings();
		}
		return FALSE;
	}

	// Move root key back to default
	if (IsWindowsVistaOrGreater()) {		// // //
		::RegOverridePredefKey(HKEY_CLASSES_ROOT, NULL);
	}

	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(m_nCmdShow);
	m_pMainWnd->UpdateWindow();
	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Initialize the sound interface, also resumes the thread
	if (!m_pSoundGenerator->InitializeSound(m_pMainWnd->m_hWnd)) {
		// If failed, restore and save default settings
		m_pSettings->DefaultSettings();
		m_pSettings->SaveSettings();
		// Quit program
		AfxMessageBox(IDS_START_ERROR, MB_ICONERROR);
		return FALSE;
	}

	// Initialize midi unit
	m_pMIDI->Init();

	if (cmdInfo.m_bPlay)
		StartPlayer(play_mode_t::Frame);

	// Save the main window handle
	RegisterSingleInstance();

#ifndef _DEBUG
	m_pMainWnd->GetMenu()->GetSubMenu(4)->RemoveMenu(ID_MODULE_CHANNELS, MF_BYCOMMAND);		// // //
#endif

	if (m_pSettings->General.bCheckVersion)		// // //
		CheckNewVersion(true);

	// Initialization is done
	TRACE(L"App: InitInstance done\n");

	return TRUE;
}

int CFamiTrackerApp::ExitInstance()
{
	// Close program
	// The document is already closed at this point (and detached from sound player)

	TRACE(L"App: Begin ExitInstance\n");

	UnregisterSingleInstance();

	ShutDownSynth();

	if (m_pMIDI) {
		m_pMIDI->Shutdown();
		m_pMIDI.reset();
	}

	if (m_pAccel) {
		m_pAccel->SaveShortcuts(m_pSettings);
		m_pAccel->Shutdown();
		m_pAccel.reset();
	}

	if (m_pSettings) {
		m_pSettings->SaveSettings();
		m_pSettings = nullptr;
	}

#ifdef SUPPORT_TRANSLATIONS
	if (m_hInstResDLL) {
		// Revert back to internal resources
		AfxSetResourceHandle(m_hInstance);
		// Unload DLL
		::FreeLibrary(m_hInstResDLL);
		m_hInstResDLL = NULL;
	}
#endif

	m_pVersionChecker.reset();		// // //

	TRACE(L"App: End ExitInstance\n");

	return CWinApp::ExitInstance();
}

BOOL CFamiTrackerApp::PreTranslateMessage(MSG* pMsg)
{
	if (CWinApp::PreTranslateMessage(pMsg)) {
		return TRUE;
	}
	else if (m_pMainWnd != NULL && m_pAccel != NULL) {
		if (m_pAccel->Translate(m_pMainWnd->m_hWnd, pMsg)) {
			return TRUE;
		}
	}

	return FALSE;
}

void CFamiTrackerApp::CheckAppThemed()
{
	HMODULE hinstDll = ::LoadLibraryW(L"UxTheme.dll");

	if (hinstDll) {
		typedef BOOL (*ISAPPTHEMEDPROC)();
		ISAPPTHEMEDPROC pIsAppThemed;
		pIsAppThemed = (ISAPPTHEMEDPROC) ::GetProcAddress(hinstDll, "IsAppThemed");

		if(pIsAppThemed)
			m_bThemeActive = (pIsAppThemed() == TRUE);

		::FreeLibrary(hinstDll);
	}
}

bool CFamiTrackerApp::IsThemeActive() const
{
	return m_bThemeActive;
}

#ifdef SUPPORT_TRANSLATIONS
bool GetFileVersion(LPCWSTR Filename, WORD &Major, WORD &Minor, WORD &Revision, WORD &Build) {
	DWORD Handle;
	DWORD Size = GetFileVersionInfoSizeW(Filename, &Handle);

	Major = Minor = Revision = Build = 0;

	if (Size > 0) {
		if (std::vector<WCHAR> pData(Size); GetFileVersionInfoW(Filename, NULL, Size, pData.data()) != 0) {		// // //
			UINT size;
			VS_FIXEDFILEINFO *pFileinfo;
			if (VerQueryValueW(pData.data(), L"\\", (LPVOID*)&pFileinfo, &size) != 0) {
				Major = pFileinfo->dwProductVersionMS >> 16;
				Minor = pFileinfo->dwProductVersionMS & 0xFFFF;
				Revision = pFileinfo->dwProductVersionLS >> 16;
				Build = pFileinfo->dwProductVersionLS & 0xFFFF;
				return true;
			}
		}
	}

	return false;
}

void CFamiTrackerApp::LoadLocalization()
{
	LPCWSTR DLL_NAME = L"language.dll";
	WORD Major, Minor, Build, Revision;

	if (GetFileVersion(DLL_NAME, Major, Minor, Revision, Build)) {
		if (0 != Compare0CCFTVersion(Major, Minor, Revision, Build))		// // //
			return;

		m_hInstResDLL = ::LoadLibraryW(DLL_NAME);

		if (m_hInstResDLL != NULL) {
			TRACE(L"App: Loaded localization DLL\n");
			AfxSetResourceHandle(m_hInstResDLL);
		}
	}
}
#endif

void CFamiTrackerApp::OnRecentFilesClear()		// // //
{
	std::unique_ptr<CRecentFileList> {m_pRecentFileList} = std::make_unique<CRecentFileList>(
		0, L"Recent File List", L"File%d", MAX_RECENT_FILES);		// // // owned by CWinApp

	auto pMenu = m_pMainWnd->GetMenu()->GetSubMenu(0)->GetSubMenu(14);
	for (int i = 0; i < MAX_RECENT_FILES; ++i)
		pMenu->RemoveMenu(ID_FILE_MRU_FILE1 + i, MF_BYCOMMAND);
	pMenu->AppendMenuW(MF_STRING, ID_FILE_MRU_FILE1, L"(File)");
}

void CFamiTrackerApp::OnUpdateRecentFilesClear(CCmdUI *pCmdUI)		// // //
{
	m_pRecentFileList->UpdateMenu(pCmdUI);
}

void CFamiTrackerApp::ShutDownSynth()
{
	// Shut down sound generator
	if (!m_pSoundGenerator) {
		TRACE(L"App: Sound generator object was not available\n");
		return;
	}

	if (!m_pSoundGenerator->m_hThread) {		// // //
		// Object was found but thread not created
		TRACE(L"App: Sound generator object was found but no thread created\n");
		return;
	}

	TRACE(L"App: Waiting for sound player thread to close\n");

	if (!m_pSoundGenerator || m_pSoundGenerator->Shutdown())		// // //
		TRACE(L"App: Sound generator has closed\n");
	else
		TRACE(L"App: Closing the sound generator thread failed\n");
}

void CFamiTrackerApp::RegisterSingleInstance()
{
	// Create a memory area with this app's window handle
	if (!GetSettings()->General.bSingleInstance)
		return;

	m_hWndMapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SHARED_MEM_SIZE, FT_SHARED_MEM_NAME);

	if (m_hWndMapFile != NULL) {
		LPTSTR pBuf = (LPTSTR) MapViewOfFile(m_hWndMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_MEM_SIZE);
		if (pBuf != NULL) {
			// Create a string of main window handle
			_itot_s((int)GetMainWnd()->m_hWnd, pBuf, SHARED_MEM_SIZE, 10);
			UnmapViewOfFile(pBuf);
		}
	}
}

void CFamiTrackerApp::UnregisterSingleInstance()
{
	// Close shared memory area
	if (m_hWndMapFile) {
		CloseHandle(m_hWndMapFile);
		m_hWndMapFile = NULL;
	}

	m_pInstanceMutex.reset();		// // //
}

void CFamiTrackerApp::CheckNewVersion(bool StartUp)		// // //
{
	m_pVersionChecker = std::make_unique<CVersionChecker>(StartUp);		// // //
}

bool CFamiTrackerApp::CheckSingleInstance(CFTCommandLineInfo &cmdInfo)
{
	// Returns true if program should close

	if (!GetSettings()->General.bSingleInstance)
		return false;

	if (cmdInfo.m_bExport || cmdInfo.m_bRender)		// // //
		return false;

	m_pInstanceMutex = std::make_unique<CMutex>(FALSE, FT_SHARED_MUTEX_NAME);		// // //

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		// Another instance detected, get window handle
		HANDLE hMapFile = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, FT_SHARED_MEM_NAME);
		if (hMapFile != NULL) {
			LPCWSTR pBuf = (LPTSTR) MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_MEM_SIZE);
			if (pBuf != NULL) {
				// Get window handle
				HWND hWnd = (HWND)_ttoi(pBuf);
				if (hWnd != NULL) {
					// Get file name
					LPTSTR pFilePath = cmdInfo.m_strFileName.GetBuffer();
					// We have the window handle & file, send a message to open the file
					COPYDATASTRUCT data;
					data.dwData = cmdInfo.m_bPlay ? IPC_LOAD_PLAY : IPC_LOAD;
					data.cbData = (DWORD)((wcslen(pFilePath) + 1) * sizeof(WCHAR));
					data.lpData = pFilePath;
					DWORD result;
					SendMessageTimeoutW(hWnd, WM_COPYDATA, NULL, (LPARAM)&data, SMTO_NORMAL, 100, &result);
					UnmapViewOfFile(pBuf);
					CloseHandle(hMapFile);
					TRACE(L"App: Found another instance, shutting down\n");
					// Then close the program
					return true;
				}

				UnmapViewOfFile(pBuf);
			}
			CloseHandle(hMapFile);
		}
	}

	return false;
}

////////////////////////////////////////////////////////
//  Things that belongs to the synth are kept below!  //
////////////////////////////////////////////////////////

// Load sound configuration
void CFamiTrackerApp::LoadSoundConfig()
{
	GetSoundGenerator()->LoadSettings();
	GetSoundGenerator()->Interrupt();
	static_cast<CFrameWnd*>(GetMainWnd())->SetMessageText(IDS_NEW_SOUND_CONFIG);
}

void CFamiTrackerApp::UpdateMenuShortcuts()		// // //
{
	CMainFrame *pMainFrm = GetMainFrame();		// // //
	if (pMainFrm != nullptr)
		pMainFrm->UpdateMenus();
}

// Silences everything
void CFamiTrackerApp::SilentEverything()
{
	GetSoundGenerator()->SilentAll();
	CFamiTrackerView::GetView()->MakeSilent();
}

// Get-functions

CMainFrame *CFamiTrackerApp::GetMainFrame() {		// // //
	return dynamic_cast<CMainFrame *>(GetMainWnd());
}

int CFamiTrackerApp::GetCPUUsage() const
{
	// Calculate CPU usage
	const int THREAD_COUNT = 2;
	static FILETIME KernelLastTime[THREAD_COUNT], UserLastTime[THREAD_COUNT];
	const HANDLE hThreads[THREAD_COUNT] = {m_hThread, m_pSoundGenerator->m_hThread};
	unsigned int TotalTime = 0;

	for (int i = 0; i < THREAD_COUNT; ++i) {
		FILETIME CreationTime, ExitTime, KernelTime, UserTime;
		GetThreadTimes(hThreads[i], &CreationTime, &ExitTime, &KernelTime, &UserTime);
		TotalTime += (KernelTime.dwLowDateTime - KernelLastTime[i].dwLowDateTime) / 1000;
		TotalTime += (UserTime.dwLowDateTime - UserLastTime[i].dwLowDateTime) / 1000;
		KernelLastTime[i] = KernelTime;
		UserLastTime[i]	= UserTime;
	}

	return TotalTime;
}

void CFamiTrackerApp::ReloadColorScheme()
{
	// Notify all views
	POSITION TemplatePos = GetFirstDocTemplatePosition();
	CDocTemplate *pDocTemplate = GetNextDocTemplate(TemplatePos);
	POSITION DocPos = pDocTemplate->GetFirstDocPosition();

	while (CDocument* pDoc = pDocTemplate->GetNextDoc(DocPos)) {
		POSITION ViewPos = pDoc->GetFirstViewPosition();
		while (CView *pView = pDoc->GetNextView(ViewPos)) {
			if (pView->IsKindOf(RUNTIME_CLASS(CFamiTrackerView)))
				static_cast<CFamiTrackerView*>(pView)->SetupColors();
		}
	}

	// Main window
	CMainFrame *pMainFrm = dynamic_cast<CMainFrame*>(GetMainWnd());

	if (pMainFrm != NULL) {
		pMainFrm->SetupColors();
		pMainFrm->RedrawWindow();
	}
}

BOOL CFamiTrackerApp::OnIdle(LONG lCount)		// // //
{
	if (CWinApp::OnIdle(lCount))
		return TRUE;

	if (m_pVersionChecker && m_pVersionChecker->IsReady())
		if (auto pChecker = std::move(m_pVersionChecker); auto result = pChecker->GetVersionCheckResult())
			if (AfxMessageBox(conv::to_wide(result->Message).data(), result->MessageBoxStyle) == IDYES)
				ShellExecuteW(NULL, L"open", conv::to_wide(result->URL).data(), NULL, NULL, SW_SHOWNORMAL);

	return FALSE;
}

// App command to run the about dialog
void CFamiTrackerApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CFamiTrackerApp message handlers

void CFamiTrackerApp::StartPlayer(play_mode_t Mode)
{
	if (m_pSoundGenerator) {		// // //
		auto cur = std::make_unique<CPlayerCursor>(CFamiTrackerView::GetView()->GetPlayerCursor(Mode));
		m_pSoundGenerator->StartPlayer(std::move(cur));
	}
}

void CFamiTrackerApp::StopPlayer()
{
	if (m_pSoundGenerator)
		m_pSoundGenerator->StopPlayer();

	m_pMIDI->ResetOutput();
}

void CFamiTrackerApp::StopPlayerAndWait()
{
	// Synchronized stop
	if (m_pSoundGenerator) {
		m_pSoundGenerator->StopPlayer();
		m_pSoundGenerator->WaitForStop();
	}
	m_pMIDI->ResetOutput();
}

void CFamiTrackerApp::TogglePlayer()
{
	if (m_pSoundGenerator) {
		if (m_pSoundGenerator->IsPlaying())
			StopPlayer();
		else
			StartPlayer(play_mode_t::Frame);
	}
}

// Player interface

void CFamiTrackerApp::ResetPlayer()
{
	// Called when changing track
	if (m_pSoundGenerator)
		m_pSoundGenerator->ResetPlayer(GetMainFrame()->GetSelectedTrack());		// // //
}

// File load/save

void CFamiTrackerApp::OnFileOpen()
{
	CStringW newName = L"";		// // //

	if (!AfxGetApp()->DoPromptFileName(newName, AFX_IDS_OPENFILE, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, TRUE, NULL))
		return; // open cancelled

	CFrameWnd *pFrameWnd = (CFrameWnd*)GetMainWnd();

	if (pFrameWnd)
		pFrameWnd->SetMessageText(IDS_LOADING_FILE);

	AfxGetApp()->OpenDocumentFile(newName);

	if (pFrameWnd)
		pFrameWnd->SetMessageText(IDS_LOADING_DONE);
}

// Various global helper functions

CStringW LoadDefaultFilter(LPCWSTR Name, LPCWSTR Ext)
{
	// Loads a single filter string including the all files option
	CStringW filter;
	CStringW allFilter;
	VERIFY(allFilter.LoadStringW(AFX_IDS_ALLFILTER));

	filter.Format(L"%s|*%s|%s|*.*||", Name, Ext, allFilter);
	return filter;
}

CStringW LoadDefaultFilter(UINT nID, LPCWSTR Ext)
{
	CStringW str;		// // //
	VERIFY(str.LoadStringW(nID));
	return LoadDefaultFilter((LPCWSTR)str, Ext);
}

void AfxFormatString3(CStringW &rString, UINT nIDS, LPCWSTR lpsz1, LPCWSTR lpsz2, LPCWSTR lpsz3)
{
	// AfxFormatString with three arguments
	LPCWSTR arr[] = {lpsz1, lpsz2, lpsz3};
	AfxFormatStrings(rString, nIDS, arr, 3);
}

CStringW MakeIntString(int val, LPCWSTR format)
{
	// Turns an int into a string
	CStringW str;
	str.Format(format, val);
	return str;
}

CStringW MakeFloatString(float val, LPCWSTR format)
{
	// Turns a float into a string
	CStringW str;
	str.Format(format, val);
	return str;
}

/**
 * CFTCommandLineInfo, a custom command line parser
 *
 */

CFTCommandLineInfo::CFTCommandLineInfo() : CCommandLineInfo(), render_type_(render_type_t::Loops) {
}

void CFTCommandLineInfo::ParseParam(const WCHAR* pszParam, BOOL bFlag, BOOL bLast)
{
	if (bFlag) {
		// Export file (/export or /e)
		if (!_wcsicmp(pszParam, L"export") || !_wcsicmp(pszParam, L"e")) {
			m_bExport = true;
			return;
		}
		// Auto play (/play or /p)
		else if (!_wcsicmp(pszParam, L"play") || !_wcsicmp(pszParam, L"p")) {
			m_bPlay = true;
			return;
		}
		// // // Render (/render or /r)
		else if (!_wcsicmp(pszParam, L"render") || !_wcsicmp(pszParam, L"r")) {
			m_bRender = true;
			return;
		}
		// Disable crash dumps (/nodump)
		else if (!_wcsicmp(pszParam, L"nodump")) {
#ifdef ENABLE_CRASH_HANDLER
			UninstallExceptionHandler();
#endif
			return;
		}
		// Enable register logger (/log), available in debug mode only
		else if (!_wcsicmp(pszParam, L"log")) {
#ifdef _DEBUG
			m_bLog = true;
			return;
#endif
		}
		// Enable console output (TODO)
		// This is intended for a small helper program that avoids the problem with console on win32 programs,
		// and should remain undocumented. I'm using it for testing.
		else if (!_wcsicmp(pszParam, L"console")) {
			FILE *f;
			AttachConsole(ATTACH_PARENT_PROCESS);
			errno_t err = freopen_s(&f, "CON", "w", stdout);
			printf("0CC-FamiTracker v%s\n", Get0CCFTVersionString());		// // //
			return;
		}
	}
	else {
		// Store NSF name, then log filename
		if (m_bExport) {
			if (m_strExportFile.IsEmpty()) {
				m_strExportFile = CStringW(pszParam);
				return;
			}
			else if (m_strExportLogFile.IsEmpty()) {
				m_strExportLogFile = CStringW(pszParam);
				return;
			}
			else if (m_strExportDPCMFile.IsEmpty()) {
				// BIN export takes another file paramter for DPCM
				m_strExportDPCMFile = CStringW(pszParam);
				return;
			}
		}
		else if (m_bRender) {		// // //
			if (m_strExportFile.IsEmpty()) {
				m_strExportFile = CStringW(pszParam);
				return;
			}
			else if (track_ == MAX_TRACKS) {
				CStringW param = pszParam;
				WCHAR *str_end = const_cast<LPTSTR>(pszParam) + param.GetLength();
				WCHAR *str_end2;
				track_ = wcstoul(pszParam, &str_end2, 10);
				if (errno || str_end2 != str_end)
					m_bRender = false;
				if (track_ >= MAX_TRACKS)
					track_ = 0;
				return;
			}
			else {
				CStringW param = pszParam;
				if (param.Right(1) == L"s") {
					param.Delete(param.GetLength() - 1);
					render_type_ = render_type_t::Seconds;
				}
				else
					render_type_ = render_type_t::Loops;
				WCHAR *str_end = const_cast<LPTSTR>(pszParam) + param.GetLength();
				WCHAR *str_end2;
				render_param_ = wcstoul(pszParam, &str_end2, 10);
				if (errno || str_end2 != str_end)
					m_bRender = false;
				return;
			}
		}
	}

	// Call default implementation
	CCommandLineInfo::ParseParam(pszParam, bFlag, bLast);
}

//
// CDocTemplate0CC class
//

// copied from http://support.microsoft.com/en-us/kb/141921

CDocTemplate0CC::CDocTemplate0CC(UINT nIDResource, CRuntimeClass *pDocClass, CRuntimeClass *pFrameClass, CRuntimeClass *pViewClass) :
	CSingleDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass)
{
}

BOOL CDocTemplate0CC::GetDocString(CStringW &rString, DocStringIndex i) const
{
	CStringW strTemp, strLeft, strRight;
	int nFindPos;
	AfxExtractSubString(strTemp, m_strDocStrings, (int)i);
	if (i == CDocTemplate::filterExt) {
		nFindPos = strTemp.Find(';');
		if (-1 != nFindPos) {
			//string contains two extensions
			strLeft = strTemp.Left(nFindPos + 1);
			strRight = strTemp.Right(strTemp.GetLength() - nFindPos - 1);
			strTemp = strLeft + strRight;
		}
	}
	rString = strTemp;
	return TRUE;
}

CDocTemplate::Confidence CDocTemplate0CC::MatchDocType(LPCWSTR lpszPathName, CDocument *&rpDocMatch)
{
	ASSERT(lpszPathName != NULL);
	rpDocMatch = NULL;

	// go through all documents
	POSITION pos = GetFirstDocPosition();
	while (pos != NULL) {
		CDocument* pDoc = GetNextDoc(pos);
		if (pDoc->GetPathName() == lpszPathName) {
		   // already open
			rpDocMatch = pDoc;
			return yesAlreadyOpen;
		}
	}  // end while

	// // // see if it matches one of the suffixes
	CStringW strFilterExt;
	if (GetDocString(strFilterExt, CDocTemplate::filterExt) && !strFilterExt.IsEmpty()) {
		 // see if extension matches
		int nDot = CStringW(lpszPathName).ReverseFind('.');
		int curPos = 0;
		CStringW tok = strFilterExt.Tokenize(L";", curPos);
		while (!tok.IsEmpty()) {
			ASSERT(tok[0] == '.');
			if (nDot >= 0 && lstrcmpiW(lpszPathName + nDot, tok) == 0)
				return yesAttemptNative; // extension matches
			tok = strFilterExt.Tokenize(L";", curPos);
		}
	}
	return yesAttemptForeign; //unknown document type
}

//
// CDocManager0CC class
//

BOOL CDocManager0CC::DoPromptFileName(CStringW &fileName, UINT nIDSTitle, DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate *pTemplate)
{
	// Copied from MFC
	// // // disregard doc template
	CStringW path = theApp.GetSettings()->GetPath(PATH_FTM) + L"\\";

	CFileDialog OpenFileDlg(bOpenFileDialog, L"0cc", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
							L"0CC-FamiTracker modules (*.0cc;*.ftm)|*.0cc; *.ftm|All files (*.*)|*.*||",		// // //
							AfxGetMainWnd(), 0);
	OpenFileDlg.m_ofn.Flags |= lFlags;
	OpenFileDlg.m_ofn.lpstrFile = fileName.GetBuffer(_MAX_PATH);
	OpenFileDlg.m_ofn.lpstrInitialDir = path.GetBuffer(_MAX_PATH);
	CStringW title;
	ENSURE(title.LoadStringW(nIDSTitle));
	OpenFileDlg.m_ofn.lpstrTitle = title;
	INT_PTR nResult = OpenFileDlg.DoModal();
	fileName.ReleaseBuffer();
	path.ReleaseBuffer();

	if (nResult == IDOK) {
		theApp.GetSettings()->SetPath(fileName, PATH_FTM);
		return true;
	}
	return false;
}
