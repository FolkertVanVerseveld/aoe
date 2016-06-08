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

#define stub fprintf(stderr,"stub: %s\n",__func__);

int prng_seed;
unsigned disable_terrain_sound = 0;
unsigned midi_no_fill = 0;

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

#define DMAPBUFSZ 260
struct dmap {
	char *dblk;
	int fd;
	char *drs_data;
	struct dmap *next;
	char filename[DMAPBUFSZ];
} *drs_list = NULL;

struct game;

int game_parse_opt(struct game *this);
int game_parse_opt2(struct game *this);

struct game_vtbl {
	int (*parse_opt)(struct game*);
} g_vtbl = {
	.parse_opt = game_parse_opt2
}, g_vtbl2 = {
	.parse_opt = game_parse_opt
};

#define OPTBUFSZ 256

struct game_cfg {
	unsigned reg_state;
	unsigned window;
	unsigned width, height;
	float gamespeed;
	unsigned difficulty;
	char pathfind, mp_pathfind;
	unsigned mouse_style;
	unsigned scroll_speed;
	char optbuf[OPTBUFSZ];
	unsigned scroll0, scroll1;
	unsigned no_start;
	unsigned sys_memmap;
	unsigned midi_enable;
	unsigned sfx_enable;
	unsigned midi_opts[7];
};

struct game {
	struct game_vtbl *vtbl;
	struct game_cfg cfg;
	unsigned state;
	unsigned midi_sync;
};

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
provides additional error checking compared to original dissassembly
*/
int read_data_mapping(const char *filename, const char *directory, int no_stat)
{
	// data types that are read from drs files must honor 32 ABI
	typedef uint32_t size32_t;
	struct dbuf {
		char data[60];
		size32_t length;
	} buf;
	struct dmap map = {.dblk = NULL, .drs_data = NULL, .next = NULL};
	char name[DMAPBUFSZ];
	int fd = -1;
	int ret = -1;
	ssize_t n;
	stub
	str2(name, sizeof name, directory, filename);
	// deze twee regels komt dubbel voor in dissassembly
	// aangezien ik niet van duplicated code hou staat het nu hier
	fd = open(name, O_RDONLY); // XXX verify mode
	if (fd == -1) goto fail;
	if (!no_stat) {
		struct stat st;
		unsigned sz;
		if (fstat(fd, &st)) goto fail;
		sz = st.st_size;
		printf("drssz: 0x%x (%u)\n", sz, sz);
		map.dblk = malloc(st.st_size);
		if (!map.dblk) {
			perror(__func__);
			goto fail;
		}
		n = read(fd, map.dblk, st.st_size);
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
	strcpy(map.filename, filename);
	if (no_stat) {
		map.fd = fd;
		map.dblk = NULL;
		lseek(fd, 0, SEEK_CUR);
		n = read(fd, &buf, sizeof buf);
		if (n != sizeof buf) {
			fprintf(stderr, "%s: bad map\n", name);
			ret = n;
			goto fail;
		}
		printf("drssz: 0x%x (%u)\n", buf.length, buf.length);
		map.drs_data = malloc(buf.length);
		if (!map.drs_data) {
			perror(__func__);
			goto fail;
		}
		lseek(fd, 0, SEEK_SET);
		n = read(fd, map.drs_data, buf.length);
		if (n != buf.length) {
			fprintf(stderr, "%s: bad map\n", name);
			ret = n;
			goto fail;
		}
	} else {
		map.fd = -1;
		map.drs_data = map.dblk;
	}
	assert(map.drs_data);
	// TODO insert map into drs list
	// check for magic "1.00tribe"
	ret = strcmp(&map.drs_data[0x2C], "tribe");
	if (!ret) ret = strncmp(&map.drs_data[0x28], data_map_magic, sizeof data_map_magic);
	printf("map: hdr valid: %d\n", ret == 0);
fail:
	// TODO remove free's once they are used outside this function
	assert(map.drs_data && !map.dblk || map.drs_data == map.dblk);
	if (map.drs_data) free(map.drs_data);
	if (map.dblk && map.dblk != map.drs_data) free(map.dblk);
	if (fd != -1) close(fd);
	if (ret) perror(name);
	return ret;
}

int reg_init(struct game *this)
{
	stub
	cfg.flags |= CFG_ROOT;
	return 1;
}

int init_time(struct game *this)
{
	stub
	return 1;
}

#define prng_init(t) prng_seed=t

static inline char game_set_pathfind(struct game *this, char pathfind)
{
	return this->cfg.pathfind = pathfind;
}

static inline char game_set_mp_pathfind(struct game *this, char mp_pathfind)
{
	return this->cfg.mp_pathfind = mp_pathfind;
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
	// FIXME initialize this->cfg.optbuf
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
	return 0;
}

int game_parse_opt(struct game *this)
{
	stub
	return game_parse_opt2(this);
}

struct game *game_vtbl_init(struct game *this, int should_start_game)
{
	stub
	this->vtbl = &g_vtbl;
	this->cfg.reg_state = 0;
	smtCursor(SMT_CURS_ARROW, SMT_CURS_SHOW);
	if (reg_init(this)) {
		if (init_time(this)) {
			if (should_start_game && !ctor_show_focus_screen(this) && !this->cfg.reg_state)
				this->cfg.reg_state = 1;
		} else
			this->cfg.reg_state = 15;
	} else
		this->cfg.reg_state = 14;
	return this;
}

struct game *ctor_game_4FDFA0(struct game *this, int should_start_game)
{
	stub
	game_vtbl_init(this, 0);
	this->vtbl = &g_vtbl2;
	if (should_start_game && !start_game(this) && !this->cfg.reg_state)
		this->cfg.reg_state = 1;
	return this;
}

static struct option long_opt[] = {
	{"help", no_argument, 0, 0},
	{"root", required_argument, 0, 0},
	{0, 0, 0, 0}
};

static void parse_opt(int argc, char **argv)
{
	int c;
	while (1) {
		int option_index;
		c = getopt_long(argc, argv, "hr:", long_opt, &option_index);
		if (c == -1) break;
		switch (c) {
		case 'h':
			puts(
				"Age of Empires Expansion - Rise of Rome\n"
				"version 00.01.6.1006\n"
				"options:\n"
				"ch long  description\n"
				" h help  this help\n"
				" r root  load resources from directory"
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
}

int main(int argc, char **argv)
{
	struct game obj, *blk = &obj;
	parse_opt(argc, argv);
	ctor_game_4FDFA0(blk, 1);
	return 0;
}
