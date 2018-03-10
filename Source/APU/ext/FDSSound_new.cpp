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

#include "APU/ext/FDSSound_new.h"
#include "APU/Types.h"
#include <cstring>
#include <cmath>

namespace xgm {

const int RC_BITS = 12;

NES_FDS::NES_FDS ()
{
    option[OPT_CUTOFF] = 2000;
    option[OPT_4085_RESET] = 0;
    option[OPT_WRITE_PROTECT] = 0; // not used here, see nsfplay.cpp

    rc_k = 0;
    rc_l = (1<<RC_BITS);

    SetClock (MASTER_CLOCK_NTSC);		// // //
    SetRate (DEFAULT_RATE);

    Reset();
}

NES_FDS::~NES_FDS ()
{
}

void NES_FDS::SetClock (double c)
{
    clock = c;
}

void NES_FDS::SetRate (double r)
{
    rate = r;

    // configure lowpass filter
    double cutoff = double(option[OPT_CUTOFF]);
    double leak = 0.0;
    if (cutoff > 0)
        leak = std::exp(-2.0 * 3.14159 * cutoff / rate);
    rc_k = int32_t(leak * double(1<<RC_BITS));
    rc_l = (1<<RC_BITS) - rc_k;
}

void NES_FDS::SetOption (int id, int val)
{
    if(id<OPT_END) option[id] = val;

    // update cutoff immediately
    if (id == OPT_CUTOFF) SetRate(rate);
}

void NES_FDS::Reset ()
{
    master_io = true;
    master_vol = 0;
    last_freq = 0;
    last_vol = 0;

    rc_accum = 0;

    for (int i=0; i<2; ++i)
    {
        std::memset(wave[i], 0, sizeof(wave[i]));
        freq[i] = 0;
        phase[i] = 0;
    }
    wav_write = false;
    wav_halt = true;
    env_halt = true;
    mod_halt = true;
    mod_pos = 0;
    mod_write_pos = 0;

    for (int i=0; i<2; ++i)
    {
        env_mode[i] = false;
        env_disable[i] = true;
        env_timer[i] = 0;
        env_speed[i] = 0;
        env_out[i] = 0;
    }
    master_env_speed = 0xFF;

    // NOTE: the FDS BIOS reset only does the following related to audio:
    //   $4023 = $00
    //   $4023 = $83 enables master_io
    //   $4080 = $80 output volume = 0, envelope disabled
    //   $408A = $E8 master envelope speed
    Write(0x4023, 0x00);
    Write(0x4023, 0x83);
    Write(0x4080, 0x80);
    Write(0x408A, 0xE8);

    // reset other stuff
    Write(0x4082, 0x00); // wav freq 0
    Write(0x4083, 0x80); // wav disable
    Write(0x4084, 0x80); // mod strength 0
    Write(0x4085, 0x00); // mod position 0
    Write(0x4086, 0x00); // mod freq 0
    Write(0x4087, 0x80); // mod disable
    Write(0x4089, 0x00); // wav write disable, max global volume}
}

void NES_FDS::Tick (uint32_t clocks)
{
    // clock envelopes
    if (!env_halt && !wav_halt && (master_env_speed != 0))
    {
        for (int i=0; i<2; ++i)
        {
            if (!env_disable[i])
            {
                env_timer[i] += clocks;
                uint32_t period = ((env_speed[i]+1) * master_env_speed) << 3;
                while (env_timer[i] >= period)
                {
                    // clock the envelope
                    if (env_mode[i])
                    {
                        if (env_out[i] < 32) ++env_out[i];
                    }
                    else
                    {
                        if (env_out[i] > 0 ) --env_out[i];
                    }
                    env_timer[i] -= period;
                }
            }
        }
    }

    // clock the mod table
    if (!mod_halt)
    {
        // advance phase, adjust for modulator
        uint32_t start_pos = phase[TMOD] >> 16;
        phase[TMOD] += (clocks * freq[TMOD]);
        uint32_t end_pos = phase[TMOD] >> 16;

        // wrap the phase to the 64-step table (+ 16 bit accumulator)
        phase[TMOD] = phase[TMOD] & 0x3FFFFF;

        // execute all clocked steps
        for (uint32_t p = start_pos; p < end_pos; ++p)
        {
            int32_t wv = wave[TMOD][p & 0x3F];
            if (wv == 4) // 4 resets mod position
                mod_pos = 0;
            else
            {
                const int32_t BIAS[8] = { 0, 1, 2, 4, 0, -4, -2, -1 };
                mod_pos += BIAS[wv];
                mod_pos &= 0x7F; // 7-bit clamp
            }
        }
    }

    // clock the wav table
    if (!wav_halt)
    {
        // complex mod calculation
        int32_t mod = 0;
        if (env_out[EMOD] != 0) // skip if modulator off
        {
            // convert mod_pos to 7-bit signed
            int32_t pos = (mod_pos < 64) ? mod_pos : (mod_pos-128);

            // multiply pos by gain,
            // shift off 4 bits but with odd "rounding" behaviour
            int32_t temp = pos * env_out[EMOD];
            int32_t rem = temp & 0x0F;
            temp >>= 4;
            if ((rem > 0) && ((temp & 0x80) == 0))
            {
                if (pos < 0) temp -= 1;
                else         temp += 2;
            }

            // wrap if range is exceeded
            while (temp >= 192) temp -= 256;
            while (temp <  -64) temp += 256;

            // multiply result by pitch,
            // shift off 6 bits, round to nearest
            temp = freq[TWAV] * temp;
            rem = temp & 0x3F;
            temp >>= 6;
            if (rem >= 32) temp += 1;

            mod = temp;
        }

        // advance wavetable position
        int32_t f = freq[TWAV] + mod;
        phase[TWAV] = phase[TWAV] + (clocks * f);
        phase[TWAV] = phase[TWAV] & 0x3FFFFF; // wrap

        // store for trackinfo
        last_freq = f;
    }

    // output volume caps at 32
    int32_t vol_out = env_out[EVOL];
    if (vol_out > 32) vol_out = 32;

    // final output
    if (!wav_write)
        fout = wave[TWAV][(phase[TWAV]>>16)&0x3F] * vol_out;

    // NOTE: during wav_halt, the unit still outputs (at phase 0)
    // and volume can affect it if the first sample is nonzero.
    // haven't worked out 100% of the conditions for volume to
    // effect (vol envelope does not seem to run, but am unsure)
    // but this implementation is very close to correct

    // store for trackinfo
    last_vol = vol_out;
}

int32_t NES_FDS::Render ()		// // //
{
    // 8 bit approximation of master volume
    const double MASTER_VOL = 2.4 * 1223.0; // max FDS vol vs max APU square (arbitrarily 1223)
    const double MAX_OUT = 32.0f * 63.0f; // value that should map to master vol
    const int32_t MASTER[4] = {
        int((MASTER_VOL / MAX_OUT) * 256.0 * 2.0f / 2.0f),
        int((MASTER_VOL / MAX_OUT) * 256.0 * 2.0f / 3.0f),
        int((MASTER_VOL / MAX_OUT) * 256.0 * 2.0f / 4.0f),
        int((MASTER_VOL / MAX_OUT) * 256.0 * 2.0f / 5.0f) };

    int32_t v = fout * MASTER[master_vol] >> 8;

    // lowpass RC filter
    int32_t rc_out = ((rc_accum * rc_k) + (v * rc_l)) >> RC_BITS;
    rc_accum = rc_out;
    return rc_out;		// // //
}

bool NES_FDS::Write (uint32_t adr, uint32_t val)
{
    // $4023 master I/O enable/disable
    if (adr == 0x4023)
    {
        master_io = ((val & 2) != 0);
        return true;
    }

    if (!master_io)
        return false;
    if (adr < 0x4040 || adr > 0x408A)
        return false;

    if (adr < 0x4080) // $4040-407F wave table write
    {
        if (wav_write)
            wave[TWAV][adr - 0x4040] = val & 0x3F;
        return true;
    }

    switch (adr & 0x00FF)
    {
    case 0x80: // $4080 volume envelope
        env_disable[EVOL] = ((val & 0x80) != 0);
        env_mode[EVOL] = ((val & 0x40) != 0);
        env_timer[EVOL] = 0;
        env_speed[EVOL] = val & 0x3F;
        if (env_disable[EVOL])
            env_out[EVOL] = env_speed[EVOL];
        return true;
    case 0x81: // $4081 ---
        return false;
    case 0x82: // $4082 wave frequency low
        freq[TWAV] = (freq[TWAV] & 0xF00) | val;
        return true;
    case 0x83: // $4083 wave frequency high / enables
        freq[TWAV] = (freq[TWAV] & 0x0FF) | ((val & 0x0F) << 8);
        wav_halt = ((val & 0x80) != 0);
        env_halt = ((val & 0x40) != 0);
        if (wav_halt)
            phase[TWAV] = 0;
        if (env_halt)
        {
            env_timer[EMOD] = 0;
            env_timer[EVOL] = 0;
        }
        return true;
    case 0x84: // $4084 mod envelope
        env_disable[EMOD] = ((val & 0x80) != 0);
        env_mode[EMOD] = ((val & 0x40) != 0);
        env_timer[EMOD] = 0;
        env_speed[EMOD] = val & 0x3F;
        if (env_disable[EMOD])
            env_out[EMOD] = env_speed[EMOD];
        return true;
    case 0x85: // $4085 mod position
        mod_pos = val & 0x7F;
        // not hardware accurate., but prevents detune due to cycle inaccuracies
        // (notably in Bio Miracle Bokutte Upa)
        if (option[OPT_4085_RESET])
            phase[TMOD] = mod_write_pos << 16;
        return true;
    case 0x86: // $4086 mod frequency low
        freq[TMOD] = (freq[TMOD] & 0xF00) | val;
        return true;
    case 0x87: // $4087 mod frequency high / enable
        freq[TMOD] = (freq[TMOD] & 0x0FF) | ((val & 0x0F) << 8);
        mod_halt = ((val & 0x80) != 0);
        if (mod_halt)
            phase[TMOD] = phase[TMOD] & 0x3F0000; // reset accumulator phase
        return true;
    case 0x88: // $4088 mod table write
        if (mod_halt)
        {
            // writes to current playback position (there is no direct way to set phase)
            wave[TMOD][(phase[TMOD] >> 16) & 0x3F] = val & 0x07;
            phase[TMOD] = (phase[TMOD] + 0x010000) & 0x3FFFFF;
            wave[TMOD][(phase[TMOD] >> 16) & 0x3F] = val & 0x07;
            phase[TMOD] = (phase[TMOD] + 0x010000) & 0x3FFFFF;
            mod_write_pos = phase[TMOD] >> 16; // used by OPT_4085_RESET
        }
        return true;
    case 0x89: // $4089 wave write enable, master volume
        wav_write = ((val & 0x80) != 0);
        master_vol = val & 0x03;
        return true;
    case 0x8A: // $408A envelope speed
        master_env_speed = val;
        // haven't tested whether this register resets phase on hardware,
        // but this ensures my inplementation won't spam envelope clocks
        // if this value suddenly goes low.
        env_timer[EMOD] = 0;
        env_timer[EVOL] = 0;
        return true;
//    default:
//        return false;
    }
    return false;
}

bool NES_FDS::Read (uint32_t adr, uint32_t & val)
{
    if (adr >= 0x4040 && adr <= 0x407F)
    {
        // TODO: if wav_write is not enabled, the
        // read address may not be reliable? need
        // to test this on hardware.
        val = wave[TWAV][adr - 0x4040];
        return true;
    }

    if (adr == 0x4090) // $4090 read volume envelope
    {
        val = env_out[EVOL] | 0x40;
        return true;
    }

    if (adr == 0x4092) // $4092 read mod envelope
    {
        val = env_out[EMOD] | 0x40;
        return true;
    }

    return false;
}

} // namespace
