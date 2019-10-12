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

// Default small text font
extern TTF_Font *fnt_default;
// Default user interface button font
extern TTF_Font *fnt_button;
// Default menu heading font
extern TTF_Font *fnt_large;

void gfx_init(void);
void gfx_free(void);
void gfx_update(void);

#ifdef __cplusplus
}
#endif

#endif
