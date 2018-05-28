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

#include <fstream>
#include <cstdint>
#include <cstddef>
#include "ft0cc/cpputil/array_view.hpp"
#include "ft0cc/cpputil/fs.hpp"
#include "BinaryReader.h"

class CSimpleFile : public CBinaryReader {
public:
	static_assert(sizeof(char) == sizeof(uint8_t));

	CSimpleFile() = default;
	CSimpleFile(const fs::path &fname, std::ios_base::openmode mode);

	explicit operator bool() const;

	void	Open(const fs::path &fname, std::ios_base::openmode mode);
	void	Close();
	std::string	GetErrorMessage() const;

	void	WriteInt8(int8_t Value);
	void	WriteInt16(int16_t Value);
	void	WriteInt32(int32_t Value);
	void	WriteBytes(array_view<const std::byte> Buf);
	void	WriteString(std::string_view sv);
	void	WriteStringNull(std::string_view sv);

	std::size_t	ReadBytes(array_view<std::byte> Buf) override;

	void		Seek(std::size_t pos);
	std::size_t GetPosition();

private:
	std::fstream m_fFile;
};
