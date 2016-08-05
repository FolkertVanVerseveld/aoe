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

// NOPTR typeof(game *AOE) == struct game
struct game AOE;
// NOPTR typeof(game_config *cfg) == struct game_config
struct game_config cfg = {
	.title = TITLE,
	.version = VERSION,
	.win_name = TITLE,
	.tr_world_txt = "tr_wrld.txt",
	.dir_empires = "data2/empires.dat",
	.reg_path = "Software/Microsoft/Games/Age of Empires/1.00",
	// optbuf gets set in main
	.icon = "AppIcon",
	.menu_name = "",
	.palette = "palette",
	.cursors = "mcursors",
	.strAOE = "AOE",
	.hPrevInst = SMT_RES_INVALID,
	.hInst = SMT_RES_INVALID,
	.nshowcmd = 0,
	.scroll0 = 84,
	.scroll1 = 84,
	.winctl = {
		0x8701C5C1,
		0x11D2337B,
		0x60009B83,
		0x08F50797
	},
	.winctl2 = {
		0x8701C5C2,
		0x11D2337B,
		0x60009B83,
		0x08F50797
	},
	.d0 = 0,
	.d1p0 = 1,
	.d3 = 3,
	.d8p0 = 8,
	.chk_time = 0,
	.time = {0, 0, 0},
	.d1p1 = 1,
	.window_request_focus = 1,
	.no_start = 0,
	.window_show_focus_update = 1,
	.mouse_opts = 1,
	.gfx8bitchk = 1,
	.sys_memmap = 0,
	.midi_enable = 1,
	.sfx_enable = 1,
	// index 4-6 get set before mouse_style
	// index 0-3 after mouse_style
	.midi_opts = {0, 0, 0, 0, 1, 1, 3},
	.f4_0p0 = 4.0f,
	.f4_0p1 = 4.0f,
	.f0_5 = 0.5f,
	.mouse_style = 2,
	.width = 800,
	.height = 600,
	.dir_data2 = DIR_DATA2,
	.dir_sound = "sound/",
	.dir_empty = "",
	.dir_save = "savegame/",
	.dir_scene = "scenario/",
	.dir_camp = "campaign/",
	.dir_data_2 = DIR_DATA2,
	.dir_data_3 = DIR_DATA2,
	.dir_movies = "avi/",
};

int main(int argc, char **argv)
{
	int argp;
	char *optptr, options[OPTBUFSZ];
	size_t optsz = 0;
	unsigned hPrevInst = 0, hInst = 0, nShowCmd = 0;
	/* < */
	atexit(drs_free);
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
	/* > */
	// see also fixme at game_config struct declaration
	strcpy(cfg.optbuf, options);
	cfg.hPrevInst = hPrevInst;
	cfg.hInst = hInst;
	cfg.nshowcmd = nShowCmd;
	game_ctor(&AOE, &cfg, 1);
	unsigned error = AOE.vtbl->get_state(&AOE);
	printf("error=%u\n", error);
	if (!error) {
		int status = AOE.vtbl->main(&AOE);
		AOE.vtbl->get_state(&AOE);
		AOE.vtbl->dtor(&AOE, 0);
		return status;
	}
	if (error != GE_FOCUS) {
		AOE.vtbl->get_res_str(STR_ERROR, cfg.prompt_title, 256);
		AOE.vtbl->strerr(&AOE, 1, error, 0, cfg.prompt_message, 256);
		AOE.vtbl->dtor(&AOE, 0);
		smtMsg(SMT_MSG_ERR, 0, cfg.prompt_title, cfg.prompt_message);
	}
	return 0;
}
