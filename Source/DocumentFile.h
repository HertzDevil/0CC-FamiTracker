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

#include <string>
#include <array>		// // //
#include <vector>		// // //
#include <memory>		// // //
#include "array_view.h"		// // //
#include <string_view>		// // //
#include <iosfwd>		// // //

// CDocumentFile, class for reading/writing document files

class CSimpleFile;
class CModuleException;

class CDocumentFile {
public:
	CDocumentFile();
	~CDocumentFile();		// // //

	// // // delegations to CSimpleFile
	CSimpleFile	&GetCSimpleFile();
	void		Open(const char *lpszFileName, std::ios::openmode nOpenFlags);		// // //
	void		Close();

	bool		Finished() const;

	// Write functions
	void		BeginDocument();		// // //
	void		EndDocument();

	void		CreateBlock(std::string_view ID, int Version);		// // //
	void		WriteBlock(array_view<unsigned char> Data);		// // //
	void		WriteBlockInt(int Value);
	void		WriteBlockChar(char Value);
	void		WriteString(std::string_view sv);		// // //
	void		WriteStringPadded(std::string_view sv, std::size_t n);		// // //
	void		WriteStringCounted(std::string_view sv);		// // //
	bool		FlushBlock();

	// Read functions
	void		ValidateFile();		// // //
	unsigned int GetFileVersion() const;

	bool		ReadBlock();
	void		GetBlock(void *Buffer, int Size);
	int			GetBlockVersion() const;
	bool		BlockDone() const;
	const char	*GetBlockHeaderID() const;		// // //
	int			GetBlockInt();
	char		GetBlockChar();

	int			GetBlockPos() const;
	int			GetBlockSize() const;

	std::string	ReadString();		// // //

	void		RollbackPointer(int count);	// avoid this

	bool		IsFileIncomplete() const;

	// // // exception
	CModuleException GetException() const;
	void SetDefaultFooter(CModuleException &e) const;
	[[noreturn]] void RaiseModuleException(const std::string &Msg) const;

private:		// // //
	unsigned Read(unsigned char *lpBuf, std::size_t nCount);
	void Write(const unsigned char *lpBuf, std::size_t nCount);

public:
	// Constants
	static const unsigned int FILE_VER;
	static const unsigned int COMPATIBLE_VER;

	static constexpr std::string_view FILE_HEADER_ID = "FamiTracker Module";		// // //
	static constexpr std::string_view FILE_END_ID = "END";

	static const unsigned int MAX_BLOCK_SIZE;
	static const unsigned int BLOCK_SIZE;
	static const unsigned int BLOCK_HEADER_SIZE = 16;		// // //

private:
	template <typename T>
	void WriteBlockData(T Value);

protected:
	void ReallocateBlock();

protected:
	std::unique_ptr<CSimpleFile> m_pFile;		// // //

	unsigned int	m_iFileVersion;
	bool			m_bFileDone;
	bool			m_bIncomplete;

	std::array<char, BLOCK_HEADER_SIZE> m_cBlockID = { };		// // //
	unsigned int	m_iBlockSize;
	unsigned int	m_iBlockVersion;
	std::vector<unsigned char> m_pBlockData;		// // //

	unsigned int	m_iMaxBlockSize;

	unsigned int	m_iBlockPointer;
	unsigned int	m_iPreviousPointer;		// // //
	uintmax_t		m_iFilePosition, m_iPreviousPosition;		// // //
};
