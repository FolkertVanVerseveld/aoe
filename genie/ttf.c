/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Genie game engine true type font support
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 * SDL2 true type font (=TTF) support completely written from scratch.
 * This subsystem looks for fonts in a fixed number of paths.
 */

#include "ttf.h"
#include <err.h>

#define TTF_INIT 1

static int ttf_init = 0;

static const char *ttf_paths[] = {
	"/usr/share/fonts/truetype/msttcorefonts",
	"/usr/share/fonts/truetype/droid",
	NULL
};

void genie_ttf_free(void)
{
	if (!ttf_init)
		return;

	ttf_init &= ~TTF_INIT;

	if (ttf_init)
		warnx(
			"%s: expected state to be zero, but got %d",
			__func__, ttf_init
		);
}

int genie_ttf_init(void)
{
	int error = 1;

	if (ttf_init) {
		warnx("%s: already initialized", __func__);
		return 0;
	}
	ttf_init = TTF_INIT;

	error = 0;
fail:
	return error;
}
