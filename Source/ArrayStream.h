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

#include "BinaryStream.h"

class CConstArrayStream : public CBinaryReader {
public:
	CConstArrayStream(array_view<const std::byte> input);

	std::size_t ReadBytes(array_view<std::byte> buf) override;

private:
	array_view<const std::byte> input_;
	array_view<const std::byte> orig_input_;
};

class CArrayStream : public CBinaryWriter {
public:
	CArrayStream(array_view<std::byte> output);

	std::size_t WriteBytes(array_view<const std::byte> buf) override;

private:
	array_view<std::byte> output_;
	array_view<std::byte> orig_output_;
};
