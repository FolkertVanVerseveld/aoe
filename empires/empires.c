/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Replicated Age of Empires shell
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <fcntl.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <xt/os_macros.h>

#include <genie/engine.h>
#include <genie/cfg.h>
#include <genie/dbg.h>
#include <genie/def.h>
#include <genie/gfx.h>
#include <genie/res.h>
#include <genie/fs.h>

#include "drs.h"

#include "sfx.h"

#include "ui.h"

#define TITLE "Age of Empires"

/* SDL handling */

unsigned init = 0;
SDL_Window *window;
SDL_Renderer *renderer;

#define BUFSZ 4096

int running = 0;

const char *vfx_start[] = {
	"logo1.avi",
	"logo2.avi",
	"intro.avi",
	NULL
};

const char *msc[] = {
#if XT_IS_LINUX
	"Track 2.wav",
	"Track 3.wav",
	"Track 4.wav",
	"Track 5.wav",
	"Track 6.wav",
	"Track 7.wav",
	"Track 8.wav",
	"Track 9.wav",
	"Track 10.wav",
	"Track 11.wav",
	"Track 12.wav",
	"Track 13.wav",
	"Track 14.wav",
#else
	"OPEN.MID",
	"WON.MID",
	"LOST.MID",
	"MUSIC1.MID",
	"MUSIC2.MID",
	"MUSIC3.MID",
	"MUSIC4.MID",
	"MUSIC5.MID",
	"MUSIC6.MID",
	"MUSIC7.MID",
	"MUSIC8.MID",
	"MUSIC9.MID",
#endif
};

const struct ge_start_cfg ge_start_cfg = {
	"Age of Empires",
	vfx_start,
	msc
};

unsigned const music_list[] = {
	MUS_GAME1, MUS_GAME2, MUS_GAME3, MUS_GAME4, MUS_GAME5,
	MUS_GAME6, MUS_GAME7, MUS_GAME8, MUS_GAME9,
#ifndef _WIN32
	MUS_GAME10,
#endif
};

unsigned music_index = 0;

int in_game = 0;

void gfx_update(void)
{
	int display;
	SDL_Rect bnds;

	if ((display = SDL_GetWindowDisplayIndex(window)) < 0)
		return;

	gfx_cfg.display = display;
	if (SDL_GetDisplayBounds(display, &bnds))
		return;

	dbgf("gfx_cfg resize to (%d,%d) %dx%d\n", bnds.x, bnds.y, bnds.w, bnds.h);
	gfx_cfg.scr_x = bnds.x;
	gfx_cfg.scr_y = bnds.y;
	//gfx_cfg.width = bnds.w;
	//gfx_cfg.height = bnds.h;
}

int main(int argc, char **argv)
{
	int err;

	if ((err = ge_init(&argc, argv)))
		return err;

	drs_add("Border.drs");
	drs_add("graphics.drs");
	drs_add("Interfac.drs");
	drs_add("sounds.drs");
	drs_add("Terrain.drs");

	sfx_init();
	gfx_init();
	drs_init();
	ui_init();

	gfx_update();

	ge_main();

	ui_free();
	drs_free();
	gfx_free();
	sfx_free();

	return ge_quit();
}
