#include <stdio.h>
#include <smt/smt.h>
#include "todo.h"
#include "gfx.h"

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
