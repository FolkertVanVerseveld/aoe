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

void genie_ttf_free(void);
int genie_ttf_init(void);

#endif
