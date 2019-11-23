/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <genie/engine.h>

#include <genie/dbg.h>
#include <genie/def.h>
#include <genie/gfx.h>
#include <genie/res.h>
#include <genie/fs.h>

#include <xt/error.h>
#include <xt/os.h>
#include <xt/string.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct pe_lib lib_lang;

struct ge_config ge_cfg = {GE_CFG_NORMAL_MOUSE, GE_CFG_MODE_800x600, 50, 0.7, 1, 0.3};

#define hasopt(x,a,b) (!strcasecmp(x[i], a b) || !strcasecmp(x[i], a "_" b) || !strcasecmp(x[i], a " " b) || (i + 1 < argc && !strcasecmp(x[i], a) && !strcasecmp(x[i+1], b)))

static void _ge_cfg_parse(struct ge_config *cfg, int argc, char **argv)
{
	for (int i = 1; i < argc; ++i) {
		if (hasopt(argv, "no", "startup"))
			cfg->options |= GE_CFG_NO_INTRO;
		/*
		 * These options are added for compatibility, but we
		 * just print we don't support any of them.
		 */
		else if (hasopt(argv, "system", "memory")) {
			// XXX this is not what the original game does
			fputs("System memory not supported\n", stderr);
			struct xtRAMInfo info;
			int code;

			if ((code = xtRAMGetInfo(&info)) != 0) {
				xtPerror("No system memory", code);
			} else {
				char strtot[32], strfree[32];

				xtFormatBytesSI(strtot, sizeof strtot, info.total - info.free, 2, true, NULL);
				xtFormatBytesSI(strfree, sizeof strfree, info.total, 2, true, NULL);

				printf("System memory: %s / %s\n", strtot, strfree);
			}
		} else if (hasopt(argv, "midi", "music"))
			fputs("Midi support unavailable\n", stderr);
		else if (!strcasecmp(argv[i], "msync"))
			fputs("SoundBlaster AWE not supported\n", stderr);
		else if (!strcasecmp(argv[i], "mfill"))
			fputs("Matrox Video adapter not supported\n", stderr);
		else if (hasopt(argv, "no", "sound"))
			cfg->options |= GE_CFG_NO_SOUND;
		else if (!strcmp(argv[i], "640"))
			cfg->screen_mode = GE_CFG_MODE_640x480;
		else if (!strcmp(argv[i], "800"))
			cfg->screen_mode = GE_CFG_MODE_800x600;
		else if (!strcmp(argv[i], "1024"))
			cfg->screen_mode = GE_CFG_MODE_1024x768;
		/* Custom option: disable changing screen resolution. */
		else if (hasopt(argv, "no", "changeres"))
			cfg->options = GE_CFG_NATIVE_RESOLUTION;
		else if (hasopt(argv, "no", "music"))
			cfg->options |= GE_CFG_NO_MUSIC;
		else if (hasopt(argv, "normal", "mouse"))
			cfg->options |= GE_CFG_NORMAL_MOUSE;
		/* Rise of Rome options: */
		else if (hasopt(argv, "no", "terrainsound"))
			cfg->options |= GE_CFG_NO_AMBIENT;
		else if (i + 1 < argc &&
			(!strcasecmp(argv[i], "limit") || !strcasecmp(argv[i], "limit=")))
		{
			int n = atoi(argv[i + 1]);
			if (n < GE_POP_MIN)
				n = GE_POP_MIN;
			else if (n > GE_POP_MAX)
				n = GE_POP_MAX;
			cfg->limit = (unsigned)n;
		}
	}

	const char *mode = "???";
	switch (cfg->screen_mode) {
	case GE_CFG_MODE_640x480: mode = "640x480"; break;
	case GE_CFG_MODE_800x600: mode = "800x600"; break;
	case GE_CFG_MODE_1024x768: mode = "1024x768"; break;
	}
	dbgf("screen mode: %s\n", mode);

	// TODO support different screen resolutions
	if (cfg->screen_mode != GE_CFG_MODE_800x600)
		show_error("Unsupported screen resolution");

	switch (cfg->screen_mode) {
	case GE_CFG_MODE_640x480: gfx_cfg.width = 640; gfx_cfg.height = 480; break;
	case GE_CFG_MODE_800x600: gfx_cfg.width = 800; gfx_cfg.height = 600; break;
	case GE_CFG_MODE_1024x768: gfx_cfg.width = 1024; gfx_cfg.height = 768; break;
	}
}

#define BUFSZ 4096

int load_lib_lang(void)
{
	char buf[BUFSZ];
	fs_game_path(buf, BUFSZ, "language.dll");
	return pe_lib_open(&lib_lang, buf);
}

int ge_init(int *pargc, char **argv, const char **vfx)
{
	int argc = *pargc;

	for (int i = 1; i < argc; ++i) {
		const char *arg = argv[i];

		if (!strcmp(arg, "--version") || !strcmp(arg, "-v"))
			puts("Genie Engine v0");

		if (!strcmp(arg, "--"))
			break;
	}

	_ge_cfg_parse(&ge_cfg, argc, argv);

	if (!find_setup_files()) {
		show_error("Please insert or mount the game CD-ROM");
		return 1;
	}

	if (load_lib_lang()) {
		show_error("CD-ROM files are corrupt");
		return 1;
	}

	game_installed = find_game_installation();
	if (has_wine)
		dbgs("wine detected");
	dbgf("game installed: %s\n", game_installed ? "yes" : "no");

	for (unsigned i = 0; vfx[i]; ++i)
		ge_video_play(vfx[i]);

	return 0;
}

int ge_quit(void)
{
	pe_lib_close(&lib_lang);
	return 0;
}
