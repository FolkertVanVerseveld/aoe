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

int running = 0;

struct config cfg = {CFG_NORMAL_MOUSE, CFG_MODE_800x600, 50, 0.7, 1, 0.3};

unsigned const music_list[] = {
	MUS_GAME1, MUS_GAME2, MUS_GAME3, MUS_GAME4, MUS_GAME5,
	MUS_GAME6, MUS_GAME7, MUS_GAME8, MUS_GAME9,
#ifndef _WIN32
	MUS_GAME10,
#endif
};

unsigned music_index = 0;

int in_game = 0;

int load_lib_lang(void)
{
	char buf[BUFSZ];
	fs_game_path(buf, BUFSZ, "language.dll");
	return pe_lib_open(&lib_lang, buf);
}

static void toggle_fullscreen(void)
{
	unsigned flags = SDL_GetWindowFlags(window);

	if (flags & SDL_WINDOW_FULLSCREEN) {
		SDL_SetWindowFullscreen(window, 0);
		gfx_update();
		return;
	}

	int desired_width, desired_height;

	switch (cfg.screen_mode) {
	case CFG_MODE_640x480: desired_width = 640; desired_height = 480; break;
	case CFG_MODE_1024x768: desired_width = 1024; desired_height = 768; break;
	default: desired_width = 800; desired_height = 600; break;
	}

	// only check the display the main window is located
	int display, modes;

	if ((display = SDL_GetWindowDisplayIndex(window)) < 0 || (modes = SDL_GetNumDisplayModes(display)) < 0) {
		show_error_format("Can't go to fullscreen mode: can't query display info: %s", SDL_GetError());
		return;
	}

	// check if the fullscreen mode is supported
	SDL_DisplayMode best = {.w = 0, .h = 0, .refresh_rate = 1};

	for (int i = 0; i < modes; ++i) {
		SDL_DisplayMode mode;
		// skip any modes we can't query
		if (SDL_GetDisplayMode(display, i, &mode))
			continue;

		dbgf("mode: %dx%d %dHz\n", mode.w, mode.h, mode.refresh_rate);

		if (mode.w == desired_width && mode.h == desired_height && mode.refresh_rate > best.refresh_rate)
			best = mode;
	}

	if (!best.w || !best.h) {
		show_error_format("Can't go to fullscreen mode: display does not support %dx%d", desired_width, desired_height);
		return;
	}

	// FIXME fucked up fullscreen mode
	/*
	 * For some reason, whenever i have two displays and go fullscreen from
	 * windowed mode 800x600 in the second display (the first one apparantly
	 * does not support anything else than 1920x1080), the fullscreen
	 * display goes in a really fucked up state where the display looks like
	 * to have proper dimensions (only aspect ratio has changed, but that's
	 * okay because we didn't ask to keep it the same) until i move the
	 * mouse to the far right edge of the screen where it continues to
	 * scroll as if the fullscreen display is really like 1920x600...
	 *
	 * So in conclusion, the display state gets fucked up and I have no idea
	 * why...
	 */
	SDL_SetWindowDisplayMode(window, &best);
	SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
	gfx_update();
}

void handle_event(SDL_Event *ev)
{
	switch (ev->type) {
	case SDL_QUIT:
		running = 0;
		return;
	case SDL_KEYDOWN: keydown(&ev->key); break;
	case SDL_KEYUP:
		if (ev->key.keysym.sym == SDLK_F4 || ev->key.keysym.sym == SDLK_F11)
			toggle_fullscreen();
		else
			keyup(&ev->key);
		break;
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

void main_event_loop(void)
{
	SDL_Event ev;

	running = 1;
	display();

	while (running) {
		while (SDL_PollEvent(&ev))
			handle_event(&ev);

		idle();
		display();

		SDL_Delay(50);

		if (!music_playing && in_game) {
			music_index = (music_index + 1) % ARRAY_SIZE(music_list);
			mus_play(music_list[music_index]);
		}
	}
}

#define hasopt(x,a,b) (!strcasecmp(x, a b) || !strcasecmp(x, a "_" b) || !strcasecmp(x, a " " b))

void cfg_parse(struct config *cfg, int argc, char **argv)
{
	for (int i = 1; i < argc; ++i) {
		if (hasopt(argv[i], "no", "startup"))
			cfg->options |= CFG_NO_INTRO;
		else if (hasopt(argv[i], "system", "memory"))
			fputs("System memory not supported\n", stderr);
		else if (hasopt(argv[i], "midi", "music"))
			fputs("Midi support unavailable\n", stderr);
		else if (!strcasecmp(argv[i], "msync"))
			fputs("SoundBlaster AWE not supported\n", stderr);
		else if (!strcasecmp(argv[i], "mfill"))
			fputs("Matrox Video adapter not supported\n", stderr);
		else if (hasopt(argv[i], "no", "sound"))
			cfg->options |= CFG_NO_SOUND;
		else if (!strcmp(argv[i], "640"))
			cfg->screen_mode = CFG_MODE_640x480;
		else if (!strcmp(argv[i], "800"))
			cfg->screen_mode = CFG_MODE_800x600;
		else if (!strcmp(argv[i], "1024"))
			cfg->screen_mode = CFG_MODE_1024x768;
		else if (hasopt(argv[i], "no", "music"))
			cfg->options |= CFG_NO_MUSIC;
		else if (hasopt(argv[i], "normal", "mouse"))
			cfg->options |= CFG_NORMAL_MOUSE;
		/* Rise of Rome options: */
		else if (hasopt(argv[i], "no", "terrainsound"))
			cfg->options |= CFG_NO_AMBIENT;
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
		show_error("Unsupported screen resolution");

	switch (cfg->screen_mode) {
	case CFG_MODE_640x480: gfx_cfg.width = 640; gfx_cfg.height = 480; break;
	case CFG_MODE_800x600: gfx_cfg.width = 800; gfx_cfg.height = 600; break;
	case CFG_MODE_1024x768: gfx_cfg.width = 1024; gfx_cfg.height = 768; break;
	}
}

void gfx_update(void)
{
	int display;
	SDL_Rect bnds;

	if ((display = SDL_GetWindowDisplayIndex(window)) < 0)
		return;

	gfx_cfg.display = display;
	if (SDL_GetDisplayBounds(display, &bnds))
		return;

	gfx_cfg.scr_x = bnds.x;
	gfx_cfg.scr_y = bnds.y;
	gfx_cfg.width = bnds.w;
	gfx_cfg.height = bnds.h;
}

void video_play(const char *name)
{
#ifdef _WIN32
	fprintf(stderr, "%s: no video playback on windows yet!\n", name);
#else
	char path[4096], buf[4096];

	if (cfg.options & CFG_NO_INTRO)
		return;

	fs_get_path(path, sizeof path, "avi/", name, 0);
	if (access(path, F_OK | R_OK)) {
		// retry and force to read from CD-ROM
		fs_get_path(path, sizeof path, "game/avi/", name, FS_OPT_NEED_CDROM);

		if (access(path, F_OK | R_OK)) {
			fprintf(stderr, "%s: file not found or readable\n", path);
			return;
		}
	}

	/*
	 * we have no real way to check if ffplay is installed, so we just try
	 * it and see if it fails. an unknown command error yields code 0x7f00
	 * on my machine...
	 *
	 * if ffplay fails for whatever reason, try cvlc and don't bother
	 * checking if that worked because there are no real distro independent
	 * alternatives to try after that point...
	 */
	snprintf(buf, sizeof buf, "ffplay -fs -loop 1 -autoexit \"%s\"", path);
	int code = system(buf);
	if (code < 0 || code == 0x7f00) {
		// probably command not found... try cvlc
		snprintf(buf, sizeof buf, "cvlc --play-and-exit -f \"%s\"", path);
		system(buf);
	}
#endif
}

// hack for windows...
#ifdef main
	#undef main
#endif

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

	video_play("logo1.avi");
	video_play("logo2.avi");
	video_play("intro.avi");

	/* Setup graphical state */
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
		panic("Could not initialize user interface");

	if (!(window = SDL_CreateWindow(
		TITLE, SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, gfx_cfg.width, gfx_cfg.height,
		SDL_WINDOW_SHOWN)))
	{
		panic("Could not create user interface");
	}

	dbgf("Available render drivers: %d\n", SDL_GetNumVideoDrivers());

	/* Create default renderer and don't care if it is accelerated. */
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

	gfx_update();

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
