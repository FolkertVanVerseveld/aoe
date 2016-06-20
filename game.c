#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <smt/smt.h>
#include "config.h"
#include "game.h"
#include "langx.h"
#include "todo.h"

static struct game *game_ref = NULL;

static int game_vtbl_580D84, game_vtbl_56146C, game_vtbl_561474;
static int game_580DA0, game_580DA8, game_580E24, game_580E28;
static struct logger *game_logger;
static unsigned game_580D98, game_580B6C;
static unsigned game_window;
static FILE *FILE_580B70;
static unsigned disable_terrain_sound = 0;
static unsigned midi_no_fill = 0;

static unsigned cfg_hInst;
static unsigned hInstance;

static const char *data_interface = "Interfac.drs";
static const char *data_border = "Border.drs";
static const char *data_terrain = "Terrain.drs";
static const char *data_graphics = "graphics.drs";
static const char *data_sounds = "sounds.drs";
static const char *directory_data = "data/";

#include "game_set.h"

unsigned game_hkey_root;
extern int prng_seed;

#define prng_init(t) prng_seed=t

int read_data_mapping(const char *filename, const char *directory, int no_stat);

int dtor_iobase(void *this, char ch)
{
	// TODO verify no op
	return printf("no op: dtor_iobase: %p, %d\n", this, (int)ch);
}

unsigned game_get_state(struct game *this)
{
	return this->state;
}

unsigned game_loop(struct game *this)
{
	//while (1) {
	while (this->running) {
		unsigned ev;
		while ((ev = smtPollev()) != SMT_EV_DONE) {
			switch (ev) {
			case SMT_EV_QUIT: return 0;
			}
		}
		smtSwapgl(this->cfg.window);
	}
	//}
	return 0;
}

char *game_get_res_str(unsigned id, char *str, int n)
{
	stub
	if (loadstr(id, str, n))
		str[n - 1] = '\0';
	return NULL;
}

char *game_strerror(struct game *this, int code, signed status, int a4, char *str, int n)
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
	}
	return error;
}

static inline char game_set_pathfind(struct game *this, char pathfind)
{
	return this->cfg.pathfind = pathfind;
}

static inline char game_set_mp_pathfind(struct game *this, char mp_pathfind)
{
	return this->cfg.mp_pathfind = mp_pathfind;
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
	stub
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
	for (size_t i = 0; *buf && i < n; ++i)
		buf[i] = toupper(buf[i]);
}

int game_parse_opt2(struct game *this)
{
	stub
	char buf[OPTBUFSZ];
	strncpy(buf, this->cfg.optbuf, OPTBUFSZ);
	buf[OPTBUFSZ - 1] = '\0';
	strup(buf, OPTBUFSZ - 1);
	printf("opts: %s\n", buf);
	if (strstr(buf, "NOSTARTUP") || strstr(buf, "NO_STARTUP") || strstr(buf, "NO STARTUP"))
		this->cfg.no_start = 1;
	if (strstr(buf, "SYSTEMMEMORY") || strstr(buf, "SYSTEM_MEMORY") || strstr(buf, "SYSTEM MEMORY"))
		this->cfg.sys_memmap = 1;
	if (strstr(buf, "MIDIMUSIC") || strstr(buf, "MIDI_MUSIC") || strstr(buf, "MIDI MUSIC")) {
		this->cfg.midi_enable = this->cfg.midi_opts[2] = 1;
		this->cfg.midi_opts[0] = this->cfg.midi_opts[1] = this->cfg.midi_opts[3] = 0;
	}
	if (strstr(buf, "MSYNC"))
		this->midi_sync = 1;
	midi_no_fill = strstr(buf, "MFILL") == 0;
	if (strstr(buf, "NOSOUND") || strstr(buf, "NO_SOUND") || strstr(buf, "NO SOUND"))
		this->cfg.sfx_enable = 0;
	if (strstr(buf, "640")) {
		this->cfg.width = 640;
		this->cfg.height = 480;
	}
	if (strstr(buf, "800")) {
		this->cfg.width = 800;
		this->cfg.height = 600;
	}
	if (strstr(buf, "1024")) {
		this->cfg.width = 1024;
		this->cfg.height = 768;
	}
	if (!this->cfg.sfx_enable || strstr(buf, "NOMUSIC") || strstr(buf, "NO_MUSIC") || strstr(buf, "NO MUSIC"))
		this->cfg.midi_enable = 0;
	if (this->cfg.mouse_opts[2] == 1 && this->cfg.mouse_opts[0] == 1)
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
		this->cfg.width, this->cfg.height,
		this->cfg.no_start,
		this->cfg.sys_memmap,
		this->cfg.midi_enable,
		this->midi_sync,
		midi_no_fill,
		this->cfg.sfx_enable,
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
	this->rpair.root = 1 | rand();
	return 1;
}

struct game *start_game(struct game *this);
static int game_logger_init(struct game *this);
static signed game_show_focus_screen(struct game *this);

struct game *game_vtbl_init(struct game *this, int should_start_game)
{
	this->num4 = 0;
	this->tbl28[0] = this->tbl28[1] = this->tbl28[2] = this->tbl28[3] = -1;
	this->num9A8 = 0;
	this->vtbl = &g_vtbl;
	game_vtbl_561474 = game_vtbl_56146C = game_vtbl_580D84 = 0;
	FILE_580B70 = NULL;
	game_set8F8(this, 1.0f);
	game_set8FC(this, 0);
	game_set9A0(this, 0);
	game_set9A4(this, 0);
	game_set97D_97E(this, 1);
	game_set97E_97D(this, 0);
	game_set_hsv(this, 96, 96, 8);
	game_set982(this, 0);
	game_set984(this, 1);
	game_set985(this, 0);
	game_set986(this, 1);
	game_set987(this, 1);
	game_set989(this, 0);
	game_set993(this, 0);
	for (int i = 0; i < 9; ++i) {
		game_tbl98A(this, i, 0);
		game_tbl98A_2(this, i, 0);
		game_tbl98A_3(this, i, 0);
		game_tbl994(this, i, 1);
	}
	game_set_pathfind(this, 0);
	game_set_mp_pathfind(this, 0);
	game_set988(this, 4);
	game_str8FD(this, "");
	game_ref = this;
	this->window = SMT_RES_INVALID;
	this->num14 = 0;
	this->running = 1;
	this->num1C = this->num24 = 0;
	hInstance = SMT_RES_INVALID;
	this->num38 = 0;
	this->state = 0;
	this->tbl44[0] = this->tbl44[1] = this->tbl44[2] = 0;
	this->ch50 = 2;
	this->no_normal_mouse = 0;
	this->tbl58[1] = this->tbl58[2] = this->tbl58[3] = 0;
	this->num68 = this->num6C = 0;
	this->ch70 = 0;
	this->tbl74[0] = this->tbl74[1] = this->tbl74[2] = 0;
	this->tbl80[0] = '\0';
	for (int i = 0; i < 8; ++i)
		this->tbl184[i] = 0;
	this->logctl = 0;
	this->rpair.root = this->rpair.value = 0;
	this->rpair.next = NULL;
	this->rpair_rootval = 0;
	this->tbl1BC[0] = this->tbl1BC[1] = this->tbl1BC[2] = 0;
	this->midi_sync = 0;
	this->num1CC = this->num1D0 = 1;
	this->cursor = SMT_CURS_ARROW;
	if (smtCursor(SMT_CURS_ARROW, SMT_CURS_SHOW))
		this->cursor = SMT_CURS_DEFAULT;
	this->num1E0 = this->num1E4 = 0;
	if (!getcwd(this->cwdbuf, CWDBUFSZ))
		perror(__func__);
	strcpy(this->libname, "language.dll");
	this->tbl3FC[0] = this->tbl3FC[1] = this->tbl3FC[2] = -1;
	this->num3F4 = this->num3F8 = 1;
	this->num402 = 1;
	this->ch404 = 0;
	for (int i = 0; i < 9; ++i)
		this->tbl504[i] = 0;
	this->num8F4 = 0;
	unsigned *tblptr = &this->tbl528[1];
	for (int i = 30; i != 0; --i, tblptr += 8)
		for (int j = -1; j < 7; ++j)
			tblptr[j] = 0;
	this->tbl8E8[0] = this->tbl8E8[1] = this->tbl8E8[2] = 0;
	this->num9AC = 0;
	game_logger = NULL;
	game_window = SMT_RES_INVALID;
	cfg_hInst = this->cfg.hInst;
	game_580DA0 = game_580DA8 = 0;
	game_hkey_root = 0;
	game_580D98 = game_580B6C = 0;
	this->num8 = 0;
	tblptr = this->tbl9B0;
	for (int i = 0; i < 9; ++i) {
		*tblptr++ = 0;
		this->tblA14[i + 1] = 0;
	}
	this->num9F4 = 0;
	this->rollover_text = 1;
	this->gamespeed = 1.0f;
	this->numA20 = 0;
	this->difficulty = 2;
	if (reg_init(this)) {
		game_hkey_root = this->rpair.root;
		if (game_logger_init(this)) {
			game_logger = &this->log;
			if (should_start_game && !game_show_focus_screen(this) && !this->state)
				this->state = 1;
			this->num9D4 = 0;
		} else
			this->state = 15;
	} else
		this->state = 14;
	return this;
}

struct game *game_ctor(struct game *this, int should_start_game)
{
	stub
	game_vtbl_init(this, 0);
	for (int i = 0; i < 15; ++i)
		this->tblBEC[i] = 0;
	this->tblBEC[1] = -1;
	this->num1250 = 0;
	this->vtbl = &g_vtbl2;
	this->num1D8 = 1;
	this->num1DC = 0;
	this->chAE8 = 0;
	this->chC58 = this->chD5C = this->chE64 = this->chF68 = 0;
	this->numE60 = 0;
	this->ch106C = 0;
	disable_terrain_sound = 0;
	game_580E24 = game_580E28 = 0;
	this->tblA24[0] = this->tblA24[1] = this->tblA24[2] = this->tblA24[3] = 0;
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
	if (should_start_game && !start_game(this) && !this->cfg.reg_state)
		this->cfg.reg_state = 1;
	return this;
}

static int game_logger_init(struct game *this)
{
	stub
	logger_init(&this->log);
	logger_write_log(&this->log, this->logctl);
	logger_write_stdout(&this->log, this->logctl);
	logger_enable_timestamp(&this->log, 1);
	return 1;
}

static signed game_show_focus_screen(struct game *this)
{
	struct timespec tp;
	stub
	clock_gettime(CLOCK_REALTIME, &tp);
	prng_init(tp.tv_nsec / 1000LU);
	unsigned w = cfg.screen_size;
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
	printf("screen size: (%u,%u)\n", sw, sh);
	this->cfg.width = sw;
	this->cfg.height = sh;
	if (cfg.mouse_style == 2)
		this->cfg.mouse_style = 2;
	else if (cfg.mouse_style == 1)
		this->cfg.mouse_style = 1;
	this->cfg.gamespeed = cfg.game_speed * 0.1;
	this->cfg.difficulty = cfg.difficulty;
	if (cfg.pathfind >= PATHFIND_LOW + 1 && cfg.pathfind <= PATHFIND_HIGH + 1)
		game_set_pathfind(this, cfg.pathfind - 1);
	else
		fprintf(stderr, "ignore pathfind: %u\n", cfg.pathfind);
	if (cfg.mp_pathfind >= PATHFIND_LOW + 1 && cfg.mp_pathfind <= PATHFIND_HIGH + 1)
		game_set_mp_pathfind(this, cfg.mp_pathfind - 1);
	else
		fprintf(stderr, "ignore mp_pathfind: %u\n", cfg.mp_pathfind);
	if (cfg.scroll_speed >= 10 && cfg.scroll_speed <= 200)
		this->cfg.scroll1 = this->cfg.scroll0 = cfg.scroll_speed;
	if (!this->vtbl->parse_opt(this))
		this->state = 2;
	return 1;
}

struct game *start_game(struct game *this)
{
	stub
	// FIXME initialize ((char*)this + 0xF26) == &this->cfg[1].gap1E0[5]
	read_data_mapping(data_sounds   , "data2/"      , 1);
	read_data_mapping(data_graphics , "data2/"      , 0);
	read_data_mapping(data_interface, "data2/"      , 0);
	read_data_mapping(data_sounds   , directory_data, 1);
	read_data_mapping(data_graphics , directory_data, 0);
	read_data_mapping(data_terrain  , directory_data, 0);
	read_data_mapping(data_border   , directory_data, 0);
	read_data_mapping(data_interface, directory_data, 0);
	if (game_show_focus_screen(this)) {
		smtCreatewin(&this->cfg.window, 1, 1, NULL, 0);
		smtCreategl(&this->cfg.gl, this->cfg.window);
	}
	return this;
}

struct game_vtbl g_vtbl = {
	.parse_opt = game_parse_opt2
}, g_vtbl2 = {
	.dtor_io = dtor_iobase,
	.main = game_loop,
	.get_state = game_get_state,
	.get_res_str = game_get_res_str,
	.strerr = game_strerror,
	.parse_opt = game_parse_opt
};
