#ifndef AOE_MAP_H
#define AOE_MAP_H

#include "gfx.h"
#include "view.h"
#include <SDL2/SDL.h>

struct map;

struct map_0 {
	void *ptr14;
};

struct map_blit {
	struct map *map;
	struct map_blit *end;
	struct map_blit *next;
};

#define MAPBLKSZ 108

struct map {
	struct video_mode *vmode;
	SDL_Window *window;
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
	struct map_blit *blit;
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
	unsigned blit_y;
	unsigned numF4;
	unsigned numF8;
	char chFC;
	char padFD[3];
};

struct map *map_init(struct map *this, const char *map_name, int a2);
void map_free(struct map *this);
int map_blit(struct map *this, struct video_mode *vmode, int w, int h, int x, int y);

#endif
