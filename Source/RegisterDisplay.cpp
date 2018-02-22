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

#include "RegisterDisplay.h"
#include "FamiTrackerEnv.h"		// // //
#include "SoundGen.h"
#include "Settings.h"
#include "APU/APU.h"
#include "APU/Noise.h"		// // //
#include "APU/DPCM.h"		// // //
#include "APU/Types.h"		// // //
#include "RegisterState.h"
#include "Graphics.h"
#include "Color.h"		// // //
#include "PatternNote.h"
#include <algorithm>
#include "str_conv/str_conv.hpp"		// // //

namespace {

const COLORREF DECAY_COLOR[CRegisterState::DECAY_RATE + 1] = {		// // //
	0xFFFF80, 0xE6F993, 0xCCF3A6, 0xB3ECB9, 0x99E6CC, 0x80E0E0, 0x80D9E3, 0x80D3E6,
	0x80CCE9, 0x80C6EC, 0x80C0F0, 0x80B9F3, 0x80B3F6, 0x80ACF9, 0x80A6FC, 0x80A0FF,
}; // BLEND has lower precision

double NoteFromFreq(double Freq) {
	// Convert frequency to note number
	return 45.0 + 12.0 * (std::log(Freq / 440.0) / log(2.0));
}

std::string NoteToStr(int Note) {
	int Octave = GET_OCTAVE(Note) + 1;		// // //
	int Index = value_cast(GET_NOTE(Note)) - 1;

	std::string str;
	if (Env.GetSettings()->Appearance.bDisplayFlats)
		str = stChanNote::NOTE_NAME_FLAT[Index];
	else
		str = stChanNote::NOTE_NAME[Index];
	str += std::to_string(Octave);
	return str;
}

CStringA GetPitchText(int digits, int period, double freq) {
	const CStringA fmt = "pitch = $%0*X (%7.2fHz %s %+03i)";
	const double note = NoteFromFreq(freq);
	const int note_conv = note >= 0 ? int(note + 0.5) : int(note - 0.5);
	const int cents = int((note - double(note_conv)) * 100.0);

	if (freq != 0.)
		return FormattedA(fmt, digits, period, freq, NoteToStr(note_conv).data(), cents);
	return FormattedA(fmt, digits, period, 0., "---", 0);
};

} // namespace

CRegisterDisplay::CRegisterDisplay(CDC &dc, COLORREF bgColor) : dc_(dc), bgColor_(bgColor) {
}

void CRegisterDisplay::Draw() {
	dc_.SetBkMode(TRANSPARENT);		// // //

	const CSoundGen *pSoundGen = Env.GetSoundGenerator();

	const int BAR_OFFSET = LINE_HEIGHT * (3 +
		pSoundGen->IsExpansionEnabled(sound_chip_t::APU) * 8 +
		pSoundGen->IsExpansionEnabled(sound_chip_t::VRC6) * 5 +
		pSoundGen->IsExpansionEnabled(sound_chip_t::MMC5) * 4 +
		pSoundGen->IsExpansionEnabled(sound_chip_t::N163) * 18 +
		pSoundGen->IsExpansionEnabled(sound_chip_t::FDS) * 13 +
		pSoundGen->IsExpansionEnabled(sound_chip_t::VRC7) * 9 +
		pSoundGen->IsExpansionEnabled(sound_chip_t::S5B) * 8);		// // //
	int vis_line = 0;

	const auto DrawVolFunc = [&] (double Freq, int Volume) {
		dc_.FillSolidRect(x - 1, BAR_OFFSET + vis_line * 10 - 1, 6 * 108 + 3, 9, 0x808080);
		dc_.FillSolidRect(x, BAR_OFFSET + vis_line * 10, 6 * 108 + 1, 7, 0);
		for (int i = 0; i < 10; i++)
			dc_.SetPixelV(x + 72 * i, BAR_OFFSET + vis_line * 10 + 3, i == 4 ? 0x808080 : 0x303030);

		const double note = NoteFromFreq(Freq);
		const int note_conv = note >= 0 ? int(note + 0.5) : int(note - 0.5);
		if (Volume > 0xFF) Volume = 0xFF;
		if (note_conv >= -12 && note_conv <= 96 && Volume)		// // //
			dc_.FillSolidRect(29 + 6 * (note_conv + 12), BAR_OFFSET + vis_line * 10, 3, 7, GREY(Volume));
		++vis_line;
	};

	// 2A03 / 2A07
	if (pSoundGen->IsExpansionEnabled(sound_chip_t::APU)) {
		DrawHeader("2A03 registers");		// // //

		for (int i = 0; i < MAX_CHANNELS_2A03; ++i) {
			GetRegs(sound_chip_t::APU, [&] (int x) { return 0x4000 + i * 4 + x; }, 4);
			DrawReg(FormattedA("$%04X:", 0x4000 + i * 4), 4);

			int period, vol;
			double freq = pSoundGen->GetChannelFrequency(sound_chip_t::APU, i);		// // //
//			dc.FillSolidRect(x + 200, y, x + 400, y + 18, m_colEmptyBg);

			CStringA text;
			switch (i) {
			case 0: case 1:
				period = reg[2] | ((reg[3] & 7) << 8);
				vol = reg[0] & 0x0F;
				text = FormattedA("%s, vol = %02i, duty = %i", (LPCSTR)GetPitchText(3, period, freq), vol, reg[0] >> 6); break;
			case 2:
				period = reg[2] | ((reg[3] & 7) << 8);
				vol = reg[0] ? 15 : 0;
				text = FormattedA("%s", (LPCSTR)GetPitchText(3, period, freq)); break;
			case 3:
				period = reg[2] & 0x0F;
				vol = reg[0] & 0x0F;
				text = FormattedA("pitch = $%01X, vol = %02i, mode = %i", period, vol, reg[2] >> 7);
				period = (period << 4) | ((reg[2] & 0x80) >> 4);
				freq /= 16; break; // for display
			case 4:
				period = reg[0] & 0x0F;
				vol = 15 * !pSoundGen->PreviewDone();
				text = FormattedA("%s, %s, size = %i byte%c", (LPCSTR)GetPitchText(1, period & 0x0F, freq),
					(reg[0] & 0x40) ? "looped" : "once", (reg[3] << 4) | 1, reg[3] ? 's' : ' ');
				freq /= 16; break; // for display
			}
/*
			dc.FillSolidRect(250 + i * 30, 0, 20, m_iWinHeight - HEADER_CHAN_HEIGHT, 0);
			dc.FillSolidRect(250 + i * 30, (period >> 1), 20, 5, RGB(vol << 4, vol << 4, vol << 4));
*/
			DrawText_(180, text);
			DrawVolFunc(freq, vol << 4);
		}

		const auto &DPCMState = pSoundGen->GetDPCMState();		// // //
		++line; y += LINE_HEIGHT;		// // //
		DrawText_(180, FormattedA("position: %02i, delta = $%02X", DPCMState.SamplePos, DPCMState.DeltaCntr));
	}

	if (pSoundGen->IsExpansionEnabled(sound_chip_t::VRC6)) {
		DrawHeader("VRC6 registers");		// // //

		for (int i = 0; i < MAX_CHANNELS_VRC6; ++i) {
			GetRegs(sound_chip_t::VRC6, [&] (int x) { return 0x9000 + i * 0x1000 + x; }, 3);
			DrawReg(FormattedA("$%04X:", 0x9000 + i * 0x1000), 3);

			int period = (reg[1] | ((reg[2] & 15) << 8));
			int vol = (reg[0] & (i == 2 ? 0x3F : 0x0F));
			double freq = pSoundGen->GetChannelFrequency(sound_chip_t::VRC6, i);		// // //

			CStringA text = FormattedA("%s, vol = %02i", (LPCSTR)GetPitchText(3, period, freq), vol);
			if (i != 2)
				AppendFormatA(text, ", duty = %i", (reg[0] >> 4) & 0x07);
			DrawText_(180, text);
			DrawVolFunc(freq, vol << (i == 2 ? 3 : 4));
		}
	}

	if (pSoundGen->IsExpansionEnabled(sound_chip_t::MMC5)) {		// // //
		DrawHeader("MMC5 registers");		// // //

		for (int i = 0; i < MAX_CHANNELS_MMC5 - 1; ++i) {
			GetRegs(sound_chip_t::MMC5, [&] (int x) { return 0x5000 + i * 4 + x; }, 4);
			DrawReg(FormattedA("$%04X:", 0x5000 + i * 4), 4);

			int period = (reg[2] | ((reg[3] & 7) << 8));
			int vol = (reg[0] & 0x0F);
			double freq = pSoundGen->GetChannelFrequency(sound_chip_t::MMC5, i);		// // //

			DrawText_(180, FormattedA("%s, vol = %02i, duty = %i", (LPCSTR)GetPitchText(3, period, freq), vol, reg[0] >> 6));
			DrawVolFunc(freq, vol << 4);
		}
	}

	if (pSoundGen->IsExpansionEnabled(sound_chip_t::N163)) {
		DrawHeader("N163 registers");		// // //

		// // // N163 wave
		const int N163_CHANS = pSoundGen->GetNamcoChannelCount();		// // //
		const int Length = 0x80 - 8 * N163_CHANS;

		y += 18;
		dc_.FillSolidRect(x + 300 - 1, y - 1, 2 * Length + 2, 17, 0x808080);
		dc_.FillSolidRect(x + 300, y, 2 * Length, 15, 0);
		for (int i = 0; i < Length; i++) {
			auto pState = pSoundGen->GetRegState(sound_chip_t::N163, i);
			const int Hi = (pState->GetValue() >> 4) & 0x0F;
			const int Lo = pState->GetValue() & 0x0F;
			COLORREF Col = BLEND(GREY(192), DECAY_COLOR[pState->GetNewValueTime()],
				(double)pState->GetLastUpdatedTime() / CRegisterState::DECAY_RATE);
			dc_.FillSolidRect(x + 300 + i * 2    , y + 15 - Lo, 1, Lo, Col);
			dc_.FillSolidRect(x + 300 + i * 2 + 1, y + 15 - Hi, 1, Hi, Col);
		}
		for (int i = 0; i < N163_CHANS; ++i) {
			auto pPosState = pSoundGen->GetRegState(sound_chip_t::N163, 0x78 - i * 8 + 6);
			auto pLenState = pSoundGen->GetRegState(sound_chip_t::N163, 0x78 - i * 8 + 4);
			const int WavePos = pPosState->GetValue();
			const int WaveLen = 0x100 - (pLenState->GetValue() & 0xFC);
			const int NewTime = std::min(pPosState->GetNewValueTime(), pLenState->GetNewValueTime());
			const int UpdateTime = std::min(pPosState->GetLastUpdatedTime(), pLenState->GetLastUpdatedTime());
			dc_.FillSolidRect(x + 300, y + 20 + i * 5, Length * 2, 3, 0);
			dc_.FillSolidRect(x + 300 + WavePos, y + 20 + i * 5, WaveLen, 3,
							   BLEND(GREY(192), DECAY_COLOR[NewTime], (double)UpdateTime / CRegisterState::DECAY_RATE));
		}
		y -= 18;

		double FreqCache[8] = { };
		int VolCache[8] = { };

		for (int i = 0; i < 16; ++i) {
			GetRegs(sound_chip_t::N163, [&] (int x) { return i * 8 + x; }, 8);
			DrawReg(FormattedA("$%02X:", i * 8), 8);

			int period = (reg[0] | (reg[2] << 8) | ((reg[4] & 0x03) << 16));
			int vol = (reg[7] & 0x0F);
			double freq = pSoundGen->GetChannelFrequency(sound_chip_t::N163, 15 - i);		// // //

			if (i >= 16 - N163_CHANS) {
				DrawText_(300, FormattedA("%s, vol = %02i", (LPCSTR)GetPitchText(5, period, freq), vol));
				FreqCache[15 - i] = freq;
				VolCache[15 - i] = vol << 4;
			}
		}

		for (int i = 0; i < N163_CHANS; ++i)		// // //
			DrawVolFunc(FreqCache[i], VolCache[i]);
	}

	if (pSoundGen->IsExpansionEnabled(sound_chip_t::FDS)) {
		DrawHeader("FDS registers");		// // //

		int period = (pSoundGen->GetReg(sound_chip_t::FDS, 0x4082) & 0xFF) | ((pSoundGen->GetReg(sound_chip_t::FDS, 0x4083) & 0x0F) << 8);
		int vol = (pSoundGen->GetReg(sound_chip_t::FDS, 0x4080) & 0x3F);
		double freq = pSoundGen->GetChannelFrequency(sound_chip_t::FDS, 0);		// // //

		for (int i = 0; i < 11; ++i) {
			GetRegs(sound_chip_t::FDS, [&] (int) { return 0x4080 + i; }, 1);
			DrawReg(FormattedA("$%04X:", 0x4080 + i), 1);
			if (!i)
				DrawText_(180, FormattedA("%s, vol = %02i", (LPCSTR)GetPitchText(3, period, freq), vol));
		}

		DrawVolFunc(freq, vol << 3);
	}

	if (pSoundGen->IsExpansionEnabled(sound_chip_t::VRC7)) {		// // //
		DrawHeader("VRC7 registers");		// // //

		GetRegs(sound_chip_t::VRC7, [] (int x) { return x; }, 8);
		DrawReg("$00:", 8);		// // //

		for (int i = 0; i < MAX_CHANNELS_VRC7; ++i) {
			GetRegs(sound_chip_t::VRC7, [&] (int x) { return i + (++x << 4); }, 3);
			DrawReg(FormattedA("$x%01X:", i), 3);

			int period = reg[0] | ((reg[1] & 0x01) << 8);
			int vol = 0x0F - (pSoundGen->GetReg(sound_chip_t::VRC7, i + 0x30) & 0x0F);
			double freq = pSoundGen->GetChannelFrequency(sound_chip_t::VRC7, i);		// // //

			DrawText_(180, FormattedA("%s, vol = %02i, patch = $%01X", (LPCSTR)GetPitchText(3, period, freq), vol, reg[2] >> 4));

			DrawVolFunc(freq, vol << 4);
		}
	}

	if (pSoundGen->IsExpansionEnabled(sound_chip_t::S5B)) {		// // //
		DrawHeader("5B registers");		// // //

		for (int i = 0; i < 4; ++i) {
			GetRegs(sound_chip_t::S5B, [&] (int x) { return i * 2 + x; }, 2);
			DrawReg(FormattedA("$%02X:", i * 2), 2);

			int period = reg[0] | ((reg[1] & 0x0F) << 8);
			int vol = pSoundGen->GetReg(sound_chip_t::S5B, 8 + i) & 0x0F;
			double freq = pSoundGen->GetChannelFrequency(sound_chip_t::S5B, i);		// // //

			if (i < MAX_CHANNELS_S5B)
				DrawText_(180, FormattedA("%s, vol = %02i, mode = %c%c%c", (LPCSTR)GetPitchText(3, period, freq), vol,
					(pSoundGen->GetReg(sound_chip_t::S5B, 7) & (1 << i)) ? L'-' : L'T',
					(pSoundGen->GetReg(sound_chip_t::S5B, 7) & (8 << i)) ? L'-' : L'N',
					(pSoundGen->GetReg(sound_chip_t::S5B, 8 + i) & 0x10) ? L'E' : L'-'));
			else
				DrawText_(180, FormattedA("pitch = $%02X", reg[0] & 0x1F));

			if (i < 3)
				DrawVolFunc(freq, vol << 4);
		}

		for (int i = 0; i < 2; ++i) {
			GetRegs(sound_chip_t::S5B, [&] (int x) { return i * 3 + x + 8; }, 3);
			DrawReg(FormattedA("$%02X:", i * 3 + 8), 3);

			if (i == 1) {
				int period = (reg[0] | (reg[1] << 8));
				double freq = pSoundGen->GetChannelFrequency(sound_chip_t::S5B, 3);		// // //
				if (freq != 0. && reg[1] == 0)
					DrawText_(180, FormattedA("%s, shape = $%01X", (LPCSTR)GetPitchText(4, period, freq), reg[2]));
				else
					DrawText_(180, FormattedA("period = $%04X, shape = $%01X", period, reg[2]));
			}
		}
	}

	// Surrounding frame
//	dc.Draw3dRect(20, 20, 200, line * 18 + 20, 0xA0A0A0, 0x505050);		// // //
}

void CRegisterDisplay::DrawHeader(const CStringA &text) {
	line += 2; y += LINE_HEIGHT * 2;
	dc_.MoveTo(x, y);
	dc_.SetBkColor(bgColor_);
	dc_.SetTextColor(0xFFAFAF);
	dc_.TextOutW(x, y, conv::to_wide(text).data());
}

void CRegisterDisplay::DrawReg(const CStringA &header, int count) {
	++line; y += LINE_HEIGHT;
	dc_.SetBkColor(bgColor_);
	dc_.SetTextColor(0xFFAFAF);
	dc_.SetTextAlign(TA_UPDATECP);
	dc_.MoveTo(x, y);
	dc_.TextOutW(0, 0, conv::to_wide(header).data());
	for (int j = 0; j < count; j++) {
		dc_.SetTextColor(BLEND(GREY(192), DECAY_COLOR[update[j] >> 4], (double)(update[j] & 0x0F) / CRegisterState::DECAY_RATE));
		dc_.TextOutW(0, 0, FormattedW(L" $%02X", reg[j]));
	}
}

void CRegisterDisplay::DrawText_(int xOffs, const CStringA &text) {
	dc_.SetTextColor(0x808080);
	dc_.SetTextAlign(TA_NOUPDATECP);
	dc_.TextOutW(x + xOffs, y, conv::to_wide(text).data());
}

template <typename F>
void CRegisterDisplay::GetRegs(sound_chip_t Chip, F f, int count) {
	for (int j = 0; j < count; ++j) {
		auto pState = Env.GetSoundGenerator()->GetRegState(Chip, f(j));		// // //
		reg[j] = pState->GetValue();
		update[j] = pState->GetLastUpdatedTime() | (pState->GetNewValueTime() << 4);
	}
}
