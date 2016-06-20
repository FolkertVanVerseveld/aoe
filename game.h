#ifndef AOE_GAME_H
#define AOE_GAME_H

#include <stdint.h>
#include "log.h"

struct game;

struct regpair {
	unsigned root, value;
	struct regpair *next;
};

struct game_vtbl {
	int (*dtor_io)(void*,char);
	unsigned (*main)(struct game*);
	// XXX consider inlining
	unsigned (*get_state)(struct game*);
	char *(*get_res_str)(unsigned, char*, int);
	char *(*strerr)(struct game*, int, signed, int, char*, int);
	int (*parse_opt)(struct game*);
};

extern struct game_vtbl g_vtbl, g_vtbl2;

#define OPTBUFSZ 256

// FIXME change char arrays to const char* if not modified anywhere
struct game_cfg {
	char title[48];
	unsigned reg_state;
	char version[21];
	char win_name[121];
	char tr_world_txt[137];
	unsigned window;
	unsigned gl;
	char dir_empires[261];
	unsigned num404, num408, num40C;
	unsigned hInst, hPrevInst;
	char reg_path[256];
	char optbuf[OPTBUFSZ];
	unsigned nshowcmd;
	char icon[41];
	char menu_name[567];
	char palette[256];
	char cursors[256];
	unsigned num878, num87C;
	short tbl880[3];
	signed num888, num88C;
	unsigned no_start;
	unsigned mouse_opts[3];
	unsigned sys_memmap;
	unsigned midi_enable;
	unsigned sfx_enable;
	unsigned midi_opts[7];
	unsigned scroll0;
	float num8CC;
	unsigned scroll1;
	float num8D4;
	float num8D8;
	unsigned mouse_style;
	unsigned width, height;
	unsigned num8E8;
	signed tbl8EC[3];
	unsigned num8F8;
	signed tbl8FC[3];
	char dir_data2[244];
	float gamespeed;
	unsigned difficulty;
	char pathfind, mp_pathfind;
	char dir_empty[261], dir_save[261], dir_scene[53];
	char dir_camp[52], dir_sound[261];
	char dir_data_2[261], dir_data_3[261];
	char dir_movies[261];
	char prompt_title[256], prompt_message[256];
};

#define CWDBUFSZ 261

struct game {
	struct game_vtbl *vtbl;
	unsigned num4, num8;
	struct game_cfg cfg;
	unsigned window;
	unsigned num14;
	unsigned running;
	unsigned num1C, num24;
	unsigned tbl28[4];
	unsigned num38, powersaving;
	unsigned state;
	unsigned tbl44[3];
	uint8_t ch50;
	unsigned no_normal_mouse;
	unsigned tbl58[4];
	unsigned num68, num6C;
	uint8_t ch70;
	unsigned tbl74[3];
	char tbl80[260];
	unsigned tbl184[8];
	struct logger log;
	unsigned logctl;
	struct regpair rpair;
	unsigned rpair_rootval;
	unsigned tbl1BC[3];
	unsigned midi_sync;
	unsigned num1CC, num1D0;
	unsigned num1D8, num1DC;
	unsigned cursor;
	unsigned num1E0, num1E4;
	char cwdbuf[CWDBUFSZ];
	char libname[15];
	unsigned num3F4, num3F8;
	short tbl3FC[3];
	short num402;
	uint8_t ch404;
	unsigned tbl504[9];
	unsigned tbl528[240];
	unsigned tbl8E8[3];
	unsigned num8F4;
	float num8F8;
	char ch8FC;
	char str8FD[128];
	unsigned num97D_97E_is_zero, num97E_97D_is_zero;
	uint8_t hsv[3];
	uint8_t ch982, ch984, ch985, ch986, ch987, ch988, ch989;
	uint8_t tbl98A[9];
	uint8_t ch993;
	uint8_t tbl994[9];
	unsigned num9A0, num9A4, num9A8, num9AC;
	unsigned tbl9B0[9];
	unsigned num9D4;
	unsigned num9F4;
	unsigned rollover_text;
	float gamespeed;
	unsigned difficulty;
	unsigned brightness;
	uint8_t tblA14[12];
	unsigned numA20;
	unsigned tblA24[4];
	unsigned numA80, numA84, numA88, numA8C, numA90;
	char tblAD6[6];
	unsigned numADC, numAE0;
	char chAE4, chAE8;
	unsigned tblBEC[20];
	char chC58, chD5C, chE64, chF68;
	unsigned numE60;
	char ch106C;
	int num1250;
};

struct game *game_ctor(struct game *this, int should_start_game);

#endif
