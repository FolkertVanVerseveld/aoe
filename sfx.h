#ifndef AOE_SFX_H
#define AOE_SFX_H

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
