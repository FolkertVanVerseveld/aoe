#include <smt/smt.h>
#include "todo.h"
#include "gfx.h"

int direct_draw_init(struct video_mode *this, unsigned hInst, unsigned window, struct pal_entry *palette, char opt0, char opt1, int width, int height, int sys_memmap)
{
	int ret;
	stub
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
