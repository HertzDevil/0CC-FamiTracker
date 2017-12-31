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
#include "FamiTracker.h"
#include "SoundGen.h"
#include "Settings.h"
#include "APU/APU.h"
#include "APU/Noise.h"		// // //
#include "APU/DPCM.h"		// // //
#include "RegisterState.h"
#include "Graphics.h"
#include "PatternNote.h"
#include <algorithm>

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
	int Index = GET_NOTE(Note) - 1;

	std::string str;
	if (theApp.GetSettings()->Appearance.bDisplayFlats)
		str = stChanNote::NOTE_NAME_FLAT[Index];
	else
		str = stChanNote::NOTE_NAME[Index];
	str += std::to_string(Octave);
	return str;
}

CString GetPitchText(int digits, int period, double freq) {
	const CString fmt = _T("pitch = $%0*X (%7.2fHz %s %+03i)");
	const double note = NoteFromFreq(freq);
	const int note_conv = note >= 0 ? int(note + 0.5) : int(note - 0.5);
	const int cents = int((note - double(note_conv)) * 100.0);

	CString str;
	if (freq != 0.)
		str.Format(fmt, digits, period, freq, NoteToStr(note_conv).c_str(), cents);
	else
		str.Format(fmt, digits, period, 0., _T("---"), 0);
	return str;
};

} // namespace

CRegisterDisplay::CRegisterDisplay(CDC &dc, COLORREF bgColor) : dc_(dc), bgColor_(bgColor) {
}

void CRegisterDisplay::Draw() {
	dc_.SetBkMode(TRANSPARENT);		// // //

	const CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	CString text;

	const int BAR_OFFSET = LINE_HEIGHT * (3 + 8 +
		pSoundGen->IsExpansionEnabled(SNDCHIP_VRC6) * 5 +
		pSoundGen->IsExpansionEnabled(SNDCHIP_MMC5) * 4 +
		pSoundGen->IsExpansionEnabled(SNDCHIP_N163) * 18 +
		pSoundGen->IsExpansionEnabled(SNDCHIP_FDS) * 13 +
		pSoundGen->IsExpansionEnabled(SNDCHIP_VRC7) * 9 +
		pSoundGen->IsExpansionEnabled(SNDCHIP_S5B) * 8);		// // //
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
			dc_.FillSolidRect(29 + 6 * (note_conv + 12), BAR_OFFSET + vis_line * 10, 3, 7, RGB(Volume, Volume, Volume));
		++vis_line;
	};

	// 2A03
	DrawHeader(_T("2A03"));		// // //

	for (int i = 0; i < 5; ++i) {
		GetRegs(SNDCHIP_NONE, [&] (int x) { return 0x4000 + i * 4 + x; }, 4);
		text.Format(_T("$%04X:"), 0x4000 + i * 4);		// // //
		DrawReg(text, 4);

		int period, vol;
		double freq = pSoundGen->GetChannelFrequency(SNDCHIP_NONE, i);		// // //
//		dc.FillSolidRect(x + 200, y, x + 400, y + 18, m_colEmptyBg);

		switch (i) {
		case 0: case 1:
			period = reg[2] | ((reg[3] & 7) << 8);
			vol = reg[0] & 0x0F;
			text.Format(_T("%s, vol = %02i, duty = %i"), GetPitchText(3, period, freq), vol, reg[0] >> 6); break;
		case 2:
			period = reg[2] | ((reg[3] & 7) << 8);
			vol = reg[0] ? 15 : 0;
			text.Format(_T("%s"), GetPitchText(3, period, freq)); break;
		case 3:
			period = reg[2] & 0x0F;
			vol = reg[0] & 0x0F;
			text.Format(_T("pitch = $%01X, vol = %02i, mode = %i"), period, vol, reg[2] >> 7);
			period = (period << 4) | ((reg[2] & 0x80) >> 4);
			freq /= 16; break; // for display
		case 4:
			period = reg[0] & 0x0F;
			vol = 15 * !pSoundGen->PreviewDone();
			text.Format(_T("%s, %s, size = %i byte%c"), GetPitchText(1, period & 0x0F, freq),
				(reg[0] & 0x40) ? _T("looped") : _T("once"), (reg[3] << 4) | 1, reg[3] ? 's' : ' ');
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
	text.Format(_T("position: %02i, delta = $%02X"), DPCMState.SamplePos, DPCMState.DeltaCntr);
	++line; y += LINE_HEIGHT;		// // //
	DrawText_(180, text);

	if (pSoundGen->IsExpansionEnabled(SNDCHIP_VRC6)) {
		DrawHeader(_T("VRC6"));		// // //

		// VRC6
		for (int i = 0; i < 3; ++i) {
			GetRegs(SNDCHIP_VRC6, [&] (int x) { return 0x9000 + i * 0x1000 + x; }, 3);
			text.Format(_T("$%04X:"), 0x9000 + i * 0x1000);		// // //
			DrawReg(text, 3);

			int period = (reg[1] | ((reg[2] & 15) << 8));
			int vol = (reg[0] & (i == 2 ? 0x3F : 0x0F));
			double freq = pSoundGen->GetChannelFrequency(SNDCHIP_VRC6, i);		// // //

			text.Format(_T("%s, vol = %02i"), GetPitchText(3, period, freq), vol);
			if (i != 2)
				text.AppendFormat(_T(", duty = %i"), (reg[0] >> 4) & 0x07);
			DrawText_(180, text);
			DrawVolFunc(freq, vol << (i == 2 ? 3 : 4));
		}
	}

	if (pSoundGen->IsExpansionEnabled(SNDCHIP_MMC5)) {		// // //
		DrawHeader(_T("MMC5"));		// // //

		// MMC5
		for (int i = 0; i < 2; ++i) {
			GetRegs(SNDCHIP_MMC5, [&] (int x) { return 0x5000 + i * 4 + x; }, 4);
			text.Format(_T("$%04X:"), 0x5000 + i * 4);
			DrawReg(text, 4);

			int period = (reg[2] | ((reg[3] & 7) << 8));
			int vol = (reg[0] & 0x0F);
			double freq = pSoundGen->GetChannelFrequency(SNDCHIP_MMC5, i);		// // //

			text.Format(_T("%s, vol = %02i, duty = %i"), GetPitchText(3, period, freq), vol, reg[0] >> 6);
			DrawText_(180, text);
			DrawVolFunc(freq, vol << 4);
		}
	}

	if (pSoundGen->IsExpansionEnabled(SNDCHIP_N163)) {
		DrawHeader(_T("N163"));		// // //

		// // // N163 wave
		const int N163_CHANS = pSoundGen->GetNamcoChannelCount();		// // //
		const int Length = 0x80 - 8 * N163_CHANS;

		y += 18;
		dc_.FillSolidRect(x + 300 - 1, y - 1, 2 * Length + 2, 17, 0x808080);
		dc_.FillSolidRect(x + 300, y, 2 * Length, 15, 0);
		for (int i = 0; i < Length; i++) {
			auto pState = pSoundGen->GetRegState(SNDCHIP_N163, i);
			const int Hi = (pState->GetValue() >> 4) & 0x0F;
			const int Lo = pState->GetValue() & 0x0F;
			COLORREF Col = BLEND(
				0xC0C0C0, DECAY_COLOR[pState->GetNewValueTime()], 100 * pState->GetLastUpdatedTime() / CRegisterState::DECAY_RATE
			);
			dc_.FillSolidRect(x + 300 + i * 2    , y + 15 - Lo, 1, Lo, Col);
			dc_.FillSolidRect(x + 300 + i * 2 + 1, y + 15 - Hi, 1, Hi, Col);
		}
		for (int i = 0; i < N163_CHANS; ++i) {
			auto pPosState = pSoundGen->GetRegState(SNDCHIP_N163, 0x78 - i * 8 + 6);
			auto pLenState = pSoundGen->GetRegState(SNDCHIP_N163, 0x78 - i * 8 + 4);
			const int WavePos = pPosState->GetValue();
			const int WaveLen = 0x100 - (pLenState->GetValue() & 0xFC);
			const int NewTime = std::min(pPosState->GetNewValueTime(), pLenState->GetNewValueTime());
			const int UpdateTime = std::min(pPosState->GetLastUpdatedTime(), pLenState->GetLastUpdatedTime());
			dc_.FillSolidRect(x + 300, y + 20 + i * 5, Length * 2, 3, 0);
			dc_.FillSolidRect(x + 300 + WavePos, y + 20 + i * 5, WaveLen, 3,
							   BLEND(0xC0C0C0, DECAY_COLOR[NewTime], 100 * UpdateTime / CRegisterState::DECAY_RATE));
		}
		y -= 18;

		double FreqCache[8] = { };
		int VolCache[8] = { };

		// N163
		for (int i = 0; i < 16; ++i) {
			GetRegs(SNDCHIP_N163, [&] (int x) { return i * 8 + x; }, 8);
			text.Format(_T("$%02X:"), i * 8);
			DrawReg(text, 8);

			int period = (reg[0] | (reg[2] << 8) | ((reg[4] & 0x03) << 16));
			int vol = (reg[7] & 0x0F);
			double freq = pSoundGen->GetChannelFrequency(SNDCHIP_N163, 15 - i);		// // //

			if (i >= 16 - N163_CHANS) {
				text.Format(_T("%s, vol = %02i"), GetPitchText(5, period, freq), vol);
				DrawText_(300, text);
				FreqCache[15 - i] = freq;
				VolCache[15 - i] = vol << 4;
			}
		}

		for (int i = 0; i < N163_CHANS; ++i)		// // //
			DrawVolFunc(FreqCache[i], VolCache[i]);
	}

	if (pSoundGen->IsExpansionEnabled(SNDCHIP_FDS)) {
		DrawHeader(_T("FDS"));		// // //

		int period = (pSoundGen->GetReg(SNDCHIP_FDS, 0x4082) & 0xFF) | ((pSoundGen->GetReg(SNDCHIP_FDS, 0x4083) & 0x0F) << 8);
		int vol = (pSoundGen->GetReg(SNDCHIP_FDS, 0x4080) & 0x3F);
		double freq = pSoundGen->GetChannelFrequency(SNDCHIP_FDS, 0);		// // //

		CString FDStext;
		FDStext.Format(_T("%s, vol = %02i"), GetPitchText(3, period, freq), vol);

		for (int i = 0; i < 11; ++i) {
			GetRegs(SNDCHIP_FDS, [&] (int) { return 0x4080 + i; }, 1);
			text.Format(_T("$%04X:"), 0x4080 + i);
			DrawReg(text, 1);
			if (!i) DrawText_(180, FDStext);
		}

		DrawVolFunc(freq, vol << 3);
	}

	if (pSoundGen->IsExpansionEnabled(SNDCHIP_VRC7)) {		// // //
		DrawHeader(_T("VRC7"));		// // //

		GetRegs(SNDCHIP_VRC7, [] (int x) { return x; }, 8);
		DrawReg(_T("$00:"), 8);		// // //

		for (int i = 0; i < 6; ++i) {
			GetRegs(SNDCHIP_VRC7, [&] (int x) { return i + (++x << 4); }, 3);
			text.Format(_T("$x%01X:"), i);
			DrawReg(text, 3);

			int period = reg[0] | ((reg[1] & 0x01) << 8);
			int vol = 0x0F - (pSoundGen->GetReg(SNDCHIP_VRC7, i + 0x30) & 0x0F);
			double freq = pSoundGen->GetChannelFrequency(SNDCHIP_VRC7, i);		// // //

			text.Format(_T("%s, vol = %02i, patch = $%01X"), GetPitchText(3, period, freq), vol, reg[2] >> 4);
			DrawText_(180, text);

			DrawVolFunc(freq, vol << 4);
		}
	}

	if (pSoundGen->IsExpansionEnabled(SNDCHIP_S5B)) {		// // //
		DrawHeader(_T("5B"));		// // //

		// S5B
		for (int i = 0; i < 4; ++i) {
			GetRegs(SNDCHIP_S5B, [&] (int x) { return i * 2 + x; }, 2);
			text.Format(_T("$%02X:"), i * 2);
			DrawReg(text, 2);

			int period = reg[0] | ((reg[1] & 0x0F) << 8);
			int vol = pSoundGen->GetReg(SNDCHIP_S5B, 8 + i) & 0x0F;
			double freq = pSoundGen->GetChannelFrequency(SNDCHIP_S5B, i);		// // //

			if (i < 3)
				text.Format(_T("%s, vol = %02i, mode = %c%c%c"), GetPitchText(3, period, freq), vol,
					(pSoundGen->GetReg(SNDCHIP_S5B, 7) & (1 << i)) ? _T('-') : _T('T'),
					(pSoundGen->GetReg(SNDCHIP_S5B, 7) & (8 << i)) ? _T('-') : _T('N'),
					(pSoundGen->GetReg(SNDCHIP_S5B, 8 + i) & 0x10) ? _T('E') : _T('-'));
			else
				text.Format(_T("pitch = $%02X"), reg[0] & 0x1F);
			DrawText_(180, text);

			if (i < 3)
				DrawVolFunc(freq, vol << 4);
		}

		for (int i = 0; i < 2; ++i) {
			GetRegs(SNDCHIP_S5B, [&] (int x) { return i * 3 + x + 8; }, 3);
			text.Format(_T("$%02X:"), i * 3 + 8);
			DrawReg(text, 3);

			if (i == 1) {
				int period = (reg[0] | (reg[1] << 8));
				double freq = pSoundGen->GetChannelFrequency(SNDCHIP_S5B, 3);		// // //
				if (freq != 0. && reg[1] == 0)
					text.Format(_T("%s, shape = $%01X"), GetPitchText(4, period, freq), reg[2]);
				else
					text.Format(_T("period = $%04X, shape = $%01X"), period, reg[2]);

				DrawText_(180, text);
			}
		}
	}

	// Surrounding frame
//	dc.Draw3dRect(20, 20, 200, line * 18 + 20, 0xA0A0A0, 0x505050);		// // //
}

void CRegisterDisplay::DrawHeader(const CString &text) {
	line += 2; y += LINE_HEIGHT * 2;
	dc_.MoveTo(x, y);
	dc_.SetBkColor(bgColor_);
	dc_.SetTextColor(0xFFAFAF);
	dc_.TextOut(x, y, text + _T(" registers"));
}

void CRegisterDisplay::DrawReg(const CString &header, int count) {
	++line; y += LINE_HEIGHT;
	dc_.SetBkColor(bgColor_);
	dc_.SetTextColor(0xFFAFAF);
	dc_.SetTextAlign(TA_UPDATECP);
	dc_.MoveTo(x, y);
	dc_.TextOut(0, 0, header);
	for (int j = 0; j < count; j++) {
		CString str;
		str.Format(_T(" $%02X"), reg[j]);
		dc_.SetTextColor(BLEND(0xC0C0C0, DECAY_COLOR[update[j] >> 4], 100 * (update[j] & 0x0F) / CRegisterState::DECAY_RATE));
		dc_.TextOut(0, 0, str);
	}
}

void CRegisterDisplay::DrawText_(int xOffs, const CString &text) {
	dc_.SetTextColor(0x808080);
	dc_.SetTextAlign(TA_NOUPDATECP);
	dc_.TextOut(x + xOffs, y, text);
}

template <typename F>
void CRegisterDisplay::GetRegs(unsigned Chip, F f, int count) {
	for (int j = 0; j < count; ++j) {
		auto pState = theApp.GetSoundGenerator()->GetRegState(Chip, f(j));		// // //
		reg[j] = pState->GetValue();
		update[j] = pState->GetLastUpdatedTime() | (pState->GetNewValueTime() << 4);
	}
}
