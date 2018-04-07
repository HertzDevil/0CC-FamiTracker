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

// // // File load / store

CSimpleFile::CSimpleFile(const wchar_t *fname, std::ios_base::openmode mode) :
	m_fFile(fname, mode)
{
}

CSimpleFile::operator bool() const
{
	return m_fFile.is_open() && (bool)m_fFile;
}

void CSimpleFile::Close()
{
	m_fFile.close();
}

void CSimpleFile::WriteInt8(int8_t Value)
{
	m_fFile.put(Value);
}

void CSimpleFile::WriteInt16(int16_t Value)
{
	m_fFile.put(static_cast<unsigned char>(Value));
	m_fFile.put(static_cast<unsigned char>(Value >> 8));
}

void CSimpleFile::WriteInt32(int32_t Value)
{
	m_fFile.put(static_cast<unsigned char>(Value));
	m_fFile.put(static_cast<unsigned char>(Value >> 8));
	m_fFile.put(static_cast<unsigned char>(Value >> 16));
	m_fFile.put(static_cast<unsigned char>(Value >> 24));
}

void CSimpleFile::WriteBytes(array_view<char> Buf)
{
	m_fFile.write(reinterpret_cast<const char *>(Buf.data()), Buf.size());
}

void CSimpleFile::WriteBytes(array_view<unsigned char> Buf)
{
	m_fFile.write(reinterpret_cast<const char *>(Buf.data()), Buf.size());
}

void CSimpleFile::WriteBytes(array_view<std::byte> Buf)
{
	m_fFile.write(reinterpret_cast<const char *>(Buf.data()), Buf.size());
}

void CSimpleFile::WriteString(std::string_view sv)
{
	int Len = sv.size();
	WriteInt32(Len);
	WriteBytes(sv);
}

void CSimpleFile::WriteStringNull(std::string_view sv)
{
	WriteBytes(sv);
	m_fFile.put('\0');
}

uint8_t CSimpleFile::ReadUint8()
{
	unsigned char buf[1] = { };
	ReadBytes(buf, std::size(buf));
	return buf[0];
}

int8_t CSimpleFile::ReadInt8()
{
	return static_cast<int8_t>(ReadUint8());
}

uint16_t CSimpleFile::ReadUint16()
{
	unsigned char buf[2] = { };
	ReadBytes(buf, std::size(buf));
	return buf[0] | (buf[1] << 8);
}

int16_t CSimpleFile::ReadInt16()
{
	return static_cast<int16_t>(ReadUint16());
}

uint32_t CSimpleFile::ReadUint32()
{
	unsigned char buf[4] = { };
	ReadBytes(buf, std::size(buf));
	return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

int32_t CSimpleFile::ReadInt32()
{
	return static_cast<int32_t>(ReadUint32());
}

void CSimpleFile::ReadBytes(void *pBuf, size_t count) {
	m_fFile.read(reinterpret_cast<char *>(pBuf), count);
}

std::string CSimpleFile::ReadString()
{
	const auto Size = ReadUint32();
	std::string str(Size, '\0');
	m_fFile.read(str.data(), Size);
	return str;
}

std::string CSimpleFile::ReadStringN(size_t count) {
	char buf[1024] = { };
	std::string str;

	while (count) {
		size_t readCount = count > std::size(buf) ? std::size(buf) : count;
		m_fFile.read(buf, readCount);
		str.append(buf, readCount);
		count -= readCount;
	}

	return str;
}

std::string CSimpleFile::ReadStringNull()
{
	std::string str;
	while (true) {
		char ch = m_fFile.get();
		if (!ch || !m_fFile)
			break;
		str += ch;
	}
	return str;
}

void CSimpleFile::Seek(std::size_t pos) {
	m_fFile.seekp(pos);
}

std::size_t CSimpleFile::GetPosition() {
	return m_fFile.tellp();
}
