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

#if defined(_DEBUG) || !defined(NDEBUG)
#  define FT0CC_DEBUG 1
#else
#  define FT0CC_DEBUG 0
#endif

#ifndef DEBUG_BREAK
#  if FT0CC_DEBUG
#    ifdef _MSC_VER
#      define DEBUG_BREAK() (__debugbreak())
#    else
#      include <csignal>
#      define DEBUG_BREAK() (raise(SIGTRAP))
#    endif
#  else
#    define DEBUG_BREAK() ((void)0)
#  endif
#endif

#ifndef Assert
#  if FT0CC_DEBUG
#    define Assert(b) do { if (!b) DEBUG_BREAK(); } while (false)
#  else
#    define Assert(b) ((void)0)
#  endif
#endif
