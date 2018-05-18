/*
The MIT License (MIT)

Copyright (c) 2014 Mitsutaka Okazaki

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#ifndef _EMU2413_H_
#define _EMU2413_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PI 3.14159265358979323846

enum OPLL_TONE_ENUM {OPLL_2413_TONE=0, OPLL_VRC7_TONE=1, OPLL_281B_TONE=2} ;

/* voice data */
typedef struct __OPLL_PATCH {
  uint32_t TL,FB,EG,ML,AR,DR,SL,RR,KR,KL,AM,PM,WF ;
} OPLL_PATCH ;

/* slot */
typedef struct __OPLL_SLOT {

  OPLL_PATCH *patch;

  int32_t type ;          /* 0 : modulator 1 : carrier */

  /* OUTPUT */
  int32_t feedback ;
  int32_t output[2] ;   /* Output value of slot */

  /* for Phase Generator (PG) */
  uint16_t *sintbl ;    /* Wavetable */
  uint32_t phase ;      /* Phase */
  uint32_t dphase ;     /* Phase increment amount */
  uint32_t pgout ;      /* output */

  /* for Envelope Generator (EG) */
  int32_t fnum ;          /* F-Number */
  int32_t block ;         /* Block */
  int32_t volume ;        /* Current volume */
  int32_t sustine ;       /* Sustine 1 = ON, 0 = OFF */
  uint32_t tll ;	      /* Total Level + Key scale level*/
  uint32_t rks ;        /* Key scale offset (Rks) */
  int32_t eg_mode ;       /* Current state */
  uint32_t eg_phase ;   /* Phase */
  uint32_t eg_dphase ;  /* Phase increment amount */
  uint32_t egout ;      /* output */

} OPLL_SLOT ;

/* Mask */
#define OPLL_MASK_CH(x) (1<<(x))
#define OPLL_MASK_HH (1<<(9))
#define OPLL_MASK_CYM (1<<(10))
#define OPLL_MASK_TOM (1<<(11))
#define OPLL_MASK_SD (1<<(12))
#define OPLL_MASK_BD (1<<(13))
#define OPLL_MASK_RHYTHM ( OPLL_MASK_HH | OPLL_MASK_CYM | OPLL_MASK_TOM | OPLL_MASK_SD | OPLL_MASK_BD )

/* opll */
typedef struct __OPLL {

  uint32_t adr ;
  int32_t out ;

  uint32_t realstep ;
  uint32_t oplltime ;
  uint32_t opllstep ;
  uint32_t pan[16];

  /* Register */
  uint8_t reg[0x40] ;
  int32_t slot_on_flag[18] ;

  /* Pitch Modulator */
  uint32_t pm_phase ;
  int32_t lfo_pm ;

  /* Amp Modulator */
  int32_t am_phase ;
  int32_t lfo_am ;

  uint32_t quality;

  /* Noise Generator */
  uint32_t noise_seed ;

  /* Channel Data */
  int32_t patch_number[9];
  int32_t key_status[9] ;

  /* Slot */
  OPLL_SLOT slot[18] ;

  /* Voice Data */
  OPLL_PATCH patch[19*2] ;
  int32_t patch_update[2] ; /* flag for check patch update */

  uint32_t mask ;

  /* Output of each channels / 0-8:TONE, 9:BD 10:HH 11:SD, 12:TOM, 13:CYM, 14:Reserved for DAC */
  int16_t ch_out[15];

} OPLL ;

/* Create Object */
OPLL *OPLL_new(uint32_t clk, uint32_t rate) ;
void OPLL_delete(OPLL *) ;

/* Setup */
void OPLL_reset(OPLL *) ;
void OPLL_reset_patch(OPLL *, int32_t) ;
void OPLL_set_rate(OPLL *opll, uint32_t r) ;
void OPLL_set_quality(OPLL *opll, uint32_t q) ;
void OPLL_set_pan(OPLL *, uint32_t ch, uint32_t pan);

/* Port/Register access */
void OPLL_writeIO(OPLL *, uint32_t reg, uint32_t val) ;
void OPLL_writeReg(OPLL *, uint32_t reg, uint32_t val) ;

/* Synthsize */
int16_t OPLL_calc(OPLL *) ;
void OPLL_calc_stereo(OPLL *, int32_t out[2]) ;

/* Misc */
void OPLL_setPatch(OPLL *, const uint8_t *dump) ;
void OPLL_copyPatch(OPLL *, int32_t, OPLL_PATCH *) ;
void OPLL_forceRefresh(OPLL *) ;
/* Utility */
void OPLL_dump2patch(const uint8_t *dump, OPLL_PATCH *patch) ;
void OPLL_patch2dump(const OPLL_PATCH *patch, uint8_t *dump) ;
void OPLL_getDefaultPatch(int32_t type, int32_t num, OPLL_PATCH *) ;

/* Channel Mask */
uint32_t OPLL_setMask(OPLL *, uint32_t mask) ;
uint32_t OPLL_toggleMask(OPLL *, uint32_t mask) ;

int16_t OPLL_getchanvol(int i);

#ifdef __cplusplus
}
#endif

#endif

