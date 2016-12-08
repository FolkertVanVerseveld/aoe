#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <GL/gl.h>
#include <SDL2/SDL_image.h>
#include "engine.h"
#include "menu.h"
#include "todo.h"
#include "../dbg.h"
#include "../genie/dmap.h"
#include "memmap.h"
#include "gfx.h"
#include "game.h"

#define INIT_IMG 1
#define INIT_TEX 2
#define INIT_GFX 4

static int init = 0;

int enum_display_modes(void *arg, int (*cmp)(struct display*, void*))
{
	int count;
	struct display scr;
	count = SDL_GetNumVideoDisplays();
	for (int i = 0; i < count; ++i) {
		int m = SDL_GetNumDisplayModes(i);
		for (int j = 0; j < m; ++j) {
			SDL_DisplayMode dmode = {SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0};
			if (SDL_GetDisplayMode(i, j, &dmode))
				continue;
			scr.bitdepth  = SDL_BITSPERPIXEL(dmode.format);
			scr.width     = dmode.w;
			scr.height    = dmode.h;
			scr.frequency = dmode.refresh_rate;
			cmp(&scr, arg);
			if (scr.bitdepth > 16) {
				// also fake 16 and 8 bit modes
				scr.frequency = 16;
				cmp(&scr, arg);
				scr.frequency = 8;
				cmp(&scr, arg);
			}
		}
	}
	return count == 0;
}

static int parse_video_modes(struct display *display, void *arg)
{
	struct video_mode *mode = arg;
	unsigned bits, w, h;
	bits = display->bitdepth;
	w = display->width;
	h = display->height;
	if (bits == 8) {
		if (w == 640 && h == 480)
			mode->mode8_640_480 = 1;
		else if (w == 800 && h == 600)
			mode->mode8_800_600 = 1;
		else if (w == 1024 && h == 768)
			mode->mode8_1024_768 = 1;
		else if (w == 1280 && h == 1024)
			mode->mode8_1280_1024 = 1;
		else if (w == 320 && h == 240)
			mode->mode8_320_240 = 1;
		else if (w == 320 && h == 200)
			mode->mode8_320_200 = 1;
	} else if (bits == 16) {
		if (w == 320 && h == 200)
			mode->mode16_320_200 = 1;
		else if (w == 320 && h == 240)
			mode->mode16_320_240 = 1;
		else if (w == 640 && h == 480)
			mode->mode16_640_480 = 1;
		else if (w == 800 && h == 600)
			mode->mode16_800_600 = 1;
		else if (w == 1024 && h == 768)
			mode->mode16_1024_768 = 1;
	}
	return 1;
}

unsigned video_mode_fetch_bounds(struct video_mode *this, int query_interface)
{
	if (query_interface)
		return enum_display_modes(this, parse_video_modes);
	SDL_Rect bounds;
	unsigned display;
	if (get_display(this->window, &display))
		return 0;
	SDL_GetDisplayBounds(display, &bounds);
	if (bounds.w >= 800)
		this->mode8_800_600 = 1;
	if (bounds.w >= 1024)
		this->mode8_1024_768 = 1;
	if (bounds.w >= 1280)
		this->mode8_1280_1024 = 1;
	return 1;
}

static int video_mode_map_init(struct video_mode *this)
{
	if (this->byte478 == 2) {
		if (!this->ddrawsurf) {
			this->ddrawsurf = 1;
			if (this->no_fullscreen) {
				glClearColor(0, 0, 0, 0);
				glClear(GL_COLOR_BUFFER_BIT);
			}
		}
		if (!this->map) {
			struct map *pmap, *map = new(sizeof(struct map));
			pmap = map ? map_init(map, "Primary Surface", 0) : NULL;
			this->map = pmap;
			if (!pmap)
				return 0;
			if (this->no_fullscreen == 1) {
				int w, h;
				SDL_GetWindowSize(this->window, &w, &h);
				if (!map_blit(this->map, this, w, h, 0, 1))
					return 0;
			} else if (!map_blit(this->map, this, this->width, this->height, 0, 1))
				return 0;
		}
	}
	halt();
	return 1;
}

int direct_draw_init(struct video_mode *this, SDL_Window *hInst, SDL_Window *window, struct pal_entry *palette, char opt0, char opt1, int width, int height, int sys_memmap)
{
	int ret;
	stub
	(void)opt0;
	(void)opt1;
	this->window = window;
	this->palette = palette;
	this->hInst = hInst;
	this->width = width;
	this->height = height;
	this->sys_memmap = sys_memmap;
	if (opt0 == 1)
		return 0;
	video_mode_fetch_bounds(this, 1);
	if (this->no_fullscreen == 1) {
		// TODO get display mode
		unsigned display;
		if (get_display(window, &display)) {
			fputs("ddraw: bad display\n", stderr);
			this->state = 1;
			return 0;
		}
	} else {
		if ((ret = go_fullscreen(this->window)) != 0) {
			fprintf(stderr, "%s: no fullscreen available: error=%d\n", __func__, ret);
			this->state = 1;
			return 0;
		}
		this->mode0 = 1;
	}
	if (!video_mode_map_init(this)) {
		fputs("ddraw: map error\n", stderr);
		this->state = 1;
		return 0;
	}
	return 1;
}

struct video_mode *video_mode_init(struct video_mode *this)
{
	stub
	this->window = NULL;
	this->palette = NULL;
	this->state = 0;
	this->no_fullscreen = 0;
	this->width = this->height = 0;
	this->sys_memmap = 0;
	this->state = 0;
	return this;
}

struct video_mode *video_mode_start_init(struct video_mode *this, const char *title, int a3, const char *a4, int a5)
{
	stub
	struct menu_ctl *v5 = (struct menu_ctl*)this;
	(void)a3;
	(void)a4;
	(void)a5;
	menu_ctl_init_title(v5, title);
	halt();
	return NULL;
}

void update_palette(struct pal_entry *tbl, unsigned start, unsigned n, struct pal_entry *src)
{
	if (start + n >= 256) {
		fprintf(stderr, "bad palette range: [%u,%u)\n", start, start + n);
		return;
	}
	for (unsigned i_src = 0, i_dest = start; i_src < n; ++i_src, ++i_dest) {
		tbl[i_dest].r = src[i_src].r;
		tbl[i_dest].g = src[i_src].g;
		tbl[i_dest].b = src[i_src].b;
		tbl[i_dest].flags = src[i_src].flags;
	}
}

static void palette_apply(struct pal_entry *pal)
{
	stub
	update_palette(win_pal, 0, 256, pal);
}

struct pal_entry *drs_palette(char *pal_fname, int res_id, int a3)
{
	if (res_id == -1)
		return 0;
	size_t count;
	off_t offset;
	struct stat st;
	if (!stat(pal_fname, &st))
		fprintf(stderr, "unsupported: dynamic drs \"%s\"\n", pal_fname);
	char *drs_item = drs_get_item(DT_BINARY, res_id, &count, &offset);
	char *palette = drs_item;
	const char *j_hdr = "JASC-PAL\r\n0100\r\n", *line = palette;
	struct pal_entry *pal = NULL;
	if (memcmp(palette, j_hdr, 16))
		goto fail;
	unsigned n;
	line = palette + 16;
	if (sscanf(line, "%u", &n) != 1)
		goto fail;
	if (n > 256) {
		fprintf(stderr, "overflow: palette entries=%u (max: 256)\n", n);
		halt();
	}
	const char *next = strchr(line, '\n');
	if (!next) goto fail;
	line = next + 1;
	pal = new(n * sizeof(struct pal_entry));
	if (!pal) {
		fputs("out of memory\n", stderr);
		halt();
	}
	for (unsigned i = 0; i < n; ++i) {
		unsigned r, g, b;
		if (sscanf(line, "%u %u %u", &r, &g, &b) != 3)
			goto fail;
		struct pal_entry *dst = &pal[i];
		dst->r = r; dst->g = g; dst->b = b;
		next = strchr(line, '\n');
		if (!next) goto fail;
		line = next + 1;
	}
	if (a3) {
		struct video_mode *mode = game_ref->mode;
		if (mode->byte478 == 1 || mode->no_fullscreen == 1)
			palette_apply(pal);
	}
	return pal;
fail:
	fputs("bad palette\n", stderr);
	hexdump(line, 16);
	if (pal)
		delete(pal);
	return pal;
}

int vmode_map_blit(struct video_mode *this, SDL_Window *window, int a3, int spurious, int a5)
{
	stub
	(void)this;
	(void)window;
	(void)a3;
	(void)spurious;
	(void)a5;
	return 0;
}

void gfx_free(void)
{
	if (init & INIT_TEX) {
		glDeleteTextures(1, &tex);
		tex = 0;
		init &= ~INIT_TEX;
	}
	if (init & INIT_IMG) {
		IMG_Quit();
		init &= ~INIT_IMG;
	}
	init &= ~INIT_GFX;
}

int gfx_init(void)
{
	int ret = 1, flags = IMG_INIT_PNG;
	SDL_Surface *surf = NULL;
	if ((IMG_Init(flags) & flags) != flags)
		goto img_error;
	surf = IMG_Load("font.png");
	if (!surf) {
img_error:
		show_error("IMG failed", IMG_GetError());
		goto fail;
	}
	if (surf->format->palette) {
		show_error("Bad font", "Color palette not supported");
		goto fail;
	}
	if (surf->w != FONT_WIDTH || surf->h != FONT_HEIGHT) {
		char buf[256];
		snprintf(buf, sizeof buf, "Dimensions should be (%d,%d), but got (%d,%d)\n", FONT_WIDTH, FONT_HEIGHT, surf->w, surf->h);
		show_error("Bad font", buf);
		goto fail;
	}
	GLenum format = (!surf->format->Amask || surf->format->BitsPerPixel == 24) ? GL_RGB : GL_RGBA;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, format, surf->w, surf->h, 0, format, GL_UNSIGNED_BYTE, surf->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	spr_w = surf->w;
	spr_h = surf->h;
	ret = 0;
fail:
	if (surf)
		SDL_FreeSurface(surf);
	return ret;
}

#define TW 16
#define TH 16

void draw_str(GLfloat x, GLfloat y, const char *str)
{
	GLfloat tx0, tx1, ty0, ty1, xp = x;
	GLfloat vx0, vx1, vy0, vy1;
	unsigned gx, gy;
	int ch;
	for (; *str; ++str, x += FONT_GW) {
		ch = *str;
		while (ch == '\n') {
			x = xp;
			y += FONT_GH;
			ch = *++str;
		}
		if (!ch) break;
		gy = ch / TW;
		gx = ch % TH;
		tx0 = 1.0f * gx / TW;
		tx1 = 1.0f * (gx + 1) / TW;
		ty0 = 1.0f * gy / TH;
		ty1 = 1.0f * (gy + 1) / TH;
		vx0 = x; vx1 = x + FONT_GW;
		vy0 = y; vy1 = y + FONT_GH;
		glTexCoord2f(tx0, ty0); glVertex2f(vx0, vy0);
		glTexCoord2f(tx1, ty0); glVertex2f(vx1, vy0);
		glTexCoord2f(tx1, ty1); glVertex2f(vx1, vy1);
		glTexCoord2f(tx0, ty1); glVertex2f(vx0, vy1);
	}
}
