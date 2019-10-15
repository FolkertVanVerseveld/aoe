/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Graphics subsystem
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 * Graphical core of game engine that handles all low-level visual stuff
 */

#ifndef GFX_H
#define GFX_H

#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>

#define FONT_NAME_DEFAULT "arial.ttf"
#define FONT_PT_DEFAULT 13

#define FONT_NAME_BUTTON "coprgtl.ttf"
#define FONT_PT_BUTTON 18

#define FONT_NAME_LARGE "coprgtl.ttf"
#define FONT_PT_LARGE 28

#ifdef __cplusplus
extern "C" {
#endif

extern struct gfx_cfg {
	// main window state
	int display;
	int scr_x, scr_y;
	// viewport in main window
	unsigned width, height;
} gfx_cfg;

extern SDL_Texture *fnt_tex_default, *fnt_tex_default_bold, *fnt_tex_button, *fnt_tex_large;

void gfx_init(void);
void gfx_free(void);
void gfx_update(void);

enum Halign {
	LEFT,
	CENTER,
	RIGHT
};

enum Valign {
	TOP,
	MIDDLE,
	BOTTOM
};

/**
 * Precompute the boundaries of the specified text and always returns 0.
 */
int gfx_get_textlen_bounds(struct SDL_Texture *font, SDL_Rect *bounds, const char *text, unsigned count);

void gfx_draw_textlen(struct SDL_Texture *font, const SDL_Rect *pos, const char *text, unsigned count);
void gfx_draw_textlen_shadow(struct SDL_Texture *font, const SDL_Color *fg, const SDL_Color *bg, const SDL_Rect *pos, const char *text, unsigned count);

void gfx_draw_textlen_shadow_ext(struct SDL_Texture *font, const SDL_Color *fg, const SDL_Color *bg, const SDL_Rect *pos, const char *text, unsigned count, enum Halign ha, enum Valign va);

#ifdef __cplusplus
}
#endif

#endif
