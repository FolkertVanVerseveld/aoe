#ifndef AOE_GAME_H
#define AOE_GAME_H

#include <stdint.h>
#include "engine.h"
#include "shp.h"
#include "map.h"
#include "gfx.h"
#include "sfx.h"
#include "comm.h"
#include "log.h"

#define GAME_LOGCTL_FILE 1
#define GAME_LOGCTL_STDOUT 2

#define GE_LIB 1
#define GE_OPT 2
#define GE_TIME 3
#define GE_FOCUS 4
#define GE_ICON 5
#define GE_FULLSCREEN 6
#define GE_GFX 7
#define GE_MOUSE 8
#define GE_WINCTL2 9
#define GE_SFX 10
#define GE_CTL 11
#define GE_SFX2 12
#define GE_WINCTL 16
#define GE_PALETTE 17
#define GE_LOWOS 20
#define GE_LOWMEM 22

struct window_ctl2 {
	unsigned num0, num4, num8, numC;
};

struct game_drive {
	unsigned driveno;
	char buf[256];
	unsigned driveno2;
	char buf2[64];
	char gap[300];
};

struct game;

struct game_vtbl {
	struct game *(*dtor)(struct game*, char);
	unsigned (*main)(struct game*);
	int (*process_intro)(struct game*, unsigned, unsigned, unsigned, unsigned);
	unsigned (*set_rpair)(struct game*, unsigned, int);
	struct regpair *(*set_rpair_next)(struct game*, struct regpair*, int);
	unsigned (*get_state)(struct game*);
	char *(*strerr)(struct game*, int, int, int, char*, unsigned);
	char *(*get_res_str)(unsigned, char*, unsigned);
	char *(*strerr2)(struct game*, int, int, int, char*, unsigned);
	int (*func)(struct game*);
	int (*chat_send)(struct game*, unsigned, char*);
	int (*parse_opt)(struct game*);
	int (*init_icon)(struct game*);
	int (*go_fullscreen)(struct game*);
	int (*gfx_init)(struct game*);
	int (*set_palette)(struct game*);
	int (*init_custom_mouse)(struct game*);
	int (*window_ctl)(struct game*);
	int (*window_ctl2)(struct game*);
	int (*init_sfx)(struct game*);
	int (*gfx_ctl)(struct game*);
	int (*init_sfx_tbl)(struct game*);
	int (*shp)(struct game*);
	int (*repaint)(struct game*);
	int (*set_ones)(struct game*);
	// XXX consider inlining
	int (*translate_event)(struct game*, int);
	void (*handle_event)(struct game*, unsigned);
	int (*cfg_apply_video_mode)(struct game*, unsigned, int, int, unsigned);
	struct map *(*map_save_area)(struct game*);
	int (*init_mouse)(struct game*);
};

extern struct game_vtbl g_vtbl, g_vtbl2;

#define OPTBUFSZ 256

// FIXME change char arrays to const char* if not modified anywhere
struct game_config {
	char title[48];
	unsigned reg_state;
	char version[21];
	char win_name[121];
	char tr_world_txt[137];
	unsigned window, gl;
	char langpath[32];
	char dir_empires[261];
	char strAOE[263];
	int d0, d1p0, d3;
	unsigned hInst, hPrevInst;
	char reg_path[256];
	char optbuf[OPTBUFSZ];
	unsigned nshowcmd;
	char icon[41];
	char menu_name[41];
	char palette[261];
	char cursors[251];
	unsigned d8p0;
	unsigned chk_time;
	short time[3];
	unsigned d1p1;
	unsigned window_request_focus;
	unsigned no_start;
	unsigned window_query_dd_interface;
	unsigned mouse_opts;
	unsigned gfx8bitchk;
	unsigned sys_memmap;
	unsigned midi_enable;
	unsigned sfx_enable;
	unsigned midi_opts[7];
	int scrollspeed;
	int scroll0;
	float f4_0p0;
	int scroll1;
	float f4_0p1, f0_5;
	short mouse_style;
	unsigned width, height;
	struct window_ctl2 winctl, winctl2;
	char dir_data2[244];
	//HKEY root;
	float gamespeed;
	unsigned difficulty;
	char pathfind;
	char dir_empty[261], dir_save[261];
	char dir_scene[53], dir_camp[52], dir_sound[261];
	char dir_data_2[261], dir_data_3[261];
	char dir_movies[261];
	char prompt_title[256], prompt_message[256];
};

struct game8 {
	char pathname[260];
	char *inf_hdr;
	void *ptr;
};

#define CWDBUFSZ 261

struct game {
	struct game_vtbl *vtbl;
	struct game_focus *focus;
	struct game8 *ptr8;
	struct game_config *cfg;
	unsigned window;
	unsigned num14;
	unsigned running;
	// REMAP typeof(HPALETTE palette) == drs_pal*
	struct drs_pal *palette;
	// REMAP typeof(HANDLE mutex) == void*
	unsigned mutex_main_error;
	unsigned num24;
	unsigned tbl28[4];
	unsigned screensaver_state;
	// REMAP typeof(PVOID powersaving) == void*
	unsigned powersaving;
	// REMAP typeof(GERRNO state) == unsigned
	unsigned state;
	unsigned timer;
	struct video_mode *mode;
	struct basegame *basegame;
	char ch50;
	char pad51[3];
	unsigned no_normal_mouse;
	unsigned short shp_count;
	// NOPTR typeof(ship_20 *shptbl) == ship_20[3]
	struct ship_20 shptbl[3];
	struct sfx_engine *sfx;
	unsigned num64;
	short sfx_count;
	struct clip **sfx_tbl;
	char ch70;
	unsigned tbl74[3];
	char ch80;
	unsigned tbl84[14];
	char gapBC[108];
	unsigned cursor_old_state;
	// REMAP typeof(HCURSOR cursor_old) == void*
	char tbl88[4];
	unsigned cursor_old;
	char tbl90[80];
	unsigned tbl184[2];
	struct comm *nethandler;
	unsigned tbl190[5];
	struct game18C *ptr18C;
	struct logger *log;
	unsigned logctl;
	struct regpair rpair_root;
	union reg_value rpair_rootval;
	unsigned num1BC;
	struct cursor_cfg *curscfg;
	unsigned num1C4;
	unsigned midi_sync;
	unsigned cursor_state;
	unsigned apply_new_cursor;
	// REMAP typeof(HCURSOR cursor) == void*
	unsigned cursor;
	unsigned num1D8;
	unsigned window2;
	short num1E0;
	char pad1E2[2];
	unsigned num1E4;
	char cwdbuf[261];
	char libname[15];
	char pad2FC[248];
	struct game3F4 *ptr3F4;
	unsigned int num3F8;
	short tbl3FC[3];
	short num402;
	char ch404;
	char pad405;
	char pad406[106];
	unsigned int tbl470[37];
	unsigned int tbl504[249];
	unsigned int tbl8E8[3];
	unsigned int num8F4;
	float start_gamespeed;
	char c0;
	char str8FD[128];
	char num97D_97E_is_zero;
	char num97E_97D_is_zero;
	char hsv[3];
	char cheats;
	char mp_pathfind;
	char ch984;
	char ch985;
	char ch986;
	char ch987;
	char ch988;
	char ch989;
	char player_tbl[9];
	unsigned difficulty2;
	char tbl994[12];
	unsigned num9A0;
	unsigned num9A4;
	unsigned vtbl9A8;
	unsigned num9AC;
	unsigned tbl9B0[9];
	unsigned state2;
	char pad9D8[36];
	unsigned num9FC;
	char padA00[8];
	unsigned rollover_text;
	float gamespeed;
	unsigned difficulty;
	char pathfind;
	char tblA15[11];
	struct map *map_area;
	unsigned tblA24[4];
	unsigned brightness;
	unsigned numA80;
	unsigned numA84;
	unsigned numA88;
	unsigned numA8C;
	unsigned numA90;
	char tblA94[12];
	unsigned tblAA0[9];
	char tblAC4[9];
	char tblACD[9];
	char chAD6;
	char chAD7;
	char chAD8;
	char chAD9;
	unsigned numADC;
	unsigned numAE0;
	char chAE4;
	char chAE5;
	char chAE6;
	char chAE7;
	char chAE8;
	char padAE9[259];
	// REMAP typeof(HWND hwnd_mci) == unsigned
	unsigned hwnd_mci;
	unsigned tblBF0[3];
	unsigned time_mci;
	unsigned tblC00[22];
	char chC58;
	char padC59[259];
	char chD5C;
	char padD5D[259];
	unsigned numE60;
	char chE64;
	char padE65[259];
	char chF68;
	char padF69[259];
	char ch106C;
	char pad106D[255];
	char blk116C[36];
	char ch1190;
	char ch1191[3];
	unsigned num1194;
	char blk1198[160];
	char pad1238[64];
	int num1250;
};

extern struct colpalette game_col_palette;

struct game *game_ctor(struct game *this, struct game_config *cfg, int should_start_game);

#endif
