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

#include "SimpleFile.h"
#include <string.h>

// // // File load / store

CSimpleFile::CSimpleFile(const fs::path &fname, std::ios_base::openmode mode) :
	m_fFile(fname, mode)
{
}

CSimpleFile::operator bool() const
{
	return m_fFile.is_open() && (bool)m_fFile;
}

void CSimpleFile::Open(const fs::path &fname, std::ios_base::openmode mode) {
	m_fFile.open(fname, mode);
	if (!m_fFile)
		throw std::runtime_error {GetErrorMessage()};
}

void CSimpleFile::Close()
{
	if (m_fFile.is_open()) {
		m_fFile.flush();
		m_fFile.close();
	}
}

std::string CSimpleFile::GetErrorMessage() const {
	if (m_fFile)
		return "";

	char err[512] = { };
#ifdef _MSC_VER
	if (0 == ::strerror_s(err, errno))
#else
	if (0 == ::strerror_r(errno, err, std::size(err)))
#endif
		return err;

	return "Unknown error";
}

std::size_t CSimpleFile::WriteBytes(array_view<const std::byte> Buf) {
	m_fFile.write(reinterpret_cast<const char *>(Buf.data()), Buf.size());
	return m_fFile ? Buf.size() : 0u;
}

std::size_t CSimpleFile::ReadBytes(array_view<std::byte> Buf) {
	m_fFile.read(reinterpret_cast<char *>(Buf.data()), Buf.size());
	return m_fFile.gcount();
}

void CSimpleFile::Seek(std::size_t pos) {
	m_fFile.seekp(pos);
}

std::size_t CSimpleFile::GetPosition() {
	return m_fFile.tellp();
}
