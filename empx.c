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
