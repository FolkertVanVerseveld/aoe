/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Genie game engine true type font support
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 * SDL2 true type font (=TTF) support completely written from scratch.
 * This is an internal header and should not be included directly.
 */

#ifndef GENIE_TTF_H
#define GENIE_TTF_H

#include <SDL2/SDL_ttf.h>

#define GENIE_TTF_DEFAULT 0
#define GENIE_TTF_DEFAULT_BOLD 1
#define GENIE_TTF_COMIC 2
#define GENIE_TTF_COMIC_BOLD 3
#define GENIE_TTF_UI 4
#define GENIE_TTF_UI_BOLD 5

void genie_ttf_free(void);
int genie_ttf_init(void);

/* Render text to surface. This cannot fail. */
SDL_Surface *genie_ttf_render_solid(unsigned id, const char *text, SDL_Color color);

#endif
