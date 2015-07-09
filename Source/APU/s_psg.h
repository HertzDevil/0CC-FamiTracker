#ifndef S_PSG_H__
#define S_PSG_H__

#include "s_logtbl.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	PSG_TYPE_AY_3_8910	= 0,	/* YAMAHA AY-3-8910 */
	PSG_TYPE_YM2149		= 1		/* YAMAHA YM2149 */
};

typedef struct {
#if DCFIX
	uint32 dcbuf[1 << DCFIX];
	uint32 dcave;
	uint32 dcptr;
#endif
	uint32 cps;
	uint32 cycles;
#if ANAEX
	int32 anaex[2];
#endif
	uint8 regs[3];
	uint8 adr;
	uint8 mute;
	uint8 key;
	uint32 count;
	int32 output;
} PSG_SQUARE;

typedef struct {
	uint32 cps;
	uint32 cycles;
	uint32 noiserng;
	uint8 regs[1];
	uint8 count;
	uint32 output;
} PSG_NOISE;

typedef struct {
	int32 cps;
	int32 cycles;
} MSX_DA;

typedef struct {
	uint32 cps;
	uint32 cycles;
	const int8 *adr;
	uint8 regs[3];
	uint8 adrmask;
} PSG_ENVELOPE;

typedef struct {
	void *ctx;
	void (*release)(void *ctx);
	void (*reset)(void *ctx, uint32 clock, uint32 freq);
	void (*synth)(void *ctx, int32 *p);
	void (*volume)(void *ctx, int32 v);
	void (*write)(void *ctx, uint32 a, uint32 v);
	uint32 (*read)(void *ctx, uint32 a);
	void (*setinst)(void *ctx, uint32 n, void *p, uint32 l);
#if 0
	void (*setrate)(void *ctx, Uint32 clock, Uint32 freq);
	void (*getinfo)(void *ctx, KMCH_INFO *cip, );
	void (*volume2)(void *ctx, Uint8 *volp, Uint32 numch);
	/* 0x00(mute),0x70(x1/2),0x80(x1),0x90(x2) */
#endif
} KMIF_SOUND_DEVICE;

KMIF_SOUND_DEVICE *PSGSoundAlloc(uint32 psg_type);

typedef struct {
	KMIF_SOUND_DEVICE kmif;
	KMIF_LOGTABLE *logtbl;
	PSG_SQUARE square[3];
	PSG_ENVELOPE envelope;
	PSG_NOISE noise;
	MSX_DA da;
	struct {
		int32 mastervolume;
		uint32 davolume;
		uint32 envout;
		uint8 daenable;
		uint8 regs[1];
		uint32 rngout;
		uint8 adr;
	} common;
	uint8 type;
	uint8 regs[0x10];
} PSGSOUND;

void __fastcall sndsynth(void *ctx, int32 *p);
void __fastcall sndvolume(void *ctx, int32 volume);
uint32 __fastcall sndread(void *ctx, uint32 a);
void __fastcall sndwrite(void *ctx, uint32 a, uint32 v);
void __fastcall sndreset(void *ctx, uint32 clock, uint32 freq);
void __fastcall sndrelease(void *ctx);


#ifdef __cplusplus
}
#endif

#endif /* S_PSG_H__ */
