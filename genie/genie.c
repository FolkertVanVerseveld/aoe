/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <genie/engine.h>

#include <genie/dbg.h>
#include <genie/def.h>
#include <genie/gfx.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct GE_config GE_cfg = {GE_CFG_NORMAL_MOUSE, GE_CFG_MODE_800x600, 50, 0.7, 1, 0.3};

#define hasopt(x,a,b) (!strcasecmp(x, a b) || !strcasecmp(x, a "_" b) || !strcasecmp(x, a " " b))

static void ge_cfg_parse(struct config *cfg, int argc, char **argv)
{
	for (int i = 1; i < argc; ++i) {
		if (hasopt(argv[i], "no", "startup"))
			GE_cfg.options |= GE_CFG_NO_INTRO;
		/*
		 * These options are added for compatibility, but we
		 * just print we don't support any of them.
		 */
		else if (hasopt(argv[i], "system", "memory"))
			fputs("System memory not supported\n", stderr);
		else if (hasopt(argv[i], "midi", "music"))
			fputs("Midi support unavailable\n", stderr);
		else if (!strcasecmp(argv[i], "msync"))
			fputs("SoundBlaster AWE not supported\n", stderr);
		else if (!strcasecmp(argv[i], "mfill"))
			fputs("Matrox Video adapter not supported\n", stderr);
		else if (hasopt(argv[i], "no", "sound"))
			GE_cfg.options |= GE_CFG_NO_SOUND;
		else if (!strcmp(argv[i], "640"))
			GE_cfg.screen_mode = GE_CFG_MODE_640x480;
		else if (!strcmp(argv[i], "800"))
			GE_cfg.screen_mode = GE_CFG_MODE_800x600;
		else if (!strcmp(argv[i], "1024"))
			GE_cfg.screen_mode = GE_CFG_MODE_1024x768;
		/* Custom option: disable changing screen resolution. */
		else if (hasopt(argv[i], "no", "changeres"))
			GE_cfg.options = GE_CFG_NATIVE_RESOLUTION;
		else if (hasopt(argv[i], "no", "music"))
			GE_cfg.options |= GE_CFG_NO_MUSIC;
		else if (hasopt(argv[i], "normal", "mouse"))
			GE_cfg.options |= GE_CFG_NORMAL_MOUSE;
		/* Rise of Rome options: */
		else if (hasopt(argv[i], "no", "terrainsound"))
			GE_cfg.options |= GE_CFG_NO_AMBIENT;
		else if (i + 1 < argc &&
			(!strcasecmp(argv[i], "limit") || !strcasecmp(argv[i], "limit=")))
		{
			int n = atoi(argv[i + 1]);
			if (n < GE_POP_MIN)
				n = GE_POP_MIN;
			else if (n > GE_POP_MAX)
				n = GE_POP_MAX;
			GE_cfg.limit = (unsigned)n;
		}
	}

	const char *mode = "???";
	switch (GE_cfg.screen_mode) {
	case GE_CFG_MODE_640x480: mode = "640x480"; break;
	case GE_CFG_MODE_800x600: mode = "800x600"; break;
	case GE_CFG_MODE_1024x768: mode = "1024x768"; break;
	}
	dbgf("screen mode: %s\n", mode);

	// TODO support different screen resolutions
	if (GE_cfg.screen_mode != GE_CFG_MODE_800x600)
		show_error("Unsupported screen resolution");

	switch (GE_cfg.screen_mode) {
	case GE_CFG_MODE_640x480: gfx_cfg.width = 640; gfx_cfg.height = 480; break;
	case GE_CFG_MODE_800x600: gfx_cfg.width = 800; gfx_cfg.height = 600; break;
	case GE_CFG_MODE_1024x768: gfx_cfg.width = 1024; gfx_cfg.height = 768; break;
	}
}

int GE_Init(int *pargc, char **argv)
{
	int argc = *pargc;

	for (int i = 1; i < argc; ++i) {
		const char *arg = argv[i];

		if (!strcmp(arg, "--version") || !strcmp(arg, "-v")) {
			puts("Genie Engine v0");
		}

		if (!strcmp(arg, "--"))
			break;
	}

	ge_cfg_parse(&GE_cfg, argc, argv);

	return 0;
}

int GE_Quit(void)
{
	return 0;
}
