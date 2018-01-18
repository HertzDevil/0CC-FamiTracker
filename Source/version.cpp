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

#include "version.h"

#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)
#define VERSION_STR(SEP) STRINGIFY(VERSION_API) SEP STRINGIFY(VERSION_MAJ) SEP \
	STRINGIFY(VERSION_MIN) SEP STRINGIFY(VERSION_REV)

#if defined(WIP) && __has_include("../commit_hash.h")
#include "../commit_hash.h"
const char *Get0CCFTVersionString() noexcept {
	return VERSION_STR(".") " [" COMMIT_HASH "]";
}

const char *GetDumpVersionString() noexcept {
	return VERSION_STR("_") "_" COMMIT_HASH;
}
#else
const char *Get0CCFTVersionString() noexcept {
	return VERSION_STR(".");
}

const char *GetDumpVersionString() noexcept {
	return VERSION_STR("_");
}
#endif
