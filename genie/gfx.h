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

void genie_gfx_draw_text(GLfloat x, GLfloat y, const char *str);

#endif
