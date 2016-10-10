#include "map.h"
#include "dbg.h"
#include "todo.h"
#include <stdlib.h>
#include <string.h>

struct map *map_init(struct map *this, const char *map_name, int a2)
{
	this->obj0 = NULL;
	this->num4 = 0;
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
	this->numAC = 0;
	this->numF4 = 0;
	this->numF0 = 0;
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
	if (this->ptrAC) {
		if (this->obj0 && this->ptrAC == this->obj0->ptr14)
			this->obj0->ptr14 = NULL;
		free(this->ptrAC);
		this->ptrAC = NULL;
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
