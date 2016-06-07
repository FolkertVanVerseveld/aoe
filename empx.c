#include <stdio.h>
#include <stdlib.h>
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

const char *data_interface = "Interfac.drs";
const char *data_border = "Border.drs";
const char *data_terrain = "Terrain.drs";
const char *data_graphics = "graphics.drs";
const char *data_sounds = "sounds.drs";
const char *directory_data = "data/";

struct obj42BF80 {
	unsigned time_millis;
};

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

struct game_cfg {
	unsigned reg_state;
	unsigned window;
	unsigned width, height;
	float gamespeed;
	unsigned difficulty;
	char pathfind, mp_pathfind;
	unsigned mouse_style;
	unsigned scroll_speed;
};

struct game {
	struct game_vtbl *vtbl;
	struct game_cfg cfg;
};

static inline void str2(char *buf, size_t n, const char *s1, const char *s2)
{
	snprintf(buf, n, "%s%s", s1, s2);
}

int read_data_mapping(const char *filename, const char *directory, int open_file)
{
	const char *tribe = "tribe";
	char name[512];
	stub
	str2(name, sizeof name, directory, filename);
	if (open_file) {
		int fd = open(name, O_RDONLY); // XXX verify mode
		if (fd == -1) {
			perror(name);
			return fd;
		}
		close(fd);
	}
	return 0;
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
	printf("size: %zu\n", sizeof (struct obj42BF80));
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
	
	return 1;
}

struct game *start_game(struct game *this)
{
	stub
	read_data_mapping(data_sounds   , ""            , 1);
	read_data_mapping(data_graphics , ""            , 0);
	read_data_mapping(data_interface, ""            , 0);
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
	return 0;
}

int game_parse_opt(struct game *this)
{
	stub
	return game_parse_opt2;
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
				perror("chdir");
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
