#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "menu.h"
#include "todo.h"
#include "dbg.h"
#include "dmap.h"
#include "memmap.h"
#include "gfx.h"
#include "game.h"

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
	stub
	(void)this;
	if (query_interface)
		return enum_display_modes(this, parse_video_modes);
	halt();
	return 0;
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
	if (this->no_fullscreen == 1) {
		// TODO get display mode
	} else {
		if (go_fullscreen(this->window)) {
			fprintf(stderr, "%s: no fullscreen available: error=%u\n", __func__, ret);
			this->state = 1;
			return 0;
		}
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
	(void)a3;
	(void)a4;
	(void)a5;
	struct menu_ctl *v5 = (struct menu_ctl*)this;
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
	stub
	if (res_id == -1)
		return 0;
	size_t count, offset;
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
	halt();
}
