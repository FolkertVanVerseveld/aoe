#include <stdio.h>
#include <smt/smt.h>
#include "engine.h"
#include "menu.h"
#include "todo.h"
#include "dbg.h"
#include "gfx.h"

int enum_display_modes(void *arg, int (*cmp)(struct display*, void*))
{
	unsigned count, w, h;
	int x, y;
	struct display scr;
	count = smtDisplayCount();
	for (unsigned i = 0; i < count; ++i) {
		smtDisplayBounds(i, &x, &y, &w, &h);
		struct smtMode mode;
		unsigned m = smtModeCount(i);
		for (unsigned j = 0; j < m; ++j) {
			if (smtModeBounds(i, j, &mode))
				continue;
			scr.bitdepth = mode.bitdepth;
			scr.width = mode.width;
			scr.height = mode.height;
			scr.frequency = mode.frequency;
			cmp(&scr, arg);
			if (mode.bitdepth > 16) {
				// also fake 16 and 8 bit modes
				mode.frequency = 16;
				cmp(&scr, arg);
				mode.frequency = 8;
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

int direct_draw_init(struct video_mode *this, unsigned hInst, unsigned window, struct pal_entry *palette, char opt0, char opt1, int width, int height, int sys_memmap)
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
		ret = smtMode(this->window, SMT_WIN_FULL_FAKE);
		if (ret) {
			fprintf(stderr, "%s: no fullscreen available: smt error=%u\n", __func__, ret);
			this->state = 1;
			return 0;
		}
	}
	return 1;
}

struct video_mode *video_mode_init(struct video_mode *this)
{
	stub
	this->window = SMT_RES_INVALID;
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