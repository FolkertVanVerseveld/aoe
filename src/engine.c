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

int eng_init(const char *path)
{
	int ret = 1;
	if (SDL_Init(SDL_INIT_VIDEO)) {
		show_error("SDL failed", SDL_GetError());
		goto fail;
	}
	init |= INIT_SDL;
	if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1)) {
		show_error("SDL failed", SDL_GetError());
		goto fail;
	}
	win = SDL_CreateWindow("AoE", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320, 240, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (!win) {
		show_error("Engine failed", "no window");
		goto fail;
	}
	gl = SDL_GL_CreateContext(win);
	if (!gl) {
		show_error("Engine failed", "no OpenGL");
		goto fail;
	}
	char buf[256];
	if (chdir(path)) {
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
	while (1) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				exit(0);
			case SDL_KEYDOWN:
				goto end;
			}
		}
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		SDL_GL_SwapWindow(win);
	}
end:
	ret = 0;
fail:
	return ret;
}

void show_error(const char *title, const char *msg)
{
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
