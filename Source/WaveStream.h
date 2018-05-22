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

#include <cstdint>
#include <memory>
#include <type_traits>
#include <algorithm>
#include <cmath>
#include "ft0cc/cpputil/array_view.hpp"
#include "SimpleFile.h"

namespace details {

#define REQUIRES_SignedInteger(T) \
	, std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>, int> = 0
#define REQUIRES_UnsignedInteger(T) \
	, std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>, int> = 0
#define REQUIRES_FloatingPoint(T) \
	, std::enable_if_t<std::is_floating_point_v<T>, int> = 0

template <typename T, typename U REQUIRES_UnsignedInteger(T) REQUIRES_UnsignedInteger(U)>
constexpr T convert_sample(U x, unsigned sigbits) noexcept {
	unsigned s2 = std::min(sigbits, static_cast<unsigned>(
		std::min(sizeof(U), sizeof(T)) * 8));
	return (x >> (sizeof(U) * 8 - s2)) << (sizeof(T) * 8 - s2);
}

template <typename T, typename U REQUIRES_SignedInteger(T) REQUIRES_UnsignedInteger(U)>
constexpr T convert_sample(U x, unsigned sigbits) noexcept {
	using T2 = std::make_unsigned_t<T>;
	auto y = convert_sample<T2>(x, sigbits);
	return static_cast<T>(y ^ static_cast<T2>(1u << (sizeof(T) * 8 - 1)));
}

template <typename T, typename U REQUIRES_SignedInteger(T) REQUIRES_SignedInteger(U)>
constexpr T convert_sample(U x, unsigned sigbits) noexcept {
	return static_cast<T>(convert_sample<std::make_unsigned_t<T>>(
		static_cast<std::make_unsigned_t<U>>(x), sigbits));
}

template <typename T, typename U REQUIRES_UnsignedInteger(T) REQUIRES_SignedInteger(U)>
constexpr T convert_sample(U x, unsigned sigbits) noexcept {
	return static_cast<T>(convert_sample<std::make_signed_t<T>>(
		static_cast<std::make_unsigned_t<U>>(x), sigbits));
}

template <typename T, typename U REQUIRES_SignedInteger(T) REQUIRES_FloatingPoint(U)>
T convert_sample(U x, unsigned sigbits) noexcept {
	auto factor = static_cast<U>(std::exp2(sigbits - 1));
	return (1u << (sizeof(T) * 8 - sigbits)) * static_cast<T>(std::floor(
		std::clamp(x * factor, -factor, factor - static_cast<U>(1))));
}

template <typename T, typename U REQUIRES_UnsignedInteger(T) REQUIRES_FloatingPoint(U)>
T convert_sample(U x, unsigned sigbits) {
	return convert_sample<T>(convert_sample<std::make_signed_t<T>>(x, sigbits),
		sigbits);
}

template <typename T, typename U REQUIRES_FloatingPoint(T) REQUIRES_SignedInteger(U)>
T convert_sample(U x, unsigned sigbits) {
	auto s2 = std::min(sigbits, static_cast<unsigned>(sizeof(U)) * 8);
	return std::floor(static_cast<T>(x) / static_cast<T>(
		std::exp2(sizeof(U) * 8 - s2))) / static_cast<T>(std::exp2(s2 - 1));
}

template <typename T, typename U REQUIRES_FloatingPoint(T) REQUIRES_UnsignedInteger(U)>
T convert_sample(U x, unsigned sigbits) {
	return convert_sample<T>(convert_sample<std::make_signed_t<U>>(x, sigbits),
		sigbits);
}

template <typename T, typename U REQUIRES_FloatingPoint(T) REQUIRES_FloatingPoint(U)>
T convert_sample(U x, unsigned /*sigbits*/) {
	return static_cast<T>(x);
}

} // namespace details

struct CWaveFileFormat {
	enum class format_code : std::uint16_t {
		pcm = 0x0001u,
		ieee_float = 0x0003u,
	};

	format_code Format = format_code::pcm;
	std::uint16_t Channels = 1u;
	std::uint32_t SampleRate = 44100u;
	std::uint16_t SampleSize = 16u;

	constexpr std::uint16_t BytesPerSample() const noexcept {
		return SampleSize / 8 + ((SampleSize & 0x0007u) ? 1 : 0);
	}
};

class CInputWaveStream {
public:
	explicit CInputWaveStream(CSimpleFile &file);

private:
	CSimpleFile &file_;
};

class COutputWaveStream {
public:
	COutputWaveStream(std::shared_ptr<CSimpleFile> file, const CWaveFileFormat &fmt);
	~COutputWaveStream() noexcept;

	void WriteWAVHeader();

	template <typename T>
	void WriteSample(T sample) {
		WriteSamples(array_view<const T> {&sample, 1});
	}

	template <typename T>
	void WriteSamples(array_view<const T> samples) {
		for (T x : samples) {
			switch (fmt_.Format) {
			case CWaveFileFormat::format_code::pcm:
				if (fmt_.SampleSize <= 8)
					DoWriteSample(details::convert_sample<std::uint8_t>(x, fmt_.SampleSize));
				else if (fmt_.SampleSize <= 16)
					DoWriteSample(details::convert_sample<std::int16_t>(x, fmt_.SampleSize));
				else if (fmt_.SampleSize <= 32)
					DoWriteSample(details::convert_sample<std::int32_t>(x, fmt_.SampleSize));
//				else if (fmt_.SampleSize <= 64)
//					DoWriteSample(details::convert_sample<std::int64_t>(x, fmt_.SampleSize));
				break;
			case CWaveFileFormat::format_code::ieee_float:
				if (fmt_.SampleSize == 32)
					DoWriteSample(details::convert_sample<float>(x, fmt_.SampleSize));
				else if (fmt_.SampleSize == 64)
					DoWriteSample(details::convert_sample<double>(x, fmt_.SampleSize));
//				else if (fmt_.SampleSize == 80)
//					DoWriteSample(details::convert_sample<long double>(x, fmt_.SampleSize));
				break;
			default:
				return;
			}
		}

		write_count_ += fmt_.BytesPerSample() * samples.size();
		sample_count_ += samples.size();
	}

private:
	template <typename T>
	void DoWriteSample(T x) {
		if constexpr (std::is_same_v<T, float>) {
			static_assert(sizeof(float) == sizeof(std::int32_t));
			union {
				float f;
				std::int32_t i;
			} u = {x};
			return DoWriteSample(u.i);
		}
		else if constexpr (std::is_same_v<T, double>) {
			static_assert(sizeof(double) == sizeof(std::int64_t));
			union {
				double f;
				std::int64_t i;
			} u = {x};
			return DoWriteSample(u.i);
		}
		else if constexpr (std::is_integral_v<T>) {
			if constexpr (sizeof(T) == sizeof(std::uint8_t))
				file_->WriteInt8(x);
			else if constexpr (sizeof(T) == sizeof(std::int16_t))
				file_->WriteInt16(x);
			else if constexpr (sizeof(T) == sizeof(std::int32_t))
				file_->WriteInt32(x);
			else {
				auto x2 = static_cast<std::make_unsigned_t<T>>(x) >> (8 * (sizeof(T) - fmt_.BytesPerSample()));
				for (std::size_t i = 0; i < fmt_.BytesPerSample(); ++i) {
					file_->WriteInt8(static_cast<std::uint8_t>(x2 & 0xFFu));
					x2 >>= 8;
				}
			}
		}
		else
			file_->WriteBytes(byte_view(x));
	}

	std::shared_ptr<CSimpleFile> file_;
	CWaveFileFormat fmt_;
	std::size_t start_pos_;
	std::size_t write_count_ = 0u;
	std::size_t sample_count_ = 0u;
};
