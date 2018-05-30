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

#include <cstddef>		// // //
#include <string>
#include <string_view>		// // //
#include <array>		// // //
#include <vector>		// // //
#include <memory>		// // //
#include "BinaryStream.h"		// // //
#include "ModuleException.h"		// // //
#include "ft0cc/cpputil/array_view.hpp"		// // //
#include "ft0cc/cpputil/fs.hpp"		// // //

class CBinaryFileStream;
class CDocumentInputBlock;

class CDocumentFile {
public:
	CDocumentFile();
	~CDocumentFile();		// // //

	// // // delegations to CBinaryFileStream
	CBinaryFileStream &GetBinaryStream();
	void Open(const fs::path &fname);		// // //
	void Close();

	bool Finished() const;

	// Read functions
	void ValidateFile();		// // //
	unsigned GetFileVersion() const;

	std::unique_ptr<CDocumentInputBlock> ReadBlock();		// // //

	bool IsFileIncomplete() const;

	// // // exception
	void SetDefaultFooter(const CDocumentInputBlock &block, CModuleException &e) const;
	[[noreturn]] void RaiseModuleException(const CDocumentInputBlock &block, const std::string &Msg) const;

public:
	// Constants
	static const unsigned int FILE_VER;
	static const unsigned int COMPATIBLE_VER;

	static constexpr std::string_view FILE_HEADER_ID = "FamiTracker Module";		// // //
	static constexpr std::string_view FILE_END_ID = "END";

	static const unsigned int BLOCK_HEADER_SIZE = 16;		// // //

protected:
	std::unique_ptr<CBinaryFileStream> m_pFile;		// // //

	unsigned int	m_iFileVersion;
	std::uintmax_t	m_iPrevFilePosition = 0u;
	bool			m_bFileDone = false;
};



class CDocumentInputBlock : public CBinaryReader {
public:
	explicit CDocumentInputBlock(const CDocumentFile &parent, std::string_view id, unsigned ver, std::vector<std::byte> data);

	unsigned GetFileVersion() const;
	unsigned GetBlockVersion() const;
	std::string_view GetBlockHeaderID() const;

	std::uintmax_t GetPreviousPosition() const;
	bool BlockDone() const;

	std::size_t ReadBytes(array_view<std::byte> buf) override;

	void AdvancePointer(int offset);

	template <module_error_level_t l = MODULE_ERROR_DEFAULT>
	void AssertFileData(bool Cond, const std::string &Msg, module_error_level_t err_lv) const {
		if (l <= err_lv && !Cond)
			parent_.RaiseModuleException(*this, Msg);
	}

	template <module_error_level_t l = MODULE_ERROR_DEFAULT, typename T, typename U, typename V>
	T AssertRange(T Value, U Min, V Max, const std::string &Desc, module_error_level_t err_lv) const {
		if (l <= err_lv && !(Value >= Min && Value <= Max))
			parent_.RaiseModuleException(*this, Desc + " out of range: expected ["
				+ std::to_string(Min) + ','
				+ std::to_string(Max) + "], got "
				+ std::to_string(Value));
		return Value;
	}

private:
	void BeforeRead() override;
	void AfterRead() override;

	const CDocumentFile &parent_;

	std::string m_sBlockID;
	unsigned m_iBlockVersion = 0u;
	std::vector<std::byte> m_pBlockData;

	std::uintmax_t m_iBlockPointer = 0u;
	std::uintmax_t m_iPreviousPointer = 0u;		// // //
	std::uintmax_t m_iBlockPointerCache = 0u;		// // //
};



class CDocumentOutputBlock : public CBinaryWriter {
public:
	CDocumentOutputBlock(std::string_view id, unsigned ver);

	unsigned GetBlockVersion() const;

	std::size_t WriteBytes(array_view<const std::byte> Data) override;

	bool FlushToFile(CBinaryWriter &file) const;

private:
	std::array<char, CDocumentFile::BLOCK_HEADER_SIZE> m_cBlockID = { };
	unsigned m_iBlockVersion = 0u;
	std::vector<std::byte> m_pBlockData;
};
