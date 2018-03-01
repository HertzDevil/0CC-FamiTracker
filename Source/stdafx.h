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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#define _CRTDBG_MAPALLOC
#define NOMINMAX

// Get rid of warnings in VS 2005
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0501		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0501		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0600	// Change this to the appropriate value to target IE 5.0 or later.
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CStringW constructors will be explicit
#define _CSTRING_DISABLE_NARROW_WIDE_CONVERSION		// // //

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#define NO_WARN_MBCS_MFC_DEPRECATION		// // // MBCS

struct IUnknown;		// // // /permissive-
class CRenderTarget;		// // //

#include <SDKDDKVer.h>		// // //
#include <afx.h>
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxdlgs.h>
#include <afxole.h>        // MFC OLE support
#include <afxmt.h>		// // // For CMutex
#include <type_traits>		// // //

#ifdef TRACE
#undef TRACE
#endif

namespace details {

template <typename T, typename CharT>		// // //
inline constexpr bool is_printf_arg_v = std::is_arithmetic_v<std::decay_t<T>> ||
	std::is_enum_v<std::decay_t<T>> || std::is_convertible_v<T, const CharT *>;

} // namespace details

template <typename... Args>
inline CStringA FormattedA(LPCSTR fmt, Args&&... args) {
	CStringA str;
	static_assert((... && details::is_printf_arg_v<Args, CHAR>), "These types cannot be passed as printf arguments");
	str.Format(fmt, std::forward<Args>(args)...);
	return str;
}

template <typename... Args>
inline CStringW FormattedW(LPCWSTR fmt, Args&&... args) {
	CStringW str;
	static_assert((... && details::is_printf_arg_v<Args, WCHAR>), "These types cannot be passed as printf arguments");
	str.Format(fmt, std::forward<Args>(args)...);
	return str;
}

template <typename... Args>
inline void AppendFormatA(CStringA &str, LPCSTR fmt, Args&&... args) {
	static_assert((... && details::is_printf_arg_v<Args, CHAR>), "These types cannot be passed as printf arguments");
	str.AppendFormat(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void AppendFormatW(CStringW &str, LPCWSTR fmt, Args&&... args) {
	static_assert((... && details::is_printf_arg_v<Args, WCHAR>), "These types cannot be passed as printf arguments");
	str.AppendFormat(fmt, std::forward<Args>(args)...);
}



namespace details {

template <typename... Args>
void debug_trace(LPCWSTR format, Args&&... args) {
	static_assert((... && details::is_printf_arg_v<Args, WCHAR>), "These types cannot be passed as printf arguments");
#ifdef _DEBUG
	OutputDebugStringW(FormattedW(format, std::forward<T>(args)...));
#endif
}

} // namespace details

#ifdef TRACE
#undef TRACE
#endif
#define TRACE details::debug_trace
