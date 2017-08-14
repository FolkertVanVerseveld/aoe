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
#include "engine.h"
#include "prompt.h"

#include <err.h>
#include <stdlib.h>

#define TTF_INIT 1
#define TTF_INIT_LIB 2

static int ttf_init = 0;

#define TTF_COUNT 6

static TTF_Font *ttf_tbl[TTF_COUNT];
static const char *ttf_names[TTF_COUNT] = {
	"ARIAL.TTF",
	"ARIALBD.TTF",
	"comic.ttf",
	"comicbd.ttf",
	"COPRGTL.TTF",
	"COPRGTB.TTF",
};
static int ttf_pts[TTF_COUNT] = {
	12, 12, 12, 12, 16, 24
};

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x)[0])

static void ttf_close(void)
{
	for (unsigned i = 0, n = TTF_COUNT; i < n; ++i)
		if (ttf_tbl[i]) {
			TTF_CloseFont(ttf_tbl[i]);
			ttf_tbl[i] = NULL;
		}
}

static int ttf_open(void)
{
	char path[4096];
	int retval = 1;

	for (unsigned i = 0, n = TTF_COUNT; i < n; ++i) {
		genie_ttf_path(path, sizeof path, ttf_names[i]);
		ttf_tbl[i] = TTF_OpenFont(path, ttf_pts[i]);
		if (!ttf_tbl[i]) {
			char buf[80];
			snprintf(buf, sizeof buf, "Missing font \"%s\"", ttf_names[i]);
			show_error(buf, TTF_GetError());
			goto fail;
		}
	}
	retval = 0;
fail:
	return retval;
}

void genie_ttf_free(void)
{
	if (!ttf_init)
		return;

	ttf_init &= ~TTF_INIT;

	ttf_close();

	if (ttf_init & TTF_INIT_LIB) {
		TTF_Quit();
		ttf_init &= ~TTF_INIT_LIB;
	}

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

	if (TTF_Init()) {
		show_error("Fonts not available", TTF_GetError());
		goto fail;
	}
	ttf_init |= TTF_INIT_LIB;

	error = ttf_open();
fail:
	return error;
}

SDL_Surface *genie_ttf_render_solid(unsigned id, const char *text, SDL_Color color)
{
	SDL_Surface *surf = TTF_RenderText_Solid(ttf_tbl[id], text, color);
	if (!surf) {
		char buf[256];
		snprintf(buf, sizeof buf, "Font rendering failed: %s", TTF_GetError());
		show_error("Fatal error", buf);
		exit(1);
	}
	return surf;
}
