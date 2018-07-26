/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

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
#define FONT_PT_DEFAULT 14

#define FONT_NAME_BUTTON "coprgtl.ttf"
#define FONT_PT_BUTTON 18

#ifdef __cplusplus
extern "C" {
#endif

// Default small text font
extern TTF_Font *fnt_default;
// Default user interface button font
extern TTF_Font *fnt_button;

void gfx_init(void);
void gfx_free(void);

#ifdef __cplusplus
}
#endif

#endif
