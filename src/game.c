#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <smt/smt.h>
#include "config.h"
#include "dbg.h"
#include "dmap.h"
#include "game.h"
#include "game_set.h"
#include "langx.h"
#include "memmap.h"
#include "todo.h"

#define get_os_version(a) a=0x1000

static struct game *game_ref = NULL;
static int game_vtbl_focus;
static int game_vtbl56146C;
static char game_fname_focus[256];
static FILE *file_vtbl_focus;

static struct logger *game_logger;
static unsigned game_window;
static unsigned game_window_580DA0;
static struct comm *comm_580DA8;
static struct sfx_engine *sfx_engine_ref;
static struct game_drive *game_drive_ref;
static unsigned disable_terrain_sound = 0;
static unsigned midi_no_fill = 0;

static unsigned players_connection_state[9];

static unsigned cfg_hInst;
static unsigned hInstance;
static int hLibModule = -1;
static unsigned game_580E24;
static unsigned game_580E28;

static const char *data_interface = "Interfac.drs";
static const char *data_border = "Border.drs";
static const char *data_terrain = "Terrain.drs";
static const char *data_graphics = "graphics.drs";
static const char *data_sounds = "sounds.drs";
static const char *directory_data = "data/";

static struct colpalette game_col_palette;

unsigned game_hkey_root;
extern int prng_seed;

#define prng_init(t) prng_seed=t

struct pal_entry game_pal[256];

static void game_free_ios_base(struct game *this)
{
	stub
	this->vtbl = &g_vtbl2;
	if (this->window2 != SMT_RES_INVALID) {
		smtFreewin(this->window2);
		this->window2 = SMT_RES_INVALID;
	}
}

static inline char game_get_pathfind(const struct game *this)
{
	return this->pathfind;
}

static inline char game_get_mp_pathfind(const struct game *this)
{
	return this->mp_pathfind;
}

static void game_free(struct game *this)
{
	stub
	struct video_mode *videomode = this->mode;
	this->vtbl = &g_vtbl;
	if (videomode)
		reg_cfg.screen_size = videomode->width;
	reg_cfg.rollover_text = 2 - (this->rollover_text != 0);
	if (this->cfg->mouse_style == 2)
		reg_cfg.mouse_style = 2;
	else
		reg_cfg.mouse_style = 1;
	reg_cfg.game_speed = (unsigned)(this->gamespeed * 10.0);
	reg_cfg.difficulty = this->difficulty;
	reg_cfg.pathfind = game_get_pathfind(this) + 1;
	reg_cfg.mp_pathfind = game_get_mp_pathfind(this) + 1;
	reg_cfg.scroll_speed = this->cfg->scrollspeed;
}

static struct game *game_dtor(struct game *this, char free_this)
{
	game_free(this);
	if (free_this & 1)
		free(this);
	return this;
}

static struct game *game_dtor_ios_base(struct game *this, char free_this)
{
	game_free_ios_base(this);
	if (free_this) free(this);
	return this;
}

static unsigned game_get_state(struct game *this)
{
	return this->state;
}

unsigned game_loop(struct game *this)
{
	stub
	while (this->running) {
		unsigned ev, state;
		state = this->rpair_root.value.dword;
		if (state != GE_FOCUS && state != GE_OPT) {
			printf("stop: state == %u\n", state);
			break;
		}
		while ((ev = smtPollev()) != SMT_EV_DONE) {
			if (ev == SMT_EV_QUIT) return 0;
			this->vtbl->translate_event(this, ev);
			this->vtbl->handle_event(this, 0);
		}
		smtSwapgl(this->cfg->window);
	}
	return 0;
}

char *game_get_res_str(unsigned id, char *str, unsigned n)
{
	stub
	if (loadstr(id, str, n))
		str[n - 1] = '\0';
	return str;
}

char *get_res_str2(unsigned id, char *str, unsigned n)
{
	if (!loadstr(id, str, n))
		*str = '\0';
	str[n - 1] = '\0';
	return str;
}

static char *game_strerror2(struct game *this, int code, int status, int a3, char *str, unsigned n)
{
	char *error = NULL;
	(void)a3;
	stub
	*str = '\0';
	if (code == 1) {
		switch (status) {
		case GE_LIB:
		case GE_OPT:
		case GE_FOCUS:
		case GE_ICON:
		case GE_FULLSCREEN:
		case 14:
		case 15:
			error = this->vtbl->get_res_str(STR_ERR_INIT, str, n);
			break;
		case GE_GFX:
		case GE_MOUSE:
		case GE_CTL:
		case 13:
		case GE_PALETTE:
			error = this->vtbl->get_res_str(STR_ERR_GFX, str, n);
			break;
		}
	}
	return error;
}

char *game_strerror(struct game *this, int code, signed status, int a4, char *str, unsigned n)
{
	char *error = NULL;
	stub
	*str = '\0';
	switch (code) {
	case 100:
		switch (status) {
		case 0:
		case 15:
		case 16:
		case 17:
			error = this->vtbl->get_res_str(4301, str, n);
			break;
		}
		break;
	case 1:
		switch (status) {
		case 100:
		case 101:
		case 102:
			error = this->vtbl->get_res_str(STR_FAIL_START, str, n);
		case 103:
			error = this->vtbl->get_res_str(STR_FAIL_SAVE_GAME, str, n);
		case 104:
			error = this->vtbl->get_res_str(STR_FAIL_SAVE_SCENARIO, str, n);
		case 105:
			error = this->vtbl->get_res_str(STR_FAIL_LOAD_GAME, str, n);
		case 106:
			error = this->vtbl->get_res_str(STR_FAIL_LOAD_SCENARIO, str, n);
		default:
			error = game_strerror2(this, code, status, a4, str, n);
			break;
		}
		break;
	}
	return error;
}

static struct map *game_map_save_area(struct game *this)
{
	struct map *result, *map;
	map = malloc(sizeof(struct map));
	if (map)
		result = map_init(map, "Diamond Map Save Area", 0);
	else
		result = NULL;
	this->map_area = result;
	if (result) {
		if (0) {
		} else {
			if (this->map_area) {
				map_free(this->map_area);
				free(this->map_area);
			}
			result = this->map_area = NULL;
		}
	}
	return result;
}

static inline char game_set_color(struct game *this, int brightness)
{
	this->brightness = brightness;
	switch (brightness) {
	case 0: brightness = game_set_hsv(this, 72, 72, 8); break;
	case 1: brightness = game_set_hsv(this, 96, 96, 8); break;
	case 2: brightness = game_set_hsv(this, 120, 120, 8); break;
	case 3: brightness = game_set_hsv(this, 144, 144, 8); break;
	case 4: brightness = game_set_hsv(this, 200, 200, 8); break;
	case 5: brightness = game_set_hsv(this, 250, 250, 8); break;
	}
	return brightness;
}

static inline void strup(char *buf, size_t n)
{
	for (size_t i = 0; buf[i] && i < n; ++i)
		buf[i] = toupper(buf[i]);
}

static int game_comm_ctl(struct game *this, int a2)
{
	int result = 1;
	int v4, player_id, v6;
	if (this->cfg->d1p0) {
		if (a2 <= 0)
			return 1;
		v4 = player_id = 0;
		for (player_id = 0, v6 = 1; v6 < 9; v6 = ++player_id) {
			if (commhnd423D10(this->nethandler, player_id + 1) == 2)
				if (game_player_is_alive(this, player_id))
					++v4;
		}
		result = a2 <= v4 * this->cfg->d3;
	}
	return result;
}

static int game_shp(struct game *this)
{
	this->shp_count = 3;
	shp_init(&this->shptbl[0], "groupnum.shp", 50403);
	shp_init(&this->shptbl[1], "waypoint.shp", 50404);
	shp_init(&this->shptbl[2], "moveto.shp"  , 50405);
	return 1;
}

static int game_handle_event_0(struct game *this)
{
	stub
	int result = 0;
	if (this->num14 && this->window)
		result = 1;
	return result;
}

static void game_handle_event(struct game *this, unsigned a2)
{
	(void)a2;
	game_handle_event_0(this);
}

static int game_process_event(struct game *this, unsigned hWnd_1, unsigned msg, unsigned wparam, unsigned hWnd)
{
	stub
	(void)this;
	(void)hWnd_1;
	(void)msg;
	(void)wparam;
	(void)hWnd;
	return 0;
}

static int game_parse_opt2(struct game *this)
{
	char buf[OPTBUFSZ];
	strncpy(buf, this->cfg->optbuf, OPTBUFSZ);
	buf[OPTBUFSZ - 1] = '\0';
	strup(buf, OPTBUFSZ - 1);
	printf("opts: %s\n", buf);
	if (strstr(buf, "NOSTARTUP") || strstr(buf, "NO_STARTUP") || strstr(buf, "NO STARTUP"))
		this->cfg->no_start = 1;
	if (strstr(buf, "SYSTEMMEMORY") || strstr(buf, "SYSTEM_MEMORY") || strstr(buf, "SYSTEM MEMORY"))
		this->cfg->sys_memmap = 1;
	if (strstr(buf, "MIDIMUSIC") || strstr(buf, "MIDI_MUSIC") || strstr(buf, "MIDI MUSIC")) {
		this->cfg->midi_enable = this->cfg->midi_opts[2] = 1;
		this->cfg->midi_opts[0] = this->cfg->midi_opts[1] = this->cfg->midi_opts[3] = 0;
	}
	if (strstr(buf, "MSYNC"))
		this->midi_sync = 1;
	midi_no_fill = strstr(buf, "MFILL") == 0;
	if (strstr(buf, "NOSOUND") || strstr(buf, "NO_SOUND") || strstr(buf, "NO SOUND"))
		this->cfg->sfx_enable = 0;
	if (strstr(buf, "640")) {
		this->cfg->width = 640;
		this->cfg->height = 480;
	}
	if (strstr(buf, "800")) {
		this->cfg->width = 800;
		this->cfg->height = 600;
	}
	if (strstr(buf, "1024")) {
		this->cfg->width = 1024;
		this->cfg->height = 768;
	}
	ORIGDROP
	if (!this->cfg->sfx_enable || strstr(buf, "NOMUSIC") || strstr(buf, "NO_MUSIC") || strstr(buf, "NO MUSIC"))
		this->cfg->midi_enable = 0;
	if (this->cfg->gfx8bitchk == 1 && this->cfg->window_query_dd_interface == 1)
		this->no_normal_mouse = 1;
	if (strstr(buf, "NORMALMOUSE") || strstr(buf, "NORMAL_MOUSE") || strstr(buf, "NORMAL MOUSE"))
		this->no_normal_mouse = 0;
	dbgf(
		"game options:\n"
		"resolution: (%d,%d)\n"
		"no_start: %d\n"
		"sys_memmap: %d\n"
		"midi: enable=%d, sync=%d, no_fill=%d\n"
		"sfx_enable: %d\n"
		"no_normal_mouse: %d\n",
		this->cfg->width, this->cfg->height,
		this->cfg->no_start,
		this->cfg->sys_memmap,
		this->cfg->midi_enable,
		this->midi_sync,
		midi_no_fill,
		this->cfg->sfx_enable,
		this->no_normal_mouse
	);
	return 1;
}

int game_parse_opt(struct game *this)
{
	char buf[256];
	strncpy(buf, this->cfg->optbuf, 255u);
	buf[255] = '\0';
	strup(buf, 255);
	ORIGDROP
	if (strstr(buf, "NOTERRAINSOUND"))
		disable_terrain_sound = 1;
	ORIGDROP
	return game_parse_opt2(this);
}

static int reg_init(struct game *this)
{
	this->rpair_root.left.key = 1 | rand();
	return config_load();
}

int start_game(struct game *this);
static int game_logger_init(struct game *this);
static signed game_show_focus_screen(struct game *this);

static inline void game_set_start_gamespeed(struct game *this, float gamespeed)
{
	this->start_gamespeed = gamespeed;
}

struct game *game_vtbl_init(struct game *this, struct game_config *cfg, int should_start_game)
{
	this->focus = NULL;
	for (int i = 0; i < 4; ++i)
		this->tbl28[i] = -1;
	this->vtbl9A8 = 0;
	this->vtbl = &g_vtbl;
	// setup some global vars
	game_vtbl_focus = 0;
	game_vtbl56146C = 0;
	*game_fname_focus = '\0';
	file_vtbl_focus = NULL;
	// setup other members
	game_set_start_gamespeed(this, 1.0f);
	game_set_c0(this, 0);
	game_set9A0(this, 0);
	game_set9A4(this, 0);
	game_set97D_97E(this, 1);
	game_set97E_97D(this, 0);
	game_set_hsv(this, 96, 96, 8);
	game_set_cheats(this, 0);
	game_set984(this, 1);
	game_set985(this, 0);
	game_set986(this, 1);
	game_set987(this, 1);
	game_set989(this, 0);
	game_set_difficulty2(this, 0);
	for (unsigned i = 0; i < 9; ++i) {
		game_init_player(this, i, 0);
		game_player_ctl(this, i, 0);
		game_player_ctl2(this, i, 0);
		game_tbl994(this, i, 1);
	}
	game_set_pathfind(this, 0);
	game_set_mp_pathfind(this, 0);
	game_set988(this, 4);
	game_str_8FD(this, "");
	// global reference
	game_ref = this;
	this->cfg = cfg;
	this->window = SMT_RES_INVALID;
	this->num14 = 0;
	this->running = 1;
	this->palette = NULL;
	this->num24 = 0;
	hInstance = SMT_RES_INVALID;
	this->screensaver_state = 0;
	this->state = 0;
	this->timer = 0;
	this->mode = NULL;
	this->basegame = NULL;
	this->no_normal_mouse = 0;
	//this->shptbl = NULL;
	this->sfx = NULL;
	this->num64 = 0;
	this->sfx_count = 0;
	this->sfx_tbl = NULL;
	this->ch70 = 0;
	this->tbl74[0] = 0;
	this->tbl74[1] = 0;
	this->tbl74[2] = 0;
	this->ch80 = 0;
	this->tbl184[0] = 0;
	this->tbl184[1] = 0;
	this->nethandler = 0;
	this->log = 0;
#ifdef DEBUG
	this->logctl = GAME_LOGCTL_STDOUT;
#else
	this->logctl = 0;
#endif
	this->tbl190[0] = 0;
	this->tbl190[2] = 0;
	this->tbl190[3] = 0;
	this->tbl190[4] = 1;
	this->tbl190[1] = 0;
	this->rpair_root.left.key = 0;
	this->rpair_root.value.dword = 0;
	this->rpair_root.other = 0;
	this->rpair_rootval.dword = 0;
	this->num1BC = 0;
	this->curscfg = NULL;
	this->num1C4 = 0;
	this->midi_sync = 0;
	this->cursor = SMT_CURS_ARROW;
	if (smtCursor(SMT_CURS_ARROW, SMT_CURS_SHOW))
		this->cursor = SMT_CURS_DEFAULT;
	this->num1E0 = 0;
	this->num1E4 = 0;
	if (!getcwd(this->cwdbuf, CWDBUFSZ))
		perror(__func__);
	strcpy(this->libname, "language.dll");
	this->tbl3FC[0] = -1;
	this->tbl3FC[1] = -1;
	this->tbl3FC[2] = -1;
	this->ptr3F4 = NULL;
	this->num3F8 = 1;
	this->num402 = 1;
	this->ch404 = 0;
	this->num8F4 = 0;
	for (unsigned i = 0; i < 249; ++i)
		this->tbl504[i] = 0;
	this->tbl8E8[0] = 0;
	this->tbl8E8[1] = 0;
	this->tbl8E8[2] = 0;
	this->num9AC = 1;
	game_logger = NULL;
	game_window = SMT_RES_INVALID;
	cfg_hInst = this->cfg->hInst;
	game_window_580DA0 = SMT_RES_INVALID;
	comm_580DA8 = 0;
	game_hkey_root = 0;
	sfx_engine_ref = NULL;
	game_drive_ref = NULL;
	this->ptr8 = NULL;
	for (unsigned i = 0; i < 9; ++i)
		this->tbl9B0[i] = this->tblA15[i] = 0;
	this->num9FC = 0;
	this->rollover_text = 1;
	this->map_area = NULL;
	this->gamespeed = 1.0f;
	this->difficulty = 2;
	if (reg_init(this)) {
		game_hkey_root = this->rpair_root.left.key;
		if (game_logger_init(this)) {
			game_logger = this->log;
			if (should_start_game && !game_show_focus_screen(this) && !this->state)
				this->state = 1;
			this->state2 = 0;
		} else
			this->state = 15;
	} else
		this->state = 14;
	return this;
}

static int game_duck_event(struct game *this, int event)
{
	(void)this;
	(void)event;
	return 1;
}

static signed game_init_icon(struct game *this)
{
	signed result;
	stub
	if (this->cfg->hPrevInst)
		result = 1;
	else {
		// TODO load icon
		result = 1;
	}
	return result;
}

struct game *game_ctor(struct game *this, struct game_config *cfg, int should_start_game)
{
	game_vtbl_init(this, cfg, 0);
	/* < */
	// TODO move this
	smtCreatewin(&this->cfg->window, 640, 480, NULL, SMT_WIN_VISIBLE | SMT_WIN_BORDER);
	smtCreategl(&this->cfg->gl, this->cfg->window);
	smtPos(this->cfg->window, 0, 0);
	/* > */
	this->hwnd_mci = 0;
	this->tblBF0[0] = -1;
	this->tblBF0[1] = 0;
	this->tblBF0[2] = 0;
	this->time_mci = 0;
	for (unsigned i = 0; i < 10; ++i)
		this->tblC00[i] = 0;
	this->num1250 = 0;
	this->vtbl = &g_vtbl2;
	this->num1D8 = 1;
	this->window2 = 0;
	this->chAE8 = 0;
	hLibModule = 0;
	this->chC58 = 0;
	this->chD5C = 0;
	this->chE64 = 0;
	this->chF68 = 0;
	this->numE60 = 0;
	this->ch106C = 0;
	disable_terrain_sound = 0;
	game_580E24 = 0;
	game_580E28 = 0;
	for (unsigned i = 0; i < 4; ++i)
		this->tblA24[i] = 0;
	game_set_color(this, 2);

	game_setA80(this, 2);
	game_setA84(this, 1);
	game_setA88(this, 1);
	game_setA8C_A90(this, 1, 1);
	game_setAD6(this, 1);
	game_setAD7(this, 0);
	game_setAD8(this, 0);
	game_setAD9(this, 0);
	game_setADC(this, 0);
	game_setAE0(this, 0);
	game_setAE4(this, 0);
	game_setAE5(this, 0);
	game_setAE6(this, 50);
	game_set1190(this, 0);
	game_set1194(this, -1);
	for (unsigned i = 10; i < 20; ++i)
		this->tblC00[i] = -1;
	this->tblC00[20] = this->tblC00[21] = -1;
	for (unsigned index = 0; index < 9; ++index) {
		game_offsetA94(this, index, index + 2);
		game_offsetAA0(this, index, index);
		game_offsetAC4(this, index, 1);
		game_offsetACD(this, index, 0);
		players_connection_state[index] = 0;
	}

	memset(this->blk116C, 0, sizeof(this->blk116C));
	game_set988(this, 4);
	game_clear1198(this);
	if (should_start_game && !start_game(this) && this->state == 0)
		this->state = 1;                            // game has stopped flag?
	return this;
}

static int game_logger_init(struct game *this)
{
	struct logger *result, *log = malloc(sizeof(struct logger));
	if (log)
		result = logger_init(log);
	else
		result = NULL;
	this->log = result;
	if (result) {
		logger_write_log(result, this->logctl & GAME_LOGCTL_FILE);
		logger_write_stdout(this->log, this->logctl & GAME_LOGCTL_STDOUT);
		logger_enable_timestamp(this->log, 1);
	}
	return result != NULL;
}

static int game_cmp_time(struct game *this)
{
	time_t t0, t1;
	struct tm tm_time;
	memset(&tm_time, 0, sizeof(tm_time));
	tm_time.tm_mon = this->cfg->time[0] - 1;
	tm_time.tm_mday = this->cfg->time[1];
	tm_time.tm_year = this->cfg->time[2];
	t0 = mktime(&tm_time);
	time(&t1);
	return t1 <= t0;
}

/** check if option has been specified on startup ignoring case */
static int game_opt_check(struct game *this, char *opt)
{
	char optbuf[OPTBUFSZ];
	strncpy(optbuf, this->cfg->optbuf, OPTBUFSZ);
	optbuf[OPTBUFSZ - 1] = '\0';
	strup(optbuf, OPTBUFSZ - 1);
	return strstr(optbuf, opt) != 0;
}

static int game_futex_window_request_focus(struct game *this)
{
	stub
	this->mutex_main_error = 1;
	return 1;
}

static int game_go_fullscreen(struct game *this)
{
	int x, y, wx, wy;
	unsigned width, height, ww, wh, display_index = 0;
	stub
	// NOTE recycle this->cfg->window
	this->window = this->cfg->window;
	smtDisplaywin(this->window, &display_index);
	smtDisplayBounds(display_index, &x, &y, &width, &height);
	if (this->window == SMT_RES_INVALID)
		return 0;
	if (this->cfg->window_query_dd_interface || (width == this->cfg->width && height == this->cfg->height)) {
		smtTitle(this->window, this->cfg->title);
		smtBorder(this->window, 0);
	}
	smtMode(this->window, SMT_WIN_FULL_FAKE);
	smtBoundsp(this->window, &wx, &wy, &ww, &wh);
	if (wx + wh != width || wy + wh != this->cfg->height) {
		int dx, dy;
		unsigned dw, dh;
		dx = x;
		dy = y;
		dw = x + width + this->cfg->width - x - wx - ww;
		dh = y + height + this->cfg->height - y - wy - wh;
		dbgf(
			"move window to (%d,%d,%u,%u)\n",
			dx, dy, dw, dh
		);
		smtBounds(this->window, dx, dy, dw, dh);
	}
	if (this->cfg->window_query_dd_interface)
		smtVisible(this->window, 1);
	game_window = this->window;
	return 1;
}

static struct drs_pal *palette_init(struct colpalette *this, char *palette, int res_id)
{
	int v5;
	char palette_path[260];
	(void)this;
	palette_path[0] = '\0';
	if (palette) {
		if (strchr(palette, '.'))
			strcpy(palette_path, palette);
		else
			sprintf(palette_path, "%s.pal", palette);
		strup(palette_path, (sizeof palette_path) - 1);
	}
	dbgf("palette: %s\n", palette);
	unsigned i, j, k;
	j = k = 0;
	v5 = -1;
	for (i = 0; i < 10; ++i) {
		if (this->paltbl[j]) {
			if ((res_id != -1 && this->restbl[j] == res_id) || !strcmp(this->path_table[i], palette_path)) {
				dbgf("pal: add ref %u\n", j);
				++this->refcnttbl[i];
				return this->paltbl[i];
			}
			j = k;
		} else if (v5 == -1)
			v5 = i;
		k = ++j;
	}
	dbgf("i=%u,v5=%d\n", i, v5);
	if (v5 == -1)
		return NULL;
	this->paltbl[v5] = drs_palette(palette_path, res_id, 1);
	if (!this->paltbl[v5])
		return NULL;
	dbgf("pal: add ref %u\n", j);
	++this->refcnttbl[v5];
	strcpy(this->path_table[v5], palette_path);
	this->restbl[v5] = res_id;
	return this->paltbl[v5];
}

static int game_gfx_init(struct game *this)
{
	stub
	struct video_mode *videomode = new(sizeof(struct video_mode));
	if (!video_mode_init(videomode))
		return 0;
	this->mode = videomode;
	video_mode_fetch_bounds(videomode, this->cfg->window_query_dd_interface);
	if (!direct_draw_init(
		this->mode,
		this->cfg->hInst, this->window,
		this->palette,
		(this->cfg->gfx8bitchk != 0) + 1,
		(this->cfg->window_query_dd_interface != 0) + 1,
		this->cfg->width,
		this->cfg->height,
		this->cfg->sys_memmap != 0))
	{
		return 0;
	}
	this->palette = palette_init(&game_col_palette, this->cfg->palette, 50500);
	return 1;
}

static int game_process_intro(struct game *this, unsigned hwnd, unsigned msg, unsigned wparam, unsigned hWnd)
{
	stub
	(void)this;
	(void)hwnd;
	(void)msg;
	(void)wparam;
	(void)hWnd;
	return 0;
}

static int game_init_custom_mouse(struct game *this)
{
	unsigned mouse = reg_cfg.custom_mouse;
	(void)mouse;
	if (reg_cfg.custom_mouse > 1) {
		mouse = 0;
		this->no_normal_mouse = 0;
	}
	return 1;
}

static int game_init_sfx(struct game *this)
{
	stub
	if (!this->cfg->sfx_enable)
		return 1;
	return 1;
}

static int game_window_ctl(struct game *this)
{
	stub
	(void)this;
	return 0;
}

static int game_window_ctl2(struct game *this)
{
	stub
	(void)this;
	return 0;
}

static int game_gfx_ctl(struct game *this)
{
	stub
	(void)this;
	return 0;
}

static int game_init_sfx_tbl(struct game *this)
{
	stub
	this->sfx_count = 17;
	return 0;
}

static int game_init_palette(struct game *this)
{
	return (this->palette = palette_init(NULL, this->cfg->palette, 50500)) != 0;
}

static int game_set_palette(struct game *this)
{
	struct pal_entry p[7] = {
		{23,  39,  124, 0},
		{39,  63, 0x90, 0},
		{63,  95, 0x9f, 0},
		{87, 123, 0xb4, 0},
		{63,  95, 0xa0, 0},
		{39,  63, 0x91, 0},
		{23,  39,  123, 0}
	};
	if (!game_init_palette(this))
		return 0;
	update_palette(this->palette, 248, 7, p);
	return 1;
}

static signed game_show_focus_screen(struct game *this)
{
	struct timespec tp;
	unsigned gamespeed;
	stub
	clock_gettime(CLOCK_REALTIME, &tp);
	prng_init(tp.tv_nsec / 1000LU);
	unsigned w = reg_cfg.screen_size;
	unsigned sw = 640, sh = 480;
	if (w > 1024) {
		if (w == 1280) {
			sw = 1280;
			sh = 1024;
		}
	} else {
		switch (w) {
		case 1024:
			sw = 1024;
			sh = 768;
			break;
		case 800:
			sw = 800;
			sh = 600;
			break;
		default:
			sw = 640;
			sh = 480;
			break;
		}
	}
	this->cfg->width = sw;
	this->cfg->height = sh;
	this->rollover_text = reg_cfg.rollover_text != 2;
	if (reg_cfg.mouse_style == 2)
		this->cfg->mouse_style = 2;
	else if (reg_cfg.mouse_style == 1)
		this->cfg->mouse_style = 1;
	gamespeed = reg_cfg.game_speed;
	this->cfg->gamespeed = gamespeed * 0.1;
	this->cfg->difficulty = reg_cfg.difficulty;
	if (reg_cfg.pathfind >= PATHFIND_LOW + 1 && reg_cfg.pathfind <= PATHFIND_HIGH + 1)
		game_set_pathfind(this, reg_cfg.pathfind - 1);
	else
		fprintf(stderr, "ignore pathfind: %u\n", reg_cfg.pathfind);
	if (reg_cfg.mp_pathfind >= PATHFIND_LOW + 1 && reg_cfg.mp_pathfind <= PATHFIND_HIGH + 1)
		game_set_mp_pathfind(this, reg_cfg.mp_pathfind - 1);
	else
		fprintf(stderr, "ignore mp_pathfind: %u\n", reg_cfg.mp_pathfind);
	if (reg_cfg.scroll_speed >= 10 && reg_cfg.scroll_speed <= 200)
		this->cfg->scroll1 = this->cfg->scroll0 = reg_cfg.scroll_speed;
	if (!this->vtbl->parse_opt(this)) {
		this->state = GE_OPT;
		return 0;
	}
	this->vtbl->init_mouse(this);
	if (this->cfg->chk_time && !game_cmp_time(this)) {
		fputs("game_cmp_time failed\n", stderr);
		this->state = GE_TIME;
		return 0;
	}
	if (this->cfg->window_request_focus && !game_futex_window_request_focus(this)) {
		fputs("game_futex_window_request_focus failed\n", stderr);
		this->state = GE_FOCUS;
		return 0;
	}
	if (!game_opt_check(this, "NODXCHECK")) {
		unsigned os;
		get_os_version(os);
		if (os < 0x501) {
			this->state = GE_LOWOS;
			return 0;
		}
	}
	smtScreensave(SMT_SCREEN_SAVE_OFF);
	this->num9AC = game_comm_ctl(this, 0);
	// initialize all vtbl stuff
	if (!this->vtbl->init_icon(this)) {
		fputs("init_icon failed\n", stderr);
		this->state = GE_ICON;
		return 0;
	}
	if (!this->vtbl->go_fullscreen(this)) {
		fputs("go_fullscreen failed\n", stderr);
		this->state = GE_FULLSCREEN;
		return 0;
	}
	if (!this->vtbl->gfx_init(this)) {
		fputs("gfx_init failed\n", stderr);
		this->state = GE_GFX;
		return 0;
	}
	if (!this->vtbl->set_palette(this)) {
		fputs("set_palette failed\n", stderr);
		this->state = GE_PALETTE;
		return 0;
	}
	if (!this->vtbl->shp(this)) {
		fputs("shp failed\n", stderr);
		this->state = GE_GFX;
		return 0;
	}
	if (!this->vtbl->map_save_area(this)) {
		fputs("map_save_area failed\n", stderr);
		this->state = GE_GFX;
		return 0;
	}
	if (!this->vtbl->init_custom_mouse(this)) {
		fputs("init_custom_mouse failed\n", stderr);
		this->state = GE_MOUSE;
		return 0;
	}
	if (!this->vtbl->init_sfx(this)) {
		fputs("init_sfx failed\n", stderr);
		this->state = GE_SFX;
		return 0;
	}
	if (!this->vtbl->window_ctl(this)) {
		fputs("window_ctl failed\n", stderr);
		this->state = GE_WINCTL;
		return 0;
	}
	if (!this->vtbl->window_ctl2(this)) {
		fputs("window_ctl2 failed\n", stderr);
		this->state = GE_WINCTL2;
		return 0;
	}
	if (!this->vtbl->gfx_ctl(this)) {
		fputs("gfx_ctl failed\n", stderr);
		this->state = GE_CTL;
		return 0;
	}
	if (!this->vtbl->init_sfx_tbl(this)) {
		fputs("init_sfx_tbl failed\n", stderr);
		this->state = GE_SFX2;
		return 0;
	}
	struct game_drive *drive = new(sizeof(struct game_drive));
	game_drive_ref = drive;
	halt();
	// TODO initialize drive
	// TODO parse game nfo
	// TODO scenario.inf
	// TODO fail label
	return 1;
}

static int game_mousestyle(struct game *this)
{
	// FIXME update and figure out type of `0' arguments
	return this->cfg->mouse_style == 1
		? game_offsetA24(this, 0, 0x28, 0, 0x28)
		: game_offsetA24(this, 0, 0x28, 0, 0x28);
}

int start_game(struct game *this)
{
	stub
	struct pal_entry p[7] = {
		{23, 39, 124, 0},
		{39, 63, 0x90, 0},
		{63, 95, 0x9f, 0},
		{87, 123, 0xb4, 0},
		{95, 0xa0, 0, 0},
		{23, 123, 0, 0}
	};
	// <
	errno = 0;
	fd_lang = loadlib("language.dll", &data_lang, &size_lang);
	if (hLibModule == -1) {
		if (errno)
			perror("language.dll");
		else
			fprintf(stderr, "%s: bad language data\n", "language.dll");
		this->state = 1;
		return 0;
	}
	// >
	errno = 0;
	fd_langx = hLibModule = loadlib("languagex.dll", &data_langx, &size_langx);
	if (hLibModule == -1) {
		if (errno)
			perror("languagex.dll");
		else
			fprintf(stderr, "%s: bad language data\n", "languagex.dll");
		this->state = 1;
		return 0;
	}
	if (findfirst("empires.exe") == -1) {
		this->state = 23;
		fprintf(stderr, "%s: no such file\n", "empires.exe");
		return 0;
	}
	// sound drs files need to be available immediately,
	// so these are not mmap'ed
	read_data_mapping(data_sounds   , "data2/"      , 1);
	read_data_mapping(data_graphics , "data2/"      , 0);
	read_data_mapping(data_interface, "data2/"      , 0);
	read_data_mapping(data_sounds   , directory_data, 1);
	read_data_mapping(data_graphics , directory_data, 0);
	read_data_mapping(data_terrain  , directory_data, 0);
	read_data_mapping(data_border   , directory_data, 0);
	// FIXME hack to also include data/Interface.drs
	read_data_mapping(data_interface, directory_data, 0);
	read_data_mapping(data_interface, directory_data, 0);
	if (!game_show_focus_screen(this))
		return 0;
	update_palette(this->palette, 248, 7, p);
	this->window2 = this->window;
	if (!game_opt_check(this, "LOBBY"))
		return 1;
	dbgs("lobby");
	struct video_mode *mode = new2(sizeof(struct video_mode), "video_mode");
	if (mode)
		video_mode_start_init(mode, "Status Screen", 1215, "scr1", 50051);
	return 1;
}

struct game_vtbl g_vtbl = {
	.dtor = game_dtor,
	.process_intro = game_process_event,
	.get_state = game_get_state,
	.get_res_str = get_res_str2,
	.strerr2 = game_strerror2,
	.parse_opt = game_parse_opt2,
	.shp = game_shp,
	.init_mouse = game_init_custom_mouse,
}, g_vtbl2 = {
	.dtor = game_dtor_ios_base,
	.main = game_loop,
	.process_intro = game_process_intro,
	.get_state = game_get_state,
	.strerr = game_strerror,
	.get_res_str = game_get_res_str,
	.parse_opt = game_parse_opt,
	.init_icon = game_init_icon,
	.go_fullscreen = game_go_fullscreen,
	.gfx_init = game_gfx_init,
	.set_palette = game_set_palette,
	.shp = game_shp,
	.translate_event = game_duck_event,
	.handle_event = game_handle_event,
	.map_save_area = game_map_save_area,
	.init_mouse = game_mousestyle,
	.init_sfx = game_init_sfx,
	.window_ctl = game_window_ctl,
	.window_ctl2 = game_window_ctl2,
	.gfx_ctl = game_gfx_ctl,
	.init_sfx_tbl = game_init_sfx_tbl,
};
