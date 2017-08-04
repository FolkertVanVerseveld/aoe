/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef AOE_GAME_H
#define AOE_GAME_H

#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include "config.h"
#include "engine.h"
#include <genie/shp.h>
#include "map.h"
#include "gfx.h"
#include "sfx.h"
#include "comm.h"
#include "log.h"

#define GAME_LOGCTL_FILE   1
#define GAME_LOGCTL_STDOUT 2

#define ACT_WORK          1
#define ACT_MOVE          2
#define ACT_BUILD         3
#define ACT_TRADE         4
#define ACT_STOP          5
#define ACT_UNSELECT      6
#define ACT_UNLOAD        7
#define ACT_GROUP         8
#define ACT_UNGROUP       9
#define ACT_FORMATION     10
#define ACT_CANCEL        11
#define ACT_NEXT          12
#define ACT_CHAT          13
#define ACT_DIPLOMACY     14
#define ACT_MENU          15
#define ACT_TRADE_WITH    16
#define ACT_RESEARCH      17
#define ACT_CREATE        18
#define ACT_BUILD2        19
#define ACT_BUILD_ABORT   20
#define ACT_HELP          21
#define ACT_STANCE_GROUND 22
#define ACT_STANCE_ATTACK 23
// XXX 24 missing
#define ACT_TRADE_FOOD    25
#define ACT_TRADE_WOOD    26
#define ACT_TRADE_STONE   27
#define ACT_HEAL          28
#define ACT_CONVERT       29
#define ACT_ATTACK        30
#define ACT_REPAIR        31
#define ACT_QUEUE_INC     32
#define ACT_QUEUE_DEC     33

#define GE_LIB        1
#define GE_OPT        2
#define GE_TIME       3
#define GE_FOCUS      4
#define GE_ICON       5
#define GE_FULLSCREEN 6
#define GE_GFX        7
#define GE_MOUSE      8
#define GE_WINCTL2    9
#define GE_SFX        10
#define GE_CTL        11
#define GE_SFX2       12
#define GE_WINCTL     16
#define GE_PALETTE    17
#define GE_LOWOS      20
#define GE_LOWMEM     22

#define TYPE_FAIL  1
#define TYPE_RES   100
#define TYPE_AGE   101
#define TYPE_ACT   102
#define TYPE_FAIL2 103
#define TYPE_ATTR  104
#define TYPE_CIV   105

#define FAIL_START      100
#define FAIL_START2     101
#define FAIL_START3     102
#define FAIL_SAVE_GAME  103
#define FAIL_SAVE_SCENE 104
#define FAIL_LOAD_GAME  105
#define FAIL_LOAD_SCENE 106

#define RES_FOOD  0
#define RES_WOOD  1
#define RES_STONE 2
#define RES_GOLD  3
#define RES_GOODS 9
#define RES_FOOD2 15
#define RES_FOOD3 16
#define RES_FOOD4 17

#define AGE_STONE  1
#define AGE_TOOL   2
#define AGE_BRONZE 3
#define AGE_IRON   4

#define CIV_EGYPT        1
#define CIV_GREEK        2
#define CIV_BABYLONIAN   3
#define CIV_ASSYRIAN     4
#define CIV_MINOAN       5
#define CIV_HITTITE      6
#define CIV_PHOENICIAN   7
#define CIV_SUMERIAN     8
#define CIV_PERSIAN      9
#define CIV_SHANG        10
#define CIV_YAMATO       11
#define CIV_CHOSON       12
#define CIV_ROMAN        13
#define CIV_CARTHAGINIAN 14
#define CIV_PALMYRIAN    15
#define CIV_MACEDONIAN   16

#define LOW_FOOD    0
#define LOW_WOOD    1
#define LOW_STONE   2
#define LOW_GOLD    3
#define LOW_POP     4
#define LOW_POP_MAX 32

#define ATTR_LOAD     1
#define ATTR_HEALTH   2
#define ATTR_ARMOR    3
#define ATTR_ATTACK   4
#define ATTR_RESEARCH 5
#define ATTR_TRAIN    6
#define ATTR_BUILD    7
#define ATTR_SIGHT    8
#define ATTR_POP      9
#define ATTR_RANGE    10
#define ATTR_SPEED    11

#define SHIP_TRADE  101
#define SHIP_LAND   102
#define SHIP_UNLOAD 103

struct window_ctl2 {
	unsigned num0, num4, num8, numC;
};

struct game;

extern struct game *game_ref;

struct game_vtbl {
	struct game *(*dtor)(struct game*, char);
	unsigned (*main)(struct game*);
	int (*process_intro)(struct game*, unsigned, unsigned, unsigned, unsigned);
	unsigned (*set_rpair)(struct game*, unsigned);
	struct regpair *(*set_rpair_next)(struct game*, struct regpair*, int);
	struct game3F4 *(*menu_init)(struct game*, int a2);
	unsigned (*get_state)(struct game*);
	const char *(*strerr)(struct game*, int, int, int, char*, unsigned);
	char *(*get_res_str)(unsigned, char*, unsigned);
	char *(*res_buf_str)(struct game*, unsigned);
	const char *(*strerr2)(struct game*, int, int, int, char*, unsigned);
	int (*scenario_stat)(struct game*);
	int (*scenario_stat2)(struct game*);
	struct game15C *(*g15C_init)(int a1);
	struct game15C *(*g15C_init2)(void*, size_t);
	struct game15C_obj *(*init_game15C_obj)(int);
	int (*process_message)(struct game*, int, int, int, int, int);
	int (*no_msg_slot)(struct game*);
	int (*comm_opt_grow)(struct game*);
	void (*comm_settings_ctl)(struct game*);
	char *(*get_ptr)(void);
	int (*cheat_ctl)(struct game*, int, const char*);
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
	int (*translate_event)(struct game*, SDL_Event*);
	void (*handle_event)(struct game*, unsigned);
	int (*cfg_apply_video_mode)(struct game*, SDL_Window*, int, int, int);
	unsigned tblEC[14];
	unsigned tbl128[7];
	struct map *(*map_save_area)(struct game*);
	int (*init_mouse)(struct game*);
	unsigned tbl14C[4];
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
	SDL_Window *window;
	SDL_GLContext *gl;
	char langpath[32];
	char dir_empires[261];
	char strAOE[263];
	int d0;
	int d1p0;
	int d3;
	SDL_Window *hInst;
	SDL_Window *hPrevInst;
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

struct game15C_vtbl {
	unsigned ptr[1];
};

struct game15C {
	struct game15C_vtbl *vtbl;
	time_t timers[3];
	unsigned tbl10[3];
};

struct game15C_obj {
	unsigned vtbl;
	char gap4[8];
	char byteC;
	char charD;
	char gapE[6266];
	float float1888;
	char buf188C[260];
	char buf1990[256];
	char buf1A90[24];
	char buf1AA8[11519];
	char buf47A8[1024];
	char buf4BA8[64];
	unsigned num4BE8;
	unsigned num4BEC;
	int dword4BF0;
	char buf4BF4[1280];
	unsigned num50F4;
	int dword50F8;
	int dword50FC;
	char buf5100[64];
	unsigned num5140;
	unsigned num5144;
	unsigned num5148;
};

struct game434 {
	char gap0[8];
	unsigned num8;
	unsigned numC;
	char gap10[36220];
	unsigned num8D8C;
};

struct game3F4_vtbl {
	unsigned ptr[40];
	void (*func)(int);
};

struct game3F4 {
	struct game3F4 *vtbl;
	char pad4[36];
	struct game434 *ptr;
	char pad2C[16];
	short player_count;
	char pad3E[62];
	short player_id;
};

#define CWDBUFSZ 261

struct game {
	struct game_vtbl *vtbl;
	struct game_focus *focus;
	struct game8 *ptr8;
	struct game_config *cfg;
	SDL_Window *window;
	unsigned num14;
	unsigned running;
	// REMAP typeof(HPALETTE palette) == pal_entry*
	struct pal_entry *palette;
	pthread_mutex_t mutex_main_error; int mutex_init;
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
	// NOPTR typeof(shp *shptbl) == shp[3]
	struct shp shptbl[3];
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
	char tbl88[4];
	// REMAP typeof(HCURSOR cursor_old) == SDL_Cursor*
	SDL_Cursor *cursor_old;
	char tbl90[80];
	unsigned tbl184[2];
	struct comm *nethandler;
	unsigned tbl190[5];
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
	// REMAP typeof(HCURSOR cursor) == SDL_Cursor*
	SDL_Cursor *cursor;
	unsigned num1D8;
	SDL_Window *window2;
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
	struct game_settings settings;
	unsigned num9A4;
	unsigned vtbl9A8;
	unsigned num9AC;
	unsigned tbl9B0[PLAYER_TBLSZ];
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
	char gapA38[68];
	unsigned brightness2;
	struct color_cfg col;
	char padAEB[257];
	// REMAP typeof(HWND hwnd_mci) == unsigned
	unsigned hwnd_mci;
	unsigned tblBF0[3];
	unsigned time_mci;
	unsigned tblC00[PLAYER_TBLSZ];
	struct menu_ctl *menu;
	int restbl[12];
	char strC58[260];
	char strD5C[260];
	unsigned numE60;
	char strE64[260];
	char strF68[260];
	char str106C[256];
	char blk116C[36];
	/* still not sure what this type is supposed to be */
	char str1190[4];
	unsigned num1194;
	char blk1198[160];
	char pad1238[24];
	int num1250;
};

extern struct colpalette game_col_palette;
extern struct pal_entry win_pal[256];

struct game *game_ctor(struct game *this, struct game_config *cfg, int should_start_game);
int go_fullscreen(SDL_Window *scr);

#endif
