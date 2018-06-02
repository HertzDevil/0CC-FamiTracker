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

#include "PCMImporter.h"
#include <MMSystem.h>
#include "../resource.h"
#include "resampler/resample.hpp"
#include "resampler/resample.inl"
#include "ft0cc/doc/dpcm_sample.hpp"
#include "APU/DPCM.h"

namespace {

// Implement a resampler using CRTP idiom
class resampler : public jarh::resample<resampler>
{
	typedef jarh::resample<resampler> base;
public:
	resampler(const jarh::sinc &sinc, float ratio, int channels, int smpsize,
			  size_t nbsamples, CFile &cfile)
	// TODO: cutoff is currently fixed to a value (.9f), make it modifiable.
	 : base(sinc), channels_(channels), smpsize_(smpsize),
	   nbsamples_(nbsamples), remain_(nbsamples), cfile_(cfile)
	{
		init(ratio, .9f);
	}

	bool initstream()
	{
		// Don't seek to the begin of wave chunk, as it is already done.
		// This stream will not be reinitialized, then.
		remain_ = nbsamples_;
		return true;
	}

	float *fill(float *first, float *end)
	{
		int val;
		for(;first != end && remain_ && ReadSample(val); ++first, --remain_)
		{
			*first = (float)val;
		}
		return first;
	}

private:
	bool ReadSample(int &v)
	{
		int ret = 0, nbytes = 0;
		if (smpsize_ == 2) {
			// 16 bit samples
			short sample_word[2];
			if (channels_ == 2) {
				ret = cfile_.Read(sample_word, nbytes = 2*sizeof(short));
				v = (sample_word[0] + sample_word[1]) / 2;
			}
			else {
				ret = cfile_.Read(sample_word, nbytes = sizeof(short));
				v = *sample_word;
			}
		}
		else if (smpsize_ == 1) {
			// 8 bit samples
			unsigned char sample_byte[2];
			if (channels_ == 2) {
				ret = cfile_.Read(sample_byte, nbytes = 2);
				// convert to a proper signed representation
				// shift left only by 7; because we want a mean
				v = ((int)sample_byte[0] + (int)sample_byte[1] - 256) << 7;
			}
			else {
				ret = cfile_.Read(sample_byte, nbytes = 1);
				v = ((int)(*sample_byte) - 128) << 8;
			}
		}
		else if (smpsize_ == 3) {
			// 24 bit samples
	        unsigned char sample_byte[6];
			if (channels_ == 2) {
				ret = cfile_.Read(sample_byte, nbytes = 6);
				v = (*((signed short*)(sample_byte + 1)) + *((signed short*)(sample_byte + 4))) / 2;
			}
			else {
				ret = cfile_.Read(sample_byte, nbytes = 3);
				v = *((signed short*)(sample_byte + 1));
			}
		}
		else if (smpsize_ == 4) {
			// 32 bit samples
	        int sample_word[2];
			if (channels_ == 2) {
				ret = cfile_.Read(sample_word, nbytes = 8);
				v = ((sample_word[0] >> 16) + (sample_word[1] >> 16)) / 2;
			}
			else {
				ret = cfile_.Read(sample_word, nbytes = 4);
				v = sample_word[0] >> 16;
			}
		}

		return ret == nbytes;
	}

	CFile &cfile_;
	int    channels_;
	int    smpsize_;
	size_t nbsamples_;
	size_t remain_;
};

} // namespace

CPCMImporter::CPCMImporter() :
	m_psinc(std::make_unique<jarh::sinc>(512, 32)) // sinc object. TODO: parametrise
{
}

bool CPCMImporter::OpenWaveFile(const fs::path &fname) {
	// Open and read wave file header
	PCMWAVEFORMAT WaveFormat = { };		// // //
	char Header[4] = { };
	bool Scanning = true;
	bool WaveFormatFound = false;
	bool ValidWave = false;
	unsigned int BlockSize;
	unsigned int FileSize;
	CFileException ex;

	m_iWaveSize = 0;
	m_ullSampleStart = 0;

	TRACE(L"DPCM import: Loading wave file %s...\n", fname.c_str());

	if (!m_fSampleFile.Open(fname.c_str(), CFile::modeRead, &ex)) {
		WCHAR szCause[255] = { };
		ex.GetErrorMessage(szCause, std::size(szCause));
		AfxMessageBox(FormattedW(L"Could not open file: %s", szCause));		// // //
		return false;
	}

	m_fSampleFile.Read(Header, 4);

	if (std::memcmp(Header, "RIFF", 4) != 0) {
		// Invalid format
		Scanning = false;
		ValidWave = false;
	}
	else {
		// Read file size
		m_fSampleFile.Read(&FileSize, 4);
	}

	// Now improved, should handle most files
	while (Scanning) {
		if (m_fSampleFile.Read(Header, 4) < 4) {
			Scanning = false;
			TRACE(L"DPCM import: End of file reached\n");
		}

		if (!std::memcmp(Header, "WAVE", 4)) {
			ValidWave = true;
		}
		else if (Scanning) {
			m_fSampleFile.Read(&BlockSize, 4);

			if (!std::memcmp(Header, "fmt ", 4)) {
				// Read the wave-format
				TRACE(L"DPCM import: Found fmt block\n");
				int ReadSize = BlockSize;
				if (ReadSize > sizeof(PCMWAVEFORMAT))
					ReadSize = sizeof(PCMWAVEFORMAT);

				m_fSampleFile.Read(&WaveFormat, ReadSize);
				m_fSampleFile.Seek(BlockSize - ReadSize, CFile::current);
				WaveFormatFound = true;

				if (WaveFormat.wf.wFormatTag != WAVE_FORMAT_PCM) {
					// Invalid audio format
					Scanning = false;
					ValidWave = false;
					TRACE(L"DPCM import: Unrecognized wave format (%i)\n", WaveFormat.wf.wFormatTag);
				}

			}
			else if (!std::memcmp(Header, "data", 4)) {
				// Actual wave-data, store the position
				TRACE(L"DPCM import: Found data block\n");
				m_iWaveSize = BlockSize;
				m_ullSampleStart = m_fSampleFile.GetPosition();
				m_fSampleFile.Seek(BlockSize, CFile::current);
			}
			else {
				// Unrecognized block
				TRACE(L"DPCM import: Unrecognized block %c%c%c%c\n", Header[0], Header[1], Header[2], Header[3]);
				m_fSampleFile.Seek(BlockSize, CFile::current);
			}
		}
	}

	if (!ValidWave || !WaveFormatFound || m_iWaveSize == 0) {
		// Failed to load file properly, display error message and quit
		TRACE(L"DPCM import: Unsupported or invalid wave file\n");
		m_fSampleFile.Close();
		AfxMessageBox(IDS_DPCM_IMPORT_INVALID_WAVEFILE, MB_ICONEXCLAMATION);
		return false;
	}

	// Save file info
	m_iChannels		  = WaveFormat.wf.nChannels;
	m_iSampleSize	  = WaveFormat.wf.nBlockAlign / WaveFormat.wf.nChannels;
	m_iBlockAlign	  = WaveFormat.wf.nBlockAlign;
	m_iAvgBytesPerSec = WaveFormat.wf.nAvgBytesPerSec;
	m_iSamplesPerSec  = WaveFormat.wf.nSamplesPerSec;

	TRACE(L"DPCM import: Scan done (%i Hz, %i bits, %i channels)\n", m_iSamplesPerSec, m_iSampleSize, m_iChannels);

	return true;
}

std::shared_ptr<ft0cc::doc::dpcm_sample> CPCMImporter::ConvertFile(int dB, int quality) {
	// Converts a WAV file to a DPCM sample
	const int DMC_BIAS = 32;

	unsigned char DeltaAcc = 0;	// DPCM sample accumulator
	int Delta = DMC_BIAS;		// Delta counter
	int AccReady = 8;

	float volume = std::powf(10, float(dB) / 20.0f);		// Convert dB to linear

	// Seek to start of samples
	m_fSampleFile.Seek(m_ullSampleStart, CFile::begin);

	// Allocate space
	std::vector<uint8_t> pSamples;		// // //

	// Determine resampling factor
	float base_freq = (float)MASTER_CLOCK_NTSC / (float)CDPCM::DMC_PERIODS_NTSC[quality];
	float resample_factor = base_freq / (float)m_iSamplesPerSec;

	resampler resmpler(*m_psinc, resample_factor, m_iChannels, m_iSampleSize, m_iWaveSize, m_fSampleFile);
	float val;
	// Conversion
	while (pSamples.size() < ft0cc::doc::dpcm_sample::max_size && resmpler.get(val)) {		// // //

		// when resampling we must clip because of possible ringing.
		const float MAX_AMP =  (1 << 16) - 1;
		const float MIN_AMP = -(1 << 16) + 1; // just being symetric
		val = std::clamp(val, MIN_AMP, MAX_AMP);

		// Volume done this way so it acts as before
		int Sample = (int)((val * volume) / 1024.f) + DMC_BIAS;

		DeltaAcc >>= 1;

		// PCM -> DPCM
		if (Sample >= Delta) {
			++Delta;
			if (Delta > 63)
				Delta = 63;
			DeltaAcc |= 0x80;
		}
		else if (Sample < Delta) {
			--Delta;
			if (Delta < 0)
				Delta = 0;
		}

		if (--AccReady == 0) {
			// Store sample
			pSamples.push_back(DeltaAcc);
			AccReady = 8;
		}
	}

	// TODO: error handling with th efile
	// if (!resmpler.eof())
	//      throw ?? or something else.

	// Adjust sample until size is x * $10 + 1 bytes
	while (pSamples.size() < ft0cc::doc::dpcm_sample::max_size && ((pSamples.size() & 0x0F) - 1) != 0)		// // //
		pSamples.push_back(0xAA);

	// Center end of sample (not yet working)
#if 0
	int CenterPos = (iSamples << 3) - 1;
	while (Delta != DMC_BIAS && CenterPos > 0) {
		if (Delta > DMC_BIAS) {
			int BitPos = CenterPos & 0x07;
			if ((pSamples[CenterPos >> 3] & (1 << BitPos)))
				--Delta;
			pSamples[CenterPos >> 3] = pSamples[CenterPos >> 3] & ~(1 << BitPos);
		}
		else if (Delta < DMC_BIAS) {
			int BitPos = CenterPos & 0x07;
			if ((pSamples[CenterPos >> 3] & (1 << BitPos)) == 0)
				++Delta;
			pSamples[CenterPos >> 3] = pSamples[CenterPos >> 3] | (1 << BitPos);
		}
		--CenterPos;
	}
#endif

	// Return a sample object
	return std::make_shared<ft0cc::doc::dpcm_sample>(std::move(pSamples), "");		// // //
}

int CPCMImporter::GetWaveSampleRate() const {
	return m_iSamplesPerSec;
}

int CPCMImporter::GetWaveSampleSize() const {
	return m_iSampleSize;
}

int CPCMImporter::GetWaveChannelCount() const {
	return m_iChannels;
}
