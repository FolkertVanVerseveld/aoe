#ifndef AOE_SFX_H
#define AOE_SFX_H

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

#endif
