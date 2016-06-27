#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <smt/smt.h>
#include "config.h"
#include "dmap.h"
#include "game.h"
#include "langx.h"
#include "todo.h"

static struct game *game_ref = NULL;

static struct logger *game_logger;
static unsigned game_window;
static unsigned disable_terrain_sound = 0;
static unsigned midi_no_fill = 0;

static unsigned players_connection_state[9];

static unsigned cfg_hInst;
static unsigned hInstance;

static const char *data_interface = "Interfac.drs";
static const char *data_border = "Border.drs";
static const char *data_terrain = "Terrain.drs";
static const char *data_graphics = "graphics.drs";
static const char *data_sounds = "sounds.drs";
static const char *directory_data = "data/";

unsigned game_hkey_root;
extern int prng_seed;

#define prng_init(t) prng_seed=t

struct pal_entry game_pal[256];

int dtor_iobase(void *this, char ch)
{
	// TODO verify no op
	return printf("no op: dtor_iobase: %p, %d\n", this, (int)ch);
}

unsigned game_loop(struct game *this)
{
	stub
	while (this->running) {
		unsigned ev, state;
		state = this->rpair_root.value.dword;
		if (state != 4 && state != 2) {
			printf("stop: state == %u\n", state);
			break;
		}
		while ((ev = smtPollev()) != SMT_EV_DONE) {
			if (ev == SMT_EV_QUIT) return 0;
			this->vtbl->translate_event(this, &ev);
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
	return NULL;
}

static char *game_strerror2(struct game *this, int code, int status, int a4, char *str, unsigned n)
{
	char *error = NULL;
	stub
	*str = '\0';
	if (code == 1) {
		switch (status) {
		case 1:
		case 2:
		case 4:
		case 5:
		case 6:
		case 14:
		case 15:
			error = this->vtbl->get_res_str(STR_ERR_INIT, str, n);
			break;
		case 7:
		case 8:
		case 11:
		case 13:
		case 17:
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
			error = this->vtbl->get_res_str(STR_ERR_START, str, n);
		case 103:
			error = this->vtbl->get_res_str(STR_ERR_SAVE_GAME, str, n);
		case 104:
			error = this->vtbl->get_res_str(STR_ERR_SAVE_SCENARIO, str, n);
		case 105:
			error = this->vtbl->get_res_str(STR_ERR_LOAD_GAME, str, n);
		case 106:
			error = this->vtbl->get_res_str(STR_ERR_LOAD_SCENARIO, str, n);
		default:
			error = game_strerror2(this, code, status, a4, str, n);
			break;
		}
		break;
	}
	return error;
}

static inline char game_set_pathfind(struct game *this, char pathfind)
{
	return this->pathfind = pathfind;
}

static inline char game_set_mp_pathfind(struct game *this, char mp_pathfind)
{
	return this->mp_pathfind = mp_pathfind;
}

static inline char game_set_hsv(struct game *this, unsigned char h, unsigned char s, unsigned char v)
{
	this->hsv[0] = h;
	this->hsv[1] = s;
	this->hsv[2] = v;
	return v;
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

static void game_handle_event(struct game *this, unsigned a2)
{
	stub
}

static int game_parse_opt2(struct game *this)
{
	stub
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
	if (!this->cfg->sfx_enable || strstr(buf, "NOMUSIC") || strstr(buf, "NO_MUSIC") || strstr(buf, "NO MUSIC"))
		this->cfg->midi_enable = 0;
	if (this->cfg->gfx8bitchk == 1 && this->cfg->mouse_opts[0] == 1)
		this->no_normal_mouse = 1;
	if (strstr(buf, "NORMALMOUSE") || strstr(buf, "NORMAL_MOUSE") || strstr(buf, "NORMAL MOUSE"))
		this->no_normal_mouse = 0;
	printf(
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
	stub
	return game_parse_opt2(this);
}

static int reg_init(struct game *this)
{
	stub
	this->rpair_root.left.key = 1 | rand();
	return 1;
}

struct game *start_game(struct game *this);
static int game_logger_init(struct game *this);
static signed game_show_focus_screen(struct game *this);

struct game *game_vtbl_init(struct game *this, struct game_config *cfg, int should_start_game)
{
	stub
	this->vtbl = &g_vtbl;
	game_set_hsv(this, 96, 96, 8);
	game_set_pathfind(this, 0);
	game_set_mp_pathfind(this, 0);
	game_ref = this;
	this->cfg = cfg;
	this->window = SMT_RES_INVALID;
	this->running = 1;
	this->palette = NULL;
	hInstance = SMT_RES_INVALID;
	this->state = 0;
	this->no_normal_mouse = 0;
	this->ptr18C = NULL;
	this->logctl = 0;
	this->rpair_root.left.key = 0;
	this->rpair_root.value.dword = 0;
	this->rpair_root.other = 0;
	this->rpair_rootval.dword = 0;
	this->midi_sync = 0;
	this->cursor = SMT_CURS_ARROW;
	if (smtCursor(SMT_CURS_ARROW, SMT_CURS_SHOW))
		this->cursor = SMT_CURS_DEFAULT;
	if (!getcwd(this->cwdbuf, CWDBUFSZ))
		perror(__func__);
	strcpy(this->libname, "language.dll");
	game_logger = NULL;
	game_window = SMT_RES_INVALID;
	cfg_hInst = this->cfg->hInst;
	game_hkey_root = 0;
	this->rollover_text = 1;
	this->gamespeed = 1.0f;
	this->difficulty = 2;
	if (reg_init(this)) {
		game_hkey_root = this->rpair_root.left.key;
		if (game_logger_init(this)) {
			game_logger = this->log;
			if (should_start_game && !game_show_focus_screen(this) && !this->state)
				this->state = 1;
		} else
			this->state = 15;
	} else
		this->state = 14;
	return this;
}

static int game_translate_event(struct game *this, unsigned *event)
{
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
	stub
	game_vtbl_init(this, cfg, 0);
	/* our stuff */
	// TODO move this
	smtCreatewin(&this->cfg->window, 640, 480, NULL, SMT_WIN_VISIBLE | SMT_WIN_BORDER);
	smtCreategl(&this->cfg->gl, this->cfg->window);
	/* original stuff */
	this->vtbl = &g_vtbl2;
	disable_terrain_sound = 0;
	game_set_color(this, 2);
	for (unsigned index = 0; index < 9; ++index) {
		players_connection_state[index] = 0;
	}
	if (should_start_game && !start_game(this) && !this->cfg->reg_state)
		this->cfg->reg_state = 1;
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
		logger_write_log(result, this->logctl);
		logger_write_stdout(this->log, this->logctl);
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
	this->mutex_state = 1;
	return 1;
}

static int game_go_fullscreen(struct game *this)
{
	int x, y;
	unsigned width, height, display_index = 0;
	stub
	// NOTE recycle this->cfg->window
	this->window = this->cfg->window;
	smtDisplaywin(this->window, &display_index);
	smtDisplayBounds(display_index, &x, &y, &width, &height);
	if (this->window == SMT_RES_INVALID)
		return 0;
	if (this->cfg->mouse_opts[0] || width == this->cfg->width && height == this->cfg->height)
		smtMode(this->window, SMT_WIN_FULL_FAKE);
	smtVisible(this->window, 1);
	game_window = this->window;
	return 1;
}

static struct pal_entry *palette_init(char *palette, int a2)
{
	char palette_path[260];
	stub
	palette_path[0] = '\0';
	if (palette) {
		if (strchr(palette, '.'))
			strcpy(palette_path, palette);
		else
			sprintf(palette_path, "%s.pal", palette);
		strup(palette_path, 260 - 1);
	}
	return game_pal;
}

static int game_gfx_init(struct game *this)
{
	stub
	if (!video_mode_init(&this->mode))
		return 0;
	if (!direct_draw_init(
		&this->mode,
		this->cfg->hInst, this->window,
		this->palette,
		(this->cfg->gfx8bitchk != 0) + 1,
		(this->cfg->mouse_opts[0] != 0) + 1,
		this->cfg->width,
		this->cfg->height,
		this->cfg->sys_memmap != 0))
	{
		return 0;
	}
	this->palette = palette_init(this->cfg->palette, 50500);
	return 1;
}

static int game_init_palette(struct game *this)
{
	return (this->palette = palette_init(this->cfg->palette, 50500)) != 0;
}

static int game_set_palette(struct game *this)
{
	struct pal_entry p[7] = {
		{23, 39, 124, 0},
		{39, 63, 0x90, 0},
		{63, 95, 0x9f, 0},
		{87, 123, 0xb4, 0},
		{63, 95, 0xa0, 0},
		{39, 63, 0x91, 0},
		{23, 39, 123, 0}
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
	} else if (w == 1024) {
		sw = 1024;
		sh = 768;
	} else if (w == 800) {
		sw = 800;
		sh = 600;
	}
	this->cfg->width = sw;
	this->cfg->height = sh;
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
		this->state = 2;
		return 0;
	}
	this->vtbl->init_mouse(this);
	smtScreensave(SMT_SCREEN_SAVE_OFF);
	if (!this->vtbl->init_icon(this)) {
		this->state = 5;
		return 0;
	}
	if (!this->vtbl->go_fullscreen(this)) {
		this->state = 6;
		return 0;
	}
	if (!this->vtbl->gfx_init(this)) {
		fputs("gfx_init failed\n", stderr);
		this->state = 7;
		return 0;
	}
	if (!this->vtbl->set_palette(this)) {
		this->state = 17;
		return 0;
	}
	return 1;
}

static int game_mousestyle(struct game *this)
{
	stub
}

struct game *start_game(struct game *this)
{
	struct pal_entry p[7] = {
		{23, 39, 124, 0},
		{39, 63, 0x90, 0},
		{63, 95, 0x9f, 0},
		{87, 123, 0xb4, 0},
		{95, 0xa0, 0, 0},
		{23, 123, 0, 0}
	};
	stub
	read_data_mapping(data_sounds   , "data2/"      , 1);
	read_data_mapping(data_graphics , "data2/"      , 0);
	read_data_mapping(data_interface, "data2/"      , 0);
	read_data_mapping(data_sounds   , directory_data, 1);
	read_data_mapping(data_graphics , directory_data, 0);
	read_data_mapping(data_terrain  , directory_data, 0);
	read_data_mapping(data_border   , directory_data, 0);
	read_data_mapping(data_interface, directory_data, 0);
	if (!game_show_focus_screen(this))
		return 0;
	update_palette(this->palette, 24, 7, p);
	if (!game_opt_check(this, "LOBBY"))
		return this;
	return this;
}

void game_free(void) {
	if (!game_ref) return;
	smtFreegl(game_ref->cfg->gl);
	smtFreewin(game_ref->cfg->window);
}

struct game_vtbl g_vtbl = {
	.parse_opt = game_parse_opt2
}, g_vtbl2 = {
	.dtor_io = dtor_iobase,
	.main = game_loop,
	.get_res_str = game_get_res_str,
	.strerr = game_strerror,
	.parse_opt = game_parse_opt,
	.init_icon = game_init_icon,
	.go_fullscreen = game_go_fullscreen,
	.gfx_init = game_gfx_init,
	.set_palette = game_set_palette,
	.translate_event = game_translate_event,
	.handle_event = game_handle_event,
	.init_mouse = game_mousestyle,
};
