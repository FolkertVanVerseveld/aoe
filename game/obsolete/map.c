/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "map.h"
#include <genie/dbg.h>
#include <genie/todo.h>
#include <genie/memmap.h>
#include <stdlib.h>
#include <string.h>

struct map *map_init(struct map *this, const char *map_name, int a2)
{
	this->vmode = NULL;
	this->window = NULL;
	this->num3C = 0;
	this->numEC = 0;
	this->hdc38 = 0;
	this->ptrC = NULL;
	this->gfx_object = 0;
	this->gfx_sel_object = 0;
	this->num8 = 0;
	this->rect_w = 0;
	this->rect_h = 0;
	this->num20 = 0;
	this->numEC = 0;
	this->chFC = -1;
	this->numEC = 0;
	this->chFC = -1;
	this->blit = NULL;
	this->numF4 = 0;
	this->blit_y = 0;
	this->numE4 = a2;
	this->rect.left = 0;
	this->rect.top = 0;
	this->rect.right = 0;
	this->rect.bottom = 0;
	this->numB4 = 0;
	this->ptrBC = NULL;
	this->ptrC0 = NULL;
	this->numDC = 0;
	this->numF8 = 0;
	this->ptrC4 = NULL;
	this->numD4 = 0;
	this->numD8 = 0;
	this->numE0 = 0;
	memset(this->blk40, 0, MAPBLKSZ);
	this->ptrC = calloc(1u, 0x428u);
	this->num24 = 1;
	this->tblC8[1] = 0;
	this->tblC8[2] = 0;
	this->tblC8[0] = 0;
	this->name = *map_name ? strdup(map_name) : NULL;
	return this;
}

void map_free(struct map *this)
{
	stub
	if (this->blit) {
		if (this->vmode && this->blit == this->vmode->blit)
			this->vmode->blit = this->blit->next;
		delete(this->blit);
		this->blit = NULL;
	}
	if (this->vmode && this->vmode->byte478 == 1) {
		if (this->hdc38) {
			if (this->gfx_object) {
				if (this->gfx_sel_object)
					this->gfx_sel_object = 0;
				this->gfx_object = 0;
			}
			this->hdc38 = 0;
		}
	}
	if (this->ptrC)
		free(this->ptrC);
	if (this->ptrBC)
		free(this->ptrBC);
	if (this->ptrC4)
		free(this->ptrC4);
	if (this->name)
		free(this->name);
}

static int map_blit_no_y(struct map *this, int w, int h, int x)
{
	stub
	(void)this;
	(void)w;
	(void)h;
	(void)x;
	halt();
	return 0;
}

int map_blit(struct map *this, struct video_mode *vmode, int w, int h, int x, int y)
{
	stub
	this->vmode = vmode;
	this->blit_y = y;
	if (!vmode)
		goto no_vmode;
	struct map_blit *blit = this->blit;
	this->window = vmode->window;
	if (!blit) {
		this->blit = new(sizeof(struct map_blit));
		if (!this->blit)
			return 0;
		this->blit->end = NULL;
		this->blit->next = NULL;
		struct map_blit *b = this->vmode->blit;
		if (b) {
			struct map_blit *i;
			for (i = b->next; i; i = i->next)
				b = i;
			b->next = this->blit;
			this->blit->end = b;
		} else
			this->vmode->blit = this->blit;
	}
	if (this->vmode->byte478 == 1)
		return 0;
	else {
no_vmode:
		map_blit_no_y(this, w, h, x);
		return 1;
	}
}
