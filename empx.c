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
#include "dmap.h"
#include "game.h"
#include "langx.h"
#include "log.h"
#include "todo.h"

#define TITLE "Age of Empires Expansion"
#define VERSION "00.01.6.1006"

#define DIR_DATA2 "data2/"

int prng_seed;

struct obj42BF80 {
	unsigned time_millis;
};

int dtor_iobase(void*, char);

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

static void cleanup(void) {
	game_free();
	drs_free();
}

static const char data_map_magic[4] = {'1', '.', '0', '0'};

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

struct game AOE;
struct game_config cfg = {
	.tr_world_txt = "tr_wrld.txt",
	.dir_empires = "data2/empires.dat",
	.reg_path = "Software/Microsoft/Games/Age of Empires/1.00",
	.menu_name = "",
	.icon = "AppIcon",
	.palette = "palette",
	.cursors = "mcursors",
	.scroll0 = 84,
	.scroll1 = 84,
	.no_start = 0,
	.mouse_opts = {1, 1},
	.gfx8bitchk = 1,
	.sys_memmap = 0,
	.midi_enable = 1,
	.sfx_enable = 1,
	.mouse_style = 2,
	.midi_opts = {0, 0, 0, 0, 1, 1, 3},
	.width = 800,
	.height = 600,
	.dir_empty = "",
	.dir_data2 = DIR_DATA2,
	.dir_sound = "sound/",
	.dir_save = "savegame/",
	.dir_scene = "scenario/",
	.dir_camp = "campaign/",
	.dir_data_2 = DIR_DATA2,
	.dir_data_3 = DIR_DATA2,
	.dir_movies = "avi/",
	.time = {0, 0, 0},
};

int main(int argc, char **argv)
{
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
	// see also fixme at game_config struct declaration
	strcpy(cfg.title, TITLE);
	strcpy(cfg.version, VERSION);
	strcpy(cfg.win_name, TITLE);
	strcpy(cfg.optbuf, options);
	//strcpy(cfg.str2fd, (const char*)&off_557778);
	cfg.hPrevInst = hPrevInst;
	cfg.hInst = hInst;
	cfg.nshowcmd = nShowCmd;
	game_ctor(&AOE, &cfg, 1);
	unsigned error;
	// INLINED error = AOE.vtbl->get_state(&AOE);
	error = game_get_state(&AOE);
	printf("error=%u\n", error);
	if (!error) {
		int status = AOE.vtbl->main(&AOE);
		// INLINED AOE.vtbl->get_state(&AOE);
		game_get_state(&AOE);
		AOE.vtbl->dtor_io(&AOE, 1);
		return status;
	}
	if (error != 4) {
		AOE.vtbl->get_res_str(2001, cfg.prompt_title, 256);
		AOE.vtbl->strerr(&AOE, 1, error, 0, cfg.prompt_message, 256);
		AOE.vtbl->dtor_io(&AOE, 1);
		smtMsg(SMT_MSG_ERR, 0, cfg.prompt_title, cfg.prompt_message);
	}
	return 0;
}
