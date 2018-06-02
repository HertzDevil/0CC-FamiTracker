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

#include "WaveStream.h"
#include "ft0cc/cpputil/enum_traits.hpp"
#include "Assertion.h"
#include <cmath>

namespace {

constexpr std::uint32_t fourcc(const char (&str)[5]) noexcept {
	return str[0] | (str[1] << 8) | (str[2] << 16) | (str[3] << 24);
}

} // namespace

CInputWaveStream::CInputWaveStream(std::shared_ptr<CBinaryReader> file) : file_(std::move(file)) {
}



COutputWaveStream::COutputWaveStream(std::shared_ptr<CBinaryWriter> file, const CWaveFileFormat &fmt) :
	file_(std::move(file)), fmt_(fmt), start_pos_(file_->GetWriterPos())
{
	Assert(fmt.Format == CWaveFileFormat::format_code::pcm || fmt.Format == CWaveFileFormat::format_code::ieee_float);
}

COutputWaveStream::~COutputWaveStream() noexcept {
	while (sample_count_ % fmt_.Channels)
		WriteSample(0);

	if (write_count_ % 2) {
		file_->WriteInt<std::uint8_t>(0);
		++write_count_;
	}

	file_->SeekWriter(start_pos_ + (fmt_.Format != CWaveFileFormat::format_code::pcm ? 54 : 40));
	file_->WriteInt<std::uint32_t>(write_count_ - (fmt_.Format != CWaveFileFormat::format_code::pcm ? 50 : 36));

	file_->SeekWriter(start_pos_ + 4u);
	file_->WriteInt<std::uint32_t>(write_count_);
}

void COutputWaveStream::WriteWAVHeader() {
	file_->WriteInt<std::uint32_t>(fourcc("RIFF"));
	file_->WriteInt<std::uint32_t>(0); // to be written later

	file_->WriteInt<std::uint32_t>(fourcc("WAVE"));
	file_->WriteInt<std::uint32_t>(fourcc("fmt "));
	file_->WriteInt<std::uint32_t>(fmt_.Format != CWaveFileFormat::format_code::pcm ? 18 : 16);
	file_->WriteInt<std::uint16_t>(value_cast(fmt_.Format)); // wFormatTag
	file_->WriteInt<std::uint16_t>(fmt_.Channels); // nChannels
	file_->WriteInt<std::uint32_t>(fmt_.SampleRate); // nSamplesPerSec
	file_->WriteInt<std::uint32_t>(fmt_.SampleRate * fmt_.Channels * fmt_.BytesPerSample()); // nAvgBytesPerSec
	file_->WriteInt<std::uint16_t>(fmt_.Channels * fmt_.BytesPerSample()); // nBlockAlign
	file_->WriteInt<std::uint16_t>(fmt_.SampleSize); // wBitsPerSample
	write_count_ += 28;

	if (fmt_.Format != CWaveFileFormat::format_code::pcm) {
		file_->WriteInt<std::uint16_t>(0); // cbSize

		file_->WriteInt<std::uint32_t>(fourcc("fact"));
		file_->WriteInt<std::uint32_t>(4);
		file_->WriteInt<std::uint32_t>(0); // dwSampleLength, to be written later

		write_count_ += 14;
	}

	file_->WriteInt<std::uint32_t>(fourcc("data"));
	file_->WriteInt<std::uint32_t>(0); // to be written later
	write_count_ += 8;
}
