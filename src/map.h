#ifndef AOE_MAP_H
#define AOE_MAP_H

#include "gfx.h"
#include "view.h"

struct map_0 {
	void *ptr14;
};

#define MAPBLKSZ 108

struct map {
	struct map_0 *obj0;
	unsigned num4;
	unsigned num8;
	void *ptrC;
	// REMAP (HGDIOBJ gfx_object) == unsigned
	unsigned gfx_object;
	// REMAP (HGDIOBJ gfx_sel_object) == unsigned
	unsigned gfx_sel_object;
	unsigned rect_w;
	unsigned rect_h;
	// REMAP (void *num20) == unsigned
	unsigned num20;
	unsigned num24;
	struct rect rect;
	// REMAP (HDC hdc38) == unsigned
	unsigned hdc38;
	unsigned num3C;
	char blk40[MAPBLKSZ];
	void *ptr50;
	char blk54[84];
	void *ptrA8;
	void *ptrAC;
	char *name;
	unsigned numB4;
	unsigned numB8;
	void *ptrBC;
	void *ptrC0;
	void *ptrC4;
	unsigned tblC8[3];
	unsigned numD4;
	unsigned numD8;
	unsigned numDC;
	unsigned numE0;
	unsigned numE4;
	unsigned numE8;
	unsigned numEC;
	unsigned numF0;
	unsigned numF4;
	unsigned numF8;
	char chFC;
	char padFD[3];
};

struct map *map_init(struct map *this, const char *map_name, int a2);
void map_free(struct map *this);

#endif
