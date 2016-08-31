#ifndef AOE_MENU_H
#define AOE_MENU_H

#include "gfx.h"

struct menu_ctl;
struct game3F4;

struct sgo498 {
	unsigned tbl[5];
};

struct menu_ctl_arg {
	struct menu_ctl *menu_ref;
	unsigned num4;
	unsigned *num8;
};

struct menu_ctl_vtbl {
	struct menu_ctl *(*dtor)(struct menu_ctl*, char);
	unsigned ptr[4];
	void (*func10)(struct menu_ctl*, int);
	unsigned ptr14;
	void (*func18)(struct menu_ctl*, int, int, int, int, int, int, int, int, int, int, int, int, int);
	unsigned ptr1C[13];
	void (*func50)(struct menu_ctl*, int, int);
	unsigned ptr54[37];
	void (*funcE8)(struct menu_ctl*, int, int);
	unsigned funcEC;
	int (*funcF0)(struct menu_ctl*, struct menu_ctl*, int, char *, char *, int, int, int, int, int, int, int);
	unsigned ptrF4[64];
};

// XXX derived from video_mode?
struct menu_ctl {
	struct menu_ctl_vtbl *vtbl;
	unsigned array[6];
	char *title;
	struct basegame *ref_start_arg;
	struct rect rect;
	char gap34[12];
	void *ptr40;
	unsigned tbl44[4];
	struct menu_ctl_arg *menu_arg;
	unsigned tbl58[5];
	unsigned dirty;
	unsigned tbl70[11];
	unsigned x;
	unsigned y;
	unsigned x2;
	unsigned y2;
	unsigned width;
	unsigned height;
	unsigned width2;
	unsigned height2;
	unsigned tblBC[12];
	char gapEC[3];
	char chEF;
	char gapF0[4];
	unsigned width3;
	unsigned height3;
	char str_screen_ref[260];
	unsigned res_id;
	unsigned dword204;
	unsigned dword208;
	unsigned dword20C;
	char str210[260];
	unsigned dword314;
	unsigned dword318;
	unsigned dword31C;
	char byte320;
	char byte321;
	char byte322;
	char byte323;
	char byte324;
	char byte325;
	unsigned dword328;
	unsigned dword32C;
	unsigned dword330;
	unsigned dword334;
	unsigned dword338;
	unsigned dword33C;
	char byte340;
	char gap341[259];
	unsigned dword444;
	unsigned dword448;
	unsigned dword44C;
	unsigned dword450;
	char byte454;
	char byte455;
	unsigned dword458;
	char byte45C;
	unsigned dword460;
	unsigned dword464;
	unsigned dword468;
	unsigned dword46C;
	unsigned num470;
	unsigned dword474;
	void *ptr478;
	unsigned num47C;
	unsigned num480;
	unsigned tbl484[4];
	unsigned num494;
	struct sgo498 obj498;
	unsigned pad49C[7];
	unsigned num4C8;
	char blk4CC[64];
	unsigned pad50C[3];
	char blk518[40];
	unsigned pad540[7];
	char blk55C[32];
	unsigned pad584[14];
	struct game3F4 *ptr5B4;
	unsigned cfg888num;
	unsigned time5BC;
	unsigned num5C0;
	unsigned time5C4;
	unsigned pad5C8[127];
	unsigned num7C4;
};

struct menu_ctl *menu_ctl_init_title(struct menu_ctl *this, const char *title);
struct menu_ctl *menu_ctl_dtor(struct menu_ctl *this, char a2);
struct menu_ctl *menu_ctl_dtor_stdiobuf(struct menu_ctl *this, char call_this_dtor);

#endif
