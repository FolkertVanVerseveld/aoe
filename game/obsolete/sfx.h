/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef AOE_SFX_H
#define AOE_SFX_H

#define SFX_PLAY_LOOP 1

#define SFX_BTN1     50300
#define SFX_BTN2     50301
#define SFX_CHATRECV 50302
#define SFX_CANTDO   50303
#define SFX_TRIBUTE  50304
#define SFX_RESDONE  50305
#define SFX_PKILLED  50306
#define SFX_PDROPPED 50307
#define SFX_MSTART   50308
#define SFX_MDONE    50309
#define SFX_MKILLED  50310
#define SFX_ARTHELD  50311
#define SFX_ARTLOST  50312
#define SFX_CONVWARN 50313
#define SFX_CONVDONE 50314
#define SFX_RES      50315
#define SFX_FARMDIE  50316

struct sfx_engine;

struct clip {
	struct sfx_engine *sfx;
	unsigned dword4;
	char byte8;
	char byte9;
	char name[13];
	char gap17;
	unsigned str_id;
	unsigned dword1C;
	unsigned dword20;
	unsigned dword24;
	unsigned dword28;
	unsigned dword2C;
	unsigned dword30;
	unsigned dword34;
	unsigned dword38;
};

struct sfx_engine {
	char byte0;
	char byte1;
	unsigned dword4;
	unsigned dword8;
	unsigned dwordC;
	unsigned mixer_mm_state;
	unsigned dword14;
	unsigned dword18;
	char byte1C;
	char gap1D[259];
	unsigned dword120;
	// REMAP typeof(HMIXEROBJ mixer) == unsigned
	unsigned mixer;
	char gap128[1392];
	unsigned dword698;
};

int clip4A3440(struct clip *this);
int clip_ctl(struct clip *this);

int sfx_init(void);
void sfx_free(void);

#define MUSIC_MAIN    0
#define MUSIC_BKG1    1
#define MUSIC_BKG2    2
#define MUSIC_BKG3    3
#define MUSIC_BKG4    4
#define MUSIC_BKG5    5
#define MUSIC_BKG6    6
#define MUSIC_BKG7    7
#define MUSIC_BKG8    8
#define MUSIC_BKG9    9
#define MUSIC_WON    10
#define MUSIC_LOST   11
#define MUSIC_XMAIN  12
#define MUSIC_XBKG1  13
#define MUSIC_XBKG2  14
#define MUSIC_XBKG3  15
#define MUSIC_XBKG4  16
#define MUSIC_XBKG5  17
#define MUSIC_XBKG6  18
#define MUSIC_XBKG7  19
#define MUSIC_XBKG8  20
#define MUSIC_XBKG9  21
#define MUSIC_XBKG10 22
#define MUSIC_XWON   23
#define MUSIC_XLOST  24

int sfx_play(unsigned id, unsigned flags, float volume);

#endif
