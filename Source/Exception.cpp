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

#include "Exception.h"
#include "stdafx.h"
#include <Dbghelp.h>
#include "version.h"
#include "str_conv/str_conv.hpp"		// // //

//
// This file contains an unhandled exception handler
// It will dump a memory file and save the current module to a new file
//

// This won't be called when running with a debugger attached

// Todo: Recover files should not be created if the crash occurred
// during the file save operation, this could be fixed with a flag.

const WCHAR FTM_DUMP[] = L"recover";
const WCHAR MINIDUMP_FILE_PRE[] = L"MiniDump";
const WCHAR MINIDUMP_FILE_END[] = L".dmp";

//#ifdef ENABLE_CRASH_HANDLER

static CStringW GetDumpFilename(int counter)
{
	// Append a timestamp to the filename
	//
	CStringW filename;
	CTime t = CTime::GetTickCount();

	filename = MINIDUMP_FILE_PRE;

	// Date
	AppendFormatW(filename, L"_%02i%02i%02i-%02i%02i", t.GetYear(), t.GetMonth(), t.GetDay(), t.GetHour(), t.GetMinute());

	// App version
	AppendFormatW(filename, L"-v%s", conv::to_wide(GetDumpVersionString()).data());		// // //
#ifdef WIP
	filename.Append(L"_beta");
#endif

	// Counter
	if (counter > 0)
		AppendFormatW(filename, L"(%i)", counter);

	filename.Append(MINIDUMP_FILE_END);

	return filename;
}

static LONG WINAPI ExceptionHandler(__in struct _EXCEPTION_POINTERS *ep)
{
	static BOOL HasDumped = FALSE;

	CStringW MinidumpFile;
	int dump_counter = 0;

	// Prevent multiple calls to this exception handler
	if (HasDumped == TRUE)
		ExitProcess(0);

	HasDumped = TRUE;

	MinidumpFile = GetDumpFilename(dump_counter++);

	while (GetFileAttributesW(MinidumpFile) != 0xFFFFFFFF)
		MinidumpFile = GetDumpFilename(dump_counter++);

	HANDLE hFile = CreateFileW(MinidumpFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// Save the memory dump file
	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))  {
		// Create the minidump
		MINIDUMP_EXCEPTION_INFORMATION mdei;
		mdei.ThreadId			= GetCurrentThreadId();
		mdei.ExceptionPointers	= ep;
		mdei.ClientPointers		= FALSE;

		HANDLE hProcess	  = GetCurrentProcess();
		DWORD dwProcessId = GetCurrentProcessId();

		MiniDumpWriteDump(hProcess, dwProcessId, hFile, MiniDumpNormal, (ep != 0) ? &mdei : NULL, 0, 0);

		CloseHandle(hFile);
	}

	// Find a free filename.
	// Start with "recover" and append a number if file exists.
	CStringW DocDumpFile = FTM_DUMP;
	int counter = 1;

	while (GetFileAttributesW(DocDumpFile + L".ftm") != 0xFFFFFFFF)
		DocDumpFile = FormattedW(L"%s%i", FTM_DUMP, counter++);

	DocDumpFile.Append(L".ftm");

	// Display a message
	CStringW text = L"This application has encountered a problem and needs to close.\n\n";
	if (ep->ExceptionRecord->ExceptionCode == 0xE06D7363)		// // //
		AppendFormatW(text, L"Unhandled C++ exception:\" %s\".\n\n", conv::to_wide(
			reinterpret_cast<const std::exception *>(ep->ExceptionRecord->ExceptionInformation[1])->what()).data());
	else
		AppendFormatW(text, L"Unhandled exception %X.\n\n", ep->ExceptionRecord->ExceptionCode);
	AppendFormatW(text, L"A memory dump file has been created (%s), please include this if you file a bug report!\n\n", LPCWSTR(MinidumpFile));
	AppendFormatW(text, L"Attempting to save current module as %s.", LPCWSTR(DocDumpFile));
//	text.Append("Application will now close.");
	AfxMessageBox(text, MB_ICONSTOP);

	// Try to save the document
	CFrameWnd *pFrameWnd = NULL;
	CDocument *pDoc = NULL;
	CWinApp *pApp = AfxGetApp();

	if (pApp != NULL)
		pFrameWnd = (CFrameWnd*)pApp->m_pMainWnd;

	if (pFrameWnd != NULL)
		pDoc = pFrameWnd->GetActiveDocument();

	if (pDoc != NULL)
		pDoc->OnSaveDocument(DocDumpFile);

	// Exit this process
	ExitProcess(0);

	// (never called)
	return EXCEPTION_CONTINUE_SEARCH;
}

void InstallExceptionHandler()
{
	SetUnhandledExceptionFilter(ExceptionHandler);
}

void UninstallExceptionHandler()
{
	SetUnhandledExceptionFilter(NULL);
}

//#endif

