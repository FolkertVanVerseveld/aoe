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
static SDL_GLContext *gl = NULL;

int findfirst(const char *fname)
{
	DIR *d = NULL;
	struct dirent *entry;
	int found = -1;
	d = opendir(".");
	if (!d) goto fail;
	while ((entry = readdir(d)) != NULL) {
		if (!strcasecmp(entry->d_name, fname)) {
			found = 0;
			break;
		}
	}
fail:
	if (d) closedir(d);
	return found;
}

void eng_free(void)
{
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
}

static int envcheck(char *buf)
{
	struct stat st;
	const char *data [] = {DRS_SFX, DRS_GFX, DRS_MAP, DRS_BORDER};
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

#define CFG_WIDTH 400
#define CFG_HEIGHT 300

#define MODESZ 64
static unsigned nmode;
static SDL_DisplayMode modes[MODESZ];

// FIXME fails if display count has changed since scr_init
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

static int cfg_loop(void)
{
	unsigned nscr, mode = 0, display = 0;
	int ret = 1;
	SDL_Rect *bnds = NULL;
	if (scr_init(&nscr, &bnds)) {
		show_error("Display failed", "Cannot fetch dimensions");
		goto fail;
	}
	if (scr_fetch_modes(display))
		goto fail;
	unsigned shift = 0, ymax = CFG_HEIGHT / FONT_GH - 8;
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
			}
		}
		if (shift > mode)
			shift = mode;
		if (mode - shift >= ymax)
			shift = mode - ymax;
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
		snprintf(buf, sizeof buf, "Display: \x11%u\x10 of %u (max %dx%d)\n", display + 1, nscr, bnds[display].w, bnds[display].h);
		draw_str(3 * FONT_GW, 3 * FONT_GH, buf);
		unsigned max = nmode <= shift + ymax + 1 ? nmode : shift + ymax + 1;
		for (unsigned i = shift; i < max; ++i) {
			SDL_DisplayMode *m = &modes[i];
			snprintf(buf, sizeof buf, "%4dx%4d %dHz", m->w, m->h, m->refresh_rate);
			draw_str(6 * FONT_GW, (5 + i - shift) * FONT_GH, buf);
		}
		draw_str(4 * FONT_GW, (5 + mode - shift) * FONT_GH, "\x10");
		draw_str(3 * FONT_GW, FONT_GW * (300 - 3 * FONT_GW) / FONT_GW, "Choose your resolution and press ENTER");
		glEnd();
		SDL_GL_SwapWindow(win);
	}
end:
	ret = 0;
fail:
	if (bnds)
		free(bnds);
	return ret;
}

int eng_init(const char *path)
{
	int ret = 1;
	if (SDL_Init(SDL_INIT_VIDEO))
		goto sdl_error;
	init |= INIT_SDL;
	if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1)) {
sdl_error:
		show_error("SDL failed", SDL_GetError());
		goto fail;
	}
	win = SDL_CreateWindow("AoE", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 400, 300, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
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
	char buf[256];
	if (path && chdir(path)) {
		snprintf(buf, sizeof buf, "%s: %s", path, strerror(errno));
		show_error("Engine failed", buf);
		goto fail;
	}
	if (envcheck(buf)) {
		perror(buf);
		show_error("Engine failed", "bad runtime path or missing files");
		goto fail;
	}
	cfg.window = win;
	cfg.gl = gl;
	init |= INIT_ENG;
	ret = cfg_loop();
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
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, msg, win);
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
