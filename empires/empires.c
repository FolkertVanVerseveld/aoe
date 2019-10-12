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

unsigned const music_list[] = {
	MUS_GAME1, MUS_GAME2, MUS_GAME3, MUS_GAME4, MUS_GAME5,
	MUS_GAME6, MUS_GAME7, MUS_GAME8, MUS_GAME9,
#ifndef _WIN32
	MUS_GAME10,
#endif
};

unsigned music_index = 0;

int in_game = 0;

#define FSE_QUERY 1
#define FSE_NO_SUPPORT 2

static void fse_show_error(int code, int desired_width, int desired_height)
{
	switch (code) {
	case FSE_QUERY: show_error_format("Can't go to fullscreen mode: can't query display info: %s", SDL_GetError()); break;
	case FSE_NO_SUPPORT: show_error_format("Can't go to fullscreen mode: display does not support %dx%d", desired_width, desired_height); break;
	default: show_error("Well, this is interesting"); break;
	}
}

static int best_fullscreen_mode(SDL_DisplayMode *bnds, int display, int desired_width, int desired_height)
{
	int modes;

	if ((modes = SDL_GetNumDisplayModes(display)) < 0)
		return FSE_QUERY;

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

	if (!best.w || !best.h)
		return FSE_NO_SUPPORT;

	*bnds = best;
	return 0;
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

	switch (GE_cfg.screen_mode) {
	case GE_CFG_MODE_640x480: desired_width = 640; desired_height = 480; break;
	case GE_CFG_MODE_1024x768: desired_width = 1024; desired_height = 768; break;
	default: desired_width = 800; desired_height = 600; break;
	}

	// FIXME fucked up fullscreen mode on linux
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
	 * why... we could change the UI subsystem to only allow fullscreen mode
	 * if it is either on the primary display or OpenGL is rendering the
	 * window rather than the SDL renderer.
	 */

	// only check the display the main window is located
	int err, count, display, modes;

	if ((count = SDL_GetNumVideoDisplays()) < 1 || (display = SDL_GetWindowDisplayIndex(window)) < 0) {
		fse_show_error(FSE_QUERY, desired_width, desired_height);
		return;
	}

	// try display window is located at
	SDL_DisplayMode best;

	if (!(err = best_fullscreen_mode(&best, display, desired_width, desired_height))) {
		dbgf("using mode: %dx%d %dHz format: %u\n", best.w, best.h, best.refresh_rate, best.format);

		SDL_SetWindowDisplayMode(window, &best);
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
		gfx_update();

		return;
	}

	// try all other displays skipping the one we've just tried
	for (int i = 0; i < count; ++i) {
		if (i == display)
			continue;

		if (!(err = best_fullscreen_mode(&best, i, desired_width, desired_height))) {
			SDL_Rect bnds;

			if (SDL_GetDisplayBounds(i, &bnds)) {
				err = FSE_QUERY;
				continue;
			}

			dbgf("using mode: %dx%d %dHz format: %u\n", best.w, best.h, best.refresh_rate, best.format);
			dbgf("move to: (%d,%d)\n", bnds.x, bnds.y);

			SDL_SetWindowPosition(window, bnds.x, bnds.y);
			SDL_SetWindowDisplayMode(window, &best);
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
			gfx_update();

			return;
		}
	}

	fse_show_error(err ? err : FSE_NO_SUPPORT, desired_width, desired_height);
}

void handle_event(SDL_Event *ev)
{
	switch (ev->type) {
	case SDL_QUIT:
		running = 0;
		return;
	case SDL_KEYDOWN: keydown(&ev->key); break;
	case SDL_KEYUP:
		/*
		 * Third party bug: whenever a zenity prompt is closed,
		 * SDL receives a spurious SDL_KEYUP event for SDLK_F4
		 */
		if (ev->key.keysym.sym == SDLK_F11)
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

void video_play(const char *name)
{
#if XT_IS_WINDOWS
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

int main(int argc, char **argv)
{
	int err;

	if ((err = GE_Init(&argc, argv)))
		return err;

	game_installed = find_game_installation();
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

	return GE_Quit();
}
