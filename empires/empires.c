/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

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

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "../setup/dbg.h"
#include "../setup/def.h"
#include "../setup/res.h"

#include "cfg.h"
#include "drs.h"
#include "fs.h"

#include "gfx.h"
#include "sfx.h"

#include "ui.h"

#define TITLE "Age of Empires"

/* SDL handling */

unsigned init = 0;
SDL_Window *window;
SDL_Renderer *renderer;

struct pe_lib lib_lang;

#define BUFSZ 4096

int polling = 0;
int running = 0;

struct config cfg = {0, CFG_MODE_800x600, 50};

int load_lib_lang(void)
{
	char buf[BUFSZ];
	fs_game_path(buf, BUFSZ, "language.dll");
	return pe_lib_open(&lib_lang, buf);
}

void handle_event(SDL_Event *ev)
{
	switch (ev->type) {
	case SDL_QUIT:
		running = 0;
		return;
	case SDL_KEYDOWN: keydown(&ev->key); break;
	case SDL_KEYUP: keyup(&ev->key); break;
	case SDL_MOUSEBUTTONDOWN: mousedown(&ev->button); break;
	case SDL_MOUSEBUTTONUP: mouseup(&ev->button); break;
	case SDL_WINDOWEVENT:
		switch (ev->window.event) {
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			repaint();
			break;
		}
		break;
	default:
		break;
	}
}

void wait_event_loop(void)
{
	SDL_Event ev;

	if (!SDL_WaitEvent(&ev))
		return;

	handle_event(&ev);
	display();
}

void poll_event_loop(void)
{
	SDL_Event ev;

	while (SDL_PollEvent(&ev))
		handle_event(&ev);

	display();

	SDL_Delay(50);

}

void main_event_loop(void)
{
	running = 1;
	display();

	while (running)
		if (polling)
			poll_event_loop();
		else
			wait_event_loop();
}

void cfg_parse(struct config *cfg, int argc, char **argv)
{
	for (int i = 1; i < argc; ++i) {
		if (!strcasecmp(argv[i], "nostartup"))
			cfg->options |= CFG_NO_VIDEO;
		else if (!strcmp(argv[i], "640"))
			cfg->screen_mode = CFG_MODE_640x480;
		else if (!strcmp(argv[i], "800"))
			cfg->screen_mode = CFG_MODE_800x600;
		else if (!strcmp(argv[i], "1024"))
			cfg->screen_mode = CFG_MODE_1024x768;
		else if (!strcasecmp(argv[i], "normalmouse"))
			cfg->options |= CFG_NORMAL_MOUSE;
		else if (!strcasecmp(argv[i], "nosound"))
			cfg->options |= CFG_NO_SOUND;
		else if (!strcasecmp(argv[i], "noterrainsound"))
			cfg->options |= CFG_NO_AMBIENT;
		else if (!strcasecmp(argv[i], "nomusic"))
			cfg->options |= CFG_NO_MUSIC;
		else if (!strcasecmp(argv[i], "mfill"))
			fputs("Matrox Video adapter not supported\n", stderr);
		else if (!strcasecmp(argv[i], "msync"))
			fputs("SoundBlaster AWE not supported\n", stderr);
		else if (!strcasecmp(argv[i], "midimusic"))
			fputs("Midi support unavailable\n", stderr);
		else if (i + 1 < argc &&
			(!strcasecmp(argv[i], "limit") || !strcasecmp(argv[i], "limit=")))
		{
			int n = atoi(argv[i + 1]);
			if (n < POP_MIN)
				n = POP_MIN;
			else if (n > POP_MAX)
				n = POP_MAX;
			cfg->limit = (unsigned)n;
		}
	}

	const char *mode = "???";
	switch (cfg->screen_mode) {
	case CFG_MODE_640x480: mode = "640x480"; break;
	case CFG_MODE_800x600: mode = "800x600"; break;
	case CFG_MODE_1024x768: mode = "1024x768"; break;
	}
	dbgf("screen mode: %s\n", mode);

	// TODO support different screen resolutions
	if (cfg->screen_mode != CFG_MODE_800x600)
		fputs("Unsupported screen resolution\n", stderr);
}

int main(int argc, char **argv)
{
	cfg_parse(&cfg, argc, argv);

	if (!find_setup_files())
		panic("Please insert or mount the game CD-ROM");
	if (load_lib_lang())
		panic("CD-ROM files are corrupt");

	game_installed = find_wine_installation();
	if (has_wine)
		dbgs("wine detected");
	dbgf("game installed: %s\n", game_installed ? "yes" : "no");

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
		panic("Could not initialize user interface");

	if (!(window = SDL_CreateWindow(
		TITLE, SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT,
		SDL_WINDOW_SHOWN)))
	{
		panic("Could not create user interface");
	}

	dbgf("Available render drivers: %d\n", SDL_GetNumVideoDrivers());

	// Create default renderer and don't care if it is accelerated.
	if (!(renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC)))
		panic("Could not create rendering context");

	drs_add("Border.drs");
	drs_add("graphics.drs");
	drs_add("Interfac.drs");
	drs_add("sounds.drs");
	drs_add("Terrain.drs");

	sfx_init();
	gfx_init();
	drs_init();
	ui_init();

	main_event_loop();

	ui_free();
	drs_free();
	gfx_free();
	sfx_free();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
