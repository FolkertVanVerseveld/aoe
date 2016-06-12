#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <smt/smt.h>
#include "config.h"
#include "langx.h"
#include "log.h"
#include "todo.h"

#define TITLE "Age of Empires Expansion"
#define VERSION "00.01.6.1006"

int prng_seed;
unsigned disable_terrain_sound = 0;
unsigned midi_no_fill = 0;

int game_vtbl_580D84, game_vtbl_56146C, game_vtbl_561474;
int game_580DA0, game_580DA8, game_580E24, game_580E28;
struct logger *game_logger;
unsigned game_580D98, game_580B6C;
unsigned game_window;
unsigned cfg_hInst;
FILE *FILE_580B70;

const char *data_interface = "Interfac.drs";
const char *data_border = "Border.drs";
const char *data_terrain = "Terrain.drs";
const char *data_graphics = "graphics.drs";
const char *data_sounds = "sounds.drs";
const char *directory_data = "data/";
const char data_map_magic[4] = {'1', '.', '0', '0'};

struct obj42BF80 {
	unsigned time_millis;
};

struct regpair {
	unsigned root, value;
	struct regpair *next;
};

unsigned game_hkey_root;

#define DMAPBUFSZ 260
struct dmap {
	char *dblk;
	int fd;
	char *drs_data;
	struct dmap *next;
	char filename[DMAPBUFSZ];
	size_t length;
} *drs_list = NULL;

struct game;

int dtor_iobase(void*, char);
static unsigned game_get_state(struct game*);
unsigned game_loop(struct game *this);
char *game_get_res_str(unsigned id, char *str, int n);
char *game_strerror(struct game *this, int code, signed status, int a4, char *str, int n);
int game_parse_opt(struct game *this);
int game_parse_opt2(struct game *this);

struct game_vtbl {
	int (*dtor_io)(void*,char);
	unsigned (*main)(struct game*);
	// XXX consider inlining
	unsigned (*get_state)(struct game*);
	char *(*get_res_str)(unsigned, char*, int);
	char *(*strerr)(struct game*, int, signed, int, char*, int);
	int (*parse_opt)(struct game*);
} g_vtbl = {
	.parse_opt = game_parse_opt2
}, g_vtbl2 = {
	.dtor_io = dtor_iobase,
	.main = game_loop,
	.get_state = game_get_state,
	.get_res_str = game_get_res_str,
	.strerr = game_strerror,
	.parse_opt = game_parse_opt
};

#define OPTBUFSZ 256

// FIXME change char arrays to const char* if not modified anywhere
struct game_cfg {
	char title[48];
	unsigned reg_state;
	char version[21];
	char win_name[121];
	char tr_world_txt[137];
	unsigned window;
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
	unsigned numA80, numA84;
	char chAE8;
	unsigned tblBEC[20];
	char chC58, chD5C, chE64, chF68;
	unsigned numE60;
	char ch106C;
	int num1250;
} *game_ref;

unsigned hInstance;

static void hexdump(const void *buf, size_t n) {
	uint16_t i, j, p = 0;
	const unsigned char *data = buf;
	while (n) {
		printf("%04hX", p & ~0xf);
		for (j = p, i = 0; n && i < 0x10; ++i, --n)
			printf(" %02hhX", data[p++]);
		putchar(' ');
		for (i = j; i < p; ++i)
			putchar(isprint(data[i]) ? data[i] : '.');
		putchar('\n');
	}
}

static void dmap_init(struct dmap *map) {
	map->dblk = map->drs_data = NULL;
	map->filename[0] = '\0';
	map->next = NULL;
}

static void dmap_free(struct dmap *map) {
	assert((map->drs_data && !map->dblk) || map->drs_data == map->dblk);
	if (map->drs_data) free(map->drs_data);
	if (map->dblk && map->dblk != map->drs_data) free(map->dblk);
	if (map) free(map);
}

static void cleanup(void) {
	if (!drs_list) return;
	struct dmap *next, *item = drs_list;
	do {
		next = item->next;
		if (item) dmap_free(item);
		item = next;
	} while (item);
}

static inline void str2(char *buf, size_t n, const char *s1, const char *s2)
{
	snprintf(buf, n, "%s%s", s1, s2);
}

static inline void strup(char *buf, size_t n)
{
	for (size_t i = 0; *buf && i < n; ++i)
		buf[i] = toupper(buf[i]);
}

/*
read data file from specified path and save all data in drs_list.
provides additional error checking compared to original dissassembly
*/
int read_data_mapping(const char *filename, const char *directory, int no_stat)
{
	// data types that are read from drs files must honor WIN32 ABI
	typedef uint32_t size32_t;
	struct dbuf {
		char data[60];
		size32_t length;
	} buf;
	struct dmap *map = NULL;
	char name[DMAPBUFSZ];
	int fd = -1;
	int ret = -1;
	ssize_t n;
	str2(name, sizeof name, directory, filename);
	// deze twee regels komt dubbel voor in dissassembly
	// aangezien ik niet van duplicated code hou staat het nu hier
	fd = open(name, O_RDONLY); // XXX verify mode
	if (fd == -1) goto fail;
	map = malloc(sizeof *map);
	if (!map) {
		perror(__func__);
		goto fail;
	}
	dmap_init(map);
	if (!no_stat) {
		struct stat st;
		if (fstat(fd, &st)) goto fail;
		map->length = st.st_size;
		map->dblk = malloc(st.st_size);
		if (!map->dblk) {
			perror(__func__);
			goto fail;
		}
		n = read(fd, map->dblk, st.st_size);
		if (n != st.st_size) {
			perror(__func__);
			ret = n;
			goto fail;
		}
	}
	if (strlen(filename) >= DMAPBUFSZ) {
		fprintf(stderr, "map: overflow: %s\n", name);
		goto fail;
	}
	strcpy(map->filename, filename);
	puts(name);
	if (no_stat) {
		map->fd = fd;
		map->dblk = NULL;
		lseek(fd, 0, SEEK_CUR);
		n = read(fd, &buf, sizeof buf);
		if (n != sizeof buf) {
			fprintf(stderr, "%s: bad map\n", name);
			ret = n;
			goto fail;
		}
		map->length = buf.length;
		map->drs_data = malloc(buf.length);
		if (!map->drs_data) {
			perror(__func__);
			goto fail;
		}
		lseek(fd, 0, SEEK_SET);
		n = read(fd, map->drs_data, buf.length);
		if (n != buf.length) {
			fprintf(stderr, "%s: bad map\n", name);
			ret = n;
			goto fail;
		}
	} else {
		map->fd = -1;
		map->drs_data = map->dblk;
	}
	assert(map->drs_data);
	// insert map into drs list
	if (drs_list) {
		struct dmap *last = drs_list;
		while (last->next) last = last->next;
		last->next = map;
	} else {
		drs_list = map;
	}
	// check for magic "1.00tribe"
	ret = strcmp(&map->drs_data[0x2C], "tribe");
	if (!ret) ret = strncmp(&map->drs_data[0x28], data_map_magic, sizeof data_map_magic);
	if (ret) fprintf(stderr, "%s: hdr invalid\n", filename);
	// FIXME if no magic, dangling ptr in last item of drs list
fail:
	if (fd != -1) close(fd);
	if (ret && map) {
		assert((map->drs_data && !map->dblk) || map->drs_data == map->dblk);
		if (map->drs_data) free(map->drs_data);
		if (map->dblk && map->dblk != map->drs_data) free(map->dblk);
		if (map) free(map);
		perror(name);
	}
	return ret;
}

static int reg_init(struct game *this)
{
	stub
	this->rpair.root = 1 | rand();
	return 1;
}

#define prng_init(t) prng_seed=t

static int game_logger_init(struct game *this)
{
	stub
	logger_init(&this->log);
	logger_write_log(&this->log, this->logctl);
	logger_write_stdout(&this->log, this->logctl);
	logger_enable_timestamp(&this->log, 1);
	return 1;
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

static inline char *game_str8FD(struct game *this, char *str)
{
	char *ptr = strncpy(this->str8FD, str, 128);
	this->str8FD[127] = '\0';
	return ptr;
}

static inline void game_set8F8(struct game *this, float value)
{
	this->num8F8 = value;
}

static inline void game_set8FC(struct game *this, char value)
{
	this->ch8FC = value;
}

static inline void game_set9A0(struct game *this, unsigned value)
{
	this->num9A0 = value;
}

static inline void game_set9A4(struct game *this, unsigned value)
{
	this->num9A4 = value;
}

static inline int game_set97D_97E(struct game *this, unsigned value)
{
	this->num97D_97E_is_zero = value;
	return this->num97E_97D_is_zero = value == 0;
}

static inline int game_set97E_97D(struct game *this, unsigned value)
{
	this->num97E_97D_is_zero = value;
	return this->num97D_97E_is_zero = value == 0;
}

static inline char game_set982(struct game *this, char v)
{
	return this->ch982 = v;
}

static inline char game_set984(struct game *this, char v)
{
	return this->ch984 = v;
}

static inline char game_set985(struct game *this, char v)
{
	return this->ch985 = v;
}

static inline char game_set986(struct game *this, char v)
{
	return this->ch986 = v;
}

static inline char game_set987(struct game *this, char v)
{
	return this->ch987 = v;
}

static inline char game_set988(struct game *this, char v)
{
	return this->ch988 = v;
}

static inline char game_set989(struct game *this, char v)
{
	return this->ch989 = v;
}

static inline char game_set993(struct game *this, char v)
{
	return this->ch993 = v;
}

static inline char game_tbl98A(struct game *this, int index, char v)
{
	return this->tbl98A[index] = v;
}

static inline char game_tbl98A_2(struct game *this, int index, char mask)
{
	return this->tbl98A[index] = mask | (this->tbl98A[index] & 0xFE);
}

static inline char game_tbl98A_3(struct game *this, int index, char value)
{
	return this->tbl98A[index] = 2 * value | (this->tbl98A[index] & 1);
}

static inline char game_tbl994(struct game *this, int index, char v)
{
	return this->tbl994[index] = v;
}

static inline unsigned game_setA80(struct game *this, unsigned v)
{
	return this->numA80 = v;
}

static inline unsigned game_setA84(struct game *this, unsigned v)
{
	return this->numA84 = v;
}

int dtor_iobase(void *this, char ch)
{
	// TODO verify no op
	return printf("no op: dtor_iobase: %p, %d\n", this, (int)ch);
}

signed ctor_show_focus_screen(struct game *this)
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
	if (ctor_show_focus_screen(this)) {
		smtCreatewin(&this->cfg.window, 1, 1, NULL, 0);
	}
	return this;
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
	printf(
		"game options:\n"
		"resolution: (%d,%d)\n"
		"no_start: %d\n"
		"sys_memmap: %d\n"
		"midi: enable=%d, sync=%d, no_fill=%d\n"
		"sfx_enable: %d\n",
		this->cfg.width, this->cfg.height,
		this->cfg.no_start,
		this->cfg.sys_memmap,
		this->cfg.midi_enable,
		this->midi_sync,
		midi_no_fill,
		this->cfg.sfx_enable
	);
	return 0;
}

int game_parse_opt(struct game *this)
{
	stub
	return game_parse_opt2(this);
}

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
			if (should_start_game && !ctor_show_focus_screen(this) && !this->state)
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
	if (should_start_game && !start_game(this) && !this->cfg.reg_state)
		this->cfg.reg_state = 1;
	return this;
}

static struct option long_opt[] = {
	{"help", no_argument, 0, 0},
	{"root", required_argument, 0, 0},
	{0, 0, 0, 0}
};

static int parse_opt(int argc, char **argv)
{
	int c;
	while (1) {
		int option_index;
		c = getopt_long(argc, argv, "hr:", long_opt, &option_index);
		if (c == -1) break;
		switch (c) {
		case 'h':
			puts(
				TITLE " - Rise of Rome\n"
				"version " VERSION "\n"
				"common options:\n"
				"ch long  description\n"
				" h help  this help\n"
				" r root  load resources from directory\n"
				"original options:\n"
				"option       description\n"
				"1024         use 1024x768\n"
				"800          use 800x600\n"
				"640          use 640x480\n"
				"nostartup    do not start game\n"
				"systemmemory query available memory\n"
				"midimusic    enable midi music\n"
				"msync        sync midi playback\n"
				"nosound      disable sound\n"
				"nomusic      disable music\n"
			);
			break;
		case 'r':
			if (chdir(optarg)) {
				perror(optarg);
				exit(1);
			}
			break;
		default:
			fprintf(stderr, "?? getopt returned character code 0%o ??\n", c);
		}
	}
	return optind;
}

int main(int argc, char **argv)
{
	struct game AOE;
	struct game_cfg *c = &AOE.cfg;
	int argp;
	char *optptr, options[OPTBUFSZ];
	size_t optsz = 0;
	unsigned hPrevInst = 0, hInst = 0, nShowCmd = 0;
	// our stuff
	atexit(cleanup);
	argp = parse_opt(argc, argv);
	// construct lpCmdLine (i.e. options) from remaining args
	options[0] = '\0';
	for (optptr = options; argp < argc; ++argp) {
		const char *arg = argv[argp];
		puts(arg);
		if (optsz + 1 < OPTBUFSZ) {
			int n = snprintf(optptr, OPTBUFSZ - optsz, "%s ", arg);
			if (n > 0) {
				optptr += n;
				optsz += n;
			}
		}
	}
	options[OPTBUFSZ - 1] = '\0';
	puts(options);
	// original stuff
	// see also fixme at game_cfg struct declaration
	strcpy(c->title, TITLE);
	strcpy(c->version, VERSION);
	strcpy(c->win_name, TITLE);
	strcpy(c->tr_world_txt, "tr_wrld.txt");
	strcpy(c->dir_empires, "data2/empires.dat");
	strcpy(c->reg_path, "Software/Microsoft/Games/Age of Empires/1.00");
	strcpy(c->optbuf, options);
	strcpy(c->icon, "AppIcon");
	c->menu_name[0] = '\0';
	strcpy(c->palette, "palette");
	strcpy(c->cursors, "mcursors");
	//strcpy(c->str2fd, (const char*)&off_557778);
	c->hPrevInst = hPrevInst;
	c->hInst = hInst;
	c->nshowcmd = nShowCmd;
	c->scroll0 = c->scroll1 = 84;
	c->num8E8 = 0x8701C5C1;
	c->tbl8EC[0] = 0x11D2337B;
	c->tbl8EC[1] = 0x60009B83;
	c->tbl8EC[2] = 0x08F50797;
	c->num8F8 = c->num8E8 + 1;
	c->tbl8FC[0] = c->tbl8EC[0];
	c->tbl8FC[1] = c->tbl8EC[1];
	c->tbl8FC[2] = c->tbl8EC[2];
	c->num404 = 0;
	c->num408 = 1;
	c->num40C = 3;
	c->num878 = 8;
	c->num87C = 0;
	c->tbl880[0] = c->tbl880[1] = c->tbl880[2] = 0;
	c->num888 = c->num88C = 1;
	c->no_start = 0;
	c->mouse_opts[0] = c->mouse_opts[1] = c->mouse_opts[2] = 1;
	c->sys_memmap = 0;
	c->midi_enable = c->sfx_enable = 1;
	c->midi_opts[4] = 1;
	c->midi_opts[5] = 1;
	c->midi_opts[6] = 3;
	c->num8CC = c->num8D4 = 4.0f;
	c->num8D8 = 0.05f;
	c->mouse_style = 2;
	c->midi_opts[0] = c->midi_opts[1] = c->midi_opts[2] = c->midi_opts[3] = 0;
	c->width = 800;
	c->height = 600;
	#define DIR_DATA2 "data2/"
	strcpy(c->dir_data2, DIR_DATA2);
	strcpy(c->dir_sound, "sound/");
	c->dir_empty[0] = '\0';
	strcpy(c->dir_save, "savegame/");
	strcpy(c->dir_scene, "scenario/");
	strcpy(c->dir_camp, "campaign/");
	strcpy(c->dir_data_2, DIR_DATA2);
	strcpy(c->dir_data_3, DIR_DATA2);
	strcpy(c->dir_movies, "avi/");
	game_ctor(&AOE, 1);
	unsigned error = AOE.vtbl->get_state(&AOE);
	printf("error=%u\n", error);
	if (!error) {
		int status = AOE.vtbl->main(&AOE);
		AOE.vtbl->get_state(&AOE);
		AOE.vtbl->dtor_io(&AOE, 1);
		return status;
	}
	if (error != 4) {
		AOE.vtbl->get_res_str(2001, c->prompt_title, 256);
		AOE.vtbl->strerr(&AOE, 1, error, 0, c->prompt_message, 256);
		AOE.vtbl->dtor_io(&AOE, 1);
		smtMsg(SMT_MSG_ERR, 0, c->prompt_title, c->prompt_message);
	}
	return 0;
}
