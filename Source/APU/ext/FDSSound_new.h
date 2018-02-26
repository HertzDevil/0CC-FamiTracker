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

// // // new FDS emulation core taken straight from rainwarrior's NSFPlay

namespace xgm {

inline const double DEFAULT_CLOCK = 1789772.0;
inline const int DEFAULT_RATE = 48000;

class NES_FDS
{
public:
    enum
    {
        OPT_CUTOFF=0,
        OPT_4085_RESET,
        OPT_WRITE_PROTECT,
        OPT_END
    };

protected:
    double rate, clock;
    int32_t fout; // current output
    int option[OPT_END];

    bool master_io;
    unsigned master_vol;
    uint32_t last_freq; // for trackinfo
    uint32_t last_vol;  // for trackinfo

    // two wavetables
    const enum { TMOD=0, TWAV=1 };
    int32_t wave[2][64];
    uint32_t freq[2];
    uint32_t phase[2];
    bool wav_write;
    bool wav_halt;
    bool env_halt;
    bool mod_halt;
    uint32_t mod_pos;
    uint32_t mod_write_pos;

    // two ramp envelopes
    const enum { EMOD=0, EVOL=1 };
    bool env_mode[2];
    bool env_disable[2];
    uint32_t env_timer[2];
    uint32_t env_speed[2];
    uint32_t env_out[2];
    uint32_t master_env_speed;

    // 1-pole RC lowpass filter
    int32_t rc_accum;
    int32_t rc_k;
    int32_t rc_l;

public:
    NES_FDS ();
    ~ NES_FDS ();

    void Reset ();
    void Tick (uint32_t clocks);
    int32_t Render ();		// // //
    bool Write (uint32_t adr, uint32_t val);
    bool Read (uint32_t adr, uint32_t & val);
    void SetRate (double);
    void SetClock (double);
    void SetOption (int, int);
};

} // namespace xgm
