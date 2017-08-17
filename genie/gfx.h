/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef GENIE_GFX_H
#define GENIE_GFX_H

#include <GL/gl.h>

#define GENIE_FONT_WIDTH  144
#define GENIE_FONT_HEIGHT 256
#define GENIE_FONT_TILE_WIDTH  16
#define GENIE_FONT_TILE_HEIGHT 16
#define GENIE_GLYPH_WIDTH  (GENIE_FONT_WIDTH / GENIE_FONT_TILE_WIDTH)
#define GENIE_GLYPH_HEIGHT (GENIE_FONT_HEIGHT / GENIE_FONT_TILE_HEIGHT)

void genie_gfx_free(void);
int genie_gfx_init(void);

/**
 * Setup orthogonal projection and identity modelview matrix.
 */
void genie_gfx_setup_ortho(GLdouble width, GLdouble height);

void genie_gfx_clear_screen(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);

void genie_gfx_draw_text(int x, int y, const char *str);

#define GENIE_HA_LEFT 0
#define GENIE_HA_CENTER 1
#define GENIE_HA_RIGHT 2

#define GENIE_VA_TOP 0
#define GENIE_VA_MIDDLE 1
#define GENIE_VA_BOTTOM 2

#define GENIE_GFX_CACHE_SIZE 16

void genie_gfx_cache_clear(void);
unsigned genie_gfx_cache_text(unsigned ttf_id, const char *text);
void genie_gfx_put_text(unsigned slot, int x, int y, unsigned halign, unsigned valign);

#endif
