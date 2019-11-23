/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "engine.h"

#include <genie/dbg.h>
#include <genie/def.h>
#include <genie/gfx.h>
#include <genie/res.h>
#include <genie/fs.h>
#include <genie/sfx.h>

#include <xt/error.h>
#include <xt/os.h>
#include <xt/string.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct pe_lib lib_lang;

struct ge_config ge_cfg = {GE_CFG_NORMAL_MOUSE, GE_CFG_MODE_800x600, 50, 0.7, 1, 0.3};

#define GE_INIT_SDL 1

extern SDL_Window *window;
extern SDL_Renderer *renderer;

extern unsigned music_index;
extern int in_game;
extern volatile int music_playing;

extern void idle(unsigned ms);
extern void display(void);
extern void repaint(void);

extern void keydown(SDL_KeyboardEvent *event);
extern void keyup(SDL_KeyboardEvent *event);

extern void mousedown(SDL_MouseButtonEvent *event);
extern void mouseup(SDL_MouseButtonEvent *event);

extern void mus_play(unsigned id);

struct _ge_state _ge_state;
struct ge_state ge_state;

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
		} else if (hasopt(argv, "midi", "music")) {
#if XT_IS_LINUX
			fputs("Midi support unavailable\n", stderr);
#endif
		} else if (!strcasecmp(argv[i], "msync")) {
			fputs("SoundBlaster AWE not supported\n", stderr);
		} else if (!strcasecmp(argv[i], "mfill")) {
			fputs("Matrox Video adapter not supported\n", stderr);
		} else if (hasopt(argv, "no", "sound")) {
			cfg->options |= GE_CFG_NO_SOUND;
		} else if (!strcmp(argv[i], "640")) {
			cfg->screen_mode = GE_CFG_MODE_640x480;
		} else if (!strcmp(argv[i], "800")) {
			cfg->screen_mode = GE_CFG_MODE_800x600;
		} else if (!strcmp(argv[i], "1024")) {
			cfg->screen_mode = GE_CFG_MODE_1024x768;
		/* Custom option: disable changing screen resolution. */
		} else if (hasopt(argv, "no", "changeres")) {
			cfg->options = GE_CFG_NATIVE_RESOLUTION;
		} else if (hasopt(argv, "no", "music")) {
			cfg->options |= GE_CFG_NO_MUSIC;
		} else if (hasopt(argv, "normal", "mouse")) {
			cfg->options |= GE_CFG_NORMAL_MOUSE;
		/* Rise of Rome options: */
		} else if (hasopt(argv, "no", "terrainsound")) {
			cfg->options |= GE_CFG_NO_AMBIENT;
		} else if (i + 1 < argc &&
			(!strcasecmp(argv[i], "limit") || !strcasecmp(argv[i], "limit=")))
		{
			int n = atoi(argv[i + 1]);

			if (n < GE_POP_MIN)
				n = GE_POP_MIN;
			else if (n > GE_POP_MAX)
				n = GE_POP_MAX;

			dbgf("pop cap changed to: %u\n", n);
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

extern int running;

#define DEFAULT_TICKS 16

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

	switch (ge_cfg.screen_mode) {
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
	int err, count, display;

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

void ge_main(void)
{
	SDL_Event ev;

	running = 1;
	display();

	_ge_state.timer = SDL_GetTicks();

	while (running) {
		while (SDL_PollEvent(&ev))
			handle_event(&ev);

		Uint32 next = SDL_GetTicks();
		idle(next >= _ge_state.timer ? next - _ge_state.timer : DEFAULT_TICKS);
		display();

		if (!music_playing && in_game) {
			music_index = (music_index + 1) % _ge_state.msc_count;
			mus_play(music_index + MUS_GAME);
		}

		_ge_state.timer = next;
	}
}

int ge_init(int *pargc, char **argv)
{
	int err = 1;
	int argc = *pargc;

	for (int i = 1; i < argc; ++i) {
		const char *arg = argv[i];

		if (!strcmp(arg, "--version") || !strcmp(arg, "-v"))
			puts("Genie Engine v0");

		if (!strcmp(arg, "--"))
			break;
	}

	_ge_cfg_parse(&ge_cfg, argc, argv);

	/* Ensure the minimal setup files are available. */
	if (!find_setup_files()) {
		show_error("Please insert or mount the game CD-ROM");
		goto fail;
	}

	if (load_lib_lang()) {
		show_error("CD-ROM files are corrupt");
		goto fail;
	}

	/*
	 * If the game has already been installed, we prefer loading game
	 * assets from the installation since it probably is a legitimate
	 * installation (or some files may have been modded, and we want to
	 * load them as well).
	 */
	game_installed = find_game_installation();
	if (has_wine)
		dbgs("wine detected");
	dbgf("game installed: %s\n", game_installed ? "yes" : "no");

	for (unsigned i = 0; ge_start_cfg.vfx[i]; ++i)
		ge_video_play(ge_start_cfg.vfx[i]);

	/* Setup graphical state */
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
		show_error("Could not initialize user interface");
		goto fail;
	}

	_ge_state.init |= GE_INIT_SDL;

	if (!(_ge_state.win = SDL_CreateWindow(
		ge_start_cfg.title, SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, gfx_cfg.width, gfx_cfg.height,
		SDL_WINDOW_SHOWN)))
	{
		show_error("Could not create user interface");
		goto fail;
	}

	dbgf("Available render drivers: %d\n", SDL_GetNumVideoDrivers());

	/* Create default renderer and don't care if it is accelerated. */
	if (!(_ge_state.ctx = SDL_CreateRenderer(_ge_state.win, -1, SDL_RENDERER_PRESENTVSYNC))) {
		show_error("Could not create rendering context");
		goto fail;
	}

	// FIXME refactor this to use _ge_state only
	window = _ge_state.win;
	renderer = _ge_state.ctx;

	for (unsigned i = 0; ge_start_cfg.msc[i]; ++i, ++_ge_state.msc_count)
		;

	err = 0;
fail:
	if (err)
		ge_quit();
	return err;
}

int ge_quit(void)
{
	if (_ge_state.init & GE_INIT_SDL) {
		if (_ge_state.ctx) {
			SDL_DestroyRenderer(_ge_state.ctx);
			renderer = NULL;
		}
		if (_ge_state.win) {
			SDL_DestroyWindow(_ge_state.win);
			window = NULL;
		}

		SDL_Quit();
		_ge_state.init &= ~GE_INIT_SDL;
	}

	if (_ge_state.init)
		fprintf(stderr, "%s: cleanup incomplete\n", __func__);

	pe_lib_close(&lib_lang);
	return 0;
}
