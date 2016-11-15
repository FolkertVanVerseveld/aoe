#include "engine.h"

#include <stddef.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <strings.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <GL/gl.h>
#include "dbg.h"
#include "empx.h"
#include "dmap.h"
#include "gfx.h"
#include "xmap.h"
#include "todo.h"

int fd_langx = -1, fd_lang = -1;
char *data_langx = NULL, *data_lang = NULL;
size_t size_langx = 0, size_lang = 0;

#define INIT_SDL 1
#define INIT_ENG 2

static int init = 0;
static SDL_Window *win = NULL;
static SDL_GLContext gl = NULL;

#define CFG_WIDTH 400
#define CFG_HEIGHT 300

struct res {
	unsigned w, h;
} resfixed[] = {
	{640, 480},
	{800, 600},
	{1024, 768},
	{1280, 1024},
};

int findfirst(const char *fname)
{
	DIR *d = NULL;
	struct dirent *entry;
	int found = -1;
	d = opendir(".");
	if (!d) goto fail;
	while ((entry = readdir(d)) != NULL)
		if (!strcasecmp(entry->d_name, fname)) {
			found = 0;
			break;
		}
fail:
	if (d) closedir(d);
	return found;
}

void eng_free(void)
{
	drs_free();
	if (gl) {
		SDL_GL_DeleteContext(gl);
		gl = NULL;
	}
	if (win) {
		SDL_DestroyWindow(win);
		win = NULL;
	}
	if (fd_langx != -1) {
		xunmap(fd_langx, data_langx, size_langx);
		fd_langx = -1;
	}
	if (fd_lang != -1) {
		xunmap(fd_lang, data_lang, size_lang);
		fd_lang = -1;
	}
	gfx_free();
	if (init & INIT_SDL) {
		SDL_Quit();
		init &= ~INIT_SDL;
	}
	config_free();
}

static int envcheck(char *buf)
{
	struct stat st;
	const char *data[] = {DRS_SFX, DRS_GFX, DRS_MAP, DRS_BORDER};
	const char *xdata[] = {DRS_SFX, DRS_GFX, DRS_UI};
	for (unsigned i = 0; i < 4; ++i) {
		sprintf(buf, "%s%s", DRS_DATA, data[i]);
		if (stat(buf, &st))
			return 1;
	}
	for (unsigned i = 0; i < 3; ++i) {
		sprintf(buf, "%s%s", DRS_XDATA, xdata[i]);
		if (stat(buf, &st))
			return 1;
	}
	strcpy(buf, "language.dll");
	if (stat(buf, &st))
		return 1;
	strcpy(buf, "languagex.dll");
	if (stat(buf, &st))
		return 1;
	return 0;
}

static int scr_init(unsigned *nscr, SDL_Rect **bnds)
{
	SDL_Rect *ptr = NULL;
	int d, ret = 1;
	d = SDL_GetNumVideoDisplays();
	*nscr = d < 0 ? 0 : d;
	if (!*nscr) {
		fputs("no display found\n", stderr);
		goto fail;
	}
	if (!(ptr = realloc(*bnds, *nscr * sizeof(SDL_Rect)))) {
		fputs("out of memory\n", stderr);
		goto fail;
	}
	*bnds = ptr;
	for (unsigned i = 0; i < *nscr; ++i)
		if (SDL_GetDisplayBounds(i, &(*bnds)[i])) {
			fprintf(stderr, "Display error: %s\n", SDL_GetError());
			goto fail;
		}
	ret = 0;
fail:
	return ret;
}

#define MODESZ 64
static unsigned nmode;
static SDL_DisplayMode modes[MODESZ];

// XXX fails if display count has changed since scr_init
// We cannot fix this since SDL2 does not recognize this.
static int scr_fetch_modes(unsigned scr)
{
	int d, ret = 1;
	d = SDL_GetNumDisplayModes(scr);
	nmode = d < 1 ? 1 : d;
	if (d < 1) {
		show_error("Display mode error", "Bad display");
		goto fail;
	}
	unsigned max = MODESZ;
	if (max > nmode) max = nmode;
	for (unsigned i = 0; i < max; ++i)
		if (SDL_GetDisplayMode(scr, i, &modes[i])) {
			show_error("Display mode error", SDL_GetError());
			goto fail;
		}
	ret = 0;
fail:
	return ret;
}

static int scr_apply(unsigned display, unsigned flags, SDL_DisplayMode *mode)
{
	char buf[256];
	SDL_Rect bnds;
	int d, ret = 1;
	Uint32 mask;
	d = SDL_GetNumVideoDisplays();
	if (d < 1 || (int)display > d) {
		show_error("Engine failed", "Display disappeared");
		goto fail;
	}
	if (SDL_GetDisplayBounds(display, &bnds))
		goto screrr;
	SDL_SetWindowPosition(win, bnds.x, bnds.y);
	SDL_SetWindowSize(win, mode->w, mode->h);
	mask = flags & CFG_FULL ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
	SDL_SetWindowFullscreen(win, mask);
	if ((SDL_GetWindowFlags(win) & mask) != mask) {
screrr:
		snprintf(buf, sizeof buf, "Display error: %s", SDL_GetError());
		show_error("Engine failed", buf);
		goto fail;
	}
	if (!(flags & CFG_FULL)) {
		int cx, cy;
		cx = bnds.x + (bnds.w - mode->w) / 2;
		cy = bnds.y + (bnds.h - mode->h) / 2;
		SDL_SetWindowPosition(win, cx, cy);
	}
	ret = 0;
fail:
	return ret;
}

static int cfg_loop(void)
{
	unsigned nscr, mode = 0, display = 0;
	int ret = 1;
	SDL_Rect *bnds = NULL;
	SDL_DisplayMode *cfg_mode = NULL;
	if (scr_init(&nscr, &bnds)) {
		show_error("Display failed", "Cannot fetch dimensions");
		goto fail;
	}
	if (scr_fetch_modes(display))
		goto fail;
	unsigned shift = 0, ymax = CFG_HEIGHT / FONT_GH - 8;
	unsigned flags = reg_cfg.flags & CFG_FULL;
	while (1) {
		SDL_Event ev;
		char buf[256];
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				exit(0);
			case SDL_KEYDOWN:
				switch (ev.key.keysym.sym) {
				case 'h':
				case SDLK_LEFT:
					display = (display + nscr - 1) % nscr;
					if (scr_fetch_modes(display))
						goto fail;
					if (mode >= nmode)
						shift = mode = 0;
					break;
				case 'l':
				case SDLK_RIGHT:
					display = (display + 1) % nscr;
					if (scr_fetch_modes(display))
						goto fail;
					if (mode >= nmode)
						shift = mode = 0;
					break;
				case 'j':
				case SDLK_DOWN:
					mode = (mode + 1) % nmode;
					break;
				case 'k':
				case SDLK_UP:
					mode = (mode + nmode - 1) % nmode;
					break;
				case 27:
				case '\r':
				case '\n':
				case ' ':
					printf("%4dx%4d %dHz\n", modes[mode].w, modes[mode].h, modes[mode].refresh_rate);
					goto end;
				}
				break;
			case SDL_KEYUP:
				switch (ev.key.keysym.sym) {
				case 'f':
					flags ^= CFG_FULL;
					break;
				}
				break;
			}
		}
		if (shift > mode)
			shift = mode;
		if (mode - shift >= ymax)
			shift = mode - ymax;
		int w, h;
		SDL_GetWindowSize(win, &w, &h);
		glViewport(0, 0, w, h);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, CFG_WIDTH, CFG_HEIGHT, 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glBindTexture(GL_TEXTURE_2D, tex);
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		draw_str(
			3 * FONT_GW, FONT_GH,
			"Age of Empires screen setup\n"
		);
		snprintf(buf, sizeof buf, "Fullscreen: %-3s (toggle with f)\nDisplay: \x11%u\x10 of %u (max %dx%d)\n", flags & CFG_FULL ? "yes" : "no", display + 1, nscr, bnds[display].w, bnds[display].h);
		draw_str(3 * FONT_GW, 2 * FONT_GH, buf);
		unsigned max = nmode <= shift + ymax + 1 ? nmode : shift + ymax + 1;
		for (unsigned i = shift; i < max; ++i) {
			SDL_DisplayMode *m = &modes[i];
			snprintf(buf, sizeof buf, "%4dx%4d %dHz", m->w, m->h, m->refresh_rate);
			draw_str(6 * FONT_GW, (5 + i - shift) * FONT_GH, buf);
		}
		draw_str(4 * FONT_GW, (5 + mode - shift) * FONT_GH, "\x10");
		draw_str(3 * FONT_GW, FONT_GW * (CFG_HEIGHT - 3 * FONT_GW) / FONT_GW, "Choose your resolution and press ENTER");
		glEnd();
		SDL_GL_SwapWindow(win);
	}
end:
	cfg_mode = &modes[mode];
	reg_cfg.display = display;
	reg_cfg.screen_size = reg_cfg.width = cfg_mode->w;
	reg_cfg.height = cfg_mode->h;
	reg_cfg.flags = flags;
	scr_apply(display, flags, cfg_mode);
	ret = 0;
fail:
	if (bnds)
		free(bnds);
	return ret;
}

int eng_init(char **path)
{
	int ret = 1;
	if (!config_init()) {
		fputs("config_init failed\n", stderr);
		goto fail;
	}
	if (!config_load(path))
		fputs("bad configuration settings\n", stderr);
	if (SDL_Init(SDL_INIT_VIDEO))
		goto sdl_error;
	init |= INIT_SDL;
	if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1)) {
sdl_error:
		show_error("SDL failed", SDL_GetError());
		goto fail;
	}
	win = SDL_CreateWindow("AoE", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, CFG_WIDTH, CFG_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (!win) {
		show_error("Engine failed", "no window");
		goto fail;
	}
	gl = SDL_GL_CreateContext(win);
	if (!gl) {
		show_error("Engine failed", "no OpenGL");
		goto fail;
	}
	if (gfx_init())
		goto fail;
	int set = 0;
	char *newpath = NULL, *oldpath = "/usr/local/games";
	while (1) {
		if (!*path) {
			// ask for directory
			newpath = prompt_folder("AoE gamedata directory", oldpath);
			if (!newpath) {
				if (set)
					show_error("Engine failed", "Invalid AoE gamedata directory");
				exit(1);
			}
		} else
			newpath = *path;
		if (!chdir(newpath)) {
			char envbuf[80];
			oldpath = newpath;
			if (envcheck(envbuf)) {
				show_error("Engine failed", "bad runtime path or missing files");
				continue;
			}
			if (*path != newpath)
				*path = strdup(newpath);
			break;
		}
		char buf[256];
		snprintf(buf, sizeof buf, "%s: %s\n", newpath, strerror(errno));
		show_error("Bad AoE gamedata directory", buf);
		if (newpath && *path) {
			free(*path);
			*path = NULL;
			set = 1;
		}
	}
	dbgf("set root: %s (%s)\n", *path, newpath);
	reg_cfg.root_path = *path;
	cfg.window = win;
	cfg.gl = gl;
	init |= INIT_ENG;
	if (cfg_loop()) goto fail;
	#define mapdrs(a,b,c) if(read_data_mapping(a,b,c)) goto fail;
	mapdrs(DRS_SFX   , DRS_XDATA, 1);
	mapdrs(DRS_GFX   , DRS_XDATA, 0);
	mapdrs(DRS_UI    , DRS_XDATA, 0);
	mapdrs(DRS_SFX   , DRS_DATA , 1);
	mapdrs(DRS_GFX   , DRS_DATA , 0);
	mapdrs(DRS_MAP   , DRS_DATA , 0);
	mapdrs(DRS_BORDER, DRS_DATA , 0);
	mapdrs(DRS_UI    , DRS_DATA , 0);
	ret = 0;
fail:
	return ret;
}

void show_error(const char *title, const char *msg)
{
	if (title)
		fprintf(stderr, "%s: %s\n", title, msg);
	else
		fprintf(stderr, "%s\n", msg);
	if (init || !isatty(fileno(stdin)))
		show_message(title, msg, BTN_OK, MSG_ERR, BTN_OK);
}

int get_display(SDL_Window *scr, unsigned *display)
{
	int i, n, x, y, w, h;
	SDL_Rect bounds;
	SDL_GetWindowSize(scr, &w, &h);
	SDL_GetWindowPosition(scr, &x, &y);
	for (i = 0, n = SDL_GetNumVideoDisplays(); i < n; ++i) {
		SDL_GetDisplayBounds(i, &bounds);
		if (x >= bounds.x && y >= bounds.y && x < bounds.x + bounds.w && y < bounds.y + bounds.h) {
			*display = i;
			return 0;
		}
	}
	return 1;
}

int go_fullscreen(SDL_Window *scr)
{
	SDL_Rect bounds;
	unsigned display;
	if (get_display(scr, &display))
		return 1;
	SDL_GetDisplayBounds(display, &bounds);
	SDL_SetWindowBordered(scr, SDL_FALSE);
	SDL_SetWindowPosition(scr, bounds.x, bounds.y);
	SDL_SetWindowSize(scr, bounds.w, bounds.h);
	return 0;
}

struct game_drive *game_drive_init(struct game_drive *this)
{
	int driveno = 1;
	this->driveno = this->driveno2 = driveno;
	strcpy(this->buf, "Z:/");
	strcpy(this->buf2, "Z:/");
	return this;
}

#define OPT_MAIN_SINGLE 1
#define OPT_MAIN_MULTI 2
#define OPT_MAIN_HELP 3
#define OPT_MAIN_EDITOR 4
#define OPT_MAIN_EXIT 5
#define OPT_MAINSZ 5

int eng_main(void)
{
	unsigned rmode, rw, rh;
	const char *opts[] = {
		"Single Player (not implemented)",
		"Multiplayer (not implemented)",
		"Help (not implemented)",
		"Scenario Builder (not implemented)",
		"Exit (not implemented)"
	};
	rmode = reg_cfg.mode_fixed;
	rw = resfixed[rmode].w;
	rh = resfixed[rmode].h;
	while (1) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				goto end;
			}
		}
		int x, y, w, h;
		SDL_GetWindowSize(win, &w, &h);
		while (rw > w || rh > h) {
			if (!rmode)
				break;
			--rmode;
			rw = resfixed[rmode].w;
			rh = resfixed[rmode].h;
		}
		x = (w - (int)rw) / 2;
		y = (h - (int)rh) / 2;
		glViewport(x, y, rw, rh);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, rw, rh, 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glBindTexture(GL_TEXTURE_2D, tex);
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glColor3f(1, 1, 1);
		draw_str(
			(rw - strlen("Age of Empires") * FONT_GW) / 2, FONT_GH,
			"Age of Empires"
		);
		unsigned opty = 4 * FONT_GH;
		for (unsigned i = 0; i < OPT_MAINSZ; ++i) {
			draw_str(
				(rw - strlen(opts[i]) * FONT_GW) / 2,
				opty, opts[i]
			);
			opty += 2 * FONT_GH;
		}
		glEnd();
		SDL_GL_SwapWindow(win);
	}
end:
	return 0;
}
