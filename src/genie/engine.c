#include "engine.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "_build.h"
#include "ui.h"
#include "gfx.h"
#include "game.h"

#define GENIE_INIT 1
#define GENIE_MODE_NOSTART     1
#define GENIE_MODE_SYSMEM      2
#define GENIE_MODE_MIDI        4
#define GENIE_MODE_MSYNC       8
#define GENIE_MODE_NOSOUND    16
#define GENIE_MODE_MFILL      32
#define GENIE_MODE_640_480    64
#define GENIE_MODE_800_600   128
#define GENIE_MODE_1024_768  256
#define GENIE_MODE_NOMUSIC   512
#define GENIE_MODE_NMOUSE   1024

static unsigned genie_init = 0;
static unsigned genie_mode = 0;

char *root_path = NULL;

static const char *version_info =
	"commit: " GENIE_GIT_SHA "\n"
	"origin: " GENIE_GIT_ORIGIN "\n"
	"configured with: " GENIE_BUILD_OPTIONS
#if GENIE_CFG_TAUNTED
	"\nconfiguration is taunted\n"
	"no support will be provided"
#endif
	;

static const char *general_help =
#ifdef DEBUG
	"commit: " GENIE_GIT_SHA "\n"
#endif
	"common options:\n"
	"ch long     description\n"
	" h help     this help\n"
	" r root     load resources from directory\n"
	" v version  dump version info";

/* Supported command line options */
static struct option long_opt[] = {
	{"help", no_argument, 0, 'h'},
	{"root", required_argument, 0, 'r'},
	{"version", no_argument, 0, 'v'},
	{0, 0, 0, 0},
};

/**
 * \brief General exit-handling
 * Cleans up any resources and other stuff.
 */
static void genie_cleanup(void)
{
	if (!genie_init)
		return;

	genie_init &= ~GENIE_INIT;

	genie_gfx_free();
	genie_ui_free(&genie_ui);

	if (genie_init)
		warnx(
			"%s: expected state to be zero, but got %d",
			__func__, genie_init
		);

	genie_init = 0;
}

#define hasopt(x,a,b) (!strcasecmp(x, a b) || !strcasecmp(x, a "_" b) || !strcasecmp(x, a " " b))

static int ge_parse_opt_legacy(int argp, int argc, char *argv[])
{
	for (; argp < argc; ++argp) {
		const char *arg = argv[argp];
		if (hasopt(arg, "no", "startup"))
			genie_mode |= GENIE_MODE_NOSTART;
		else if (hasopt(arg, "system", "memory"))
			genie_mode |= GENIE_MODE_SYSMEM;
		else if (hasopt(arg, "midi", "music"))
			genie_mode |= GENIE_MODE_MIDI;
		else if (!strcasecmp(arg, "msync"))
			genie_mode |= GENIE_MODE_MSYNC;
		else if (!strcasecmp(arg, "mfill"))
			genie_mode |= GENIE_MODE_MFILL;
		else if (hasopt(arg, "no", "sound"))
			genie_mode |= GENIE_MODE_NOSOUND;
		else if (!strcmp(arg, "640"))
			genie_mode |= GENIE_MODE_640_480;
		else if (!strcmp(arg, "800"))
			genie_mode |= GENIE_MODE_800_600;
		else if (!strcmp(arg, "1024"))
			genie_mode |= GENIE_MODE_1024_768;
		else if (hasopt(arg, "no", "music"))
			genie_mode |= GENIE_MODE_NOMUSIC;
		else if (hasopt(arg, "normal", "mouse"))
			genie_mode |= GENIE_MODE_NMOUSE;
		else if (!strcasecmp(arg, "no") && argp + 1 < argc) {
			arg = argv[argp + 1];
			if (!strcasecmp(arg, "startup"))
				genie_mode |= GENIE_MODE_NOSTART;
			else if (!strcasecmp(arg, "sound"))
				genie_mode |= GENIE_MODE_NOSOUND;
			else if (!strcasecmp(arg, "music"))
				genie_mode |= GENIE_MODE_NOMUSIC;
			else
				break;
		} else
			break;
	}
	return argp;
}

int ge_print_options(char *str, size_t size)
{
	size_t n, max = size - 1;
	if (!size)
		return 0;
	*str = '\0';
	if (genie_mode & GENIE_MODE_NOSTART)
		strncat(str, "no_startup ", max);
	if (genie_mode & GENIE_MODE_SYSMEM)
		strncat(str, "system_memory ", max);
	if (genie_mode & GENIE_MODE_MIDI)
		strncat(str, "midi_music ", max);
	if (genie_mode & GENIE_MODE_MSYNC)
		strncat(str, "msync ", max);
	if (genie_mode & GENIE_MODE_MFILL)
		strncat(str, "mfill ", max);
	if (genie_mode & GENIE_MODE_NOSOUND)
		strncat(str, "nosound ", max);
	if (genie_mode & GENIE_MODE_640_480)
		strncat(str, "640 ", max);
	if (genie_mode & GENIE_MODE_800_600)
		strncat(str, "800 ", max);
	if (genie_mode & GENIE_MODE_1024_768)
		strncat(str, "1024 ", max);
	str[(n = strlen(str)) - 1] = '\0';
	return n;
}

/**
 * \brief Process command options and return index to first non-parsed argument
 */
static int ge_parse_opt(int argc, char *argv[], unsigned options)
{
	const char *game_title;
	int c, option_index;

	game_title = genie_ui.game_title;

	while (1) {
		/* Get next argument */
		c = getopt_long(argc, argv, "hr:v", long_opt, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 'v':
			printf("%s\n%s\n", game_title, version_info);
			exit(0);
			break;
		case 'h':
			printf("%s\n%s\n", game_title, general_help);
			exit(0);
			break;
		case 'r':
			root_path = strdup(optarg);
			if (!root_path) {
				fputs("Out of memory", stderr);
				exit(1);
			}
			break;
		default:
			fprintf(stderr, "Unknown option: 0%o (%c)\n", c, c);
			break;
		}
	}

	if (options & GE_INIT_LEGACY_OPTIONS)
		optind = ge_parse_opt_legacy(optind, argc, argv);

	return optind;
}

int ge_init(int argc, char **argv, const char *title, unsigned options)
{
	int argp;

	if (genie_init) {
		warnx("%s: already initialized", __func__);
		return 0;
	}

	genie_init |= GENIE_INIT;
	atexit(genie_cleanup);

	genie_ui.game_title = title;

	argp = ge_parse_opt(argc, argv, options);
#ifdef DEBUG
	char buf[256];
	ge_print_options(buf, sizeof buf);
	printf("options = \"%s\"\n", buf);
#endif
	return argp;
}

int ge_main(void)
{
	int error = 1;

	error = genie_ui_init(&genie_ui, &genie_game);
	if (error)
		goto fail;

	error = genie_gfx_init();
	if (error)
		goto fail;

	genie_game_init(&genie_game, &genie_ui);

	error = genie_game_main(&genie_game);
fail:
	return error;
}
