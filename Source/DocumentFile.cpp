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

#define _SCL_SECURE_NO_WARNINGS
#include "DocumentFile.h"
#include "BinaryFileStream.h"
#include "ModuleException.h"
#include "ft0cc/cpputil/array_view.hpp"
#include "NumConv.h"
#include <iterator>		// // //
#include <algorithm>		// // //
#include "Assertion.h"		// // //

//
// This class is based on CBinaryFileStream and has some simple extensions to create and read FTM files
//

// Class constants
const unsigned int CDocumentFile::FILE_VER		 = 0x0440;			// Current file version (4.40)
const unsigned int CDocumentFile::COMPATIBLE_VER = 0x0100;			// Compatible file version (1.0)

//const std::string_view CDocumentFile::FILE_HEADER_ID = {"FamiTracker Module", 18};		// // //
//const std::string_view CDocumentFile::FILE_END_ID = "END";

const unsigned int CDocumentFile::BLOCK_SIZE = 0x10000;

CDocumentFile::CDocumentFile() :
	m_pFile(std::make_unique<CBinaryFileStream>())
{
}

CDocumentFile::~CDocumentFile() {
	Close();
}

// // // delegations to CBinaryFileStream

CBinaryFileStream &CDocumentFile::GetBinaryStream() {
	return *m_pFile;
}

void CDocumentFile::Open(const fs::path &fname) {		// // //
	m_pFile->Open(fname, std::ios::in | std::ios::binary);
}

void CDocumentFile::Close() {
	m_pFile->Close();
}

// CDocumentFile

bool CDocumentFile::Finished() const
{
	return m_bFileDone;
}

void CDocumentFile::ValidateFile()
{
	// Checks if loaded file is valid

	try {
		// Check ident string
		if (m_pFile->ReadStringN<char>(FILE_HEADER_ID.size()) != FILE_HEADER_ID)
			throw CModuleException::WithMessage("File is not a FamiTracker module");

		// Read file version
		m_iFileVersion = m_pFile->ReadInt<std::uint32_t>() & 0xFFFFu;		// // //
	}
	catch (CBinaryIOException &) {
		throw CModuleException::WithMessage("File is not a FamiTracker module");
	}

	// // // Older file version
	if (GetFileVersion() < COMPATIBLE_VER)
		throw CModuleException::WithMessage("FamiTracker module version too old (0x" + conv::from_int_hex(GetFileVersion()) +
			"), expected 0x" + conv::from_int_hex(COMPATIBLE_VER) + " or above");

	// // // File version is too new
	if (GetFileVersion() > 0x450u /*FILE_VER*/)		// // // 050B
		throw CModuleException::WithMessage("FamiTracker module version too new (0x" + conv::from_int_hex(GetFileVersion()) +
			"), expected 0x" + conv::from_int_hex(0x450u) + " or below");

	m_bFileDone = false;
}

unsigned int CDocumentFile::GetFileVersion() const {
	return m_iFileVersion;
}

std::unique_ptr<CDocumentInputBlock> CDocumentFile::ReadBlock() {		// // //
	m_iPrevFilePosition = m_pFile->GetPosition();

	std::array<char, BLOCK_HEADER_SIZE> buf = { };
	std::string_view id {buf.data(), m_pFile->ReadBytes(byte_view(buf))};
	if (id.size() < BLOCK_HEADER_SIZE) {
		if (id == FILE_END_ID)
			m_bFileDone = true;
		return nullptr;
	}
	id = id.substr(0, id.find('\0'));

	try {
		std::uint32_t BlockVersion = m_pFile->ReadInt<std::uint32_t>();
		std::size_t Size = m_pFile->ReadInt<std::uint32_t>();
		if (Size > 50000000u) { // File is probably corrupt
			m_pFile->Seek(m_pFile->GetPosition() + Size);
			return nullptr;
		}

		m_iPrevFilePosition = m_pFile->GetPosition();
		auto BlockData = std::vector<std::byte>(Size);		// // //
		if (m_pFile->ReadBytes(byte_view(BlockData)) != Size)
			return nullptr;

		return std::make_unique<CDocumentInputBlock>(*this, id, BlockVersion, std::move(BlockData)); // temp
	}
	catch (CBinaryIOException &) {
		return nullptr;
	}
}

void CDocumentFile::SetDefaultFooter(const CDocumentInputBlock &block, CModuleException &e) const		// // //
{
	std::string msg = "At address 0x" + conv::from_int_hex(block.GetPreviousPosition()) +
		" in " + std::string {block.GetBlockHeaderID()} + " block,\n"
		"address 0x" + conv::from_int_hex(block.GetPreviousPosition() + m_iPrevFilePosition) + " in file";		// // //
	e.SetFooter(std::move(msg));
}

void CDocumentFile::RaiseModuleException(const CDocumentInputBlock &block, const std::string &Msg) const		// // //
{
	auto e = CModuleException::WithMessage(Msg);
	SetDefaultFooter(block, e);
	throw e;
}



CDocumentInputBlock::CDocumentInputBlock(const CDocumentFile &parent, std::string_view id, unsigned ver, std::vector<std::byte> data) :
	parent_(parent),
	m_sBlockID(id),
	m_iBlockVersion(ver),
	m_pBlockData(std::move(data))
{
}

unsigned CDocumentInputBlock::GetFileVersion() const {
	return parent_.GetFileVersion();
}

unsigned CDocumentInputBlock::GetBlockVersion() const {
	return m_iBlockVersion;
}

std::string_view CDocumentInputBlock::GetBlockHeaderID() const {
	return m_sBlockID;
}

std::uintmax_t CDocumentInputBlock::GetPreviousPosition() const {
	return m_iPreviousPointer;
}

bool CDocumentInputBlock::BlockDone() const {
	return m_iBlockPointer == m_pBlockData.size();
}

std::size_t CDocumentInputBlock::ReadBytes(array_view<std::byte> Buf) {
	if (m_iBlockPointer + Buf.size() > m_pBlockData.size())
		return 0u;
	std::copy_n(m_pBlockData.data() + m_iBlockPointer, Buf.size(), Buf.begin());
	AdvancePointer(Buf.size());
	return Buf.size();
}

void CDocumentInputBlock::BeforeRead() {
	m_iBlockPointerCache = m_iBlockPointer;
}

void CDocumentInputBlock::AfterRead() {
	m_iPreviousPointer = m_iBlockPointerCache;
}

void CDocumentInputBlock::AdvancePointer(int offset) {
	m_iPreviousPointer = m_iBlockPointer;
	m_iBlockPointer += offset;
}



CDocumentOutputBlock::CDocumentOutputBlock(std::string_view id, unsigned ver) :
	m_iBlockVersion(ver & 0xFFFFu)
{
	Assert(id.size() < CDocumentFile::BLOCK_HEADER_SIZE);
	id = id.substr(0, CDocumentFile::BLOCK_HEADER_SIZE - 1);
	id.copy(m_cBlockID.data(), id.size());
}

unsigned CDocumentOutputBlock::GetBlockVersion() const {
	return m_iBlockVersion;
}

std::size_t CDocumentOutputBlock::WriteBytes(array_view<const std::byte> Data) {
	std::copy(Data.begin(), Data.end(), std::back_inserter(m_pBlockData));
	return Data.size();
}

bool CDocumentOutputBlock::FlushToFile(CBinaryWriter &file) const {
	try {
		if (!m_pBlockData.empty()) {
			file.WriteBytes(byte_view(m_cBlockID));
			file.WriteInt<std::uint32_t>(m_iBlockVersion);
			file.WriteInt<std::uint32_t>(m_pBlockData.size());
			file.WriteBytes(byte_view(m_pBlockData));
		}
		return true;
	}
	catch (CBinaryIOException &) {
		return false;
	}
}
