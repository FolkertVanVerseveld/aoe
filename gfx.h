#ifndef AOE_GFX_H
#define AOE_GFX_H

struct pal_entry {
	uint8_t r, g, b, flags;
};

extern struct pal_entry game_pal[256];

struct video_mode {
	unsigned hInst;
	unsigned window;
	struct pal_entry *palette;
	int sys_memmap;
	unsigned width, height;
	unsigned state;
	int no_fullscreen;
};

struct video_mode *video_mode_init(struct video_mode *this);
int direct_draw_init(struct video_mode *this, unsigned hInst, unsigned window, struct pal_entry *palette, char opt0, char opt1, int width, int height, int sys_memmap);
void update_palette(struct pal_entry *tbl, unsigned start, unsigned n, struct pal_entry *src);

#endif
